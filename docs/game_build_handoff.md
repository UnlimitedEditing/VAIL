# Handoff: Build the VAIL demo game

**Read this first in the fresh session.** It captures everything the previous session verified
about VAIL's tool reliability, so the game-build session doesn't have to re-discover it.

## The goal

Build a small top-down horde-survival game (Vampire Survivors-like), mobile-targeted, in the
`vailtest` UE 5.8 project — driven through VAIL tool calls, not by hand-editing in the UE GUI.
The point of building it this way is to demonstrate VAIL actually works: a working, polished
game built almost entirely through VAIL tool calls is the proof, more convincing than any
synthetic benchmark.

## Environment

- **Project**: `D:\UE5.8\vailtest\vailtest.uproject` (UE 5.8, World Partition enabled)
- **Plugin source of truth**: `D:\UE5.8\VAIL\plugin\` — always edit here first, then copy
  changed files into `D:\UE5.8\vailtest\Plugins\VAIL\` (same relative path) before rebuilding.
  `vailtest`'s copy is what's actually compiled and running; the `VAIL\plugin\` copy is git-tracked
  history.
- **CLI**: `D:\UE5.8\VAIL\python\vail_cli.py <tool_name> '<json_args>' [--pretty]` — direct TCP
  dispatch to the plugin, no MCP registration needed. Default timeout 15s (UMG compiles can be
  slow). Exit code 0 on `status: success`, 1 otherwise.
- **Socket**: `127.0.0.1:55557`, only live while the `vailtest` editor is actually open.
- **The saved level**: `/Game/Maps/VAIL_TestLevel`. Make sure whatever level is open has been
  saved with a real path — an "Untitled" World Partition level has unreliable actor persistence
  (confirmed during this session: actors would spawn successfully but vanish/be unfindable
  until the level was saved for real).

### Rebuild workflow

- **Edits to existing function bodies** (most cases): Live Coding, `Ctrl+Alt+F11` in the editor.
  Fast, no restart.
- **New `#include`s of headers already available to the module**: still fine with Live Coding.
- **`.Build.cs` changes** (new module dependencies): Live Coding can't handle this. Full editor
  restart required. **Once, a restart silently reused a stale DLL and didn't actually recompile**
  — if a fix doesn't seem to take effect after a restart, check
  `Plugins/VAIL/Binaries/Win64/UnrealEditor-<Module>.dll` timestamps against the source edit time.
  If stale: close the editor fully, delete/move aside `Plugins/VAIL/Binaries/` and
  `Plugins/VAIL/Intermediate/`, then reopen — this forces a genuine full rebuild.

## Tool reliability status (verified this session, not assumed)

**24 of 40 tools are real** — confirmed by actually exercising them against a live editor and
checking engine-side state, not just trusting their JSON response.

**Use these freely:**
`vail_set_scope`, `vail_get_tree`, `vail_find`, `vail_set_property`, `vail_wait_for`,
`vail_execute_command`, `vail_begin_batch`, `vail_end_batch`, `vail_graph_get_topology`,
`vail_graph_add_node`, `vail_graph_connect_pins`, `vail_graph_delete_node`, `vail_asset_create`
(supports `Blueprint`/`BP`, `Material`/`M`, `MaterialInstance`/`MI`, `WidgetBlueprint`/`Widget`/`WBP`,
`InputAction`, `InputMappingContext` — nothing else), `vail_asset_query`, `vail_asset_save`,
`vail_level_spawn_actor`, `vail_level_delete_actor`, `vail_level_query_actors`,
`vail_viewport_frame`, `vail_widget_tree_get`, `vail_widget_add_element` (Button/TextBlock/
Image/ProgressBar/Border/Overlay/CanvasPanel/VerticalBox/HorizontalBox only),
`vail_widget_set_slot` (CanvasPanelSlot, HorizontalBoxSlot, VerticalBoxSlot),
`vail_widget_bind_event`, `vail_audio_play`.

**Added and proven during the BP_Player build (2026-08-28/29 session), not in VAIL's
original tool set:**
- `vail_component_add(asset_path, component_class, component_name, parent_component, attach_socket)`
  — adds a component to a Blueprint's Components tree headlessly, including attaching under
  an inherited native parent (e.g. Character's Capsule root) and at a named socket (e.g. a
  SpringArm's `SpringEndpoint` — **required** for a Camera child, otherwise it sits at the
  boom's origin, not its end).
- `vail_component_remove(asset_path, component_name)`.
- `vail_variable_add(asset_path, var_name, var_type, default_value)` — `var_type` one of
  `Float`, `Int`, `Bool`, `String`, `Vector`.
- `vail_input_map_key(context_asset_path, action_asset_path, key_name, modifiers)` —
  modifiers: `Negate`, `SwizzleYXZ` (the WASD-to-2D-axis trick).
- `vail_graph_set_pin_default(pin_spec, value, asset_path, graph_name)` — was already
  implemented server-side, just never wired to a command until now.
- `vail_plugin_version()` — call after any restart/Live Coding attempt to confirm new code
  is actually running; don't reason from DLL/source timestamps.
- `vail_graph_add_node` `node_type` grammar gained: `InputAxis:<AxisName>` (legacy — likely
  non-functional on its own in 5.8, Enhanced Input mappings don't fire it; kept for other
  uses), `EnhancedInputAction:<InputActionAssetPath>`, `CustomEvent:<EventName>`. The
  **generic K2Node class-spawn fallback** (any `node_type` not matching a special-cased
  prefix) now routes through `UBlueprintNodeSpawner`, not a raw `NewObject`+
  `AllocateDefaultPins` — the old version hard-crashed the editor on
  `K2Node_SpawnActorFromClass`. `SpawnActorFromClass` itself is now confirmed working
  through this path.
- `vail_set_scope` target syntax gained `<BlueprintAssetPath>::<ComponentName>` (resolves an
  SCS component's template, including inherited-native ones by class-name fallback — e.g.
  `::CharacterMovement` resolves the native `CharMoveComp`) and `<BlueprintAssetPath>::Self`
  / `::CDO` (resolves the Blueprint's class-default-object for actor-level defaults like
  `bUseControllerRotationYaw`).
- Movement input requires Enhanced Input, not legacy Axis Mappings (5.8 shows a "deprecated"
  banner for Axis/Action Mappings in Project Settings and they did not fire `InputAxis`
  Blueprint events in testing). Runtime-callable Blueprint function libraries (e.g. one that
  activates an Input Mapping Context) **must live in a `Type: Runtime` plugin module** — a
  `Type: Editor` module's `UFUNCTION`s are rejected by the Blueprint compiler when called
  from a gameplay Blueprint ("Cannot use the editor function ... in this runtime
  Blueprint"). See `VAILRuntime` module, added this session for exactly this.

**Still fake — do not use, they silently return plausible-looking fabricated data with no
engine effect:** `vail_sequencer_query/add_track/add_key`, `vail_landscape_create/sculpt/paint`,
`vail_foliage_scatter/query`, `vail_control_rig_query/set_transform`, `vail_audio_set_parameter`,
`vail_project_build`, `vail_sense_optical/spatial/mesh/shader` (these four are also explicitly
`Status: Parked` in `docs/sensory_validation_critic_handoff.md` — deliberately deferred design
work, not a bug).

## Tier 3 target: procedural geometry (Geometry Script) — researched, not yet built

Jacob's actual goal past this game-build slice is **procedural geometry**, not just materials —
see the tiered roadmap: Tier 1 materials/shaders, Tier 2 Niagara VFX, **Tier 3 procedural
geometry (the real target)**, Tier 4 real meshes (spun off as a separate future "VAIL for
Blender" project, out of scope here). Researched this session (2026-08-29), not yet
implemented or tested live:

- **`GeometryScriptingCore`** (Engine plugin, `Type: Runtime` — works in packaged builds, not
  editor-only) exposes `UGeometryScriptLibrary_MeshPrimitiveFunctions` with dozens of plain
  `BlueprintCallable` static functions — `AppendBox`, `AppendSphere`, `AppendCapsule`,
  `AppendCone`, boolean ops, etc. Each takes a `UDynamicMesh*` and returns the same one
  (`ScriptMethod` meta = chainable in the Blueprint graph), so this is directly reachable
  through VAIL's **already-proven, already-working** `CallFunction:` node type — no new VAIL
  C++ needed for the geometry-generation calls themselves.
- **`UDynamicMeshComponent`** (`Engine/Source/Runtime/GeometryFramework`, a core engine
  runtime module, not a plugin) is the component that renders a `UDynamicMesh`. Its
  `GetDynamicMesh()` is NOT Blueprint-callable (commented-out `UFUNCTION` in the header) --
  don't try to call it via `CallFunction:`. The real write path is
  `SetDynamicMesh(UDynamicMesh* NewMesh)`, which **is** `BlueprintCallable`.
- Expected pattern once implemented: create a blank `UDynamicMesh` (via a GeometryScript
  "Create New Mesh" helper -- not yet located exactly, check
  `UGeometryScriptLibrary_CreateNewMeshFunctions` or similar in
  `MeshPrimitiveFunctions.h`'s neighboring headers first) -> chain `AppendBox`/`AppendSphere`/
  etc. `CallFunction:` nodes onto it -> `SetDynamicMesh()` on a `UDynamicMeshComponent` added
  via the already-proven `vail_component_add`.
- Apply the seeding process before writing any code: finish locating the "create mesh"
  helper and the exact primitive-options struct shape first, then batch, then verify via
  `vail_plugin_version` -- same discipline as everything else in this doc.

**Practical implication for the build:**
- Gameplay-time enemy spawning must be done via Blueprint graph nodes (a `SpawnActorFromClass`
  K2 node via `vail_graph_add_node` — confirmed working now that the generic fallback routes
  through `UBlueprintNodeSpawner`), **not** `vail_level_spawn_actor` — that tool is
  editor-time-only placement, it has no meaning at runtime.
- No landscape/foliage — use a flat/simple ground plane (a scaled `StaticMeshActor` cube or
  plane works fine for a top-down arena).
- No Sequencer — skip cutscenes/intro entirely for this build.
- No Control Rig — not needed for basic character movement.
- Test builds via Play-In-Editor, not `vail_project_build` (fake).

## Suggested build order

1. **Player**: `BP_Player` (Pawn or Character parent), top-down movement, health/speed
   variables, a fixed top-down camera.
2. **Enemy**: `BP_EnemyBasic` — chases the player (simple `MoveTo`/velocity-toward-player each
   tick), health, contact damage on overlap.
3. **Spawner**: a Blueprint actor with a timer that spawns waves of `BP_EnemyBasic` around the
   player at an increasing rate — via graph nodes, not `vail_level_spawn_actor`.
4. **Weapon**: auto-fire toward the nearest enemy, a simple projectile Blueprint with
   overlap damage.
5. **XP/leveling**: pickup actor + player XP variable + level-up threshold.
6. **HUD**: `WBP_HUD` — health bar and XP bar (`ProgressBar`), wave/timer `TextBlock`. Build with
   `vail_widget_add_element` + `vail_widget_set_slot`.
7. **Upgrade-pick screen**: `WBP_UpgradeSelect` — 3 buttons, bound via `vail_widget_bind_event`,
   pauses the game and offers a weapon/stat choice on level-up. This is the UI VAIL couldn't have
   built before this session's fixes — worth highlighting in whatever demo comes out of this.
8. **Mobile touch controls**: virtual joystick. Mostly Project Settings + Enhanced Input Touch
   actions, not a VAIL tool call — treat as a secondary/stretch task.
9. **Polish**: `vail_audio_play` for hits/pickups/music, material/particle tweaks, camera shake.

## First concrete task for the fresh session

Create `BP_Player`, give it basic top-down movement and a fixed camera, and confirm it visually
in Play-In-Editor before building anything else on top of it.
