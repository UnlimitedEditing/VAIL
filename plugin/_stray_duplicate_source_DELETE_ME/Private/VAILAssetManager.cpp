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
