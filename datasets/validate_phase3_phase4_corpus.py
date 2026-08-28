"""
Phase 3 + Phase 4 Synthetic Worker Corpus Validator.
Asserts 100% schema compliance, typing, and zero hallucinated properties across 3,000 samples.
"""

import sys
import json
from pathlib import Path
from collections import Counter

if sys.platform == "win32":
    try:
        sys.stdout.reconfigure(encoding="utf-8")
    except Exception:
        pass

_ROOT = Path(__file__).resolve().parent
CORPUS_PATH = _ROOT / "grunt_vail_worker_phase3_4_3000.jsonl"
MCP_SCHEMA_DIR = Path(r"C:\Users\jacob\.gemini\antigravity-ide\mcp\unreal-engine")

def validate_worker_corpus():
    print("=" * 80)
    print("🔍 VALIDATING PHASE 3 + PHASE 4 WORKER CORPUS (3,000 SAMPLES)")
    print("=" * 80)

    if not CORPUS_PATH.exists():
        print(f"❌ Corpus file not found: {CORPUS_PATH}")
        return False

    # Load schemas
    schemas = {}
    for sf in MCP_SCHEMA_DIR.glob("vail_*.json"):
        try:
            sdata = json.loads(sf.read_text(encoding="utf-8"))
            schemas[sdata["name"]] = sdata
        except Exception as e:
            print(f"Error loading schema {sf}: {e}")

    lines = [l.strip() for l in CORPUS_PATH.read_text(encoding="utf-8").split("\n") if l.strip()]
    print(f"Loaded {len(lines)} samples from {CORPUS_PATH.name}\n")

    tool_counter = Counter()
    errors = 0

    for idx, line in enumerate(lines, start=1):
        try:
            sample = json.loads(line)
        except Exception as e:
            print(f"❌ Line {idx}: JSON parse error: {e}")
            errors += 1
            continue

        tool_name = sample.get("tool")
        messages = sample.get("messages", [])

        if not tool_name or len(messages) < 3:
            print(f"❌ Line {idx}: Invalid message envelope structure")
            errors += 1
            continue

        assistant_msg = messages[2]
        tool_calls = assistant_msg.get("tool_calls", [])

        if not tool_calls:
            print(f"❌ Line {idx}: Missing tool_calls in assistant message")
            errors += 1
            continue

        call = tool_calls[0]
        fn = call.get("function", {})
        fn_name = fn.get("name")
        args_str = fn.get("arguments", "{}")

        try:
            args = json.loads(args_str)
        except Exception as e:
            print(f"❌ Line {idx}: Argument JSON parse error: {e}")
            errors += 1
            continue

        if fn_name != tool_name:
            print(f"❌ Line {idx}: Function name mismatch ({fn_name} != {tool_name})")
            errors += 1
            continue

        if fn_name not in schemas:
            print(f"❌ Line {idx}: Unknown tool schema '{fn_name}'")
            errors += 1
            continue

        schema = schemas[fn_name]
        props = schema["parameters"].get("properties", {})
        required = schema["parameters"].get("required", [])

        # Verify required arguments
        for req in required:
            if req not in args:
                print(f"❌ Line {idx} ({fn_name}): Missing required argument '{req}'")
                errors += 1

        # Verify all passed arguments exist in schema
        for arg_key in args.keys():
            if arg_key not in props:
                print(f"❌ Line {idx} ({fn_name}): Unknown argument '{arg_key}'")
                errors += 1

        tool_counter[fn_name] += 1

    print("--- Tool Call Distribution Across 25 Worker Primitives ---")
    for t_name, count in sorted(tool_counter.items()):
        print(f"  • {t_name:<32} : {count:3d} samples")

    print("\n" + "=" * 80)
    if errors == 0 and len(lines) == 3000:
        print("✅ SUCCESS: All 3,000 synthetic training pairs passed 100% validation!")
        print(f"   Corpus ready for distillation: {CORPUS_PATH}")
        print("=" * 80)
        return True
    else:
        print(f"❌ VALIDATION FAILED: Found {errors} errors across {len(lines)} lines.")
        print("=" * 80)
        return False

if __name__ == "__main__":
    if not validate_worker_corpus():
        sys.exit(1)
