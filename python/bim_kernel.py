"""
BimKernel Native Archicad 30 / 29 C++ Kernel SDK (v3.0.0)
=========================================================
Python wrapper for the `BimKernel.apx` native C++ Add-On.
Exposes 22 kernel-level commands under the `"BimKernel"` namespace via
`ACAPI_AddOnAddOnCommunication_InstallAddOnCommandHandler`.
"""

from typing import Any, Dict, List, Optional, Union
from archicad import ACConnection

NAMESPACE = "BimKernel"
SDK_VERSION = "3.0.0"


def _get_connection(conn: Optional[ACConnection] = None) -> ACConnection:
    if conn is None:
        conn = ACConnection.connect()
        if not conn:
            raise RuntimeError("Could not connect to Archicad. Ensure Archicad 30 or 29 is open.")
    return conn


def _exec(
    command_name: str,
    params: Optional[Dict[str, Any]] = None,
    conn: Optional[ACConnection] = None,
) -> Dict[str, Any]:
    conn = _get_connection(conn)
    cmd_id = conn.types.AddOnCommandId(NAMESPACE, command_name)
    return conn.commands.ExecuteAddOnCommand(cmd_id, params or {})


# =============================================================================
# 1. Ping & Diagnostics
# =============================================================================

def ping(conn: Optional[ACConnection] = None) -> Dict[str, Any]:
    """Verify connection to the BimKernel C++ Kernel v3.0.0 and list registered commands."""
    return _exec("Ping", {}, conn)


# =============================================================================
# 2. Layer Management (List, Move Elements, Lock/Unlock, Show/Hide, Delete)
# =============================================================================

def list_layers(conn: Optional[ACConnection] = None) -> Dict[str, Any]:
    """List all layers in the project with index, name, GUID, locked, and hidden states."""
    return _exec("ListLayers", {}, conn)


def change_elements_layer(
    element_guids: List[Union[str, Any]],
    layer_name: str,
    conn: Optional[ACConnection] = None,
) -> Dict[str, Any]:
    """Move elements to the specified layer (automatically creates the layer if missing)."""
    guids_str = [str(g.guid) if hasattr(g, "guid") else str(g) for g in element_guids]
    return _exec(
        "ChangeElementsLayer",
        {"layerName": layer_name, "elements": guids_str},
        conn,
    )


def modify_layers_state(
    layer_names: List[str],
    locked: Optional[bool] = None,
    hidden: Optional[bool] = None,
    delete_layer: bool = False,
    conn: Optional[ACConnection] = None,
) -> Dict[str, Any]:
    """Batch lock/unlock, show/hide, or delete layers in the Archicad kernel."""
    params: Dict[str, Any] = {"layers": layer_names, "deleteLayer": delete_layer}
    if locked is not None:
        params["locked"] = locked
    if hidden is not None:
        params["hidden"] = hidden
    return _exec("ModifyLayersState", params, conn)


# =============================================================================
# 3. Hotlink & Module Lifecycle (List, Import, Break, Delete)
# =============================================================================

def list_hotlinks(conn: Optional[ACConnection] = None) -> Dict[str, Any]:
    """List all Hotlink nodes in the current Archicad project."""
    return _exec("ManageHotlinks", {"action": "list"}, conn)


def import_hotlink_module(
    file_path: str,
    name: str = "BimKernelHotlink",
    layer_name: str = "",
    x: float = 0.0,
    y: float = 0.0,
    z: float = 0.0,
    floor_ind: int = 0,
    break_link: bool = False,
    single_story: bool = False,
    conn: Optional[ACConnection] = None,
) -> Dict[str, Any]:
    """
    Create a Hotlink Node from an external file (.mod, .pln, .ifc), place an instance
    at (x, y, z) on `floor_ind`, and optionally break the link immediately to convert
    all imported geometries into native Archicad elements.
    """
    return _exec(
        "ManageHotlinks",
        {
            "action": "import",
            "filePath": file_path,
            "name": name,
            "layerName": layer_name,
            "x": float(x),
            "y": float(y),
            "z": float(z),
            "floorInd": int(floor_ind),
            "breakLink": bool(break_link),
            "singleStory": bool(single_story),
        },
        conn,
    )


def break_hotlinks(
    node_guid: Optional[str] = None, conn: Optional[ACConnection] = None
) -> Dict[str, Any]:
    """Break a specific hotlink node (or all hotlinks if node_guid is None) into native elements."""
    params: Dict[str, Any] = {"action": "break"}
    if node_guid:
        params["nodeGuid"] = str(node_guid)
    return _exec("ManageHotlinks", params, conn)


def delete_hotlinks(
    node_guid: Optional[str] = None, conn: Optional[ACConnection] = None
) -> Dict[str, Any]:
    """Delete a specific hotlink node (or all hotlinks if node_guid is None)."""
    params: Dict[str, Any] = {"action": "delete"}
    if node_guid:
        params["nodeGuid"] = str(node_guid)
    return _exec("ManageHotlinks", params, conn)


# =============================================================================
# 4. Element Deletion, Transformation & Parametric Geometry Modification
# =============================================================================

def delete_elements(
    element_guids: List[Union[str, Any]], conn: Optional[ACConnection] = None
) -> Dict[str, Any]:
    """Delete elements by GUID in a single undoable kernel transaction."""
    guids_str = [str(g.guid) if hasattr(g, "guid") else str(g) for g in element_guids]
    return _exec("DeleteElements", {"elements": guids_str}, conn)


def transform_elements(
    element_guids: List[Union[str, Any]],
    edit_action: str = "drag",
    dx: float = 0.0,
    dy: float = 0.0,
    dz: float = 0.0,
    origin_x: float = 0.0,
    origin_y: float = 0.0,
    angle_deg: float = 0.0,
    copy: bool = False,
    conn: Optional[ACConnection] = None,
) -> Dict[str, Any]:
    """
    Transform elements using ACAPI_Element_Edit (`drag`, `elevate`, `rotate`).
    Automatically preserves groups, associative dimensions, and wall openings.
    """
    guids_str = [str(g.guid) if hasattr(g, "guid") else str(g) for g in element_guids]
    return _exec(
        "ModifyElementsGeometry",
        {
            "editAction": edit_action,
            "elements": guids_str,
            "dx": float(dx),
            "dy": float(dy),
            "dz": float(dz),
            "originX": float(origin_x),
            "originY": float(origin_y),
            "angleDeg": float(angle_deg),
            "copy": bool(copy),
        },
        conn,
    )


def modify_elements_geometry(
    modifications: List[Dict[str, Any]], conn: Optional[ACConnection] = None
) -> Dict[str, Any]:
    """
    Modify element coordinates, heights, offsets, thicknesses, story index, layer, materials, composites, or Element ID.
    Each dict in `modifications` requires `"guid"` and any of:
    `"elemId"`, `"floorInd"`, `"layerName"`, `"begX"`, `"begY"`, `"endX"`, `"endY"`,
    `"posX"`, `"posY"`, `"height"`, `"bottomOffset"`, `"thickness"`, `"level"`, `"offset"`,
    `"buildingMaterial"`, `"composite"`, `"contPen"`, `"contPen3D"`, `"pen"`,
    `"xRatio"`, `"yRatio"`, `"useXYFixSize"`, `"angle"`, `"objLoc"`, `"lower"`, `"width"`, `"revealDepthOffset"`.
    """
    return _exec("ModifyElementsGeometry", {"modifications": modifications}, conn)


# =============================================================================
# 5. Solid Element Operations (SEO Boolean Link / Unlink)
# =============================================================================

def solid_element_operations(
    targets: List[Union[str, Any]],
    operators: List[Union[str, Any]],
    operation: str = "substract_upward",
    action: str = "link",
    conn: Optional[ACConnection] = None,
) -> Dict[str, Any]:
    """
    Create or remove native Solid Element Operations (SEO) boolean links between targets and operators.
    `operation`: `'substract'`, `'substract_upward'`, `'substract_downward'`, `'intersect'`, or `'add'`.
    `action`: `'link'` or `'unlink'`.
    """
    t_str = [str(g.guid) if hasattr(g, "guid") else str(g) for g in targets]
    o_str = [str(g.guid) if hasattr(g, "guid") else str(g) for g in operators]
    return _exec(
        "SolidElementOperations",
        {
            "action": action,
            "operation": operation,
            "targets": t_str,
            "operators": o_str,
        },
        conn,
    )


# =============================================================================
# 6. Native Parametric Element Creation (Wall, Column, Beam, Door, Window, Object, Slab, Mesh, Zone)
# =============================================================================

def create_native_elements(
    elements: List[Dict[str, Any]], conn: Optional[ACConnection] = None
) -> Dict[str, Any]:
    """
    Batch create native Archicad elements in a single undoable transaction.
    Supported `"type"` values: `"Wall"`, `"Column"`, `"Beam"`, `"Door"`, `"Window"`, `"Object"`, `"Slab"`, `"Mesh"`, `"Zone"`.
    - For `"Door"` and `"Window"`, pass `"wallGuid"`, `"objLoc"`, `"lower"`, `"width"`, `"height"`.
    - For `"Slab"`, `"Mesh"`, and `"Zone"`, pass `"polygon": [{"x": x1, "y": y1}, ...]`.
    """
    return _exec("CreateNativeElements", {"elements": elements}, conn)


# =============================================================================
# 7. GDL Parameters & Live Script Injection (API_ElementMemo & API_LibPartSection)
# =============================================================================

def get_gdl_parameters(
    element_guid: Union[str, Any], conn: Optional[ACConnection] = None
) -> Dict[str, Any]:
    """Retrieve all GDL parameters (`API_AddParType`) and 2D/3D GDL scripts from an element."""
    g_str = str(element_guid.guid) if hasattr(element_guid, "guid") else str(element_guid)
    return _exec("GetSetGDLParameters", {"guid": g_str, "action": "get"}, conn)


def set_gdl_parameters(
    element_guid: Union[str, Any],
    updates: List[Dict[str, Any]],
    conn: Optional[ACConnection] = None,
) -> Dict[str, Any]:
    """
    Modify GDL parameters on an element's `API_ElementMemo`.
    Each item in `updates` should be `{"name": "<paramName>", "valueNum": 1.23}`
    or `{"name": "<paramName>", "valueStr": "text"}`.
    """
    g_str = str(element_guid.guid) if hasattr(element_guid, "guid") else str(element_guid)
    return _exec(
        "GetSetGDLParameters",
        {"guid": g_str, "action": "set", "updates": updates},
        conn,
    )


def set_gdl_scripts(
    element_guid: Union[str, Any],
    script_3d: Optional[str] = None,
    script_2d: Optional[str] = None,
    conn: Optional[ACConnection] = None,
) -> Dict[str, Any]:
    """Live-update the 3D and/or 2D GDL script (`API_Sect3DScript`, `API_Sect2DScript`) of an `API_ObjectID`'s Library Part."""
    g_str = str(element_guid.guid) if hasattr(element_guid, "guid") else str(element_guid)
    params: Dict[str, Any] = {"guid": g_str, "action": "setScript"}
    if script_3d is not None:
        params["script3D"] = script_3d
    if script_2d is not None:
        params["script2D"] = script_2d
    return _exec("GetSetGDLParameters", params, conn)


# =============================================================================
# 8. Building Materials, Composites & 3D Surfaces
# =============================================================================

def list_building_materials(conn: Optional[ACConnection] = None) -> Dict[str, Any]:
    """List all Building Materials with connection priority, cut fill, pens, and surface material."""
    return _exec("ModifyBuildingMaterials", {"action": "list"}, conn)


def modify_building_materials(
    materials: List[Dict[str, Any]], conn: Optional[ACConnection] = None
) -> Dict[str, Any]:
    """
    Modify or create Archicad Building Materials (`connPriority`, `cutFill`, `cutFillPen`, `cutFillBackgroundPen`, `cutMaterial`).
    Set `"createIfMissing": True` inside a material dict to create it if not found.
    """
    return _exec("ModifyBuildingMaterials", {"action": "modify", "materials": materials}, conn)


def list_composites(conn: Optional[ACConnection] = None) -> Dict[str, Any]:
    """List all Composite structures (`API_CompWallID`) and their exact multi-skin definitions."""
    return _exec("ManageComposites", {"action": "list"}, conn)


def create_or_modify_composite(
    name: str,
    skins: List[Dict[str, Any]],
    conn: Optional[ACConnection] = None,
) -> Dict[str, Any]:
    """
    Create or modify a multi-skin Composite structure (`API_CompWallID`).
    Each skin dict in `skins` supports:
    `{"buildingMaterial": "<nameOrGuid>", "thickness": 0.14, "framePen": 1, "isCore": True, "isFinish": False}`.
    """
    return _exec(
        "ManageComposites",
        {"action": "modify", "name": name, "skins": skins},
        conn,
    )


def list_surfaces(conn: Optional[ACConnection] = None) -> Dict[str, Any]:
    """List all 3D Surface Materials (`API_MaterialID`) with RGB colors, transparency, and shininess."""
    return _exec("ManageSurfaces", {"action": "list"}, conn)


def modify_surfaces(
    surfaces: List[Dict[str, Any]], conn: Optional[ACConnection] = None,
) -> Dict[str, Any]:
    """
    Create or modify 3D Surface Materials (`API_MaterialID`).
    Each dict in `surfaces` requires `"surface"` (name or GUID) and any of:
    `"red"` (0.0-1.0), `"green"` (0.0-1.0), `"blue"` (0.0-1.0), `"transpPc"` (0-100),
    `"shine"` (0-10000), `"ambientPc"`, `"diffusePc"`, `"specularPc"`.
    """
    return _exec("ManageSurfaces", {"action": "modify", "surfaces": surfaces}, conn)


# =============================================================================
# 9. Story Settings & Building Levels
# =============================================================================

def list_stories(conn: Optional[ACConnection] = None) -> Dict[str, Any]:
    """List all building stories (`API_StoryInfo`) with index, name, elevation level, and story height."""
    return _exec("ManageStories", {"action": "list"}, conn)


def manage_story(
    action: str,
    index: int = 0,
    name: Optional[str] = None,
    height: Optional[float] = None,
    elevation: Optional[float] = None,
    conn: Optional[ACConnection] = None,
) -> Dict[str, Any]:
    """
    Execute story operations (`'goTo'`, `'setHeight'`, `'setElevation'`, `'rename'`, `'insAbove'`, `'insBelow'`, `'delete'`).
    """
    params: Dict[str, Any] = {"action": action, "index": int(index)}
    if name is not None:
        params["name"] = name
    if height is not None:
        params["height"] = float(height)
    if elevation is not None:
        params["elevation"] = float(elevation)
    return _exec("ManageStories", params, conn)


# =============================================================================
# 10. Batch Kernel Element Telemetry & 3D Bounding Box Inspector
# =============================================================================

def inspect_elements(
    element_guids: Optional[List[Union[str, Any]]] = None,
    types: Optional[List[str]] = None,
    floor_ind: Optional[int] = None,
    conn: Optional[ACConnection] = None,
) -> Dict[str, Any]:
    """
    High-speed C++ kernel telemetry: returns 3D bounding boxes (`xMin..zMax`), story, layer,
    Element ID, coordinates, thicknesses, Building Materials, Composites, and opening dimensions
    in a single call.
    """
    params: Dict[str, Any] = {}
    if element_guids:
        params["elements"] = [str(g.guid) if hasattr(g, "guid") else str(g) for g in element_guids]
    if types:
        params["types"] = types
    if floor_ind is not None:
        params["floorInd"] = int(floor_ind)
    return _exec("InspectElements", params, conn)


# =============================================================================
# 11. Navigator View Map, Layout Book, Cut Plane, Publisher & Direct Export
# =============================================================================

def list_navigator_views(conn: Optional[ACConnection] = None) -> Dict[str, Any]:
    """List all Views in the Public View Map with layer combination, MVO, pen set, and scale."""
    return _exec("ManageNavigatorViews", {"action": "list"}, conn)


def modify_navigator_view(
    view_guid: str,
    name: Optional[str] = None,
    layer_combination: Optional[str] = None,
    model_view_opt_name: Optional[str] = None,
    drawing_scale: Optional[int] = None,
    conn: Optional[ACConnection] = None,
) -> Dict[str, Any]:
    """Modify a Navigator View's name, layer combination, Model View Options, or drawing scale."""
    params: Dict[str, Any] = {"action": "modify", "viewGuid": str(view_guid)}
    if name is not None:
        params["name"] = name
    if layer_combination is not None:
        params["layerCombination"] = layer_combination
    if model_view_opt_name is not None:
        params["modelViewOptName"] = model_view_opt_name
    if drawing_scale is not None:
        params["drawingScale"] = int(drawing_scale)
    return _exec("ManageNavigatorViews", params, conn)


def go_to_navigator_view(view_guid: str, conn: Optional[ACConnection] = None) -> Dict[str, Any]:
    """Activate and navigate to the specified Navigator View GUID."""
    return _exec("ManageNavigatorViews", {"action": "goTo", "viewGuid": str(view_guid)}, conn)


def direct_export(
    file_path: str,
    format: str = "ifc",
    view_2d: bool = True,
    conn: Optional[ACConnection] = None,
) -> Dict[str, Any]:
    """
    Export the active project or view directly to disk (`'ifc'`, `'pdf'`, `'pla'`, `'pln'`, `'png'`).
    """
    return _exec(
        "DirectExport",
        {"filePath": str(file_path), "format": format, "view2D": bool(view_2d)},
        conn,
    )


def list_layouts(conn: Optional[ACConnection] = None) -> Dict[str, Any]:
    """List all Layout databases in the project's Layout Book."""
    return _exec("ListLayoutDatabases", {}, conn)


def place_drawing_on_layout(
    layout_guid: str,
    view_guid: str,
    pos_x: float = 0.21,
    pos_y: float = 0.1485,
    ratio: float = 1.0,
    conn: Optional[ACConnection] = None,
) -> Dict[str, Any]:
    """Place a Navigator View onto a Layout sheet as an `API_DrawingID` element."""
    return _exec(
        "PlaceDrawingOnLayout",
        {
            "layoutGuid": str(layout_guid),
            "viewGuid": str(view_guid),
            "posX": float(pos_x),
            "posY": float(pos_y),
            "ratio": float(ratio),
        },
        conn,
    )


def place_drawings_batch(
    placements: List[Dict[str, Any]], conn: Optional[ACConnection] = None
) -> Dict[str, Any]:
    """Batch place multiple Navigator Views onto Layout sheets in one transaction."""
    return _exec("PlaceDrawingOnLayout", {"placements": placements}, conn)


def set_cut_plane(
    cut_level: float = 3.20,
    top_cut_level: Optional[float] = None,
    bottom_cut_level: Optional[float] = None,
    conn: Optional[ACConnection] = None,
) -> Dict[str, Any]:
    """Set the global Floor Plan Cut Plane elevation (`currCutLevel`)."""
    params: Dict[str, Any] = {"cutLevel": float(cut_level)}
    if top_cut_level is not None:
        params["topCutLevel"] = float(top_cut_level)
    if bottom_cut_level is not None:
        params["bottomCutLevel"] = float(bottom_cut_level)
    return _exec("SetFloorPlanCutPlane", params, conn)


def publish_set(
    set_name: str = "", output_path: str = "", conn: Optional[ACConnection] = None
) -> Dict[str, Any]:
    """Trigger native Archicad Publisher Set export."""
    return _exec(
        "PublishSet", {"setName": set_name, "outputPath": output_path}, conn
    )


def database_and_view_control(
    action: str = "rebuild",
    regenerate: bool = True,
    path: str = "",
    conn: Optional[ACConnection] = None,
) -> Dict[str, Any]:
    """Execute `'rebuild'`, `'redraw'`, `'save'`, or `'saveAs'` on the active Archicad project."""
    params: Dict[str, Any] = {"action": action, "regenerate": bool(regenerate)}
    if path:
        params["path"] = str(path)
    return _exec("DatabaseAndViewControl", params, conn)


if __name__ == "__main__":
    print(f"Testing BimKernel SDK v{SDK_VERSION}...")
    print(ping())
