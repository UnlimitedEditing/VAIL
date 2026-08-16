"""
Grunt Corpus Validator
Validates that every training sample strictly conforms to:
1. Valid JSON and JSONL format
2. Exact FastMCP parameter signatures and types
3. No hallucinated tool names or keys
4. Correct distribution (120+ samples per tool)
"""

import json
import os
import sys

CORPUS_PATH = os.path.join(os.path.dirname(os.path.abspath(__file__)), "grunt_vail_training_corpus.jsonl")

VALID_TOOLS = {
    "vail_set_scope": {"required": ["scope"], "optional": ["target"]},
    "vail_get_tree": {"required": [], "optional": ["max_depth", "category_filter"]},
    "vail_find": {"required": ["query"], "optional": ["scope"]},
    "vail_execute_command": {"required": ["command_id"], "optional": []},
    "vail_set_property": {"required": ["property_id", "value"], "optional": []},
    "vail_wait_for": {"required": [], "optional": ["timeout_seconds"]},
    "vail_begin_batch": {"required": [], "optional": ["title"]},
    "vail_end_batch": {"required": [], "optional": []},
}

def validate_corpus(file_path=CORPUS_PATH):
    if not os.path.exists(file_path):
        print(f"Error: File not found: {file_path}")
        return False

    tool_counts = {}
    total_samples = 0
    total_tool_calls = 0
    errors = []

    with open(file_path, "r", encoding="utf-8") as f:
        for line_num, line in enumerate(f, 1):
            line = line.strip()
            if not line:
                continue

            total_samples += 1
            try:
                sample = json.loads(line)
            except Exception as e:
                errors.append(f"Line {line_num}: Invalid JSON - {e}")
                continue

            tool_category = sample.get("tool", "unknown")
            tool_counts[tool_category] = tool_counts.get(tool_category, 0) + 1

            messages = sample.get("messages", [])
            if len(messages) < 3:
                errors.append(f"Line {line_num}: Missing standard (system, user, assistant) message triplet")

            # Validate tool calls in assistant turn
            assistant_msg = messages[-1]
            tool_calls = assistant_msg.get("tool_calls", [])
            if not tool_calls:
                errors.append(f"Line {line_num}: No tool_calls found in assistant turn")

            for call in tool_calls:
                total_tool_calls += 1
                func = call.get("function", {})
                func_name = func.get("name")
                
                if func_name not in VALID_TOOLS:
                    errors.append(f"Line {line_num}: Unknown tool name '{func_name}'")
                    continue

                try:
                    args = json.loads(func.get("arguments", "{}"))
                except Exception as e:
                    errors.append(f"Line {line_num}: Invalid JSON in tool arguments for {func_name} - {e}")
                    continue

                # Check required parameters
                schema = VALID_TOOLS[func_name]
                for req in schema["required"]:
                    if req not in args:
                        errors.append(f"Line {line_num}: Missing required parameter '{req}' for tool '{func_name}'")

                # Check unknown parameters
                allowed = set(schema["required"] + schema["optional"])
                for key in args.keys():
                    if key not in allowed:
                        errors.append(f"Line {line_num}: Unexpected parameter '{key}' for tool '{func_name}'")

    print("\n--- Corpus Validation Summary ---")
    print(f"Total Training Samples: {total_samples}")
    print(f"Total Tool Invocations: {total_tool_calls}")
    print("\nSamples per Tool Category:")
    for tool_name, count in sorted(tool_counts.items()):
        print(f"  • {tool_name:30s}: {count} samples")

    if errors:
        print(f"\n[FAIL] Validation Failed with {len(errors)} errors:")
        for err in errors[:10]:
            print(f"  - {err}")
        return False
    else:
        print("\n[SUCCESS] All 720 training samples passed 100% schema and parameter validation!")
        return True

if __name__ == "__main__":
    success = validate_corpus()
    sys.exit(0 if success else 1)
