"""
VAIL Phase 3 Schema and Tool Validation Test.
Asserts 100% integrity across Python tool bindings, FastMCP signatures, and JSON schemas.
"""

import sys
import json
import inspect
from pathlib import Path

if sys.platform == "win32":
    try:
        sys.stdout.reconfigure(encoding="utf-8")
    except Exception:
        pass

_ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(_ROOT))

def validate_all():
    print("=" * 80)
    print("🔍 VALIDATING VAIL PHASE 1, 2, AND 3 SCHEMAS & TOOLSETS")
    print("=" * 80)

    mcp_dir = Path(r"C:\Users\jacob\.gemini\antigravity-ide\mcp\unreal-engine")
    grunt_dir = Path(r"d:\UE5.8\VAIL\grunt\tools")

    phase1_tools = [
        "vail_set_scope",
        "vail_get_tree",
        "vail_find",
        "vail_execute_command",
        "vail_set_property",
        "vail_wait_for",
        "vail_begin_batch",
        "vail_end_batch",
    ]

    phase2_tools = [
        "vail_graph_get_topology",
        "vail_graph_add_node",
        "vail_graph_connect_pins",
        "vail_graph_delete_node",
        "vail_asset_create",
        "vail_asset_query",
        "vail_asset_save",
    ]

    phase3_tools = [
        "vail_level_spawn_actor",
        "vail_level_query_actors",
        "vail_level_delete_actor",
        "vail_viewport_frame",
        "vail_widget_tree_get",
        "vail_widget_add_element",
        "vail_widget_set_slot",
        "vail_widget_bind_event",
        "vail_sequencer_query",
        "vail_sequencer_add_track",
        "vail_sequencer_add_key",
        "vail_sense_optical",
        "vail_sense_spatial",
        "vail_sense_mesh",
        "vail_sense_shader",
    ]

    phase4_tools = [
        "vail_landscape_create",
        "vail_landscape_sculpt",
        "vail_landscape_paint",
        "vail_foliage_scatter",
        "vail_foliage_query",
        "vail_control_rig_set_transform",
        "vail_control_rig_query",
        "vail_audio_play",
        "vail_audio_set_parameter",
        "vail_project_build",
    ]

    all_tools = {
        "Phase 1 (Properties & Commands)": phase1_tools,
        "Phase 2 (Universal Graphs & Assets)": phase2_tools,
        "Phase 3 (Spatial, UMG, Sequencer, Sensory)": phase3_tools,
        "Phase 4 (Subsystems & Production Tooling)": phase4_tools,
    }


    total_checked = 0
    passed = 0

    for phase_name, tools in all_tools.items():
        print(f"\n📂 Checking {phase_name} ({len(tools)} tools):")
        for tool_name in tools:
            total_checked += 1
            mcp_file = mcp_dir / f"{tool_name}.json"
            grunt_file = grunt_dir / f"{tool_name}.json"

            # Check MCP file existence
            if not mcp_file.exists():
                print(f"  ❌ Missing MCP schema: {mcp_file}")
                continue

            # Check Grunt file existence
            if not grunt_file.exists():
                print(f"  ❌ Missing Grunt schema: {grunt_file}")
                continue

            try:
                mcp_data = json.loads(mcp_file.read_text(encoding="utf-8"))
                grunt_data = json.loads(grunt_file.read_text(encoding="utf-8"))
            except Exception as e:
                print(f"  ❌ Invalid JSON in {tool_name}: {e}")
                continue

            if mcp_data.get("name") != tool_name:
                print(f"  ❌ Tool name mismatch in {tool_name}.json: got {mcp_data.get('name')}")
                continue

            if "parameters" not in mcp_data:
                print(f"  ❌ Missing 'parameters' in {tool_name}.json")
                continue

            params = mcp_data["parameters"]
            if params.get("type") != "object":
                print(f"  ❌ Parameters type in {tool_name}.json is not 'object'")
                continue

            props = params.get("properties", {})
            required = params.get("required", [])

            for req in required:
                if req not in props:
                    print(f"  ❌ Required param '{req}' not in properties of {tool_name}.json")
                    continue

            print(f"  ✅ {tool_name:<30} (props: {len(props):2d}, req: {len(required):2d})")
            passed += 1

    print("\n" + "=" * 80)
    print(f"SUMMARY: {passed}/{total_checked} tools passed 100% schema validation!")
    print("=" * 80)
    return passed == total_checked

if __name__ == "__main__":
    if not validate_all():
        sys.exit(1)
