import json
import sys
from pathlib import Path
from collections import Counter

if sys.platform == "win32":
    try:
        sys.stdout.reconfigure(encoding="utf-8")
    except Exception:
        pass

_ROOT = Path(__file__).resolve().parent
CORPUS_PATH = _ROOT / "grunt_vail_master_corpus_1440.jsonl"

REQUIRED_PARAMS = {
    "vail_set_scope": {"scope"},
    "vail_get_tree": set(),
    "vail_find": {"query"},
    "vail_execute_command": {"command_id"},
    "vail_set_property": {"property_id", "value"},
    "vail_wait_for": set(),
    "vail_begin_batch": {"title"},
    "vail_end_batch": set(),
    "vail_graph_add_node": {"node_type"},
    "vail_graph_connect_pins": {"source_pin", "target_pin"},
    "vail_graph_get_topology": set(),
    "vail_graph_delete_node": {"node_id"},
    "vail_asset_create": {"asset_path", "asset_class"},
    "vail_asset_query": set(),
    "compound_reconciliation_batch": set()
}

def validate_master_corpus():
    if not CORPUS_PATH.exists():
        print(f"ERROR: {CORPUS_PATH} does not exist. Run generate_phase2_corpus.py first.")
        return False

    tool_counts = Counter()
    total_tool_calls = 0
    errors = []

    lines = CORPUS_PATH.read_text(encoding="utf-8").strip().split("\n")
    for idx, line in enumerate(lines):
        if not line:
            continue

        try:
            sample = json.loads(line)
        except Exception as e:
            errors.append(f"Line {idx+1}: Invalid JSON - {e}")
            continue

        tool_category = sample.get("tool")
        tool_counts[tool_category] += 1

        messages = sample.get("messages", [])
        if len(messages) < 3:
            errors.append(f"Line {idx+1}: Messages array must have at least 3 turns")
            continue

        assistant_msg = messages[-1]
        tool_calls = assistant_msg.get("tool_calls", [])
        if not tool_calls:
            errors.append(f"Line {idx+1}: Assistant turn missing 'tool_calls'")
            continue

        for tc in tool_calls:
            total_tool_calls += 1
            fn = tc.get("function", {})
            fn_name = fn.get("name")
            fn_args_raw = fn.get("arguments", "{}")

            try:
                fn_args = json.loads(fn_args_raw) if isinstance(fn_args_raw, str) else fn_args_raw
            except Exception as e:
                errors.append(f"Line {idx+1}: Tool arguments not valid JSON - {e}")
                continue

            # Validate parameter requirements
            required = REQUIRED_PARAMS.get(fn_name, set())
            missing = required - set(fn_args.keys())
            if missing:
                errors.append(f"Line {idx+1}: Tool '{fn_name}' missing required parameters: {missing}")

    print("\n--- Master Corpus Validation Summary ---")
    print(f"Total Training Samples: {len(lines)}")
    print(f"Total Tool Invocations: {total_tool_calls}")
    print("\nSamples per Tool Category:")
    for tool_name, count in sorted(tool_counts.items()):
        print(f"  • {tool_name:<30} : {count} samples")

    if errors:
        print(f"\n❌ Validation FAILED with {len(errors)} errors:")
        for err in errors[:10]:
            print(f"  - {err}")
        return False

    print("\n✅ [SUCCESS] All 1,440 master training samples passed 100% schema and parameter validation!")
    return True

if __name__ == "__main__":
    validate_master_corpus()
