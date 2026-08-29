// Copyright Epic Games, Inc. All Rights Reserved.

#include "VAILAssetManager.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Factories/MaterialFactoryNew.h"
#include "Factories/MaterialInstanceConstantFactoryNew.h"
#include "Materials/MaterialInstanceConstant.h"
#include "FileHelpers.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "UObject/Package.h"
#include "WidgetBlueprint.h"
#include "WidgetBlueprintFactory.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#include "Components/ActorComponent.h"
#include "Components/SceneComponent.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "InputCoreTypes.h"
#include "EdGraphSchema_K2.h"

FVAILAssetManager& FVAILAssetManager::Get()
{
	static FVAILAssetManager Singleton;
	return Singleton;
}

FVAILAssetManager::FVAILAssetManager()
{
}

FVAILAssetManager::~FVAILAssetManager()
{
}

bool FVAILAssetManager::CreateAsset(
	const FString& AssetPath,
	const FString& AssetClass,
	const FString& ParentClass,
	UObject*& OutAsset,
	FString& ErrorMessage)
{
	OutAsset = nullptr;

	FString PackageName;
	FString AssetName;
	if (!AssetPath.Split(TEXT("/"), &PackageName, &AssetName, ESearchCase::IgnoreCase, ESearchDir::FromEnd))
	{
		PackageName = TEXT("/Game");
		AssetName = AssetPath;
	}
	else
	{
		if (PackageName.IsEmpty())
		{
			PackageName = TEXT("/Game");
		}
	}

	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();

	// 1. Create Blueprint Asset
	if (AssetClass.Equals(TEXT("Blueprint"), ESearchCase::IgnoreCase) ||
		AssetClass.Equals(TEXT("BP"), ESearchCase::IgnoreCase))
	{
		UClass* ParentUClass = AActor::StaticClass();
		if (!ParentClass.IsEmpty())
		{
			if (ParentClass.Equals(TEXT("Character"), ESearchCase::IgnoreCase))
			{
				ParentUClass = ACharacter::StaticClass();
			}
			else if (ParentClass.Equals(TEXT("Pawn"), ESearchCase::IgnoreCase))
			{
				ParentUClass = APawn::StaticClass();
			}
			else
			{
				// Resolve generic parent class
				for (TObjectIterator<UClass> It; It; ++It)
				{
					if (It->IsChildOf(AActor::StaticClass()) &&
						(It->GetName().Equals(ParentClass, ESearchCase::IgnoreCase) ||
						 It->GetName().Equals(TEXT("A") + ParentClass, ESearchCase::IgnoreCase)))
					{
						ParentUClass = *It;
						break;
					}
				}
			}
		}

		UBlueprint* NewBP = FKismetEditorUtilities::CreateBlueprint(
			ParentUClass,
			CreatePackage(*(PackageName / AssetName)),
			FName(*AssetName),
			BPTYPE_Normal,
			UBlueprint::StaticClass(),
			UBlueprintGeneratedClass::StaticClass()
		);

		if (!NewBP)
		{
			ErrorMessage = FString::Printf(TEXT("Failed to create Blueprint at '%s/%s'"), *PackageName, *AssetName);
			return false;
		}

		FAssetRegistryModule::AssetCreated(NewBP);
		NewBP->MarkPackageDirty();
		OutAsset = NewBP;
		return true;
	}

	// 2. Create Material Asset
	if (AssetClass.Equals(TEXT("Material"), ESearchCase::IgnoreCase) ||
		AssetClass.Equals(TEXT("M"), ESearchCase::IgnoreCase))
	{
		UMaterialFactoryNew* Factory = NewObject<UMaterialFactoryNew>();
		UObject* NewMat = AssetTools.CreateAsset(
			AssetName,
			PackageName,
			UMaterial::StaticClass(),
			Factory
		);

		if (!NewMat)
		{
			ErrorMessage = FString::Printf(TEXT("Failed to create Material at '%s/%s'"), *PackageName, *AssetName);
			return false;
		}

		FAssetRegistryModule::AssetCreated(NewMat);
		NewMat->MarkPackageDirty();
		OutAsset = NewMat;
		return true;
	}

	// 3. Create Material Instance Constant
	if (AssetClass.Equals(TEXT("MaterialInstance"), ESearchCase::IgnoreCase) ||
		AssetClass.Equals(TEXT("MaterialInstanceConstant"), ESearchCase::IgnoreCase) ||
		AssetClass.Equals(TEXT("MI"), ESearchCase::IgnoreCase))
	{
		UMaterialInstanceConstantFactoryNew* Factory = NewObject<UMaterialInstanceConstantFactoryNew>();
		UObject* NewMI = AssetTools.CreateAsset(
			AssetName,
			PackageName,
			UMaterialInstanceConstant::StaticClass(),
			Factory
		);

		if (!NewMI)
		{
			ErrorMessage = FString::Printf(TEXT("Failed to create MaterialInstance at '%s/%s'"), *PackageName, *AssetName);
			return false;
		}

		FAssetRegistryModule::AssetCreated(NewMI);
		NewMI->MarkPackageDirty();
		OutAsset = NewMI;
		return true;
	}

	// 4. Create Widget Blueprint (UMG)
	if (AssetClass.Equals(TEXT("WidgetBlueprint"), ESearchCase::IgnoreCase) ||
		AssetClass.Equals(TEXT("Widget"), ESearchCase::IgnoreCase) ||
		AssetClass.Equals(TEXT("WBP"), ESearchCase::IgnoreCase))
	{
		UWidgetBlueprintFactory* Factory = NewObject<UWidgetBlueprintFactory>();
		UObject* NewWidget = AssetTools.CreateAsset(
			AssetName,
			PackageName,
			UWidgetBlueprint::StaticClass(),
			Factory
		);

		UWidgetBlueprint* NewWidgetBP = Cast<UWidgetBlueprint>(NewWidget);
		if (!NewWidgetBP)
		{
			ErrorMessage = FString::Printf(TEXT("Failed to create WidgetBlueprint at '%s/%s'"), *PackageName, *AssetName);
			return false;
		}

		// UWidgetBlueprintFactory normally seeds a root CanvasPanel itself, but
		// guarantee it here too -- vail_widget_add_element needs a root panel
		// to parent onto, and a widget tree with no root would silently break
		// every widget tool downstream of this one.
		if (!NewWidgetBP->WidgetTree->RootWidget)
		{
			UCanvasPanel* RootCanvas = NewWidgetBP->WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
			NewWidgetBP->WidgetTree->RootWidget = RootCanvas;
		}

		FKismetEditorUtilities::CompileBlueprint(NewWidgetBP);
		FAssetRegistryModule::AssetCreated(NewWidgetBP);
		NewWidgetBP->MarkPackageDirty();
		OutAsset = NewWidgetBP;
		return true;
	}

	// 5. Create Input Action / Input Mapping Context -- plain UObject-derived data assets
	// with no dedicated public UFactory, so they're created directly via NewObject + the
	// same AssetRegistry/package registration every UFactory does under the hood.
	if (AssetClass.Equals(TEXT("InputAction"), ESearchCase::IgnoreCase))
	{
		UPackage* Package = CreatePackage(*(PackageName / AssetName));
		UInputAction* NewAction = NewObject<UInputAction>(Package, FName(*AssetName), RF_Public | RF_Standalone);
		if (!NewAction)
		{
			ErrorMessage = FString::Printf(TEXT("Failed to create InputAction at '%s/%s'"), *PackageName, *AssetName);
			return false;
		}
		FAssetRegistryModule::AssetCreated(NewAction);
		NewAction->MarkPackageDirty();
		OutAsset = NewAction;
		return true;
	}

	if (AssetClass.Equals(TEXT("InputMappingContext"), ESearchCase::IgnoreCase))
	{
		UPackage* Package = CreatePackage(*(PackageName / AssetName));
		UInputMappingContext* NewContext = NewObject<UInputMappingContext>(Package, FName(*AssetName), RF_Public | RF_Standalone);
		if (!NewContext)
		{
			ErrorMessage = FString::Printf(TEXT("Failed to create InputMappingContext at '%s/%s'"), *PackageName, *AssetName);
			return false;
		}
		FAssetRegistryModule::AssetCreated(NewContext);
		NewContext->MarkPackageDirty();
		OutAsset = NewContext;
		return true;
	}

	ErrorMessage = FString::Printf(TEXT("Asset class '%s' is not supported for automated creation"), *AssetClass);
	return false;
}

bool FVAILAssetManager::DuplicateAsset(
	const FString& SourcePath,
	const FString& DestinationPath,
	FString& ErrorMessage)
{
	FString SourcePackage, SourceName;
	if (!SourcePath.Split(TEXT("/"), &SourcePackage, &SourceName, ESearchCase::IgnoreCase, ESearchDir::FromEnd))
	{
		SourceName = SourcePath;
	}

	FString DestPackage, DestName;
	if (!DestinationPath.Split(TEXT("/"), &DestPackage, &DestName, ESearchCase::IgnoreCase, ESearchDir::FromEnd))
	{
		DestPackage = TEXT("/Game");
		DestName = DestinationPath;
	}

	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	UObject* Duplicated = AssetTools.DuplicateAsset(DestName, DestPackage, LoadObject<UObject>(nullptr, *SourcePath));
	if (!Duplicated)
	{
		ErrorMessage = FString::Printf(TEXT("Failed to duplicate asset from '%s' to '%s'"), *SourcePath, *DestinationPath);
		return false;
	}

	FAssetRegistryModule::AssetCreated(Duplicated);
	Duplicated->MarkPackageDirty();
	return true;
}

bool FVAILAssetManager::QueryAssets(
	const FString& PackagePath,
	const FString& ClassFilter,
	TArray<FVAILAssetInfo>& OutAssets,
	FString& ErrorMessage)
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	TArray<FAssetData> AssetDataList;

	FARFilter Filter;
	if (!PackagePath.IsEmpty())
	{
		Filter.PackagePaths.Add(FName(*PackagePath));
		Filter.bRecursivePaths = true;
	}
	else
	{
		Filter.PackagePaths.Add(TEXT("/Game"));
		Filter.bRecursivePaths = true;
	}

	AssetRegistryModule.Get().GetAssets(Filter, AssetDataList);

	FString ClassFilterLower = ClassFilter.ToLower();

	for (const FAssetData& Data : AssetDataList)
	{
		FString AssetClassName = Data.AssetClassPath.GetAssetName().ToString();
		if (!ClassFilterLower.IsEmpty() && !AssetClassName.ToLower().Equals(ClassFilterLower))
		{
			continue;
		}

		FVAILAssetInfo Info;
		Info.AssetName = Data.AssetName.ToString();
		Info.PackagePath = Data.PackagePath.ToString();
		Info.AssetClass = AssetClassName;
		OutAssets.Add(Info);
	}

	return true;
}

bool FVAILAssetManager::SaveAsset(
	const FString& AssetPath,
	FString& ErrorMessage)
{
	UPackage* PackageToSave = nullptr;
	if (UObject* TargetObject = LoadObject<UObject>(nullptr, *AssetPath))
	{
		PackageToSave = TargetObject->GetOutermost();
	}
	else
	{
		PackageToSave = FindPackage(nullptr, *AssetPath);
	}

	if (!PackageToSave)
	{
		ErrorMessage = FString::Printf(TEXT("Could not find package for '%s'"), *AssetPath);
		return false;
	}

	TArray<UPackage*> PackagesToSave;
	PackagesToSave.Add(PackageToSave);

	FEditorFileUtils::EPromptReturnCode ReturnCode = FEditorFileUtils::PromptForCheckoutAndSave(
		PackagesToSave,
		false, // bCheckDirty
		false  // bPromptToSave
	);

	if (ReturnCode != FEditorFileUtils::PR_Success)
	{
		ErrorMessage = FString::Printf(TEXT("Failed to save package '%s'"), *AssetPath);
		return false;
	}

	return true;
}

bool FVAILAssetManager::AddComponent(
	const FString& AssetPath,
	const FString& ComponentClass,
	const FString& ComponentName,
	const FString& ParentComponentName,
	const FString& AttachSocket,
	FString& OutCreatedComponentName,
	FString& ErrorMessage)
{
	UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, *AssetPath);
	if (!Blueprint)
	{
		ErrorMessage = FString::Printf(TEXT("Could not load Blueprint at '%s'"), *AssetPath);
		return false;
	}

	if (!Blueprint->SimpleConstructionScript)
	{
		ErrorMessage = FString::Printf(TEXT("Blueprint '%s' has no SimpleConstructionScript (not an Actor-based Blueprint?)"), *AssetPath);
		return false;
	}

	// Resolve the component class by short name ("SpringArmComponent", "Camera") or full
	// engine name ("USpringArmComponent") -- mirrors the parent-class resolution in CreateAsset.
	UClass* CompClass = nullptr;
	if (!ComponentClass.IsEmpty())
	{
		for (TObjectIterator<UClass> It; It; ++It)
		{
			if (!It->IsChildOf(UActorComponent::StaticClass()))
			{
				continue;
			}
			const FString ClassName = It->GetName();
			if (ClassName.Equals(ComponentClass, ESearchCase::IgnoreCase) ||
				ClassName.Equals(TEXT("U") + ComponentClass, ESearchCase::IgnoreCase) ||
				ClassName.Equals(ComponentClass + TEXT("Component"), ESearchCase::IgnoreCase) ||
				ClassName.Equals(TEXT("U") + ComponentClass + TEXT("Component"), ESearchCase::IgnoreCase))
			{
				CompClass = *It;
				break;
			}
		}
	}

	if (!CompClass)
	{
		ErrorMessage = FString::Printf(TEXT("Could not resolve component class '%s'"), *ComponentClass);
		return false;
	}

	USimpleConstructionScript* SCS = Blueprint->SimpleConstructionScript;

	FName DesiredName = ComponentName.IsEmpty() ? FName(*CompClass->GetName()) : FName(*ComponentName);
	DesiredName = FBlueprintEditorUtils::FindUniqueKismetName(Blueprint, DesiredName.ToString());

	USCS_Node* NewNode = SCS->CreateNode(CompClass, DesiredName);
	if (!NewNode)
	{
		ErrorMessage = FString::Printf(TEXT("Failed to create component node '%s' of class '%s'"), *DesiredName.ToString(), *ComponentClass);
		return false;
	}

	if (!AttachSocket.IsEmpty())
	{
		// e.g. attaching a Camera to a SpringArm must target the arm's "SpringEndpoint"
		// socket, not its origin, or the camera ends up sitting at the pawn's pivot.
		NewNode->AttachToName = FName(*AttachSocket);
	}

	bool bAttached = false;
	if (!ParentComponentName.IsEmpty())
	{
		// First try an existing node already in this Blueprint's own SCS tree.
		for (USCS_Node* Node : SCS->GetAllNodes())
		{
			if (Node->GetVariableName().ToString().Equals(ParentComponentName, ESearchCase::IgnoreCase))
			{
				Node->AddChildNode(NewNode);
				bAttached = true;
				break;
			}
		}

		// Fall back to attaching under an inherited native component (e.g. Character's
		// CapsuleComponent root, which lives on the C++ parent class, not in this SCS).
		// USCS_Node has no SetParent(name, class) overload -- an inherited-native parent is
		// recorded directly on these fields instead (mirrors what FKismetEditorUtilities'
		// internal AddNewComponent flow does for the "attach to inherited component" case).
		if (!bAttached)
		{
			NewNode->ParentComponentOrVariableName = FName(*ParentComponentName);
			NewNode->bIsParentComponentNative = true;
			SCS->AddNode(NewNode);
			bAttached = true;
		}
	}
	else
	{
		// No explicit parent: attach under the Blueprint's existing root (native or SCS) so
		// the new component follows the actor instead of floating as a disconnected root.
		if (USCS_Node* RootNode = SCS->GetDefaultSceneRootNode())
		{
			if (CompClass->IsChildOf(USceneComponent::StaticClass()))
			{
				RootNode->AddChildNode(NewNode);
			}
			else
			{
				SCS->AddNode(NewNode);
			}
		}
		else
		{
			SCS->AddNode(NewNode);
		}
		bAttached = true;
	}

	FKismetEditorUtilities::CompileBlueprint(Blueprint);
	Blueprint->MarkPackageDirty();

	OutCreatedComponentName = NewNode->GetVariableName().ToString();
	return true;
}

bool FVAILAssetManager::RemoveComponent(
	const FString& AssetPath,
	const FString& ComponentName,
	FString& ErrorMessage)
{
	UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, *AssetPath);
	if (!Blueprint || !Blueprint->SimpleConstructionScript)
	{
		ErrorMessage = FString::Printf(TEXT("Could not load Blueprint (or its SCS) at '%s'"), *AssetPath);
		return false;
	}

	USimpleConstructionScript* SCS = Blueprint->SimpleConstructionScript;
	for (USCS_Node* Node : SCS->GetAllNodes())
	{
		if (Node->GetVariableName().ToString().Equals(ComponentName, ESearchCase::IgnoreCase))
		{
			SCS->RemoveNode(Node);
			FKismetEditorUtilities::CompileBlueprint(Blueprint);
			Blueprint->MarkPackageDirty();
			return true;
		}
	}

	ErrorMessage = FString::Printf(TEXT("No SCS component named '%s' found on '%s'"), *ComponentName, *AssetPath);
	return false;
}

bool FVAILAssetManager::AddInputKeyMapping(
	const FString& ContextAssetPath,
	const FString& ActionAssetPath,
	const FString& KeyName,
	const TArray<FString>& Modifiers,
	FString& ErrorMessage)
{
	UInputMappingContext* Context = LoadObject<UInputMappingContext>(nullptr, *ContextAssetPath);
	if (!Context)
	{
		ErrorMessage = FString::Printf(TEXT("Could not load InputMappingContext at '%s'"), *ContextAssetPath);
		return false;
	}

	UInputAction* Action = LoadObject<UInputAction>(nullptr, *ActionAssetPath);
	if (!Action)
	{
		ErrorMessage = FString::Printf(TEXT("Could not load InputAction at '%s'"), *ActionAssetPath);
		return false;
	}

	const FKey Key = FName(*KeyName);
	if (!Key.IsValid())
	{
		ErrorMessage = FString::Printf(TEXT("'%s' is not a recognized key name"), *KeyName);
		return false;
	}

	Context->Modify();
	FEnhancedActionKeyMapping& Mapping = Context->MapKey(Action, Key);

	for (const FString& ModifierName : Modifiers)
	{
		UInputModifier* NewModifier = nullptr;
		if (ModifierName.Equals(TEXT("Negate"), ESearchCase::IgnoreCase))
		{
			NewModifier = NewObject<UInputModifierNegate>(Context);
		}
		else if (ModifierName.Equals(TEXT("SwizzleYXZ"), ESearchCase::IgnoreCase) || ModifierName.Equals(TEXT("Swizzle"), ESearchCase::IgnoreCase))
		{
			NewModifier = NewObject<UInputModifierSwizzleAxis>(Context);
		}
		else
		{
			ErrorMessage = FString::Printf(TEXT("Unrecognized modifier '%s' (supported: Negate, SwizzleYXZ)"), *ModifierName);
			return false;
		}
		Mapping.Modifiers.Add(NewModifier);
	}

	Context->MarkPackageDirty();
	return true;
}

bool FVAILAssetManager::AddVariable(
	const FString& AssetPath,
	const FString& VarName,
	const FString& VarType,
	const FString& DefaultValue,
	FString& ErrorMessage)
{
	UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, *AssetPath);
	if (!Blueprint)
	{
		ErrorMessage = FString::Printf(TEXT("Could not load Blueprint at '%s'"), *AssetPath);
		return false;
	}

	FEdGraphPinType PinType;
	if (VarType.Equals(TEXT("Float"), ESearchCase::IgnoreCase))
	{
		PinType.PinCategory = UEdGraphSchema_K2::PC_Real;
		PinType.PinSubCategory = UEdGraphSchema_K2::PC_Double;
	}
	else if (VarType.Equals(TEXT("Int"), ESearchCase::IgnoreCase) || VarType.Equals(TEXT("Integer"), ESearchCase::IgnoreCase))
	{
		PinType.PinCategory = UEdGraphSchema_K2::PC_Int;
	}
	else if (VarType.Equals(TEXT("Bool"), ESearchCase::IgnoreCase) || VarType.Equals(TEXT("Boolean"), ESearchCase::IgnoreCase))
	{
		PinType.PinCategory = UEdGraphSchema_K2::PC_Boolean;
	}
	else if (VarType.Equals(TEXT("String"), ESearchCase::IgnoreCase))
	{
		PinType.PinCategory = UEdGraphSchema_K2::PC_String;
	}
	else if (VarType.Equals(TEXT("Vector"), ESearchCase::IgnoreCase))
	{
		PinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
		PinType.PinSubCategoryObject = TBaseStructure<FVector>::Get();
	}
	else
	{
		ErrorMessage = FString::Printf(TEXT("Unsupported variable type '%s' (supported: Float, Int, Bool, String, Vector)"), *VarType);
		return false;
	}

	if (!FBlueprintEditorUtils::AddMemberVariable(Blueprint, FName(*VarName), PinType, DefaultValue))
	{
		ErrorMessage = FString::Printf(TEXT("Failed to add variable '%s' to '%s'"), *VarName, *AssetPath);
		return false;
	}

	Blueprint->MarkPackageDirty();
	return true;
}
