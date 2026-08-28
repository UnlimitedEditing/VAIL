"""
One task per real VAIL tool (see grunt/tools/, minus the quarantined
_unimplemented/ spatial_* schemas) -- the full 40-tool sweep for
haiku_tool_trial.py --full.

Each prompt is written to plausibly require exactly the named tool, so a
clean trial's tool_calls should show that tool exercised at least once.
Some tasks may reasonably pull in a companion call (e.g. vail_set_scope
before a property read) -- that's expected and fine; what matters is the
target tool gets exercised and Haiku's arguments for it are inspectable.

Paths/names used (BP_Player, /Game/Blueprints/..., DirectionalLight, etc.)
match the conventions already used elsewhere in this repo's docs/benchmarks
so results are comparable across scripts.
"""

from typing import Dict, List

FULL_SWEEP_TASKS: List[Dict[str, str]] = [
    {"scenario_id": "vail_set_scope", "prompt": "Set your attention scope to the actor 'DirectionalLight' in the Details panel."},
    {"scenario_id": "vail_get_tree", "prompt": "Get the compact property tree for the currently scoped actor, up to depth 2."},
    {"scenario_id": "vail_find", "prompt": "Search for the editor command or property related to 'Compile' anywhere in the editor."},
    {"scenario_id": "vail_execute_command", "prompt": "Execute the editor command with semantic ID 'Kismet.Compile'."},
    {"scenario_id": "vail_set_property", "prompt": "Set the property 'Mobility' on the currently scoped actor to 'Movable'."},
    {"scenario_id": "vail_wait_for", "prompt": "Wait for the editor UI and any background compilation to fully settle, up to 5 seconds."},
    {"scenario_id": "vail_begin_batch", "prompt": "Begin a compound transaction batch titled 'Sweep Test Batch' so the next edits group into one Undo step."},
    {"scenario_id": "vail_end_batch", "prompt": "Commit and close the currently active compound transaction batch."},
    {"scenario_id": "vail_graph_get_topology", "prompt": "Get the node graph topology (nodes, pins, wires) of the Blueprint asset at /Game/Blueprints/BP_Player."},
    {"scenario_id": "vail_graph_add_node", "prompt": "Add a new 'PrintString' node at position (200, 300) into the EventGraph of the Blueprint at /Game/Blueprints/BP_Player."},
    {"scenario_id": "vail_graph_connect_pins", "prompt": "Connect the output pin 'Then' of node 'EventBeginPlay' to the input pin 'exec' of node 'PrintString' in the EventGraph of /Game/Blueprints/BP_Player."},
    {"scenario_id": "vail_graph_delete_node", "prompt": "Delete the node with ID 'PrintString_1' from the EventGraph of the Blueprint at /Game/Blueprints/BP_Player."},
    {"scenario_id": "vail_asset_create", "prompt": "Create a new Blueprint asset at /Game/Blueprints/BP_TestSweep with parent class 'Actor'."},
    {"scenario_id": "vail_asset_query", "prompt": "Query the Asset Registry for all assets under package path /Game/Blueprints of class 'Blueprint'."},
    {"scenario_id": "vail_asset_save", "prompt": "Save the asset package at /Game/Blueprints/BP_Player to disk."},
    {"scenario_id": "vail_level_spawn_actor", "prompt": "Spawn a new actor labeled 'TestCube' from asset /Engine/BasicShapes/Cube at location (0,0,100)."},
    {"scenario_id": "vail_level_delete_actor", "prompt": "Delete the actor with ID 'TestCube' from the active level."},
    {"scenario_id": "vail_level_query_actors", "prompt": "Query all actors in the level whose name matches the pattern 'BP_*', up to 20 results."},
    {"scenario_id": "vail_viewport_frame", "prompt": "Frame the viewport camera onto the actor 'BP_Player' from a distance of 500 units."},
    {"scenario_id": "vail_project_build", "prompt": "Trigger a project build for the Windows target platform, using the Development build configuration."},
    {"scenario_id": "vail_landscape_create", "prompt": "Create a new landscape terrain actor at location (0,0,0) with a 1x1 component grid."},
    {"scenario_id": "vail_landscape_sculpt", "prompt": "Apply a raise sculpting brush at target location (500,500,0) with radius 1000 and strength 0.5."},
    {"scenario_id": "vail_landscape_paint", "prompt": "Paint the landscape layer 'Grass' at target location (500,500,0) with radius 500 and strength 0.8."},
    {"scenario_id": "vail_foliage_scatter", "prompt": "Scatter foliage using mesh /Game/Foliage/Fern in a 1000-unit radius around (0,0,0) with density 0.3."},
    {"scenario_id": "vail_foliage_query", "prompt": "Query foliage instance counts within a 1000-unit radius of (0,0,0)."},
    {"scenario_id": "vail_audio_play", "prompt": "Play the sound asset /Game/Audio/SFX_Explosion at location (0,0,0)."},
    {"scenario_id": "vail_audio_set_parameter", "prompt": "Set the MetaSound parameter 'Volume' to '0.5' on the audio actor 'Ambience_01'."},
    {"scenario_id": "vail_control_rig_query", "prompt": "Query the control hierarchy of the Control Rig asset at /Game/Characters/CR_Player."},
    {"scenario_id": "vail_control_rig_set_transform", "prompt": "Set the control 'hand_r_ctrl' on Control Rig /Game/Characters/CR_Player to location (10,20,30)."},
    {"scenario_id": "vail_sequencer_query", "prompt": "Inspect all tracks and keyframe ranges of the LevelSequence asset at /Game/Cinematics/Seq_Intro."},
    {"scenario_id": "vail_sequencer_add_track", "prompt": "Add a transform track binding for actor 'BP_Player' to the LevelSequence at /Game/Cinematics/Seq_Intro."},
    {"scenario_id": "vail_sequencer_add_key", "prompt": "Insert a keyframe at frame 30 with value '(X=0,Y=0,Z=200)' on the transform track of the LevelSequence at /Game/Cinematics/Seq_Intro."},
    {"scenario_id": "vail_widget_tree_get", "prompt": "Retrieve the full widget hierarchy of the Widget Blueprint at /Game/UI/WBP_MainMenu."},
    {"scenario_id": "vail_widget_add_element", "prompt": "Add a new Button element named 'PlayButton' into the root panel of the Widget Blueprint at /Game/UI/WBP_MainMenu."},
    {"scenario_id": "vail_widget_set_slot", "prompt": "Set the slot layout of element 'PlayButton' in /Game/UI/WBP_MainMenu to anchor centered with size (200,50)."},
    {"scenario_id": "vail_widget_bind_event", "prompt": "Bind the 'OnClicked' event of element 'PlayButton' in /Game/UI/WBP_MainMenu to a Blueprint function named 'HandlePlayClicked'."},
    {"scenario_id": "vail_sense_mesh", "prompt": "Inspect the geometric health (triangle count, Nanite status, UV overlap) of the asset /Game/Meshes/SM_Rock_01."},
    {"scenario_id": "vail_sense_shader", "prompt": "Inspect the shader complexity and instruction count of the material at /Game/Materials/M_Rock."},
    {"scenario_id": "vail_sense_optical", "prompt": "Calculate rendering optics metrics for the target camera 'CineCameraActor_1'."},
    {"scenario_id": "vail_sense_spatial", "prompt": "Compute spatial proprioception (ground clearance, surface normal) for actor 'BP_Player' along the visibility trace channel."},
]
