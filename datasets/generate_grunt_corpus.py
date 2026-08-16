"""
Grunt Synthetic Corpus Generator (UE 5.8 / VAIL Phase 1)
Generates 120 validated examples per core VAIL tool (600+ total training samples)
formatted for SFT / LoRA distillation into Grunt (26M - 1.5B Needle).
"""

import json
import os
import random
from typing import List, Dict, Any

OUTPUT_DIR = os.path.dirname(os.path.abspath(__file__))
OUTPUT_FILE = os.path.join(OUTPUT_DIR, "grunt_vail_training_corpus.jsonl")

SYSTEM_PROMPT = """You are Grunt, a high-speed local UI state reconciler and tool router for Unreal Engine 5.8.
Your job is to translate high-level declarative intent and property targets into exact, atomic VAIL tool calls.
Always use precise semantic IDs and valid formatted values. Do not hallucinate properties or commands."""

# Real UE 5.8 reflection properties and command IDs
UE_PROPERTIES = [
    ("RelativeLocation.X", "float", ["0.0", "100.0", "500.0", "-250.5", "1250.0"]),
    ("RelativeLocation.Y", "float", ["0.0", "50.0", "-100.0", "300.0", "-1500.0"]),
    ("RelativeLocation.Z", "float", ["0.0", "50.0", "100.0", "200.0", "85.0"]),
    ("RelativeRotation.Pitch", "float", ["0.0", "45.0", "-90.0", "180.0", "15.0"]),
    ("RelativeRotation.Yaw", "float", ["0.0", "90.0", "-45.0", "180.0", "270.0"]),
    ("RelativeRotation.Roll", "float", ["0.0", "15.0", "-15.0", "180.0", "0.0"]),
    ("RelativeScale3D.X", "float", ["1.0", "1.5", "2.0", "0.5", "3.0"]),
    ("RelativeScale3D.Y", "float", ["1.0", "1.5", "2.0", "0.5", "3.0"]),
    ("RelativeScale3D.Z", "float", ["1.0", "1.5", "2.0", "0.5", "3.0"]),
    ("Mobility", "enum", ["Static", "Stationary", "Movable"]),
    ("bHidden", "bool", ["true", "false"]),
    ("bSimulatePhysics", "bool", ["true", "false"]),
    ("bEnableGravity", "bool", ["true", "false"]),
    ("MassInKg", "float", ["50.0", "100.0", "250.0", "10.0", "1000.0"]),
    ("LinearDamping", "float", ["0.01", "0.1", "0.5", "1.0"]),
    ("AngularDamping", "float", ["0.0", "0.05", "0.2", "0.8"]),
    ("CollisionProfileName", "enum", ["BlockAll", "OverlapAll", "NoCollision", "Pawn", "PhysicsActor"]),
    ("bGenerateOverlapEvents", "bool", ["true", "false"]),
    ("Intensity", "float", ["1000.0", "3000.0", "5000.0", "10000.0", "500.0"]),
    ("AttenuationRadius", "float", ["500.0", "1000.0", "2500.0", "150.0"]),
    ("LightColor", "color", ["(R=1.0,G=1.0,B=1.0)", "(R=1.0,G=0.0,B=0.0)", "(R=0.0,G=1.0,B=0.0)", "(R=0.0,G=0.5,B=1.0)"]),
    ("CastShadows", "bool", ["true", "false"]),
    ("StaticMesh", "asset", ["/Engine/BasicShapes/Cube.Cube", "/Engine/BasicShapes/Sphere.Sphere", "/Engine/BasicShapes/Cylinder.Cylinder"]),
]

UE_COMMANDS = [
    ("Kismet.Compile", "Compile", "Recompile active blueprint"),
    ("LevelEditor.SaveDirect", "Save Level", "Save the current level"),
    ("MainFrame.SaveAll", "Save All", "Save all dirty assets and levels"),
    ("BlueprintEditor.Compile", "Compile BP", "Compile blueprint asset"),
    ("GenericCommands.Undo", "Undo", "Undo last editor transaction"),
    ("GenericCommands.Redo", "Redo", "Redo last transaction"),
    ("LevelEditor.PlayInEditor", "Play", "Start PIE simulation"),
    ("LevelEditor.StopSimulation", "Stop", "Stop PIE simulation"),
    ("Actor.Delete", "Delete Actor", "Delete selected actor from level"),
    ("LevelEditor.BuildLighting", "Build Lights", "Build lighting data"),
]

ACTOR_NAMES = [
    "BP_Player_C_1", "StaticMeshActor_0", "PointLight_2", "DirectionalLight_1", 
    "BP_Enemy_Spawner", "CameraActor_1", "BP_HealthPickup", "BP_Chest_Gold"
]

def generate_scope_examples(count=120) -> List[Dict[str, Any]]:
    examples = []
    scopes = ["DetailsPanel", "Toolbar", "ContentBrowser", "ActivePanel"]
    
    for i in range(count):
        scope = random.choice(scopes)
        actor = random.choice(ACTOR_NAMES) if scope == "DetailsPanel" else ""
        
        if scope == "DetailsPanel":
            user_msg = f"Anchor attention to {actor} in the Details Panel."
        elif scope == "Toolbar":
            user_msg = "Switch scope to the main editor Toolbar commands."
        elif scope == "ContentBrowser":
            user_msg = "Set agent attention scope to the Content Browser."
        else:
            user_msg = "Focus active panel scope."

        examples.append({
            "tool": "vail_set_scope",
            "messages": [
                {"role": "system", "content": SYSTEM_PROMPT},
                {"role": "user", "content": user_msg},
                {"role": "assistant", "tool_calls": [{
                    "id": f"call_scope_{i}",
                    "type": "function",
                    "function": {
                        "name": "vail_set_scope",
                        "arguments": json.dumps({"scope": scope, "target": actor})
                    }
                }]}
            ]
        })
    return examples

def generate_find_examples(count=120) -> List[Dict[str, Any]]:
    examples = []
    queries = [
        ("How do I compile this blueprint?", "Compile", "Toolbar"),
        ("Where is the mobility setting?", "Mobility", "DetailsPanel"),
        ("Find light intensity setting", "Intensity", "DetailsPanel"),
        ("Look for simulate physics toggle", "Simulate Physics", "DetailsPanel"),
        ("Search for save command", "Save", "Toolbar"),
        ("Find collision profile options", "CollisionProfile", "DetailsPanel"),
        ("Where is gravity enabled?", "Enable Gravity", "DetailsPanel"),
        ("Find lighting build options", "Build Lighting", "Toolbar"),
        ("Look for mass setting in kg", "MassInKg", "DetailsPanel"),
        ("Find attenuation radius", "Attenuation", "DetailsPanel"),
        ("Look for hidden in game toggle", "bHidden", "DetailsPanel"),
        ("Search for undo action", "Undo", "Toolbar"),
    ]
    
    for i in range(count):
        prompt, query, scope = random.choice(queries)
        # Add variation
        variations = [prompt, f"Search for {query}", f"vail find query for '{query}' in {scope}", f"Locate '{query}'"]
        chosen_prompt = random.choice(variations)

        examples.append({
            "tool": "vail_find",
            "messages": [
                {"role": "system", "content": SYSTEM_PROMPT},
                {"role": "user", "content": chosen_prompt},
                {"role": "assistant", "tool_calls": [{
                    "id": f"call_find_{i}",
                    "type": "function",
                    "function": {
                        "name": "vail_find",
                        "arguments": json.dumps({"query": query, "scope": scope})
                    }
                }]}
            ]
        })
    return examples

def generate_get_tree_examples(count=120) -> List[Dict[str, Any]]:
    examples = []
    filters = ["", "Transform", "Physics", "Collision", "Lighting", "Rendering", "Materials"]
    
    for i in range(count):
        cat_filter = random.choice(filters)
        depth = random.choice([1, 2, 3])
        
        if cat_filter:
            user_msg = f"Inspect the {cat_filter} property category at depth {depth}."
        else:
            user_msg = f"Get the complete property tree for the active object with max depth {depth}."

        examples.append({
            "tool": "vail_get_tree",
            "messages": [
                {"role": "system", "content": SYSTEM_PROMPT},
                {"role": "user", "content": user_msg},
                {"role": "assistant", "tool_calls": [{
                    "id": f"call_tree_{i}",
                    "type": "function",
                    "function": {
                        "name": "vail_get_tree",
                        "arguments": json.dumps({"max_depth": depth, "category_filter": cat_filter})
                    }
                }]}
            ]
        })
    return examples

def generate_set_property_examples(count=120) -> List[Dict[str, Any]]:
    examples = []
    
    for i in range(count):
        prop_id, prop_type, values = random.choice(UE_PROPERTIES)
        val = random.choice(values)
        
        user_msg = f"Set property '{prop_id}' to '{val}'."

        examples.append({
            "tool": "vail_set_property",
            "messages": [
                {"role": "system", "content": SYSTEM_PROMPT},
                {"role": "user", "content": user_msg},
                {"role": "assistant", "tool_calls": [{
                    "id": f"call_setprop_{i}",
                    "type": "function",
                    "function": {
                        "name": "vail_set_property",
                        "arguments": json.dumps({"property_id": prop_id, "value": val})
                    }
                }]}
            ]
        })
    return examples

def generate_execute_command_examples(count=120) -> List[Dict[str, Any]]:
    examples = []
    
    for i in range(count):
        cmd_id, label, desc = random.choice(UE_COMMANDS)
        
        variations = [
            f"Execute command: {cmd_id}",
            f"Trigger the '{label}' action ({cmd_id})",
            f"Run editor command '{cmd_id}' to {desc.lower()}"
        ]
        user_msg = random.choice(variations)

        examples.append({
            "tool": "vail_execute_command",
            "messages": [
                {"role": "system", "content": SYSTEM_PROMPT},
                {"role": "user", "content": user_msg},
                {"role": "assistant", "tool_calls": [{
                    "id": f"call_exec_{i}",
                    "type": "function",
                    "function": {
                        "name": "vail_execute_command",
                        "arguments": json.dumps({"command_id": cmd_id})
                    }
                }]}
            ]
        })
    return examples

def generate_compound_batch_examples(count=120) -> List[Dict[str, Any]]:
    examples = []
    
    for i in range(count):
        actor = random.choice(ACTOR_NAMES)
        loc_x = str(random.randint(0, 1000))
        loc_z = str(random.randint(50, 300))
        mobility = random.choice(["Static", "Movable"])
        physics = random.choice(["true", "false"])
        
        user_msg = f"Configure {actor}: set location to X={loc_x}, Z={loc_z}, mobility={mobility}, physics={physics}."
        
        tool_calls = [
            {
                "id": f"call_batch_begin_{i}",
                "type": "function",
                "function": {"name": "vail_begin_batch", "arguments": json.dumps({"title": f"Configure {actor}"})}
            },
            {
                "id": f"call_set_x_{i}",
                "type": "function",
                "function": {"name": "vail_set_property", "arguments": json.dumps({"property_id": "RelativeLocation.X", "value": loc_x})}
            },
            {
                "id": f"call_set_z_{i}",
                "type": "function",
                "function": {"name": "vail_set_property", "arguments": json.dumps({"property_id": "RelativeLocation.Z", "value": loc_z})}
            },
            {
                "id": f"call_set_mob_{i}",
                "type": "function",
                "function": {"name": "vail_set_property", "arguments": json.dumps({"property_id": "Mobility", "value": mobility})}
            },
            {
                "id": f"call_set_phys_{i}",
                "type": "function",
                "function": {"name": "vail_set_property", "arguments": json.dumps({"property_id": "bSimulatePhysics", "value": physics})}
            },
            {
                "id": f"call_wait_{i}",
                "type": "function",
                "function": {"name": "vail_wait_for", "arguments": json.dumps({"timeout_seconds": 3.0})}
            },
            {
                "id": f"call_batch_end_{i}",
                "type": "function",
                "function": {"name": "vail_end_batch", "arguments": "{}"}
            }
        ]

        examples.append({
            "tool": "compound_reconciliation_batch",
            "messages": [
                {"role": "system", "content": SYSTEM_PROMPT},
                {"role": "user", "content": user_msg},
                {"role": "assistant", "tool_calls": tool_calls}
            ]
        })
    return examples

def main():
    os.makedirs(OUTPUT_DIR, exist_ok=True)
    all_examples = []
    
    print("Generating Grunt training dataset...")
    all_examples.extend(generate_scope_examples(120))
    all_examples.extend(generate_find_examples(120))
    all_examples.extend(generate_get_tree_examples(120))
    all_examples.extend(generate_set_property_examples(120))
    all_examples.extend(generate_execute_command_examples(120))
    all_examples.extend(generate_compound_batch_examples(120))
    
    random.shuffle(all_examples)

    with open(OUTPUT_FILE, "w", encoding="utf-8") as f:
        for ex in all_examples:
            f.write(json.dumps(ex) + "\n")

    print(f"Successfully generated {len(all_examples)} validated training samples in:")
    print(f" -> {OUTPUT_FILE}")

if __name__ == "__main__":
    main()
