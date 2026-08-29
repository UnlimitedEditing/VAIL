# Geometry Script workflow index — from primitives to real shapes

**Purpose.** Jacob's framing (2026-08-29): the gap in reaching Tier 3 (procedural geometry —
see `vail_roadmap_tiers` memory / `vail_tool_seeding_process.md`) isn't "find the raw
functions," it's that Geometry Script's ~40 header files and 400+ functions have no vocabulary
connecting *design intent* ("make it jagged," "carve a hole," "extrude along a path") to the
actual call sequence. This doc is that vocabulary layer — read it before writing any geometry
Blueprint graph, the same way `vail_tool_seeding_process.md` says to read engine headers
before writing plugin C++.

**The mechanism (confirmed, not yet live-tested).** Every function below is a plain
`BlueprintCallable` static function on a `UBlueprintFunctionLibrary` subclass, taking and
returning `UDynamicMesh*` (chainable via `ScriptMethod` meta — each call both mutates and
returns the same mesh object, so a design "recipe" is just a chain of `CallFunction:` nodes).
This is **already directly reachable through VAIL's existing, proven `CallFunction:` node
type** — no new VAIL plugin C++ needed for the geometry calls themselves. What's still
unverified: locating the "create a blank `UDynamicMesh`" helper, and confirming
`UDynamicMeshComponent::SetDynamicMesh()` wiring live in the editor (see
`docs/game_build_handoff.md`'s Tier 3 section for exactly what's confirmed vs. not).

All headers live under
`F:\UE_5.8\Engine\Plugins\Runtime\GeometryScripting\Source\GeometryScriptingCore\Public\GeometryScript\`
on this machine (binary engine install — headers only, no source `.cpp`).

## By design intent

**"Start from a basic shape"** → `MeshPrimitiveFunctions.h` (38 functions). `AppendBox`,
`AppendSphere`, `AppendCapsule`, `AppendCone`, `AppendCylinder`, `AppendTorus`, plus
`AppendSimpleCollisionShapesToMesh`. Each takes `FGeometryScriptPrimitiveOptions`, an
`FTransform`, and dimension params; `EGeometryScriptPrimitiveOriginMode` controls pivot
placement (Base vs Center).

**"Carve, combine, or cut it"** → `MeshBooleanFunctions.h` (6 functions) — fully read, this is
the sharpest tool for "make it look designed, not primitive":
- `ApplyMeshBoolean(Target, TargetTransform, Tool, ToolTransform, Operation, Options)` —
  `EGeometryScriptBooleanOperation`: Union, Intersection, Subtract, TrimInside, TrimOutside.
  This is THE function for "carve a hole," "punch a window," "combine two primitives into one
  silhouette."
- `ApplyMeshSelfUnion` — repairs self-intersections / removes floating geometry after a messy
  combine.
- `ApplyMeshPlaneCut` / `ApplyMeshPlaneSlice` — cut a mesh with a plane (slice keeps both
  halves, cut discards one side), with hole-filling options.
- `ApplyMeshMirror` — mirror across a plane with optional weld (symmetric creature/prop
  shapes).

**"Make it organic / less primitive-looking"** → `MeshDeformFunctions.h` (10 functions) —
fully read, this is the other sharpest tool:
- `ApplyPerlinNoiseToMesh2(Target, Selection, FGeometryScriptPerlinNoiseOptions, Debug)` —
  3D Perlin noise surface displacement. `PerlinNoiseOptions.BaseLayer` has `Magnitude`,
  `Frequency`, `RandomSeed`. **This is the single highest-value function for "jagged
  rock"/"organic blob"/"not a perfect primitive" look** — apply to any primitive mesh for an
  instant procedural, non-repeating surface variation.
- `ApplyBendWarpToMesh` / `ApplyTwistWarpToMesh` / `ApplyFlareWarpToMesh` — classic modeling
  deformers (bend around an axis, twist, bulge/taper), each with a `FTransform` orientation
  and an extent/angle. Good for stylized weapon shapes, creature limbs, architectural forms.
- `ApplyMathWarpToMesh` — sine-wave-based warps (`EGeometryScriptMathWarpType`: 1D/2D/3D),
  for rippling/wavy surfaces.
- `ApplyIterativeSmoothingToMesh` — relaxation smoothing, good after a noisy/boolean op to
  soften harsh edges.
- `ApplyDisplaceFromTextureMap` / `ApplyDisplaceFromPerVertexVectors` — displace by a
  Texture2D channel or an explicit per-vertex vector list; more control, more setup.

**"Shape it like a path/profile" (not yet read in depth)** → `PolyPathFunctions.h` (21
functions), `PolygonFunctions.h` (37 functions), `ShapeFunctions.h` (45 functions). Likely
covers extrude-along-spline, revolve, loft, and 2D-polygon-to-3D operations — the mechanism
for non-primitive silhouettes (a sword blade, a tree trunk, a wall segment). **Read next**,
before attempting anything path/profile-based.

**"Simplify / clean up / prep for game use" (not yet read)** → `MeshSimplifyFunctions.h` (9),
`MeshRemeshFunctions.h` (2), `MeshRepairFunctions.h` (11), `MeshNormalsFunctions.h` (18),
`MeshSubdivideFunctions.h` (3). Needed once a procedural mesh is "designed" and needs to
become game-ready (triangle budget, correct normals, no cracks).

**"Texture/material it"** → `MeshUVFunctions.h` (29), `MeshMaterialFunctions.h` (17),
`MeshVertexColorFunctions.h` (8), `TextureMapFunctions.h` (2), `MeshBakeFunctions.h` (18).
Pairs with the Tier 1 material/shader work already planned — a procedurally-shaped mesh still
needs UVs and a material assigned to look finished.

**Query/utility categories (lower priority for design work, useful for validation/scripting
logic)**: `MeshQueryFunctions.h` (51), `ListUtilityFunctions.h` (56), `VectorMathFunctions.h`
(22), `MeshSelectionFunctions.h` (27) + `MeshSelectionQueryFunctions.h` (2),
`MeshPolygroupFunctions.h` (19), `MeshTransformFunctions.h` (11), `MeshComparisonFunctions.h`
(3), `MeshDecompositionFunctions.h` (11), `MeshSpatialFunctions.h` (8),
`MeshWeightMapFunctions.h` (6), `MeshBoneWeightFunctions.h` (20) (skeletal-mesh-specific,
probably not relevant to static prop generation), `MeshSculptLayersFunctions.h` (9),
`MeshVoxelFunctions.h` (2), `MeshSamplingFunctions.h` (6), `PointSetFunctions.h` (9),
`MeshGeodesicFunctions.h` (3), `MeshAssetFunctions.h` (15) (likely import/export to
StaticMesh assets — relevant once a procedural result needs to become a permanent asset),
`MeshModelingFunctions.h` (12), `MeshPoolFunctions.h` (2), `ContainmentFunctions.h` (4),
`CollisionFunctions.h` (23), `SceneUtilityFunctions.h` (5), `VolumeTextureBakeFunctions.h` (1).

## Next steps to actually reach Tier 3 (not yet done)

1. Locate the "create a blank `UDynamicMesh`" entry point (check `MeshPoolFunctions.h` or a
   `UGeometryScriptLibrary_CreateNewMeshFunctions`-style helper — not yet found).
2. Read `PolyPathFunctions.h` / `ShapeFunctions.h` in the same depth as Booleans/Deformations
   above — these are the likely path to non-primitive silhouettes.
3. First live test (apply the seeding process — pre-read done, now batch + verify via
   `vail_plugin_version`, don't guess live): add a `DynamicMeshComponent` via the
   already-proven `vail_component_add`, chain one `AppendBox` → one `ApplyPerlinNoiseToMesh2`
   → `SetDynamicMesh`, and confirm it renders as an irregular, non-primitive-looking shape in
   PIE. That one successful chain proves the whole Tier 3 mechanism end-to-end.
