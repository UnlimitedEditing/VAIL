"""
Grunt Phase 3 & Phase 4 Synthetic Worker Corpus Generator (UE 5.8 / VAIL)
Generates 3,000 schema-validated training pairs (120 samples per tool x 25 tools)
covering Spatial Spawning, UMG Widget Trees, Sequencer Timelines, Sensory Feedback,
Landscape, Foliage, Control Rig, MetaSounds, and Packaging Subsystems.
"""

import sys
import json
import random
from pathlib import Path

if sys.platform == "win32":
    try:
        sys.stdout.reconfigure(encoding="utf-8")
    except Exception:
        pass

_ROOT = Path(__file__).resolve().parent
OUTPUT_CORPUS = _ROOT / "grunt_vail_worker_phase3_4_3000.jsonl"

SYSTEM_PROMPT = """You are Grunt, a high-speed local UI state reconciler and tool router for Unreal Engine 5.8.
Your job is to translate high-level declarative intent, spatial viewport requests, UMG layouts, timeline tracks, and subsystem operations into exact, atomic VAIL tool calls.
Always use precise semantic IDs, valid coordinate vectors, and valid formatted arguments. Do not hallucinate invalid tool names or parameters."""

def make_sample(tool_name: str, user_query: str, arguments: dict, call_id: str) -> dict:
    return {
        "tool": tool_name,
        "messages": [
            {"role": "system", "content": SYSTEM_PROMPT},
            {"role": "user", "content": user_query},
            {
                "role": "assistant",
                "tool_calls": [
                    {
                        "id": call_id,
                        "type": "function",
                        "function": {
                            "name": tool_name,
                            "arguments": json.dumps(arguments, ensure_ascii=False)
                        }
                    }
                ]
            }
        ]
    }

def generate_worker_corpus():
    random.seed(42)
    samples = []
    print("=" * 80)
    print("🚀 GENERATING PHASE 3 + PHASE 4 SYNTHETIC WORKER CORPUS (3,000 SAMPLES)")
    print("=" * 80)

    # -------------------------------------------------------------------------
    # PHASE 3: SPATIAL & LEVEL VIEWPORT (4 tools * 120 = 480 samples)
    # -------------------------------------------------------------------------

    # 1. vail_level_spawn_actor (120)
    spawnable_assets = [
        ("/Game/Meshes/SM_DungeonPillar", "DungeonPillar", "Architecture"),
        ("/Game/Meshes/SM_TorchHolder", "TorchWall", "Lighting"),
        ("/Game/Blueprints/BP_EnemyOrc", "EnemyOrc", "Characters"),
        ("/Game/Blueprints/BP_TreasureChest", "TreasureChest", "Props/Interactable"),
        ("/Engine/BasicShapes/Cube", "PlatformBlock", "Geometry"),
        ("PointLight", "RoomFillLight", "Lighting"),
        ("DirectionalLight", "SunLight", "Lighting/Environment"),
        ("SpotLight", "StageSpotlight", "Lighting")
    ]
    for i in range(120):
        asset, base_label, folder = random.choice(spawnable_assets)
        label = f"{base_label}_{i+1:02d}"
        loc = [round(random.uniform(-2000, 2000), 1), round(random.uniform(-2000, 2000), 1), round(random.uniform(0, 500), 1)]
        rot = [0.0, round(random.choice([0, 45, 90, 180, 270]), 1), 0.0]
        scale = [round(random.choice([0.5, 1.0, 1.5, 2.0]), 1)] * 3
        
        args = {
            "asset_path": asset,
            "actor_label": label,
            "location": loc,
            "rotation": rot,
            "scale": scale,
            "folder_path": folder
        }
        query = f"Spawn '{asset}' at location {loc} with yaw {rot[1]} and label '{label}' into folder '{folder}'."
        samples.append(make_sample("vail_level_spawn_actor", query, args, f"call_spawn_{i:03d}"))

    # 2. vail_level_query_actors (120)
    patterns = ["*Light*", "*Enemy*", "SM_*", "*Chest*", "Platform*", "*Wall*", "BP_*", "*Boss*"]
    classes = ["PointLight", "DirectionalLight", "StaticMeshActor", "Character", "SpotLight", ""]
    for i in range(120):
        pat = random.choice(patterns)
        cls = random.choice(classes)
        limit = random.choice([10, 25, 50, 100])
        args = {"pattern": pat, "class_filter": cls, "tag_filter": "", "max_results": limit}
        query = f"Find all level actors matching wildcard '{pat}' of class '{cls or 'any'}' (limit {limit})."
        samples.append(make_sample("vail_level_query_actors", query, args, f"call_query_{i:03d}"))

    # 3. vail_level_delete_actor (120)
    for i in range(120):
        target = f"Old_TemporaryActor_{i+1:03d}"
        args = {"actor_id": target}
        query = f"Delete the actor named '{target}' from the level."
        samples.append(make_sample("vail_level_delete_actor", query, args, f"call_del_{i:03d}"))

    # 4. vail_viewport_frame (120)
    for i in range(120):
        target = f"Hero_Player_{i:02d}" if i % 2 == 0 else f"{random.randint(-1000, 1000)},{random.randint(-1000, 1000)},{random.randint(0, 500)}"
        dist = float(random.choice([300, 500, 800, 1200]))
        pitch = float(random.choice([-15, -25, -35, -45]))
        yaw = float(random.choice([0, 45, 90, 135, 180]))
        args = {"target": target, "distance": dist, "pitch": pitch, "yaw": yaw}
        query = f"Frame viewport camera on '{target}' at distance {dist}, pitch {pitch} deg, yaw {yaw} deg."
        samples.append(make_sample("vail_viewport_frame", query, args, f"call_frame_{i:03d}"))

    # -------------------------------------------------------------------------
    # PHASE 3: UMG WIDGET TREE DATA-CHANNEL (4 tools * 120 = 480 samples)
    # -------------------------------------------------------------------------

    # 5. vail_widget_tree_get (120)
    widgets = ["/Game/UI/WBP_MainMenu", "/Game/UI/WBP_HUD", "/Game/UI/WBP_InventoryGrid", "/Game/UI/WBP_PauseMenu", "/Game/UI/WBP_DialogueBox"]
    for i in range(120):
        w = random.choice(widgets)
        args = {"widget_path": w}
        query = f"Inspect the complete widget hierarchy and slots for widget blueprint '{w}'."
        samples.append(make_sample("vail_widget_tree_get", query, args, f"call_wtree_{i:03d}"))

    # 6. vail_widget_add_element (120)
    elem_types = ["Button", "TextBlock", "Image", "ProgressBar", "VerticalBox", "HorizontalBox", "Border"]
    for i in range(120):
        w = random.choice(widgets)
        etype = random.choice(elem_types)
        ename = f"UI_{etype}_{i+1:02d}"
        parent = random.choice(["RootCanvas", "ContentBox", "MainOverlay", "HeaderPanel"])
        args = {
            "widget_path": w,
            "element_type": etype,
            "element_name": ename,
            "parent_name": parent,
            "slot_properties": {"anchors": [0.5, 0.5, 0.5, 0.5], "size": [200, 50]},
            "content_properties": {"text": "Click Me"} if etype in ["Button", "TextBlock"] else {}
        }
        query = f"In widget '{w}', add a {etype} named '{ename}' under parent '{parent}'."
        samples.append(make_sample("vail_widget_add_element", query, args, f"call_wadd_{i:03d}"))

    # 7. vail_widget_set_slot (120)
    for i in range(120):
        w = random.choice(widgets)
        ename = f"UI_Element_{i:02d}"
        layout = {"anchors": [0.5, 0.5, 0.5, 0.5], "position": [random.randint(-200, 200), random.randint(-100, 100)], "size": [random.choice([150, 200, 300]), random.choice([40, 60, 80])], "alignment": [0.5, 0.5]}
        args = {"widget_path": w, "element_name": ename, "layout": layout, "slot_type": "CanvasSlot"}
        query = f"Update slot layout for widget '{ename}' in '{w}' with position {layout['position']} and size {layout['size']}."
        samples.append(make_sample("vail_widget_set_slot", query, args, f"call_wslot_{i:03d}"))

    # 8. vail_widget_bind_event (120)
    events = ["OnClicked", "OnHovered", "OnUnhovered", "OnPressed", "OnReleased"]
    for i in range(120):
        w = random.choice(widgets)
        ename = f"Btn_Action_{i:02d}"
        ev = random.choice(events)
        fn = f"Handle_{ename}_{ev}"
        args = {"widget_path": w, "element_name": ename, "event_name": ev, "function_name": fn}
        query = f"Bind event '{ev}' on button '{ename}' in '{w}' to handler function '{fn}'."
        samples.append(make_sample("vail_widget_bind_event", query, args, f"call_wbind_{i:03d}"))

    # -------------------------------------------------------------------------
    # PHASE 3: SEQUENCER & CINEMATICS (3 tools * 120 = 360 samples)
    # -------------------------------------------------------------------------

    # 9. vail_sequencer_query (120)
    seqs = ["/Game/Cinematics/LS_IntroScene", "/Game/Cinematics/LS_BossCutscene", "/Game/Cinematics/LS_VictoryScreen"]
    for i in range(120):
        s = random.choice(seqs)
        args = {"sequence_path": s}
        query = f"Query all tracks, bindings, and sections in level sequence '{s}'."
        samples.append(make_sample("vail_sequencer_query", query, args, f"call_squery_{i:03d}"))

    # 10. vail_sequencer_add_track (120)
    ttypes = [("Transform", "CineCameraActor_1", ""), ("Property", "DirectionalLight", "Intensity"), ("Audio", "MusicEmitter", ""), ("Fade", "", "")]
    for i in range(120):
        s = random.choice(seqs)
        tt, obj, prop = random.choice(ttypes)
        args = {"sequence_path": s, "track_type": tt, "target_object": obj, "property_path": prop}
        query = f"Add a '{tt}' track for object '{obj or 'Camera'}' to sequence '{s}'."
        samples.append(make_sample("vail_sequencer_add_track", query, args, f"call_strack_{i:03d}"))

    # 11. vail_sequencer_add_key (120)
    for i in range(120):
        s = random.choice(seqs)
        frame = random.choice([0, 30, 60, 90, 120, 150, 180])
        val = random.choice(["X=0 Y=0 Z=100", "50000.0", "(R=1,G=0.8,B=0.4,A=1)", "0.0"])
        args = {"sequence_path": s, "track_id": f"Track_Transform_{i:02d}", "frame_number": frame, "value": val, "interp_mode": "Cubic"}
        query = f"Set keyframe at frame {frame} with value '{val}' on track 'Track_Transform_{i:02d}' in sequence '{s}'."
        samples.append(make_sample("vail_sequencer_add_key", query, args, f"call_skey_{i:03d}"))

    # -------------------------------------------------------------------------
    # PHASE 3: SENSORY TELEMETRY DIAGNOSTICS (4 tools * 120 = 480 samples)
    # -------------------------------------------------------------------------

    # 12. vail_sense_optical (120)
    for i in range(120):
        cam = random.choice(["", "CineCamera_Hero", "CineCamera_WideShot"])
        reg = random.choice(["full", "center_spot", "quadrants"])
        args = {"target_camera": cam, "sample_region": reg}
        query = f"Measure optical rendering telemetry (luminance, hue, EV100) using sampling region '{reg}'."
        samples.append(make_sample("vail_sense_optical", query, args, f"call_opt_{i:03d}"))

    # 13. vail_sense_spatial (120)
    for i in range(120):
        target = f"Actor_Prop_{i:02d}"
        ch = random.choice(["Visibility", "WorldStatic", "Camera"])
        args = {"actor_id": target, "trace_channel": ch}
        query = f"Calculate spatial clearance, ground normal, and penetration depth for actor '{target}'."
        samples.append(make_sample("vail_sense_spatial", query, args, f"call_spa_{i:03d}"))

    # 14. vail_sense_mesh (120)
    meshes = ["/Game/Meshes/SM_HeroCastle", "/Game/Meshes/SM_DragonStatue", "/Game/Meshes/SM_AncientTree", "StaticMeshActor_5"]
    for i in range(120):
        m = random.choice(meshes)
        args = {"asset_or_actor": m}
        query = f"Inspect geometric health, triangle count, Nanite status, and UV overlaps for '{m}'."
        samples.append(make_sample("vail_sense_mesh", query, args, f"call_msh_{i:03d}"))

    # 15. vail_sense_shader (120)
    mats = ["/Game/Materials/M_HeroLandscape", "/Game/Materials/M_WaterOcean", "/Game/Materials/M_GlowingObsidian"]
    for i in range(120):
        mat = random.choice(mats)
        args = {"material_path": mat}
        query = f"Measure shader complexity, instruction counts, and Lumen surface cache validity for '{mat}'."
        samples.append(make_sample("vail_sense_shader", query, args, f"call_shd_{i:03d}"))

    # -------------------------------------------------------------------------
    # PHASE 4: SUBSYSTEMS & PRODUCTION TOOLING (10 tools * 120 = 1,200 samples)
    # -------------------------------------------------------------------------

    # 16. vail_landscape_create (120)
    for i in range(120):
        sec = random.choice([31, 63, 127])
        cx = random.choice([4, 8, 16])
        mat = "/Game/Materials/M_LandscapeMaster"
        args = {"section_size": sec, "sections_per_component": 1, "component_count_x": cx, "component_count_y": cx, "material_path": mat, "location": [0.0, 0.0, 0.0]}
        query = f"Create a new landscape with section size {sec}x{sec} ({cx}x{cx} components) using material '{mat}'."
        samples.append(make_sample("vail_landscape_create", query, args, f"call_lcreate_{i:03d}"))

    # 17. vail_landscape_sculpt (120)
    modes = ["Sculpt", "Smooth", "Flatten", "Ramp", "Erosion"]
    for i in range(120):
        mode = random.choice(modes)
        loc = [round(random.uniform(-5000, 5000), 1), round(random.uniform(-5000, 5000), 1), 0.0]
        rad = float(random.choice([1024, 2048, 4096]))
        st = round(random.uniform(0.1, 0.8), 2)
        args = {"target_location": loc, "brush_mode": mode, "radius": rad, "falloff": 0.5, "strength": st}
        query = f"Apply landscape {mode} brush at {loc} with radius {rad} and strength {st}."
        samples.append(make_sample("vail_landscape_sculpt", query, args, f"call_lsculpt_{i:03d}"))

    # 18. vail_landscape_paint (120)
    layers = ["Grass", "Rock", "Mud", "Snow", "Sand", "Cobblestone"]
    for i in range(120):
        layer = random.choice(layers)
        loc = [round(random.uniform(-4000, 4000), 1), round(random.uniform(-4000, 4000), 1), 0.0]
        rad = float(random.choice([512, 1024, 2048]))
        args = {"layer_name": layer, "target_location": loc, "radius": rad, "strength": 1.0}
        query = f"Paint landscape layer '{layer}' at {loc} with brush radius {rad}."
        samples.append(make_sample("vail_landscape_paint", query, args, f"call_lpaint_{i:03d}"))

    # 19. vail_foliage_scatter (120)
    foliage_meshes = ["/Game/Environment/Trees/SM_PineTree_01", "/Game/Environment/Rocks/SM_CliffRock", "/Game/Environment/Foliage/SM_FernBush"]
    for i in range(120):
        fmesh = random.choice(foliage_meshes)
        center = [round(random.uniform(-3000, 3000), 1), round(random.uniform(-3000, 3000), 1), 0.0]
        dens = random.choice([25, 50, 100, 200])
        args = {
            "mesh_path": fmesh,
            "center_location": center,
            "radius": 3000.0,
            "density": dens,
            "scale_min": 0.8,
            "scale_max": 1.2,
            "align_to_normal": True,
            "random_yaw": True
        }
        query = f"Scatter {dens} foliage instances of '{fmesh}' around {center} within radius 3000."
        samples.append(make_sample("vail_foliage_scatter", query, args, f"call_fscatter_{i:03d}"))

    # 20. vail_foliage_query (120)
    for i in range(120):
        center = [round(random.uniform(-2000, 2000), 1), round(random.uniform(-2000, 2000), 1), 0.0]
        rad = float(random.choice([3000, 5000, 8000]))
        args = {"bounds_center": center, "bounds_radius": rad}
        query = f"Inspect foliage instance density and species coverage around {center} within radius {rad}."
        samples.append(make_sample("vail_foliage_query", query, args, f"call_fquery_{i:03d}"))

    # 21. vail_control_rig_set_transform (120)
    ctrls = ["hand_r_ctrl", "hand_l_ctrl", "head_ctrl", "pelvis_ctrl", "foot_r_ik_ctrl", "foot_l_ik_ctrl"]
    for i in range(120):
        rig = "/Game/Characters/Mannequin/CR_Mannequin"
        c = random.choice(ctrls)
        loc = [round(random.uniform(-50, 50), 1), round(random.uniform(-50, 50), 1), round(random.uniform(50, 150), 1)]
        rot = [round(random.uniform(-30, 30), 1), round(random.uniform(-30, 30), 1), 0.0]
        args = {"rig_path": rig, "control_name": c, "location": loc, "rotation": rot}
        query = f"Set Control Rig '{rig}' control '{c}' to location {loc} and rotation {rot}."
        samples.append(make_sample("vail_control_rig_set_transform", query, args, f"call_crset_{i:03d}"))

    # 22. vail_control_rig_query (120)
    rigs = ["/Game/Characters/Mannequin/CR_Mannequin", "/Game/Characters/Monsters/CR_BossDragon", "/Game/Vehicles/CR_HoverTank"]
    for i in range(120):
        r = random.choice(rigs)
        args = {"rig_path": r}
        query = f"Query control hierarchies, available bone solvers, and limits for Control Rig '{r}'."
        samples.append(make_sample("vail_control_rig_query", query, args, f"call_crquery_{i:03d}"))

    # 23. vail_audio_play (120)
    sounds = ["/Game/Audio/MS_Ambience_Forest", "/Game/Audio/SFX_Explosion_Huge", "/Game/Audio/SFX_ChestOpen", "/Game/Audio/MS_Footsteps_Grass"]
    for i in range(120):
        snd = random.choice(sounds)
        loc = [round(random.uniform(-1000, 1000), 1), round(random.uniform(-1000, 1000), 1), 100.0] if i % 2 == 0 else None
        vol = round(random.choice([0.7, 0.9, 1.0, 1.2]), 2)
        args = {"sound_path": snd, "location": loc, "volume_multiplier": vol, "pitch_multiplier": 1.0}
        query = f"Audition audio asset '{snd}' at {loc or '2D'} with volume {vol}."
        samples.append(make_sample("vail_audio_play", query, args, f"call_aplay_{i:03d}"))

    # 24. vail_audio_set_parameter (120)
    params = [("PitchMod", "1.2"), ("ReverbGain", "0.45"), ("TriggerPlay", "true"), ("CutoffFrequency", "4500.0")]
    for i in range(120):
        actor = f"AudioComponent_{i:02d}"
        pname, val = random.choice(params)
        args = {"sound_actor": actor, "parameter_name": pname, "value": val}
        query = f"Set MetaSound parameter '{pname}' to '{val}' on audio component '{actor}'."
        samples.append(make_sample("vail_audio_set_parameter", query, args, f"call_aparam_{i:03d}"))

    # 25. vail_project_build (120)
    configs = ["Development", "Shipping", "DebugGame"]
    plats = ["Windows", "Linux", "Android"]
    for i in range(120):
        plat = random.choice(plats)
        cfg = random.choice(configs)
        clean = random.choice([True, False])
        args = {"target_platform": plat, "build_config": cfg, "clean": clean}
        query = f"Trigger project cook and packaging for '{plat}' in '{cfg}' configuration (clean={clean})."
        samples.append(make_sample("vail_project_build", query, args, f"call_build_{i:03d}"))

    # Shuffle dataset
    random.shuffle(samples)

    # Write out JSONL file
    with open(OUTPUT_CORPUS, "w", encoding="utf-8") as f:
        for s in samples:
            f.write(json.dumps(s, ensure_ascii=False) + "\n")

    print(f"\n[SUCCESS] Generated {len(samples)} synthetic worker training samples in:")
    print(f" -> {OUTPUT_CORPUS}")
    return len(samples)

if __name__ == "__main__":
    generate_worker_corpus()
