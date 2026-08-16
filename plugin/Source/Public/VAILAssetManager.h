// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AssetRegistry/AssetData.h"

struct FVAILAssetInfo
{
	FString AssetName;
	FString PackagePath;
	FString AssetClass;
};

class VAILCORE_API FVAILAssetManager
{
public:
	static FVAILAssetManager& Get();

	/**
	 * Headless creation of new assets in the Content Browser (Blueprint, Material, Level, etc.)
	 */
	bool CreateAsset(
		const FString& AssetPath, // e.g. "/Game/Materials/M_Test"
		const FString& AssetClass, // "Material", "Blueprint", "MaterialInstanceConstant"
		const FString& ParentClass, // For Blueprints: e.g. "Actor", "Character", "Pawn"
		UObject*& OutAsset,
		FString& ErrorMessage
	);

	/**
	 * Duplicates an existing asset to a new path.
	 */
	bool DuplicateAsset(
		const FString& SourcePath,
		const FString& DestinationPath,
		FString& ErrorMessage
	);

	/**
	 * Queries the Asset Registry with optional package path and class filters.
	 */
	bool QueryAssets(
		const FString& PackagePath, // e.g. "/Game"
		const FString& ClassFilter, // e.g. "Blueprint", "Material"
		TArray<FVAILAssetInfo>& OutAssets,
		FString& ErrorMessage
	);

private:
	FVAILAssetManager();
	~FVAILAssetManager();
};
