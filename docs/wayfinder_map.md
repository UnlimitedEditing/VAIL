# Virtual Agent Interface Layer (VAIL) Map

## Destination

A complete, validated technical specification and architectural contract for VAIL (Virtual Agent Interface Layer) in UE 5.8, ready for immediate implementation. Resolves all architectural boundaries, identity registry mechanics, headless property inspection, settle state machine hooks, and the agent tool contract.

## Notes

- **Domain:** Unreal Engine 5.8 Editor internals, Slate Architecture, Reflection System (`FProperty`), Command Routing (`FUICommandInfo`), MCP Tool Design.
- **Skills/Methods:** Wayfinder decision mapping, HITL architectural grilling, AFK engine header research, C++ prototype stubs.
- **Preferences:** Plan before building; maintain dual-channel separation (UI vs. Data channel); ensure zero OS mouse/focus hijacking; verify through deterministic engine state diffs.

## Decisions so far

<!-- the index — one line per closed ticket: enough to judge relevance, then zoom the link for the detail the ticket holds -->

- [[Architecture Boundary: In-Plugin Handler vs Standalone Plugin/Module](file:///d:/UE5.8/Tools/Wayfinder/vail-map/tickets/ticket-1-architecture-boundary.md)] — Decided on Option B (Dual-module architecture: pure C++ `VAILCore` + `UnrealMCP` TCP bridge) with full staged deprecation of legacy bespoke tools in favor of VAIL primitives.
- [[FUICommandInfo & Command List Runtime Enumeration in UE 5.8](file:///d:/UE5.8/Tools/Wayfinder/vail-map/tickets/ticket-2-fuicommandinfo-runtime-enumeration.md)] — Confirmed headless static command schema extraction via `FInputBindingManager` and dynamic execution routing via `OnRegisterCommandList` delegate without requiring window focus or synthetic mouse clicks.
- [[Headless Property Introspection via IPropertyRowGenerator](file:///d:/UE5.8/Tools/Wayfinder/vail-map/tickets/ticket-3-headless-property-introspection.md)] — Validated prototype using `IPropertyRowGenerator` and `IPropertyHandle` for off-screen category/property tree generation, semantic path IDs, and direct string mutations.
- [[Slate Invalidation & Settle State Machine Engine Hooks](file:///d:/UE5.8/Tools/Wayfinder/vail-map/tickets/ticket-4-slate-quiescence-and-settle-hooks.md)] — Locked concrete 3-tier settle APIs: `SWindow::NeedsSlowPath()` (Slate layout), `GEditor->IsCompiling()` + `FAssetCompilingManager` (async jobs), and `FSlateApplication::GetActiveModalWindow()` (modal blockers).
- [[Transaction & Undo/Redo Parity for Agent Property Mutations](file:///d:/UE5.8/Tools/Wayfinder/vail-map/tickets/ticket-5-transaction-and-undo-redo-parity.md)] — Locked `VAIL:` prefixed transaction audit trail, hybrid atomic/batched transaction model (`vail_begin_batch`), and insulated auto-rollback contract.
- [[VAIL JSON-RPC Protocol & Python MCP Tool Schema](file:///d:/UE5.8/Tools/Wayfinder/vail-map/tickets/ticket-6-vail-json-rpc-protocol-and-tool-schema.md)] — Finalized and froze wire JSON-RPC schema, standard response envelopes with diffs, and Python FastMCP function contracts in [vail_protocol_spec.md](file:///d:/UE5.8/Tools/Wayfinder/vail-map/assets/vail_protocol_spec.md).

## Not yet specified

- **Graph Editor Data-Channel Reflection:** Abstracting `UEdGraph`, `UEdGraphNode`, and `UEdGraphPin` into a reflection-driven mutation protocol that avoids Slate canvas coordinates. (Graduates once Phase 1 Details Panel & Commands are implemented).
- **Token Budget Compression & Pruning Heuristics:** Optimal serialization format (Indented S-expressions vs. compact JSON) and pruning depth for massive actor trees (10,000+ properties/components).
- **Ghost Cursor & Viewport Visual Feedback:** Slate overlay widget rendering ghost cursor trajectories and ripple indicators without interfering with active hit-test grids.
- **Local SLM (1.5B–3B) Trace Logging & Distillation:** Structured logging format for agent action-result traces to enable downstream local model routing.

## Out of scope

- **Full-Screen Computer Vision & OS `SendInput`:** Rejecting OS-level mouse stealing or 4K screenshot OCR.
- **Runtime Game HUD / In-Game UMG Agent Control:** VAIL is strictly scoped to the Unreal Editor development environment, not shipping runtime player UI.
- **Third-party Unregistered Custom Slate Canvas Paint Parsing:** Custom widgets that do not register `FUICommandInfo` or reflect `FProperty` are excluded from Phase 1.
