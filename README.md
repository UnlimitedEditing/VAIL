# ⚡ VAIL: Virtual Agent Interface Layer

> **The ultra-fast, token-saving bridge between AI agents and Unreal Engine.**

---

## 🎯 What is VAIL?

When AI agents work inside game engines like Unreal Engine, they usually have two bad options:
1. **Taking full-screen screenshots and guessing where to click.** This is slow, blurry, and eats thousands of expensive cloud tokens every second.
2. **Writing hundreds of fragile API tools.** These tools bypass the real editor, break whenever the engine updates, and hide silent errors.

**VAIL fixes this by using a simple truth:**  
Everything you see on your screen in Unreal Engine (the Details panel, the buttons, the properties) is already stored as clean, structured data sitting right between the user interface and your project files.

Instead of wasting expensive cloud tokens having a frontier AI read thousands of lines of raw buttons and menus, **VAIL hands all the boring busywork to a tiny, super-fast local model ("Grunt")**.

```
┌─────────────────────────┐
│   Frontier AI (Cloud)   │  "Make the player faster and change the light to red."
│  (Plans the big ideas)  │
└────────────┬────────────┘
             │ (1 short message: ~40 tokens)
             ▼
┌─────────────────────────┐
│      Grunt (Local)      │  Uses 5 simple tools locally in milliseconds:
│  (Tiny 26M - 1.5B Model)│  1. Look up where the setting is
│   (Zero API Token Cost) │  2. Inspect the current values
│                         │  3. Apply the changes
│                         │  4. Make sure the editor finished updating
└────────────┬────────────┘
             │ (Local C++ / TCP Socket)
             ▼
┌─────────────────────────┐
│      Unreal Engine      │  Cleanly updates properties, creates undo steps,
│  (VAILCore C++ Plugin)  │  and confirms the change without stealing your mouse!
└─────────────────────────┘
```

---

## 🚀 Why VAIL is Better

- 🪙 **98% Fewer Cloud Tokens:** The expensive cloud model only gives the high-level goal and gets back a clean summary. All the repetitive search and property tweaks happen locally for free.
- 🖱️ **Zero Mouse Hijacking:** VAIL sends virtual signals directly into Unreal’s engine loop. You can keep typing and clicking while the agent works in the background.
- ↩️ **Full Ctrl+Z Undo Support:** Every change made by an agent is cleanly grouped in Unreal's Undo History with a `VAIL:` label. If something goes wrong, it automatically rolls back.
- ⏱️ **Settle Detection:** VAIL doesn't guess if an action worked—it watches the engine's layout and compilation state to make sure changes are 100% finished before moving on.

---

## 🧰 The 5 Core Tools

Grunt only needs 5 simple tools to drive the entire editor:

1. **`vail_set_scope` (Focus):** Tells the agent which actor, panel, or window to look at.
2. **`vail_find` (Search):** Finds any button or property using normal human words (like *"jump height"* or *"recompile"*).
3. **`vail_get_tree` (Inspect):** Reads only the necessary properties in a compact list (<250 tokens).
4. **`vail_set_property` (Change):** Updates a property (like setting location, color, or speed) using plain text.
5. **`vail_execute_command` (Click/Action):** Triggers real editor commands (like compiling blueprints or saving levels).

*(Plus `vail_wait_for` to wait for background tasks and `vail_begin_batch`/`vail_end_batch` to bundle multiple changes into a single undo step).*

---

## 📁 Repository Layout

```
VAIL/
├── plugin/               # Unreal Engine 5.8 C++ Plugin (VAILCore)
│   ├── VAIL.uplugin
│   └── Source/VAILCore/  # Pure C++ engine inspection & settle state machine
├── python/               # FastMCP Server & Client tools (tools/vail_tools.py)
├── datasets/             # 720 pre-validated training samples to train Grunt
└── docs/                 # Protocol specs, Wayfinder decision maps, and handoffs
```

---

## 🤓 For Agents and Nerds (Technical Deep-Dive)

This section contains the low-level architecture, thread safety rules, and engine API bindings powering VAIL.

### 1. Dual-Module Decoupled Architecture
VAIL separates engine logic from transport adapters:
- **`VAILCore` (Native C++ Editor Module):** Pure engine library with zero knowledge of sockets, JSON, or Python. Directly binds to `FProperty` reflection, `IPropertyRowGenerator`, `FInputBindingManager`, and `FSlateInvalidationRoot`.
- **Network / Transport Adapters:** TCP JSON-RPC bridge (port `55557`) and FastMCP Python server (`vail_server.py`) that serialize tool requests into native C++ calls.

### 2. Widget Identity via `FUICommandInfo` & `IPropertyRowGenerator`
Rather than crawling transient Slate widget memory addresses, VAIL guarantees identity stability through two engine lookup tables:
- **Commands & Toolbar Actions:** Ingested via `FInputBindingManager::Get().GetCommandInfosFromContext()` and executed via `OnRegisterCommandList` delegate subscriptions (`FUICommandList::TryExecuteAction()`).
- **Details Panel Properties:** Headless `IPropertyRowGenerator` generates `IDetailTreeNode` instances off-screen. Bidirectional mutations execute through `IPropertyHandle::SetValueFromFormattedString()` on the Game Thread.

### 3. 3-Tier Deterministic Settle State Machine
Every mutation blocks and validates before returning:
1. **Tier 1 (Slate Quiescence):** Iterates top-level `SWindow` instances (`FSlateInvalidationRoot`) checking `NeedsSlowPath()` and `IsProcessingAttributeUpdate()`.
2. **Tier 2 (Async Operations):** Queries `GEditor->IsCompiling()`, `FAssetCompilingManager::Get().GetNumRemainingAssets()`, and `UBlueprint::bBeingCompiled`.
3. **Tier 3 (Modal/Menu Blockers):** Verifies `FSlateApplication::Get().GetActiveModalWindow()` is null and `AnyMenusVisible()` is false.

### 4. Insulated Auto-Rollback Contract
If an operation fails range validation or settle checks:
1. `GEditor->UndoTransaction()` is invoked immediately on the Game Thread.
2. The response returns `{ "status": "error", "rolled_back": true, "current_state_restored": true, "pre_action_state": { ... } }`.
3. Prevents model hallucination caused by partial state corruption.

### 5. Grunt Distillation Dataset
Located in `datasets/grunt_vail_training_corpus.jsonl`. Contains 720 schema-validated ChatML samples (120 per tool category) across:
- `vail_set_scope`
- `vail_find`
- `vail_get_tree`
- `vail_set_property`
- `vail_execute_command`
- `compound_reconciliation_batch`

For model fine-tuning hyperparameters and GGUF deployment instructions, see [docs/grunt_training_handoff.md](docs/grunt_training_handoff.md).
