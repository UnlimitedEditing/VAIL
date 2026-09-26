// Copyright Epic Games, Inc. All Rights Reserved.

#include "VAILGraphInspector.h"
#include "K2Node_CallFunction.h"
#include "K2Node_Event.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "K2Node_IfThenElse.h"
#include "K2Node_ExecutionSequence.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Materials/MaterialExpression.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialExpressionAdd.h"
#include "Materials/MaterialExpressionConstant.h"
#include "Materials/MaterialExpressionTextureSample.h"
#include "MaterialGraph/MaterialGraph.h"
#include "MaterialGraph/MaterialGraphNode.h"
#include "MaterialGraph/MaterialGraphNode_Root.h"
#include "MaterialGraph/MaterialGraphSchema.h"
#include "UObject/UObjectIterator.h"

FVAILGraphInspector& FVAILGraphInspector::Get()
{
	static FVAILGraphInspector Singleton;
	return Singleton;
}

FVAILGraphInspector::FVAILGraphInspector()
{
}

FVAILGraphInspector::~FVAILGraphInspector()
{
}

UEdGraph* FVAILGraphInspector::ResolveGraph(UObject* AssetObject, const FString& GraphName, FString& ErrorMessage) const
{
	if (!AssetObject)
	{
		ErrorMessage = TEXT("Asset object is null");
		return nullptr;
	}

	// 1. Blueprint Graphs
	if (UBlueprint* Blueprint = Cast<UBlueprint>(AssetObject))
	{
		if (GraphName.IsEmpty() || GraphName.Equals(TEXT("EventGraph"), ESearchCase::IgnoreCase))
		{
			if (Blueprint->UbergraphPages.Num() > 0)
			{
				return Blueprint->UbergraphPages[0];
			}
		}

		for (UEdGraph* Graph : Blueprint->UbergraphPages)
		{
			if (Graph && Graph->GetName().Equals(GraphName, ESearchCase::IgnoreCase))
			{
				return Graph;
			}
		}

		for (UEdGraph* Graph : Blueprint->FunctionGraphs)
		{
			if (Graph && Graph->GetName().Equals(GraphName, ESearchCase::IgnoreCase))
			{
				return Graph;
			}
		}

		for (UEdGraph* Graph : Blueprint->MacroGraphs)
		{
			if (Graph && Graph->GetName().Equals(GraphName, ESearchCase::IgnoreCase))
			{
				return Graph;
			}
		}

		if (Blueprint->UbergraphPages.Num() > 0)
		{
			return Blueprint->UbergraphPages[0];
		}

		ErrorMessage = FString::Printf(TEXT("Graph '%s' not found on Blueprint '%s'"), *GraphName, *Blueprint->GetName());
		return nullptr;
	}

	// 2. Material Graphs
	if (UMaterial* Material = Cast<UMaterial>(AssetObject))
	{
		if (!Material->MaterialGraph)
		{
			UMaterialGraph* NewMaterialGraph = NewObject<UMaterialGraph>(Material, UMaterialGraph::StaticClass(), NAME_None, RF_Transactional);
			NewMaterialGraph->Schema = UMaterialGraphSchema::StaticClass();
			NewMaterialGraph->Material = Material;
			Material->MaterialGraph = NewMaterialGraph;
			NewMaterialGraph->RebuildGraph();
		}

		return Material->MaterialGraph;
	}

	// 3. Direct UEdGraph
	if (UEdGraph* DirectGraph = Cast<UEdGraph>(AssetObject))
	{
		return DirectGraph;
	}

	ErrorMessage = FString::Printf(TEXT("Object '%s' (Class: %s) does not support graph inspection"), *AssetObject->GetName(), *AssetObject->GetClass()->GetName());
	return nullptr;
}

UEdGraphNode* FVAILGraphInspector::FindNode(UEdGraph* Graph, const FString& NodeId) const
{
	if (!Graph) return nullptr;

	for (UEdGraphNode* Node : Graph->Nodes)
	{
		if (Node)
		{
			if (Node->NodeGuid.ToString().Equals(NodeId, ESearchCase::IgnoreCase) ||
				Node->GetName().Equals(NodeId, ESearchCase::IgnoreCase))
			{
				return Node;
			}

			if (Node->IsA<UMaterialGraphNode_Root>() &&
				(NodeId.Equals(TEXT("Root"), ESearchCase::IgnoreCase) ||
				 NodeId.Equals(TEXT("Material"), ESearchCase::IgnoreCase) ||
				 NodeId.Equals(TEXT("Result"), ESearchCase::IgnoreCase)))
			{
				return Node;
			}
		}
	}
	return nullptr;
}

	auto NormalizePinName = [](const FString& InName) -> FString
	{
		FString Result = InName;
		Result.RemoveSpacesInline();
		Result.ReplaceInline(TEXT("_"), TEXT(""));
		return Result.ToLower();
	};

	FString TargetNormalized = NormalizePinName(PinName);

	// 1. Exact or normalized match
	for (UEdGraphPin* Pin : Node->Pins)
	{
		if (Pin)
		{
			if (Direction != EGPD_MAX && Pin->Direction != Direction)
			{
				continue;
			}

			if (Pin->PinName.ToString().Equals(PinName, ESearchCase::IgnoreCase) ||
				Pin->PinFriendlyName.ToString().Equals(PinName, ESearchCase::IgnoreCase) ||
				NormalizePinName(Pin->PinName.ToString()) == TargetNormalized ||
				NormalizePinName(Pin->PinFriendlyName.ToString()) == TargetNormalized)
			{
				return Pin;
			}
		}
	}

	// 2. Fuzzy / fallback alias match
	for (UEdGraphPin* Pin : Node->Pins)
	{
		if (Pin)
		{
			if (Direction != EGPD_MAX && Pin->Direction != Direction)
			{
				continue;
			}

			if ((TargetNormalized == TEXT("output") || TargetNormalized == TEXT("out") || TargetNormalized == TEXT("result")) && Pin->Direction == EGPD_Output)
			{
				return Pin;
			}
			if ((TargetNormalized == TEXT("rgba") || TargetNormalized == TEXT("rgb")) && Pin->PinName.ToString().StartsWith(TEXT("RGB"), ESearchCase::IgnoreCase))
			{
				return Pin;
			}
		}
	}

	return nullptr;
}

bool FVAILGraphInspector::InspectGraph(
	UObject* AssetObject,
	const FString& GraphName,
	FVAILGraphTopology& OutTopology,
	FString& ErrorMessage)
{
	UEdGraph* Graph = ResolveGraph(AssetObject, GraphName, ErrorMessage);
	if (!Graph)
	{
		return false;
	}

	OutTopology.GraphName = Graph->GetName();
	OutTopology.GraphType = Graph->GetClass()->GetName();
	OutTopology.AssetPath = AssetObject->GetPathName();

	for (UEdGraphNode* Node : Graph->Nodes)
	{
		if (!Node) continue;

		FVAILGraphNode VNode;
		VNode.NodeId = Node->NodeGuid.IsValid() ? Node->NodeGuid.ToString() : Node->GetName();
		VNode.NodeTitle = Node->GetNodeTitle(ENodeTitleType::ListView).ToString();
		VNode.NodeClass = Node->GetClass()->GetName();
		VNode.Position = FVector2D(Node->NodePosX, Node->NodePosY);

		for (UEdGraphPin* Pin : Node->Pins)
		{
			if (!Pin) continue;

			FVAILGraphPin VPin;
			VPin.PinId = Pin->PinName.ToString();
			VPin.PinName = Pin->PinFriendlyName.IsEmpty() ? Pin->PinName.ToString() : Pin->PinFriendlyName.ToString();
			VPin.Direction = (Pin->Direction == EGPD_Input) ? TEXT("Input") : TEXT("Output");
			VPin.PinCategory = Pin->PinType.PinCategory.ToString();
			VPin.PinSubCategory = Pin->PinType.PinSubCategory.ToString();
			VPin.DefaultValue = Pin->GetDefaultAsString();

			for (UEdGraphPin* LinkedPin : Pin->LinkedTo)
			{
				if (LinkedPin && LinkedPin->GetOwningNode())
				{
					FString LinkedNodeId = LinkedPin->GetOwningNode()->NodeGuid.IsValid() ?
						LinkedPin->GetOwningNode()->NodeGuid.ToString() : LinkedPin->GetOwningNode()->GetName();
					VPin.LinkedToPins.Add(FString::Printf(TEXT("%s:%s"), *LinkedNodeId, *LinkedPin->PinName.ToString()));
				}
			}

			VNode.Pins.Add(VPin);
		}

		OutTopology.Nodes.Add(VNode);
	}

	return true;
}

bool FVAILGraphInspector::SpawnNode(
	UObject* AssetObject,
	const FString& GraphName,
	const FString& NodeType,
	const FVector2D& Position,
	const TMap<FString, FString>& ExtraParams,
	FString& OutNodeId,
	FString& ErrorMessage)
{
	UEdGraph* Graph = ResolveGraph(AssetObject, GraphName, ErrorMessage);
	if (!Graph)
	{
		return false;
	}

	// 1. Spawning inside Blueprint Graph
	if (UBlueprint* Blueprint = Cast<UBlueprint>(AssetObject))
	{
		Graph->Modify();

		// Handle CallFunction
		if (NodeType.StartsWith(TEXT("CallFunction:")))
		{
			FString FunctionName = NodeType.Mid(13);
			UK2Node_CallFunction* CallFuncNode = NewObject<UK2Node_CallFunction>(Graph);
			
			// Resolve function from Kismet libraries or engine classes
			UFunction* TargetFunc = nullptr;
			for (TObjectIterator<UFunction> It; It; ++It)
			{
				if (It->GetName().Equals(FunctionName, ESearchCase::IgnoreCase))
				{
					TargetFunc = *It;
					break;
				}
			}

			if (!TargetFunc)
			{
				ErrorMessage = FString::Printf(TEXT("Function '%s' not found"), *FunctionName);
				return false;
			}

			CallFuncNode->CreateNewGuid();
			CallFuncNode->PostPlacedNewNode();
			CallFuncNode->SetFromFunction(TargetFunc);
			CallFuncNode->NodePosX = Position.X;
			CallFuncNode->NodePosY = Position.Y;
			CallFuncNode->AllocateDefaultPins();
			Graph->AddNode(CallFuncNode, true, false);

			OutNodeId = CallFuncNode->NodeGuid.ToString();
			FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
			return true;
		}

		// Handle Event Node
		if (NodeType.StartsWith(TEXT("Event:")))
		{
			FString EventName = NodeType.Mid(6);
			UK2Node_Event* EventNode = NewObject<UK2Node_Event>(Graph);
			EventNode->CreateNewGuid();
			EventNode->PostPlacedNewNode();
			EventNode->EventReference.SetExternalMember(FName(*EventName), AActor::StaticClass());
			EventNode->CustomFunctionName = FName(*EventName);
			EventNode->NodePosX = Position.X;
			EventNode->NodePosY = Position.Y;
			EventNode->AllocateDefaultPins();
			Graph->AddNode(EventNode, true, false);

			OutNodeId = EventNode->NodeGuid.ToString();
			FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
			return true;
		}

		// Generic K2Node Class Spawn
		UClass* NodeClass = nullptr;
		for (TObjectIterator<UClass> It; It; ++It)
		{
			if (It->IsChildOf(UK2Node::StaticClass()) &&
				(It->GetName().Equals(NodeType, ESearchCase::IgnoreCase) ||
				 It->GetName().Equals(TEXT("K2Node_") + NodeType, ESearchCase::IgnoreCase)))
			{
				NodeClass = *It;
				break;
			}
		}

		if (NodeClass)
		{
			UK2Node* NewNode = NewObject<UK2Node>(Graph, NodeClass);
			NewNode->CreateNewGuid();
			NewNode->PostPlacedNewNode();
			NewNode->NodePosX = Position.X;
			NewNode->NodePosY = Position.Y;
			NewNode->AllocateDefaultPins();
			Graph->AddNode(NewNode, true, false);

			OutNodeId = NewNode->NodeGuid.ToString();
			FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
			return true;
		}

		ErrorMessage = FString::Printf(TEXT("Unrecognized Blueprint node type '%s'"), *NodeType);
		return false;
	}

	// 2. Spawning inside Material Graph
	if (UMaterial* Material = Cast<UMaterial>(AssetObject))
	{
		Material->PreEditChange(nullptr);

		UClass* ExpressionClass = nullptr;
		for (TObjectIterator<UClass> It; It; ++It)
		{
			if (It->IsChildOf(UMaterialExpression::StaticClass()) &&
				(It->GetName().Equals(NodeType, ESearchCase::IgnoreCase) ||
				 It->GetName().Equals(TEXT("MaterialExpression") + NodeType, ESearchCase::IgnoreCase)))
			{
				ExpressionClass = *It;
				break;
			}
		}

		if (!ExpressionClass)
		{
			ErrorMessage = FString::Printf(TEXT("Material expression class '%s' not found"), *NodeType);
			return false;
		}

		UMaterialExpression* NewExpr = NewObject<UMaterialExpression>(Material, ExpressionClass);
		NewExpr->MaterialExpressionEditorX = Position.X;
		NewExpr->MaterialExpressionEditorY = Position.Y;

		// Apply parameter names or values if provided
		if (UMaterialExpressionVectorParameter* VecParam = Cast<UMaterialExpressionVectorParameter>(NewExpr))
		{
			if (const FString* ParamName = ExtraParams.Find(TEXT("ParameterName")))
			{
				VecParam->ParameterName = FName(**ParamName);
			}
			if (const FString* DefaultVal = ExtraParams.Find(TEXT("DefaultValue")))
			{
				FLinearColor Color;
				if (Color.InitFromString(*DefaultVal))
				{
					VecParam->DefaultValue = Color;
				}
			}
		}
		else if (UMaterialExpressionScalarParameter* ScalarParam = Cast<UMaterialExpressionScalarParameter>(NewExpr))
		{
			if (const FString* ParamName = ExtraParams.Find(TEXT("ParameterName")))
			{
				ScalarParam->ParameterName = FName(**ParamName);
			}
			if (const FString* DefaultVal = ExtraParams.Find(TEXT("DefaultValue")))
			{
				ScalarParam->DefaultValue = FCString::Atof(**DefaultVal);
			}
		}

		Material->GetExpressionCollection().AddExpression(NewExpr);
		Material->AddExpressionParameter(NewExpr, Material->EditorParameters);

		if (Material->MaterialGraph)
		{
			Material->MaterialGraph->AddExpression(NewExpr, false);
			Material->MaterialGraph->RebuildGraph();
		}

		Material->PostEditChange();
		Material->MarkPackageDirty();

		OutNodeId = NewExpr->GetName();
		return true;
	}

	ErrorMessage = FString::Printf(TEXT("Node spawning not supported on asset '%s'"), *AssetObject->GetName());
	return false;
}

bool FVAILGraphInspector::ConnectPins(
	UObject* AssetObject,
	const FString& GraphName,
	const FString& SourcePinSpec,
	const FString& TargetPinSpec,
	FString& ErrorMessage)
{
	UEdGraph* Graph = ResolveGraph(AssetObject, GraphName, ErrorMessage);
	if (!Graph || !Graph->GetSchema())
	{
		return false;
	}

	FString SourceNodeId, SourcePinName;
	if (!SourcePinSpec.Split(TEXT(":"), &SourceNodeId, &SourcePinName))
	{
		ErrorMessage = FString::Printf(TEXT("Invalid SourcePinSpec format '%s'. Expected 'NodeId:PinName'"), *SourcePinSpec);
		return false;
	}

	FString TargetNodeId, TargetPinName;
	if (!TargetPinSpec.Split(TEXT(":"), &TargetNodeId, &TargetPinName))
	{
		ErrorMessage = FString::Printf(TEXT("Invalid TargetPinSpec format '%s'. Expected 'NodeId:PinName'"), *TargetPinSpec);
		return false;
	}

	UEdGraphNode* SourceNode = FindNode(Graph, SourceNodeId);
	if (!SourceNode)
	{
		ErrorMessage = FString::Printf(TEXT("Source node '%s' not found in graph"), *SourceNodeId);
		return false;
	}

	UEdGraphNode* TargetNode = FindNode(Graph, TargetNodeId);
	if (!TargetNode)
	{
		ErrorMessage = FString::Printf(TEXT("Target node '%s' not found in graph"), *TargetNodeId);
		return false;
	}

	UEdGraphPin* SourcePin = FindPin(SourceNode, SourcePinName, EGPD_Output);
	if (!SourcePin)
	{
		// Fallback to any direction if not strictly output
		SourcePin = FindPin(SourceNode, SourcePinName);
	}

	UEdGraphPin* TargetPin = FindPin(TargetNode, TargetPinName, EGPD_Input);
	if (!TargetPin)
	{
		// Fallback to any direction if not strictly input
		TargetPin = FindPin(TargetNode, TargetPinName);
	}

	if (!SourcePin || !TargetPin)
	{
		ErrorMessage = FString::Printf(TEXT("Could not resolve pins: SourcePin=%s (%s), TargetPin=%s (%s)"),
			*SourcePinName, SourcePin ? TEXT("Found") : TEXT("Missing"),
			*TargetPinName, TargetPin ? TEXT("Found") : TEXT("Missing"));
		return false;
	}

	// Schema-validated connection
	Graph->Modify();
	const bool bConnected = Graph->GetSchema()->TryCreateConnection(SourcePin, TargetPin);
	if (!bConnected)
	{
		ErrorMessage = FString::Printf(TEXT("Schema rejected connection between '%s' and '%s'"), *SourcePinSpec, *TargetPinSpec);
		return false;
	}

	if (UBlueprint* Blueprint = Cast<UBlueprint>(AssetObject))
	{
		FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
	}
	else if (UMaterial* Material = Cast<UMaterial>(AssetObject))
	{
		Material->PostEditChange();
		Material->MarkPackageDirty();
	}

	return true;
}

bool FVAILGraphInspector::SetPinDefaultValue(
	UObject* AssetObject,
	const FString& GraphName,
	const FString& PinSpec,
	const FString& Value,
	FString& ErrorMessage)
{
	UEdGraph* Graph = ResolveGraph(AssetObject, GraphName, ErrorMessage);
	if (!Graph || !Graph->GetSchema())
	{
		return false;
	}

	FString NodeId, PinName;
	if (!PinSpec.Split(TEXT(":"), &NodeId, &PinName))
	{
		ErrorMessage = FString::Printf(TEXT("Invalid PinSpec format '%s'. Expected 'NodeId:PinName'"), *PinSpec);
		return false;
	}

	UEdGraphNode* Node = FindNode(Graph, NodeId);
	if (!Node)
	{
		ErrorMessage = FString::Printf(TEXT("Node '%s' not found in graph"), *NodeId);
		return false;
	}

	UEdGraphPin* Pin = FindPin(Node, PinName, EGPD_Input);
	if (!Pin)
	{
		ErrorMessage = FString::Printf(TEXT("Input pin '%s' not found on node '%s'"), *PinName, *NodeId);
		return false;
	}

	Graph->GetSchema()->TrySetDefaultValue(*Pin, Value);

	if (UBlueprint* Blueprint = Cast<UBlueprint>(AssetObject))
	{
		FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
	}

	return true;
}

bool FVAILGraphInspector::DeleteNode(
	UObject* AssetObject,
	const FString& GraphName,
	const FString& NodeId,
	FString& ErrorMessage)
{
	UEdGraph* Graph = ResolveGraph(AssetObject, GraphName, ErrorMessage);
	if (!Graph)
	{
		return false;
	}

	UEdGraphNode* Node = FindNode(Graph, NodeId);
	if (!Node)
	{
		ErrorMessage = FString::Printf(TEXT("Node '%s' not found in graph"), *NodeId);
		return false;
	}

	Graph->Modify();
	Node->BreakAllNodeLinks();
	Graph->RemoveNode(Node);

	if (UBlueprint* Blueprint = Cast<UBlueprint>(AssetObject))
	{
		FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
	}
	else if (UMaterial* Material = Cast<UMaterial>(AssetObject))
	{
		Material->PostEditChange();
		Material->MarkPackageDirty();
	}

	return true;
}
