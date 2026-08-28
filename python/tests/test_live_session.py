import sys
import json
from pathlib import Path

if sys.platform == "win32":
    try:
        sys.stdout.reconfigure(encoding="utf-8")
    except Exception:
        pass

_ROOT = Path(r"d:\UE5.8\VAIL\python")
sys.path.insert(0, str(_ROOT))
sys.path.insert(0, r"d:\UE5.8\Tools\UnrealMCP_Python")

from unreal_mcp_server import UnrealConnection

def test_live_session():
    conn = UnrealConnection()
    if not conn.connect():
        print("❌ Could not connect to Unreal Engine 5.8 on port 55557")
        return

    print("🔌 Connected to Live Unreal Engine 5.8 Session!")

    # 1. Blueprint Inspection
    bp_path = "/Game/VAIL_Test/BP_LaserTurret"
    print(f"\n--- 1. Query Blueprint Topology for '{bp_path}' ---")
    top_res = conn.send_command("vail_graph_get_topology", {
        "asset_path": bp_path,
        "graph_name": "EventGraph"
    })
    print(f"Topology Nodes Count: {len(top_res.get('result', {}).get('nodes', []))}")
    for n in top_res.get('result', {}).get('nodes', []):
        print(f"  - Node: {n.get('node_title')} [{n.get('node_class')}] (ID: {n.get('node_id')})")

    # 2. Add Delay Node
    print("\n--- 2. Add 'Delay' function call node ---")
    add_res = conn.send_command("vail_graph_add_node", {
        "asset_path": bp_path,
        "graph_name": "EventGraph",
        "node_type": "CallFunction:Delay",
        "pos_x": 200.0,
        "pos_y": 0.0
    })
    print(f"Delay Node Result: {json.dumps(add_res, indent=2)}")
    delay_node_id = add_res.get("result", {}).get("node_id")

    # 3. Find BeginPlay and PrintString IDs
    begin_play_id = None
    print_node_id = None
    for n in top_res.get('result', {}).get('nodes', []):
        if "BeginPlay" in n.get("node_title", ""):
            begin_play_id = n.get("node_id")
        if "PrintString" in n.get("node_title", ""):
            print_node_id = n.get("node_id")

    # 4. Rewire: BeginPlay -> Delay -> PrintString
    print("\n--- 3. Wire BeginPlay -> Delay -> PrintString ---")
    if begin_play_id and delay_node_id:
        wire1 = conn.send_command("vail_graph_connect_pins", {
            "asset_path": bp_path,
            "graph_name": "EventGraph",
            "source_pin": f"{begin_play_id}:then",
            "target_pin": f"{delay_node_id}:execute"
        })
        print(f"Wire BeginPlay -> Delay: {json.dumps(wire1, indent=2)}")

    if delay_node_id and print_node_id:
        wire2 = conn.send_command("vail_graph_connect_pins", {
            "asset_path": bp_path,
            "graph_name": "EventGraph",
            "source_pin": f"{delay_node_id}:then",
            "target_pin": f"{print_node_id}:execute"
        })
        print(f"Wire Delay -> PrintString: {json.dumps(wire2, indent=2)}")

    # 5. Compile Blueprint
    print("\n--- 4. Set Scope and Trigger Compile ---")
    scope_res = conn.send_command("vail_set_scope", {
        "scope": "DetailsPanel",
        "target": "BP_LaserTurret"
    })
    print(f"Set Scope: {json.dumps(scope_res, indent=2)}")

    comp_res = conn.send_command("vail_execute_command", {
        "command_id": "Kismet.Compile"
    })
    print(f"Execute Compile: {json.dumps(comp_res, indent=2)}")

    # 6. Settle Check
    print("\n--- 5. Wait For Settle ---")
    settle_res = conn.send_command("vail_wait_for", {
        "timeout_seconds": 3.0
    })
    print(f"Settle Result: {json.dumps(settle_res, indent=2)}")

    # 7. Final Topology Verification
    print("\n--- 6. Final Blueprint Topology ---")
    final_top = conn.send_command("vail_graph_get_topology", {
        "asset_path": bp_path,
        "graph_name": "EventGraph"
    })
    print(f"Final Nodes: {len(final_top.get('result', {}).get('nodes', []))}")
    for n in final_top.get('result', {}).get('nodes', []):
        links = []
        for p in n.get('pins', []):
            if p.get('linked_to'):
                links.append(f"{p.get('pin_name')} -> {p.get('linked_to')}")
        link_str = f" | Links: {', '.join(links)}" if links else ""
        print(f"  • {n.get('node_title'):<25} ({n.get('node_class')}){link_str}")

    conn.disconnect()
    print("\n🎉 Live Manual Verification Complete!")

if __name__ == "__main__":
    test_live_session()
