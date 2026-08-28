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

def fix_fcolor_sunset():
    conn = UnrealConnection()
    if not conn.connect():
        print("❌ Could not connect to Unreal Engine 5.8 on port 55557")
        return False

    print("🌅 Fixing FColor (0-255 scale) & Sun Pitch...")
    
    conn.send_command("vail_set_scope", {"scope": "DetailsPanel", "target": "DirectionalLight"})
    conn.send_command("vail_begin_batch", {"title": "Fix FColor Scale & Realistic Golden Hour"})

    # 1. LightColor in FColor (0-255 range, soft golden cream)
    res_col = conn.send_command("vail_set_property", {
        "property_id": "DirectionalLightComponent->LightColor",
        "value": "(R=255,G=235,B=205,A=255)"
    })
    print(f"Color: {json.dumps(res_col, indent=2)}")

    # 2. Intensity (20,000 Lux)
    res_int = conn.send_command("vail_set_property", {
        "property_id": "DirectionalLightComponent->Intensity",
        "value": "20000.0"
    })
    print(f"Intensity: {json.dumps(res_int, indent=2)}")

    # 3. Sun Pitch (-25 deg: golden hour slant across the sky, pointing down towards ground)
    res_rot = conn.send_command("vail_set_property", {
        "property_id": "DirectionalLightComponent->RelativeRotation",
        "value": "Pitch=-25.000000,Yaw=135.000000,Roll=0.000000"
    })
    print(f"Rotation: {json.dumps(res_rot, indent=2)}")

    # 4. Settle
    res_settle = conn.send_command("vail_wait_for", {"timeout_seconds": 3.0})
    conn.send_command("vail_end_batch", {})

    print(f"Settle: {json.dumps(res_settle, indent=2)}")
    conn.disconnect()
    print("✨ Golden Hour fixed!")
    return True

if __name__ == "__main__":
    fix_fcolor_sunset()
