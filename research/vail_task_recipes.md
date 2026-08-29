# VAIL task recipes — verified, reusable tool-call sequences

**Purpose.** Distilled at stabilization checkpoints, not per-edit (see
`vail_tool_seeding_process.md` for why). Each recipe here has been run end-to-end and
confirmed working in PIE, not just "compiled without errors." Recipes name the tool/node types
and the order that matters; they are not copy-paste JSON (parameters like asset paths, damage
numbers, speeds are per-project). When a recipe needs a capability that didn't exist when this
doc was written, that capability is named explicitly so it's obvious if a future VAIL version
lacks it.

---

## Recipe: Top-down Character setup

**Goal:** A Character-based Pawn with a fixed top-down camera and Enhanced Input WASD movement.

1. `vail_asset_create` → `Blueprint`, parent `Character`.
2. `vail_component_add` → `SpringArmComponent`, parent `CapsuleComponent`. Set
   `RelativeRotation` (pitch ~-60 for top-down), `TargetArmLength`, `bDoCollisionTest=false`,
   `bUsePawnControlRotation=false`.
3. `vail_component_add` → `CameraComponent`, parent = the SpringArm, **`attach_socket` must be
   `SpringEndpoint`** — without it the camera sits at the boom's origin, not its end (this bit
   us the first time; the socket name is not optional).
4. `::CharacterMovement` scope → `bOrientRotationToMovement=true`, `MaxWalkSpeed`.
5. `::Self` scope → `bUseControllerRotationYaw/Pitch/Roll = false` (actor-level, not the
   movement component).
6. Enhanced Input, since legacy Axis Mappings don't fire `InputAxis` events in 5.8:
   - `vail_asset_create` → `InputAction` (set `ValueType = Axis2D`), `InputMappingContext`.
   - `vail_input_map_key` four times (W/A/S/D) — W and S need `modifiers: ["SwizzleYXZ"]`
     (S also `"Negate"`), A needs `["Negate"]`, D needs none. This is the standard
     WASD-onto-2D-axis trick.
   - Graph: `EnhancedInputAction:<IA path>` node → `Triggered` exec → `CallFunction:BreakVector2D`
     on its `ActionValue` pin (NOT `BreakInputActionValue` — that's for the untyped struct case;
     an `Axis2D`-typed action's `ActionValue` pin is already a concrete `Vector2D`) → two
     `CallFunction:AddMovementInput` calls, `WorldDirection` literals `(1,0,0)`/`(0,1,0)`,
     `ScaleValue` from the broken X/Y.
   - `BeginPlay` → a runtime-module helper (`AddInputMappingContextByPath` in this project's
     `VAILRuntime` module) to activate the `InputMappingContext` — **must live in a `Type:
     Runtime` plugin module**, not `Editor`, or the Blueprint compiler rejects the call from a
     gameplay Blueprint.
7. Placeholder visuals: `vail_component_add` → `StaticMeshComponent`, mesh
   `/Engine/BasicShapes/Cylinder.Cylinder`, scale to roughly match the capsule
   (`(radius*2/100, radius*2/100, halfheight*2/200)` — the cylinder's own origin is its
   *center*, not its base, so no Z offset is needed if the capsule is also centered on the
   actor origin).
8. GameMode: create a `GameModeBase`-parented Blueprint, set its `DefaultPawnClass` to this
   character, set it as the level's `WorldSettings.DefaultGameMode` override (not the
   project-wide default). **Save the level itself after this** — it's a level-asset change,
   not a Blueprint-asset change, and gets silently lost on the next restart/reload otherwise
   (this happened twice tonight).

---

## Recipe: Chasing enemy with contact damage

**Goal:** A Character that walks toward the nearest player pawn every tick and damages it on
overlap.

1. Same Character-Blueprint + placeholder-mesh steps as above (skip camera/input).
2. `vail_variable_add` → `Health` (`Float`).
3. Chase graph (`Event Tick`): `CallFunction:GetPlayerPawn` (`WorldContextObject` = a `Self`
   node) → `CallFunction:K2_GetActorLocation` on both the player pawn and `Self` → `CallFunction:
   Subtract_VectorVector` (player - self) → `CallFunction:Normal` → `CallFunction:
   AddMovementInput` (`Target` = `Self`, `WorldDirection` = the normalized vector, `ScaleValue`
   default 1.0).
4. **Critical, easy to miss**: set `bRunPhysicsWithNoController = true` on the
   `CharacterMovementComponent`. A Character spawned at runtime via `SpawnActorFromClass` never
   gets an AI controller just from `AutoPossessAI = PlacedInWorld` (that only fires for
   actors placed at edit time) — with this flag at its default `false`, the component silently
   stays at `MovementMode = MOVE_None` forever: no gravity, no movement, regardless of how
   correct the `AddMovementInput` calls are. This one property flip is the whole fix. See the
   debugging playbook below for how this was actually found.
5. Contact damage: the Character's default `Event ActorBeginOverlap` → `DynamicCast:<player
   Blueprint path>` → `VariableGet:<player path>::Health` → `CallFunction:Subtract_DoubleDouble`
   → `VariableSet:<player path>::Health` (`Target` = the cast result both times).
6. **Also required**: the Pawn-vs-Pawn collision response defaults to `Block`, which both
   physically stops the enemy at the player's capsule *and* prevents `ActorBeginOverlap` from
   firing at all (a block isn't an overlap). Fix via `BeginPlay` → `Get CapsuleComponent` →
   `CallFunction:SetCollisionResponseToChannel` (`Channel = ECC_Pawn`, `NewResponse =
   ECR_Overlap`) — there's no plain reflected property for this
   (`BodyInstance->CollisionProfileName` isn't exposed by VAIL's property inspector, it's a
   special dropdown widget in the real UI), so it has to be a function call, not a property set.

---

## Recipe: Wave spawner (timer-driven, spawns around the player)

1. New `Actor`-parented Blueprint.
2. `BeginPlay` → `CallFunction:K2_SetTimer` (`Object` = `Self`, `FunctionName` = a string
   matching a `CustomEvent:` node you also add to the same graph, `bLooping = true`).
3. On the `CustomEvent`: `CallFunction:GetPlayerPawn` → `K2_GetActorLocation` for the spawn
   center. For the random offset, **do not use `RandomUnitVector` directly** — it's a full 3D
   sphere sample and will place spawns up to the full radius *above* the player (this happened
   tonight: enemies spawned floating in the sky, then barely moved because `CharacterMovement`
   gives heavily reduced control while airborne — two symptoms, one root cause). Flatten it
   first: `RandomUnitVector` → `BreakVector` → `MakeVector(X, Y, Z=0)` → `Normal` → `Multiply_
   VectorFloat` by the desired radius → `Add_VectorVector` onto the player's location.
4. `CallFunction:MakeTransform` (Location = the computed spawn point) → `SpawnActorFromClass`
   node (`Class` pin set to the enemy Blueprint's path, `SpawnTransform` = the made transform).

---

## Recipe: Simple health bar bound to a variable

1. `vail_asset_create` → `WidgetBlueprint`. `vail_widget_add_element` → `ProgressBar`.
2. Graph (`Event Tick`, which exists by default on a `UserWidget`): `CallFunction:
   GetOwningPlayerPawn` (needs a `Self` node wired to its `Target` pin even though it's an
   instance method — VAIL's generic `CallFunction:` spawner doesn't auto-hide self pins the
   way the real editor UI does) → `DynamicCast:<player path>` → `VariableGet:<player
   path>::Health` → `CallFunction:Divide_DoubleDouble` (by max health) → `VariableGet:
   <the ProgressBar's own variable name>` (self-context, no `::`) → `CallFunction:SetPercent`.
3. To actually show it: on the player's `BeginPlay`, `CallFunction:CreateWidget` (`Class` pin
   = the widget's path) → `CallFunction:AddToViewport`.

---

## Debugging playbook: "the graph looks right but nothing happens"

When a Blueprint graph is fully wired correctly (verified by dumping its whole topology and
checking every pin's `linked_to`) but the visible behavior still doesn't happen, the fastest
diagnosis — used tonight to find the `bRunPhysicsWithNoController` bug — is a **cascading,
color-coded `PrintString` chain** spliced into the exec flow, each printing one more layer
down the causal chain, in a distinct color so multiple simultaneous streams (e.g. one per
spawned enemy) don't blur together:

1. Print the *computed input value* first (cyan: the direction vector). If this is degenerate
   (zero, NaN), the bug is upstream in the math — stop here.
2. If the input is valid, print the *actual world state that should be changing* (red: actor
   location) across many ticks. If it's frozen while the input keeps varying, the bug is
   downstream of the input call, not in the graph logic at all.
3. Narrow further with whatever native queries are available for the specific subsystem
   involved — for movement, `CallFunction:GetVelocity` (green) proved velocity was *exactly*
   zero (not just small), which rules out "movement is just slow" and points at the component
   not processing physics at all.
4. Read a `BlueprintReadOnly` state property directly if one exists — `MovementMode`
   (yellow, via `CallFunction:Conv_ByteToString`) gave the literal answer (`MOVE_None`) in one
   step once accessible. Reading an arbitrary native class's property from a Blueprint graph
   needed a VAIL fix that session (`VariableGet:<NativeClassName>::<PropertyName>`, extended
   to resolve native engine classes by short name, not just Blueprint assets) — worth checking
   whether this capability already exists before assuming it needs rebuilding.

Once the numeric readout narrows the culprit to a specific mode/flag/property, look it up
directly in the engine header rather than guessing further — that's how
`bRunPhysicsWithNoController` was found, by reading `CharacterMovementComponent.h`'s property
list once the symptom (`MOVE_None`, frozen velocity) was concrete enough to search for.

**Always remove the debug print chain and reconnect the direct exec wire before calling a
Blueprint done** — it's cheap to re-add if needed again, and leaving it in place screen-spams
gameplay video.
