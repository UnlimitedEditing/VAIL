// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/UICommandInfo.h"
#include "Framework/Commands/UICommandList.h"
#include "Framework/Commands/InputBindingManager.h"

struct FVAILCommandDescriptor
{
	FString SemanticId;         // e.g. "Kismet.Compile", "LevelEditor.Save"
	FName ContextName;
	FName CommandName;
	FText Label;
	FText Description;
	FString InputChord;
	bool bIsActionMapped;
};

class VAILCORE_API FVAILIdentityRegistry
{
public:
	static FVAILIdentityRegistry& Get();

	void Initialize();
	void Shutdown();

	/** Refreshes static catalog of known command contexts and descriptors */
	void RebuildStaticCommandCatalog();

	/** Searches known commands by query (matches SemanticId, Label, or Description) */
	TArray<FVAILCommandDescriptor> FindCommandsByQuery(const FString& Query, const FString& ContextScope = TEXT("")) const;

	/** Gets a specific command descriptor by its SemanticId */
	bool GetCommandDescriptor(const FString& SemanticId, FVAILCommandDescriptor& OutDescriptor) const;

	/** Executes a registered command directly without synthetic mouse or window focus */
	bool ExecuteCommand(const FString& SemanticId, FString& OutErrorMessage);

private:
	FVAILIdentityRegistry();
	~FVAILIdentityRegistry();

	void HandleCommandListRegistered(const FName ContextName, TSharedRef<FUICommandList> CommandList);
	void HandleCommandListUnregistered(const FName ContextName, TSharedRef<FUICommandList> CommandList);

	// Static catalog of command descriptors keyed by SemanticId
	TMap<FString, FVAILCommandDescriptor> StaticCommandCatalog;

	// Active command lists registered by editor panels/viewports
	TMap<FName, TWeakPtr<FUICommandList>> ActiveCommandLists;

	bool bInitialized;
};
