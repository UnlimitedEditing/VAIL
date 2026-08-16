// Copyright Epic Games, Inc. All Rights Reserved.

#include "VAILPropertyInspector.h"
#include "VAILCoreModule.h"

FVAILPropertyInspector& FVAILPropertyInspector::Get()
{
	static FVAILPropertyInspector Singleton;
	return Singleton;
}

FVAILPropertyInspector::FVAILPropertyInspector()
{
	FPropertyRowGeneratorArgs Args;
	Args.bAllowEditingClassDefaultObjects = true;
	Args.bShouldShowHiddenProperties = false;
	Args.DefaultsOnlyVisibility = EEditDefaultsOnlyNodeVisibility::Show;

	FPropertyEditorModule& PropertyEditorModule = 
		FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

	RowGenerator = PropertyEditorModule.CreatePropertyRowGenerator(Args);
}

FVAILPropertyInspector::~FVAILPropertyInspector()
{
}

bool FVAILPropertyInspector::InspectObject(
	UObject* TargetObject,
	FVAILObjectInspectionResult& OutResult,
	int32 MaxDepth,
	const FString& CategoryFilter)
{
	check(IsInGameThread());

	if (!TargetObject)
	{
		return false;
	}

	if (!RowGenerator.IsValid())
	{
		return false;
	}

	RowGenerator->SetObjects({ TargetObject });

	OutResult.ObjectName = TargetObject->GetName();
	OutResult.ObjectClass = TargetObject->GetClass()->GetName();
	OutResult.Categories.Empty();

	const TArray<TSharedRef<IDetailTreeNode>>& RootNodes = RowGenerator->GetRootTreeNodes();

	for (const TSharedRef<IDetailTreeNode>& RootNode : RootNodes)
	{
		if (RootNode->GetNodeType() == EDetailNodeType::Category)
		{
			const FString CategoryName = RootNode->GetNodeName().ToString();
			if (!CategoryFilter.IsEmpty() && !CategoryName.Contains(CategoryFilter))
			{
				continue;
			}

			FVAILCategoryNode CatNode;
			CatNode.Category = CategoryName;

			TArray<TSharedRef<IDetailTreeNode>> Children;
			RootNode->GetChildren(Children);

			for (const TSharedRef<IDetailTreeNode>& Child : Children)
			{
				TraverseNode(Child, CatNode.Properties, 1, MaxDepth);
			}

			if (CatNode.Properties.Num() > 0)
			{
				OutResult.Categories.Add(CatNode);
			}
		}
	}

	return true;
}

void FVAILPropertyInspector::TraverseNode(
	TSharedRef<IDetailTreeNode> Node,
	TArray<FVAILPropertyNode>& OutProperties,
	int32 CurrentDepth,
	int32 MaxDepth)
{
	if (CurrentDepth > MaxDepth)
	{
		return;
	}

	TSharedPtr<IPropertyHandle> Handle = Node->CreatePropertyHandle();
	if (Handle.IsValid() && Handle->IsValidHandle())
	{
		FVAILPropertyNode PropNode;
		PropNode.Id = FString(Handle->GetPropertyPath());
		PropNode.Label = Handle->GetPropertyDisplayName().ToString();
		Handle->GetValueAsFormattedString(PropNode.Value);
		PropNode.bIsEditable = Handle->IsEditable();
		PropNode.bDiffersFromDefault = Handle->DiffersFromDefault();

		TArray<FString> OptionStrings;
		TArray<FText> Tooltips;
		TArray<bool> Restricted;
		if (Handle->GeneratePossibleValues(OptionStrings, Tooltips, Restricted) && OptionStrings.Num() > 0)
		{
			PropNode.Options = OptionStrings;
		}

		TArray<TSharedRef<IDetailTreeNode>> Children;
		Node->GetChildren(Children);
		if (Children.Num() > 0 && CurrentDepth < MaxDepth)
		{
			for (const TSharedRef<IDetailTreeNode>& ChildNode : Children)
			{
				TraverseNode(ChildNode, PropNode.Children, CurrentDepth + 1, MaxDepth);
			}
		}

		OutProperties.Add(PropNode);
	}
}

bool FVAILPropertyInspector::SetPropertyValue(
	UObject* TargetObject,
	const FString& PropertyPath,
	const FString& ValueAsString,
	FString& OutOldValue,
	FString& OutErrorMessage)
{
	check(IsInGameThread());

	if (!TargetObject)
	{
		OutErrorMessage = TEXT("TargetObject is null");
		return false;
	}

	if (!RowGenerator.IsValid())
	{
		OutErrorMessage = TEXT("RowGenerator is invalid");
		return false;
	}

	RowGenerator->SetObjects({ TargetObject });
	const TArray<TSharedRef<IDetailTreeNode>>& RootNodes = RowGenerator->GetRootTreeNodes();

	TSharedPtr<IPropertyHandle> Handle = FindPropertyHandleRecursive(RootNodes, PropertyPath);
	if (!Handle.IsValid() || !Handle->IsValidHandle())
	{
		OutErrorMessage = FString::Printf(TEXT("Property '%s' not found on object '%s'"), *PropertyPath, *TargetObject->GetName());
		return false;
	}

	if (!Handle->IsEditable())
	{
		OutErrorMessage = FString::Printf(TEXT("Property '%s' is read-only (EditConst)"), *PropertyPath);
		return false;
	}

	Handle->GetValueAsFormattedString(OutOldValue);

	const FPropertyAccess::Result Result = Handle->SetValueFromFormattedString(
		ValueAsString,
		EPropertyValueSetFlags::DefaultFlags
	);

	if (Result != FPropertyAccess::Success)
	{
		OutErrorMessage = FString::Printf(TEXT("Failed to set property '%s' to value '%s' (Result code: %d)"), *PropertyPath, *ValueAsString, (int32)Result);
		return false;
	}

	return true;
}

bool FVAILPropertyInspector::ResetPropertyToDefault(
	UObject* TargetObject,
	const FString& PropertyPath,
	FString& OutOldValue,
	FString& OutErrorMessage)
{
	check(IsInGameThread());

	if (!TargetObject)
	{
		OutErrorMessage = TEXT("TargetObject is null");
		return false;
	}

	RowGenerator->SetObjects({ TargetObject });
	const TArray<TSharedRef<IDetailTreeNode>>& RootNodes = RowGenerator->GetRootTreeNodes();

	TSharedPtr<IPropertyHandle> Handle = FindPropertyHandleRecursive(RootNodes, PropertyPath);
	if (!Handle.IsValid() || !Handle->IsValidHandle())
	{
		OutErrorMessage = FString::Printf(TEXT("Property '%s' not found"), *PropertyPath);
		return false;
	}

	Handle->GetValueAsFormattedString(OutOldValue);
	Handle->ResetToDefault();

	return true;
}

TSharedPtr<IPropertyHandle> FVAILPropertyInspector::FindPropertyHandleRecursive(
	const TArray<TSharedRef<IDetailTreeNode>>& Nodes,
	const FString& TargetPath)
{
	for (const TSharedRef<IDetailTreeNode>& Node : Nodes)
	{
		TSharedPtr<IPropertyHandle> Handle = Node->CreatePropertyHandle();
		if (Handle.IsValid() && Handle->IsValidHandle())
		{
			if (FString(Handle->GetPropertyPath()) == TargetPath ||
				Handle->GetPropertyDisplayName().ToString() == TargetPath)
			{
				return Handle;
			}
		}

		TArray<TSharedRef<IDetailTreeNode>> Children;
		Node->GetChildren(Children);
		if (Children.Num() > 0)
		{
			TSharedPtr<IPropertyHandle> Found = FindPropertyHandleRecursive(Children, TargetPath);
			if (Found.IsValid())
			{
				return Found;
			}
		}
	}
	return nullptr;
}
