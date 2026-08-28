"""
Model loading for Grunt: routes tool-calling requests through the real
cactus-compute/needle backend (a 26M-param SimpleAttentionNetwork, JAX/Flax).

Needle lives in its own isolated venv under needle-src/.venv (installed via
`needle-src/setup`, or `needle-src/.venv/Scripts/python.exe -m pip install -e
needle-src` on Windows where the setup script's `source .venv/bin/activate`
silently no-ops — Windows venvs use Scripts/, not bin/). Grunt's own venv
stays free of jax/flax/etc.; run_inference() shells out to the isolated
interpreter with a small inline driver script rather than importing needle
into this process, keeping the ML dependency footprint out of every
consumer of grunt.py.

The driver calls needle's generate(..., stream=False) directly instead of
the `needle run` CLI: the CLI's main() prints human-formatted progress text
("Loading checkpoint: ...", "Query: ...") to stdout and never returns the
generated string, so its stdout isn't valid JSON. generate(stream=False)
returns exactly the model's raw tool-call JSON with no wrapper noise.

run_inference_batch() is the fix for the checkpoint-load + JIT-compile
overhead flagged as a follow-up here previously (confirmed via needle's own
run.py: both are one-time PER-PROCESS costs, not per-query -- JIT is cached
in _get_decode_fn's _decode_fn_cache, keyed by (id(model), max_gen_len)).
It shells out ONCE to needle's generate_batch(), which does a single encode
pass and a shared decode loop across the whole batch. Use this for bulk
offline eval trials (many known queries up front); run_inference() stays the
per-call path for grunt.py's live single-shot CLI usage, where there's only
ever one query per process anyway so batching wouldn't help.

Measured (see research/benchmark_batch_vs_percall.py), not assumed: batching
gives ~1.8x wall-clock speedup (18.4s/query per-call vs 10.3s/query batched,
N=20), not the "near-instant per item after warmup" that amortizing a
one-time cost would suggest. Root cause: needle's decode loop runs
constrained decoding as a plain Python per-batch-item loop with a JAX->NumPy
device sync at every token position (see needle-src/needle/model/run.py's
generate_batch()) -- that per-item cost doesn't vanish just because the
process and JIT compilation are shared. Real win, not a full fix; bulk eval
trials are meaningfully cheaper this way but still real wall-clock at scale.
"""
import json
import math
import os
import subprocess
from pathlib import Path

_GRUNT_DIR = Path(__file__).resolve().parent
_NEEDLE_SRC = Path(os.environ.get("GRUNT_NEEDLE_SRC", _GRUNT_DIR / "needle-src"))
_NEEDLE_USE_V1 = os.environ.get("GRUNT_NEEDLE_USE_V1", "0").lower() in ("1", "true", "yes")

_NEEDLE_CHECKPOINT = Path(os.environ.get(
    "GRUNT_NEEDLE_CHECKPOINT",
    _GRUNT_DIR / "checkpoints" / "grunt_needle_master_v4.pkl",
))
_VAIL_CHECKPOINT = Path(os.environ.get(
    "GRUNT_VAIL_CHECKPOINT",
    _GRUNT_DIR / "checkpoints" / "grunt_needle_vail_master.pkl",
))
_VAIL_TOOLS_DIR = _GRUNT_DIR / "tools" / "vail"
# Windows venv layout is Scripts/python.exe, not bin/python3.
_NEEDLE_PYTHON = _NEEDLE_SRC / ".venv" / "Scripts" / "python.exe"

_DRIVER = r"""
import json, sys, os
sys.path.insert(0, os.environ.get("GRUNT_NEEDLE_SRC", "."))
from needle.model.run import generate, load_checkpoint
from needle.model.architecture import SimpleAttentionNetwork
from needle.dataset.dataset import get_tokenizer

checkpoint_path, tools_json, query = sys.argv[1], sys.argv[2], sys.argv[3]
params, config = load_checkpoint(checkpoint_path)
model = SimpleAttentionNetwork(config)
tokenizer = get_tokenizer()
result = generate(model, params, tokenizer, query=query, tools=tools_json, stream=False)
print(result)
"""

_NEEDLE2_DRIVER = r"""
import json, sys, math
from needle.model.run import load_checkpoint
from needle.model.architecture import SimpleAttentionNetwork
from needle.dataset.dataset import get_tokenizer
from needle.model.decode import batched_generate

checkpoint_path, tools_json, query = sys.argv[1], sys.argv[2], sys.argv[3]
params, config = load_checkpoint(checkpoint_path)
model = SimpleAttentionNetwork(config)
tokenizer = get_tokenizer()

texts, out_ids, out_lps = batched_generate(config, params, tokenizer, [query], max_new_tokens=96)
raw_text = texts[0] if texts else ""
logps = out_lps[0] if out_lps else []
confidence = math.exp(sum(logps) / len(logps)) if logps else 0.0

print(json.dumps({"output": raw_text, "confidence": confidence}))
"""

_BATCH_DRIVER = r"""
import json, sys
from needle.model.run import generate_batch, load_checkpoint
from needle.model.architecture import SimpleAttentionNetwork
from needle.dataset.dataset import get_tokenizer

checkpoint_path = sys.argv[1]
items = json.loads(sys.stdin.read())  # list of {"task": str, "tools": list}
params, config = load_checkpoint(checkpoint_path)
model = SimpleAttentionNetwork(config)
tokenizer = get_tokenizer()
queries = [item["task"] for item in items]
tools_list = [json.dumps(item["tools"]) for item in items]
results = generate_batch(model, params, tokenizer, queries, tools_list)
print(json.dumps(results))
"""


def load_grunt_model(model_path: str = None) -> dict:
    """Verifies the isolated needle venv and checkpoint are in place. Does
    NOT load the model into this process — loading happens per-call inside
    the subprocess spawned by run_inference().
    """
    checkpoint = Path(model_path) if model_path else _NEEDLE_CHECKPOINT
    if not _NEEDLE_PYTHON.exists():
        raise RuntimeError(
            f"Needle venv not found at {_NEEDLE_PYTHON}. From needle-src/, run: "
            f".venv\\Scripts\\python.exe -m pip install -e ."
        )
    if not checkpoint.exists():
        raise RuntimeError(
            f"Needle checkpoint not found at {checkpoint}. Download it first:\n"
            "  from huggingface_hub import hf_hub_download\n"
            "  hf_hub_download(repo_id='Cactus-Compute/needle', filename='needle.pkl', "
            "repo_type='model', local_dir='needle-src/checkpoints')"
        )
    return {"backend": "needle", "checkpoint": str(checkpoint)}


def load_vail_tools() -> list[dict]:
    """Loads all VAIL FastMCP JSON tool definitions from tools/vail/."""
    tools = []
    if _VAIL_TOOLS_DIR.exists():
        for path in sorted(_VAIL_TOOLS_DIR.glob("*.json")):
            try:
                tools.append(json.loads(path.read_text(encoding="utf-8")))
            except Exception as e:
                pass
    return tools


def load_grunt_vail_model(model_path: str = None) -> dict:
    """Verifies the isolated needle venv and VAIL checkpoint are in place."""
    checkpoint = Path(model_path) if model_path else _VAIL_CHECKPOINT
    if not _NEEDLE_PYTHON.exists():
        raise RuntimeError(
            f"Needle venv not found at {_NEEDLE_PYTHON}. From needle-src/, run: "
            f".venv\\Scripts\\python.exe -m pip install -e ."
        )
    if not checkpoint.exists():
        raise RuntimeError(
            f"VAIL Needle checkpoint not found at {checkpoint}. Train or copy it first:\n"
            "  python train_vail_checkpoint.py"
        )
    return {"backend": "needle", "checkpoint": str(checkpoint), "domain": "vail"}


def flatten_tool_for_needle(tool: dict) -> dict:
    """Adapts a standard JSON-Schema tool def (parameters.type/properties/
    required) to the flat parameter-dict shape needle's constrained decoder
    actually expects.

    needle-src/needle/model/constrained.py's ToolConstraints builds its
    argument-key trie via `for key, val in tool["parameters"].items()`,
    keeping only keys whose value is itself a dict. Fed a standard
    JSON-Schema `parameters` block, the only such top-level key is
    "properties" itself -- so the trie ends up containing exactly the word
    "properties" and the decoder hard-masks out every real argument name
    (target_path, artifact_type, ...) on every call. Confirmed directly
    against tools/index_okf_directory.json. needle's own run.py built-in
    examples use this flat shape (`'parameters': {'location': {'type':
    'string', 'required': True}}`), so this adapts our registry's schema at
    the needle boundary rather than changing the canonical tool format that
    supervisor.py/eval_candidate.py's static gate rely on.
    """
    params = tool.get("parameters", {})
    props = params.get("properties", {}) if isinstance(params, dict) else {}
    required = set(params.get("required", [])) if isinstance(params, dict) else set()
    flat_params = {}
    for key, schema in props.items():
        flat = dict(schema) if isinstance(schema, dict) else {}
        flat["required"] = key in required
        flat_params[key] = flat
    return {
        "name": tool["name"],
        "description": tool.get("description", ""),
        "parameters": flat_params,
    }


def validate_tool_schema_for_needle(tools: list) -> list:
    """Guardrail for research/prepare_training_run.py: catches the
    flatten_tool_for_needle() bug class before a training run starts,
    instead of after it finishes.

    Checks each tool's flattened parameter set actually contains its real
    argument names (not e.g. a bare "properties" key, which is what a
    nested-JSON-Schema tool produces if it's ever fed to needle unflattened
    -- see flatten_tool_for_needle's docstring for the real incident this
    guards against). Returns a list of human-readable problem strings, one
    per tool that fails; empty list means every tool is safe to train on.
    """
    problems = []
    for tool in tools:
        name = tool.get("name", "<unnamed>")
        params = tool.get("parameters", {})
        if not isinstance(params, dict) or "properties" not in params:
            problems.append(f"{name}: no 'properties' object defined under parameters")
            continue
        declared = set(params.get("properties", {}).keys())
        flat = flatten_tool_for_needle(tool)
        flat_keys = set(flat.get("parameters", {}).keys())
        if flat_keys != declared:
            problems.append(
                f"{name}: flattened argument keys {sorted(flat_keys)} don't match "
                f"declared parameters {sorted(declared)} -- needle's constrained decoder "
                f"would be trained/run against the wrong key set"
            )
    return problems


def _adapt_needle_output(raw: str) -> str:
    """Adapts needle's list-of-tool-calls output to Grunt's single-object
    stdout contract: exactly one match -> that object; zero matches -> an
    error JSON; multiple matches -> an error JSON (ambiguous, needs
    disambiguation this router doesn't attempt yet). Shared by both the
    single-call and batch paths.
    """
    try:
        calls = json.loads(raw)
    except json.JSONDecodeError:
        return json.dumps({"error": "invalid JSON from needle", "raw": raw})

    if isinstance(calls, dict):
        return json.dumps(calls)
    if isinstance(calls, list):
        if len(calls) == 1:
            return json.dumps(calls[0])
        if len(calls) == 0:
            return json.dumps({"error": "no matching tool (needle backend)"})
        return json.dumps({"error": "ambiguous: multiple tool calls", "candidates": calls})
    return json.dumps({"error": "unexpected needle output shape", "raw": raw})


def run_inference_with_confidence(model: dict, tools: list, user_task: str) -> tuple[str, float]:
    """Runs needle in its isolated venv for a single query.
    Returns (adapted_output_json, confidence_score).
    If using legacy Needle 1 checkpoint (or GRUNT_NEEDLE_USE_V1=1), uses the Needle 1 driver.
    """
    tools_json = json.dumps([flatten_tool_for_needle(t) for t in tools])
    use_v1 = True
    if os.environ.get("GRUNT_NEEDLE_USE_V2", "0").lower() in ("1", "true", "yes"):
        use_v1 = False
    elif os.environ.get("GRUNT_NEEDLE_USE_V1", "").lower() in ("0", "false", "no"):
        use_v1 = False

    driver_script = _DRIVER if use_v1 else _NEEDLE2_DRIVER
    checkpoint_path = model.get("checkpoint", str(_NEEDLE_CHECKPOINT))

    proc = subprocess.run(
        [str(_NEEDLE_PYTHON), "-c", driver_script, checkpoint_path, tools_json, user_task],
        capture_output=True,
        text=True,
        cwd=str(_NEEDLE_SRC),
    )
    if proc.returncode != 0:
        err = json.dumps({"error": "needle inference failed", "stderr": proc.stderr[-2000:]})
        return err, 0.0

    lines = [line for line in proc.stdout.splitlines() if line.strip()]
    raw = lines[-1] if lines else ""

    if not use_v1:
        try:
            parsed = json.loads(raw)
            if isinstance(parsed, dict) and "output" in parsed:
                output_str = _adapt_needle_output(parsed["output"])
                conf = float(parsed.get("confidence", 0.0))
                return output_str, conf
        except json.JSONDecodeError:
            pass

    return _adapt_needle_output(raw), 0.85


def run_inference(model: dict, tools: list, user_task: str) -> str:
    """Runs needle in its isolated venv via subprocess for a single query."""
    output_str, _ = run_inference_with_confidence(model, tools, user_task)
    return output_str


def run_inference_batch(model: dict, items: list) -> list:
    """Runs needle ONCE for a whole list of queries, amortizing checkpoint
    load + JIT compile across all of them instead of paying it per item.

    items: list of {"task": str, "tools": list} dicts (each item can have
    its own tools list, matching needle's generate_batch() signature).
    Returns a list of adapted single-object JSON strings, same contract as
    run_inference(), one per item, in the same order.
    """
    if not items:
        return []
    flattened_items = [
        {"task": it["task"], "tools": [flatten_tool_for_needle(t) for t in it["tools"]]}
        for it in items
    ]
    proc = subprocess.run(
        [str(_NEEDLE_PYTHON), "-c", _BATCH_DRIVER, model["checkpoint"]],
        input=json.dumps(flattened_items),
        capture_output=True,
        text=True,
        cwd=str(_NEEDLE_SRC),
    )
    if proc.returncode != 0:
        error = json.dumps({"error": "needle batch inference failed", "stderr": proc.stderr[-2000:]})
        return [error] * len(items)

    lines = [line for line in proc.stdout.splitlines() if line.strip()]
    raw_line = lines[-1] if lines else ""
    try:
        raw_results = json.loads(raw_line)
    except json.JSONDecodeError:
        error = json.dumps({"error": "invalid JSON from needle batch driver", "raw": raw_line})
        return [error] * len(items)

    if not isinstance(raw_results, list) or len(raw_results) != len(items):
        error = json.dumps({"error": "unexpected needle batch output shape", "raw": raw_line})
        return [error] * len(items)

    return [_adapt_needle_output(r) for r in raw_results]
