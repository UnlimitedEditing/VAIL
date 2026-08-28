#!/usr/bin/env python3
"""
VAIL A/B Token Benchmark

Runs the same set of editor tasks two ways and logs REAL measured token
counts for both, via VAILTelemetrySession:

  Baseline path  -- the task is solved the "naive agent" way: dump a raw
                    UE reflection/state blob (standing in for a screenshot
                    description) into a Claude message alongside the
                    instruction, and let Claude answer in prose describing
                    what it would change. Real `usage.input_tokens` /
                    `usage.output_tokens` from the actual API response are
                    what get logged -- nothing estimated.

  VAIL path      -- the same instruction is routed through GruntDispatcher
                    (macros first, neural Grunt fallback second). The
                    resulting tool-call JSON is what would actually cross
                    the wire to the UE plugin. Its token count is measured
                    with Claude's real tokenizer (via the Anthropic
                    `count_tokens` API, so it's the same units as the
                    baseline, not a character/4 guess).

Nothing here is faked and nothing here is optional to label: every report
row says exactly how its numbers were obtained (see `token_source` in
vail_telemetry.py). Run with --dry-run to sanity-check the wiring with a
mocked API client (a mocked run is clearly stamped "DRY RUN / NOT REAL
MEASUREMENTS" in its own output and report and must never be quoted as a
real result).

Usage:
    set ANTHROPIC_API_KEY=sk-...
    python run_ab_benchmark.py                 # real API calls, real UE plugin required for VAIL execution
    python run_ab_benchmark.py --dry-run        # wiring smoke test, mocked responses, no real numbers
    python run_ab_benchmark.py --no-execute     # measure tokens only, don't require a live UE TCP connection
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

from vail_telemetry import VAILTelemetrySession  # noqa: E402

# A raw reflection-style state dump standing in for "what a screenshot-driven
# agent would be forced to read" -- the exact contents don't matter for the
# benchmark's validity, only that it's a realistic per-task size and that
# BOTH paths solve the SAME task from it.
_RAW_UE_STATE_TEMPLATE = """\
Active Details Panel for actor '{actor}':
  Transform:
    RelativeLocation: X=0.000000 Y=0.000000 Z=0.000000
    RelativeRotation: Pitch=0.000000 Yaw=0.000000 Roll=0.000000
    RelativeScale3D: X=1.000000 Y=1.000000 Z=1.000000
  Light:
    Intensity: 10.000000 (unit: lux)
    LightColor: (R=1.000000,G=1.000000,B=1.000000,A=1.000000)
    Mobility: Movable
    CastShadows: true
    AttenuationRadius: 1000.000000
  Rendering:
    bHidden: false
    bVisible: true
    CastVolumetricShadow: true
  ... (full panel continues for another ~40 properties, category headers,
  tooltips, and widget metadata -- this is what a screenshot-parsing or
  full-reflection-dump agent has to read for every single edit) ...
"""

TASKS: List[Dict[str, str]] = [
    {
        "scenario_id": "set_light_intensity",
        "actor": "DirectionalLight",
        "instruction": "Set property 'DirectionalLightComponent->Intensity' to '35000.0'",
    },
    {
        "scenario_id": "set_light_color",
        "actor": "DirectionalLight",
        "instruction": "Set 'DirectionalLight' color to 0.25,0.45,1.0",
    },
    {
        "scenario_id": "move_actor",
        "actor": "BP_Player",
        "instruction": "Move 'BP_Player' to 100,200,50",
    },
    {
        "scenario_id": "rename_actor",
        "actor": "StaticMeshActor_3",
        "instruction": "Rename 'StaticMeshActor_3' to 'Rock_Boulder_01'",
    },
    {
        "scenario_id": "save_and_compile",
        "actor": "BP_Player",
        "instruction": "save and compile",
    },
]


def _load_anthropic_client(dry_run: bool):
    if dry_run:
        return _MockAnthropicClient()
    try:
        import anthropic
    except ImportError:
        sys.exit("anthropic package not installed. `pip install anthropic`, or run with --dry-run to test wiring only.")
    api_key = os.environ.get("ANTHROPIC_API_KEY")
    if not api_key:
        sys.exit("ANTHROPIC_API_KEY not set. Export it, or run with --dry-run to test wiring only.")
    return anthropic.Anthropic(api_key=api_key)


class _MockAnthropicClient:
    """Deterministic stand-in for wiring smoke tests. Every number it returns
    is synthetic and must be treated as such -- never fed into a real report."""

    class _Usage:
        def __init__(self, inp, out):
            self.input_tokens = inp
            self.output_tokens = out

    class _Message:
        def __init__(self, usage):
            self.usage = usage
            self.content = [type("Block", (), {"text": "[dry-run] mocked response"})()]

    class _Messages:
        def create(self, **kwargs):
            prompt_len = sum(len(str(m.get("content", ""))) for m in kwargs.get("messages", []))
            return _MockAnthropicClient._Message(_MockAnthropicClient._Usage(inp=max(1, prompt_len // 4), out=40))

        def count_tokens(self, **kwargs):
            text = kwargs.get("messages", [{}])[0].get("content", "")
            return type("Count", (), {"input_tokens": max(1, len(str(text)) // 4)})()

    def __init__(self):
        self.messages = self._Messages()


def run_baseline(client, model: str, actor: str, instruction: str) -> Dict[str, Any]:
    state_blob = _RAW_UE_STATE_TEMPLATE.format(actor=actor)
    prompt = (
        f"{state_blob}\n\nInstruction: {instruction}\n\n"
        "Describe exactly what property change(s) you would make to satisfy this instruction."
    )
    t0 = time.perf_counter()
    resp = client.messages.create(
        model=model,
        max_tokens=200,
        messages=[{"role": "user", "content": prompt}],
    )
    latency_ms = (time.perf_counter() - t0) * 1000.0
    return {
        "input_tokens": resp.usage.input_tokens,
        "output_tokens": resp.usage.output_tokens,
        "total_tokens": resp.usage.input_tokens + resp.usage.output_tokens,
        "latency_ms": latency_ms,
    }


def run_vail_side(client, model: str, dispatcher, instruction: str) -> Dict[str, Any]:
    t0 = time.perf_counter()
    routed = dispatcher.dispatch(instruction)
    latency_ms = (time.perf_counter() - t0) * 1000.0

    wire_payload = json.dumps(routed["calls"])
    count = client.messages.count_tokens(model=model, messages=[{"role": "user", "content": wire_payload}])
    return {
        "path": routed["path"],
        "macro_id": routed.get("macro_id"),
        "calls": routed["calls"],
        "wire_tokens": count.input_tokens,
        "latency_ms": latency_ms,
    }


def main():
    parser = argparse.ArgumentParser(description="VAIL A/B token benchmark (real measured numbers only)")
    parser.add_argument("--dry-run", action="store_true", help="Mock the Anthropic client to smoke-test wiring. Output is NOT a real result.")
    parser.add_argument("--no-execute", action="store_true", help="Skip live UE TCP execution; measure routing/tokens only.")
    parser.add_argument("--model", default="claude-sonnet-4-5", help="Anthropic model id for baseline + tokenizer.")
    parser.add_argument("--out-dir", default=str(_HERE / "reports"), help="Directory to write the telemetry report to.")
    args = parser.parse_args()

    client = _load_anthropic_client(args.dry_run)

    from dispatcher import GruntDispatcher
    dispatcher = GruntDispatcher(lazy_model=True)

    session = VAILTelemetrySession(suite_name="VAIL_AB_Token_Benchmark" + ("_DRYRUN" if args.dry_run else ""))

    print("=" * 80)
    print("VAIL A/B TOKEN BENCHMARK" + ("  [DRY RUN - MOCKED, NOT REAL NUMBERS]" if args.dry_run else ""))
    print("=" * 80)

    for task in TASKS:
        baseline = run_baseline(client, args.model, task["actor"], task["instruction"])
        vail = run_vail_side(client, args.model, dispatcher, task["instruction"])

        saved = baseline["total_tokens"] - vail["wire_tokens"]
        print(f"\n[{task['scenario_id']}] \"{task['instruction']}\"")
        print(f"  baseline: {baseline['total_tokens']} tokens (in={baseline['input_tokens']}, out={baseline['output_tokens']})")
        print(f"  vail:     {vail['wire_tokens']} tokens via {vail['path']} path" + (f" (macro: {vail['macro_id']})" if vail["macro_id"] else ""))
        print(f"  saved:    {saved} tokens ({(saved / baseline['total_tokens'] * 100):.1f}%)")

        session.record_step(
            scenario_id=task["scenario_id"],
            macro_intent=task["instruction"],
            grunt_tool_call=vail["calls"][0] if vail["calls"] else {},
            grunt_latency_ms=vail["latency_ms"],
            engine_response={"status": "not_executed" if args.no_execute else "unknown"},
            engine_latency_ms=0.0,
            settle_verified=False,
            baseline_tokens=baseline["total_tokens"],
            vail_tokens=vail["wire_tokens"],
            token_source="mock_dry_run" if args.dry_run else "anthropic_api_usage+count_tokens",
        )

    report_path = session.export_report(Path(args.out_dir))
    print("\n" + "=" * 80)
    print(f"Report written to: {report_path}")
    if args.dry_run:
        print("^^ DRY RUN: numbers above are synthetic wiring-test values. Do not quote them anywhere.")
    print("=" * 80)


if __name__ == "__main__":
    main()
