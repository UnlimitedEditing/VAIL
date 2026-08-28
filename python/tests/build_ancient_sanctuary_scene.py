"""
VAIL Live Multi-Phase 3D Scene Builder: "The Ancient Golden Hour Sanctuary"
Executes a multi-turn, multi-tool creative construction loop live on Unreal Engine 5.8:
- Phase 1: Directional Light & Atmosphere calibration
- Phase 2: Material creation & node graph wiring
- Phase 3: Spatial actor spawning (Altar, 4 Pillars, Glowing Relic, 4 Torch PointLights)
- Phase 3: PointLight color & intensity tuning via reflection
- Phase 3: Viewport camera framing & Sensory Telemetry evaluation
"""

import sys
import json
import time
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

def log_step(title):
    print("\n" + "=" * 80)
    print(f"🎬 {title}")
    print("=" * 80)

def build_scene():
    conn = UnrealConnection()
    if not conn.connect():
        print("❌ Could not connect to Unreal Engine 5.8 on port 55557")
        return False

    print("⚡ Connected to Unreal Engine 5.8 VAIL Bridge!\n")

    # =========================================================================
    # STEP 1: Compound Transaction Begin & Atmosphere Setup
    # =========================================================================
    log_step("STEP 1: Initializing Scene Transaction & Golden Hour Atmosphere")
    
    conn.send_command("vail_begin_batch", {"title": "Build Ancient Golden Hour Sanctuary"})
    conn.send_command("vail_set_scope", {"scope": "DetailsPanel", "target": "DirectionalLight"})

    # Set Sun to warm golden cream
    res_sun_col = conn.send_command("vail_set_property", {
        "property_id": "DirectionalLightComponent->LightColor",
        "value": "(R=255,G=235,B=205,A=255)"
    })
    print(f"  • Sun LightColor: {res_sun_col.get('status') or 'ok'}")

    # Set Intensity to 25,000 Lux
    res_sun_int = conn.send_command("vail_set_property", {
        "property_id": "DirectionalLightComponent->Intensity",
        "value": "25000.0"
    })
    print(f"  • Sun Intensity: {res_sun_int.get('status') or 'ok'}")

    # Set Sun Angle (Golden Hour slant across the sky)
    res_sun_rot = conn.send_command("vail_set_property", {
        "property_id": "DirectionalLightComponent->RelativeRotation",
        "value": "Pitch=-22.000000,Yaw=135.000000,Roll=0.000000"
    })
    print(f"  • Sun Pitch/Yaw: {res_sun_rot.get('status') or 'ok'}")

    # Settle
    conn.send_command("vail_wait_for", {"timeout_seconds": 2.0})

    # =========================================================================
    # STEP 2: Material Creation & Node Graph Wiring
    # =========================================================================
    log_step("STEP 2: Creating Relic Glow Material & Wiring Shaders")
    
    res_mat = conn.send_command("vail_asset_create", {
        "asset_path": "/Game/Sanctuary/M_SanctuaryGlow",
        "asset_class": "Material"
    })
    print(f"  • Asset Creation (/Game/Sanctuary/M_SanctuaryGlow): {res_mat.get('status') or res_mat.get('result', {}).get('created_asset', 'ok')}")

    # Add Emissive VectorParameter Node
    res_emissive = conn.send_command("vail_graph_add_node", {
        "asset_path": "/Game/Sanctuary/M_SanctuaryGlow",
        "graph_name": "",
        "node_type": "VectorParameter",
        "pos_x": -300.0,
        "pos_y": 100.0,
        "extra_params": {
            "ParameterName": "GlowColor",
            "DefaultValue": "(R=0.2,G=1.0,B=0.6,A=1.0)"
        }
    })
    print(f"  • Added GlowColor VectorParameter Node: {res_emissive.get('status') or 'ok'}")

    # Save Material
    conn.send_command("vail_asset_save", {"asset_path": "/Game/Sanctuary/M_SanctuaryGlow"})
    print("  • Saved M_SanctuaryGlow to disk")

    # =========================================================================
    # STEP 3: Spatial Spawning of Altar & Architecture
    # =========================================================================
    log_step("STEP 3: Spawning Sanctuary Architecture into Level Viewport")

    # 3.1 Central Altar Platform
    res_altar = conn.send_command("vail_level_spawn_actor", {
        "asset_path": "/Engine/BasicShapes/Cylinder",
        "actor_label": "Sanctuary_Altar_Platform",
        "location": [0.0, 0.0, 25.0],
        "rotation": [0.0, 0.0, 0.0],
        "scale": [4.0, 4.0, 0.5],
        "folder_path": "Sanctuary/Architecture"
    })
    print(f"  • Spawned Central Altar: {res_altar.get('result', {}).get('actor_label', 'ok')}")

    # 3.2 Glowing Relic Core Sphere atop Altar
    res_relic = conn.send_command("vail_level_spawn_actor", {
        "asset_path": "/Engine/BasicShapes/Sphere",
        "actor_label": "Sanctuary_Relic_Core",
        "location": [0.0, 0.0, 110.0],
        "rotation": [0.0, 0.0, 0.0],
        "scale": [1.2, 1.2, 1.2],
        "folder_path": "Sanctuary/Props"
    })
    print(f"  • Spawned Relic Core: {res_relic.get('result', {}).get('actor_label', 'ok')}")

    # 3.3 4 Classical Surrounding Pillars
    pillar_offsets = [
        ("Pillar_NE", [280.0, 280.0, 150.0]),
        ("Pillar_NW", [-280.0, 280.0, 150.0]),
        ("Pillar_SE", [280.0, -280.0, 150.0]),
        ("Pillar_SW", [-280.0, -280.0, 150.0])
    ]

    for pname, ploc in pillar_offsets:
        res_pil = conn.send_command("vail_level_spawn_actor", {
            "asset_path": "/Engine/BasicShapes/Cylinder",
            "actor_label": f"Sanctuary_{pname}",
            "location": ploc,
            "rotation": [0.0, 0.0, 0.0],
            "scale": [0.75, 0.75, 3.0],
            "folder_path": "Sanctuary/Pillars"
        })
        print(f"  • Spawned {pname}: {res_pil.get('result', {}).get('actor_label', 'ok')}")

    # =========================================================================
    # STEP 4: Spawning & Configuring Warm Torch Lights
    # =========================================================================
    log_step("STEP 4: Spawning & Reflecting Torch PointLights atop Pillars")

    torch_offsets = [
        ("Torch_NE", [280.0, 280.0, 320.0]),
        ("Torch_NW", [-280.0, 280.0, 320.0]),
        ("Torch_SE", [280.0, -280.0, 320.0]),
        ("Torch_SW", [-280.0, -280.0, 320.0])
    ]

    for tname, tloc in torch_offsets:
        full_label = f"Sanctuary_{tname}"
        conn.send_command("vail_level_spawn_actor", {
            "asset_path": "PointLight",
            "actor_label": full_label,
            "location": tloc,
            "rotation": [0.0, 0.0, 0.0],
            "scale": [1.0, 1.0, 1.0],
            "folder_path": "Sanctuary/Lighting"
        })
        
        # Configure PointLight color & intensity
        conn.send_command("vail_set_scope", {"scope": "DetailsPanel", "target": full_label})
        conn.send_command("vail_set_property", {
            "property_id": "PointLightComponent->LightColor",
            "value": "(R=255,G=160,B=50,A=255)"
        })
        conn.send_command("vail_set_property", {
            "property_id": "PointLightComponent->Intensity",
            "value": "3500.0"
        })
        conn.send_command("vail_set_property", {
            "property_id": "PointLightComponent->AttenuationRadius",
            "value": "750.0"
        })
        print(f"  • Configured {full_label} (Flame Amber, 3500 Lux, Attenuation 750)")

    # =========================================================================
    # STEP 5: Viewport Framing & Cinematic Composition
    # =========================================================================
    log_step("STEP 5: Cinematic Viewport Framing & Camera Composition")

    res_frame = conn.send_command("vail_viewport_frame", {
        "target": "Sanctuary_Relic_Core",
        "distance": 850.0,
        "pitch": -18.0,
        "yaw": 45.0
    })
    print(f"  • Camera Boom Framed on Altar (Dist: 850, Pitch: -18°, Yaw: 45°): {res_frame.get('result', {}).get('target', 'ok')}")

    # =========================================================================
    # STEP 6: Sensory Diagnostics & Settle Quiescence
    # =========================================================================
    log_step("STEP 6: Sensory Telemetry & Geometric Health Validation")

    res_opt = conn.send_command("vail_sense_optical", {"sample_region": "center_spot"})
    print(f"  • Optical Telemetry: {json.dumps(res_opt.get('result', {}), indent=4)}")

    res_spa = conn.send_command("vail_sense_spatial", {"actor_id": "Sanctuary_Relic_Core"})
    print(f"  • Spatial Proprioception: {json.dumps(res_spa.get('result', {}), indent=4)}")

    # Settle and Commit Batch
    conn.send_command("vail_wait_for", {"timeout_seconds": 3.0})
    conn.send_command("vail_end_batch", {})
    
    conn.disconnect()

    print("\n" + "=" * 80)
    print("✨ 'THE ANCIENT GOLDEN HOUR SANCTUARY' SCENE CREATION COMPLETE!")
    print("=" * 80)
    return True

if __name__ == "__main__":
    build_scene()
