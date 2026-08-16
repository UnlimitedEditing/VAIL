// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraph/EdGraphSchema.h"
#include "Engine/Blueprint.h"
#include "Materials/Material.h"

struct FVAILGraphPin
{
	FString PinId;
	FString PinName;
	FString Direction; // "Input" or "Output"
	FString PinCategory; // "exec", "float", "vector", "object", etc.
	FString PinSubCategory;
	FString DefaultValue;
	TArray<FString> LinkedToPins; // "NodeId:PinName"
};

struct FVAILGraphNode
{
	FString NodeId;
	FString NodeTitle;
	FString NodeClass;
	FVector2D Position;
	TArray<FVAILGraphPin> Pins;
};

struct FVAILGraphTopology
{
	FString GraphName;
	FString GraphType;
	FString AssetPath;
	TArray<FVAILGraphNode> Nodes;
};

class VAILCORE_API FVAILGraphInspector
{
public:
	static FVAILGraphInspector& Get();

	/**
	 * Inspects an asset's graph topology and returns a compact list of nodes and connected pins.
	 */
	bool InspectGraph(
		UObject* AssetObject,
		const FString& GraphName,
		FVAILGraphTopology& OutTopology,
		FString& ErrorMessage
	);

	/**
	 * Spawns a node polymorphically inside an asset's graph (Blueprint K2Node, Material Expression, etc.)
	 */
	bool SpawnNode(
		UObject* AssetObject,
		const FString& GraphName,
		const FString& NodeType,
		const FVector2D& Position,
		const TMap<FString, FString>& ExtraParams,
		FString& OutNodeId,
		FString& ErrorMessage
	);

	/**
	 * Connects two pins using native schema validation rules (TryCreateConnection).
	 */
	bool ConnectPins(
		UObject* AssetObject,
		const FString& GraphName,
		const FString& SourcePinSpec, // "NodeId:PinName"
		const FString& TargetPinSpec, // "NodeId:PinName"
		FString& ErrorMessage
	);

	/**
	 * Sets the literal default value on an unconnected input pin.
	 */
	bool SetPinDefaultValue(
		UObject* AssetObject,
		const FString& GraphName,
		const FString& PinSpec, // "NodeId:PinName"
		const FString& Value,
		FString& ErrorMessage
	);

	/**
	 * Deletes a node from a graph.
	 */
	bool DeleteNode(
		UObject* AssetObject,
		const FString& GraphName,
		const FString& NodeId,
		FString& ErrorMessage
	);

	/**
	 * Resolves a UEdGraph pointer from an asset object and optional graph name.
	 */
	UEdGraph* ResolveGraph(UObject* AssetObject, const FString& GraphName, FString& ErrorMessage) const;

private:
	FVAILGraphInspector();
	~FVAILGraphInspector();

	UEdGraphPin* FindPin(UEdGraphNode* Node, const FString& PinName, EEdGraphPinDirection Direction = EGPD_MAX) const;
	UEdGraphNode* FindNode(UEdGraph* Graph, const FString& NodeId) const;
};
