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

	/**
	 * Headlessly saves an asset or package to disk.
	 */
	bool SaveAsset(
		const FString& AssetPath,
		FString& ErrorMessage
	);

	/**
	 * Adds a new component to a Blueprint's Simple Construction Script (its Components panel),
	 * headlessly -- no Blueprint editor window needs to be open.
	 */
	bool AddComponent(
		const FString& AssetPath, // Blueprint asset path, e.g. "/Game/Blueprints/BP_Player"
		const FString& ComponentClass, // e.g. "SpringArmComponent", "CameraComponent", "StaticMeshComponent"
		const FString& ComponentName, // Name to give the new component node
		const FString& ParentComponentName, // Existing component to attach under; empty = attach to root
		const FString& AttachSocket, // Optional socket/bone name on the parent to attach at (e.g. SpringArm's "SpringEndpoint"); empty = parent's origin
		FString& OutCreatedComponentName,
		FString& ErrorMessage
	);

	/**
	 * Removes a component from a Blueprint's Simple Construction Script by variable name.
	 */
	bool RemoveComponent(
		const FString& AssetPath,
		const FString& ComponentName,
		FString& ErrorMessage
	);

	/**
	 * Maps a key to an Input Action on an Input Mapping Context, optionally applying named
	 * modifiers (e.g. "SwizzleYXZ", "Negate") to combine multiple 1D key presses into a 2D
	 * movement axis -- the same pattern the standard ThirdPerson template's IMC uses for WASD.
	 */
	bool AddInputKeyMapping(
		const FString& ContextAssetPath,
		const FString& ActionAssetPath,
		const FString& KeyName,
		const TArray<FString>& Modifiers,
		FString& ErrorMessage
	);

	/**
	 * Adds a member variable to a Blueprint. Supported VarType values: 'Float', 'Int',
	 * 'Bool', 'String', 'Vector'.
	 */
	bool AddVariable(
		const FString& AssetPath,
		const FString& VarName,
		const FString& VarType,
		const FString& DefaultValue,
		FString& ErrorMessage
	);

private:
	FVAILAssetManager();
	~FVAILAssetManager();
};
