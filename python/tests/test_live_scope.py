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

def test_properties_and_scope():
    conn = UnrealConnection()
    if not conn.connect():
        print("❌ Could not connect to Unreal Engine 5.8 on port 55557")
        return

    print("🔌 Connected to Live Session for Property & Scope Testing")

    # 1. Set scope to full asset path
    scope_res = conn.send_command("vail_set_scope", {
        "scope": "DetailsPanel",
        "target": "/Game/VAIL_Test/BP_LaserTurret"
    })
    print(f"Scope Asset Result: {json.dumps(scope_res, indent=2)}")

    # 2. Get tree
    tree_res = conn.send_command("vail_get_tree", {
        "max_depth": 2
    })
    print(f"Get Tree Result (success={tree_res.get('status') == 'success'}, categories={len(tree_res.get('result', {}).get('categories', []))})")

    # 3. Find commands or properties
    find_res = conn.send_command("vail_find", {
        "query": "Save"
    })
    print(f"Find 'Save' Result: {json.dumps(find_res, indent=2)}")

    conn.disconnect()

if __name__ == "__main__":
    test_properties_and_scope()
