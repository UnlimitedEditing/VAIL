"""
VAIL (Virtual Agent Interface Layer) Universal FastMCP Tools
"""

import logging
from typing import Optional, Dict, Any

logger = logging.getLogger("VAIL.Tools")

def register_vail_tools(mcp):
    """Register universal VAIL primitives on the FastMCP server."""
    
    from vail_server import get_vail_connection

    @mcp.tool()
    def vail_set_scope(scope: str = "DetailsPanel", target: str = "") -> Dict[str, Any]:
        """Set the active agent attention scope in Unreal Editor.
        
        Args:
            scope: Target panel or context ('DetailsPanel', 'Toolbar', 'ContentBrowser', 'ActivePanel').
            target: Optional target actor label/name, asset package path (e.g. '/Game/Blueprints/BP_Player').
        """
        conn = get_vail_connection()
        return conn.send_command("vail_set_scope", {"scope": scope, "target": target})

    @mcp.tool()
    def vail_get_tree(max_depth: int = 2, category_filter: str = "") -> Dict[str, Any]:
        """Get the pruned, compact semantic hierarchy of interactable properties/widgets in the active scope.
        
        Args:
            max_depth: Maximum property nesting depth (default: 2).
            category_filter: Optional substring filter for category names (e.g. 'Transform', 'Physics').
        """
        conn = get_vail_connection()
        return conn.send_command("vail_get_tree", {"max_depth": max_depth, "category_filter": category_filter})

    @mcp.tool()
    def vail_find(query: str, scope: str = "") -> Dict[str, Any]:
        """Fuzzy-search for editor commands, properties, or tooltips by human label or identifier.
        
        Args:
            query: Search query (e.g. 'Compile', 'Location', 'Simulate Physics', 'Save').
            scope: Optional scope restriction (e.g. 'Toolbar', 'DetailsPanel').
        """
        conn = get_vail_connection()
        return conn.send_command("vail_find", {"query": query, "scope": scope})

    @mcp.tool()
    def vail_execute_command(command_id: str) -> Dict[str, Any]:
        """Execute a human-facing editor command by its stable FUICommandInfo semantic ID.
        
        Args:
            command_id: The semantic identifier (e.g. 'Kismet.Compile', 'LevelEditor.SaveDirect', 'MainFrame.SaveAll').
        """
        conn = get_vail_connection()
        return conn.send_command("vail_execute_command", {"command_id": command_id})

    @mcp.tool()
    def vail_set_property(property_id: str, value: str) -> Dict[str, Any]:
        """Set a property value on the active scoped object using human-formatted strings.
        
        Args:
            property_id: Dot-separated property path (e.g. 'RelativeLocation.X', 'Mobility', 'bHidden').
            value: Formatted new value (e.g. '500.0', 'Movable', 'true').
        """
        conn = get_vail_connection()
        return conn.send_command("vail_set_property", {"property_id": property_id, "value": value})

    @mcp.tool()
    def vail_wait_for(timeout_seconds: float = 5.0) -> Dict[str, Any]:
        """Block until the editor UI, Slate layout passes, and background compilation pipelines settle.
        
        Args:
            timeout_seconds: Maximum wait duration before returning timeout error (default: 5.0).
        """
        conn = get_vail_connection()
        return conn.send_command("vail_wait_for", {"timeout_seconds": timeout_seconds})

    @mcp.tool()
    def vail_begin_batch(title: str = "Agent Mutation Batch") -> Dict[str, Any]:
        """Begin a compound transaction batch so subsequent operations group into a single Undo step.
        
        Args:
            title: Human-readable description for Edit -> Undo History.
        """
        conn = get_vail_connection()
        return conn.send_command("vail_begin_batch", {"title": title})

    @mcp.tool()
    def vail_end_batch() -> Dict[str, Any]:
        """Commit and close the active compound transaction batch."""
        conn = get_vail_connection()
        return conn.send_command("vail_end_batch", {})
