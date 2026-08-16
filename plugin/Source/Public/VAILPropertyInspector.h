// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "IPropertyRowGenerator.h"
#include "PropertyEditorModule.h"
#include "PropertyHandle.h"
#include "IDetailTreeNode.h"

struct FVAILPropertyNode
{
	FString Id;                 // Semantic path (e.g. "RelativeLocation.X", "Mobility")
	FString Label;              // Display label (e.g. "Location X", "Mobility")
	FString Value;              // Formatted string value (e.g. "500.000", "Static")
	bool bIsEditable;
	bool bDiffersFromDefault;
	TArray<FString> Options;    // Available options for enums/combos
	TArray<FVAILPropertyNode> Children;
};

struct FVAILCategoryNode
{
	FString Category;
	TArray<FVAILPropertyNode> Properties;
};

struct FVAILObjectInspectionResult
{
	FString ObjectName;
	FString ObjectClass;
	TArray<FVAILCategoryNode> Categories;
};

class VAILCORE_API FVAILPropertyInspector
{
public:
	static FVAILPropertyInspector& Get();

	/** Inspects a target object headless and returns its categorized property tree */
	bool InspectObject(
		UObject* TargetObject,
		FVAILObjectInspectionResult& OutResult,
		int32 MaxDepth = 2,
		const FString& CategoryFilter = TEXT("")
	);

	/** Sets a property value via formatted string, capturing the old value for diffing */
	bool SetPropertyValue(
		UObject* TargetObject,
		const FString& PropertyPath,
		const FString& ValueAsString,
		FString& OutOldValue,
		FString& OutErrorMessage
	);

	/** Resets a property back to default CDO value */
	bool ResetPropertyToDefault(
		UObject* TargetObject,
		const FString& PropertyPath,
		FString& OutOldValue,
		FString& OutErrorMessage
	);

private:
	FVAILPropertyInspector();
	~FVAILPropertyInspector();

	void TraverseNode(
		TSharedRef<IDetailTreeNode> Node,
		TArray<FVAILPropertyNode>& OutProperties,
		int32 CurrentDepth,
		int32 MaxDepth
	);

	TSharedPtr<IPropertyHandle> FindPropertyHandleRecursive(
		const TArray<TSharedRef<IDetailTreeNode>>& Nodes,
		const FString& TargetPath
	);

	TSharedPtr<IPropertyRowGenerator> RowGenerator;
};
