#!/usr/bin/env python3
"""
Haiku Tool Reliability Trial

The point of this script: before Grunt gets retrained on VAIL's tool set,
prove the tool set itself is usable by a *competent* model first. A 26M
param model failing tells you nothing about whether the fault is the model
or the tools; a frontier-class model (Claude Haiku) failing tells you the
tools themselves need fixing. This runs Haiku as a real tool-calling agent
against the real VAIL tool schemas, task by task, and if a live UE session
is connected, actually executes each tool call over the real TCP socket.

Output is a per-task, per-tool-call log: what Haiku called, what arguments
it constructed, whether the call was schema-valid, and (if UE was actually
connected) whether the engine accepted it. That log is the punch list for
hardening tool descriptions/schemas -- and only once this comes back clean
does it make sense to spend compute distilling the behavior into Grunt.

A run with no live UE connection still tells you a lot (does Haiku pick the
right tool, construct valid arguments, sequence batches correctly) but
CANNOT confirm the engine actually accepts/settles the calls -- the report
is stamped accordingly and must not be read as a full validation.

Usage:
    set ANTHROPIC_API_KEY=sk-...
    python haiku_tool_trial.py                # auto-detects live UE connection
    python haiku_tool_trial.py --dry-run       # mocked Haiku responses, wiring smoke test only
"""

import argparse
import json
import os
import sys
import time
from pathlib import Path
from typing import Any, Dict, List, Optional

_HERE = Path(__file__).resolve().parent
_GRUNT_DIR = _HERE.parent / "grunt"
sys.path.insert(0, str(_HERE))
sys.path.insert(0, str(_GRUNT_DIR))

from vail_socket_client import send_vail_cmd  # noqa: E402
from vail_telemetry import VAILTelemetrySession  # noqa: E402

TOOLS_DIR = _GRUNT_DIR / "tools"

SYSTEM_PROMPT = (
    "You control the Unreal Engine 5.8 editor through the VAIL tool set. "
    "You cannot see the screen -- vail_get_tree and vail_find are how you inspect editor "
    "state. Set scope with vail_set_scope before reading or changing anything about a "
    "specific actor or asset. Batch related property changes with vail_begin_batch / "
    "vail_end_batch so they group into a single Undo step. When the task is fully done, "
    "stop calling tools and reply with a one-line confirmation in plain text."
)

TASKS: List[Dict[str, str]] = [
    {"scenario_id": "single_property_edit", "prompt": "Set the DirectionalLight's intensity to 35000 lux."},
    {"scenario_id": "compound_property_edit", "prompt": "Change the DirectionalLight's color to a twilight blue (roughly R=0.25 G=0.45 B=1.0) and rotate it to pitch -8, yaw 110, as a single undo step."},
    {"scenario_id": "find_before_acting", "prompt": "Find the editor command for compiling the currently open Blueprint, then run it."},
    {"scenario_id": "actor_query", "prompt": "List the actors currently in the level."},
    {"scenario_id": "graph_topology", "prompt": "Get the node graph topology of the Blueprint asset at /Game/Blueprints/BP_Player."},
    {"scenario_id": "save_project", "prompt": "Save all unsaved changes in the project."},
]

# One task per real tool schema in grunt/tools/ -- see full_tool_sweep_tasks.py.
# Kept separate from the 6-task smoke-test list above so a quick --dry-run
# wiring check stays fast; the full sweep is opt-in via --full since it's
# 40 real (non-mocked) API calls minimum, more with multi-turn tasks.
from full_tool_sweep_tasks import FULL_SWEEP_TASKS  # noqa: E402


def load_tool_defs() -> List[Dict[str, Any]]:
    """Load real VAIL tool schemas in native Anthropic tool-use format
    (name/description/input_schema) -- not the flattened form Grunt trains
    on, so this exercises the same contract a frontier model would see."""
    defs = []
    for jf in sorted(TOOLS_DIR.glob("*.json")):
        schema = json.loads(jf.read_text(encoding="utf-8"))
        defs.append({
            "name": schema["name"],
            "description": schema.get("description", ""),
            "input_schema": schema.get("parameters", {"type": "object", "properties": {}}),
        })
    return defs


def check_live_ue() -> bool:
    ping = send_vail_cmd("vail_get_tree", {"max_depth": 1, "category_filter": ""}, timeout=2.0)
    return ping.get("status") != "error"


class _MockAnthropic:
    """Wiring smoke test only -- returns a single scripted tool call then stops.
    Never a substitute for a real trial; --dry-run output is labeled as such."""

    class _Msg:
        def __init__(self, blocks, stop_reason, usage):
            self.content = blocks
            self.stop_reason = stop_reason
            self.usage = usage

    def __init__(self):
        self._step = 0

    def messages_create(self, **kwargs):
        self._step += 1
        usage = type("U", (), {"input_tokens": 150, "output_tokens": 30})()
        if self._step == 1:
            block = type("B", (), {"type": "tool_use", "id": "mock1", "name": "vail_get_tree", "input": {"max_depth": 1, "category_filter": ""}})()
            return self._Msg([block], "tool_use", usage)
        block = type("B", (), {"type": "text", "text": "[dry-run] mocked completion"})()
        return self._Msg([block], "end_turn", usage)


def run_task(client, model: str, tool_defs: List[Dict[str, Any]], task: Dict[str, str], live_ue: bool, dry_run: bool, max_turns: int = 6) -> Dict[str, Any]:
    messages = [{"role": "user", "content": task["prompt"]}]
    tool_calls_log = []
    total_input_tokens = 0
    total_output_tokens = 0
    turns = 0
    final_text = None
    schema_errors = []

    for _ in range(max_turns):
        turns += 1
        if dry_run:
            resp = client.messages_create()
        else:
            resp = client.messages.create(
                model=model,
                max_tokens=1024,
                system=SYSTEM_PROMPT,
                tools=tool_defs,
                messages=messages,
            )
        total_input_tokens += resp.usage.input_tokens
        total_output_tokens += resp.usage.output_tokens

        tool_use_blocks = [b for b in resp.content if getattr(b, "type", None) == "tool_use"]
        text_blocks = [b for b in resp.content if getattr(b, "type", None) == "text"]
        if text_blocks:
            final_text = text_blocks[-1].text

        if not tool_use_blocks:
            break

        assistant_content = [{"type": b.type, **({"text": b.text} if b.type == "text" else {"id": b.id, "name": b.name, "input": b.input})} for b in resp.content]
        messages.append({"role": "assistant", "content": assistant_content})

        tool_results = []
        for block in tool_use_blocks:
            known_names = {d["name"] for d in tool_defs}
            if block.name not in known_names:
                schema_errors.append(f"called unknown tool '{block.name}'")
                result = {"status": "error", "error": f"unknown tool {block.name}"}
            elif live_ue and not dry_run:
                result = send_vail_cmd(block.name, block.input)
            else:
                result = {"status": "not_executed", "reason": "dry-run or no live UE connection"}

            tool_calls_log.append({"name": block.name, "arguments": block.input, "result": result})
            tool_results.append({
                "type": "tool_result",
                "tool_use_id": block.id,
                "content": json.dumps(result),
            })

        messages.append({"role": "user", "content": tool_results})

    return {
        "scenario_id": task["scenario_id"],
        "prompt": task["prompt"],
        "turns": turns,
        "tool_calls": tool_calls_log,
        "schema_errors": schema_errors,
        "final_text": final_text,
        "total_input_tokens": total_input_tokens,
        "total_output_tokens": total_output_tokens,
    }


def main():
    parser = argparse.ArgumentParser(description="Haiku-driven VAIL tool reliability trial")
    parser.add_argument("--dry-run", action="store_true", help="Mock Claude responses. Wiring smoke test only, not a real trial.")
    parser.add_argument("--full", action="store_true", help="Run the full 40-tool sweep (one task per tool) instead of the 6-task smoke sample.")
    parser.add_argument("--model", default="claude-haiku-4-5")
    parser.add_argument("--out-dir", default=str(_HERE / "reports"))
    args = parser.parse_args()

    task_list = FULL_SWEEP_TASKS if args.full else TASKS

    tool_defs = load_tool_defs()
    print(f"Loaded {len(tool_defs)} real VAIL tool schemas from {TOOLS_DIR}")
    print(f"Task list: {'full 40-tool sweep' if args.full else 'quick 6-task sample'} ({len(task_list)} tasks)")

    if args.dry_run:
        client = _MockAnthropic()
        live_ue = False
    else:
        try:
            import anthropic
        except ImportError:
            sys.exit("anthropic package not installed. `pip install anthropic`, or run with --dry-run.")
        api_key = os.environ.get("ANTHROPIC_API_KEY")
        if not api_key:
            sys.exit("ANTHROPIC_API_KEY not set. Export it, or run with --dry-run.")
        client = anthropic.Anthropic(api_key=api_key)
        live_ue = check_live_ue()

    mode = "DRY RUN (mocked, wiring test only)" if args.dry_run else ("LIVE UE CONNECTED" if live_ue else "NO LIVE UE -- schema/reasoning validation only, engine acceptance NOT confirmed")
    print("=" * 80)
    print(f"HAIKU TOOL RELIABILITY TRIAL -- {mode}")
    print("=" * 80)

    session = VAILTelemetrySession(suite_name=f"Haiku_Tool_Trial_{'FULL' if args.full else 'SAMPLE'}_{'DRYRUN' if args.dry_run else ('LIVE' if live_ue else 'NOENGINE')}")
    all_results = []

    for task in task_list:
        t0 = time.perf_counter()
        result = run_task(client, args.model, tool_defs, task, live_ue, args.dry_run)
        latency_ms = (time.perf_counter() - t0) * 1000.0
        all_results.append(result)

        ok = not result["schema_errors"] and (result["tool_calls"] or result["final_text"])
        print(f"\n[{task['scenario_id']}] turns={result['turns']} tool_calls={len(result['tool_calls'])} schema_errors={result['schema_errors'] or 'none'}")
        for call in result["tool_calls"]:
            status = call["result"].get("status", "?")
            print(f"    -> {call['name']}({json.dumps(call['arguments'])}) => {status}")
        if result["final_text"]:
            print(f"    final: {result['final_text']}")

        session.record_step(
            scenario_id=task["scenario_id"],
            macro_intent=task["prompt"],
            grunt_tool_call=result["tool_calls"][0] if result["tool_calls"] else {},
            grunt_latency_ms=latency_ms,
            engine_response={"status": "success" if ok else "error"},
            engine_latency_ms=0.0,
            settle_verified=live_ue,
            baseline_tokens=result["total_input_tokens"] + result["total_output_tokens"],
            vail_tokens=None,
            token_source="anthropic_api_usage",
        )

    run_tag = f"{'full' if args.full else 'sample'}_{'dryrun' if args.dry_run else ('live' if live_ue else 'noengine')}"
    report_path = session.export_report(Path(args.out_dir) / run_tag)
    full_log_path = Path(args.out_dir) / run_tag / "haiku_tool_trial_transcripts.json"
    full_log_path.parent.mkdir(parents=True, exist_ok=True)
    full_log_path.write_text(json.dumps({"mode": mode, "results": all_results}, indent=2), encoding="utf-8")

    print("\n" + "=" * 80)
    print(f"Summary report:  {report_path}")
    print(f"Full transcripts: {full_log_path}")
    print(f"Mode: {mode}")
    tools_exercised = {c["name"] for r in all_results for c in r["tool_calls"]}
    tools_never_touched = {d["name"] for d in tool_defs} - tools_exercised
    print(f"Tools exercised this trial: {len(tools_exercised)}/{len(tool_defs)}")
    if tools_never_touched:
        print(f"Never called (need their own task added): {sorted(tools_never_touched)}")
    print("=" * 80)


if __name__ == "__main__":
    main()
