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

    @mcp.tool()
    def vail_asset_save(asset_path: str = "") -> Dict[str, Any]:
        """Headlessly save an asset package to disk without requiring UI focus.
        
        Args:
            asset_path: Optional path of the asset to save (e.g. '/Game/Materials/M_HeroShader'). Defaults to active scope.
        """
        conn = get_unreal_connection()
        if not conn:
            return {"status": "error", "error": "Not connected to Unreal Engine"}
        return conn.send_command("vail_asset_save", {"asset_path": asset_path})

    # =========================================================================
    # Phase 3: Level & Viewport Spatial Manipulation
    # =========================================================================

    @mcp.tool()
    def vail_level_spawn_actor(
        asset_path: str,
        actor_label: str = "",
        location: Optional[Any] = None,
        rotation: Optional[Any] = None,
        scale: Optional[Any] = None,
        folder_path: str = "",
        parent_actor: str = ""
    ) -> Dict[str, Any]:
        """Spawns an actor into the active level viewport headlessly with transform and outliner placement.
        
        Args:
            asset_path: Asset to place (e.g. '/Game/Meshes/SM_Pillar', '/Game/Blueprints/BP_Door') or native class name ('PointLight', 'DirectionalLight', 'StaticMeshActor').
            actor_label: Human-readable label in the World Outliner.
            location: World coordinates as [X, Y, Z] list or {'X': 0, 'Y': 0, 'Z': 0} dict (default: [0, 0, 0]).
            rotation: Euler angles as [Pitch, Yaw, Roll] list or {'Pitch': 0, 'Yaw': 0, 'Roll': 0} dict (default: [0, 0, 0]).
            scale: Scale as [ScaleX, ScaleY, ScaleZ] list or {'X': 1, 'Y': 1, 'Z': 1} dict (default: [1, 1, 1]).
            folder_path: Outliner folder path for organization (e.g. 'Lighting/Exterior').
            parent_actor: Label or ID of actor to attach to.
        """
        conn = get_unreal_connection()
        if not conn:
            return {"status": "error", "error": "Not connected to Unreal Engine"}
        return conn.send_command("vail_level_spawn_actor", {
            "asset_path": asset_path,
            "actor_label": actor_label,
            "location": location if location is not None else [0.0, 0.0, 0.0],
            "rotation": rotation if rotation is not None else [0.0, 0.0, 0.0],
            "scale": scale if scale is not None else [1.0, 1.0, 1.0],
            "folder_path": folder_path,
            "parent_actor": parent_actor
        })

    @mcp.tool()
    def vail_level_query_actors(
        pattern: str = "*",
        class_filter: str = "",
        tag_filter: str = "",
        max_results: int = 50
    ) -> Dict[str, Any]:
        """Queries actors in the level matching pattern, class, or tag filters.
        
        Args:
            pattern: Wildcard pattern for actor label (e.g. 'Light*', '*Enemy*').
            class_filter: Filter by actor class name (e.g. 'PointLight', 'StaticMeshActor').
            tag_filter: Filter by actor tag or gameplay tag.
            max_results: Maximum number of actors to return (default: 50).
        """
        conn = get_unreal_connection()
        if not conn:
            return {"status": "error", "error": "Not connected to Unreal Engine"}
        return conn.send_command("vail_level_query_actors", {
            "pattern": pattern,
            "class_filter": class_filter,
            "tag_filter": tag_filter,
            "max_results": max_results
        })

    @mcp.tool()
    def vail_level_delete_actor(actor_id: str) -> Dict[str, Any]:
        """Safely removes an actor from the active level within an Undo transaction.
        
        Args:
            actor_id: Actor label, name, or path in the current world.
        """
        conn = get_unreal_connection()
        if not conn:
            return {"status": "error", "error": "Not connected to Unreal Engine"}
        return conn.send_command("vail_level_delete_actor", {"actor_id": actor_id})

    @mcp.tool()
    def vail_viewport_frame(
        target: str,
        distance: float = 500.0,
        pitch: float = -20.0,
        yaw: float = 0.0
    ) -> Dict[str, Any]:
        """Frames the viewport camera onto an actor or spatial coordinate without stealing OS focus.
        
        Args:
            target: Actor label or 'X,Y,Z' target coordinate.
            distance: Boom camera distance (default: 500.0).
            pitch: Pitch angle in degrees (default: -20.0).
            yaw: Yaw angle in degrees (default: 0.0).
        """
        conn = get_unreal_connection()
        if not conn:
            return {"status": "error", "error": "Not connected to Unreal Engine"}
        return conn.send_command("vail_viewport_frame", {
            "target": target,
            "distance": distance,
            "pitch": pitch,
            "yaw": yaw
        })

    # =========================================================================
    # Phase 3: UMG Widget Tree & UI Canvas Data-Channel
    # =========================================================================

    @mcp.tool()
    def vail_widget_tree_get(widget_path: str) -> Dict[str, Any]:
        """Retrieves the complete widget hierarchy, slot layouts, and component properties of a Widget Blueprint.
        
        Args:
            widget_path: Path to Widget Blueprint asset (e.g. '/Game/UI/WBP_MainMenu').
        """
        conn = get_unreal_connection()
        if not conn:
            return {"status": "error", "error": "Not connected to Unreal Engine"}
        return conn.send_command("vail_widget_tree_get", {"widget_path": widget_path})

    @mcp.tool()
    def vail_widget_add_element(
        widget_path: str,
        element_type: str,
        element_name: str,
        parent_name: str = "",
        slot_properties: Optional[Dict[str, Any]] = None,
        content_properties: Optional[Dict[str, Any]] = None
    ) -> Dict[str, Any]:
        """Adds a UMG widget element into a widget tree hierarchy with initial slot layout and styling.
        
        Args:
            widget_path: Target Widget Blueprint asset path.
            element_type: Widget class ('CanvasPanel', 'Button', 'TextBlock', 'Image', 'VerticalBox', 'HorizontalBox', 'Overlay', 'ProgressBar', 'Slider', 'Border').
            element_name: Unique variable name for the element.
            parent_name: Parent container widget name (defaults to root widget).
            slot_properties: Slot layout dict (anchors, offsets, position, size, alignment, padding, z_order).
            content_properties: Widget property presets (text, color, font_size, visibility, brush).
        """
        conn = get_unreal_connection()
        if not conn:
            return {"status": "error", "error": "Not connected to Unreal Engine"}
        return conn.send_command("vail_widget_add_element", {
            "widget_path": widget_path,
            "element_type": element_type,
            "element_name": element_name,
            "parent_name": parent_name,
            "slot_properties": slot_properties or {},
            "content_properties": content_properties or {}
        })

    @mcp.tool()
    def vail_widget_set_slot(
        widget_path: str,
        element_name: str,
        layout: Dict[str, Any],
        slot_type: str = "CanvasSlot"
    ) -> Dict[str, Any]:
        """Mutates slot positioning, anchors, and alignment for a widget inside its parent container.
        
        Args:
            widget_path: Path to Widget Blueprint asset.
            element_name: Target widget element name.
            layout: Layout configuration dictionary (e.g. {'anchors': [0.5, 0.5, 0.5, 0.5], 'position': [0, 0], 'size': [200, 50], 'alignment': [0.5, 0.5]}).
            slot_type: Slot class type ('CanvasSlot', 'VerticalBoxSlot', 'HorizontalBoxSlot', 'OverlaySlot').
        """
        conn = get_unreal_connection()
        if not conn:
            return {"status": "error", "error": "Not connected to Unreal Engine"}
        return conn.send_command("vail_widget_set_slot", {
            "widget_path": widget_path,
            "element_name": element_name,
            "layout": layout,
            "slot_type": slot_type
        })

    @mcp.tool()
    def vail_widget_bind_event(
        widget_path: str,
        element_name: str,
        event_name: str = "OnClicked",
        function_name: str = ""
    ) -> Dict[str, Any]:
        """Binds an interactive widget delegate event to a Blueprint graph handler.
        
        Args:
            widget_path: Path to Widget Blueprint asset.
            element_name: Widget component variable name.
            event_name: Delegate event name ('OnClicked', 'OnHovered', 'OnUnhovered', 'OnPressed', 'OnReleased', 'OnValueChanged').
            function_name: Optional handler function name or auto-created event node.
        """
        conn = get_unreal_connection()
        if not conn:
            return {"status": "error", "error": "Not connected to Unreal Engine"}
        return conn.send_command("vail_widget_bind_event", {
            "widget_path": widget_path,
            "element_name": element_name,
            "event_name": event_name,
            "function_name": function_name
        })

    # =========================================================================
    # Phase 3: Sequencer & Cine Timeline Data-Channel
    # =========================================================================

    @mcp.tool()
    def vail_sequencer_query(sequence_path: str) -> Dict[str, Any]:
        """Inspects all tracks, bindings, sections, and keyframe ranges of a LevelSequence asset.
        
        Args:
            sequence_path: Path to LevelSequence asset (e.g. '/Game/Cinematics/LS_IntroCutscene').
        """
        conn = get_unreal_connection()
        if not conn:
            return {"status": "error", "error": "Not connected to Unreal Engine"}
        return conn.send_command("vail_sequencer_query", {"sequence_path": sequence_path})

    @mcp.tool()
    def vail_sequencer_add_track(
        sequence_path: str,
        track_type: str,
        target_object: str = "",
        property_path: str = ""
    ) -> Dict[str, Any]:
        """Adds a track binding to a LevelSequence asset.
        
        Args:
            sequence_path: Path to LevelSequence asset.
            track_type: Track type identifier ('Transform', 'Property', 'Audio', 'CameraCut', 'SkeletalAnimation', 'Fade', 'Event').
            target_object: Actor label, component path, or binding ID.
            property_path: Property path for Property tracks (e.g. 'Intensity', 'LightColor', 'CurrentFocalLength').
        """
        conn = get_unreal_connection()
        if not conn:
            return {"status": "error", "error": "Not connected to Unreal Engine"}
        return conn.send_command("vail_sequencer_add_track", {
            "sequence_path": sequence_path,
            "track_type": track_type,
            "target_object": target_object,
            "property_path": property_path
        })

    @mcp.tool()
    def vail_sequencer_add_key(
        sequence_path: str,
        track_id: str,
        frame_number: int,
        value: Any,
        interp_mode: str = "Linear"
    ) -> Dict[str, Any]:
        """Inserts or modifies keyframes on a LevelSequence track.
        
        Args:
            sequence_path: Path to LevelSequence asset.
            track_id: Track identifier or property binding name.
            frame_number: Sequence frame index.
            value: Formatted value string or numeric value (e.g. '5000.0', '(R=1,G=0.5,B=0,A=1)', 'X=100 Y=0 Z=50').
            interp_mode: Key interpolation curve ('Linear', 'Cubic', 'Constant', 'User').
        """
        conn = get_unreal_connection()
        if not conn:
            return {"status": "error", "error": "Not connected to Unreal Engine"}
        return conn.send_command("vail_sequencer_add_key", {
            "sequence_path": sequence_path,
            "track_id": track_id,
            "frame_number": frame_number,
            "value": str(value),
            "interp_mode": interp_mode
        })

    # =========================================================================
    # Phase 3: Sensory Telemetry Vectors (Critic & Quality Diagnostics)
    # =========================================================================

    @mcp.tool()
    def vail_sense_optical(target_camera: str = "", sample_region: str = "full") -> Dict[str, Any]:
        """Calculates rendering optics metrics without transferring full 4K framebuffers.
        
        Args:
            target_camera: Target CineCameraActor label or empty for active viewport camera.
            sample_region: Region to sample ('full', 'center_spot', 'quadrants').
        """
        conn = get_unreal_connection()
        if not conn:
            return {"status": "error", "error": "Not connected to Unreal Engine"}
        return conn.send_command("vail_sense_optical", {
            "target_camera": target_camera,
            "sample_region": sample_region
        })

    @mcp.tool()
    def vail_sense_spatial(actor_id: str, trace_channel: str = "Visibility") -> Dict[str, Any]:
        """Computes spatial proprioception, ground clearance, surface normal, overlap depth, and frustum visibility.
        
        Args:
            actor_id: Target actor label or identifier.
            trace_channel: Collision trace channel ('Visibility', 'Camera', 'WorldStatic').
        """
        conn = get_unreal_connection()
        if not conn:
            return {"status": "error", "error": "Not connected to Unreal Engine"}
        return conn.send_command("vail_sense_spatial", {
            "actor_id": actor_id,
            "trace_channel": trace_channel
        })

    @mcp.tool()
    def vail_sense_mesh(asset_or_actor: str) -> Dict[str, Any]:
        """Inspects geometric health, triangle count, Nanite status, UV overlap %, and collision complexity.
        
        Args:
            asset_or_actor: StaticMesh / SkeletalMesh asset path or level actor label.
        """
        conn = get_unreal_connection()
        if not conn:
            return {"status": "error", "error": "Not connected to Unreal Engine"}
        return conn.send_command("vail_sense_mesh", {"asset_or_actor": asset_or_actor})

    @mcp.tool()
    def vail_sense_shader(material_path: str) -> Dict[str, Any]:
        """Inspects shader complexity, instruction count, texture samplers, roughness, and Lumen cache status.
        
        Args:
            material_path: Material or MaterialInstance asset path (e.g. '/Game/Materials/M_HeroShader').
        """
        conn = get_unreal_connection()
        if not conn:
            return {"status": "error", "error": "Not connected to Unreal Engine"}
        return conn.send_command("vail_sense_shader", {"material_path": material_path})

    # =========================================================================
    # Phase 4: Subsystems & Specialized Production Tooling
    # =========================================================================

    @mcp.tool()
    def vail_landscape_create(
        section_size: int = 63,
        sections_per_component: int = 1,
        component_count_x: int = 8,
        component_count_y: int = 8,
        material_path: str = "",
        location: Optional[Any] = None
    ) -> Dict[str, Any]:
        """Creates a landscape terrain actor with specified grid resolution and material.
        
        Args:
            section_size: Quads per section (7, 15, 31, 63, 127, 255; default: 63).
            sections_per_component: Sections per component (1 or 2; default: 1).
            component_count_x: Component count along X axis (default: 8).
            component_count_y: Component count along Y axis (default: 8).
            material_path: Landscape material package path.
            location: World placement origin [X, Y, Z] (default: [0, 0, 0]).
        """
        conn = get_unreal_connection()
        if not conn:
            return {"status": "error", "error": "Not connected to Unreal Engine"}
        return conn.send_command("vail_landscape_create", {
            "section_size": section_size,
            "sections_per_component": sections_per_component,
            "component_count_x": component_count_x,
            "component_count_y": component_count_y,
            "material_path": material_path,
            "location": location if location is not None else [0.0, 0.0, 0.0]
        })

    @mcp.tool()
    def vail_landscape_sculpt(
        target_location: Any,
        brush_mode: str = "Sculpt",
        radius: float = 2048.0,
        falloff: float = 0.5,
        strength: float = 0.3
    ) -> Dict[str, Any]:
        """Applies a sculpting brush mutation to the landscape heightmap.
        
        Args:
            target_location: World target hit point as [X, Y, Z].
            brush_mode: Sculpting mode ('Sculpt', 'Smooth', 'Flatten', 'Ramp', 'Erosion').
            radius: Brush radius in Unreal units (default: 2048.0).
            falloff: Falloff factor between 0.0 and 1.0 (default: 0.5).
            strength: Delta height intensity (default: 0.3).
        """
        conn = get_unreal_connection()
        if not conn:
            return {"status": "error", "error": "Not connected to Unreal Engine"}
        return conn.send_command("vail_landscape_sculpt", {
            "target_location": target_location,
            "brush_mode": brush_mode,
            "radius": radius,
            "falloff": falloff,
            "strength": strength
        })

    @mcp.tool()
    def vail_landscape_paint(
        layer_name: str,
        target_location: Any,
        radius: float = 1024.0,
        strength: float = 1.0
    ) -> Dict[str, Any]:
        """Paints landscape layer weightmaps onto terrain.
        
        Args:
            layer_name: Target landscape layer name (e.g. 'Grass', 'Rock', 'Mud', 'Snow').
            target_location: World target hit point [X, Y, Z].
            radius: Brush radius (default: 1024.0).
            strength: Paint weight strength (default: 1.0).
        """
        conn = get_unreal_connection()
        if not conn:
            return {"status": "error", "error": "Not connected to Unreal Engine"}
        return conn.send_command("vail_landscape_paint", {
            "layer_name": layer_name,
            "target_location": target_location,
            "radius": radius,
            "strength": strength
        })

    @mcp.tool()
    def vail_foliage_scatter(
        mesh_path: str,
        center_location: Any,
        radius: float = 3000.0,
        density: int = 50,
        scale_min: float = 0.8,
        scale_max: float = 1.2,
        align_to_normal: bool = True,
        random_yaw: bool = True
    ) -> Dict[str, Any]:
        """Procedurally scatters instanced static mesh foliage within an area.
        
        Args:
            mesh_path: StaticMesh asset path to instance (e.g. '/Game/Environment/Trees/SM_PineTree').
            center_location: Center point coordinates [X, Y, Z].
            radius: Scatter radius (default: 3000.0).
            density: Number of instances to scatter (default: 50).
            scale_min: Minimum random scale factor (default: 0.8).
            scale_max: Maximum random scale factor (default: 1.2).
            align_to_normal: Align instance up-vector to terrain surface normal (default: True).
            random_yaw: Apply random 360-degree yaw rotation (default: True).
        """
        conn = get_unreal_connection()
        if not conn:
            return {"status": "error", "error": "Not connected to Unreal Engine"}
        return conn.send_command("vail_foliage_scatter", {
            "mesh_path": mesh_path,
            "center_location": center_location,
            "radius": radius,
            "density": density,
            "scale_min": scale_min,
            "scale_max": scale_max,
            "align_to_normal": align_to_normal,
            "random_yaw": random_yaw
        })

    @mcp.tool()
    def vail_foliage_query(bounds_center: Optional[Any] = None, bounds_radius: float = 5000.0) -> Dict[str, Any]:
        """Queries foliage instance counts, mesh species, and coverage within a volume.
        
        Args:
            bounds_center: Center of query volume [X, Y, Z] (default: [0, 0, 0]).
            bounds_radius: Search radius (default: 5000.0).
        """
        conn = get_unreal_connection()
        if not conn:
            return {"status": "error", "error": "Not connected to Unreal Engine"}
        return conn.send_command("vail_foliage_query", {
            "bounds_center": bounds_center if bounds_center is not None else [0.0, 0.0, 0.0],
            "bounds_radius": bounds_radius
        })

    @mcp.tool()
    def vail_control_rig_set_transform(
        rig_path: str,
        control_name: str,
        location: Optional[Any] = None,
        rotation: Optional[Any] = None
    ) -> Dict[str, Any]:
        """Sets a target transform on a Control Rig bone or control solver goal.
        
        Args:
            rig_path: Control Rig asset path or SkeletalMeshComponent.
            control_name: Control bone name (e.g. 'hand_r_ctrl', 'head_ctrl', 'pelvis_ctrl').
            location: Target location [X, Y, Z].
            rotation: Target rotation [Pitch, Yaw, Roll].
        """
        conn = get_unreal_connection()
        if not conn:
            return {"status": "error", "error": "Not connected to Unreal Engine"}
        return conn.send_command("vail_control_rig_set_transform", {
            "rig_path": rig_path,
            "control_name": control_name,
            "location": location if location is not None else [0.0, 0.0, 0.0],
            "rotation": rotation if rotation is not None else [0.0, 0.0, 0.0]
        })

    @mcp.tool()
    def vail_control_rig_query(rig_path: str) -> Dict[str, Any]:
        """Queries the hierarchy of controls, bones, and limits for a Control Rig.
        
        Args:
            rig_path: Control Rig asset path or actor name.
        """
        conn = get_unreal_connection()
        if not conn:
            return {"status": "error", "error": "Not connected to Unreal Engine"}
        return conn.send_command("vail_control_rig_query", {"rig_path": rig_path})

    @mcp.tool()
    def vail_audio_play(
        sound_path: str,
        location: Optional[Any] = None,
        volume_multiplier: float = 1.0,
        pitch_multiplier: float = 1.0
    ) -> Dict[str, Any]:
        """Auditions or triggers a SoundCue, SoundWave, or MetaSound asset.
        
        Args:
            sound_path: Audio asset path (e.g. '/Game/Audio/MS_Ambience_Forest').
            location: Optional 3D world location [X, Y, Z]. If omitted, plays 2D.
            volume_multiplier: Volume multiplier (default: 1.0).
            pitch_multiplier: Pitch multiplier (default: 1.0).
        """
        conn = get_unreal_connection()
        if not conn:
            return {"status": "error", "error": "Not connected to Unreal Engine"}
        return conn.send_command("vail_audio_play", {
            "sound_path": sound_path,
            "location": location,
            "volume_multiplier": volume_multiplier,
            "pitch_multiplier": pitch_multiplier
        })

    @mcp.tool()
    def vail_audio_set_parameter(sound_actor: str, parameter_name: str, value: Any) -> Dict[str, Any]:
        """Sets a runtime parameter on an active MetaSound audio graph.
        
        Args:
            sound_actor: AudioComponent or Sound actor name.
            parameter_name: MetaSound input parameter name (e.g. 'PitchMod', 'ReverbGain', 'TriggerPlay').
            value: Formatted value or numeric float/bool.
        """
        conn = get_unreal_connection()
        if not conn:
            return {"status": "error", "error": "Not connected to Unreal Engine"}
        return conn.send_command("vail_audio_set_parameter", {
            "sound_actor": sound_actor,
            "parameter_name": parameter_name,
            "value": str(value)
        })

    @mcp.tool()
    def vail_project_build(
        target_platform: str = "Windows",
        build_config: str = "Development",
        clean: bool = False
    ) -> Dict[str, Any]:
        """Triggers project compilation, cooking, or standalone packaging pipelines.
        
        Args:
            target_platform: Target platform ('Windows', 'Linux', 'Android', 'iOS'; default: 'Windows').
            build_config: Build configuration ('Development', 'Shipping', 'DebugGame'; default: 'Development').
            clean: If True, performs a clean rebuild (default: False).
        """
        conn = get_unreal_connection()
        if not conn:
            return {"status": "error", "error": "Not connected to Unreal Engine"}
        return conn.send_command("vail_project_build", {
            "target_platform": target_platform,
            "build_config": build_config,
            "clean": clean
        })


