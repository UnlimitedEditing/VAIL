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
  "settle_ms": 16.4
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

## 3. Python FastMCP Tool Bindings (`vail_tools.py`)

```python
"""
VAIL (Virtual Agent Interface Layer) FastMCP Tool Registration.
Exposes universal, human-semantic UI & data interaction primitives to agents.
"""

from typing import Optional, Dict, Any
from unreal_mcp_server import get_unreal_connection

def register_vail_tools(mcp):
    """Register all universal VAIL primitives on the FastMCP server."""

    @mcp.tool()
    def vail_set_scope(scope: str, target: str = "") -> Dict[str, Any]:
        """Set the active agent attention scope in Unreal Editor.
        
        Args:
            scope: Target panel or context (e.g. 'DetailsPanel', 'Toolbar', 'ContentBrowser').
            target: Optional target actor name, asset path, or object identifier.
        """
        conn = get_unreal_connection()
        return conn.send_command("vail_set_scope", {"scope": scope, "target": target})

    @mcp.tool()
    def vail_get_tree(max_depth: int = 2, category_filter: str = "") -> Dict[str, Any]:
        """Get the pruned, compact semantic hierarchy of interactable properties/widgets in active scope.
        
        Args:
            max_depth: Maximum property nesting depth (default: 2).
            category_filter: Optional substring filter for category names (e.g. 'Transform').
        """
        conn = get_unreal_connection()
        return conn.send_command("vail_get_tree", {"max_depth": max_depth, "category_filter": category_filter})

    @mcp.tool()
    def vail_find(query: str, scope: str = "") -> Dict[str, Any]:
        """Fuzzy-search for editor commands, properties, or tooltips by human label.
        
        Args:
            query: Search query (e.g. 'Compile', 'Location', 'Simulate Physics').
            scope: Optional scope restriction (e.g. 'Toolbar', 'DetailsPanel').
        """
        conn = get_unreal_connection()
        return conn.send_command("vail_find", {"query": query, "scope": scope})

    @mcp.tool()
    def vail_execute_command(command_id: str) -> Dict[str, Any]:
        """Execute a human-facing editor command by its stable FUICommandInfo semantic ID.
        
        Args:
            command_id: The semantic identifier (e.g. 'Kismet.Compile', 'LevelEditor.Save').
        """
        conn = get_unreal_connection()
        return conn.send_command("vail_execute_command", {"command_id": command_id})

    @mcp.tool()
    def vail_set_property(property_id: str, value: str) -> Dict[str, Any]:
        """Set a property value on the active scoped object using human-formatted strings.
        
        Args:
            property_id: Dot-separated property path (e.g. 'RelativeLocation.X', 'Mobility').
            value: Formatted new value (e.g. '500.0', 'Movable', 'true').
        """
        conn = get_unreal_connection()
        return conn.send_command("vail_set_property", {"property_id": property_id, "value": value})

    @mcp.tool()
    def vail_wait_for(condition: str = "quiescent", timeout_seconds: float = 5.0) -> Dict[str, Any]:
        """Block until the editor UI and background asset/compilation pipelines settle.
        
        Args:
            condition: Condition to wait for ('quiescent', 'compilation_complete', 'modal_cleared').
            timeout_seconds: Maximum wait duration before returning timeout error.
        """
        conn = get_unreal_connection()
        return conn.send_command("vail_wait_for", {"condition": condition, "timeout_seconds": timeout_seconds})

    @mcp.tool()
    def vail_begin_batch(title: str) -> Dict[str, Any]:
        """Begin a compound transaction batch so subsequent operations group into a single Undo step.
        
        Args:
            title: Human-readable description for Edit -> Undo History.
        """
        conn = get_unreal_connection()
        return conn.send_command("vail_begin_batch", {"title": title})

    @mcp.tool()
    def vail_end_batch() -> Dict[str, Any]:
        """Commit and close the active compound transaction batch."""
        conn = get_unreal_connection()
        return conn.send_command("vail_end_batch", {})
```
