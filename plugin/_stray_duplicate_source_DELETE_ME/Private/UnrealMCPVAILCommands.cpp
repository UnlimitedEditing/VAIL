// Copyright Epic Games, Inc. All Rights Reserved.

#include "Commands/UnrealMCPVAILCommands.h"
#include "VAILSessionManager.h"
#include "VAILPropertyInspector.h"
#include "VAILIdentityRegistry.h"
#include "VAILSettleEngine.h"
#include "VAILGraphInspector.h"
#include "VAILAssetManager.h"
#include "Dom/JsonValue.h"

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
