# UE 3D design systems survey — what exists, what's scriptable, what's not yet integrated

**Purpose.** Jacob's framing (2026-08-29): there will be "bajillions" of 3D meta-workflows,
methods, and design flows in UE — the job isn't to rediscover them one at a time under
pressure, it's to build a standing map of *which systems exist* and *how VAIL could reach
each one*, so a future session (or a stuck one) has somewhere to look instead of starting
from zero. This is the top-level index; `geometry_script_workflow_index.md` is the deep
dive on the first (and best-understood) system below.

## Systems surveyed

### 1. Geometry Script — mesh generation/editing. Best understood, actively being integrated.
Plain `BlueprintCallable` functions on `UDynamicMesh*`, reachable through VAIL's existing,
proven `CallFunction:` node type. See `geometry_script_workflow_index.md` for the full
category breakdown (primitives, booleans, deformations, path/shape, simplify/repair, UV/
material). **Status: mechanism proven live this session** (component-add + node chain worked
without crashing) — the exact "get a working `UDynamicMesh` reference into a component" wiring
was still being tested when this doc was written; check `game_build_handoff.md`'s Tier 3
section for the latest confirmed state before assuming it's finished.

### 2. PCG (Procedural Content Generation Framework) — world/level population. NOT integrated, separate graph system, bigger lift.
`F:\UE_5.8\Engine\Plugins\PCG\` — `Type: Runtime`, enabled by default. Epic's own description:
"Visual scripting framework for procedurally populating worlds with content in editor and/or
at run-time." This is almost certainly the right system for level-design-scale asks ("scatter
rocks along this path," "populate this area with foliage density-mapped to a noise field," "lay
out a room grid") — bigger scope than a single mesh.

**Why it's a separate integration effort, not a `CallFunction:` extension**: PCG graphs
(`PCGGraph` assets) are their own node-graph system with their own settings-node hierarchy
(`UPCGSettings` subclasses), not `UEdGraph`/`K2Node`-based like Blueprints. VAIL's graph tool
(`vail_graph_add_node`, `vail_graph_connect_pins`) is built specifically around the Blueprint
K2 graph API (`UEdGraphSchema_K2`, `UK2Node`) — it has no reason to work on a `PCGGraph` at all.
Reaching PCG would mean researching the PCG graph API from scratch (likely
`Engine/Plugins/PCG/Source/PCG/Public/PCGGraph.h` and friends) and possibly adding a parallel
set of VAIL commands, not extending the existing ones. **Not started. Worth its own dedicated
research pass before attempting.**

### 3. Splines / path-based construction — likely reachable, not yet read in depth.
Two angles: `USplineComponent` (a normal Blueprint-exposed component, addable via the
already-proven `vail_component_add`) for authoring a path, and Geometry Script's
`PolyPathFunctions.h` (21 functions, flagged but not yet read in
`geometry_script_workflow_index.md`) for turning a path into geometry (extrude, sweep, loft).
**Next natural read** after finishing the primitives/booleans/deformations chain, since it's
the same `CallFunction:`-reachable mechanism, just a different function category.

### 4. Landscape — partially scriptable, but VAIL's own tools here are confirmed fake.
`docs/game_build_handoff.md` already flags `vail_landscape_create/sculpt/paint` as fabricating
data with no engine effect (confirmed earlier this session). Separately, the engine does expose
`LandscapeBlueprintBrushBase` (`Engine/Source/Runtime/Landscape/Public/`) — a real,
Blueprint-scriptable mechanism for procedural landscape sculpting via custom Blueprint brush
actors — but nothing in VAIL touches it, and it's a different mechanism than the fake
`vail_landscape_*` commands. Skip landscape work entirely for now (matches the existing "no
landscape" build guidance) unless a future session decides it's worth building this properly.

### 5. Foliage / instanced scattering — minimal native Blueprint surface, likely PCG's job instead.
`InstancedFoliageActor.h` has only 2 `BlueprintCallable` functions — foliage painting is
mostly an editor-tool-driven workflow (the Foliage Mode brush), not a rich scriptable API.
For "scatter N meshes across an area," PCG (system 2 above) is almost certainly the better
target than fighting the native Foliage API.

### 6. Niagara (VFX) — already flagged in `vail_roadmap_tiers` memory as Tier 2, not started.
Own asset type (`NiagaraSystem`), own graph editor. Needs new `vail_asset_create` support.
Not surveyed further this session — lower priority than Tier 3 geometry per Jacob's explicit
ranking.

## How to use this doc

Before starting any new "make VAIL do X 3D thing" task: check here first for which system X
belongs to, and whether it's already been scoped. If it's an unlisted system, add it here with
the same three questions answered: (1) what does it do, (2) is it Blueprint/`CallFunction:`
reachable or does it need its own graph-API integration, (3) has anything been proven live yet.
That's the whole discipline — cheap to follow, expensive to skip.
