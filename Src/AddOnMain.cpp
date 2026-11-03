#include "APIEnvir.h"
#include "ACAPinc.h"

#include "ResourceIds.hpp"
#include "RS.hpp"
#include "DGModule.hpp"
#include "UniString.hpp"
#include "ObjectState.hpp"
#include "StringConversion.hpp"
#include "API_Guid.hpp"
#include "APIdefs_AttributeIndex.hpp"
#include "Owner.hpp"
#include <cmath>
#include <cstring>

static const GSResID AddOnInfoID         = ID_ADDON_INFO;
static const Int32   AddOnNameID         = 1;
static const Int32   AddOnDescriptionID  = 2;
static const short   AddOnMenuID         = ID_ADDON_MENU;
static const Int32   AddOnCommandID      = 1;

// =============================================================================
// Helper Utilities
// =============================================================================

static API_AttributeIndex FindOrCreateLayerByName (const GS::UniString& layerName)
{
	if (layerName.IsEmpty ()) {
		return APIApplicationLayerAttributeIndex;
	}

	API_Attr_Head attrHead {};
	attrHead.typeID = API_LayerID;
	GS::UniString searchName = layerName;
	attrHead.uniStringNamePtr = &searchName;
	if (ACAPI_Attribute_Search (&attrHead) == NoError) {
		return attrHead.index;
	}

	API_Attribute attrib {};
	attrib.header.typeID = API_LayerID;
	attrib.header.uniStringNamePtr = &searchName;
	if (ACAPI_Attribute_Create (&attrib, nullptr) == NoError) {
		return attrib.header.index;
	}

	return APIInvalidAttributeIndex;
}

static API_AttributeIndex FindBuildingMaterial (const GS::UniString& identifier)
{
	if (identifier.IsEmpty ()) {
		return APIInvalidAttributeIndex;
	}

	API_Guid bmatGuid = APIGuidFromString (identifier.ToCStr ());
	if (bmatGuid != APINULLGuid) {
		API_Attr_Head attrHead {};
		attrHead.typeID = API_BuildingMaterialID;
		attrHead.guid = bmatGuid;
		if (ACAPI_Attribute_Search (&attrHead) == NoError) {
			return attrHead.index;
		}
	}

	API_Attr_Head attrHead {};
	attrHead.typeID = API_BuildingMaterialID;
	GS::UniString searchName = identifier;
	attrHead.uniStringNamePtr = &searchName;
	if (ACAPI_Attribute_Search (&attrHead) == NoError) {
		return attrHead.index;
	}

	return APIInvalidAttributeIndex;
}

static API_AttributeIndex FindComposite (const GS::UniString& identifier)
{
	if (identifier.IsEmpty ()) {
		return APIInvalidAttributeIndex;
	}

	API_Guid compGuid = APIGuidFromString (identifier.ToCStr ());
	if (compGuid != APINULLGuid) {
		API_Attr_Head attrHead {};
		attrHead.typeID = API_CompWallID;
		attrHead.guid = compGuid;
		if (ACAPI_Attribute_Search (&attrHead) == NoError) {
			return attrHead.index;
		}
	}

	API_Attr_Head attrHead {};
	attrHead.typeID = API_CompWallID;
	GS::UniString searchName = identifier;
	attrHead.uniStringNamePtr = &searchName;
	if (ACAPI_Attribute_Search (&attrHead) == NoError) {
		return attrHead.index;
	}

	return APIInvalidAttributeIndex;
}

static API_AttributeIndex FindSurfaceMaterial (const GS::UniString& identifier)
{
	if (identifier.IsEmpty ()) {
		return APIInvalidAttributeIndex;
	}

	API_Guid matGuid = APIGuidFromString (identifier.ToCStr ());
	if (matGuid != APINULLGuid) {
		API_Attr_Head attrHead {};
		attrHead.typeID = API_MaterialID;
		attrHead.guid = matGuid;
		if (ACAPI_Attribute_Search (&attrHead) == NoError) {
			return attrHead.index;
		}
	}

	API_Attr_Head attrHead {};
	attrHead.typeID = API_MaterialID;
	GS::UniString searchName = identifier;
	attrHead.uniStringNamePtr = &searchName;
	if (ACAPI_Attribute_Search (&attrHead) == NoError) {
		return attrHead.index;
	}

	return APIInvalidAttributeIndex;
}

static GS::UniString GetAttributeNameByIndex (API_AttrTypeID typeID, API_AttributeIndex index)
{
	if (index == APIInvalidAttributeIndex) {
		return "";
	}
	API_Attribute attr {};
	attr.header.typeID = typeID;
	attr.header.index = index;
	if (ACAPI_Attribute_Get (&attr) == NoError) {
		return GS::UniString (attr.header.name);
	}
	return "";
}

// =============================================================================
// BimKernel Base Command
// =============================================================================

class BimKernelBaseCommand : public API_AddOnCommand {
public:
	virtual GS::String GetNamespace () const override
	{
		return "BimKernel";
	}

	virtual GS::Optional<GS::UniString> GetSchemaDefinitions () const override
	{
		return GS::NoValue;
	}

	virtual GS::Optional<GS::UniString> GetInputParametersSchema () const override
	{
		return GS::NoValue;
	}

	virtual GS::Optional<GS::UniString> GetResponseSchema () const override
	{
		return GS::NoValue;
	}

	virtual API_AddOnCommandExecutionPolicy GetExecutionPolicy () const override
	{
		return API_AddOnCommandExecutionPolicy::ScheduleForExecutionOnMainThread;
	}

	virtual bool IsProcessWindowVisible () const override
	{
		return false;
	}

	virtual void OnResponseValidationFailed (const GS::ObjectState& /*response*/) const override
	{
	}
};

// =============================================================================
// 1. Ping Command
// =============================================================================

class PingCommand : public BimKernelBaseCommand {
public:
	virtual GS::String GetName () const override
	{
		return "Ping";
	}

	virtual GS::ObjectState Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const override
	{
		GS::ObjectState res;
		res.Add ("status", "ok");
		res.Add ("addOn", "BimKernel");
		res.Add ("version", "3.0.0");
		res.Add ("accessLevel", 5);

		API_ServerApplicationInfo appInfo {};
		ACAPI_AddOnIdentification_Application (&appInfo);
		res.Add ("archicadMainVersion", (Int32) appInfo.mainVersion);
		res.Add ("archicadReleaseVersion", (Int32) appInfo.releaseVersion);
		res.Add ("archicadBuildNum", (Int32) appInfo.buildNum);
		res.Add ("message", "BimKernel Native C++ Kernel v3.0.0 connected.");

		GS::Array<GS::UniString> commands;
		commands.Push ("Ping");
		commands.Push ("ChangeElementsLayer");
		commands.Push ("ManageHotlinks");
		commands.Push ("DeleteElements");
		commands.Push ("ListLayers");
		commands.Push ("ModifyLayersState");
		commands.Push ("PublishSet");
		commands.Push ("ListLayoutDatabases");
		commands.Push ("PlaceDrawingOnLayout");
		commands.Push ("SetFloorPlanCutPlane");
		commands.Push ("SolidElementOperations");
		commands.Push ("ModifyElementsGeometry");
		commands.Push ("CreateNativeElements");
		commands.Push ("GetSetGDLParameters");
		commands.Push ("DatabaseAndViewControl");
		commands.Push ("ModifyBuildingMaterials");
		commands.Push ("ManageComposites");
		commands.Push ("ManageSurfaces");
		commands.Push ("ManageStories");
		commands.Push ("InspectElements");
		commands.Push ("ManageNavigatorViews");
		commands.Push ("DirectExport");
		res.Add ("commandCount", (Int32) commands.GetSize ());
		res.Add ("commands", commands);
		return res;
	}
};

// =============================================================================
// 2. ChangeElementsLayer Command
// =============================================================================

class ChangeElementsLayerCommand : public BimKernelBaseCommand {
public:
	virtual GS::String GetName () const override
	{
		return "ChangeElementsLayer";
	}

	virtual GS::ObjectState Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const override
	{
		GS::ObjectState res;
		GS::UniString targetLayerName;
		if (!parameters.Get ("layerName", targetLayerName) || targetLayerName.IsEmpty ()) {
			res.Add ("success", false);
			res.Add ("error", "layerName parameter is required");
			return res;
		}

		GS::Array<GS::UniString> elements;
		if (!parameters.Get ("elements", elements) || elements.IsEmpty ()) {
			res.Add ("success", false);
			res.Add ("error", "elements array parameter is required");
			return res;
		}

		API_AttributeIndex targetLayerIndex = FindOrCreateLayerByName (targetLayerName);
		if (targetLayerIndex == APIInvalidAttributeIndex) {
			res.Add ("success", false);
			res.Add ("error", "Could not find or create target layer");
			return res;
		}

		UInt32 modifiedCount = 0;
		GSErrCode lastErr = NoError;
		ACAPI_CallUndoableCommand ("Change Elements Layer", [&] () -> GSErrCode {
			for (UIndex i = 0; i < elements.GetSize (); ++i) {
				API_Guid elemGuid = APIGuidFromString (elements[i].ToCStr ());
				API_Element element {};
				element.header.guid = elemGuid;
				if (ACAPI_Element_GetHeader (&element.header) == NoError) {
					if (ACAPI_Element_Get (&element) == NoError) {
						API_Element mask {};
						ACAPI_ELEMENT_MASK_CLEAR (mask);
						ACAPI_ELEMENT_MASK_SET (mask, API_Elem_Head, layer);
						element.header.layer = targetLayerIndex;
						GSErrCode chgErr = ACAPI_Element_Change (&element, &mask, nullptr, 0, true);
						if (chgErr == NoError) {
							modifiedCount++;
						} else {
							lastErr = chgErr;
						}
					}
				}
			}
			return NoError;
		});

		res.Add ("success", true);
		res.Add ("modifiedCount", (Int32) modifiedCount);
		res.Add ("layerName", targetLayerName);
		res.Add ("layerIndex", targetLayerIndex.ToInt32_Deprecated ());
		if (modifiedCount == 0 && lastErr != NoError) {
			res.Add ("lastError", (Int32) lastErr);
		}
		return res;
	}
};

// =============================================================================
// 3. ManageHotlinks Command (list, break, delete, import)
// =============================================================================

class ManageHotlinksCommand : public BimKernelBaseCommand {
public:
	virtual GS::String GetName () const override
	{
		return "ManageHotlinks";
	}

	virtual GS::ObjectState Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const override
	{
		GS::ObjectState res;
		GS::UniString action = "list";
		parameters.Get ("action", action);

		if (action == "list") {
			GS::Array<API_Guid> nodeList;
			GSErrCode err = ACAPI_Hotlink_GetHotlinkNodes (nullptr, &nodeList);
			GS::Array<GS::ObjectState> nodesArray;
			if (err == NoError) {
				for (UIndex i = 0; i < nodeList.GetSize (); ++i) {
					API_HotlinkNode node {};
					node.guid = nodeList[i];
					if (ACAPI_Hotlink_GetHotlinkNode (&node) == NoError) {
						GS::ObjectState nState;
						nState.Add ("guid", APIGuidToString (node.guid));
						nState.Add ("name", GS::UniString (node.name));
						if (node.sourceLocation != nullptr) {
							nState.Add ("sourceLocation", node.sourceLocation->ToDisplayText ());
						}
						GS::Array<API_Guid> instances;
						ACAPI_Hotlink_GetHotlinkInstances (&node.guid, &instances);
						nState.Add ("instanceCount", (Int32) instances.GetSize ());
						nodesArray.Push (nState);
					}
				}
			}
			res.Add ("success", true);
			res.Add ("nodes", nodesArray);
			return res;
		}

		if (action == "break") {
			GS::UniString targetGuidStr;
			parameters.Get ("nodeGuid", targetGuidStr);
			GS::Array<API_Guid> toBreak;
			if (!targetGuidStr.IsEmpty ()) {
				toBreak.Push (APIGuidFromString (targetGuidStr.ToCStr ()));
			} else {
				ACAPI_Hotlink_GetHotlinkNodes (nullptr, &toBreak);
			}

			Int32 brokenCount = 0;
			ACAPI_CallUndoableCommand ("Break Hotlinks", [&] () -> GSErrCode {
				for (UIndex i = 0; i < toBreak.GetSize (); ++i) {
					if (ACAPI_Hotlink_BreakHotlinkNode (&(toBreak[i])) == NoError) {
						brokenCount++;
					}
				}
				return NoError;
			});
			res.Add ("success", true);
			res.Add ("brokenCount", brokenCount);
			return res;
		}

		if (action == "delete") {
			GS::UniString targetGuidStr;
			parameters.Get ("nodeGuid", targetGuidStr);
			GS::Array<API_Guid> toDelete;
			if (!targetGuidStr.IsEmpty ()) {
				toDelete.Push (APIGuidFromString (targetGuidStr.ToCStr ()));
			} else {
				ACAPI_Hotlink_GetHotlinkNodes (nullptr, &toDelete);
			}

			Int32 deletedCount = 0;
			ACAPI_CallUndoableCommand ("Delete Hotlinks", [&] () -> GSErrCode {
				for (UIndex i = 0; i < toDelete.GetSize (); ++i) {
					if (ACAPI_Hotlink_DeleteHotlinkNode (&(toDelete[i])) == NoError) {
						deletedCount++;
					}
				}
				return NoError;
			});
			res.Add ("success", true);
			res.Add ("deletedCount", deletedCount);
			return res;
		}

		if (action == "import") {
			GS::UniString filePath;
			if (!parameters.Get ("filePath", filePath) || filePath.IsEmpty ()) {
				res.Add ("success", false);
				res.Add ("error", "filePath parameter is required for action='import'");
				return res;
			}

			GS::UniString nodeName = "BimKernelHotlink";
			parameters.Get ("name", nodeName);
			GS::UniString layerName;
			parameters.Get ("layerName", layerName);
			double posX = 0.0, posY = 0.0, posZ = 0.0;
			parameters.Get ("x", posX);
			parameters.Get ("y", posY);
			parameters.Get ("z", posZ);
			short floorInd = 0;
			Int32 floorParam = 0;
			if (parameters.Get ("floorInd", floorParam)) {
				floorInd = (short) floorParam;
			}
			bool breakLink = false;
			parameters.Get ("breakLink", breakLink);
			bool singleStory = false;
			parameters.Get ("singleStory", singleStory);

			API_Guid createdNodeGuid = APINULLGuid;
			API_Guid createdElemGuid = APINULLGuid;
			GSErrCode importErr = NoError;

			ACAPI_CallUndoableCommand ("Import Hotlink Module", [&] () -> GSErrCode {
				API_HotlinkNode hotlinkNode {};
				hotlinkNode.type = APIHotlink_Module;
				hotlinkNode.storyRangeType = singleStory ? APIHotlink_SingleStory : APIHotlink_AllStories;
				hotlinkNode.sourceLocation = new IO::Location (filePath);
				GS::ucsncpy (hotlinkNode.name, nodeName.ToUStr (), API_UniLongNameLen - 1);
				hotlinkNode.refFloorInd = floorInd;

				ACAPI_Hotlink_GetHotlinkStoryInfo (&hotlinkNode);
				importErr = ACAPI_Hotlink_CreateHotlinkNode (&hotlinkNode);
				if (importErr != NoError) {
					return importErr;
				}
				createdNodeGuid = hotlinkNode.guid;

				API_Element element {};
				element.header.type = API_HotlinkID;
				element.header.floorInd = floorInd;
				element.header.layer = FindOrCreateLayerByName (layerName);
				if (element.header.layer == APIInvalidAttributeIndex) {
					element.header.layer = APIApplicationLayerAttributeIndex;
				}
				element.hotlink.type = APIHotlink_Module;
				element.hotlink.hotlinkNodeGuid = hotlinkNode.guid;
				element.hotlink.ignoreTopFloorLinks = true;

				for (Int32 j = 0; j < 12; ++j) {
					element.hotlink.transformation.tmx[j] = 0.0;
				}
				element.hotlink.transformation.tmx[0]  = 1.0;
				element.hotlink.transformation.tmx[5]  = 1.0;
				element.hotlink.transformation.tmx[10] = 1.0;
				element.hotlink.transformation.tmx[3]  = posX;
				element.hotlink.transformation.tmx[7]  = posY;
				element.hotlink.transformation.tmx[11] = posZ;

				importErr = ACAPI_Element_Create (&element, nullptr);
				if (importErr == NoError) {
					createdElemGuid = element.header.guid;
					if (breakLink) {
						ACAPI_Hotlink_BreakHotlinkNode (&createdNodeGuid);
					}
				}
				return importErr;
			});

			res.Add ("success", importErr == NoError);
			res.Add ("nodeGuid", APIGuidToString (createdNodeGuid));
			res.Add ("elementGuid", APIGuidToString (createdElemGuid));
			res.Add ("broken", breakLink);
			if (importErr != NoError) {
				res.Add ("errorCode", (Int32) importErr);
			}
			return res;
		}

		res.Add ("success", false);
		res.Add ("error", "Unsupported action: use 'list', 'break', 'delete', or 'import'");
		return res;
	}
};

// =============================================================================
// 4. DeleteElements Command
// =============================================================================

class DeleteElementsCommand : public BimKernelBaseCommand {
public:
	virtual GS::String GetName () const override
	{
		return "DeleteElements";
	}

	virtual GS::ObjectState Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const override
	{
		GS::ObjectState res;
		GS::Array<GS::UniString> elements;
		if (!parameters.Get ("elements", elements) || elements.IsEmpty ()) {
			res.Add ("success", false);
			res.Add ("error", "elements array parameter is required");
			return res;
		}

		GS::Array<API_Guid> guidsToDelete;
		for (UIndex i = 0; i < elements.GetSize (); ++i) {
			guidsToDelete.Push (APIGuidFromString (elements[i].ToCStr ()));
		}

		GSErrCode err = ACAPI_CallUndoableCommand ("Delete Elements", [&] () -> GSErrCode {
			return ACAPI_Element_Delete (guidsToDelete);
		});

		res.Add ("success", err == NoError);
		res.Add ("deletedCount", (Int32) guidsToDelete.GetSize ());
		if (err != NoError) {
			res.Add ("errorCode", (Int32) err);
		}
		return res;
	}
};

// =============================================================================
// 5. ListLayers Command
// =============================================================================

class ListLayersCommand : public BimKernelBaseCommand {
public:
	virtual GS::String GetName () const override
	{
		return "ListLayers";
	}

	virtual GS::ObjectState Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const override
	{
		GS::ObjectState res;
		UInt32 count = 0;
		GSErrCode err = ACAPI_Attribute_GetNum (API_LayerID, count);
		if (err != NoError) {
			res.Add ("success", false);
			res.Add ("error", "Failed to retrieve layer count");
			return res;
		}

		GS::Array<GS::ObjectState> layersArray;
		API_Attribute attr {};
		attr.header.typeID = API_LayerID;

		for (UInt32 i = 1; i <= count; ++i) {
			attr.header.index = ACAPI_CreateAttributeIndex ((Int32) i);
			if (ACAPI_Attribute_Get (&attr) == NoError) {
				GS::ObjectState lState;
				lState.Add ("index", attr.header.index.ToInt32_Deprecated ());
				lState.Add ("name", GS::UniString (attr.header.name));
				lState.Add ("guid", APIGuidToString (attr.header.guid));
				lState.Add ("locked", (attr.layer.head.flags & APILay_Locked) != 0);
				lState.Add ("hidden", (attr.layer.head.flags & APILay_Hidden) != 0);
				layersArray.Push (lState);
			}
		}

		res.Add ("success", true);
		res.Add ("layerCount", (Int32) layersArray.GetSize ());
		res.Add ("layers", layersArray);
		return res;
	}
};

// =============================================================================
// 6. ModifyLayersState Command (lock/unlock, show/hide, delete layers)
// =============================================================================

class ModifyLayersStateCommand : public BimKernelBaseCommand {
public:
	virtual GS::String GetName () const override
	{
		return "ModifyLayersState";
	}

	virtual GS::ObjectState Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const override
	{
		GS::ObjectState res;
		GS::Array<GS::UniString> layerNames;
		parameters.Get ("layers", layerNames);

		bool hasLocked = parameters.Contains ("locked");
		bool lockedVal = false;
		if (hasLocked) {
			parameters.Get ("locked", lockedVal);
		}

		bool hasHidden = parameters.Contains ("hidden");
		bool hiddenVal = false;
		if (hasHidden) {
			parameters.Get ("hidden", hiddenVal);
		}

		bool deleteLayer = false;
		parameters.Get ("deleteLayer", deleteLayer);

		Int32 modifiedCount = 0;
		ACAPI_CallUndoableCommand ("Modify Layers State", [&] () -> GSErrCode {
			for (UIndex i = 0; i < layerNames.GetSize (); ++i) {
				API_Attr_Head attrHead {};
				attrHead.typeID = API_LayerID;
				GS::UniString searchName = layerNames[i];
				attrHead.uniStringNamePtr = &searchName;
				if (ACAPI_Attribute_Search (&attrHead) == NoError && attrHead.index != APIApplicationLayerAttributeIndex) {
					if (deleteLayer) {
						if (ACAPI_Attribute_Delete (attrHead) == NoError) {
							modifiedCount++;
						}
					} else {
						API_Attribute attr {};
						attr.header.typeID = API_LayerID;
						attr.header.index = attrHead.index;
						if (ACAPI_Attribute_Get (&attr) == NoError) {
							if (hasLocked) {
								if (lockedVal) attr.layer.head.flags |= APILay_Locked;
								else attr.layer.head.flags &= ~APILay_Locked;
							}
							if (hasHidden) {
								if (hiddenVal) attr.layer.head.flags |= APILay_Hidden;
								else attr.layer.head.flags &= ~APILay_Hidden;
							}
							if (ACAPI_Attribute_Modify (&attr, nullptr) == NoError) {
								modifiedCount++;
							}
						}
					}
				}
			}
			return NoError;
		});

		res.Add ("success", true);
		res.Add ("modifiedCount", modifiedCount);
		return res;
	}
};

// =============================================================================
// 7. PublishSet Command
// =============================================================================

class PublishSetCommand : public BimKernelBaseCommand {
public:
	virtual GS::String GetName () const override
	{
		return "PublishSet";
	}

	virtual GS::ObjectState Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const override
	{
		GS::ObjectState res;
		GS::UniString targetSetName;
		parameters.Get ("setName", targetSetName);
		GS::UniString outPathStr;
		parameters.Get ("outputPath", outPathStr);

		Int32 numOfSets = 0;
		GSErrCode err = ACAPI_Navigator_GetNavigatorSetNum (&numOfSets);
		if (err != NoError || numOfSets <= 0) {
			res.Add ("success", false);
			res.Add ("error", "No navigator sets found in project");
			return res;
		}

		API_Guid publisherSetGuid = APINULLGuid;
		GS::UniString foundSetName;

		for (Int32 i = 0; i < numOfSets; ++i) {
			API_NavigatorSet set {};
			set.mapId = API_PublisherSets;
			if (ACAPI_Navigator_GetNavigatorSet (&set, &i) == NoError) {
				GS::UniString sName (set.name);
				if (targetSetName.IsEmpty () || sName.Contains (targetSetName)) {
					publisherSetGuid = set.rootGuid;
					foundSetName = sName;
					break;
				}
			}
		}

		if (publisherSetGuid == APINULLGuid) {
			res.Add ("success", false);
			res.Add ("error", "Requested publisher set was not found");
			return res;
		}

		API_PublishPars publishPars {};
		publishPars.guid = publisherSetGuid;
		IO::Location* customLoc = nullptr;
		if (!outPathStr.IsEmpty ()) {
			customLoc = new IO::Location (outPathStr);
			publishPars.path = customLoc;
		}

		err = ACAPI_ProjectOperation_Publish (&publishPars);
		if (customLoc != nullptr) {
			delete customLoc;
		}

		res.Add ("success", err == NoError);
		res.Add ("publisherSet", foundSetName);
		if (err != NoError) {
			res.Add ("errorCode", (Int32) err);
		}
		return res;
	}
};

// =============================================================================
// 8. ListLayoutDatabases Command
// =============================================================================

class ListLayoutDatabasesCommand : public BimKernelBaseCommand {
public:
	virtual GS::String GetName () const override
	{
		return "ListLayoutDatabases";
	}

	virtual GS::ObjectState Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const override
	{
		GS::ObjectState res;
		GS::Array<API_DatabaseUnId> layoutDbs;
		GSErrCode err = ACAPI_Database_GetLayoutDatabases (nullptr, &layoutDbs);
		if (err != NoError) {
			res.Add ("success", false);
			res.Add ("error", "Failed to retrieve layout databases");
			return res;
		}

		GS::Array<GS::ObjectState> layoutsArray;
		for (UIndex i = 0; i < layoutDbs.GetSize (); ++i) {
			API_DatabaseInfo dbInfo {};
			dbInfo.databaseUnId = layoutDbs[i];
			dbInfo.typeID = APIWind_LayoutID;
			if (ACAPI_Window_GetDatabaseInfo (&dbInfo) == NoError) {
				GS::ObjectState lState;
				lState.Add ("guid", APIGuidToString (dbInfo.databaseUnId.elemSetId));
				lState.Add ("name", GS::UniString (dbInfo.name));
				lState.Add ("title", GS::UniString (dbInfo.title));
				lState.Add ("ref", GS::UniString (dbInfo.ref));
				layoutsArray.Push (lState);
			}
		}

		res.Add ("success", true);
		res.Add ("count", (Int32) layoutsArray.GetSize ());
		res.Add ("layouts", layoutsArray);
		return res;
	}
};

// =============================================================================
// 9. PlaceDrawingOnLayout Command (single & batch layout sheet automation)
// =============================================================================

class PlaceDrawingOnLayoutCommand : public BimKernelBaseCommand {
public:
	virtual GS::String GetName () const override
	{
		return "PlaceDrawingOnLayout";
	}

	virtual GS::ObjectState Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const override
	{
		GS::ObjectState res;

		struct PlacementSpec {
			API_Guid layoutGuid;
			API_Guid viewGuid;
			double posX;
			double posY;
			double ratio;
		};

		GS::Array<PlacementSpec> specs;
		GS::Array<GS::ObjectState> placementsParam;
		if (parameters.Get ("placements", placementsParam) && !placementsParam.IsEmpty ()) {
			for (UIndex i = 0; i < placementsParam.GetSize (); ++i) {
				GS::UniString lStr, vStr;
				placementsParam[i].Get ("layoutGuid", lStr);
				placementsParam[i].Get ("viewGuid", vStr);
				double px = 0.21, py = 0.1485, rat = 1.0;
				placementsParam[i].Get ("posX", px);
				placementsParam[i].Get ("posY", py);
				placementsParam[i].Get ("ratio", rat);
				if (!lStr.IsEmpty () && !vStr.IsEmpty ()) {
					specs.Push ({ APIGuidFromString (lStr.ToCStr ()), APIGuidFromString (vStr.ToCStr ()), px, py, rat });
				}
			}
		} else {
			GS::UniString lStr, vStr;
			parameters.Get ("layoutGuid", lStr);
			parameters.Get ("viewGuid", vStr);
			double px = 0.21, py = 0.1485, rat = 1.0;
			parameters.Get ("posX", px);
			parameters.Get ("posY", py);
			parameters.Get ("ratio", rat);
			if (!lStr.IsEmpty () && !vStr.IsEmpty ()) {
				specs.Push ({ APIGuidFromString (lStr.ToCStr ()), APIGuidFromString (vStr.ToCStr ()), px, py, rat });
			}
		}

		if (specs.IsEmpty ()) {
			res.Add ("success", false);
			res.Add ("error", "Provide layoutGuid and viewGuid, or placements array");
			return res;
		}

		API_DatabaseInfo origDb {};
		ACAPI_Database_GetCurrentDatabase (&origDb);

		GS::Array<GS::UniString> createdDrawings;
		GSErrCode lastErr = NoError;

		ACAPI_CallUndoableCommand ("Place Drawings On Layout", [&] () -> GSErrCode {
			for (UIndex i = 0; i < specs.GetSize (); ++i) {
				API_DatabaseInfo targetDb {};
				targetDb.typeID = APIWind_LayoutID;
				targetDb.databaseUnId.elemSetId = specs[i].layoutGuid;

				GSErrCode dbErr = ACAPI_Database_ChangeCurrentDatabase (&targetDb);
				if (dbErr != NoError) {
					lastErr = dbErr;
					continue;
				}

				API_Element element {};
				element.header.type = API_DrawingID;
				if (ACAPI_Element_GetDefaults (&element, nullptr) == NoError) {
					element.drawing.drawingGuid = specs[i].viewGuid;
					element.drawing.pos.x = specs[i].posX;
					element.drawing.pos.y = specs[i].posY;
					element.drawing.ratio = specs[i].ratio;
					element.drawing.anchorPoint = APIAnc_MM;
					element.drawing.isCutWithFrame = false;

					GSErrCode crErr = ACAPI_Element_Create (&element, nullptr);
					if (crErr == NoError) {
						createdDrawings.Push (APIGuidToString (element.header.guid));
					} else {
						lastErr = crErr;
					}
				}
			}

			if (origDb.typeID != API_ZombieWindowID) {
				ACAPI_Database_ChangeCurrentDatabase (&origDb);
			}
			return NoError;
		});

		res.Add ("success", !createdDrawings.IsEmpty ());
		res.Add ("placedCount", (Int32) createdDrawings.GetSize ());
		res.Add ("drawingGuids", createdDrawings);
		if (lastErr != NoError) {
			res.Add ("lastError", (Int32) lastErr);
		}
		return res;
	}
};

// =============================================================================
// 10. SetFloorPlanCutPlane Command
// =============================================================================

class SetFloorPlanCutPlaneCommand : public BimKernelBaseCommand {
public:
	virtual GS::String GetName () const override
	{
		return "SetFloorPlanCutPlane";
	}

	virtual GS::ObjectState Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const override
	{
		GS::ObjectState res;
		double cutLevel = 3.20;
		parameters.Get ("cutLevel", cutLevel);

		API_FloorPlanCutDefinition cutDef {};
		GSErrCode err = ACAPI_ProjectSetting_GetPreferences (&cutDef, APIPrefs_FloorPlanCutPlaneDef);
		if (err == NoError) {
			cutDef.currCutLevel = cutLevel;
			if (parameters.Contains ("topCutLevel")) {
				parameters.Get ("topCutLevel", cutDef.topCutLevel);
			}
			if (parameters.Contains ("bottomCutLevel")) {
				parameters.Get ("bottomCutLevel", cutDef.bottomCutLevel);
			}
			err = ACAPI_ProjectSetting_SetPreferences (&cutDef, APIPrefs_FloorPlanCutPlaneDef);
		}

		res.Add ("success", err == NoError);
		res.Add ("cutLevel", cutLevel);
		if (err != NoError) {
			res.Add ("errorCode", (Int32) err);
		}
		return res;
	}
};

// =============================================================================
// 11. SolidElementOperations Command (SEO Boolean Link / Unlink)
// =============================================================================

class SolidElementOperationsCommand : public BimKernelBaseCommand {
public:
	virtual GS::String GetName () const override
	{
		return "SolidElementOperations";
	}

	virtual GS::ObjectState Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const override
	{
		GS::ObjectState res;
		GS::UniString action = "link";
		parameters.Get ("action", action);

		GS::Array<GS::UniString> targets;
		GS::Array<GS::UniString> operators;
		parameters.Get ("targets", targets);
		parameters.Get ("operators", operators);

		if (targets.IsEmpty () || operators.IsEmpty ()) {
			res.Add ("success", false);
			res.Add ("error", "targets and operators GUID arrays are required");
			return res;
		}

		GS::UniString opStr = "substract_upward";
		parameters.Get ("operation", opStr);

		API_SolidOperationID solidOp = APISolid_SubstUp;
		if (opStr == "substract") solidOp = APISolid_Substract;
		else if (opStr == "substract_upward") solidOp = APISolid_SubstUp;
		else if (opStr == "substract_downward") solidOp = APISolid_SubstDown;
		else if (opStr == "intersect") solidOp = APISolid_Intersect;
		else if (opStr == "add") solidOp = APISolid_Add;

		Int32 linkedCount = 0;
		GSErrCode lastErr = NoError;

		ACAPI_CallUndoableCommand ("Solid Element Operations", [&] () -> GSErrCode {
			for (UIndex t = 0; t < targets.GetSize (); ++t) {
				API_Guid targetGuid = APIGuidFromString (targets[t].ToCStr ());
				for (UIndex o = 0; o < operators.GetSize (); ++o) {
					API_Guid operatorGuid = APIGuidFromString (operators[o].ToCStr ());
					GSErrCode seoErr = NoError;
					if (action == "unlink") {
						seoErr = ACAPI_Element_SolidLink_Remove (targetGuid, operatorGuid);
					} else {
						seoErr = ACAPI_Element_SolidLink_Create (targetGuid, operatorGuid, solidOp, APISolidFlag_OperatorAttrib);
					}
					if (seoErr == NoError) {
						linkedCount++;
					} else {
						lastErr = seoErr;
					}
				}
			}
			return NoError;
		});

		res.Add ("success", linkedCount > 0);
		res.Add ("linkedCount", linkedCount);
		res.Add ("operation", opStr);
		if (lastErr != NoError) {
			res.Add ("lastError", (Int32) lastErr);
		}
		return res;
	}
};

// =============================================================================
// 12. ModifyElementsGeometry Command (Transform / Edit / Set Coordinates & ID)
// =============================================================================

class ModifyElementsGeometryCommand : public BimKernelBaseCommand {
public:
	virtual GS::String GetName () const override
	{
		return "ModifyElementsGeometry";
	}

	virtual GS::ObjectState Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const override
	{
		GS::ObjectState res;

		// Option A: Batch transform via ACAPI_Element_Edit (drag, elevate, rotate)
		GS::UniString editAction;
		if (parameters.Get ("editAction", editAction) && !editAction.IsEmpty ()) {
			GS::Array<GS::UniString> elemGuids;
			parameters.Get ("elements", elemGuids);
			if (elemGuids.IsEmpty ()) {
				res.Add ("success", false);
				res.Add ("error", "elements array is required for editAction");
				return res;
			}

			GS::Array<API_Neig> items;
			for (UIndex i = 0; i < elemGuids.GetSize (); ++i) {
				items.Push (API_Neig (APIGuidFromString (elemGuids[i].ToCStr ())));
			}

			double dx = 0.0, dy = 0.0, dz = 0.0;
			parameters.Get ("dx", dx);
			parameters.Get ("dy", dy);
			parameters.Get ("dz", dz);
			bool copyElements = false;
			parameters.Get ("copy", copyElements);

			API_EditPars editPars {};
			editPars.withDelete = !copyElements;
			editPars.begC = { 0.0, 0.0, 0.0 };
			editPars.endC = { dx, dy, dz };

			if (editAction == "elevate") {
				editPars.typeID = APIEdit_Elevate;
			} else if (editAction == "rotate") {
				editPars.typeID = APIEdit_Rotate;
				double ox = 0.0, oy = 0.0, angleDeg = 90.0;
				parameters.Get ("originX", ox);
				parameters.Get ("originY", oy);
				parameters.Get ("angleDeg", angleDeg);
				double rad = angleDeg * 3.14159265358979323846 / 180.0;
				editPars.origC = { ox, oy };
				editPars.begC = { ox + 1.0, oy, 0.0 };
				editPars.endC = { ox + std::cos (rad), oy + std::sin (rad), dz };
			} else {
				editPars.typeID = APIEdit_Drag;
			}

			GSErrCode editErr = ACAPI_CallUndoableCommand ("Transform Elements", [&] () -> GSErrCode {
				return ACAPI_Element_Edit (&items, editPars);
			});

			res.Add ("success", editErr == NoError);
			res.Add ("editedCount", (Int32) items.GetSize ());
			if (editErr != NoError) {
				res.Add ("errorCode", (Int32) editErr);
			}
			return res;
		}

		// Option B: Per-element direct parametric modification
		GS::Array<GS::ObjectState> modifications;
		if (!parameters.Get ("modifications", modifications) || modifications.IsEmpty ()) {
			res.Add ("success", false);
			res.Add ("error", "Provide either editAction or modifications array");
			return res;
		}

		Int32 modifiedCount = 0;
		GSErrCode lastErr = NoError;

		ACAPI_CallUndoableCommand ("Modify Elements Geometry", [&] () -> GSErrCode {
			for (UIndex i = 0; i < modifications.GetSize (); ++i) {
				const GS::ObjectState& mod = modifications[i];
				GS::UniString guidStr;
				if (!mod.Get ("guid", guidStr) || guidStr.IsEmpty ()) {
					continue;
				}
				API_Guid elemGuid = APIGuidFromString (guidStr.ToCStr ());

				// Optional Element ID rename
				GS::UniString newElemId;
				if (mod.Get ("elemId", newElemId) && !newElemId.IsEmpty ()) {
					ACAPI_Element_ChangeElementInfoString (&elemGuid, &newElemId);
				}

				API_Element element {};
				element.header.guid = elemGuid;
				if (ACAPI_Element_Get (&element) != NoError) {
					continue;
				}

				API_Element mask {};
				ACAPI_ELEMENT_MASK_CLEAR (mask);
				bool hasChanges = false;

				Int32 floorInd = 0;
				if (mod.Get ("floorInd", floorInd)) {
					element.header.floorInd = (short) floorInd;
					ACAPI_ELEMENT_MASK_SET (mask, API_Elem_Head, floorInd);
					hasChanges = true;
				}

				GS::UniString layerName;
				if (mod.Get ("layerName", layerName) && !layerName.IsEmpty ()) {
					API_AttributeIndex lIdx = FindOrCreateLayerByName (layerName);
					if (lIdx != APIInvalidAttributeIndex) {
						element.header.layer = lIdx;
						ACAPI_ELEMENT_MASK_SET (mask, API_Elem_Head, layer);
						hasChanges = true;
					}
				}

				if (element.header.type == API_WallID) {
					if (mod.Contains ("begX") && mod.Contains ("begY")) {
						mod.Get ("begX", element.wall.begC.x);
						mod.Get ("begY", element.wall.begC.y);
						ACAPI_ELEMENT_MASK_SET (mask, API_WallType, begC);
						hasChanges = true;
					}
					if (mod.Contains ("endX") && mod.Contains ("endY")) {
						mod.Get ("endX", element.wall.endC.x);
						mod.Get ("endY", element.wall.endC.y);
						ACAPI_ELEMENT_MASK_SET (mask, API_WallType, endC);
						hasChanges = true;
					}
					if (mod.Contains ("height")) {
						mod.Get ("height", element.wall.height);
						element.wall.relativeTopStory = 0;
						ACAPI_ELEMENT_MASK_SET (mask, API_WallType, height);
						ACAPI_ELEMENT_MASK_SET (mask, API_WallType, relativeTopStory);
						hasChanges = true;
					}
					if (mod.Contains ("bottomOffset")) {
						mod.Get ("bottomOffset", element.wall.bottomOffset);
						ACAPI_ELEMENT_MASK_SET (mask, API_WallType, bottomOffset);
						hasChanges = true;
					}
					if (mod.Contains ("thickness")) {
						mod.Get ("thickness", element.wall.thickness);
						ACAPI_ELEMENT_MASK_SET (mask, API_WallType, thickness);
						hasChanges = true;
					}
					GS::UniString bmatStr;
					if (mod.Get ("buildingMaterial", bmatStr) && !bmatStr.IsEmpty ()) {
						API_AttributeIndex bmIdx = FindBuildingMaterial (bmatStr);
						if (bmIdx != APIInvalidAttributeIndex) {
							element.wall.modelElemStructureType = API_BasicStructure;
							element.wall.buildingMaterial = bmIdx;
							ACAPI_ELEMENT_MASK_SET (mask, API_WallType, modelElemStructureType);
							ACAPI_ELEMENT_MASK_SET (mask, API_WallType, buildingMaterial);
							hasChanges = true;
						}
					}
					GS::UniString compStr;
					if (mod.Get ("composite", compStr) && !compStr.IsEmpty ()) {
						API_AttributeIndex cIdx = FindComposite (compStr);
						if (cIdx != APIInvalidAttributeIndex) {
							element.wall.modelElemStructureType = API_CompositeStructure;
							element.wall.composite = cIdx;
							ACAPI_ELEMENT_MASK_SET (mask, API_WallType, modelElemStructureType);
							ACAPI_ELEMENT_MASK_SET (mask, API_WallType, composite);
							hasChanges = true;
						}
					}
					Int32 penVal = 0;
					if (mod.Get ("contPen", penVal)) {
						element.wall.contPen = (short) penVal;
						ACAPI_ELEMENT_MASK_SET (mask, API_WallType, contPen);
						hasChanges = true;
					}
					if (mod.Get ("contPen3D", penVal)) {
						element.wall.contPen3D = (short) penVal;
						ACAPI_ELEMENT_MASK_SET (mask, API_WallType, contPen3D);
						hasChanges = true;
					}
				} else if (element.header.type == API_ColumnID) {
					if (mod.Contains ("posX") && mod.Contains ("posY")) {
						mod.Get ("posX", element.column.origoPos.x);
						mod.Get ("posY", element.column.origoPos.y);
						ACAPI_ELEMENT_MASK_SET (mask, API_ColumnType, origoPos);
						hasChanges = true;
					}
					if (mod.Contains ("bottomOffset")) {
						mod.Get ("bottomOffset", element.column.bottomOffset);
						ACAPI_ELEMENT_MASK_SET (mask, API_ColumnType, bottomOffset);
						hasChanges = true;
					}
					if (mod.Contains ("height")) {
						mod.Get ("height", element.column.height);
						element.column.relativeTopStory = 0;
						ACAPI_ELEMENT_MASK_SET (mask, API_ColumnType, height);
						ACAPI_ELEMENT_MASK_SET (mask, API_ColumnType, relativeTopStory);
						hasChanges = true;
					}
				} else if (element.header.type == API_BeamID) {
					if (mod.Contains ("begX") && mod.Contains ("begY")) {
						mod.Get ("begX", element.beam.begC.x);
						mod.Get ("begY", element.beam.begC.y);
						ACAPI_ELEMENT_MASK_SET (mask, API_BeamType, begC);
						hasChanges = true;
					}
					if (mod.Contains ("endX") && mod.Contains ("endY")) {
						mod.Get ("endX", element.beam.endC.x);
						mod.Get ("endY", element.beam.endC.y);
						ACAPI_ELEMENT_MASK_SET (mask, API_BeamType, endC);
						hasChanges = true;
					}
					if (mod.Contains ("level")) {
						mod.Get ("level", element.beam.level);
						ACAPI_ELEMENT_MASK_SET (mask, API_BeamType, level);
						hasChanges = true;
					}
					if (mod.Contains ("offset")) {
						mod.Get ("offset", element.beam.offset);
						ACAPI_ELEMENT_MASK_SET (mask, API_BeamType, offset);
						hasChanges = true;
					}
				} else if (element.header.type == API_SlabID) {
					if (mod.Contains ("level")) {
						mod.Get ("level", element.slab.level);
						ACAPI_ELEMENT_MASK_SET (mask, API_SlabType, level);
						hasChanges = true;
					}
					if (mod.Contains ("thickness")) {
						mod.Get ("thickness", element.slab.thickness);
						ACAPI_ELEMENT_MASK_SET (mask, API_SlabType, thickness);
						hasChanges = true;
					}
					GS::UniString bmatStr;
					if (mod.Get ("buildingMaterial", bmatStr) && !bmatStr.IsEmpty ()) {
						API_AttributeIndex bmIdx = FindBuildingMaterial (bmatStr);
						if (bmIdx != APIInvalidAttributeIndex) {
							element.slab.modelElemStructureType = API_BasicStructure;
							element.slab.buildingMaterial = bmIdx;
							ACAPI_ELEMENT_MASK_SET (mask, API_SlabType, modelElemStructureType);
							ACAPI_ELEMENT_MASK_SET (mask, API_SlabType, buildingMaterial);
							hasChanges = true;
						}
					}
					GS::UniString compStr;
					if (mod.Get ("composite", compStr) && !compStr.IsEmpty ()) {
						API_AttributeIndex cIdx = FindComposite (compStr);
						if (cIdx != APIInvalidAttributeIndex) {
							element.slab.modelElemStructureType = API_CompositeStructure;
							element.slab.composite = cIdx;
							ACAPI_ELEMENT_MASK_SET (mask, API_WallType, modelElemStructureType);
							ACAPI_ELEMENT_MASK_SET (mask, API_SlabType, composite);
							hasChanges = true;
						}
					}
					Int32 penVal = 0;
					if (mod.Get ("pen", penVal)) {
						element.slab.pen = (short) penVal;
						ACAPI_ELEMENT_MASK_SET (mask, API_SlabType, pen);
						hasChanges = true;
					}
				} else if (element.header.type == API_ObjectID) {
					if (mod.Contains ("posX") && mod.Contains ("posY")) {
						mod.Get ("posX", element.object.pos.x);
						mod.Get ("posY", element.object.pos.y);
						ACAPI_ELEMENT_MASK_SET (mask, API_ObjectType, pos);
						hasChanges = true;
					}
					if (mod.Contains ("level")) {
						mod.Get ("level", element.object.level);
						ACAPI_ELEMENT_MASK_SET (mask, API_ObjectType, level);
						hasChanges = true;
					}
					if (mod.Contains ("angle")) {
						mod.Get ("angle", element.object.angle);
						ACAPI_ELEMENT_MASK_SET (mask, API_ObjectType, angle);
						hasChanges = true;
					}
					if (mod.Contains ("xRatio")) {
						mod.Get ("xRatio", element.object.xRatio);
						ACAPI_ELEMENT_MASK_SET (mask, API_ObjectType, xRatio);
						hasChanges = true;
					}
					if (mod.Contains ("yRatio")) {
						mod.Get ("yRatio", element.object.yRatio);
						ACAPI_ELEMENT_MASK_SET (mask, API_ObjectType, yRatio);
						hasChanges = true;
					}
					bool useFix = false;
					if (mod.Get ("useXYFixSize", useFix)) {
						element.object.useXYFixSize = useFix;
						ACAPI_ELEMENT_MASK_SET (mask, API_ObjectType, useXYFixSize);
						hasChanges = true;
					}
				} else if (element.header.type == API_DoorID) {
					if (mod.Contains ("objLoc")) {
						mod.Get ("objLoc", element.door.objLoc);
						ACAPI_ELEMENT_MASK_SET (mask, API_DoorType, objLoc);
						hasChanges = true;
					}
					if (mod.Contains ("lower")) {
						mod.Get ("lower", element.door.lower);
						ACAPI_ELEMENT_MASK_SET (mask, API_DoorType, lower);
						hasChanges = true;
					}
					if (mod.Contains ("width")) {
						mod.Get ("width", element.door.openingBase.width);
						ACAPI_ELEMENT_MASK_SET (mask, API_DoorType, openingBase.width);
						hasChanges = true;
					}
					if (mod.Contains ("height")) {
						mod.Get ("height", element.door.openingBase.height);
						ACAPI_ELEMENT_MASK_SET (mask, API_DoorType, openingBase.height);
						hasChanges = true;
					}
					if (mod.Contains ("revealDepthOffset")) {
						mod.Get ("revealDepthOffset", element.door.revealDepthOffset);
						ACAPI_ELEMENT_MASK_SET (mask, API_DoorType, revealDepthOffset);
						hasChanges = true;
					}
				} else if (element.header.type == API_WindowID) {
					if (mod.Contains ("objLoc")) {
						mod.Get ("objLoc", element.window.objLoc);
						ACAPI_ELEMENT_MASK_SET (mask, API_WindowType, objLoc);
						hasChanges = true;
					}
					if (mod.Contains ("lower")) {
						mod.Get ("lower", element.window.lower);
						ACAPI_ELEMENT_MASK_SET (mask, API_WindowType, lower);
						hasChanges = true;
					}
					if (mod.Contains ("width")) {
						mod.Get ("width", element.window.openingBase.width);
						ACAPI_ELEMENT_MASK_SET (mask, API_WindowType, openingBase.width);
						hasChanges = true;
					}
					if (mod.Contains ("height")) {
						mod.Get ("height", element.window.openingBase.height);
						ACAPI_ELEMENT_MASK_SET (mask, API_WindowType, openingBase.height);
						hasChanges = true;
					}
					if (mod.Contains ("revealDepthOffset")) {
						mod.Get ("revealDepthOffset", element.window.revealDepthOffset);
						ACAPI_ELEMENT_MASK_SET (mask, API_WindowType, revealDepthOffset);
						hasChanges = true;
					}
				}

				if (hasChanges) {
					GSErrCode chgErr = ACAPI_Element_Change (&element, &mask, nullptr, 0, true);
					if (chgErr == NoError) {
						modifiedCount++;
					} else {
						lastErr = chgErr;
					}
				} else if (!newElemId.IsEmpty ()) {
					modifiedCount++;
				}
			}
			return NoError;
		});

		res.Add ("success", modifiedCount > 0);
		res.Add ("modifiedCount", modifiedCount);
		if (lastErr != NoError) {
			res.Add ("lastError", (Int32) lastErr);
		}
		return res;
	}
};

// =============================================================================
// 13. CreateNativeElements Command (Wall, Column, Beam, Door, Window, Object, Slab, Mesh, Zone)
// =============================================================================

static bool PopulatePolygonInMemo (const GS::Array<GS::ObjectState>& polyNodes, API_Polygon& outPoly, API_ElementMemo& memo)
{
	if (polyNodes.GetSize () < 3) {
		return false;
	}

	GS::Array<API_Coord> pts;
	for (UIndex p = 0; p < polyNodes.GetSize (); ++p) {
		API_Coord c { 0.0, 0.0 };
		polyNodes[p].Get ("x", c.x);
		polyNodes[p].Get ("y", c.y);
		pts.Push (c);
	}
	// Ensure closed polygon (duplicate first vertex at the end if not already closed)
	if (std::fabs (pts[0].x - pts[pts.GetSize () - 1].x) > 1e-6 ||
		std::fabs (pts[0].y - pts[pts.GetSize () - 1].y) > 1e-6) {
		pts.Push (pts[0]);
	}

	Int32 nCoords = (Int32) pts.GetSize ();
	outPoly.nCoords = nCoords;
	outPoly.nSubPolys = 1;
	outPoly.nArcs = 0;

	if (memo.coords != nullptr) {
		BMKillHandle ((GSHandle*) &memo.coords);
	}
	if (memo.pends != nullptr) {
		BMKillHandle ((GSHandle*) &memo.pends);
	}
	if (memo.parcs != nullptr) {
		BMKillHandle ((GSHandle*) &memo.parcs);
	}
	if (memo.vertexIDs != nullptr) {
		BMKillHandle ((GSHandle*) &memo.vertexIDs);
	}
	if (memo.edgeIDs != nullptr) {
		BMKillHandle ((GSHandle*) &memo.edgeIDs);
	}
	if (memo.contourIDs != nullptr) {
		BMKillHandle ((GSHandle*) &memo.contourIDs);
	}

	memo.coords = (API_Coord**) BMAllocateHandle ((nCoords + 1) * sizeof (API_Coord), ALLOCATE_CLEAR, 0);
	memo.pends  = (Int32**) BMAllocateHandle (2 * sizeof (Int32), ALLOCATE_CLEAR, 0);

	if (memo.coords == nullptr || memo.pends == nullptr) {
		return false;
	}

	(*memo.coords)[0] = { 0.0, 0.0 };
	for (Int32 k = 0; k < nCoords; ++k) {
		(*memo.coords)[k + 1] = pts[k];
	}
	(*memo.pends)[0] = 0;
	(*memo.pends)[1] = nCoords;

	return true;
}

class CreateNativeElementsCommand : public BimKernelBaseCommand {
public:
	virtual GS::String GetName () const override
	{
		return "CreateNativeElements";
	}

	virtual GS::ObjectState Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const override
	{
		GS::ObjectState res;
		GS::Array<GS::ObjectState> items;
		if (!parameters.Get ("elements", items) || items.IsEmpty ()) {
			res.Add ("success", false);
			res.Add ("error", "elements array parameter is required");
			return res;
		}

		GS::Array<GS::UniString> createdGuids;
		GSErrCode lastErr = NoError;

		ACAPI_CallUndoableCommand ("Create Native Elements", [&] () -> GSErrCode {
			for (UIndex i = 0; i < items.GetSize (); ++i) {
				const GS::ObjectState& spec = items[i];
				GS::UniString typeStr = "Wall";
				spec.Get ("type", typeStr);

				API_Element element {};
				API_ElementMemo memo {};
				bool hasMemo = false;

				if (typeStr == "Wall") element.header.type = API_WallID;
				else if (typeStr == "Column") element.header.type = API_ColumnID;
				else if (typeStr == "Beam") element.header.type = API_BeamID;
				else if (typeStr == "Door") element.header.type = API_DoorID;
				else if (typeStr == "Window") element.header.type = API_WindowID;
				else if (typeStr == "Object") element.header.type = API_ObjectID;
				else if (typeStr == "Slab") element.header.type = API_SlabID;
				else if (typeStr == "Mesh") element.header.type = API_MeshID;
				else if (typeStr == "Zone") element.header.type = API_ZoneID;
				else continue;

				if (ACAPI_Element_GetDefaults (&element, &memo) != NoError) {
					ACAPI_DisposeElemMemoHdls (&memo);
					continue;
				}
				hasMemo = true;

				Int32 floorInd = 0;
				if (spec.Get ("floorInd", floorInd)) {
					element.header.floorInd = (short) floorInd;
				}

				GS::UniString layerName;
				if (spec.Get ("layerName", layerName) && !layerName.IsEmpty ()) {
					API_AttributeIndex lIdx = FindOrCreateLayerByName (layerName);
					if (lIdx != APIInvalidAttributeIndex) {
						element.header.layer = lIdx;
					}
				}

				if (typeStr == "Wall") {
					spec.Get ("begX", element.wall.begC.x);
					spec.Get ("begY", element.wall.begC.y);
					spec.Get ("endX", element.wall.endC.x);
					spec.Get ("endY", element.wall.endC.y);
					if (spec.Contains ("height")) {
						spec.Get ("height", element.wall.height);
						element.wall.relativeTopStory = 0;
					}
					if (spec.Contains ("bottomOffset")) {
						spec.Get ("bottomOffset", element.wall.bottomOffset);
					}
					if (spec.Contains ("thickness")) {
						spec.Get ("thickness", element.wall.thickness);
						element.wall.modelElemStructureType = API_BasicStructure;
					}
					GS::UniString bmatStr;
					if (spec.Get ("buildingMaterial", bmatStr) && !bmatStr.IsEmpty ()) {
						API_AttributeIndex bmIdx = FindBuildingMaterial (bmatStr);
						if (bmIdx != APIInvalidAttributeIndex) {
							element.wall.modelElemStructureType = API_BasicStructure;
							element.wall.buildingMaterial = bmIdx;
						}
					}
					GS::UniString compStr;
					if (spec.Get ("composite", compStr) && !compStr.IsEmpty ()) {
						API_AttributeIndex cIdx = FindComposite (compStr);
						if (cIdx != APIInvalidAttributeIndex) {
							element.wall.modelElemStructureType = API_CompositeStructure;
							element.wall.composite = cIdx;
						}
					}
				} else if (typeStr == "Column") {
					spec.Get ("posX", element.column.origoPos.x);
					spec.Get ("posY", element.column.origoPos.y);
					if (spec.Contains ("bottomOffset")) {
						spec.Get ("bottomOffset", element.column.bottomOffset);
					}
					if (spec.Contains ("height")) {
						spec.Get ("height", element.column.height);
						element.column.relativeTopStory = 0;
					}
				} else if (typeStr == "Beam") {
					spec.Get ("begX", element.beam.begC.x);
					spec.Get ("begY", element.beam.begC.y);
					spec.Get ("endX", element.beam.endC.x);
					spec.Get ("endY", element.beam.endC.y);
					if (spec.Contains ("level")) {
						spec.Get ("level", element.beam.level);
					}
				} else if (typeStr == "Door" || typeStr == "Window") {
					GS::UniString wallGuidStr;
					spec.Get ("wallGuid", wallGuidStr);
					double objLoc = 0.5, lower = 0.0, width = 0.90, height = 2.10;
					spec.Get ("objLoc", objLoc);
					spec.Get ("lower", lower);
					spec.Get ("width", width);
					spec.Get ("height", height);
					if (typeStr == "Door") {
						element.door.owner = APIGuidFromString (wallGuidStr.ToCStr ());
						element.door.objLoc = objLoc;
						element.door.lower = lower;
						element.door.openingBase.width = width;
						element.door.openingBase.height = height;
					} else {
						element.window.owner = APIGuidFromString (wallGuidStr.ToCStr ());
						element.window.objLoc = objLoc;
						element.window.lower = lower;
						element.window.openingBase.width = width;
						element.window.openingBase.height = height;
					}
				} else if (typeStr == "Object") {
					spec.Get ("posX", element.object.pos.x);
					spec.Get ("posY", element.object.pos.y);
					if (spec.Contains ("level")) {
						spec.Get ("level", element.object.level);
					}
					if (spec.Contains ("angle")) {
						spec.Get ("angle", element.object.angle);
					}
				} else if (typeStr == "Slab") {
					if (spec.Contains ("level")) {
						spec.Get ("level", element.slab.level);
					}
					if (spec.Contains ("thickness")) {
						spec.Get ("thickness", element.slab.thickness);
						element.slab.modelElemStructureType = API_BasicStructure;
					}
					GS::UniString bmatStr;
					if (spec.Get ("buildingMaterial", bmatStr) && !bmatStr.IsEmpty ()) {
						API_AttributeIndex bmIdx = FindBuildingMaterial (bmatStr);
						if (bmIdx != APIInvalidAttributeIndex) {
							element.slab.modelElemStructureType = API_BasicStructure;
							element.slab.buildingMaterial = bmIdx;
						}
					}
					GS::UniString compStr;
					if (spec.Get ("composite", compStr) && !compStr.IsEmpty ()) {
						API_AttributeIndex cIdx = FindComposite (compStr);
						if (cIdx != APIInvalidAttributeIndex) {
							element.slab.modelElemStructureType = API_CompositeStructure;
							element.slab.composite = cIdx;
						}
					}
					GS::Array<GS::ObjectState> polyNodes;
					if (spec.Get ("polygon", polyNodes)) {
						PopulatePolygonInMemo (polyNodes, element.slab.poly, memo);
					}
				} else if (typeStr == "Mesh") {
					if (spec.Contains ("level")) {
						spec.Get ("level", element.mesh.level);
					}
					if (spec.Contains ("skirtLevel")) {
						spec.Get ("skirtLevel", element.mesh.skirtLevel);
					}
					GS::UniString bmatStr;
					if (spec.Get ("buildingMaterial", bmatStr) && !bmatStr.IsEmpty ()) {
						API_AttributeIndex bmIdx = FindBuildingMaterial (bmatStr);
						if (bmIdx != APIInvalidAttributeIndex) {
							element.mesh.buildingMaterial = bmIdx;
						}
					}
					GS::Array<GS::ObjectState> polyNodes;
					if (spec.Get ("polygon", polyNodes) && PopulatePolygonInMemo (polyNodes, element.mesh.poly, memo)) {
						Int32 nCoords = element.mesh.poly.nCoords;
						if (memo.meshPolyZ != nullptr) {
							BMKillHandle ((GSHandle*) &memo.meshPolyZ);
						}
						memo.meshPolyZ = (double**) BMAllocateHandle ((nCoords + 1) * sizeof (double), ALLOCATE_CLEAR, 0);
						if (memo.meshPolyZ != nullptr) {
							for (UIndex p = 0; p < polyNodes.GetSize () && (Int32) p < nCoords; ++p) {
								double zVal = 0.0;
								polyNodes[p].Get ("z", zVal);
								(*memo.meshPolyZ)[p + 1] = zVal;
							}
							(*memo.meshPolyZ)[nCoords] = (*memo.meshPolyZ)[1];
						}
					}
				} else if (typeStr == "Zone") {
					GS::UniString roomName, roomNo;
					if (spec.Get ("roomName", roomName) && !roomName.IsEmpty ()) {
						GS::ucsncpy (element.zone.roomName, roomName.ToUStr (), API_UniLongNameLen - 1);
					}
					if (spec.Get ("roomNo", roomNo) && !roomNo.IsEmpty ()) {
						GS::ucsncpy (element.zone.roomNoStr, roomNo.ToUStr (), 31);
					}
					if (spec.Contains ("level")) {
						spec.Get ("level", element.zone.roomBaseLev);
					}
					if (spec.Contains ("height")) {
						spec.Get ("height", element.zone.roomHeight);
						element.zone.relativeTopStory = 0;
					}
					element.zone.manual = true;
					GS::Array<GS::ObjectState> polyNodes;
					if (spec.Get ("polygon", polyNodes) && PopulatePolygonInMemo (polyNodes, element.zone.poly, memo)) {
						double sumX = 0.0, sumY = 0.0;
						for (UIndex p = 0; p < polyNodes.GetSize (); ++p) {
							double px = 0.0, py = 0.0;
							polyNodes[p].Get ("x", px);
							polyNodes[p].Get ("y", py);
							sumX += px;
							sumY += py;
						}
						element.zone.pos.x = sumX / (double) polyNodes.GetSize ();
						element.zone.pos.y = sumY / (double) polyNodes.GetSize ();
					}
					if (spec.Contains ("posX") && spec.Contains ("posY")) {
						spec.Get ("posX", element.zone.pos.x);
						spec.Get ("posY", element.zone.pos.y);
					}
				}

				GSErrCode crErr = ACAPI_Element_Create (&element, hasMemo ? &memo : nullptr);
				if (hasMemo) {
					ACAPI_DisposeElemMemoHdls (&memo);
				}

				if (crErr == NoError) {
					createdGuids.Push (APIGuidToString (element.header.guid));
					GS::UniString elemId;
					if (spec.Get ("elemId", elemId) && !elemId.IsEmpty ()) {
						ACAPI_Element_ChangeElementInfoString (&element.header.guid, &elemId);
					}
				} else {
					lastErr = crErr;
				}
			}
			return NoError;
		});

		res.Add ("success", !createdGuids.IsEmpty ());
		res.Add ("createdCount", (Int32) createdGuids.GetSize ());
		res.Add ("createdGuids", createdGuids);
		if (lastErr != NoError) {
			res.Add ("lastError", (Int32) lastErr);
		}
		return res;
	}
};

// =============================================================================
// 14. GetSetGDLParameters Command (Inspect & Modify GDL AddPars & Scripts)
// =============================================================================

class GetSetGDLParametersCommand : public BimKernelBaseCommand {
public:
	virtual GS::String GetName () const override
	{
		return "GetSetGDLParameters";
	}

	virtual GS::ObjectState Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const override
	{
		GS::ObjectState res;
		GS::UniString guidStr;
		if (!parameters.Get ("guid", guidStr) || guidStr.IsEmpty ()) {
			res.Add ("success", false);
			res.Add ("error", "guid parameter is required");
			return res;
		}

		API_Guid elemGuid = APIGuidFromString (guidStr.ToCStr ());
		GS::UniString action = "get";
		parameters.Get ("action", action);

		API_ElementMemo memo {};
		GSErrCode err = ACAPI_Element_GetMemo (elemGuid, &memo, APIMemoMask_AddPars);
		if (err != NoError || memo.params == nullptr) {
			ACAPI_DisposeElemMemoHdls (&memo);
			res.Add ("success", false);
			res.Add ("error", "Element has no GDL parameters or could not read memo");
			return res;
		}

		GSSize numParams = BMGetHandleSize ((GSHandle) memo.params) / sizeof (API_AddParType);

		if (action == "get") {
			API_Element elem {};
			elem.header.guid = elemGuid;
			if (ACAPI_Element_Get (&elem) == NoError && elem.header.type == API_ObjectID) {
				res.Add ("libInd", elem.object.libInd);
				res.Add ("posX", elem.object.pos.x);
				res.Add ("posY", elem.object.pos.y);
				res.Add ("level", elem.object.level);
				res.Add ("angle", elem.object.angle);
				res.Add ("xRatio", elem.object.xRatio);
				res.Add ("yRatio", elem.object.yRatio);
				res.Add ("useXYFixSize", elem.object.useXYFixSize);

				API_LibPartSection sect {};
				sect.sectType = API_Sect3DScript;
				GS::UniString script3D;
				if (ACAPI_LibraryPart_GetSection (elem.object.libInd, &sect, nullptr, &script3D) == NoError) {
					res.Add ("script3D", script3D);
				}
				sect.sectType = API_Sect2DScript;
				GS::UniString script2D;
				if (ACAPI_LibraryPart_GetSection (elem.object.libInd, &sect, nullptr, &script2D) == NoError) {
					res.Add ("script2D", script2D);
				}
			}
			GS::Array<GS::ObjectState> paramList;
			for (GSSize i = 0; i < numParams; ++i) {
				const API_AddParType& par = (*memo.params)[i];
				if (par.typeID == API_ZombieParT || par.typeID == APIParT_Separator) {
					continue;
				}
				GS::ObjectState pState;
				pState.Add ("name", GS::UniString (par.name));
				pState.Add ("typeID", (Int32) par.typeID);
				if (par.typeID == APIParT_CString || par.typeID == APIParT_Title) {
					pState.Add ("valueStr", GS::UniString (par.value.uStr));
				} else {
					pState.Add ("valueNum", par.value.real);
				}
				paramList.Push (pState);
			}
			ACAPI_DisposeElemMemoHdls (&memo);
			res.Add ("success", true);
			res.Add ("count", (Int32) paramList.GetSize ());
			res.Add ("parameters", paramList);
			return res;
		}

		if (action == "setScript") {
			ACAPI_DisposeElemMemoHdls (&memo);
			API_Element elem {};
			elem.header.guid = elemGuid;
			if (ACAPI_Element_Get (&elem) != NoError || elem.header.type != API_ObjectID) {
				res.Add ("success", false);
				res.Add ("error", "Element is not an API_ObjectID");
				return res;
			}
			GS::UniString new3D, new2D;
			bool has3D = parameters.Get ("script3D", new3D);
			bool has2D = parameters.Get ("script2D", new2D);
			GSErrCode updErr = NoError;
			if (has3D) {
				API_LibPartSection sect {};
				sect.sectType = API_Sect3DScript;
				updErr = ACAPI_LibraryPart_UpdateSection (elem.object.libInd, &sect, nullptr, &new3D);
			}
			if (has2D && updErr == NoError) {
				API_LibPartSection sect {};
				sect.sectType = API_Sect2DScript;
				updErr = ACAPI_LibraryPart_UpdateSection (elem.object.libInd, &sect, nullptr, &new2D);
			}
			res.Add ("success", updErr == NoError);
			if (updErr != NoError) {
				res.Add ("errorCode", (Int32) updErr);
			}
			return res;
		}

		if (action == "set") {
			GS::Array<GS::ObjectState> updates;
			if (!parameters.Get ("updates", updates) || updates.IsEmpty ()) {
				ACAPI_DisposeElemMemoHdls (&memo);
				res.Add ("success", false);
				res.Add ("error", "updates array is required for action='set'");
				return res;
			}

			Int32 updatedCount = 0;
			for (UIndex u = 0; u < updates.GetSize (); ++u) {
				GS::UniString targetName;
				if (!updates[u].Get ("name", targetName) || targetName.IsEmpty ()) {
					continue;
				}
				for (GSSize i = 0; i < numParams; ++i) {
					API_AddParType& par = (*memo.params)[i];
					if (GS::UniString (par.name) == targetName) {
						if (par.typeID == APIParT_CString || par.typeID == APIParT_Title) {
							GS::UniString newStr;
							if (updates[u].Get ("valueStr", newStr)) {
								GS::ucsncpy (par.value.uStr, newStr.ToUStr (), API_UAddParStrLen - 1);
								updatedCount++;
							}
						} else {
							double newNum = 0.0;
							if (updates[u].Get ("valueNum", newNum)) {
								par.value.real = newNum;
								updatedCount++;
							}
						}
						break;
					}
				}
			}

			GSErrCode chgErr = ACAPI_CallUndoableCommand ("Modify GDL Parameters", [&] () -> GSErrCode {
				return ACAPI_Element_ChangeMemo (elemGuid, APIMemoMask_AddPars, &memo);
			});

			ACAPI_DisposeElemMemoHdls (&memo);
			res.Add ("success", chgErr == NoError);
			res.Add ("updatedCount", updatedCount);
			if (chgErr != NoError) {
				res.Add ("errorCode", (Int32) chgErr);
			}
			return res;
		}

		ACAPI_DisposeElemMemoHdls (&memo);
		res.Add ("success", false);
		res.Add ("error", "Unsupported action: use 'get', 'set', or 'setScript'");
		return res;
	}
};

// =============================================================================
// 15. DatabaseAndViewControl Command (rebuild, redraw, save, saveAs)
// =============================================================================

class DatabaseAndViewControlCommand : public BimKernelBaseCommand {
public:
	virtual GS::String GetName () const override
	{
		return "DatabaseAndViewControl";
	}

	virtual GS::ObjectState Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const override
	{
		GS::ObjectState res;
		GS::UniString action = "rebuild";
		parameters.Get ("action", action);

		GSErrCode err = NoError;
		if (action == "rebuild") {
			bool regenerate = true;
			parameters.Get ("regenerate", regenerate);
			err = ACAPI_View_Rebuild (&regenerate);
		} else if (action == "redraw") {
			err = ACAPI_View_Redraw ();
		} else if (action == "save") {
			err = ACAPI_ProjectOperation_Save ();
		} else if (action == "saveAs") {
			GS::UniString filePath;
			if (parameters.Get ("path", filePath) && !filePath.IsEmpty ()) {
				API_FileSavePars fileSavePars {};
				fileSavePars.fileTypeID = APIFType_PlanFile;
				IO::Location loc (filePath);
				fileSavePars.file = &loc;
				err = ACAPI_ProjectOperation_Save (&fileSavePars);
			} else {
				err = APIERR_BADPARS;
			}
		} else {
			res.Add ("success", false);
			res.Add ("error", "Unsupported action: use 'rebuild', 'redraw', 'save', or 'saveAs'");
			return res;
		}

		res.Add ("success", err == NoError);
		res.Add ("action", action);
		if (err != NoError) {
			res.Add ("errorCode", (Int32) err);
		}
		return res;
	}
};

// =============================================================================
// 16. ModifyBuildingMaterials Command (list, modify, create)
// =============================================================================

class ModifyBuildingMaterialsCommand : public BimKernelBaseCommand {
public:
	virtual GS::String GetName () const override
	{
		return "ModifyBuildingMaterials";
	}

	virtual GS::ObjectState Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const override
	{
		GS::ObjectState res;
		GS::UniString action = "modify";
		parameters.Get ("action", action);

		if (action == "list") {
			UInt32 count = 0;
			ACAPI_Attribute_GetNum (API_BuildingMaterialID, count);
			GS::Array<GS::ObjectState> bmatList;
			for (UInt32 i = 1; i <= count; ++i) {
				API_Attribute attr {};
				attr.header.typeID = API_BuildingMaterialID;
				attr.header.index = ACAPI_CreateAttributeIndex ((Int32) i);
				if (ACAPI_Attribute_Get (&attr) == NoError) {
					GS::ObjectState bState;
					bState.Add ("index", attr.header.index.ToInt32_Deprecated ());
					bState.Add ("name", GS::UniString (attr.header.name));
					bState.Add ("guid", APIGuidToString (attr.header.guid));
					bState.Add ("connPriority", attr.buildingMaterial.connPriority);
					bState.Add ("cutFill", attr.buildingMaterial.cutFill.ToInt32_Deprecated ());
					bState.Add ("cutFillName", GetAttributeNameByIndex (API_FilltypeID, attr.buildingMaterial.cutFill));
					bState.Add ("cutFillPen", (Int32) attr.buildingMaterial.cutFillPen);
					bState.Add ("cutFillBackgroundPen", (Int32) attr.buildingMaterial.cutFillBackgroundPen);
					bState.Add ("cutMaterial", attr.buildingMaterial.cutMaterial.ToInt32_Deprecated ());
					bState.Add ("cutMaterialName", GetAttributeNameByIndex (API_MaterialID, attr.buildingMaterial.cutMaterial));
					bmatList.Push (bState);
				}
			}
			res.Add ("success", true);
			res.Add ("count", (Int32) bmatList.GetSize ());
			res.Add ("buildingMaterials", bmatList);
			return res;
		}

		GS::Array<GS::ObjectState> materials;
		if (!parameters.Get ("materials", materials) || materials.IsEmpty ()) {
			res.Add ("success", false);
			res.Add ("error", "materials array is required");
			return res;
		}

		Int32 modifiedCount = 0;
		GSErrCode lastErr = NoError;

		for (UIndex i = 0; i < materials.GetSize (); ++i) {
			const GS::ObjectState& item = materials[i];
			GS::UniString nameOrGuid;
			if (!item.Get ("material", nameOrGuid) || nameOrGuid.IsEmpty ()) {
				continue;
			}

			API_Attribute attr {};
			attr.header.typeID = API_BuildingMaterialID;

			API_Guid mGuid = APIGuidFromString (nameOrGuid.ToCStr ());
			if (mGuid != APINULLGuid) {
				attr.header.guid = mGuid;
			} else {
				attr.header.uniStringNamePtr = &nameOrGuid;
			}

			bool exists = (ACAPI_Attribute_Search (&attr.header) == NoError && ACAPI_Attribute_Get (&attr) == NoError);
			if (!exists) {
				bool createIfMissing = false;
				item.Get ("createIfMissing", createIfMissing);
				if (!createIfMissing && action != "create") {
					continue;
				}
				attr = {};
				attr.header.typeID = API_BuildingMaterialID;
				attr.header.uniStringNamePtr = &nameOrGuid;
				attr.buildingMaterial.connPriority = 500;
				attr.buildingMaterial.cutFill = ACAPI_CreateAttributeIndex (1);
				attr.buildingMaterial.cutFillPen = 1;
				attr.buildingMaterial.cutFillBackgroundPen = 0;
				attr.buildingMaterial.cutMaterial = ACAPI_CreateAttributeIndex (1);
			}

			bool changed = !exists;

			Int32 priority = 0;
			if (item.Get ("connPriority", priority)) {
				attr.buildingMaterial.connPriority = priority;
				changed = true;
			}

			Int32 pen = 0;
			if (item.Get ("cutFillPen", pen)) {
				attr.buildingMaterial.cutFillPen = (short) pen;
				changed = true;
			}

			Int32 bgPen = 0;
			if (item.Get ("cutFillBackgroundPen", bgPen)) {
				attr.buildingMaterial.cutFillBackgroundPen = (short) bgPen;
				changed = true;
			}

			GS::UniString fillNameOrGuid;
			if (item.Get ("cutFill", fillNameOrGuid) && !fillNameOrGuid.IsEmpty ()) {
				API_Attr_Head fillHead {};
				fillHead.typeID = API_FilltypeID;
				API_Guid fGuid = APIGuidFromString (fillNameOrGuid.ToCStr ());
				if (fGuid != APINULLGuid) {
					fillHead.guid = fGuid;
				} else {
					fillHead.uniStringNamePtr = &fillNameOrGuid;
				}
				if (ACAPI_Attribute_Search (&fillHead) == NoError) {
					attr.buildingMaterial.cutFill = fillHead.index;
					changed = true;
				}
			}

			GS::UniString surfNameOrGuid;
			if (item.Get ("cutMaterial", surfNameOrGuid) && !surfNameOrGuid.IsEmpty ()) {
				API_AttributeIndex sIdx = FindSurfaceMaterial (surfNameOrGuid);
				if (sIdx != APIInvalidAttributeIndex) {
					attr.buildingMaterial.cutMaterial = sIdx;
					changed = true;
				}
			}

			if (changed) {
				GSErrCode modErr = exists ? ACAPI_Attribute_Modify (&attr, nullptr) : ACAPI_Attribute_Create (&attr, nullptr);
				if (modErr == NoError) {
					modifiedCount++;
				} else {
					lastErr = modErr;
				}
			}
		}

		res.Add ("success", modifiedCount > 0);
		res.Add ("modifiedCount", modifiedCount);
		if (lastErr != NoError) {
			res.Add ("errorCode", (Int32) lastErr);
		}
		return res;
	}
};

// =============================================================================
// 17. ManageComposites Command (list, create, modify API_CompWallID skins)
// =============================================================================

class ManageCompositesCommand : public BimKernelBaseCommand {
public:
	virtual GS::String GetName () const override
	{
		return "ManageComposites";
	}

	virtual GS::ObjectState Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const override
	{
		GS::ObjectState res;
		GS::UniString action = "list";
		parameters.Get ("action", action);

		if (action == "list") {
			UInt32 count = 0;
			ACAPI_Attribute_GetNum (API_CompWallID, count);
			GS::Array<GS::ObjectState> compList;

			for (UInt32 i = 1; i <= count; ++i) {
				API_Attribute attr {};
				attr.header.typeID = API_CompWallID;
				attr.header.index = ACAPI_CreateAttributeIndex ((Int32) i);
				if (ACAPI_Attribute_Get (&attr) == NoError) {
					GS::ObjectState cState;
					cState.Add ("index", attr.header.index.ToInt32_Deprecated ());
					cState.Add ("name", GS::UniString (attr.header.name));
					cState.Add ("guid", APIGuidToString (attr.header.guid));
					cState.Add ("totalThick", attr.compWall.totalThick);
					cState.Add ("nComps", (Int32) attr.compWall.nComps);
					cState.Add ("forWall", (attr.compWall.head.flags & APICWall_ForWall) != 0);
					cState.Add ("forSlab", (attr.compWall.head.flags & APICWall_ForSlab) != 0);
					cState.Add ("forRoof", (attr.compWall.head.flags & APICWall_ForRoof) != 0);
					cState.Add ("forShell", (attr.compWall.head.flags & APICWall_ForShell) != 0);

					API_AttributeDefExt defs {};
					if (ACAPI_Attribute_GetDefExt (API_CompWallID, attr.header.index, &defs) == NoError) {
						GS::Array<GS::ObjectState> skins;
						if (defs.cwall_compItems != nullptr) {
							for (short s = 0; s < attr.compWall.nComps; ++s) {
								const API_CWallComponent& comp = (*defs.cwall_compItems)[s];
								GS::ObjectState sState;
								sState.Add ("buildingMaterialIndex", comp.buildingMaterial.ToInt32_Deprecated ());
								sState.Add ("buildingMaterial", GetAttributeNameByIndex (API_BuildingMaterialID, comp.buildingMaterial));
								sState.Add ("thickness", comp.fillThick);
								sState.Add ("framePen", (Int32) comp.framePen);
								sState.Add ("isCore", (comp.flagBits & APICWallComp_Core) != 0);
								sState.Add ("isFinish", (comp.flagBits & APICWallComp_Finish) != 0);
								skins.Push (sState);
							}
						}
						cState.Add ("skins", skins);
						ACAPI_DisposeAttrDefsHdlsExt (&defs);
					}
					compList.Push (cState);
				}
			}

			res.Add ("success", true);
			res.Add ("count", (Int32) compList.GetSize ());
			res.Add ("composites", compList);
			return res;
		}

		if (action == "create" || action == "modify") {
			GS::UniString compName;
			if (!parameters.Get ("name", compName) || compName.IsEmpty ()) {
				res.Add ("success", false);
				res.Add ("error", "name parameter is required for composite creation/modification");
				return res;
			}

			GS::Array<GS::ObjectState> skinsParam;
			if (!parameters.Get ("skins", skinsParam) || skinsParam.IsEmpty ()) {
				res.Add ("success", false);
				res.Add ("error", "skins array parameter is required");
				return res;
			}

			API_Attribute attr {};
			attr.header.typeID = API_CompWallID;
			attr.header.uniStringNamePtr = &compName;
			bool exists = (ACAPI_Attribute_Search (&attr.header) == NoError && ACAPI_Attribute_Get (&attr) == NoError);

			if (!exists) {
				attr = {};
				attr.header.typeID = API_CompWallID;
				attr.header.uniStringNamePtr = &compName;
				attr.compWall.head.flags = APICWall_ForWall | APICWall_ForSlab | APICWall_ForRoof | APICWall_ForShell;
			}

			short nComps = (short) skinsParam.GetSize ();
			attr.compWall.nComps = nComps;
			double totalThick = 0.0;

			API_AttributeDefExt defs {};
			defs.cwall_compItems = (API_CWallComponent**) BMAllocateHandle (nComps * sizeof (API_CWallComponent), ALLOCATE_CLEAR, 0);
			defs.cwall_compLItems = (API_CWallLineComponent**) BMAllocateHandle ((nComps + 1) * sizeof (API_CWallLineComponent), ALLOCATE_CLEAR, 0);

			for (short s = 0; s < nComps; ++s) {
				const GS::ObjectState& skinSpec = skinsParam[s];
				GS::UniString bmatStr;
				skinSpec.Get ("buildingMaterial", bmatStr);
				API_AttributeIndex bmIdx = FindBuildingMaterial (bmatStr);
				if (bmIdx == APIInvalidAttributeIndex) {
					bmIdx = ACAPI_CreateAttributeIndex (1);
				}

				double thick = 0.10;
				skinSpec.Get ("thickness", thick);
				totalThick += thick;

				Int32 framePen = 1;
				skinSpec.Get ("framePen", framePen);

				bool isCore = false, isFinish = false;
				skinSpec.Get ("isCore", isCore);
				skinSpec.Get ("isFinish", isFinish);
				short flags = 0;
				if (isCore) flags |= APICWallComp_Core;
				if (isFinish) flags |= APICWallComp_Finish;

				(*defs.cwall_compItems)[s].buildingMaterial = bmIdx;
				(*defs.cwall_compItems)[s].fillThick = thick;
				(*defs.cwall_compItems)[s].framePen = (short) framePen;
				(*defs.cwall_compItems)[s].flagBits = flags;
			}

			for (short l = 0; l <= nComps; ++l) {
				(*defs.cwall_compLItems)[l].ltypeInd = ACAPI_CreateAttributeIndex (1);
				(*defs.cwall_compLItems)[l].linePen = 1;
			}

			attr.compWall.totalThick = totalThick;

			GSErrCode err = exists ? ACAPI_Attribute_ModifyExt (&attr, &defs) : ACAPI_Attribute_CreateExt (&attr, &defs);
			ACAPI_DisposeAttrDefsHdlsExt (&defs);

			res.Add ("success", err == NoError);
			res.Add ("name", compName);
			res.Add ("index", attr.header.index.ToInt32_Deprecated ());
			res.Add ("guid", APIGuidToString (attr.header.guid));
			res.Add ("totalThick", totalThick);
			if (err != NoError) {
				res.Add ("errorCode", (Int32) err);
			}
			return res;
		}

		res.Add ("success", false);
		res.Add ("error", "Unsupported action: use 'list', 'create', or 'modify'");
		return res;
	}
};

// =============================================================================
// 18. ManageSurfaces Command (list, create, modify API_MaterialID 3D surfaces)
// =============================================================================

class ManageSurfacesCommand : public BimKernelBaseCommand {
public:
	virtual GS::String GetName () const override
	{
		return "ManageSurfaces";
	}

	virtual GS::ObjectState Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const override
	{
		GS::ObjectState res;
		GS::UniString action = "list";
		parameters.Get ("action", action);

		if (action == "list") {
			UInt32 count = 0;
			ACAPI_Attribute_GetNum (API_MaterialID, count);
			GS::Array<GS::ObjectState> surfList;

			for (UInt32 i = 1; i <= count; ++i) {
				API_Attribute attr {};
				attr.header.typeID = API_MaterialID;
				attr.header.index = ACAPI_CreateAttributeIndex ((Int32) i);
				if (ACAPI_Attribute_Get (&attr) == NoError) {
					if (attr.material.texture.fileLoc != nullptr) {
						delete attr.material.texture.fileLoc;
						attr.material.texture.fileLoc = nullptr;
					}
					GS::ObjectState sState;
					sState.Add ("index", attr.header.index.ToInt32_Deprecated ());
					sState.Add ("name", GS::UniString (attr.header.name));
					sState.Add ("guid", APIGuidToString (attr.header.guid));
					sState.Add ("red", attr.material.surfaceRGB.f_red);
					sState.Add ("green", attr.material.surfaceRGB.f_green);
					sState.Add ("blue", attr.material.surfaceRGB.f_blue);
					sState.Add ("transpPc", (Int32) attr.material.transpPc);
					sState.Add ("shine", (Int32) attr.material.shine);
					sState.Add ("ambientPc", (Int32) attr.material.ambientPc);
					sState.Add ("diffusePc", (Int32) attr.material.diffusePc);
					sState.Add ("specularPc", (Int32) attr.material.specularPc);
					surfList.Push (sState);
				}
			}

			res.Add ("success", true);
			res.Add ("count", (Int32) surfList.GetSize ());
			res.Add ("surfaces", surfList);
			return res;
		}

		GS::Array<GS::ObjectState> surfaces;
		if (!parameters.Get ("surfaces", surfaces) || surfaces.IsEmpty ()) {
			res.Add ("success", false);
			res.Add ("error", "surfaces array parameter is required");
			return res;
		}

		Int32 modifiedCount = 0;
		GSErrCode lastErr = NoError;

		for (UIndex i = 0; i < surfaces.GetSize (); ++i) {
			const GS::ObjectState& item = surfaces[i];
			GS::UniString nameOrGuid;
			if (!item.Get ("surface", nameOrGuid) || nameOrGuid.IsEmpty ()) {
				continue;
			}

			API_Attribute attr {};
			attr.header.typeID = API_MaterialID;
			API_Guid sGuid = APIGuidFromString (nameOrGuid.ToCStr ());
			if (sGuid != APINULLGuid) {
				attr.header.guid = sGuid;
			} else {
				attr.header.uniStringNamePtr = &nameOrGuid;
			}

			bool exists = (ACAPI_Attribute_Search (&attr.header) == NoError && ACAPI_Attribute_Get (&attr) == NoError);
			if (!exists) {
				attr = {};
				attr.header.typeID = API_MaterialID;
				attr.header.index = ACAPI_CreateAttributeIndex (1);
				ACAPI_Attribute_Get (&attr);
				if (attr.material.texture.fileLoc != nullptr) {
					delete attr.material.texture.fileLoc;
					attr.material.texture.fileLoc = nullptr;
				}
				attr.header.index = APIInvalidAttributeIndex;
				attr.header.guid = APINULLGuid;
				attr.header.uniStringNamePtr = &nameOrGuid;
			}

			if (item.Contains ("red")) item.Get ("red", attr.material.surfaceRGB.f_red);
			if (item.Contains ("green")) item.Get ("green", attr.material.surfaceRGB.f_green);
			if (item.Contains ("blue")) item.Get ("blue", attr.material.surfaceRGB.f_blue);

			Int32 val = 0;
			if (item.Get ("transpPc", val)) attr.material.transpPc = (short) val;
			if (item.Get ("shine", val)) attr.material.shine = (short) val;
			if (item.Get ("ambientPc", val)) attr.material.ambientPc = (short) val;
			if (item.Get ("diffusePc", val)) attr.material.diffusePc = (short) val;
			if (item.Get ("specularPc", val)) attr.material.specularPc = (short) val;

			GSErrCode err = exists ? ACAPI_Attribute_Modify (&attr, nullptr) : ACAPI_Attribute_Create (&attr, nullptr);
			if (attr.material.texture.fileLoc != nullptr) {
				delete attr.material.texture.fileLoc;
				attr.material.texture.fileLoc = nullptr;
			}
			if (err == NoError) {
				modifiedCount++;
			} else {
				lastErr = err;
			}
		}

		res.Add ("success", modifiedCount > 0);
		res.Add ("modifiedCount", modifiedCount);
		if (lastErr != NoError) {
			res.Add ("errorCode", (Int32) lastErr);
		}
		return res;
	}
};

// =============================================================================
// 19. ManageStories Command (list, setHeight, setElevation, rename, insAbove, insBelow, delete, goTo)
// =============================================================================

class ManageStoriesCommand : public BimKernelBaseCommand {
public:
	virtual GS::String GetName () const override
	{
		return "ManageStories";
	}

	virtual GS::ObjectState Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const override
	{
		GS::ObjectState res;
		GS::UniString action = "list";
		parameters.Get ("action", action);

		if (action == "list") {
			API_StoryInfo storyInfo {};
			GSErrCode err = ACAPI_ProjectSetting_GetStorySettings (&storyInfo);
			if (err != NoError) {
				res.Add ("success", false);
				res.Add ("errorCode", (Int32) err);
				return res;
			}

			GS::Array<GS::ObjectState> storiesList;
			if (storyInfo.data != nullptr) {
				Int32 count = storyInfo.lastStory - storyInfo.firstStory + 1;
				for (Int32 i = 0; i < count; ++i) {
					const API_StoryType& st = (*storyInfo.data)[i];
					double nextLevel = (*storyInfo.data)[i + 1].level;
					GS::ObjectState sState;
					sState.Add ("index", (Int32) st.index);
					sState.Add ("floorId", (Int32) st.floorId);
					sState.Add ("name", GS::UniString (st.uName));
					sState.Add ("level", st.level);
					sState.Add ("height", nextLevel - st.level);
					sState.Add ("dispOnSections", st.dispOnSections);
					storiesList.Push (sState);
				}
				BMKillHandle ((GSHandle*) &storyInfo.data);
			}

			res.Add ("success", true);
			res.Add ("firstStory", (Int32) storyInfo.firstStory);
			res.Add ("lastStory", (Int32) storyInfo.lastStory);
			res.Add ("actStory", (Int32) storyInfo.actStory);
			res.Add ("stories", storiesList);
			return res;
		}

		API_StoryCmdType storyCmd {};
		Int32 idx = 0;
		parameters.Get ("index", idx);
		storyCmd.index = (short) idx;

		if (action == "goTo") {
			storyCmd.action = APIStory_GoTo;
		} else if (action == "delete") {
			storyCmd.action = APIStory_Delete;
		} else if (action == "insAbove") {
			storyCmd.action = APIStory_InsAbove;
			storyCmd.height = 3.0;
			parameters.Get ("height", storyCmd.height);
			GS::UniString sName;
			if (parameters.Get ("name", sName)) {
				GS::ucsncpy (storyCmd.uName, sName.ToUStr (), API_UniLongNameLen - 1);
			}
		} else if (action == "insBelow") {
			storyCmd.action = APIStory_InsBelow;
			storyCmd.height = 3.0;
			parameters.Get ("height", storyCmd.height);
			GS::UniString sName;
			if (parameters.Get ("name", sName)) {
				GS::ucsncpy (storyCmd.uName, sName.ToUStr (), API_UniLongNameLen - 1);
			}
		} else if (action == "setHeight") {
			storyCmd.action = APIStory_SetHeight;
			parameters.Get ("height", storyCmd.height);
		} else if (action == "setElevation") {
			storyCmd.action = APIStory_SetElevation;
			parameters.Get ("elevation", storyCmd.elevation);
		} else if (action == "rename") {
			storyCmd.action = APIStory_Rename;
			GS::UniString sName;
			if (parameters.Get ("name", sName)) {
				GS::ucsncpy (storyCmd.uName, sName.ToUStr (), API_UniLongNameLen - 1);
			}
		} else {
			res.Add ("success", false);
			res.Add ("error", "Unsupported action: use 'list', 'goTo', 'setHeight', 'setElevation', 'rename', 'insAbove', 'insBelow', or 'delete'");
			return res;
		}

		GSErrCode err = ACAPI_ProjectSetting_ChangeStorySettings (&storyCmd);
		res.Add ("success", err == NoError);
		res.Add ("action", action);
		res.Add ("index", idx);
		if (err != NoError) {
			res.Add ("errorCode", (Int32) err);
		}
		return res;
	}
};

// =============================================================================
// 20. InspectElements Command (Kernel Batch Telemetry & 3D Bounding Box Inspector)
// =============================================================================

static GS::UniString ElemTypeToString (const API_ElemType& type)
{
	switch (type.typeID) {
		case API_WallID: return "Wall";
		case API_ColumnID: return "Column";
		case API_BeamID: return "Beam";
		case API_WindowID: return "Window";
		case API_DoorID: return "Door";
		case API_ObjectID: return "Object";
		case API_LampID: return "Lamp";
		case API_SlabID: return "Slab";
		case API_RoofID: return "Roof";
		case API_MeshID: return "Mesh";
		case API_ZoneID: return "Zone";
		case API_CurtainWallID: return "CurtainWall";
		case API_ShellID: return "Shell";
		case API_SkylightID: return "Skylight";
		case API_MorphID: return "Morph";
		case API_StairID: return "Stair";
		case API_RailingID: return "Railing";
		case API_OpeningID: return "Opening";
		default: return "Other";
	}
}

class InspectElementsCommand : public BimKernelBaseCommand {
public:
	virtual GS::String GetName () const override
	{
		return "InspectElements";
	}

	virtual GS::ObjectState Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const override
	{
		GS::ObjectState res;
		GS::Array<API_Guid> targetGuids;

		GS::Array<GS::UniString> guidStrings;
		if (parameters.Get ("elements", guidStrings) && !guidStrings.IsEmpty ()) {
			for (UIndex i = 0; i < guidStrings.GetSize (); ++i) {
				targetGuids.Push (APIGuidFromString (guidStrings[i].ToCStr ()));
			}
		} else {
			GS::Array<GS::UniString> typesFilter;
			parameters.Get ("types", typesFilter);
			if (typesFilter.IsEmpty ()) {
				ACAPI_Element_GetElemList (API_ZombieElemID, &targetGuids);
			} else {
				for (UIndex t = 0; t < typesFilter.GetSize (); ++t) {
					API_ElemTypeID tid = API_ZombieElemID;
					if (typesFilter[t] == "Wall") tid = API_WallID;
					else if (typesFilter[t] == "Slab") tid = API_SlabID;
					else if (typesFilter[t] == "Column") tid = API_ColumnID;
					else if (typesFilter[t] == "Beam") tid = API_BeamID;
					else if (typesFilter[t] == "Window") tid = API_WindowID;
					else if (typesFilter[t] == "Door") tid = API_DoorID;
					else if (typesFilter[t] == "Object") tid = API_ObjectID;
					else if (typesFilter[t] == "Roof") tid = API_RoofID;
					else if (typesFilter[t] == "Mesh") tid = API_MeshID;
					else if (typesFilter[t] == "Morph") tid = API_MorphID;
					else if (typesFilter[t] == "Zone") tid = API_ZoneID;
					else if (typesFilter[t] == "Stair") tid = API_StairID;
					else if (typesFilter[t] == "Railing") tid = API_RailingID;
					if (tid != API_ZombieElemID) {
						GS::Array<API_Guid> list;
						ACAPI_Element_GetElemList (tid, &list);
						for (UIndex k = 0; k < list.GetSize (); ++k) {
							targetGuids.Push (list[k]);
						}
					}
				}
			}
		}

		bool hasFloorFilter = parameters.Contains ("floorInd");
		Int32 floorFilter = 0;
		if (hasFloorFilter) {
			parameters.Get ("floorInd", floorFilter);
		}

		GS::Array<GS::ObjectState> results;
		for (UIndex i = 0; i < targetGuids.GetSize (); ++i) {
			API_Element elem {};
			elem.header.guid = targetGuids[i];
			if (ACAPI_Element_Get (&elem) != NoError) {
				continue;
			}
			if (hasFloorFilter && (Int32) elem.header.floorInd != floorFilter) {
				continue;
			}

			GS::ObjectState item;
			item.Add ("guid", APIGuidToString (elem.header.guid));
			item.Add ("type", ElemTypeToString (elem.header.type));
			item.Add ("floorInd", (Int32) elem.header.floorInd);
			item.Add ("layerIndex", elem.header.layer.ToInt32_Deprecated ());
			item.Add ("layerName", GetAttributeNameByIndex (API_LayerID, elem.header.layer));

			GS::UniString elemIdStr;
			if (ACAPI_Element_GetElementInfoString (&elem.header.guid, &elemIdStr) == NoError) {
				item.Add ("elemId", elemIdStr);
			}

			API_Box3D bounds {};
			if (ACAPI_Element_CalcBounds (&elem.header, &bounds) == NoError) {
				item.Add ("xMin", bounds.xMin);
				item.Add ("yMin", bounds.yMin);
				item.Add ("zMin", bounds.zMin);
				item.Add ("xMax", bounds.xMax);
				item.Add ("yMax", bounds.yMax);
				item.Add ("zMax", bounds.zMax);
			}

			if (elem.header.type == API_WallID) {
				item.Add ("begX", elem.wall.begC.x);
				item.Add ("begY", elem.wall.begC.y);
				item.Add ("endX", elem.wall.endC.x);
				item.Add ("endY", elem.wall.endC.y);
				item.Add ("height", elem.wall.height);
				item.Add ("bottomOffset", elem.wall.bottomOffset);
				item.Add ("thickness", elem.wall.thickness);
				item.Add ("contPen", (Int32) elem.wall.contPen);
				if (elem.wall.modelElemStructureType == API_BasicStructure) {
					item.Add ("structure", "Basic");
					item.Add ("buildingMaterial", GetAttributeNameByIndex (API_BuildingMaterialID, elem.wall.buildingMaterial));
				} else if (elem.wall.modelElemStructureType == API_CompositeStructure) {
					item.Add ("structure", "Composite");
					item.Add ("composite", GetAttributeNameByIndex (API_CompWallID, elem.wall.composite));
				}
			} else if (elem.header.type == API_SlabID) {
				item.Add ("level", elem.slab.level);
				item.Add ("thickness", elem.slab.thickness);
				if (elem.slab.modelElemStructureType == API_BasicStructure) {
					item.Add ("structure", "Basic");
					item.Add ("buildingMaterial", GetAttributeNameByIndex (API_BuildingMaterialID, elem.slab.buildingMaterial));
				} else if (elem.slab.modelElemStructureType == API_CompositeStructure) {
					item.Add ("structure", "Composite");
					item.Add ("composite", GetAttributeNameByIndex (API_CompWallID, elem.slab.composite));
				}
			} else if (elem.header.type == API_ColumnID) {
				item.Add ("posX", elem.column.origoPos.x);
				item.Add ("posY", elem.column.origoPos.y);
				item.Add ("bottomOffset", elem.column.bottomOffset);
				item.Add ("height", elem.column.height);
			} else if (elem.header.type == API_BeamID) {
				item.Add ("begX", elem.beam.begC.x);
				item.Add ("begY", elem.beam.begC.y);
				item.Add ("endX", elem.beam.endC.x);
				item.Add ("endY", elem.beam.endC.y);
				item.Add ("level", elem.beam.level);
			} else if (elem.header.type == API_ObjectID) {
				item.Add ("posX", elem.object.pos.x);
				item.Add ("posY", elem.object.pos.y);
				item.Add ("level", elem.object.level);
				item.Add ("angle", elem.object.angle);
				item.Add ("xRatio", elem.object.xRatio);
				item.Add ("yRatio", elem.object.yRatio);
				item.Add ("libInd", elem.object.libInd);
			} else if (elem.header.type == API_WindowID || elem.header.type == API_DoorID) {
				const bool isWin = (elem.header.type == API_WindowID);
				item.Add ("ownerGuid", APIGuidToString (isWin ? elem.window.owner : elem.door.owner));
				item.Add ("objLoc", isWin ? elem.window.objLoc : elem.door.objLoc);
				item.Add ("lower", isWin ? elem.window.lower : elem.door.lower);
				item.Add ("width", isWin ? elem.window.openingBase.width : elem.door.openingBase.width);
				item.Add ("height", isWin ? elem.window.openingBase.height : elem.door.openingBase.height);
				item.Add ("revealDepthOffset", isWin ? elem.window.revealDepthOffset : elem.door.revealDepthOffset);
			}

			results.Push (item);
		}

		res.Add ("success", true);
		res.Add ("count", (Int32) results.GetSize ());
		res.Add ("elements", results);
		return res;
	}
};

// =============================================================================
// 21. ManageNavigatorViews Command (list, modify, goTo)
// =============================================================================

static void CollectNavItemsRecursive (API_NavigatorMapID mapId, const API_Guid& parentGuid, GS::Array<GS::ObjectState>& outList)
{
	API_NavigatorItem parentItem {};
	parentItem.guid = parentGuid;
	parentItem.mapId = mapId;

	GS::Array<API_NavigatorItem> children;
	if (ACAPI_Navigator_GetNavigatorChildrenItems (&parentItem, &children) != NoError) {
		return;
	}

	for (UIndex i = 0; i < children.GetSize (); ++i) {
		API_NavigatorItem& child = children[i];
		child.mapId = mapId;
		if (child.itemType != API_FolderNavItem && child.itemType != API_UndefinedNavItem && child.itemType != API_ProjectNavItem) {
			GS::ObjectState vState;
			vState.Add ("guid", APIGuidToString (child.guid));
			vState.Add ("name", GS::UniString (child.uName));
			vState.Add ("uiId", GS::UniString (child.uiId));
			vState.Add ("itemType", (Int32) child.itemType);

			API_NavigatorView navView {};
			if (ACAPI_Navigator_GetNavigatorView (&child, &navView) == NoError) {
				vState.Add ("layerCombination", GS::UniString (navView.layerCombination));
				vState.Add ("modelViewOptName", GS::UniString (navView.modelViewOptName));
				vState.Add ("penSetName", GS::UniString (navView.penSetName));
				vState.Add ("drawingScale", navView.drawingScale);
				if (navView.modelViewOpt != nullptr) {
					ACAPI_FreeGDLModelViewOptionsPtr (&navView.modelViewOpt->gdlOptions);
					BMKillPtr ((GSPtr*) &navView.modelViewOpt);
				}
				if (navView.dimPrefs != nullptr) {
					BMKillPtr ((GSPtr*) &navView.dimPrefs);
				}
			}
			outList.Push (vState);
		}
		CollectNavItemsRecursive (mapId, child.guid, outList);
	}
}

class ManageNavigatorViewsCommand : public BimKernelBaseCommand {
public:
	virtual GS::String GetName () const override
	{
		return "ManageNavigatorViews";
	}

	virtual GS::ObjectState Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const override
	{
		GS::ObjectState res;
		GS::UniString action = "list";
		parameters.Get ("action", action);

		if (action == "list") {
			API_NavigatorSet viewSet {};
			viewSet.mapId = API_PublicViewMap;
			GS::Array<GS::ObjectState> viewList;
			if (ACAPI_Navigator_GetNavigatorSet (&viewSet) == NoError) {
				CollectNavItemsRecursive (API_PublicViewMap, viewSet.rootGuid, viewList);
			}
			res.Add ("success", true);
			res.Add ("count", (Int32) viewList.GetSize ());
			res.Add ("views", viewList);
			return res;
		}

		if (action == "goTo") {
			GS::UniString viewGuidStr;
			if (!parameters.Get ("viewGuid", viewGuidStr) || viewGuidStr.IsEmpty ()) {
				res.Add ("success", false);
				res.Add ("error", "viewGuid parameter is required for action='goTo'");
				return res;
			}
			GSErrCode err = ACAPI_View_GoToView (viewGuidStr.ToCStr ().Get ());
			res.Add ("success", err == NoError);
			if (err != NoError) {
				res.Add ("errorCode", (Int32) err);
			}
			return res;
		}

		if (action == "modify") {
			GS::UniString viewGuidStr;
			if (!parameters.Get ("viewGuid", viewGuidStr) || viewGuidStr.IsEmpty ()) {
				res.Add ("success", false);
				res.Add ("error", "viewGuid parameter is required for action='modify'");
				return res;
			}

			API_Guid vGuid = APIGuidFromString (viewGuidStr.ToCStr ());
			API_NavigatorItem item {};
			item.guid = vGuid;
			item.mapId = API_PublicViewMap;
			if (ACAPI_Navigator_GetNavigatorItem (&vGuid, &item) != NoError) {
				res.Add ("success", false);
				res.Add ("error", "Navigator view item not found");
				return res;
			}

			API_NavigatorView view {};
			ACAPI_Navigator_GetNavigatorView (&item, &view);

			GS::UniString newName;
			if (parameters.Get ("name", newName) && !newName.IsEmpty ()) {
				GS::ucsncpy (item.uName, newName.ToUStr (), API_UniLongNameLen - 1);
				item.customName = true;
			}

			GS::UniString layerComb;
			if (parameters.Get ("layerCombination", layerComb) && !layerComb.IsEmpty ()) {
				std::strncpy (view.layerCombination, layerComb.ToCStr ().Get (), API_AttrNameLen - 1);
				view.saveLaySet = true;
			}

			GS::UniString mvoName;
			if (parameters.Get ("modelViewOptName", mvoName) && !mvoName.IsEmpty ()) {
				std::strncpy (view.modelViewOptName, mvoName.ToCStr ().Get (), API_LongNameLen - 1);
				view.saveDispOpt = true;
			}

			Int32 scale = 0;
			if (parameters.Get ("drawingScale", scale) && scale > 0) {
				view.drawingScale = scale;
				view.saveDScale = true;
			}

			GSErrCode err = ACAPI_Navigator_ChangeNavigatorView (&item, &view);
			res.Add ("success", err == NoError);
			if (err != NoError) {
				res.Add ("errorCode", (Int32) err);
			}
			return res;
		}

		res.Add ("success", false);
		res.Add ("error", "Unsupported action: use 'list', 'goTo', or 'modify'");
		return res;
	}
};

// =============================================================================
// 22. DirectExport Command (Direct export to IFC, PDF, PLN, PLA, PNG, JPEG)
// =============================================================================

class DirectExportCommand : public BimKernelBaseCommand {
public:
	virtual GS::String GetName () const override
	{
		return "DirectExport";
	}

	virtual GS::ObjectState Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const override
	{
		GS::ObjectState res;
		GS::UniString filePath;
		if (!parameters.Get ("filePath", filePath) || filePath.IsEmpty ()) {
			res.Add ("success", false);
			res.Add ("error", "filePath parameter is required");
			return res;
		}

		GS::UniString format = "ifc";
		parameters.Get ("format", format);

		IO::Location loc (filePath);
		API_FileSavePars fsp {};
		fsp.file = &loc;
		GSErrCode err = NoError;

		if (format == "ifc" || format == "IFC") {
			fsp.fileTypeID = APIFType_IfcFile;
			API_SavePars_Ifc ifcPars {};
			ifcPars.subType = API_IFC;
			ifcPars.elementsToIfcExport = API_EntireProject;
			ifcPars.includeBoundingBoxGeometry = false;

			GS::Array<API_IFCTranslatorIdentifier> translators;
			if (ACAPI_IFC_GetIFCExportTranslatorsList (translators) == NoError && !translators.IsEmpty ()) {
				ifcPars.translatorIdentifier = translators[0];
			}
			err = ACAPI_ProjectOperation_Save (&fsp, &ifcPars);
		} else if (format == "pdf" || format == "PDF") {
			fsp.fileTypeID = APIFType_PdfFile;
			API_SavePars_Pdf pdfPars {};
			err = ACAPI_ProjectOperation_Save (&fsp, &pdfPars);
		} else if (format == "pla" || format == "PLA") {
			fsp.fileTypeID = APIFType_A_PlanFile;
			API_SavePars_Archive archPars {};
			archPars.picturesInTIFF = true;
			archPars.texturesOn = true;
			archPars.propertiesOn = true;
			archPars.libraryPartsOn = true;
			err = ACAPI_ProjectOperation_Save (&fsp, &archPars);
		} else if (format == "png" || format == "PNG") {
			fsp.fileTypeID = APIFType_PNGFile;
			API_SavePars_Picture picPars {};
			picPars.colorDepth = APIColorDepth_TC32;
			picPars.view2D = true;
			parameters.Get ("view2D", picPars.view2D);
			err = ACAPI_ProjectOperation_Save (&fsp, &picPars);
		} else {
			fsp.fileTypeID = APIFType_PlanFile;
			err = ACAPI_ProjectOperation_Save (&fsp);
		}

		res.Add ("success", err == NoError);
		res.Add ("format", format);
		res.Add ("filePath", filePath);
		if (err != NoError) {
			res.Add ("errorCode", (Int32) err);
		}
		return res;
	}
};

// =============================================================================
// Dialog & Menu Handling
// =============================================================================

class ExampleDialog : public DG::ModalDialog,
					  public DG::PanelObserver,
					  public DG::ButtonItemObserver,
					  public DG::CompoundItemObserver
{
public:
	enum DialogResourceIds {
		ExampleDialogResourceId = ID_ADDON_DLG,
		OKButtonId = 1,
		CancelButtonId = 2,
		SeparatorId = 3
	};

	ExampleDialog () :
		DG::ModalDialog (ACAPI_GetOwnResModule (), ExampleDialogResourceId, ACAPI_GetOwnResModule ()),
		okButton (GetReference (), OKButtonId),
		cancelButton (GetReference (), CancelButtonId),
		separator (GetReference (), SeparatorId)
	{
		SetTitle ("BimKernel v3.0.0");
		AttachToAllItems (*this);
		Attach (*this);
	}

	~ExampleDialog ()
	{
		Detach (*this);
		DetachFromAllItems (*this);
	}

private:
	virtual void PanelResized (const DG::PanelResizeEvent& ev) override
	{
		BeginMoveResizeItems ();
		okButton.Move (ev.GetHorizontalChange (), ev.GetVerticalChange ());
		cancelButton.Move (ev.GetHorizontalChange (), ev.GetVerticalChange ());
		separator.MoveAndResize (0, ev.GetVerticalChange (), ev.GetHorizontalChange (), 0);
		EndMoveResizeItems ();
	}

	virtual void ButtonClicked (const DG::ButtonClickEvent& ev) override
	{
		if (ev.GetSource () == &okButton) {
			PostCloseRequest (DG::ModalDialog::Accept);
		} else if (ev.GetSource () == &cancelButton) {
			PostCloseRequest (DG::ModalDialog::Cancel);
		}
	}

	DG::Button    okButton;
	DG::Button    cancelButton;
	DG::Separator separator;
};

static GSErrCode MenuCommandHandler (const API_MenuParams *menuParams)
{
	switch (menuParams->menuItemRef.menuResID) {
		case AddOnMenuID:
			switch (menuParams->menuItemRef.itemIndex) {
				case AddOnCommandID:
					{
						ExampleDialog dialog;
						dialog.Invoke ();
					}
					break;
			}
			break;
	}
	return NoError;
}

// =============================================================================
// Entry Points
// =============================================================================

API_AddonType CheckEnvironment (API_EnvirParams* envir)
{
	RSGetIndString (&envir->addOnInfo.name, AddOnInfoID, AddOnNameID, ACAPI_GetOwnResModule ());
	RSGetIndString (&envir->addOnInfo.description, AddOnInfoID, AddOnDescriptionID, ACAPI_GetOwnResModule ());

	return APIAddon_Preload;
}

GSErrCode RegisterInterface (void)
{
	return ACAPI_MenuItem_RegisterMenu (AddOnMenuID, 0, MenuCode_Tools, MenuFlag_Default);
}

GSErrCode Initialize (void)
{
	ACAPI_MenuItem_InstallMenuHandler (AddOnMenuID, MenuCommandHandler);

	// Register BimKernel v3.0.0 commands (22 commands)
	ACAPI_AddOnAddOnCommunication_InstallAddOnCommandHandler (GS::NewOwned<PingCommand> ());
	ACAPI_AddOnAddOnCommunication_InstallAddOnCommandHandler (GS::NewOwned<ChangeElementsLayerCommand> ());
	ACAPI_AddOnAddOnCommunication_InstallAddOnCommandHandler (GS::NewOwned<ManageHotlinksCommand> ());
	ACAPI_AddOnAddOnCommunication_InstallAddOnCommandHandler (GS::NewOwned<DeleteElementsCommand> ());
	ACAPI_AddOnAddOnCommunication_InstallAddOnCommandHandler (GS::NewOwned<ListLayersCommand> ());
	ACAPI_AddOnAddOnCommunication_InstallAddOnCommandHandler (GS::NewOwned<ModifyLayersStateCommand> ());
	ACAPI_AddOnAddOnCommunication_InstallAddOnCommandHandler (GS::NewOwned<PublishSetCommand> ());
	ACAPI_AddOnAddOnCommunication_InstallAddOnCommandHandler (GS::NewOwned<ListLayoutDatabasesCommand> ());
	ACAPI_AddOnAddOnCommunication_InstallAddOnCommandHandler (GS::NewOwned<PlaceDrawingOnLayoutCommand> ());
	ACAPI_AddOnAddOnCommunication_InstallAddOnCommandHandler (GS::NewOwned<SetFloorPlanCutPlaneCommand> ());
	ACAPI_AddOnAddOnCommunication_InstallAddOnCommandHandler (GS::NewOwned<SolidElementOperationsCommand> ());
	ACAPI_AddOnAddOnCommunication_InstallAddOnCommandHandler (GS::NewOwned<ModifyElementsGeometryCommand> ());
	ACAPI_AddOnAddOnCommunication_InstallAddOnCommandHandler (GS::NewOwned<CreateNativeElementsCommand> ());
	ACAPI_AddOnAddOnCommunication_InstallAddOnCommandHandler (GS::NewOwned<GetSetGDLParametersCommand> ());
	ACAPI_AddOnAddOnCommunication_InstallAddOnCommandHandler (GS::NewOwned<DatabaseAndViewControlCommand> ());
	ACAPI_AddOnAddOnCommunication_InstallAddOnCommandHandler (GS::NewOwned<ModifyBuildingMaterialsCommand> ());
	ACAPI_AddOnAddOnCommunication_InstallAddOnCommandHandler (GS::NewOwned<ManageCompositesCommand> ());
	ACAPI_AddOnAddOnCommunication_InstallAddOnCommandHandler (GS::NewOwned<ManageSurfacesCommand> ());
	ACAPI_AddOnAddOnCommunication_InstallAddOnCommandHandler (GS::NewOwned<ManageStoriesCommand> ());
	ACAPI_AddOnAddOnCommunication_InstallAddOnCommandHandler (GS::NewOwned<InspectElementsCommand> ());
	ACAPI_AddOnAddOnCommunication_InstallAddOnCommandHandler (GS::NewOwned<ManageNavigatorViewsCommand> ());
	ACAPI_AddOnAddOnCommunication_InstallAddOnCommandHandler (GS::NewOwned<DirectExportCommand> ());

	return NoError;
}

GSErrCode FreeData (void)
{
	return NoError;
}
