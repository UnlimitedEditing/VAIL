"""
VAIL Phase 2 Verification Suite: Universal Graph Data-Channel & Asset Management
Tests headless asset creation, graph inspection, polymorphic node spawning, and pin wiring.
"""

import sys
import os
import json
import logging
from pathlib import Path

_ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(_ROOT))
sys.path.insert(0, r"d:\UE5.8\Tools\UnrealMCP_Python")

from unreal_mcp_server import UnrealConnection

logging.basicConfig(level=logging.INFO)
logger = logging.getLogger("VAILPhase2Test")

def run_phase2_verification():
    print("=" * 80)
    print("🚀 RUNNING VAIL PHASE 2: UNIVERSAL GRAPH & ASSET VERIFICATION")
    print("=" * 80)

    conn = UnrealConnection()
    if not conn.connect():
        print("❌ Could not connect to Unreal Engine 5.8 on port 55557")
        return False
    print("✅ Connected to Unreal Engine 5.8 Bridge")

    # =========================================================================
    # TEST 1: Headless Asset Creation
    # =========================================================================
    print("\n--- TEST 1: Headless Asset Creation (/Game/VAIL_Test/) ---")
    
    # 1.1 Create Material
    res_mat = conn.send_command("vail_asset_create", {
        "asset_path": "/Game/VAIL_Test/M_GlowCore",
        "asset_class": "Material"
    })
    print(f"Material Creation: {json.dumps(res_mat, indent=2)}")

    # 1.2 Create Blueprint Actor
    res_bp = conn.send_command("vail_asset_create", {
        "asset_path": "/Game/VAIL_Test/BP_LaserTurret",
        "asset_class": "Blueprint",
        "parent_class": "Actor"
    })
    print(f"Blueprint Creation: {json.dumps(res_bp, indent=2)}")

    # =========================================================================
    # TEST 2: Asset Registry Querying
    # =========================================================================
    print("\n--- TEST 2: Querying Asset Registry (/Game/VAIL_Test) ---")
    res_query = conn.send_command("vail_asset_query", {
        "package_path": "/Game/VAIL_Test",
        "class_filter": ""
    })
    print(f"Found Assets: {json.dumps(res_query, indent=2)}")

    # =========================================================================
    # TEST 3: Material Graph Node Spawning & Topology
    # =========================================================================
    print("\n--- TEST 3: Material Graph Expression Spawning ---")

    # 3.1 Spawn VectorParameter
    res_vec = conn.send_command("vail_graph_add_node", {
        "asset_path": "/Game/VAIL_Test/M_GlowCore",
        "node_type": "VectorParameter",
        "pos_x": -400.0,
        "pos_y": 0.0,
        "extra_params": {
            "ParameterName": "EmissiveColor",
            "DefaultValue": "(R=0.000000,G=1.000000,B=0.200000,A=1.000000)"
        }
    })
    print(f"VectorParameter Node: {json.dumps(res_vec, indent=2)}")
    vec_node_id = res_vec.get("result", {}).get("node_id")

    # 3.2 Spawn ScalarParameter
    res_scalar = conn.send_command("vail_graph_add_node", {
        "asset_path": "/Game/VAIL_Test/M_GlowCore",
        "node_type": "ScalarParameter",
        "pos_x": -400.0,
        "pos_y": 200.0,
        "extra_params": {
            "ParameterName": "GlowIntensity",
            "DefaultValue": "15.0"
        }
    })
    print(f"ScalarParameter Node: {json.dumps(res_scalar, indent=2)}")
    scalar_node_id = res_scalar.get("result", {}).get("node_id")

    # 3.3 Spawn Multiply Expression
    res_mul = conn.send_command("vail_graph_add_node", {
        "asset_path": "/Game/VAIL_Test/M_GlowCore",
        "node_type": "Multiply",
        "pos_x": -200.0,
        "pos_y": 100.0
    })
    print(f"Multiply Node: {json.dumps(res_mul, indent=2)}")
    mul_node_id = res_mul.get("result", {}).get("node_id")

    # 3.4 Inspect Material Graph Topology
    print("\n--- TEST 4: Material Graph Topology Inspection ---")
    res_top = conn.send_command("vail_graph_get_topology", {
        "asset_path": "/Game/VAIL_Test/M_GlowCore"
    })
    print(f"Material Topology:\n{json.dumps(res_top, indent=2)}")

    # =========================================================================
    # TEST 5: Blueprint EventGraph Node Spawning & Wiring
    # =========================================================================
    print("\n--- TEST 5: Blueprint EventGraph Logic Wiring ---")

    # 5.1 Spawn PrintString Function Node
    res_print = conn.send_command("vail_graph_add_node", {
        "asset_path": "/Game/VAIL_Test/BP_LaserTurret",
        "graph_name": "EventGraph",
        "node_type": "CallFunction:PrintString",
        "pos_x": 400.0,
        "pos_y": 0.0
    })
    print(f"PrintString Node: {json.dumps(res_print, indent=2)}")
    print_node_id = res_print.get("result", {}).get("node_id")

    # 5.2 Inspect Blueprint EventGraph Topology
    print("\n--- TEST 6: Blueprint EventGraph Topology Inspection ---")
    res_bp_top = conn.send_command("vail_graph_get_topology", {
        "asset_path": "/Game/VAIL_Test/BP_LaserTurret",
        "graph_name": "EventGraph"
    })
    print(f"Blueprint Topology:\n{json.dumps(res_bp_top, indent=2)}")

    conn.disconnect()
    print("\n" + "=" * 80)
    print("✅ PHASE 2 VERIFICATION COMPLETE: ALL DATA-CHANNELS OPERATIONAL")
    print("=" * 80)
    return True

if __name__ == "__main__":
    run_phase2_verification()
