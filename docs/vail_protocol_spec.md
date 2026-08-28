# VAIL Protocol & Python MCP Tool Specification (v1.0 Frozen)

> **Specification Status:** Frozen / Final Architectural Contract  
> **Module Target:** UE 5.8 `VAILCore` C++ & `UnrealMCP_Python` FastMCP Server  
> **Transport:** TCP JSON-RPC over `127.0.0.1:55557` (Newline-delimited JSON)

---

## 1. Transport & Wire Format

Every request and response over the TCP socket is a single-line JSON string followed by `\n`.

### Standard Request Envelope
```json
{
  "type": "vail_<command_name>",
  "params": {
    "key": "value"
  }
}
```

### Standard Response Envelope (Success)
```json
{
  "status": "success",
  "result": {
    "data": "..."
  },
  "diff": {
    "changed": [
      { "id": "RelativeLocation.X", "old": "0.000", "new": "500.000" }
    ]
  },
  "settle_ms": 16.4,
  "screen_warnings": [
    "Cached lighting in Lumen... is going to be clipped. Exposure: -8.5."
  ]
}
```

### Standard Response Envelope (Error with Auto-Rollback)
```json
{
  "status": "error",
  "error": "Property 'RelativeLocation.W' not found on StaticMeshActor_1",
  "error_code": "PROPERTY_NOT_FOUND",
  "rolled_back": true,
  "current_state_restored": true,
  "settle_ms": 32.1
}
```

---

## 2. Core Tool Primitives (~10 Tools)

### 1. `vail_set_scope`
Anchors agent attention to a specific panel, actor, or active context.
- **Request:**
  ```json
  {
    "type": "vail_set_scope",
    "params": {
      "scope": "DetailsPanel",
      "target": "StaticMeshActor_1"
    }
  }
  ```
- **Response:**
  ```json
  {
    "status": "success",
    "result": {
      "scope": "DetailsPanel",
      "target": "StaticMeshActor_1",
      "class": "StaticMeshActor",
      "root_categories": ["TransformCommon", "StaticMesh", "Materials", "Physics", "Collision"]
    }
  }
  ```

---

### 2. `vail_get_tree`
Returns a compact, pruned semantic tree of interactable properties/widgets within the active scope.
- **Request:**
  ```json
  {
    "type": "vail_get_tree",
    "params": {
      "max_depth": 2,
      "category_filter": "TransformCommon"
    }
  }
  ```
- **Response:**
  ```json
  {
    "status": "success",
    "result": {
      "categories": [
        {
          "category": "TransformCommon",
          "properties": [
            {
              "id": "RelativeLocation",
              "label": "Location",
              "value": "X=0.000 Y=0.000 Z=100.000",
              "is_editable": true,
              "properties": [
                { "id": "RelativeLocation.X", "label": "X", "value": "0.000" },
                { "id": "RelativeLocation.Y", "label": "Y", "value": "0.000" },
                { "id": "RelativeLocation.Z", "label": "Z", "value": "100.000" }
              ]
            },
            {
              "id": "Mobility",
              "label": "Mobility",
              "value": "Static",
              "options": ["Static", "Stationary", "Movable"]
            }
          ]
        }
      ]
    }
  }
  ```

---

### 3. `vail_find`
Fuzzy / substring search across all available commands, properties, and tooltips in current or global context.
- **Request:**
  ```json
  {
    "type": "vail_find",
    "params": {
      "query": "Compile",
      "scope": "Toolbar"
    }
  }
  ```
- **Response:**
  ```json
  {
    "status": "success",
    "result": {
      "matches": [
        {
          "id": "Kismet.Compile",
          "label": "Compile",
          "description": "Recompiles the blueprint",
          "chord": "F7",
          "is_available": true
        }
      ]
    }
  }
  ```

---

### 4. `vail_execute_command`
Executes an editor command by its stable `FUICommandInfo` semantic ID.
- **Request:**
  ```json
  {
    "type": "vail_execute_command",
    "params": {
      "command_id": "Kismet.Compile"
    }
  }
  ```
- **Response:**
  ```json
  {
    "status": "success",
    "result": {
      "executed": true,
      "command_id": "Kismet.Compile"
    },
    "diff": {
      "compilation_status": { "old": "Dirty", "new": "UpToDate" }
    },
    "settle_ms": 48.2
  }
  ```

---

### 5. `vail_set_property`
Directly mutates an object property via `IPropertyHandle` using human-formatted strings.
- **Request:**
  ```json
  {
    "type": "vail_set_property",
    "params": {
      "property_id": "RelativeLocation.X",
      "value": "500.0"
    }
  }
  ```
- **Response:**
  ```json
  {
    "status": "success",
    "result": {
      "property_id": "RelativeLocation.X",
      "applied_value": "500.000"
    },
    "diff": {
      "changed": [
        { "id": "RelativeLocation.X", "old": "0.000", "new": "500.000" }
      ]
    },
    "settle_ms": 16.1
  }
  ```

---

### 6. `vail_wait_for`
Blocks until an asynchronous condition settles or times out.
- **Request:**
  ```json
  {
    "type": "vail_wait_for",
    "params": {
      "condition": "quiescent",
      "timeout_seconds": 5.0
    }
  }
  ```
- **Response:**
  ```json
  {
    "status": "success",
    "result": {
      "settled": true,
      "quiescent_frames": 2
    },
    "settle_ms": 33.0
  }
  ```

---

### 7. `vail_begin_batch` & `vail_end_batch`
Groups multiple tool calls into a single named human undo step.
- **Begin Request:**
  ```json
  {
    "type": "vail_begin_batch",
    "params": {
      "title": "Configure Player Spawn Transform"
    }
  }
  ```
- **End Request:**
  ```json
  {
    "type": "vail_end_batch"
  }
  ```

---

## 3. Phase 2: Universal Graph & Asset Primitives

### 8. `vail_graph_get_topology`
Returns the node-and-pin connectivity graph of an asset (Blueprint, Material).
- **Request:**
  ```json
  {
    "type": "vail_graph_get_topology",
    "params": {
      "asset_path": "/Game/Materials/M_HeroShader",
      "graph_name": ""
    }
  }
  ```

### 9. `vail_graph_add_node`
Spawns a node in an asset graph (K2Node, MaterialExpression).
- **Request:**
  ```json
  {
    "type": "vail_graph_add_node",
    "params": {
      "node_type": "CallFunction:PrintString",
      "asset_path": "/Game/Blueprints/BP_Player",
      "graph_name": "EventGraph",
      "pos_x": 400.0,
      "pos_y": 0.0
    }
  }
  ```

### 10. `vail_graph_connect_pins` & `vail_graph_delete_node`
Connects two pins with schema validation or deletes a node by GUID.

### 11. `vail_asset_create`, `vail_asset_query`, `vail_asset_save`
Headless Content Browser asset lifecycle tools.

---

## 4. Phase 3: Spatial, UMG, Sequencer & Sensory Vectors

### 12. Level & Viewport Spatial Manipulation
- **`vail_level_spawn_actor`**: Places an asset or actor into the active level viewport.
  ```json
  {
    "type": "vail_level_spawn_actor",
    "params": {
      "asset_path": "/Game/Meshes/SM_Pillar",
      "actor_label": "Pillar_Entrance_01",
      "location": [500.0, 200.0, 0.0],
      "rotation": [0.0, 90.0, 0.0],
      "scale": [1.0, 1.0, 1.0],
      "folder_path": "Architecture/Exterior"
    }
  }
  ```
- **`vail_level_query_actors`**: Queries actors by wildcard pattern, class, or tags.
- **`vail_level_delete_actor`**: Deletes an actor in an undoable transaction.
- **`vail_viewport_frame`**: Focuses/frames the viewport camera smoothly onto an actor or point without stealing OS mouse/window focus.

### 13. UMG Widget Tree & UI Canvas Channel
- **`vail_widget_tree_get`**: Inspects the widget tree hierarchy of a Widget Blueprint.
- **`vail_widget_add_element`**: Adds CanvasPanel, Button, TextBlock, Image, etc. with slot layout and content presets.
- **`vail_widget_set_slot`**: Mutates anchors, positions, sizes, alignments, and padding.
- **`vail_widget_bind_event`**: Binds delegate events (`OnClicked`, `OnHovered`) into the graph.

### 14. Sequencer & Cine Timeline Channel
- **`vail_sequencer_query`**: Queries tracks, bindings, and sections of a LevelSequence.
- **`vail_sequencer_add_track`**: Adds Transform, Property, Audio, CameraCut tracks.
- **`vail_sequencer_add_key`**: Inserts keyframes on track channels with interpolation curves.

### 15. Sensory Telemetry Vectors (The 4 Diagnostic Vectors)
- **`vail_sense_optical`**: Returns compact optical telemetry (dominant RGB, hue, saturation, luminance lux, exposure, clipping %).
- **`vail_sense_spatial`**: Returns spatial clearance, surface normal, collision overlap depth, and camera frustum visibility.
- **`vail_sense_mesh`**: Returns geometric health, triangle count, Nanite status, UV overlap %.
- **`vail_sense_shader`**: Returns shader complexity, instruction count, texture samplers, roughness, and Lumen cache validity.

---

## 5. Phase 4: Subsystems & Specialized Production Tooling

### 16. Landscape & Terrain
- **`vail_landscape_create`**: Creates landscape grid terrain headlessly.
  ```json
  {
    "type": "vail_landscape_create",
    "params": {
      "section_size": 63,
      "sections_per_component": 1,
      "component_count_x": 8,
      "component_count_y": 8,
      "material_path": "/Game/Materials/M_LandscapeMaster"
    }
  }
  ```
- **`vail_landscape_sculpt`**: Applies heightmap sculpting brushes (`Sculpt`, `Smooth`, `Flatten`, `Ramp`, `Erosion`).
- **`vail_landscape_paint`**: Paints weightmap layers (`Grass`, `Rock`, `Snow`).

### 17. Foliage & Instancing
- **`vail_foliage_scatter`**: Procedurally scatters instanced meshes within radius with scale and normal alignment.
- **`vail_foliage_query`**: Queries foliage instance counts and species in bounding volumes.

### 18. Control Rig & Solvers
- **`vail_control_rig_set_transform`**: Mutates Control Rig bone and control solver target goals.
- **`vail_control_rig_query`**: Inspects available rig controls, bones, and limits.

### 19. MetaSounds & Audio
- **`vail_audio_play`**: Auditions SoundCue or MetaSound assets in 2D or 3D space.
- **`vail_audio_set_parameter`**: Sets runtime parameters on active MetaSound graphs (`PitchMod`, `ReverbGain`, `TriggerPlay`).

### 20. Project Build & Automation
- **`vail_project_build`**: Triggers cook, package, or build pipelines for target platforms (`Windows`, `Linux`, `Android`).


