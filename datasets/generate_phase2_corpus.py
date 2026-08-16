import json
import random
import sys
from pathlib import Path

if sys.platform == "win32":
    try:
        sys.stdout.reconfigure(encoding="utf-8")
    except Exception:
        pass

_ROOT = Path(__file__).resolve().parent

# Common blueprint functions
BP_FUNCTIONS = [
    "PrintString", "SetActorLocation", "GetActorLocation", "SetActorRotation", "GetActorRotation",
    "SpawnActor", "DestroyActor", "PlaySound2D", "PlaySoundAtLocation", "AddActorWorldOffset",
    "SetActorScale3D", "GetActorForwardVector", "LineTraceByChannel", "ApplyDamage", "SetVisibility",
    "SetMaterial", "SetPhysicsLinearVelocity", "AddImpulse", "Delay", "RetriggerableDelay"
]

BP_EVENTS = [
    "ReceiveBeginPlay", "ReceiveTick", "ReceiveActorBeginOverlap", "ReceiveActorEndOverlap",
    "ReceivePointDamage", "ReceiveRadialDamage", "ReceiveDestroyed"
]

K2_NODES = [
    "IfThenElse", "ExecutionSequence", "K2Node_SpawnActorFromClass", "K2Node_CallFunction",
    "K2Node_Event", "K2Node_VariableGet", "K2Node_VariableSet", "K2Node_MakeArray"
]

MAT_EXPRESSIONS = [
    ("VectorParameter", "EmissiveColor", "(R=1.0,G=0.5,B=0.0,A=1.0)"),
    ("VectorParameter", "BaseColorTint", "(R=0.2,G=0.8,B=0.2,A=1.0)"),
    ("ScalarParameter", "RoughnessValue", "0.35"),
    ("ScalarParameter", "MetallicValue", "0.9"),
    ("ScalarParameter", "GlowMultiplier", "12.0"),
    ("Multiply", "", ""),
    ("Add", "", ""),
    ("Constant", "DefaultFloat", "1.0"),
    ("TextureSample", "AlbedoTexture", ""),
    ("Time", "", ""),
    ("Sine", "", ""),
    ("Cosine", "", "")
]

ASSET_PATHS = [
    ("/Game/Blueprints/BP_PlayerCharacter", "Blueprint", "Character"),
    ("/Game/Blueprints/BP_EnemyPawn", "Blueprint", "Pawn"),
    ("/Game/Blueprints/BP_InteractiveChest", "Blueprint", "Actor"),
    ("/Game/Blueprints/BP_DoorTrigger", "Blueprint", "Actor"),
    ("/Game/Materials/M_GlowingObsidian", "Material", ""),
    ("/Game/Materials/M_SciFiMetal", "Material", ""),
    ("/Game/Materials/M_WaterSurface", "Material", ""),
    ("/Game/Materials/Instances/MI_MetalGold", "MaterialInstanceConstant", "")
]

SYSTEM_PROMPT = """You are Grunt, a high-speed local UI state reconciler and tool router for Unreal Engine 5.8.
Your job is to translate high-level declarative intent, graph topology requirements, and property targets into exact, atomic VAIL tool calls.
Always use precise semantic IDs, pin identifiers, and valid formatted values. Do not hallucinate properties, node types, or commands."""

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

def generate_master_corpus():
    samples = []
    
    # 1. Load Phase 1 (720 samples) if present
    phase1_path = _ROOT / "grunt_vail_training_corpus.jsonl"
    if phase1_path.exists():
        for line in phase1_path.read_text(encoding="utf-8").strip().split("\n"):
            if line:
                samples.append(json.loads(line))
        print(f"Loaded {len(samples)} Phase 1 training samples.")

    # 2. Generate Phase 2 (720 new samples: 120 per tool)

    # 2.1 vail_graph_add_node (120 samples)
    for i in range(120):
        asset = random.choice(ASSET_PATHS)
        pos_x = random.randint(-600, 800)
        pos_y = random.randint(-400, 600)
        call_id = f"call_addnode_{i+1:03d}"

        if "Blueprint" in asset[1]:
            choice = random.choice(["func", "event", "generic"])
            if choice == "func":
                fn = random.choice(BP_FUNCTIONS)
                query = f"In '{asset[0]}', add a '{fn}' function call node at ({pos_x}, {pos_y})."
                args = {"asset_path": asset[0], "graph_name": "EventGraph", "node_type": f"CallFunction:{fn}", "pos_x": float(pos_x), "pos_y": float(pos_y)}
            elif choice == "event":
                ev = random.choice(BP_EVENTS)
                query = f"Spawn event '{ev}' in EventGraph of '{asset[0]}' at coordinates ({pos_x}, {pos_y})."
                args = {"asset_path": asset[0], "graph_name": "EventGraph", "node_type": f"Event:{ev}", "pos_x": float(pos_x), "pos_y": float(pos_y)}
            else:
                node = random.choice(K2_NODES)
                query = f"Add a '{node}' node to '{asset[0]}' graph at ({pos_x}, {pos_y})."
                args = {"asset_path": asset[0], "graph_name": "EventGraph", "node_type": node, "pos_x": float(pos_x), "pos_y": float(pos_y)}
        else:
            expr, param_name, def_val = random.choice(MAT_EXPRESSIONS)
            extra = {}
            if param_name:
                extra["ParameterName"] = param_name
            if def_val:
                extra["DefaultValue"] = def_val
            query = f"Add a '{expr}' expression to material '{asset[0]}' at ({pos_x}, {pos_y})."
            args = {"asset_path": asset[0], "graph_name": "MaterialGraph", "node_type": expr, "pos_x": float(pos_x), "pos_y": float(pos_y)}
            if extra:
                args["extra_params"] = extra

        samples.append(make_sample("vail_graph_add_node", query, args, call_id))

    # 2.2 vail_graph_connect_pins (120 samples)
    PIN_PAIRS = [
        ("K2Node_Event_0:then", "K2Node_CallFunction_0:execute"),
        ("K2Node_CallFunction_0:then", "K2Node_CallFunction_1:execute"),
        ("K2Node_CallFunction_0:ReturnValue", "K2Node_CallFunction_1:InLocation"),
        ("MaterialExpressionVectorParameter_0:RGBA", "MaterialGraphNode_Root_0:BaseColor"),
        ("MaterialExpressionScalarParameter_0:Output", "MaterialGraphNode_Root_0:Roughness"),
        ("MaterialExpressionMultiply_0:Output", "MaterialGraphNode_Root_0:EmissiveColor"),
        ("MaterialExpressionTime_0:Output", "MaterialExpressionSine_0:Input"),
        ("MaterialExpressionSine_0:Output", "MaterialExpressionMultiply_0:A"),
        ("MaterialExpressionTextureSample_0:RGB", "MaterialGraphNode_Root_0:BaseColor")
    ]
    for i in range(120):
        asset = random.choice(ASSET_PATHS)
        src_pin, tgt_pin = random.choice(PIN_PAIRS)
        query = f"Connect '{src_pin}' to '{tgt_pin}' in graph for '{asset[0]}'."
        args = {"asset_path": asset[0], "source_pin": src_pin, "target_pin": tgt_pin}
        samples.append(make_sample("vail_graph_connect_pins", query, args, f"call_conn_{i+1:03d}"))

    # 2.3 vail_graph_get_topology (120 samples)
    for i in range(120):
        asset = random.choice(ASSET_PATHS)
        gname = "EventGraph" if "Blueprint" in asset[1] else "MaterialGraph"
        query = f"Get the topology and active node links for '{asset[0]}' ({gname})."
        args = {"asset_path": asset[0], "graph_name": gname}
        samples.append(make_sample("vail_graph_get_topology", query, args, f"call_top_{i+1:03d}"))

    # 2.4 vail_graph_delete_node (120 samples)
    for i in range(120):
        asset = random.choice(ASSET_PATHS)
        node_id = f"K2Node_{random.choice(BP_FUNCTIONS)}_{i}" if "Blueprint" in asset[1] else f"MaterialExpressionMultiply_{i}"
        query = f"Delete node '{node_id}' from graph '{asset[0]}'."
        args = {"asset_path": asset[0], "node_id": node_id}
        samples.append(make_sample("vail_graph_delete_node", query, args, f"call_del_{i+1:03d}"))

    # 2.5 vail_asset_create (120 samples)
    for i in range(120):
        asset_path, asset_class, parent_class = random.choice(ASSET_PATHS)
        unique_path = f"{asset_path}_{i+1}"
        query = f"Create a new {asset_class} named '{unique_path.split('/')[-1]}' in '{unique_path.rsplit('/', 1)[0]}'."
        args = {"asset_path": unique_path, "asset_class": asset_class}
        if parent_class:
            args["parent_class"] = parent_class
            query += f" inheriting from '{parent_class}'"
        samples.append(make_sample("vail_asset_create", query, args, f"call_create_{i+1:03d}"))

    # 2.6 vail_asset_query (120 samples)
    PACKAGES = ["/Game", "/Game/Blueprints", "/Game/Materials", "/Game/Characters", "/Game/Environment", "/Game/VFX"]
    CLASSES = ["", "Blueprint", "Material", "MaterialInstanceConstant", "World", "StaticMesh"]
    for i in range(120):
        pkg = random.choice(PACKAGES)
        cls_filt = random.choice(CLASSES)
        query = f"Query assets in '{pkg}'" + (f" with class filter '{cls_filt}'." if cls_filt else ".")
        args = {"package_path": pkg}
        if cls_filt:
            args["class_filter"] = cls_filt
        samples.append(make_sample("vail_asset_query", query, args, f"call_query_{i+1:03d}"))

    out_file = _ROOT / "grunt_vail_master_corpus_1440.jsonl"
    with open(out_file, "w", encoding="utf-8") as f:
        for s in samples:
            f.write(json.dumps(s, ensure_ascii=False) + "\n")

    print(f"✅ Generated {len(samples)} master training samples in {out_file}")

if __name__ == "__main__":
    generate_master_corpus()
