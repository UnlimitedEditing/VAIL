"""
VAIL (Virtual Agent Interface Layer) FastMCP Tool Registration.
Exposes universal, human-semantic UI & data interaction primitives to agents.
"""

import logging
from typing import Optional, Dict, Any

logger = logging.getLogger("UnrealMCP.VAIL")

def register_vail_tools(mcp):
    """Register all universal VAIL primitives on the FastMCP server."""

    # Import get_unreal_connection dynamically from unreal_mcp_server
    from unreal_mcp_server import get_unreal_connection

    @mcp.tool()
    def vail_set_scope(scope: str = "DetailsPanel", target: str = "") -> Dict[str, Any]:
        """Set the active agent attention scope in Unreal Editor.
        
        Args:
            scope: Target panel or context ('DetailsPanel', 'Toolbar', 'ContentBrowser', 'ActivePanel').
            target: Optional target actor label/name, asset package path (e.g. '/Game/Blueprints/BP_Player').
        """
        conn = get_unreal_connection()
        if not conn:
            return {"status": "error", "error": "Not connected to Unreal Engine"}
        return conn.send_command("vail_set_scope", {"scope": scope, "target": target})

    @mcp.tool()
    def vail_get_tree(max_depth: int = 2, category_filter: str = "") -> Dict[str, Any]:
        """Get the pruned, compact semantic hierarchy of interactable properties/widgets in the active scope.
        
        Args:
            max_depth: Maximum property nesting depth (default: 2).
            category_filter: Optional substring filter for category names (e.g. 'Transform', 'Physics').
        """
        conn = get_unreal_connection()
        if not conn:
            return {"status": "error", "error": "Not connected to Unreal Engine"}
        return conn.send_command("vail_get_tree", {"max_depth": max_depth, "category_filter": category_filter})

    @mcp.tool()
    def vail_find(query: str, scope: str = "") -> Dict[str, Any]:
        """Fuzzy-search for editor commands, properties, or tooltips by human label or identifier.
        
        Args:
            query: Search query (e.g. 'Compile', 'Location', 'Simulate Physics', 'Save').
            scope: Optional scope restriction (e.g. 'Toolbar', 'DetailsPanel').
        """
        conn = get_unreal_connection()
        if not conn:
            return {"status": "error", "error": "Not connected to Unreal Engine"}
        return conn.send_command("vail_find", {"query": query, "scope": scope})

    @mcp.tool()
    def vail_execute_command(command_id: str) -> Dict[str, Any]:
        """Execute a human-facing editor command by its stable FUICommandInfo semantic ID.
        
        Args:
            command_id: The semantic identifier (e.g. 'Kismet.Compile', 'LevelEditor.SaveDirect', 'MainFrame.SaveAll').
        """
        conn = get_unreal_connection()
        if not conn:
            return {"status": "error", "error": "Not connected to Unreal Engine"}
        return conn.send_command("vail_execute_command", {"command_id": command_id})

    @mcp.tool()
    def vail_set_property(property_id: str, value: str) -> Dict[str, Any]:
        """Set a property value on the active scoped object using human-formatted strings.
        
        Args:
            property_id: Dot-separated property path (e.g. 'RelativeLocation.X', 'Mobility', 'bHidden').
            value: Formatted new value (e.g. '500.0', 'Movable', 'true').
        """
        conn = get_unreal_connection()
        if not conn:
            return {"status": "error", "error": "Not connected to Unreal Engine"}
        return conn.send_command("vail_set_property", {"property_id": property_id, "value": value})

    @mcp.tool()
    def vail_wait_for(timeout_seconds: float = 5.0) -> Dict[str, Any]:
        """Block until the editor UI, Slate layout passes, and background compilation pipelines settle.
        
        Args:
            timeout_seconds: Maximum wait duration before returning timeout error (default: 5.0).
        """
        conn = get_unreal_connection()
        if not conn:
            return {"status": "error", "error": "Not connected to Unreal Engine"}
        return conn.send_command("vail_wait_for", {"timeout_seconds": timeout_seconds})

    @mcp.tool()
    def vail_begin_batch(title: str = "Agent Mutation Batch") -> Dict[str, Any]:
        """Begin a compound transaction batch so subsequent operations group into a single Undo step.
        
        Args:
            title: Human-readable description for Edit -> Undo History.
        """
        conn = get_unreal_connection()
        if not conn:
            return {"status": "error", "error": "Not connected to Unreal Engine"}
        return conn.send_command("vail_begin_batch", {"title": title})

    @mcp.tool()
    def vail_end_batch() -> Dict[str, Any]:
        """Commit and close the active compound transaction batch."""
        conn = get_unreal_connection()
        if not conn:
            return {"status": "error", "error": "Not connected to Unreal Engine"}
        return conn.send_command("vail_end_batch", {})

    # =========================================================================
    # Phase 2: Universal Graph Data-Channel (Blueprints, Materials, Niagara)
    # =========================================================================

    @mcp.tool()
    def vail_graph_get_topology(asset_path: str = "", graph_name: str = "") -> Dict[str, Any]:
        """Get the topology (nodes, pins, and wire connections) of an asset graph (Blueprint, Material, etc.).
        
        Args:
            asset_path: Optional path to the asset (e.g. '/Game/Materials/M_Test'). Defaults to scoped object.
            graph_name: Optional name of the sub-graph (e.g. 'EventGraph', 'UserConstructionScript').
        """
        conn = get_unreal_connection()
        if not conn:
            return {"status": "error", "error": "Not connected to Unreal Engine"}
        return conn.send_command("vail_graph_get_topology", {"asset_path": asset_path, "graph_name": graph_name})

    @mcp.tool()
    def vail_graph_add_node(
        node_type: str,
        asset_path: str = "",
        graph_name: str = "",
        pos_x: float = 0.0,
        pos_y: float = 0.0,
        extra_params: Optional[Dict[str, str]] = None
    ) -> Dict[str, Any]:
        """Spawns a node polymorphically in an asset graph (Blueprint K2Node, Material Expression, etc.).
        
        Args:
            node_type: Node type identifier (e.g. 'CallFunction:PrintString', 'Event:ReceiveBeginPlay', 'VectorParameter', 'Multiply').
            asset_path: Optional path to target asset.
            graph_name: Optional sub-graph name.
            pos_x: X-coordinate on graph canvas.
            pos_y: Y-coordinate on graph canvas.
            extra_params: Optional dictionary of parameter presets (e.g. {'ParameterName': 'BaseColor', 'DefaultValue': '(R=1,G=0,B=0,A=1)'}).
        """
        conn = get_unreal_connection()
        if not conn:
            return {"status": "error", "error": "Not connected to Unreal Engine"}
        params = {
            "node_type": node_type,
            "asset_path": asset_path,
            "graph_name": graph_name,
            "pos_x": pos_x,
            "pos_y": pos_y,
            "extra_params": extra_params or {}
        }
        return conn.send_command("vail_graph_add_node", params)

    @mcp.tool()
    def vail_graph_connect_pins(
        source_pin: str,
        target_pin: str,
        asset_path: str = "",
        graph_name: str = ""
    ) -> Dict[str, Any]:
        """Connects two graph pins using schema validation rules (TryCreateConnection).
        
        Args:
            source_pin: Source pin in 'NodeId:PinName' format.
            target_pin: Target pin in 'NodeId:PinName' format.
            asset_path: Optional asset path.
            graph_name: Optional graph name.
        """
        conn = get_unreal_connection()
        if not conn:
            return {"status": "error", "error": "Not connected to Unreal Engine"}
        return conn.send_command("vail_graph_connect_pins", {
            "source_pin": source_pin,
            "target_pin": target_pin,
            "asset_path": asset_path,
            "graph_name": graph_name
        })

    @mcp.tool()
    def vail_graph_delete_node(node_id: str, asset_path: str = "", graph_name: str = "") -> Dict[str, Any]:
        """Deletes a node from a graph by its ID.
        
        Args:
            node_id: Node GUID or Name.
            asset_path: Optional asset path.
            graph_name: Optional graph name.
        """
        conn = get_unreal_connection()
        if not conn:
            return {"status": "error", "error": "Not connected to Unreal Engine"}
        return conn.send_command("vail_graph_delete_node", {
            "node_id": node_id,
            "asset_path": asset_path,
            "graph_name": graph_name
        })

    # =========================================================================
    # Phase 2: Content Browser Asset Management
    # =========================================================================

    @mcp.tool()
    def vail_asset_create(asset_path: str, asset_class: str, parent_class: str = "") -> Dict[str, Any]:
        """Headless creation of new assets in the Content Browser.
        
        Args:
            asset_path: Target asset path (e.g. '/Game/Materials/M_HeroShader', '/Game/Blueprints/BP_Enemy').
            asset_class: Class of asset ('Blueprint', 'Material', 'MaterialInstanceConstant').
            parent_class: For Blueprints: parent class (e.g. 'Actor', 'Character', 'Pawn').
        """
        conn = get_unreal_connection()
        if not conn:
            return {"status": "error", "error": "Not connected to Unreal Engine"}
        return conn.send_command("vail_asset_create", {
            "asset_path": asset_path,
            "asset_class": asset_class,
            "parent_class": parent_class
        })

    @mcp.tool()
    def vail_asset_query(package_path: str = "/Game", class_filter: str = "") -> Dict[str, Any]:
        """Query the Asset Registry for assets matching package and class filters.
        
        Args:
            package_path: Base package path to search (default: '/Game').
            class_filter: Optional asset class name to filter by (e.g. 'Blueprint', 'Material').
        """
        conn = get_unreal_connection()
        if not conn:
            return {"status": "error", "error": "Not connected to Unreal Engine"}
        return conn.send_command("vail_asset_query", {
            "package_path": package_path,
            "class_filter": class_filter
        })
