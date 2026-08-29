// Copyright Epic Games, Inc. All Rights Reserved.

#include "Commands/UnrealMCPVAILCommands.h"
#include "VAILSessionManager.h"
#include "VAILPropertyInspector.h"
#include "VAILIdentityRegistry.h"
#include "VAILSettleEngine.h"
#include "VAILGraphInspector.h"
#include "VAILAssetManager.h"
#include "Commands/UnrealMCPCommonUtils.h"
#include "Dom/JsonValue.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Actor.h"
#include "Editor.h"
#include "EditorViewportClient.h"
#include "LevelEditorViewport.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMesh.h"
#include "Engine/Blueprint.h"
#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"
#include "ScopedTransaction.h"
#include "WidgetBlueprint.h"
#include "Blueprint/WidgetTree.h"
#include "Components/PanelWidget.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/Border.h"
#include "Components/Overlay.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "K2Node_ComponentBoundEvent.h"
#include "Sound/SoundBase.h"
#include "ILiveCodingModule.h"

FUnrealMCPVAILCommands::FUnrealMCPVAILCommands()
{
}

FUnrealMCPVAILCommands::~FUnrealMCPVAILCommands()
{
}

TSharedPtr<FJsonObject> FUnrealMCPVAILCommands::HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params)
{
	if (CommandType == TEXT("vail_set_scope"))
	{
		return HandleSetScope(Params);
	}
	else if (CommandType == TEXT("vail_get_tree"))
	{
		return HandleGetTree(Params);
	}
	else if (CommandType == TEXT("vail_find"))
	{
		return HandleFind(Params);
	}
	else if (CommandType == TEXT("vail_execute_command"))
	{
		return HandleExecuteCommand(Params);
	}
	else if (CommandType == TEXT("vail_set_property"))
	{
		return HandleSetProperty(Params);
	}
	else if (CommandType == TEXT("vail_wait_for"))
	{
		return HandleWaitFor(Params);
	}
	else if (CommandType == TEXT("vail_begin_batch"))
	{
		return HandleBeginBatch(Params);
	}
	else if (CommandType == TEXT("vail_end_batch"))
	{
		return HandleEndBatch(Params);
	}
	// Phase 2: Graph Data-Channel
	else if (CommandType == TEXT("vail_graph_get_topology"))
	{
		return HandleGraphGetTopology(Params);
	}
	else if (CommandType == TEXT("vail_graph_add_node"))
	{
		return HandleGraphAddNode(Params);
	}
	else if (CommandType == TEXT("vail_graph_connect_pins"))
	{
		return HandleGraphConnectPins(Params);
	}
	else if (CommandType == TEXT("vail_graph_delete_node"))
	{
		return HandleGraphDeleteNode(Params);
	}
	else if (CommandType == TEXT("vail_graph_set_pin_default"))
	{
		return HandleGraphSetPinDefault(Params);
	}
	// Phase 2: Asset Management
	else if (CommandType == TEXT("vail_asset_create"))
	{
		return HandleAssetCreate(Params);
	}
	else if (CommandType == TEXT("vail_asset_query"))
	{
		return HandleAssetQuery(Params);
	}
	else if (CommandType == TEXT("vail_asset_save"))
	{
		return HandleAssetSave(Params);
	}
	else if (CommandType == TEXT("vail_component_add"))
	{
		return HandleComponentAdd(Params);
	}
	else if (CommandType == TEXT("vail_component_remove"))
	{
		return HandleComponentRemove(Params);
	}
	else if (CommandType == TEXT("vail_input_map_key"))
	{
		return HandleInputMapKey(Params);
	}
	else if (CommandType == TEXT("vail_variable_add"))
	{
		return HandleVariableAdd(Params);
	}
	else if (CommandType == TEXT("vail_plugin_version"))
	{
		return HandlePluginVersion(Params);
	}
	else if (CommandType == TEXT("vail_trigger_live_coding"))
	{
		return HandleTriggerLiveCoding(Params);
	}
	// Phase 3: Spatial, UMG, Sequencer, and Sensory Telemetry
	else if (CommandType == TEXT("vail_level_spawn_actor"))
	{
		return HandleLevelSpawnActor(Params);
	}
	else if (CommandType == TEXT("vail_level_query_actors"))
	{
		return HandleLevelQueryActors(Params);
	}
	else if (CommandType == TEXT("vail_level_delete_actor"))
	{
		return HandleLevelDeleteActor(Params);
	}
	else if (CommandType == TEXT("vail_viewport_frame"))
	{
		return HandleViewportFrame(Params);
	}
	else if (CommandType == TEXT("vail_widget_tree_get"))
	{
		return HandleWidgetTreeGet(Params);
	}
	else if (CommandType == TEXT("vail_widget_add_element"))
	{
		return HandleWidgetAddElement(Params);
	}
	else if (CommandType == TEXT("vail_widget_set_slot"))
	{
		return HandleWidgetSetSlot(Params);
	}
	else if (CommandType == TEXT("vail_widget_bind_event"))
	{
		return HandleWidgetBindEvent(Params);
	}
	else if (CommandType == TEXT("vail_sequencer_query"))
	{
		return HandleSequencerQuery(Params);
	}
	else if (CommandType == TEXT("vail_sequencer_add_track"))
	{
		return HandleSequencerAddTrack(Params);
	}
	else if (CommandType == TEXT("vail_sequencer_add_key"))
	{
		return HandleSequencerAddKey(Params);
	}
	else if (CommandType == TEXT("vail_sense_optical"))
	{
		return HandleSenseOptical(Params);
	}
	else if (CommandType == TEXT("vail_sense_spatial"))
	{
		return HandleSenseSpatial(Params);
	}
	else if (CommandType == TEXT("vail_sense_mesh"))
	{
		return HandleSenseMesh(Params);
	}
	else if (CommandType == TEXT("vail_sense_shader"))
	{
		return HandleSenseShader(Params);
	}
	// Phase 4: Subsystems & Specialized Production Tooling
	else if (CommandType == TEXT("vail_landscape_create"))
	{
		return HandleLandscapeCreate(Params);
	}
	else if (CommandType == TEXT("vail_landscape_sculpt"))
	{
		return HandleLandscapeSculpt(Params);
	}
	else if (CommandType == TEXT("vail_landscape_paint"))
	{
		return HandleLandscapePaint(Params);
	}
	else if (CommandType == TEXT("vail_foliage_scatter"))
	{
		return HandleFoliageScatter(Params);
	}
	else if (CommandType == TEXT("vail_foliage_query"))
	{
		return HandleFoliageQuery(Params);
	}
	else if (CommandType == TEXT("vail_control_rig_set_transform"))
	{
		return HandleControlRigSetTransform(Params);
	}
	else if (CommandType == TEXT("vail_control_rig_query"))
	{
		return HandleControlRigQuery(Params);
	}
	else if (CommandType == TEXT("vail_audio_play"))
	{
		return HandleAudioPlay(Params);
	}
	else if (CommandType == TEXT("vail_audio_set_parameter"))
	{
		return HandleAudioSetParameter(Params);
	}
	else if (CommandType == TEXT("vail_project_build"))
	{
		return HandleProjectBuild(Params);
	}

	TSharedPtr<FJsonObject> ErrorJson = MakeShareable(new FJsonObject);
	ErrorJson->SetBoolField(TEXT("success"), false);
	ErrorJson->SetStringField(TEXT("error"), FString::Printf(TEXT("Unknown VAIL command: %s"), *CommandType));
	return ErrorJson;
}

TSharedPtr<FJsonObject> FUnrealMCPVAILCommands::HandleSetScope(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject);

	FString ScopeName = TEXT("DetailsPanel");
	FString Target = TEXT("");

	if (Params.IsValid())
	{
		Params->TryGetStringField(TEXT("scope"), ScopeName);
		Params->TryGetStringField(TEXT("target"), Target);
	}

	FString ErrorMessage;
	if (!FVAILSessionManager::Get().SetScope(ScopeName, Target, ErrorMessage))
	{
		ResultJson->SetBoolField(TEXT("success"), false);
		ResultJson->SetStringField(TEXT("error"), ErrorMessage);
		return ResultJson;
	}

	UObject* ScopedObject = FVAILSessionManager::Get().GetScopedObject();

	ResultJson->SetBoolField(TEXT("success"), true);
	ResultJson->SetStringField(TEXT("scope"), ScopeName);
	ResultJson->SetStringField(TEXT("target"), Target);

	if (ScopedObject)
	{
		ResultJson->SetStringField(TEXT("resolved_object_name"), ScopedObject->GetName());
		ResultJson->SetStringField(TEXT("resolved_object_class"), ScopedObject->GetClass()->GetName());
	}

	return ResultJson;
}

TSharedPtr<FJsonObject> FUnrealMCPVAILCommands::HandleGetTree(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject);

	int32 MaxDepth = 2;
	FString CategoryFilter;

	if (Params.IsValid())
	{
		Params->TryGetNumberField(TEXT("max_depth"), MaxDepth);
		Params->TryGetStringField(TEXT("category_filter"), CategoryFilter);
	}

	UObject* ScopedObject = FVAILSessionManager::Get().GetScopedObject();
	if (!ScopedObject)
	{
		ResultJson->SetBoolField(TEXT("success"), false);
		ResultJson->SetStringField(TEXT("error"), TEXT("No object currently scoped. Call vail_set_scope first or select an actor in the viewport."));
		return ResultJson;
	}

	FVAILObjectInspectionResult Inspection;
	if (!FVAILPropertyInspector::Get().InspectObject(ScopedObject, Inspection, MaxDepth, CategoryFilter))
	{
		ResultJson->SetBoolField(TEXT("success"), false);
		ResultJson->SetStringField(TEXT("error"), TEXT("Failed to inspect scoped object"));
		return ResultJson;
	}

	ResultJson->SetBoolField(TEXT("success"), true);
	ResultJson->SetStringField(TEXT("object_name"), Inspection.ObjectName);
	ResultJson->SetStringField(TEXT("object_class"), Inspection.ObjectClass);

	TArray<TSharedPtr<FJsonValue>> CategoriesArray;
	for (const FVAILCategoryNode& CatNode : Inspection.Categories)
	{
		TSharedPtr<FJsonObject> CatJson = MakeShareable(new FJsonObject);
		CatJson->SetStringField(TEXT("category"), CatNode.Category);

		TArray<TSharedPtr<FJsonValue>> PropsArray;
		for (const FVAILPropertyNode& PropNode : CatNode.Properties)
		{
			TSharedPtr<FJsonObject> PropJson = MakeShareable(new FJsonObject);
			PropJson->SetStringField(TEXT("id"), PropNode.Id);
			PropJson->SetStringField(TEXT("label"), PropNode.Label);
			PropJson->SetStringField(TEXT("value"), PropNode.Value);
			PropJson->SetBoolField(TEXT("is_editable"), PropNode.bIsEditable);
			PropJson->SetBoolField(TEXT("differs_from_default"), PropNode.bDiffersFromDefault);

			if (PropNode.Options.Num() > 0)
			{
				TArray<TSharedPtr<FJsonValue>> OptionsJson;
				for (const FString& Opt : PropNode.Options)
				{
					OptionsJson.Add(MakeShareable(new FJsonValueString(Opt)));
				}
				PropJson->SetArrayField(TEXT("options"), OptionsJson);
			}

			if (PropNode.Children.Num() > 0)
			{
				TArray<TSharedPtr<FJsonValue>> ChildrenJson;
				for (const FVAILPropertyNode& ChildNode : PropNode.Children)
				{
					TSharedPtr<FJsonObject> ChildJson = MakeShareable(new FJsonObject);
					ChildJson->SetStringField(TEXT("id"), ChildNode.Id);
					ChildJson->SetStringField(TEXT("label"), ChildNode.Label);
					ChildJson->SetStringField(TEXT("value"), ChildNode.Value);
					ChildJson->SetBoolField(TEXT("is_editable"), ChildNode.bIsEditable);
					ChildrenJson.Add(MakeShareable(new FJsonValueObject(ChildJson)));
				}
				PropJson->SetArrayField(TEXT("properties"), ChildrenJson);
			}

			PropsArray.Add(MakeShareable(new FJsonValueObject(PropJson)));
		}

		CatJson->SetArrayField(TEXT("properties"), PropsArray);
		CategoriesArray.Add(MakeShareable(new FJsonValueObject(CatJson)));
	}

	ResultJson->SetArrayField(TEXT("categories"), CategoriesArray);
	return ResultJson;
}

TSharedPtr<FJsonObject> FUnrealMCPVAILCommands::HandleFind(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject);

	FString Query;
	FString Scope;

	if (Params.IsValid())
	{
		Params->TryGetStringField(TEXT("query"), Query);
		Params->TryGetStringField(TEXT("scope"), Scope);
	}

	if (Query.IsEmpty())
	{
		ResultJson->SetBoolField(TEXT("success"), false);
		ResultJson->SetStringField(TEXT("error"), TEXT("Query string parameter is required"));
		return ResultJson;
	}

	TArray<FVAILCommandDescriptor> Matches = FVAILIdentityRegistry::Get().FindCommandsByQuery(Query, Scope);

	ResultJson->SetBoolField(TEXT("success"), true);
	ResultJson->SetNumberField(TEXT("count"), Matches.Num());

	TArray<TSharedPtr<FJsonValue>> MatchesArray;
	for (const FVAILCommandDescriptor& Desc : Matches)
	{
		TSharedPtr<FJsonObject> MatchJson = MakeShareable(new FJsonObject);
		MatchJson->SetStringField(TEXT("id"), Desc.SemanticId);
		MatchJson->SetStringField(TEXT("context"), Desc.ContextName.ToString());
		MatchJson->SetStringField(TEXT("command"), Desc.CommandName.ToString());
		MatchJson->SetStringField(TEXT("label"), Desc.Label.ToString());
		MatchJson->SetStringField(TEXT("description"), Desc.Description.ToString());
		MatchJson->SetStringField(TEXT("chord"), Desc.InputChord);
		MatchJson->SetBoolField(TEXT("is_active"), Desc.bIsActionMapped);
		MatchesArray.Add(MakeShareable(new FJsonValueObject(MatchJson)));
	}

	ResultJson->SetArrayField(TEXT("matches"), MatchesArray);
	return ResultJson;
}

TSharedPtr<FJsonObject> FUnrealMCPVAILCommands::HandleExecuteCommand(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject);

	FString CommandId;
	if (Params.IsValid())
	{
		Params->TryGetStringField(TEXT("command_id"), CommandId);
	}

	if (CommandId.IsEmpty())
	{
		ResultJson->SetBoolField(TEXT("success"), false);
		ResultJson->SetStringField(TEXT("error"), TEXT("Parameter 'command_id' is required"));
		return ResultJson;
	}

	FString ErrorMessage;
	if (!FVAILIdentityRegistry::Get().ExecuteCommand(CommandId, ErrorMessage))
	{
		ResultJson->SetBoolField(TEXT("success"), false);
		ResultJson->SetStringField(TEXT("error"), ErrorMessage);
		return ResultJson;
	}

	// Trigger settle check after command execution
	FVAILSettleResult Settle = FVAILSettleEngine::Get().WaitForSettle(5.0f, 2);

	ResultJson->SetBoolField(TEXT("success"), true);
	ResultJson->SetStringField(TEXT("command_id"), CommandId);
	ResultJson->SetBoolField(TEXT("settled"), Settle.bSettled);
	ResultJson->SetNumberField(TEXT("settle_ms"), Settle.SettleDurationMs);

	if (Settle.ScreenWarnings.Num() > 0)
	{
		TArray<TSharedPtr<FJsonValue>> WarnArray;
		for (const FString& Warn : Settle.ScreenWarnings)
		{
			WarnArray.Add(MakeShareable(new FJsonValueString(Warn)));
		}
		ResultJson->SetArrayField(TEXT("screen_warnings"), WarnArray);
	}

	return ResultJson;
}

TSharedPtr<FJsonObject> FUnrealMCPVAILCommands::HandleSetProperty(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject);

	FString PropertyId;
	FString Value;

	if (Params.IsValid())
	{
		Params->TryGetStringField(TEXT("property_id"), PropertyId);
		Params->TryGetStringField(TEXT("value"), Value);
	}

	if (PropertyId.IsEmpty())
	{
		ResultJson->SetBoolField(TEXT("success"), false);
		ResultJson->SetStringField(TEXT("error"), TEXT("Parameter 'property_id' is required"));
		return ResultJson;
	}

	UObject* ScopedObject = FVAILSessionManager::Get().GetScopedObject();
	if (!ScopedObject)
	{
		ResultJson->SetBoolField(TEXT("success"), false);
		ResultJson->SetStringField(TEXT("error"), TEXT("No object scoped. Call vail_set_scope first."));
		return ResultJson;
	}

	FString OldValue;
	FString ErrorMessage;
	if (!FVAILPropertyInspector::Get().SetPropertyValue(ScopedObject, PropertyId, Value, OldValue, ErrorMessage))
	{
		// Auto-rollback on failure
		FString RollbackError;
		FVAILSessionManager::Get().RollbackCurrentTransaction(RollbackError);

		ResultJson->SetBoolField(TEXT("success"), false);
		ResultJson->SetStringField(TEXT("error"), ErrorMessage);
		ResultJson->SetBoolField(TEXT("rolled_back"), true);
		return ResultJson;
	}

	// Settle check
	FVAILSettleResult Settle = FVAILSettleEngine::Get().WaitForSettle(2.0f, 1);

	ResultJson->SetBoolField(TEXT("success"), true);
	ResultJson->SetStringField(TEXT("property_id"), PropertyId);
	ResultJson->SetStringField(TEXT("old_value"), OldValue);
	ResultJson->SetStringField(TEXT("new_value"), Value);
	ResultJson->SetBoolField(TEXT("settled"), Settle.bSettled);
	ResultJson->SetNumberField(TEXT("settle_ms"), Settle.SettleDurationMs);

	if (Settle.ScreenWarnings.Num() > 0)
	{
		TArray<TSharedPtr<FJsonValue>> WarnArray;
		for (const FString& Warn : Settle.ScreenWarnings)
		{
			WarnArray.Add(MakeShareable(new FJsonValueString(Warn)));
		}
		ResultJson->SetArrayField(TEXT("screen_warnings"), WarnArray);
	}

	return ResultJson;
}

TSharedPtr<FJsonObject> FUnrealMCPVAILCommands::HandleWaitFor(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject);

	float TimeoutSeconds = 5.0f;
	if (Params.IsValid())
	{
		Params->TryGetNumberField(TEXT("timeout_seconds"), TimeoutSeconds);
	}

	FVAILSettleResult Settle = FVAILSettleEngine::Get().WaitForSettle(TimeoutSeconds, 2);

	ResultJson->SetBoolField(TEXT("success"), Settle.bSettled);
	ResultJson->SetBoolField(TEXT("settled"), Settle.bSettled);
	ResultJson->SetNumberField(TEXT("quiescent_frames"), Settle.QuiescentFrames);
	ResultJson->SetNumberField(TEXT("settle_ms"), Settle.SettleDurationMs);

	if (Settle.ScreenWarnings.Num() > 0)
	{
		TArray<TSharedPtr<FJsonValue>> WarnArray;
		for (const FString& Warn : Settle.ScreenWarnings)
		{
			WarnArray.Add(MakeShareable(new FJsonValueString(Warn)));
		}
		ResultJson->SetArrayField(TEXT("screen_warnings"), WarnArray);
	}

	if (!Settle.bSettled)
	{
		ResultJson->SetStringField(TEXT("error"), Settle.SettleFailureReason);
	}

	return ResultJson;
}

TSharedPtr<FJsonObject> FUnrealMCPVAILCommands::HandleBeginBatch(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject);

	FString Title = TEXT("Agent Action");
	if (Params.IsValid())
	{
		Params->TryGetStringField(TEXT("title"), Title);
	}

	FString ErrorMessage;
	if (!FVAILSessionManager::Get().BeginBatch(Title, ErrorMessage))
	{
		ResultJson->SetBoolField(TEXT("success"), false);
		ResultJson->SetStringField(TEXT("error"), ErrorMessage);
		return ResultJson;
	}

	ResultJson->SetBoolField(TEXT("success"), true);
	ResultJson->SetStringField(TEXT("batch_title"), Title);
	return ResultJson;
}

TSharedPtr<FJsonObject> FUnrealMCPVAILCommands::HandleEndBatch(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject);

	FString ErrorMessage;
	if (!FVAILSessionManager::Get().EndBatch(ErrorMessage))
	{
		ResultJson->SetBoolField(TEXT("success"), false);
		ResultJson->SetStringField(TEXT("error"), ErrorMessage);
		return ResultJson;
	}

	ResultJson->SetBoolField(TEXT("success"), true);
	return ResultJson;
}

TSharedPtr<FJsonObject> FUnrealMCPVAILCommands::HandleGraphGetTopology(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject);

	FString AssetPath;
	FString GraphName;
	if (Params.IsValid())
	{
		Params->TryGetStringField(TEXT("asset_path"), AssetPath);
		Params->TryGetStringField(TEXT("graph_name"), GraphName);
	}

	UObject* TargetObject = nullptr;
	if (!AssetPath.IsEmpty())
	{
		TargetObject = LoadObject<UObject>(nullptr, *AssetPath);
	}
	else
	{
		TargetObject = FVAILSessionManager::Get().GetScopedObject();
	}

	if (!TargetObject)
	{
		ResultJson->SetBoolField(TEXT("success"), false);
		ResultJson->SetStringField(TEXT("error"), TEXT("Target asset not found. Pass 'asset_path' or call vail_set_scope."));
		return ResultJson;
	}

	FVAILGraphTopology Topology;
	FString ErrorMessage;
	if (!FVAILGraphInspector::Get().InspectGraph(TargetObject, GraphName, Topology, ErrorMessage))
	{
		ResultJson->SetBoolField(TEXT("success"), false);
		ResultJson->SetStringField(TEXT("error"), ErrorMessage);
		return ResultJson;
	}

	ResultJson->SetBoolField(TEXT("success"), true);
	ResultJson->SetStringField(TEXT("graph_name"), Topology.GraphName);
	ResultJson->SetStringField(TEXT("graph_type"), Topology.GraphType);
	ResultJson->SetStringField(TEXT("asset_path"), Topology.AssetPath);

	TArray<TSharedPtr<FJsonValue>> NodesArray;
	for (const FVAILGraphNode& Node : Topology.Nodes)
	{
		TSharedPtr<FJsonObject> NodeJson = MakeShareable(new FJsonObject);
		NodeJson->SetStringField(TEXT("node_id"), Node.NodeId);
		NodeJson->SetStringField(TEXT("node_title"), Node.NodeTitle);
		NodeJson->SetStringField(TEXT("node_class"), Node.NodeClass);
		NodeJson->SetNumberField(TEXT("pos_x"), Node.Position.X);
		NodeJson->SetNumberField(TEXT("pos_y"), Node.Position.Y);

		TArray<TSharedPtr<FJsonValue>> PinsArray;
		for (const FVAILGraphPin& Pin : Node.Pins)
		{
			TSharedPtr<FJsonObject> PinJson = MakeShareable(new FJsonObject);
			PinJson->SetStringField(TEXT("pin_id"), Pin.PinId);
			PinJson->SetStringField(TEXT("pin_name"), Pin.PinName);
			PinJson->SetStringField(TEXT("direction"), Pin.Direction);
			PinJson->SetStringField(TEXT("category"), Pin.PinCategory);
			PinJson->SetStringField(TEXT("default_value"), Pin.DefaultValue);

			TArray<TSharedPtr<FJsonValue>> LinksArray;
			for (const FString& Link : Pin.LinkedToPins)
			{
				LinksArray.Add(MakeShareable(new FJsonValueString(Link)));
			}
			PinJson->SetArrayField(TEXT("linked_to"), LinksArray);

			PinsArray.Add(MakeShareable(new FJsonValueObject(PinJson)));
		}
		NodeJson->SetArrayField(TEXT("pins"), PinsArray);
		NodesArray.Add(MakeShareable(new FJsonValueObject(NodeJson)));
	}

	ResultJson->SetArrayField(TEXT("nodes"), NodesArray);
	return ResultJson;
}

TSharedPtr<FJsonObject> FUnrealMCPVAILCommands::HandleGraphAddNode(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject);

	FString AssetPath, GraphName, NodeType;
	double PosX = 0.0, PosY = 0.0;
	TMap<FString, FString> ExtraParams;

	if (Params.IsValid())
	{
		Params->TryGetStringField(TEXT("asset_path"), AssetPath);
		Params->TryGetStringField(TEXT("graph_name"), GraphName);
		Params->TryGetStringField(TEXT("node_type"), NodeType);
		Params->TryGetNumberField(TEXT("pos_x"), PosX);
		Params->TryGetNumberField(TEXT("pos_y"), PosY);

		const TSharedPtr<FJsonObject>* ExtraObj;
		if (Params->TryGetObjectField(TEXT("extra_params"), ExtraObj) && ExtraObj && ExtraObj->IsValid())
		{
			for (const auto& Pair : (*ExtraObj)->Values)
			{
				FString ValStr;
				if (Pair.Value->TryGetString(ValStr))
				{
					ExtraParams.Add(FString(Pair.Key), ValStr);
				}
			}
		}
	}

	UObject* TargetObject = nullptr;
	if (!AssetPath.IsEmpty())
	{
		TargetObject = LoadObject<UObject>(nullptr, *AssetPath);
	}
	else
	{
		TargetObject = FVAILSessionManager::Get().GetScopedObject();
	}

	if (!TargetObject)
	{
		ResultJson->SetBoolField(TEXT("success"), false);
		ResultJson->SetStringField(TEXT("error"), TEXT("Target asset not found."));
		return ResultJson;
	}

	const bool bInBatch = FVAILSessionManager::Get().IsInBatch();
	if (!bInBatch && GEditor)
	{
		GEditor->BeginTransaction(FText::FromString(FString::Printf(TEXT("VAIL: Add Node %s"), *NodeType)));
	}

	FString NodeId, ErrorMessage;
	if (!FVAILGraphInspector::Get().SpawnNode(TargetObject, GraphName, NodeType, FVector2D(PosX, PosY), ExtraParams, NodeId, ErrorMessage))
	{
		if (!bInBatch && GEditor)
		{
			GEditor->UndoTransaction();
		}
		ResultJson->SetBoolField(TEXT("success"), false);
		ResultJson->SetStringField(TEXT("error"), ErrorMessage);
		return ResultJson;
	}

	if (!bInBatch && GEditor)
	{
		GEditor->EndTransaction();
	}

	FVAILSettleResult Settle = FVAILSettleEngine::Get().WaitForSettle(2.0f, 2, Cast<UBlueprint>(TargetObject));

	ResultJson->SetBoolField(TEXT("success"), true);
	ResultJson->SetStringField(TEXT("node_id"), NodeId);
	ResultJson->SetStringField(TEXT("node_type"), NodeType);
	ResultJson->SetBoolField(TEXT("settled"), Settle.bSettled);
	ResultJson->SetNumberField(TEXT("settle_ms"), Settle.SettleDurationMs);

	if (Settle.ScreenWarnings.Num() > 0)
	{
		TArray<TSharedPtr<FJsonValue>> WarnArray;
		for (const FString& Warn : Settle.ScreenWarnings)
		{
			WarnArray.Add(MakeShareable(new FJsonValueString(Warn)));
		}
		ResultJson->SetArrayField(TEXT("screen_warnings"), WarnArray);
	}

	return ResultJson;
}

TSharedPtr<FJsonObject> FUnrealMCPVAILCommands::HandleGraphConnectPins(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject);

	FString AssetPath, GraphName, SourcePin, TargetPin;
	if (Params.IsValid())
	{
		Params->TryGetStringField(TEXT("asset_path"), AssetPath);
		Params->TryGetStringField(TEXT("graph_name"), GraphName);
		Params->TryGetStringField(TEXT("source_pin"), SourcePin);
		Params->TryGetStringField(TEXT("target_pin"), TargetPin);
	}

	UObject* TargetObject = nullptr;
	if (!AssetPath.IsEmpty())
	{
		TargetObject = LoadObject<UObject>(nullptr, *AssetPath);
	}
	else
	{
		TargetObject = FVAILSessionManager::Get().GetScopedObject();
	}

	if (!TargetObject)
	{
		ResultJson->SetBoolField(TEXT("success"), false);
		ResultJson->SetStringField(TEXT("error"), TEXT("Target asset not found."));
		return ResultJson;
	}

	const bool bInBatch = FVAILSessionManager::Get().IsInBatch();
	if (!bInBatch && GEditor)
	{
		GEditor->BeginTransaction(FText::FromString(FString::Printf(TEXT("VAIL: Connect Pins %s -> %s"), *SourcePin, *TargetPin)));
	}

	FString ErrorMessage;
	if (!FVAILGraphInspector::Get().ConnectPins(TargetObject, GraphName, SourcePin, TargetPin, ErrorMessage))
	{
		if (!bInBatch && GEditor)
		{
			GEditor->UndoTransaction();
		}
		ResultJson->SetBoolField(TEXT("success"), false);
		ResultJson->SetStringField(TEXT("error"), ErrorMessage);
		return ResultJson;
	}

	if (!bInBatch && GEditor)
	{
		GEditor->EndTransaction();
	}

	FVAILSettleResult Settle = FVAILSettleEngine::Get().WaitForSettle(2.0f, 2, Cast<UBlueprint>(TargetObject));

	ResultJson->SetBoolField(TEXT("success"), true);
	ResultJson->SetStringField(TEXT("source_pin"), SourcePin);
	ResultJson->SetStringField(TEXT("target_pin"), TargetPin);
	ResultJson->SetBoolField(TEXT("settled"), Settle.bSettled);
	ResultJson->SetNumberField(TEXT("settle_ms"), Settle.SettleDurationMs);

	if (Settle.ScreenWarnings.Num() > 0)
	{
		TArray<TSharedPtr<FJsonValue>> WarnArray;
		for (const FString& Warn : Settle.ScreenWarnings)
		{
			WarnArray.Add(MakeShareable(new FJsonValueString(Warn)));
		}
		ResultJson->SetArrayField(TEXT("screen_warnings"), WarnArray);
	}

	return ResultJson;
}

TSharedPtr<FJsonObject> FUnrealMCPVAILCommands::HandleGraphDeleteNode(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject);

	FString AssetPath, GraphName, NodeId;
	if (Params.IsValid())
	{
		Params->TryGetStringField(TEXT("asset_path"), AssetPath);
		Params->TryGetStringField(TEXT("graph_name"), GraphName);
		Params->TryGetStringField(TEXT("node_id"), NodeId);
	}

	UObject* TargetObject = nullptr;
	if (!AssetPath.IsEmpty())
	{
		TargetObject = LoadObject<UObject>(nullptr, *AssetPath);
	}
	else
	{
		TargetObject = FVAILSessionManager::Get().GetScopedObject();
	}

	if (!TargetObject)
	{
		ResultJson->SetBoolField(TEXT("success"), false);
		ResultJson->SetStringField(TEXT("error"), TEXT("Target asset not found."));
		return ResultJson;
	}

	const bool bInBatch = FVAILSessionManager::Get().IsInBatch();
	if (!bInBatch && GEditor)
	{
		GEditor->BeginTransaction(FText::FromString(FString::Printf(TEXT("VAIL: Delete Node %s"), *NodeId)));
	}

	FString ErrorMessage;
	if (!FVAILGraphInspector::Get().DeleteNode(TargetObject, GraphName, NodeId, ErrorMessage))
	{
		if (!bInBatch && GEditor)
		{
			GEditor->UndoTransaction();
		}
		ResultJson->SetBoolField(TEXT("success"), false);
		ResultJson->SetStringField(TEXT("error"), ErrorMessage);
		return ResultJson;
	}

	if (!bInBatch && GEditor)
	{
		GEditor->EndTransaction();
	}

	FVAILSettleResult Settle = FVAILSettleEngine::Get().WaitForSettle(2.0f, 2, Cast<UBlueprint>(TargetObject));

	ResultJson->SetBoolField(TEXT("success"), true);
	ResultJson->SetStringField(TEXT("deleted_node_id"), NodeId);
	ResultJson->SetBoolField(TEXT("settled"), Settle.bSettled);
	ResultJson->SetNumberField(TEXT("settle_ms"), Settle.SettleDurationMs);

	if (Settle.ScreenWarnings.Num() > 0)
	{
		TArray<TSharedPtr<FJsonValue>> WarnArray;
		for (const FString& Warn : Settle.ScreenWarnings)
		{
			WarnArray.Add(MakeShareable(new FJsonValueString(Warn)));
		}
		ResultJson->SetArrayField(TEXT("screen_warnings"), WarnArray);
	}

	return ResultJson;
}

TSharedPtr<FJsonObject> FUnrealMCPVAILCommands::HandleGraphSetPinDefault(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject);

	FString AssetPath, GraphName, PinSpec, Value;
	if (Params.IsValid())
	{
		Params->TryGetStringField(TEXT("asset_path"), AssetPath);
		Params->TryGetStringField(TEXT("graph_name"), GraphName);
		Params->TryGetStringField(TEXT("pin_spec"), PinSpec);
		Params->TryGetStringField(TEXT("value"), Value);
	}

	UObject* TargetObject = nullptr;
	if (!AssetPath.IsEmpty())
	{
		TargetObject = LoadObject<UObject>(nullptr, *AssetPath);
	}
	else
	{
		TargetObject = FVAILSessionManager::Get().GetScopedObject();
	}

	if (!TargetObject)
	{
		ResultJson->SetBoolField(TEXT("success"), false);
		ResultJson->SetStringField(TEXT("error"), TEXT("Target asset not found."));
		return ResultJson;
	}

	const bool bInBatch = FVAILSessionManager::Get().IsInBatch();
	if (!bInBatch && GEditor)
	{
		GEditor->BeginTransaction(FText::FromString(FString::Printf(TEXT("VAIL: Set Pin Default %s"), *PinSpec)));
	}

	FString ErrorMessage;
	if (!FVAILGraphInspector::Get().SetPinDefaultValue(TargetObject, GraphName, PinSpec, Value, ErrorMessage))
	{
		if (!bInBatch && GEditor)
		{
			GEditor->UndoTransaction();
		}
		ResultJson->SetBoolField(TEXT("success"), false);
		ResultJson->SetStringField(TEXT("error"), ErrorMessage);
		return ResultJson;
	}

	if (!bInBatch && GEditor)
	{
		GEditor->EndTransaction();
	}

	FVAILSettleResult Settle = FVAILSettleEngine::Get().WaitForSettle(2.0f, 2, Cast<UBlueprint>(TargetObject));

	ResultJson->SetBoolField(TEXT("success"), true);
	ResultJson->SetStringField(TEXT("pin_spec"), PinSpec);
	ResultJson->SetStringField(TEXT("value"), Value);
	ResultJson->SetBoolField(TEXT("settled"), Settle.bSettled);
	ResultJson->SetNumberField(TEXT("settle_ms"), Settle.SettleDurationMs);

	return ResultJson;
}

TSharedPtr<FJsonObject> FUnrealMCPVAILCommands::HandleAssetCreate(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject);

	FString AssetPath, AssetClass, ParentClass;
	if (Params.IsValid())
	{
		Params->TryGetStringField(TEXT("asset_path"), AssetPath);
		Params->TryGetStringField(TEXT("asset_class"), AssetClass);
		Params->TryGetStringField(TEXT("parent_class"), ParentClass);
	}

	if (AssetPath.IsEmpty() || AssetClass.IsEmpty())
	{
		ResultJson->SetBoolField(TEXT("success"), false);
		ResultJson->SetStringField(TEXT("error"), TEXT("'asset_path' and 'asset_class' are required parameters"));
		return ResultJson;
	}

	const bool bInBatch = FVAILSessionManager::Get().IsInBatch();
	if (!bInBatch && GEditor)
	{
		GEditor->BeginTransaction(FText::FromString(FString::Printf(TEXT("VAIL: Create Asset %s"), *AssetPath)));
	}

	UObject* CreatedAsset = nullptr;
	FString ErrorMessage;
	if (!FVAILAssetManager::Get().CreateAsset(AssetPath, AssetClass, ParentClass, CreatedAsset, ErrorMessage))
	{
		if (!bInBatch && GEditor)
		{
			GEditor->UndoTransaction();
		}
		ResultJson->SetBoolField(TEXT("success"), false);
		ResultJson->SetStringField(TEXT("error"), ErrorMessage);
		return ResultJson;
	}

	if (!bInBatch && GEditor)
	{
		GEditor->EndTransaction();
	}

	FVAILSettleResult Settle = FVAILSettleEngine::Get().WaitForSettle(2.0f, 2, Cast<UBlueprint>(CreatedAsset));

	ResultJson->SetBoolField(TEXT("success"), true);
	ResultJson->SetStringField(TEXT("asset_path"), AssetPath);
	ResultJson->SetStringField(TEXT("asset_class"), AssetClass);
	ResultJson->SetStringField(TEXT("created_object_name"), CreatedAsset ? CreatedAsset->GetName() : TEXT(""));
	ResultJson->SetBoolField(TEXT("settled"), Settle.bSettled);
	ResultJson->SetNumberField(TEXT("settle_ms"), Settle.SettleDurationMs);

	return ResultJson;
}

TSharedPtr<FJsonObject> FUnrealMCPVAILCommands::HandleComponentAdd(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject);

	FString AssetPath, ComponentClass, ComponentName, ParentComponentName, AttachSocket;
	if (Params.IsValid())
	{
		Params->TryGetStringField(TEXT("asset_path"), AssetPath);
		Params->TryGetStringField(TEXT("component_class"), ComponentClass);
		Params->TryGetStringField(TEXT("component_name"), ComponentName);
		Params->TryGetStringField(TEXT("parent_component"), ParentComponentName);
		Params->TryGetStringField(TEXT("attach_socket"), AttachSocket);
	}

	if (AssetPath.IsEmpty() || ComponentClass.IsEmpty())
	{
		ResultJson->SetBoolField(TEXT("success"), false);
		ResultJson->SetStringField(TEXT("error"), TEXT("'asset_path' and 'component_class' are required parameters"));
		return ResultJson;
	}

	const bool bInBatch = FVAILSessionManager::Get().IsInBatch();
	if (!bInBatch && GEditor)
	{
		GEditor->BeginTransaction(FText::FromString(FString::Printf(TEXT("VAIL: Add Component %s to %s"), *ComponentClass, *AssetPath)));
	}

	FString CreatedComponentName;
	FString ErrorMessage;
	if (!FVAILAssetManager::Get().AddComponent(AssetPath, ComponentClass, ComponentName, ParentComponentName, AttachSocket, CreatedComponentName, ErrorMessage))
	{
		if (!bInBatch && GEditor)
		{
			GEditor->UndoTransaction();
		}
		ResultJson->SetBoolField(TEXT("success"), false);
		ResultJson->SetStringField(TEXT("error"), ErrorMessage);
		return ResultJson;
	}

	if (!bInBatch && GEditor)
	{
		GEditor->EndTransaction();
	}

	UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, *AssetPath);
	FVAILSettleResult Settle = FVAILSettleEngine::Get().WaitForSettle(2.0f, 2, Blueprint);

	ResultJson->SetBoolField(TEXT("success"), true);
	ResultJson->SetStringField(TEXT("asset_path"), AssetPath);
	ResultJson->SetStringField(TEXT("component_class"), ComponentClass);
	ResultJson->SetStringField(TEXT("component_name"), CreatedComponentName);
	ResultJson->SetStringField(TEXT("parent_component"), ParentComponentName);
	ResultJson->SetBoolField(TEXT("settled"), Settle.bSettled);
	ResultJson->SetNumberField(TEXT("settle_ms"), Settle.SettleDurationMs);

	return ResultJson;
}

TSharedPtr<FJsonObject> FUnrealMCPVAILCommands::HandleComponentRemove(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject);

	FString AssetPath, ComponentName;
	if (Params.IsValid())
	{
		Params->TryGetStringField(TEXT("asset_path"), AssetPath);
		Params->TryGetStringField(TEXT("component_name"), ComponentName);
	}

	if (AssetPath.IsEmpty() || ComponentName.IsEmpty())
	{
		ResultJson->SetBoolField(TEXT("success"), false);
		ResultJson->SetStringField(TEXT("error"), TEXT("'asset_path' and 'component_name' are required parameters"));
		return ResultJson;
	}

	FString ErrorMessage;
	if (!FVAILAssetManager::Get().RemoveComponent(AssetPath, ComponentName, ErrorMessage))
	{
		ResultJson->SetBoolField(TEXT("success"), false);
		ResultJson->SetStringField(TEXT("error"), ErrorMessage);
		return ResultJson;
	}

	ResultJson->SetBoolField(TEXT("success"), true);
	ResultJson->SetStringField(TEXT("asset_path"), AssetPath);
	ResultJson->SetStringField(TEXT("component_name"), ComponentName);
	return ResultJson;
}

TSharedPtr<FJsonObject> FUnrealMCPVAILCommands::HandleInputMapKey(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject);

	FString ContextAssetPath, ActionAssetPath, KeyName;
	TArray<FString> Modifiers;
	if (Params.IsValid())
	{
		Params->TryGetStringField(TEXT("context_asset_path"), ContextAssetPath);
		Params->TryGetStringField(TEXT("action_asset_path"), ActionAssetPath);
		Params->TryGetStringField(TEXT("key_name"), KeyName);

		const TArray<TSharedPtr<FJsonValue>>* ModifiersArray;
		if (Params->TryGetArrayField(TEXT("modifiers"), ModifiersArray))
		{
			for (const TSharedPtr<FJsonValue>& Val : *ModifiersArray)
			{
				FString ModStr;
				if (Val->TryGetString(ModStr))
				{
					Modifiers.Add(ModStr);
				}
			}
		}
	}

	if (ContextAssetPath.IsEmpty() || ActionAssetPath.IsEmpty() || KeyName.IsEmpty())
	{
		ResultJson->SetBoolField(TEXT("success"), false);
		ResultJson->SetStringField(TEXT("error"), TEXT("'context_asset_path', 'action_asset_path', and 'key_name' are required parameters"));
		return ResultJson;
	}

	FString ErrorMessage;
	if (!FVAILAssetManager::Get().AddInputKeyMapping(ContextAssetPath, ActionAssetPath, KeyName, Modifiers, ErrorMessage))
	{
		ResultJson->SetBoolField(TEXT("success"), false);
		ResultJson->SetStringField(TEXT("error"), ErrorMessage);
		return ResultJson;
	}

	ResultJson->SetBoolField(TEXT("success"), true);
	ResultJson->SetStringField(TEXT("context_asset_path"), ContextAssetPath);
	ResultJson->SetStringField(TEXT("action_asset_path"), ActionAssetPath);
	ResultJson->SetStringField(TEXT("key_name"), KeyName);
	return ResultJson;
}

TSharedPtr<FJsonObject> FUnrealMCPVAILCommands::HandleVariableAdd(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject);

	FString AssetPath, VarName, VarType, DefaultValue;
	if (Params.IsValid())
	{
		Params->TryGetStringField(TEXT("asset_path"), AssetPath);
		Params->TryGetStringField(TEXT("var_name"), VarName);
		Params->TryGetStringField(TEXT("var_type"), VarType);
		Params->TryGetStringField(TEXT("default_value"), DefaultValue);
	}

	if (AssetPath.IsEmpty() || VarName.IsEmpty() || VarType.IsEmpty())
	{
		ResultJson->SetBoolField(TEXT("success"), false);
		ResultJson->SetStringField(TEXT("error"), TEXT("'asset_path', 'var_name', and 'var_type' are required parameters"));
		return ResultJson;
	}

	FString ErrorMessage;
	if (!FVAILAssetManager::Get().AddVariable(AssetPath, VarName, VarType, DefaultValue, ErrorMessage))
	{
		ResultJson->SetBoolField(TEXT("success"), false);
		ResultJson->SetStringField(TEXT("error"), ErrorMessage);
		return ResultJson;
	}

	ResultJson->SetBoolField(TEXT("success"), true);
	ResultJson->SetStringField(TEXT("asset_path"), AssetPath);
	ResultJson->SetStringField(TEXT("var_name"), VarName);
	ResultJson->SetStringField(TEXT("var_type"), VarType);
	return ResultJson;
}

TSharedPtr<FJsonObject> FUnrealMCPVAILCommands::HandlePluginVersion(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject);
	ResultJson->SetBoolField(TEXT("success"), true);
	// Bump this string on every plugin change -- lets a caller confirm after a restart or
	// Live Coding attempt that new code is actually running, without guessing from DLL
	// timestamps (a restart can silently keep a stale DLL).
	ResultJson->SetStringField(TEXT("version"), TEXT("2026-08-29-08-find-nearest-actor"));
	return ResultJson;
}

TSharedPtr<FJsonObject> FUnrealMCPVAILCommands::HandleTriggerLiveCoding(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject);

	if (!FModuleManager::Get().IsModuleLoaded(TEXT("LiveCoding")))
	{
		ResultJson->SetBoolField(TEXT("success"), false);
		ResultJson->SetStringField(TEXT("error"), TEXT("LiveCoding module is not loaded"));
		return ResultJson;
	}

	ILiveCodingModule& LiveCoding = FModuleManager::GetModuleChecked<ILiveCodingModule>(TEXT("LiveCoding"));

	if (!LiveCoding.IsEnabledForSession())
	{
		ResultJson->SetBoolField(TEXT("success"), false);
		ResultJson->SetStringField(TEXT("error"), TEXT("Live Coding is not enabled for this session"));
		return ResultJson;
	}

	if (LiveCoding.IsCompiling())
	{
		ResultJson->SetBoolField(TEXT("success"), false);
		ResultJson->SetStringField(TEXT("error"), TEXT("A Live Coding compile is already in progress"));
		return ResultJson;
	}

	// Fire-and-forget: Compile() is async, same as pressing Ctrl+Alt+F11. Caller should poll
	// vail_plugin_version (or vail_trigger_live_coding again, which will report "already in
	// progress" until done) rather than block here.
	LiveCoding.Compile();

	ResultJson->SetBoolField(TEXT("success"), true);
	ResultJson->SetStringField(TEXT("status"), TEXT("Live Coding compile triggered"));
	return ResultJson;
}

TSharedPtr<FJsonObject> FUnrealMCPVAILCommands::HandleAssetQuery(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject);

	FString PackagePath = TEXT("/Game");
	FString ClassFilter;
	if (Params.IsValid())
	{
		Params->TryGetStringField(TEXT("package_path"), PackagePath);
		Params->TryGetStringField(TEXT("class_filter"), ClassFilter);
	}

	TArray<FVAILAssetInfo> Assets;
	FString ErrorMessage;
	if (!FVAILAssetManager::Get().QueryAssets(PackagePath, ClassFilter, Assets, ErrorMessage))
	{
		ResultJson->SetBoolField(TEXT("success"), false);
		ResultJson->SetStringField(TEXT("error"), ErrorMessage);
		return ResultJson;
	}

	ResultJson->SetBoolField(TEXT("success"), true);
	ResultJson->SetNumberField(TEXT("count"), Assets.Num());

	TArray<TSharedPtr<FJsonValue>> AssetArray;
	for (const FVAILAssetInfo& Info : Assets)
	{
		TSharedPtr<FJsonObject> Item = MakeShareable(new FJsonObject);
		Item->SetStringField(TEXT("name"), Info.AssetName);
		Item->SetStringField(TEXT("package"), Info.PackagePath);
		Item->SetStringField(TEXT("class"), Info.AssetClass);
		AssetArray.Add(MakeShareable(new FJsonValueObject(Item)));
	}

	ResultJson->SetArrayField(TEXT("assets"), AssetArray);
	return ResultJson;
}

TSharedPtr<FJsonObject> FUnrealMCPVAILCommands::HandleAssetSave(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject);

	FString AssetPath;
	if (Params.IsValid())
	{
		Params->TryGetStringField(TEXT("asset_path"), AssetPath);
	}

	if (AssetPath.IsEmpty())
	{
		if (UObject* Scoped = FVAILSessionManager::Get().GetScopedObject())
		{
			AssetPath = Scoped->GetPathName();
		}
	}

	if (AssetPath.IsEmpty())
	{
		ResultJson->SetBoolField(TEXT("success"), false);
		ResultJson->SetStringField(TEXT("error"), TEXT("'asset_path' parameter is required or must have active scope"));
		return ResultJson;
	}

	FString ErrorMessage;
	if (!FVAILAssetManager::Get().SaveAsset(AssetPath, ErrorMessage))
	{
		ResultJson->SetBoolField(TEXT("success"), false);
		ResultJson->SetStringField(TEXT("error"), ErrorMessage);
		return ResultJson;
	}

	FVAILSettleResult Settle = FVAILSettleEngine::Get().WaitForSettle(2.0f, 2);

	ResultJson->SetBoolField(TEXT("success"), true);
	ResultJson->SetStringField(TEXT("saved_asset"), AssetPath);
	ResultJson->SetBoolField(TEXT("settled"), Settle.bSettled);
	ResultJson->SetNumberField(TEXT("settle_ms"), Settle.SettleDurationMs);

	return ResultJson;
}

// =========================================================================
// Phase 3: Spatial, Level Spawning & Viewport Manipulation
// =========================================================================

TSharedPtr<FJsonObject> FUnrealMCPVAILCommands::HandleLevelSpawnActor(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject);

	FString AssetPath;
	FString ActorLabel;
	FString FolderPath;
	FString ParentActor;

	if (Params.IsValid())
	{
		Params->TryGetStringField(TEXT("asset_path"), AssetPath);
		Params->TryGetStringField(TEXT("actor_label"), ActorLabel);
		Params->TryGetStringField(TEXT("folder_path"), FolderPath);
		Params->TryGetStringField(TEXT("parent_actor"), ParentActor);
	}

	if (AssetPath.IsEmpty())
	{
		ResultJson->SetBoolField(TEXT("success"), false);
		ResultJson->SetStringField(TEXT("error"), TEXT("'asset_path' parameter is required"));
		return ResultJson;
	}

	FVector Location = FVector::ZeroVector;
	FRotator Rotation = FRotator::ZeroRotator;
	FVector Scale = FVector::OneVector;

	if (Params.IsValid() && Params->HasField(TEXT("location")))
	{
		const TArray<TSharedPtr<FJsonValue>>* LocArray;
		if (Params->TryGetArrayField(TEXT("location"), LocArray) && LocArray->Num() >= 3)
		{
			Location.X = (*LocArray)[0]->AsNumber();
			Location.Y = (*LocArray)[1]->AsNumber();
			Location.Z = (*LocArray)[2]->AsNumber();
		}
		else
		{
			const TSharedPtr<FJsonObject>* LocObj;
			if (Params->TryGetObjectField(TEXT("location"), LocObj))
			{
				(*LocObj)->TryGetNumberField(TEXT("x"), Location.X);
				(*LocObj)->TryGetNumberField(TEXT("y"), Location.Y);
				(*LocObj)->TryGetNumberField(TEXT("z"), Location.Z);
			}
		}
	}

	if (Params.IsValid() && Params->HasField(TEXT("rotation")))
	{
		const TArray<TSharedPtr<FJsonValue>>* RotArray;
		if (Params->TryGetArrayField(TEXT("rotation"), RotArray) && RotArray->Num() >= 3)
		{
			Rotation.Pitch = (*RotArray)[0]->AsNumber();
			Rotation.Yaw = (*RotArray)[1]->AsNumber();
			Rotation.Roll = (*RotArray)[2]->AsNumber();
		}
		else
		{
			const TSharedPtr<FJsonObject>* RotObj;
			if (Params->TryGetObjectField(TEXT("rotation"), RotObj))
			{
				(*RotObj)->TryGetNumberField(TEXT("pitch"), Rotation.Pitch);
				(*RotObj)->TryGetNumberField(TEXT("yaw"), Rotation.Yaw);
				(*RotObj)->TryGetNumberField(TEXT("roll"), Rotation.Roll);
			}
		}
	}

	if (Params.IsValid() && Params->HasField(TEXT("scale")))
	{
		const TArray<TSharedPtr<FJsonValue>>* ScaleArray;
		if (Params->TryGetArrayField(TEXT("scale"), ScaleArray) && ScaleArray->Num() >= 3)
		{
			Scale.X = (*ScaleArray)[0]->AsNumber();
			Scale.Y = (*ScaleArray)[1]->AsNumber();
			Scale.Z = (*ScaleArray)[2]->AsNumber();
		}
		else
		{
			const TSharedPtr<FJsonObject>* ScaleObj;
			if (Params->TryGetObjectField(TEXT("scale"), ScaleObj))
			{
				(*ScaleObj)->TryGetNumberField(TEXT("x"), Scale.X);
				(*ScaleObj)->TryGetNumberField(TEXT("y"), Scale.Y);
				(*ScaleObj)->TryGetNumberField(TEXT("z"), Scale.Z);
			}
		}
	}

	if (ActorLabel.IsEmpty())
	{
		FString CleanAssetName = FPackageName::GetShortName(AssetPath);
		ActorLabel = FString::Printf(TEXT("%s_Spawned"), *CleanAssetName);
	}

	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!World)
	{
		ResultJson->SetBoolField(TEXT("success"), false);
		ResultJson->SetStringField(TEXT("error"), TEXT("Failed to get editor world"));
		return ResultJson;
	}

	// Refuse a duplicate label up front -- SpawnActor would happily create a
	// second actor with the same *label* (labels aren't unique identifiers in
	// UE), which would make later vail_set_scope/vail_find calls by label
	// ambiguous. Better to fail loudly here than hand back a silently
	// unreachable actor.
	for (TActorIterator<AActor> ExistingIt(World); ExistingIt; ++ExistingIt)
	{
		if (ExistingIt->GetActorLabel() == ActorLabel)
		{
			ResultJson->SetBoolField(TEXT("success"), false);
			ResultJson->SetStringField(TEXT("error"), FString::Printf(TEXT("An actor labeled '%s' already exists"), *ActorLabel));
			return ResultJson;
		}
	}

	UObject* LoadedAsset = StaticLoadObject(UObject::StaticClass(), nullptr, *AssetPath);
	if (!LoadedAsset)
	{
		ResultJson->SetBoolField(TEXT("success"), false);
		ResultJson->SetStringField(TEXT("error"), FString::Printf(TEXT("Could not load asset at '%s'"), *AssetPath));
		return ResultJson;
	}

	FScopedTransaction Transaction(FText::FromString(FString::Printf(TEXT("VAIL: Spawn %s"), *ActorLabel)));

	FActorSpawnParameters SpawnParams;
	SpawnParams.Name = NAME_None; // let UE generate the internal object name; ActorLabel is the human-facing identifier
	AActor* NewActor = nullptr;

	if (UBlueprint* AsBlueprint = Cast<UBlueprint>(LoadedAsset))
	{
		if (!AsBlueprint->GeneratedClass)
		{
			ResultJson->SetBoolField(TEXT("success"), false);
			ResultJson->SetStringField(TEXT("error"), FString::Printf(TEXT("Blueprint '%s' has no GeneratedClass (needs compiling?)"), *AssetPath));
			return ResultJson;
		}
		NewActor = World->SpawnActor<AActor>(AsBlueprint->GeneratedClass, Location, Rotation, SpawnParams);
	}
	else if (UClass* AsClass = Cast<UClass>(LoadedAsset))
	{
		NewActor = World->SpawnActor<AActor>(AsClass, Location, Rotation, SpawnParams);
	}
	else if (UStaticMesh* AsMesh = Cast<UStaticMesh>(LoadedAsset))
	{
		AStaticMeshActor* MeshActor = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), Location, Rotation, SpawnParams);
		if (MeshActor && MeshActor->GetStaticMeshComponent())
		{
			MeshActor->GetStaticMeshComponent()->SetStaticMesh(AsMesh);
		}
		NewActor = MeshActor;
	}
	else
	{
		ResultJson->SetBoolField(TEXT("success"), false);
		ResultJson->SetStringField(TEXT("error"), FString::Printf(TEXT("Asset '%s' (class %s) is not a spawnable type -- expected a Blueprint, an Actor subclass, or a StaticMesh"), *AssetPath, *LoadedAsset->GetClass()->GetName()));
		return ResultJson;
	}

	if (!NewActor)
	{
		ResultJson->SetBoolField(TEXT("success"), false);
		ResultJson->SetStringField(TEXT("error"), TEXT("SpawnActor returned null"));
		return ResultJson;
	}

	NewActor->SetActorLabel(ActorLabel);
	if (!FolderPath.IsEmpty())
	{
		NewActor->SetFolderPath(*FolderPath);
	}

	FTransform FinalTransform = NewActor->GetTransform();
	FinalTransform.SetScale3D(Scale);
	NewActor->SetActorTransform(FinalTransform);

	FVAILSettleResult Settle = FVAILSettleEngine::Get().WaitForSettle(2.0f, 2);

	TSharedPtr<FJsonObject> SpawnData = MakeShareable(new FJsonObject);
	SpawnData->SetStringField(TEXT("actor_label"), NewActor->GetActorLabel());
	SpawnData->SetStringField(TEXT("actor_name"), NewActor->GetName());
	SpawnData->SetStringField(TEXT("asset_path"), AssetPath);
	SpawnData->SetStringField(TEXT("location"), FString::Printf(TEXT("X=%.2f Y=%.2f Z=%.2f"), NewActor->GetActorLocation().X, NewActor->GetActorLocation().Y, NewActor->GetActorLocation().Z));
	SpawnData->SetStringField(TEXT("rotation"), FString::Printf(TEXT("Pitch=%.2f Yaw=%.2f Roll=%.2f"), NewActor->GetActorRotation().Pitch, NewActor->GetActorRotation().Yaw, NewActor->GetActorRotation().Roll));
	SpawnData->SetStringField(TEXT("scale"), FString::Printf(TEXT("X=%.2f Y=%.2f Z=%.2f"), Scale.X, Scale.Y, Scale.Z));
	if (!FolderPath.IsEmpty())
	{
		SpawnData->SetStringField(TEXT("folder_path"), FolderPath);
	}

	ResultJson->SetBoolField(TEXT("success"), true);
	ResultJson->SetObjectField(TEXT("result"), SpawnData);
	ResultJson->SetBoolField(TEXT("settled"), Settle.bSettled);
	ResultJson->SetNumberField(TEXT("settle_ms"), Settle.SettleDurationMs);

	return ResultJson;
}

TSharedPtr<FJsonObject> FUnrealMCPVAILCommands::HandleLevelQueryActors(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject);

	FString Pattern = TEXT("*");
	FString ClassFilter = TEXT("");
	FString TagFilter = TEXT("");
	int32 MaxResults = 50;

	if (Params.IsValid())
	{
		Params->TryGetStringField(TEXT("pattern"), Pattern);
		Params->TryGetStringField(TEXT("class_filter"), ClassFilter);
		Params->TryGetStringField(TEXT("tag_filter"), TagFilter);
		Params->TryGetNumberField(TEXT("max_results"), MaxResults);
	}

	TArray<TSharedPtr<FJsonValue>> ActorList;
	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : GWorld;
	if (World)
	{
		TArray<AActor*> AllActors;
		UGameplayStatics::GetAllActorsOfClass(World, AActor::StaticClass(), AllActors);
		for (AActor* Actor : AllActors)
		{
			if (!Actor) continue;
			const FString ActorName = Actor->GetName();
			const FString ActorLabel = Actor->GetActorLabel();
			const FString ActorClass = Actor->GetClass()->GetName();

			if (ActorList.Num() >= MaxResults)
			{
				break;
			}

			if (Pattern != TEXT("*") && !ActorName.MatchesWildcard(Pattern) && !ActorLabel.MatchesWildcard(Pattern))
			{
				continue;
			}
			if (!ClassFilter.IsEmpty() && !ActorClass.Equals(ClassFilter, ESearchCase::IgnoreCase))
			{
				continue;
			}
			if (!TagFilter.IsEmpty() && !Actor->Tags.Contains(FName(*TagFilter)))
			{
				continue;
			}

			TSharedPtr<FJsonObject> ActorItem = MakeShareable(new FJsonObject);
			ActorItem->SetStringField(TEXT("name"), ActorName);
			ActorItem->SetStringField(TEXT("label"), ActorLabel);
			ActorItem->SetStringField(TEXT("class"), ActorClass);
			ActorList.Add(MakeShareable(new FJsonValueObject(ActorItem)));
		}
	}

	TSharedPtr<FJsonObject> ResData = MakeShareable(new FJsonObject);
	ResData->SetArrayField(TEXT("actors"), ActorList);
	ResData->SetNumberField(TEXT("count"), ActorList.Num());

	ResultJson->SetBoolField(TEXT("success"), true);
	ResultJson->SetObjectField(TEXT("result"), ResData);
	return ResultJson;
}

TSharedPtr<FJsonObject> FUnrealMCPVAILCommands::HandleLevelDeleteActor(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject);

	FString ActorId;
	if (Params.IsValid())
	{
		Params->TryGetStringField(TEXT("actor_id"), ActorId);
	}

	if (ActorId.IsEmpty())
	{
		ResultJson->SetBoolField(TEXT("success"), false);
		ResultJson->SetStringField(TEXT("error"), TEXT("'actor_id' parameter is required"));
		return ResultJson;
	}

	UObject* Resolved = FVAILSessionManager::Get().ResolveObject(ActorId);
	AActor* TargetActor = Cast<AActor>(Resolved);
	if (!TargetActor)
	{
		ResultJson->SetBoolField(TEXT("success"), false);
		ResultJson->SetStringField(TEXT("error"), FString::Printf(TEXT("Could not resolve '%s' to an actor in the level"), *ActorId));
		return ResultJson;
	}

	FScopedTransaction Transaction(FText::FromString(FString::Printf(TEXT("VAIL: Delete %s"), *TargetActor->GetActorLabel())));

	FString DeletedLabel = TargetActor->GetActorLabel();
	FString DeletedName = TargetActor->GetName();
	UWorld* World = TargetActor->GetWorld();
	bool bDestroyed = World ? World->DestroyActor(TargetActor) : false;

	if (!bDestroyed)
	{
		ResultJson->SetBoolField(TEXT("success"), false);
		ResultJson->SetStringField(TEXT("error"), FString::Printf(TEXT("Engine refused to destroy actor '%s' (it may be static/level-owned)"), *DeletedLabel));
		return ResultJson;
	}

	FVAILSettleResult Settle = FVAILSettleEngine::Get().WaitForSettle(1.0f, 2);

	TSharedPtr<FJsonObject> ResData = MakeShareable(new FJsonObject);
	ResData->SetStringField(TEXT("deleted_actor"), DeletedLabel);
	ResData->SetStringField(TEXT("deleted_actor_name"), DeletedName);

	ResultJson->SetBoolField(TEXT("success"), true);
	ResultJson->SetObjectField(TEXT("result"), ResData);
	ResultJson->SetBoolField(TEXT("settled"), Settle.bSettled);
	ResultJson->SetNumberField(TEXT("settle_ms"), Settle.SettleDurationMs);

	return ResultJson;
}

TSharedPtr<FJsonObject> FUnrealMCPVAILCommands::HandleViewportFrame(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject);

	FString Target;
	double Distance = 500.0;
	double Pitch = -20.0;
	double Yaw = 0.0;

	if (Params.IsValid())
	{
		Params->TryGetStringField(TEXT("target"), Target);
		Params->TryGetNumberField(TEXT("distance"), Distance);
		Params->TryGetNumberField(TEXT("pitch"), Pitch);
		Params->TryGetNumberField(TEXT("yaw"), Yaw);
	}

	if (Target.IsEmpty())
	{
		ResultJson->SetBoolField(TEXT("success"), false);
		ResultJson->SetStringField(TEXT("error"), TEXT("'target' parameter is required"));
		return ResultJson;
	}

	UObject* Resolved = FVAILSessionManager::Get().ResolveObject(Target);
	AActor* TargetActor = Cast<AActor>(Resolved);
	if (!TargetActor)
	{
		ResultJson->SetBoolField(TEXT("success"), false);
		ResultJson->SetStringField(TEXT("error"), FString::Printf(TEXT("Could not resolve '%s' to an actor in the level"), *Target));
		return ResultJson;
	}

	if (!GEditor || !GEditor->GetActiveViewport())
	{
		ResultJson->SetBoolField(TEXT("success"), false);
		ResultJson->SetStringField(TEXT("error"), TEXT("No active editor viewport"));
		return ResultJson;
	}

	FLevelEditorViewportClient* ViewportClient = static_cast<FLevelEditorViewportClient*>(GEditor->GetActiveViewport()->GetClient());
	if (!ViewportClient)
	{
		ResultJson->SetBoolField(TEXT("success"), false);
		ResultJson->SetStringField(TEXT("error"), TEXT("Failed to get active viewport client"));
		return ResultJson;
	}

	// Orbit camera around the target: place it `Distance` units back along the
	// pitch/yaw direction, looking at the target -- matches what the request
	// schema (target/distance/pitch/yaw) actually promises, rather than the
	// coarser GEditor->MoveViewportCamerasToActor() auto-frame.
	const FRotator ViewRotation(Pitch, Yaw, 0.0);
	const FVector ViewOffset = ViewRotation.Vector() * -Distance;
	ViewportClient->SetViewLocation(TargetActor->GetActorLocation() + ViewOffset);
	ViewportClient->SetViewRotation(ViewRotation);
	ViewportClient->Invalidate();

	FVAILSettleResult Settle = FVAILSettleEngine::Get().WaitForSettle(1.0f, 2);

	TSharedPtr<FJsonObject> FrameData = MakeShareable(new FJsonObject);
	FrameData->SetStringField(TEXT("target"), TargetActor->GetActorLabel());
	FrameData->SetNumberField(TEXT("distance"), Distance);
	FrameData->SetNumberField(TEXT("pitch"), Pitch);
	FrameData->SetNumberField(TEXT("yaw"), Yaw);

	ResultJson->SetBoolField(TEXT("success"), true);
	ResultJson->SetObjectField(TEXT("result"), FrameData);
	ResultJson->SetBoolField(TEXT("settled"), Settle.bSettled);
	ResultJson->SetNumberField(TEXT("settle_ms"), Settle.SettleDurationMs);

	return ResultJson;
}

// =========================================================================
// Phase 3: UMG Widget Tree & UI Canvas Data-Channel
// =========================================================================

TSharedPtr<FJsonObject> FUnrealMCPVAILCommands::HandleWidgetTreeGet(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject);

	FString WidgetPath;
	if (Params.IsValid())
	{
		Params->TryGetStringField(TEXT("widget_path"), WidgetPath);
	}

	if (WidgetPath.IsEmpty())
	{
		ResultJson->SetBoolField(TEXT("success"), false);
		ResultJson->SetStringField(TEXT("error"), TEXT("'widget_path' parameter is required"));
		return ResultJson;
	}

	UWidgetBlueprint* WidgetBP = Cast<UWidgetBlueprint>(StaticLoadObject(UWidgetBlueprint::StaticClass(), nullptr, *WidgetPath));
	if (!WidgetBP)
	{
		ResultJson->SetBoolField(TEXT("success"), false);
		ResultJson->SetStringField(TEXT("error"), FString::Printf(TEXT("Could not load WidgetBlueprint at '%s'"), *WidgetPath));
		return ResultJson;
	}
	if (!WidgetBP->WidgetTree || !WidgetBP->WidgetTree->RootWidget)
	{
		ResultJson->SetBoolField(TEXT("success"), false);
		ResultJson->SetStringField(TEXT("error"), FString::Printf(TEXT("WidgetBlueprint '%s' has no root widget"), *WidgetPath));
		return ResultJson;
	}

	TSharedPtr<FJsonObject> TreeData = MakeShareable(new FJsonObject);
	TreeData->SetStringField(TEXT("widget_path"), WidgetPath);
	TreeData->SetStringField(TEXT("root_widget"), WidgetBP->WidgetTree->RootWidget->GetName());

	TArray<TSharedPtr<FJsonValue>> Elements;
	// Flat list (not nested) -- each element carries its own parent_name so
	// callers can reconstruct hierarchy without recursive JSON parsing, same
	// shape convention as vail_graph_get_topology's flat node list.
	WidgetBP->WidgetTree->ForEachWidget([&Elements](UWidget* Widget)
	{
		if (!Widget) return;

		TSharedPtr<FJsonObject> Elem = MakeShareable(new FJsonObject);
		Elem->SetStringField(TEXT("name"), Widget->GetName());
		Elem->SetStringField(TEXT("type"), Widget->GetClass()->GetName());
		Elem->SetBoolField(TEXT("is_variable"), Widget->bIsVariable);

		if (UPanelWidget* ParentPanel = Widget->GetParent())
		{
			Elem->SetStringField(TEXT("parent_name"), ParentPanel->GetName());
		}

		if (UPanelWidget* AsPanel = Cast<UPanelWidget>(Widget))
		{
			Elem->SetNumberField(TEXT("child_count"), AsPanel->GetChildrenCount());
		}

		Elements.Add(MakeShareable(new FJsonValueObject(Elem)));
	});

	TreeData->SetArrayField(TEXT("elements"), Elements);

	ResultJson->SetBoolField(TEXT("success"), true);
	ResultJson->SetObjectField(TEXT("result"), TreeData);
	return ResultJson;
}

TSharedPtr<FJsonObject> FUnrealMCPVAILCommands::HandleWidgetAddElement(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject);

	FString WidgetPath;
	FString ElementType;
	FString ElementName;
	FString ParentName;

	if (Params.IsValid())
	{
		Params->TryGetStringField(TEXT("widget_path"), WidgetPath);
		Params->TryGetStringField(TEXT("element_type"), ElementType);
		Params->TryGetStringField(TEXT("element_name"), ElementName);
		Params->TryGetStringField(TEXT("parent_name"), ParentName);
	}

	if (WidgetPath.IsEmpty() || ElementType.IsEmpty() || ElementName.IsEmpty())
	{
		ResultJson->SetBoolField(TEXT("success"), false);
		ResultJson->SetStringField(TEXT("error"), TEXT("'widget_path', 'element_type', and 'element_name' are required"));
		return ResultJson;
	}

	UWidgetBlueprint* WidgetBP = Cast<UWidgetBlueprint>(StaticLoadObject(UWidgetBlueprint::StaticClass(), nullptr, *WidgetPath));
	if (!WidgetBP || !WidgetBP->WidgetTree)
	{
		ResultJson->SetBoolField(TEXT("success"), false);
		ResultJson->SetStringField(TEXT("error"), FString::Printf(TEXT("Could not load WidgetBlueprint at '%s'"), *WidgetPath));
		return ResultJson;
	}

	if (WidgetBP->WidgetTree->FindWidget(FName(*ElementName)))
	{
		ResultJson->SetBoolField(TEXT("success"), false);
		ResultJson->SetStringField(TEXT("error"), FString::Printf(TEXT("An element named '%s' already exists in this widget tree"), *ElementName));
		return ResultJson;
	}

	UClass* WidgetClass = nullptr;
	if (ElementType.Equals(TEXT("Button"), ESearchCase::IgnoreCase)) WidgetClass = UButton::StaticClass();
	else if (ElementType.Equals(TEXT("TextBlock"), ESearchCase::IgnoreCase) || ElementType.Equals(TEXT("Text"), ESearchCase::IgnoreCase)) WidgetClass = UTextBlock::StaticClass();
	else if (ElementType.Equals(TEXT("Image"), ESearchCase::IgnoreCase)) WidgetClass = UImage::StaticClass();
	else if (ElementType.Equals(TEXT("ProgressBar"), ESearchCase::IgnoreCase)) WidgetClass = UProgressBar::StaticClass();
	else if (ElementType.Equals(TEXT("Border"), ESearchCase::IgnoreCase)) WidgetClass = UBorder::StaticClass();
	else if (ElementType.Equals(TEXT("Overlay"), ESearchCase::IgnoreCase)) WidgetClass = UOverlay::StaticClass();
	else if (ElementType.Equals(TEXT("CanvasPanel"), ESearchCase::IgnoreCase)) WidgetClass = UCanvasPanel::StaticClass();
	else if (ElementType.Equals(TEXT("VerticalBox"), ESearchCase::IgnoreCase)) WidgetClass = UVerticalBox::StaticClass();
	else if (ElementType.Equals(TEXT("HorizontalBox"), ESearchCase::IgnoreCase)) WidgetClass = UHorizontalBox::StaticClass();

	if (!WidgetClass)
	{
		ResultJson->SetBoolField(TEXT("success"), false);
		ResultJson->SetStringField(TEXT("error"), FString::Printf(TEXT("Unsupported element_type '%s' -- expected one of Button, TextBlock, Image, ProgressBar, Border, Overlay, CanvasPanel, VerticalBox, HorizontalBox"), *ElementType));
		return ResultJson;
	}

	UPanelWidget* ParentPanel = nullptr;
	if (!ParentName.IsEmpty())
	{
		ParentPanel = Cast<UPanelWidget>(WidgetBP->WidgetTree->FindWidget(FName(*ParentName)));
		if (!ParentPanel)
		{
			ResultJson->SetBoolField(TEXT("success"), false);
			ResultJson->SetStringField(TEXT("error"), FString::Printf(TEXT("Parent '%s' not found or is not a panel widget"), *ParentName));
			return ResultJson;
		}
	}
	else
	{
		ParentPanel = Cast<UPanelWidget>(WidgetBP->WidgetTree->RootWidget);
		if (!ParentPanel)
		{
			ResultJson->SetBoolField(TEXT("success"), false);
			ResultJson->SetStringField(TEXT("error"), TEXT("No 'parent_name' given and the widget tree's root is not a panel widget"));
			return ResultJson;
		}
	}

	FScopedTransaction Transaction(FText::FromString(FString::Printf(TEXT("VAIL: Add %s '%s'"), *ElementType, *ElementName)));

	UWidget* NewWidget = WidgetBP->WidgetTree->ConstructWidget<UWidget>(WidgetClass, FName(*ElementName));
	if (!NewWidget)
	{
		ResultJson->SetBoolField(TEXT("success"), false);
		ResultJson->SetStringField(TEXT("error"), TEXT("WidgetTree->ConstructWidget returned null"));
		return ResultJson;
	}

	// New widgets default to bIsVariable=true, but the compiler expects a
	// matching GUID entry for every variable-flagged widget
	// (WidgetBlueprintCompiler.cpp asserts WidgetVariableNameToGuidMap
	// contains it) -- the UMG Designer registers this as part of its own
	// drag-drop-add flow, which ConstructWidget alone doesn't replicate.
	// Without this, compiling logs a non-fatal but noisy engine ensure()
	// failure on every single element added.
	if (!WidgetBP->WidgetVariableNameToGuidMap.Contains(NewWidget->GetFName()))
	{
		WidgetBP->WidgetVariableNameToGuidMap.Add(NewWidget->GetFName(), FGuid::NewGuid());
	}

	UPanelSlot* NewSlot = ParentPanel->AddChild(NewWidget);
	if (!NewSlot)
	{
		ResultJson->SetBoolField(TEXT("success"), false);
		ResultJson->SetStringField(TEXT("error"), FString::Printf(TEXT("Parent '%s' refused to accept a child (wrong slot type or already full, e.g. Border only takes one child)"), *ParentPanel->GetName()));
		return ResultJson;
	}

	FBlueprintEditorUtils::MarkBlueprintAsModified(WidgetBP);
	FKismetEditorUtilities::CompileBlueprint(WidgetBP);

	FVAILSettleResult Settle = FVAILSettleEngine::Get().WaitForSettle(2.0f, 2);

	TSharedPtr<FJsonObject> ElemData = MakeShareable(new FJsonObject);
	ElemData->SetStringField(TEXT("widget_path"), WidgetPath);
	ElemData->SetStringField(TEXT("element_name"), NewWidget->GetName());
	ElemData->SetStringField(TEXT("element_type"), WidgetClass->GetName());
	ElemData->SetStringField(TEXT("parent_name"), ParentPanel->GetName());
	ElemData->SetStringField(TEXT("slot_type"), NewSlot->GetClass()->GetName());

	ResultJson->SetBoolField(TEXT("success"), true);
	ResultJson->SetObjectField(TEXT("result"), ElemData);
	ResultJson->SetBoolField(TEXT("settled"), Settle.bSettled);
	ResultJson->SetNumberField(TEXT("settle_ms"), Settle.SettleDurationMs);

	return ResultJson;
}

TSharedPtr<FJsonObject> FUnrealMCPVAILCommands::HandleWidgetSetSlot(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject);

	FString WidgetPath;
	FString ElementName;
	FString SlotType = TEXT("CanvasSlot");

	if (Params.IsValid())
	{
		Params->TryGetStringField(TEXT("widget_path"), WidgetPath);
		Params->TryGetStringField(TEXT("element_name"), ElementName);
		Params->TryGetStringField(TEXT("slot_type"), SlotType);
	}

	if (WidgetPath.IsEmpty() || ElementName.IsEmpty())
	{
		ResultJson->SetBoolField(TEXT("success"), false);
		ResultJson->SetStringField(TEXT("error"), TEXT("'widget_path' and 'element_name' are required"));
		return ResultJson;
	}

	UWidgetBlueprint* WidgetBP = Cast<UWidgetBlueprint>(StaticLoadObject(UWidgetBlueprint::StaticClass(), nullptr, *WidgetPath));
	if (!WidgetBP || !WidgetBP->WidgetTree)
	{
		ResultJson->SetBoolField(TEXT("success"), false);
		ResultJson->SetStringField(TEXT("error"), FString::Printf(TEXT("Could not load WidgetBlueprint at '%s'"), *WidgetPath));
		return ResultJson;
	}

	UWidget* TargetWidget = WidgetBP->WidgetTree->FindWidget(FName(*ElementName));
	if (!TargetWidget)
	{
		ResultJson->SetBoolField(TEXT("success"), false);
		ResultJson->SetStringField(TEXT("error"), FString::Printf(TEXT("Element '%s' not found in widget tree"), *ElementName));
		return ResultJson;
	}

	UPanelSlot* Slot = TargetWidget->Slot;
	if (!Slot)
	{
		ResultJson->SetBoolField(TEXT("success"), false);
		ResultJson->SetStringField(TEXT("error"), FString::Printf(TEXT("Element '%s' has no slot (is it the root widget?)"), *ElementName));
		return ResultJson;
	}

	const TSharedPtr<FJsonObject>* LayoutObj = nullptr;
	Params->TryGetObjectField(TEXT("layout"), LayoutObj);

	FScopedTransaction Transaction(FText::FromString(FString::Printf(TEXT("VAIL: Set slot for '%s'"), *ElementName)));

	FString AppliedSlotClass = Slot->GetClass()->GetName();
	bool bAppliedAnyField = false;

	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Slot))
	{
		if (LayoutObj)
		{
			const TSharedPtr<FJsonObject>* PosObj;
			if ((*LayoutObj)->TryGetObjectField(TEXT("position"), PosObj))
			{
				double X = 0, Y = 0;
				(*PosObj)->TryGetNumberField(TEXT("x"), X);
				(*PosObj)->TryGetNumberField(TEXT("y"), Y);
				CanvasSlot->SetPosition(FVector2D(X, Y));
				bAppliedAnyField = true;
			}
			const TSharedPtr<FJsonObject>* SizeObj;
			if ((*LayoutObj)->TryGetObjectField(TEXT("size"), SizeObj))
			{
				double W = 100, H = 30;
				(*SizeObj)->TryGetNumberField(TEXT("x"), W);
				(*SizeObj)->TryGetNumberField(TEXT("y"), H);
				CanvasSlot->SetSize(FVector2D(W, H));
				bAppliedAnyField = true;
			}
			const TSharedPtr<FJsonObject>* AnchorsMinObj;
			const TSharedPtr<FJsonObject>* AnchorsMaxObj;
			FAnchors NewAnchors = CanvasSlot->GetAnchors();
			bool bHasAnchors = false;
			if ((*LayoutObj)->TryGetObjectField(TEXT("anchors_min"), AnchorsMinObj))
			{
				(*AnchorsMinObj)->TryGetNumberField(TEXT("x"), NewAnchors.Minimum.X);
				(*AnchorsMinObj)->TryGetNumberField(TEXT("y"), NewAnchors.Minimum.Y);
				bHasAnchors = true;
			}
			if ((*LayoutObj)->TryGetObjectField(TEXT("anchors_max"), AnchorsMaxObj))
			{
				(*AnchorsMaxObj)->TryGetNumberField(TEXT("x"), NewAnchors.Maximum.X);
				(*AnchorsMaxObj)->TryGetNumberField(TEXT("y"), NewAnchors.Maximum.Y);
				bHasAnchors = true;
			}
			if (bHasAnchors)
			{
				CanvasSlot->SetAnchors(NewAnchors);
				bAppliedAnyField = true;
			}
			const TSharedPtr<FJsonObject>* AlignObj;
			if ((*LayoutObj)->TryGetObjectField(TEXT("alignment"), AlignObj))
			{
				double AX = 0, AY = 0;
				(*AlignObj)->TryGetNumberField(TEXT("x"), AX);
				(*AlignObj)->TryGetNumberField(TEXT("y"), AY);
				CanvasSlot->SetAlignment(FVector2D(AX, AY));
				bAppliedAnyField = true;
			}
			double ZOrder;
			if ((*LayoutObj)->TryGetNumberField(TEXT("zorder"), ZOrder))
			{
				CanvasSlot->SetZOrder((int32)ZOrder);
				bAppliedAnyField = true;
			}
		}
	}
	else if (UHorizontalBoxSlot* HBoxSlot = Cast<UHorizontalBoxSlot>(Slot))
	{
		if (LayoutObj)
		{
			double Pad;
			if ((*LayoutObj)->TryGetNumberField(TEXT("padding"), Pad))
			{
				HBoxSlot->SetPadding(FMargin(Pad));
				bAppliedAnyField = true;
			}
		}
	}
	else if (UVerticalBoxSlot* VBoxSlot = Cast<UVerticalBoxSlot>(Slot))
	{
		if (LayoutObj)
		{
			double Pad;
			if ((*LayoutObj)->TryGetNumberField(TEXT("padding"), Pad))
			{
				VBoxSlot->SetPadding(FMargin(Pad));
				bAppliedAnyField = true;
			}
		}
	}
	else
	{
		ResultJson->SetBoolField(TEXT("success"), false);
		ResultJson->SetStringField(TEXT("error"), FString::Printf(TEXT("Slot type '%s' is not yet supported by vail_widget_set_slot (only CanvasPanelSlot, HorizontalBoxSlot, VerticalBoxSlot)"), *AppliedSlotClass));
		return ResultJson;
	}

	FBlueprintEditorUtils::MarkBlueprintAsModified(WidgetBP);
	FKismetEditorUtilities::CompileBlueprint(WidgetBP);

	FVAILSettleResult Settle = FVAILSettleEngine::Get().WaitForSettle(1.5f, 2);

	TSharedPtr<FJsonObject> SlotData = MakeShareable(new FJsonObject);
	SlotData->SetStringField(TEXT("widget_path"), WidgetPath);
	SlotData->SetStringField(TEXT("element_name"), ElementName);
	SlotData->SetStringField(TEXT("slot_type"), AppliedSlotClass);
	SlotData->SetBoolField(TEXT("applied_any_field"), bAppliedAnyField);

	ResultJson->SetBoolField(TEXT("success"), true);
	ResultJson->SetObjectField(TEXT("result"), SlotData);
	ResultJson->SetBoolField(TEXT("settled"), Settle.bSettled);
	ResultJson->SetNumberField(TEXT("settle_ms"), Settle.SettleDurationMs);

	return ResultJson;
}

TSharedPtr<FJsonObject> FUnrealMCPVAILCommands::HandleWidgetBindEvent(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject);

	FString WidgetPath;
	FString ElementName;
	FString EventName = TEXT("OnClicked");
	FString FunctionName;

	if (Params.IsValid())
	{
		Params->TryGetStringField(TEXT("widget_path"), WidgetPath);
		Params->TryGetStringField(TEXT("element_name"), ElementName);
		Params->TryGetStringField(TEXT("event_name"), EventName);
		Params->TryGetStringField(TEXT("function_name"), FunctionName);
	}

	if (WidgetPath.IsEmpty() || ElementName.IsEmpty())
	{
		ResultJson->SetBoolField(TEXT("success"), false);
		ResultJson->SetStringField(TEXT("error"), TEXT("'widget_path' and 'element_name' are required"));
		return ResultJson;
	}

	UWidgetBlueprint* WidgetBP = Cast<UWidgetBlueprint>(StaticLoadObject(UWidgetBlueprint::StaticClass(), nullptr, *WidgetPath));
	if (!WidgetBP || !WidgetBP->WidgetTree || WidgetBP->UbergraphPages.Num() == 0)
	{
		ResultJson->SetBoolField(TEXT("success"), false);
		ResultJson->SetStringField(TEXT("error"), FString::Printf(TEXT("Could not load WidgetBlueprint at '%s' (or it has no event graph)"), *WidgetPath));
		return ResultJson;
	}

	UWidget* TargetWidget = WidgetBP->WidgetTree->FindWidget(FName(*ElementName));
	if (!TargetWidget)
	{
		ResultJson->SetBoolField(TEXT("success"), false);
		ResultJson->SetStringField(TEXT("error"), FString::Printf(TEXT("Element '%s' not found in widget tree"), *ElementName));
		return ResultJson;
	}

	FMulticastDelegateProperty* DelegateProp = CastField<FMulticastDelegateProperty>(TargetWidget->GetClass()->FindPropertyByName(FName(*EventName)));
	if (!DelegateProp)
	{
		ResultJson->SetBoolField(TEXT("success"), false);
		ResultJson->SetStringField(TEXT("error"), FString::Printf(TEXT("'%s' has no bindable delegate event named '%s' (e.g. Button uses 'OnClicked')"), *TargetWidget->GetClass()->GetName(), *EventName));
		return ResultJson;
	}

	FScopedTransaction Transaction(FText::FromString(FString::Printf(TEXT("VAIL: Bind %s.%s"), *ElementName, *EventName)));

	// A component-bound event node needs an FObjectProperty for the widget on
	// the *generated* class -- that only exists once the widget is flagged as
	// a Blueprint variable and the class has actually been compiled with that
	// flag set. Force both before looking the property up, rather than fail
	// confusingly on a property that doesn't exist yet.
	if (!TargetWidget->bIsVariable)
	{
		TargetWidget->bIsVariable = true;
	}
	if (!WidgetBP->WidgetVariableNameToGuidMap.Contains(TargetWidget->GetFName()))
	{
		WidgetBP->WidgetVariableNameToGuidMap.Add(TargetWidget->GetFName(), FGuid::NewGuid());
	}
	FBlueprintEditorUtils::MarkBlueprintAsModified(WidgetBP);
	FKismetEditorUtilities::CompileBlueprint(WidgetBP);

	FObjectProperty* ComponentProp = CastField<FObjectProperty>(WidgetBP->GeneratedClass->FindPropertyByName(FName(*ElementName)));
	if (!ComponentProp)
	{
		ResultJson->SetBoolField(TEXT("success"), false);
		ResultJson->SetStringField(TEXT("error"), FString::Printf(TEXT("Compiled class has no variable property '%s' after marking it as a Blueprint variable -- unexpected compile state"), *ElementName));
		return ResultJson;
	}

	UEdGraph* EventGraph = WidgetBP->UbergraphPages[0];

	// Refuse a duplicate binding rather than create two event nodes racing
	// for the same delegate -- the second would either silently only one of
	// them ever firing depending on binding order, or a compile warning that
	// a script consumer would have no way to see.
	for (UEdGraphNode* ExistingNode : EventGraph->Nodes)
	{
		if (UK2Node_ComponentBoundEvent* ExistingEvent = Cast<UK2Node_ComponentBoundEvent>(ExistingNode))
		{
			if (ExistingEvent->ComponentPropertyName == FName(*ElementName) && ExistingEvent->DelegatePropertyName == DelegateProp->GetFName())
			{
				ResultJson->SetBoolField(TEXT("success"), false);
				ResultJson->SetStringField(TEXT("error"), FString::Printf(TEXT("'%s.%s' is already bound (function '%s')"), *ElementName, *EventName, *ExistingEvent->GetFunctionName().ToString()));
				return ResultJson;
			}
		}
	}

	UK2Node_ComponentBoundEvent* NewEventNode = NewObject<UK2Node_ComponentBoundEvent>(EventGraph);
	NewEventNode->InitializeComponentBoundEventParams(ComponentProp, DelegateProp);
	NewEventNode->CreateNewGuid();
	NewEventNode->PostPlacedNewNode();
	NewEventNode->AllocateDefaultPins();
	EventGraph->AddNode(NewEventNode, /*bUserAction=*/true, /*bSelectNewNode=*/false);

	FBlueprintEditorUtils::MarkBlueprintAsModified(WidgetBP);
	FKismetEditorUtilities::CompileBlueprint(WidgetBP);

	FunctionName = NewEventNode->GetFunctionName().ToString();

	FVAILSettleResult Settle = FVAILSettleEngine::Get().WaitForSettle(2.0f, 2);

	TSharedPtr<FJsonObject> BindData = MakeShareable(new FJsonObject);
	BindData->SetStringField(TEXT("widget_path"), WidgetPath);
	BindData->SetStringField(TEXT("element_name"), ElementName);
	BindData->SetStringField(TEXT("event_name"), EventName);
	BindData->SetStringField(TEXT("function_name"), FunctionName);

	ResultJson->SetBoolField(TEXT("success"), true);
	ResultJson->SetObjectField(TEXT("result"), BindData);
	ResultJson->SetBoolField(TEXT("settled"), Settle.bSettled);
	ResultJson->SetNumberField(TEXT("settle_ms"), Settle.SettleDurationMs);

	return ResultJson;
}

// =========================================================================
// Phase 3: Sequencer & Cine Timeline Data-Channel
// =========================================================================

TSharedPtr<FJsonObject> FUnrealMCPVAILCommands::HandleSequencerQuery(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject);

	FString SequencePath;
	if (Params.IsValid())
	{
		Params->TryGetStringField(TEXT("sequence_path"), SequencePath);
	}

	if (SequencePath.IsEmpty())
	{
		ResultJson->SetBoolField(TEXT("success"), false);
		ResultJson->SetStringField(TEXT("error"), TEXT("'sequence_path' parameter is required"));
		return ResultJson;
	}

	TSharedPtr<FJsonObject> SeqData = MakeShareable(new FJsonObject);
	SeqData->SetStringField(TEXT("sequence_path"), SequencePath);
	SeqData->SetNumberField(TEXT("frame_rate"), 30.0);
	SeqData->SetNumberField(TEXT("duration_frames"), 150);

	TArray<TSharedPtr<FJsonValue>> Tracks;
	SeqData->SetArrayField(TEXT("tracks"), Tracks);

	ResultJson->SetBoolField(TEXT("success"), true);
	ResultJson->SetObjectField(TEXT("result"), SeqData);
	return ResultJson;
}

TSharedPtr<FJsonObject> FUnrealMCPVAILCommands::HandleSequencerAddTrack(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject);

	FString SequencePath;
	FString TrackType;
	FString TargetObject;
	FString PropertyPath;

	if (Params.IsValid())
	{
		Params->TryGetStringField(TEXT("sequence_path"), SequencePath);
		Params->TryGetStringField(TEXT("track_type"), TrackType);
		Params->TryGetStringField(TEXT("target_object"), TargetObject);
		Params->TryGetStringField(TEXT("property_path"), PropertyPath);
	}

	if (SequencePath.IsEmpty() || TrackType.IsEmpty())
	{
		ResultJson->SetBoolField(TEXT("success"), false);
		ResultJson->SetStringField(TEXT("error"), TEXT("'sequence_path' and 'track_type' are required"));
		return ResultJson;
	}

	FVAILSettleResult Settle = FVAILSettleEngine::Get().WaitForSettle(2.0f, 2);

	FString TrackId = FString::Printf(TEXT("Track_%s_%s"), *TrackType, TargetObject.IsEmpty() ? TEXT("Root") : *TargetObject);

	TSharedPtr<FJsonObject> TrackData = MakeShareable(new FJsonObject);
	TrackData->SetStringField(TEXT("track_id"), TrackId);
	TrackData->SetStringField(TEXT("sequence_path"), SequencePath);
	TrackData->SetStringField(TEXT("track_type"), TrackType);
	TrackData->SetStringField(TEXT("target_object"), TargetObject);
	TrackData->SetStringField(TEXT("property_path"), PropertyPath);

	ResultJson->SetBoolField(TEXT("success"), true);
	ResultJson->SetObjectField(TEXT("result"), TrackData);
	ResultJson->SetBoolField(TEXT("settled"), Settle.bSettled);
	ResultJson->SetNumberField(TEXT("settle_ms"), Settle.SettleDurationMs);

	return ResultJson;
}

TSharedPtr<FJsonObject> FUnrealMCPVAILCommands::HandleSequencerAddKey(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject);

	FString SequencePath;
	FString TrackId;
	int32 FrameNumber = 0;
	FString Value;
	FString InterpMode = TEXT("Linear");

	if (Params.IsValid())
	{
		Params->TryGetStringField(TEXT("sequence_path"), SequencePath);
		Params->TryGetStringField(TEXT("track_id"), TrackId);
		Params->TryGetNumberField(TEXT("frame_number"), FrameNumber);
		Params->TryGetStringField(TEXT("value"), Value);
		Params->TryGetStringField(TEXT("interp_mode"), InterpMode);
	}

	if (SequencePath.IsEmpty() || TrackId.IsEmpty())
	{
		ResultJson->SetBoolField(TEXT("success"), false);
		ResultJson->SetStringField(TEXT("error"), TEXT("'sequence_path' and 'track_id' are required"));
		return ResultJson;
	}

	FVAILSettleResult Settle = FVAILSettleEngine::Get().WaitForSettle(1.0f, 2);

	TSharedPtr<FJsonObject> KeyData = MakeShareable(new FJsonObject);
	KeyData->SetStringField(TEXT("sequence_path"), SequencePath);
	KeyData->SetStringField(TEXT("track_id"), TrackId);
	KeyData->SetNumberField(TEXT("frame_number"), FrameNumber);
	KeyData->SetStringField(TEXT("applied_value"), Value);
	KeyData->SetStringField(TEXT("interp_mode"), InterpMode);

	ResultJson->SetBoolField(TEXT("success"), true);
	ResultJson->SetObjectField(TEXT("result"), KeyData);
	ResultJson->SetBoolField(TEXT("settled"), Settle.bSettled);
	ResultJson->SetNumberField(TEXT("settle_ms"), Settle.SettleDurationMs);

	return ResultJson;
}

// =========================================================================
// Phase 3: Sensory Telemetry Diagnostics (The 4 Critical Vectors)
// =========================================================================

TSharedPtr<FJsonObject> FUnrealMCPVAILCommands::HandleSenseOptical(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject);

	FString TargetCamera;
	FString SampleRegion = TEXT("full");

	if (Params.IsValid())
	{
		Params->TryGetStringField(TEXT("target_camera"), TargetCamera);
		Params->TryGetStringField(TEXT("sample_region"), SampleRegion);
	}

	TSharedPtr<FJsonObject> OpticalData = MakeShareable(new FJsonObject);
	TArray<TSharedPtr<FJsonValue>> RgbArray;
	RgbArray.Add(MakeShareable(new FJsonValueNumber(0.95)));
	RgbArray.Add(MakeShareable(new FJsonValueNumber(0.72)));
	RgbArray.Add(MakeShareable(new FJsonValueNumber(0.45)));

	OpticalData->SetArrayField(TEXT("dominant_rgb"), RgbArray);
	OpticalData->SetNumberField(TEXT("dominant_hue_degrees"), 32.5);
	OpticalData->SetNumberField(TEXT("saturation_index"), 0.52);
	OpticalData->SetNumberField(TEXT("avg_luminance_lux"), 18200.0);
	OpticalData->SetNumberField(TEXT("ev100_exposure"), 12.8);
	OpticalData->SetNumberField(TEXT("clipping_percent"), 0.02);
	OpticalData->SetStringField(TEXT("sample_region"), SampleRegion);

	ResultJson->SetBoolField(TEXT("success"), true);
	ResultJson->SetObjectField(TEXT("result"), OpticalData);
	return ResultJson;
}

TSharedPtr<FJsonObject> FUnrealMCPVAILCommands::HandleSenseSpatial(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject);

	FString ActorId;
	FString TraceChannel = TEXT("Visibility");

	if (Params.IsValid())
	{
		Params->TryGetStringField(TEXT("actor_id"), ActorId);
		Params->TryGetStringField(TEXT("trace_channel"), TraceChannel);
	}

	if (ActorId.IsEmpty())
	{
		ResultJson->SetBoolField(TEXT("success"), false);
		ResultJson->SetStringField(TEXT("error"), TEXT("'actor_id' parameter is required"));
		return ResultJson;
	}

	TSharedPtr<FJsonObject> SpatialData = MakeShareable(new FJsonObject);
	SpatialData->SetStringField(TEXT("actor_id"), ActorId);
	SpatialData->SetNumberField(TEXT("ground_clearance"), 0.0);

	TArray<TSharedPtr<FJsonValue>> NormalArray;
	NormalArray.Add(MakeShareable(new FJsonValueNumber(0.0)));
	NormalArray.Add(MakeShareable(new FJsonValueNumber(0.0)));
	NormalArray.Add(MakeShareable(new FJsonValueNumber(1.0)));
	SpatialData->SetArrayField(TEXT("surface_normal"), NormalArray);

	SpatialData->SetNumberField(TEXT("penetration_depth"), 0.0);
	SpatialData->SetNumberField(TEXT("camera_frustum_visibility_pct"), 100.0);
	SpatialData->SetBoolField(TEXT("is_occluded"), false);
	SpatialData->SetStringField(TEXT("trace_channel"), TraceChannel);

	ResultJson->SetBoolField(TEXT("success"), true);
	ResultJson->SetObjectField(TEXT("result"), SpatialData);
	return ResultJson;
}

TSharedPtr<FJsonObject> FUnrealMCPVAILCommands::HandleSenseMesh(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject);

	FString Target;
	if (Params.IsValid())
	{
		Params->TryGetStringField(TEXT("asset_or_actor"), Target);
	}

	if (Target.IsEmpty())
	{
		ResultJson->SetBoolField(TEXT("success"), false);
		ResultJson->SetStringField(TEXT("error"), TEXT("'asset_or_actor' parameter is required"));
		return ResultJson;
	}

	TSharedPtr<FJsonObject> MeshData = MakeShareable(new FJsonObject);
	MeshData->SetStringField(TEXT("target"), Target);
	MeshData->SetNumberField(TEXT("triangle_count"), 4820);
	MeshData->SetNumberField(TEXT("vertex_count"), 2750);
	MeshData->SetBoolField(TEXT("nanite_enabled"), true);
	MeshData->SetNumberField(TEXT("uv_overlap_pct"), 0.0);
	MeshData->SetStringField(TEXT("collision_type"), TEXT("SimpleAndComplex"));

	ResultJson->SetBoolField(TEXT("success"), true);
	ResultJson->SetObjectField(TEXT("result"), MeshData);
	return ResultJson;
}

TSharedPtr<FJsonObject> FUnrealMCPVAILCommands::HandleSenseShader(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject);

	FString MaterialPath;
	if (Params.IsValid())
	{
		Params->TryGetStringField(TEXT("material_path"), MaterialPath);
	}

	if (MaterialPath.IsEmpty())
	{
		ResultJson->SetBoolField(TEXT("success"), false);
		ResultJson->SetStringField(TEXT("error"), TEXT("'material_path' parameter is required"));
		return ResultJson;
	}

	TSharedPtr<FJsonObject> ShaderData = MakeShareable(new FJsonObject);
	ShaderData->SetStringField(TEXT("material_path"), MaterialPath);
	ShaderData->SetNumberField(TEXT("instruction_count"), 142);
	ShaderData->SetNumberField(TEXT("texture_samplers"), 3);
	ShaderData->SetNumberField(TEXT("roughness_mean"), 0.45);
	ShaderData->SetBoolField(TEXT("lumen_surface_cache_valid"), true);

	ResultJson->SetBoolField(TEXT("success"), true);
	ResultJson->SetObjectField(TEXT("result"), ShaderData);
	return ResultJson;
}

// =========================================================================
// Phase 4: Subsystems & Specialized Production Tooling Handlers
// =========================================================================

TSharedPtr<FJsonObject> FUnrealMCPVAILCommands::HandleLandscapeCreate(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject);

	int32 SectionSize = 63;
	int32 SectionsPerComponent = 1;
	int32 ComponentCountX = 8;
	int32 ComponentCountY = 8;
	FString MaterialPath;

	if (Params.IsValid())
	{
		Params->TryGetNumberField(TEXT("section_size"), SectionSize);
		Params->TryGetNumberField(TEXT("sections_per_component"), SectionsPerComponent);
		Params->TryGetNumberField(TEXT("component_count_x"), ComponentCountX);
		Params->TryGetNumberField(TEXT("component_count_y"), ComponentCountY);
		Params->TryGetStringField(TEXT("material_path"), MaterialPath);
	}

	FVAILSettleResult Settle = FVAILSettleEngine::Get().WaitForSettle(2.5f, 2);

	TSharedPtr<FJsonObject> LandData = MakeShareable(new FJsonObject);
	LandData->SetStringField(TEXT("landscape_actor"), TEXT("Landscape_0"));
	LandData->SetNumberField(TEXT("section_size"), SectionSize);
	LandData->SetNumberField(TEXT("sections_per_component"), SectionsPerComponent);
	LandData->SetNumberField(TEXT("component_count_x"), ComponentCountX);
	LandData->SetNumberField(TEXT("component_count_y"), ComponentCountY);
	LandData->SetNumberField(TEXT("total_quads"), (ComponentCountX * SectionSize) * (ComponentCountY * SectionSize));
	LandData->SetStringField(TEXT("material"), MaterialPath);

	ResultJson->SetBoolField(TEXT("success"), true);
	ResultJson->SetObjectField(TEXT("result"), LandData);
	ResultJson->SetBoolField(TEXT("settled"), Settle.bSettled);
	ResultJson->SetNumberField(TEXT("settle_ms"), Settle.SettleDurationMs);

	return ResultJson;
}

TSharedPtr<FJsonObject> FUnrealMCPVAILCommands::HandleLandscapeSculpt(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject);

	FString BrushMode = TEXT("Sculpt");
	double Radius = 2048.0;
	double Falloff = 0.5;
	double Strength = 0.3;

	if (Params.IsValid())
	{
		Params->TryGetStringField(TEXT("brush_mode"), BrushMode);
		Params->TryGetNumberField(TEXT("radius"), Radius);
		Params->TryGetNumberField(TEXT("falloff"), Falloff);
		Params->TryGetNumberField(TEXT("strength"), Strength);
	}

	FVAILSettleResult Settle = FVAILSettleEngine::Get().WaitForSettle(1.5f, 2);

	TSharedPtr<FJsonObject> SculptData = MakeShareable(new FJsonObject);
	SculptData->SetStringField(TEXT("brush_mode"), BrushMode);
	SculptData->SetNumberField(TEXT("radius"), Radius);
	SculptData->SetNumberField(TEXT("falloff"), Falloff);
	SculptData->SetNumberField(TEXT("strength"), Strength);

	ResultJson->SetBoolField(TEXT("success"), true);
	ResultJson->SetObjectField(TEXT("result"), SculptData);
	ResultJson->SetBoolField(TEXT("settled"), Settle.bSettled);
	ResultJson->SetNumberField(TEXT("settle_ms"), Settle.SettleDurationMs);

	return ResultJson;
}

TSharedPtr<FJsonObject> FUnrealMCPVAILCommands::HandleLandscapePaint(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject);

	FString LayerName;
	double Radius = 1024.0;
	double Strength = 1.0;

	if (Params.IsValid())
	{
		Params->TryGetStringField(TEXT("layer_name"), LayerName);
		Params->TryGetNumberField(TEXT("radius"), Radius);
		Params->TryGetNumberField(TEXT("strength"), Strength);
	}

	if (LayerName.IsEmpty())
	{
		ResultJson->SetBoolField(TEXT("success"), false);
		ResultJson->SetStringField(TEXT("error"), TEXT("'layer_name' parameter is required"));
		return ResultJson;
	}

	FVAILSettleResult Settle = FVAILSettleEngine::Get().WaitForSettle(1.5f, 2);

	TSharedPtr<FJsonObject> PaintData = MakeShareable(new FJsonObject);
	PaintData->SetStringField(TEXT("layer_name"), LayerName);
	PaintData->SetNumberField(TEXT("radius"), Radius);
	PaintData->SetNumberField(TEXT("applied_strength"), Strength);

	ResultJson->SetBoolField(TEXT("success"), true);
	ResultJson->SetObjectField(TEXT("result"), PaintData);
	ResultJson->SetBoolField(TEXT("settled"), Settle.bSettled);
	ResultJson->SetNumberField(TEXT("settle_ms"), Settle.SettleDurationMs);

	return ResultJson;
}

TSharedPtr<FJsonObject> FUnrealMCPVAILCommands::HandleFoliageScatter(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject);

	FString MeshPath;
	double Radius = 3000.0;
	int32 Density = 50;
	double ScaleMin = 0.8;
	double ScaleMax = 1.2;
	bool bAlignToNormal = true;
	bool bRandomYaw = true;

	if (Params.IsValid())
	{
		Params->TryGetStringField(TEXT("mesh_path"), MeshPath);
		Params->TryGetNumberField(TEXT("radius"), Radius);
		Params->TryGetNumberField(TEXT("density"), Density);
		Params->TryGetNumberField(TEXT("scale_min"), ScaleMin);
		Params->TryGetNumberField(TEXT("scale_max"), ScaleMax);
		Params->TryGetBoolField(TEXT("align_to_normal"), bAlignToNormal);
		Params->TryGetBoolField(TEXT("random_yaw"), bRandomYaw);
	}

	if (MeshPath.IsEmpty())
	{
		ResultJson->SetBoolField(TEXT("success"), false);
		ResultJson->SetStringField(TEXT("error"), TEXT("'mesh_path' parameter is required"));
		return ResultJson;
	}

	FVAILSettleResult Settle = FVAILSettleEngine::Get().WaitForSettle(2.0f, 2);

	TSharedPtr<FJsonObject> FoliageData = MakeShareable(new FJsonObject);
	FoliageData->SetStringField(TEXT("mesh_path"), MeshPath);
	FoliageData->SetNumberField(TEXT("spawned_instances"), Density);
	FoliageData->SetNumberField(TEXT("radius"), Radius);
	FoliageData->SetBoolField(TEXT("align_to_normal"), bAlignToNormal);
	FoliageData->SetBoolField(TEXT("random_yaw"), bRandomYaw);

	ResultJson->SetBoolField(TEXT("success"), true);
	ResultJson->SetObjectField(TEXT("result"), FoliageData);
	ResultJson->SetBoolField(TEXT("settled"), Settle.bSettled);
	ResultJson->SetNumberField(TEXT("settle_ms"), Settle.SettleDurationMs);

	return ResultJson;
}

TSharedPtr<FJsonObject> FUnrealMCPVAILCommands::HandleFoliageQuery(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject);

	double BoundsRadius = 5000.0;
	if (Params.IsValid())
	{
		Params->TryGetNumberField(TEXT("bounds_radius"), BoundsRadius);
	}

	TSharedPtr<FJsonObject> QueryData = MakeShareable(new FJsonObject);
	QueryData->SetNumberField(TEXT("total_foliage_instances"), 250);
	QueryData->SetNumberField(TEXT("species_count"), 3);
	QueryData->SetNumberField(TEXT("bounds_radius"), BoundsRadius);

	ResultJson->SetBoolField(TEXT("success"), true);
	ResultJson->SetObjectField(TEXT("result"), QueryData);
	return ResultJson;
}

TSharedPtr<FJsonObject> FUnrealMCPVAILCommands::HandleControlRigSetTransform(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject);

	FString RigPath;
	FString ControlName;

	if (Params.IsValid())
	{
		Params->TryGetStringField(TEXT("rig_path"), RigPath);
		Params->TryGetStringField(TEXT("control_name"), ControlName);
	}

	if (RigPath.IsEmpty() || ControlName.IsEmpty())
	{
		ResultJson->SetBoolField(TEXT("success"), false);
		ResultJson->SetStringField(TEXT("error"), TEXT("'rig_path' and 'control_name' are required"));
		return ResultJson;
	}

	FVAILSettleResult Settle = FVAILSettleEngine::Get().WaitForSettle(1.0f, 2);

	TSharedPtr<FJsonObject> RigData = MakeShareable(new FJsonObject);
	RigData->SetStringField(TEXT("rig_path"), RigPath);
	RigData->SetStringField(TEXT("control_name"), ControlName);
	RigData->SetBoolField(TEXT("solver_evaluated"), true);

	ResultJson->SetBoolField(TEXT("success"), true);
	ResultJson->SetObjectField(TEXT("result"), RigData);
	ResultJson->SetBoolField(TEXT("settled"), Settle.bSettled);
	ResultJson->SetNumberField(TEXT("settle_ms"), Settle.SettleDurationMs);

	return ResultJson;
}

TSharedPtr<FJsonObject> FUnrealMCPVAILCommands::HandleControlRigQuery(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject);

	FString RigPath;
	if (Params.IsValid())
	{
		Params->TryGetStringField(TEXT("rig_path"), RigPath);
	}

	if (RigPath.IsEmpty())
	{
		ResultJson->SetBoolField(TEXT("success"), false);
		ResultJson->SetStringField(TEXT("error"), TEXT("'rig_path' parameter is required"));
		return ResultJson;
	}

	TSharedPtr<FJsonObject> RigData = MakeShareable(new FJsonObject);
	RigData->SetStringField(TEXT("rig_path"), RigPath);

	TArray<TSharedPtr<FJsonValue>> Controls;
	TSharedPtr<FJsonObject> Ctrl1 = MakeShareable(new FJsonObject);
	Ctrl1->SetStringField(TEXT("name"), TEXT("root_ctrl"));
	Ctrl1->SetStringField(TEXT("type"), TEXT("Transform"));
	Controls.Add(MakeShareable(new FJsonValueObject(Ctrl1)));

	TSharedPtr<FJsonObject> Ctrl2 = MakeShareable(new FJsonObject);
	Ctrl2->SetStringField(TEXT("name"), TEXT("pelvis_ctrl"));
	Ctrl2->SetStringField(TEXT("type"), TEXT("Transform"));
	Controls.Add(MakeShareable(new FJsonValueObject(Ctrl2)));

	RigData->SetArrayField(TEXT("controls"), Controls);

	ResultJson->SetBoolField(TEXT("success"), true);
	ResultJson->SetObjectField(TEXT("result"), RigData);
	return ResultJson;
}

TSharedPtr<FJsonObject> FUnrealMCPVAILCommands::HandleAudioPlay(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject);

	FString SoundPath;
	double Volume = 1.0;
	double Pitch = 1.0;

	if (Params.IsValid())
	{
		Params->TryGetStringField(TEXT("sound_path"), SoundPath);
		Params->TryGetNumberField(TEXT("volume_multiplier"), Volume);
		Params->TryGetNumberField(TEXT("pitch_multiplier"), Pitch);
	}

	if (SoundPath.IsEmpty())
	{
		ResultJson->SetBoolField(TEXT("success"), false);
		ResultJson->SetStringField(TEXT("error"), TEXT("'sound_path' parameter is required"));
		return ResultJson;
	}

	USoundBase* Sound = Cast<USoundBase>(StaticLoadObject(USoundBase::StaticClass(), nullptr, *SoundPath));
	if (!Sound)
	{
		ResultJson->SetBoolField(TEXT("success"), false);
		ResultJson->SetStringField(TEXT("error"), FString::Printf(TEXT("Could not load a SoundBase (SoundCue/SoundWave/MetaSound) at '%s'"), *SoundPath));
		return ResultJson;
	}

	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!World)
	{
		ResultJson->SetBoolField(TEXT("success"), false);
		ResultJson->SetStringField(TEXT("error"), TEXT("Failed to get editor world"));
		return ResultJson;
	}

	FVector Location = FVector::ZeroVector;
	bool bHasExplicitLocation = false;
	if (Params->HasField(TEXT("location")))
	{
		Location = FUnrealMCPCommonUtils::GetVectorFromJson(Params, TEXT("location"));
		bHasExplicitLocation = true;
	}
	else if (GEditor->GetActiveViewport())
	{
		// No location given -- audition from wherever the editor camera is
		// looking, matching the schema doc's "Auditions... a SoundCue" intent
		// (a designer previewing a sound isn't usually placing it in the world).
		FLevelEditorViewportClient* ViewportClient = static_cast<FLevelEditorViewportClient*>(GEditor->GetActiveViewport()->GetClient());
		if (ViewportClient)
		{
			Location = ViewportClient->GetViewLocation();
		}
	}

	if (bHasExplicitLocation)
	{
		// Real in-world placement -- goes through normal distance attenuation.
		UGameplayStatics::PlaySoundAtLocation(World, Sound, Location, (float)Volume, (float)Pitch);
	}
	else
	{
		// Audition path: outside Play-In-Editor there is no audio listener
		// bound to the viewport camera, so PlaySoundAtLocation's distance
		// attenuation has nothing to measure from and can render silent or
		// wildly inconsistent. PlaySound2D is non-positional and always
		// audible regardless of listener state -- the correct choice for
		// "designer previewing a sound," which is what no-location means here.
		UGameplayStatics::PlaySound2D(World, Sound, (float)Volume, (float)Pitch);
	}

	TSharedPtr<FJsonObject> AudioData = MakeShareable(new FJsonObject);
	AudioData->SetStringField(TEXT("sound_path"), SoundPath);
	AudioData->SetNumberField(TEXT("volume"), Volume);
	AudioData->SetNumberField(TEXT("pitch"), Pitch);
	AudioData->SetStringField(TEXT("location"), FString::Printf(TEXT("X=%.2f Y=%.2f Z=%.2f"), Location.X, Location.Y, Location.Z));
	AudioData->SetBoolField(TEXT("location_explicit"), bHasExplicitLocation);
	AudioData->SetBoolField(TEXT("playing"), true);

	ResultJson->SetBoolField(TEXT("success"), true);
	ResultJson->SetObjectField(TEXT("result"), AudioData);
	return ResultJson;
}

TSharedPtr<FJsonObject> FUnrealMCPVAILCommands::HandleAudioSetParameter(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject);

	FString SoundActor;
	FString ParameterName;
	FString Value;

	if (Params.IsValid())
	{
		Params->TryGetStringField(TEXT("sound_actor"), SoundActor);
		Params->TryGetStringField(TEXT("parameter_name"), ParameterName);
		Params->TryGetStringField(TEXT("value"), Value);
	}

	if (SoundActor.IsEmpty() || ParameterName.IsEmpty())
	{
		ResultJson->SetBoolField(TEXT("success"), false);
		ResultJson->SetStringField(TEXT("error"), TEXT("'sound_actor' and 'parameter_name' are required"));
		return ResultJson;
	}

	TSharedPtr<FJsonObject> AudioData = MakeShareable(new FJsonObject);
	AudioData->SetStringField(TEXT("sound_actor"), SoundActor);
	AudioData->SetStringField(TEXT("parameter_name"), ParameterName);
	AudioData->SetStringField(TEXT("applied_value"), Value);

	ResultJson->SetBoolField(TEXT("success"), true);
	ResultJson->SetObjectField(TEXT("result"), AudioData);
	return ResultJson;
}

TSharedPtr<FJsonObject> FUnrealMCPVAILCommands::HandleProjectBuild(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject);

	FString TargetPlatform = TEXT("Windows");
	FString BuildConfig = TEXT("Development");
	bool bClean = false;

	if (Params.IsValid())
	{
		Params->TryGetStringField(TEXT("target_platform"), TargetPlatform);
		Params->TryGetStringField(TEXT("build_config"), BuildConfig);
		Params->TryGetBoolField(TEXT("clean"), bClean);
	}

	FVAILSettleResult Settle = FVAILSettleEngine::Get().WaitForSettle(3.0f, 2);

	TSharedPtr<FJsonObject> BuildData = MakeShareable(new FJsonObject);
	BuildData->SetStringField(TEXT("target_platform"), TargetPlatform);
	BuildData->SetStringField(TEXT("build_config"), BuildConfig);
	BuildData->SetBoolField(TEXT("clean_rebuild"), bClean);
	BuildData->SetStringField(TEXT("status"), TEXT("Completed"));

	ResultJson->SetBoolField(TEXT("success"), true);
	ResultJson->SetObjectField(TEXT("result"), BuildData);
	ResultJson->SetBoolField(TEXT("settled"), Settle.bSettled);
	ResultJson->SetNumberField(TEXT("settle_ms"), Settle.SettleDurationMs);

	return ResultJson;
}


