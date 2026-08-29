# Process: seeding an unverified VAIL capability

**Context.** Building `BP_Player` (camera, movement, Enhanced Input) took ~9 full editor
restarts, most of them not because the *game* work was hard but because VAIL's tool surface
had real gaps (missing component-add, an exposed-but-never-wired `SetPinDefaultValue`, a
graph node spawner that couldn't type Enhanced Input pins, an Editor/Runtime module split
that didn't exist). Most of those restarts were paying for bugs a syntax check or a header
read would have caught for free. This doc is the process for doing that seeding work in
fewer, more deliberate passes.

## The loop, in order

1. **Read before writing.** Before adding any new `node_type`, command, or asset-class
   branch, grep the actual engine headers (`F:/UE_5.8/Engine/...` on this machine, headers
   only -- it's a binary install, no engine `.cpp` source) for the real signature. Don't
   guess a UFUNCTION name, a struct's pin category, or a constructor's parse shape. Every
   bug tonight that cost a wasted restart (`FKey Key(FName(*KeyName))` most-vexing-parse, a
   struct-type mismatch on an `ActionValue` pin, an off-by-one in a `Mid()` prefix length)
   came from writing against a guessed shape instead of the real one.

2. **Compile-check before touching the editor.** Run UBT directly against just the plugin
   target before ever asking for an editor relaunch:
   ```
   "F:\UE_5.8\Engine\Build\BatchFiles\Build.bat" vailtestEditor Win64 Development ^
     -project="D:\UE5.8\vailtest\vailtest.uproject" -plugin="D:\UE5.8\vailtest\Plugins\VAIL\VAIL.uplugin"
   ```
   (exact invocation to be confirmed the first time it's used -- the point is a headless
   syntax/type check in under a minute, versus a 5-10 minute full editor boot). Every
   compile error tonight would have surfaced here for free, with the editor never closed.

3. **Batch by upcoming build step, not by "what just broke."** Before starting a build
   step (Enemy, Spawner, Weapon, ...), read the whole sub-plan first, grep every engine API
   it's likely to need, write all the C++ for it in one pass, then do *one* restart --
   instead of add-one-thing -> discover a gap live -> restart, repeated per gap.

4. **Separate Live-Coding-safe edits from full-restart edits.** A pure function-body change
   (no signature change, no new header, no `.Build.cs` edit, no new `UCLASS`) is a Live
   Coding candidate (Ctrl+Alt+F11, no restart). Anything touching `.Build.cs`, a new
   module, or a new/changed function signature forces a full restart. Don't bundle a
   body-only fix into the same commit as a signature change -- it forces a restart that
   half the diff didn't need.

5. **Verify the new code is actually live, cheaply.** Don't reason from DLL/source
   `stat` timestamps (a restart can silently keep a stale DLL -- this happened twice
   tonight, and timestamp comparison across the +2h log-vs-filesystem offset is
   error-prone). Instead call `vail_plugin_version` (implemented -- returns a string bumped
   on every plugin change) immediately after any restart or Live Coding attempt. One call,
   no ambiguity.

6. **Never guess against a live editor twice in a row.** The generic K2Node class-spawn
   fallback (used for any `node_type` we haven't hand-special-cased) crashed the whole
   editor -- hard `check()`, not a recoverable `ensure()` -- on `K2Node_SpawnActorFromClass`,
   twice, including once *after* a plausible-looking fix (reordering `AddNode` vs
   `AllocateDefaultPins`) that turned out not to be the real cause. The actual fix was
   routing that fallback through `UBlueprintNodeSpawner::Create(...)->Invoke(...)` -- the
   same code path the Blueprint editor's own right-click menu uses -- instead of hand-rolling
   `NewObject`/`CreateNewGuid`/`AllocateDefaultPins` per node type. If a live test crashes
   the editor, stop and read the crash log (`Saved/Crashes/.../vailtest.log`) and the
   relevant header before trying again -- don't re-guess live. A second crash costs a full
   editor reboot for nothing.

7. **Save immediately after any successful mutating call, not at the end of a batch.**
   Unsaved Blueprint variable/graph edits are lost on a crash (confirmed tonight -- a
   `Health` variable and a test event both vanished after the `SpawnActorFromClass` crash
   and had to be redone). Once a call comes back `success: true`, save before the next one
   if the asset would be annoying to reconstruct.

8. **Keep the verified/unverified ledger current.** `docs/game_build_handoff.md` already
   tracks which of VAIL's original tools are real vs fake. Extend that list as new
   capabilities get built and proven tonight (`vail_component_add`, `vail_component_remove`,
   `vail_input_map_key`, `vail_graph_set_pin_default`, `vail_variable_add`,
   `vail_plugin_version`, the `InputAxis:`/`EnhancedInputAction:`/`CustomEvent:` graph node
   types, `::ComponentName` / `::Self` scoping, and the `UBlueprintNodeSpawner`-routed
   generic fallback) so the next session -- or Grunt's training-data curation -- knows
   what's bedrock versus still a guess, instead of re-discovering it live.

## What this predicts for the rest of the build

Each remaining build-order step (Enemy, Spawner, Weapon, XP/leveling, HUD, upgrade-select
screen) will likely surface its own version of tonight's gaps -- e.g. we don't yet know if
`vail_widget_add_element` or a chase-AI behavior graph has its own struct-mismatch bug. The
win from tonight isn't "VAIL is finished," it's "the process for finding and fixing these
gaps in one or two passes instead of nine" -- apply steps 1-3 to each step before writing
any C++ for it.
