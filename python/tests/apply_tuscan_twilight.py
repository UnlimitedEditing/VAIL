import sys
import os
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

def apply_tuscan_twilight():
    conn = UnrealConnection()
    if not conn.connect():
        print("❌ Could not connect to Unreal Engine 5.8 on port 55557")
        return False

    print("🌅 Applying Option B: Romantic Tuscan Twilight...")
    
    conn.send_command("vail_set_scope", {"scope": "DetailsPanel", "target": "DirectionalLight"})
    conn.send_command("vail_begin_batch", {"title": "Romantic Tuscan Twilight Lighting"})

    # 1. Soft Peach / Tuscan Warm Sun Color
    res_col = conn.send_command("vail_set_property", {
        "property_id": "DirectionalLightComponent->LightColor",
        "value": "(R=1.000000,G=0.750000,B=0.580000,A=1.000000)"
    })
    print(f"Color: {json.dumps(res_col, indent=2)}")

    # 2. Balanced Intensity (16,000 Lux for low twilight sun)
    res_int = conn.send_command("vail_set_property", {
        "property_id": "DirectionalLightComponent->Intensity",
        "value": "16000.0"
    })
    print(f"Intensity: {json.dumps(res_int, indent=2)}")

    # 3. Sun Pitch right on the mountain ridge (-8 deg)
    res_rot = conn.send_command("vail_set_property", {
        "property_id": "DirectionalLightComponent->RelativeRotation",
        "value": "Pitch=-8.000000,Yaw=145.000000,Roll=0.000000"
    })
    print(f"Rotation: {json.dumps(res_rot, indent=2)}")

    # 4. Settle & Commit Batch
    res_settle = conn.send_command("vail_wait_for", {"timeout_seconds": 3.0})
    conn.send_command("vail_end_batch", {})

    print(f"Settle: {json.dumps(res_settle, indent=2)}")
    conn.disconnect()
    print("✨ Tuscan Twilight successfully applied!")
    return True

if __name__ == "__main__":
    apply_tuscan_twilight()
