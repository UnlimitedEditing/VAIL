// Copyright Epic Games, Inc. All Rights Reserved.

#include "VAILIdentityRegistry.h"
#include "VAILCoreModule.h"

FVAILIdentityRegistry& FVAILIdentityRegistry::Get()
{
	static FVAILIdentityRegistry Singleton;
	return Singleton;
}

FVAILIdentityRegistry::FVAILIdentityRegistry()
	: bInitialized(false)
{
}

FVAILIdentityRegistry::~FVAILIdentityRegistry()
{
	Shutdown();
}

void FVAILIdentityRegistry::Initialize()
{
	if (bInitialized)
	{
		return;
	}

	bInitialized = true;

	// Subscribe to command list registrations across the editor
	FInputBindingManager::Get().OnRegisterCommandList.AddRaw(
		this, &FVAILIdentityRegistry::HandleCommandListRegistered
	);
	FInputBindingManager::Get().OnUnregisterCommandList.AddRaw(
		this, &FVAILIdentityRegistry::HandleCommandListUnregistered
	);

	// Build initial static catalog
	RebuildStaticCommandCatalog();

	UE_LOG(LogVAILCore, Log, TEXT("VAILIdentityRegistry: Initialized with %d registered commands"), StaticCommandCatalog.Num());
}

void FVAILIdentityRegistry::Shutdown()
{
	if (!bInitialized)
	{
		return;
	}

	bInitialized = false;

	if (FInputBindingManager* BindingManager = &FInputBindingManager::Get())
	{
		BindingManager->OnRegisterCommandList.RemoveAll(this);
		BindingManager->OnUnregisterCommandList.RemoveAll(this);
	}

	ActiveCommandLists.Empty();
	StaticCommandCatalog.Empty();
}

void FVAILIdentityRegistry::HandleCommandListRegistered(const FName ContextName, TSharedRef<FUICommandList> CommandList)
{
	ActiveCommandLists.FindOrAdd(ContextName) = CommandList;
	UE_LOG(LogVAILCore, Verbose, TEXT("VAILIdentityRegistry: CommandList registered for context: %s"), *ContextName.ToString());
}

void FVAILIdentityRegistry::HandleCommandListUnregistered(const FName ContextName, TSharedRef<FUICommandList> CommandList)
{
	if (TWeakPtr<FUICommandList>* Found = ActiveCommandLists.Find(ContextName))
	{
		if (Found->HasSameObject(&CommandList.Get()))
		{
			ActiveCommandLists.Remove(ContextName);
			UE_LOG(LogVAILCore, Verbose, TEXT("VAILIdentityRegistry: CommandList unregistered for context: %s"), *ContextName.ToString());
		}
	}
}

void FVAILIdentityRegistry::RebuildStaticCommandCatalog()
{
	StaticCommandCatalog.Empty();

	TArray<TSharedPtr<FBindingContext>> Contexts;
	FInputBindingManager::Get().GetKnownInputContexts(Contexts);

	for (const TSharedPtr<FBindingContext>& Context : Contexts)
	{
		if (!Context.IsValid())
		{
			continue;
		}

		const FName ContextName = Context->GetContextName();
		TArray<TSharedPtr<FUICommandInfo>> Commands;
		FInputBindingManager::Get().GetCommandInfosFromContext(ContextName, Commands);

		for (const TSharedPtr<FUICommandInfo>& CommandInfo : Commands)
		{
			if (!CommandInfo.IsValid())
			{
				continue;
			}

			FVAILCommandDescriptor Desc;
			Desc.ContextName = ContextName;
			Desc.CommandName = CommandInfo->GetCommandName();
			Desc.SemanticId = FString::Printf(TEXT("%s.%s"), *ContextName.ToString(), *Desc.CommandName.ToString());
			Desc.Label = CommandInfo->GetLabel();
			Desc.Description = CommandInfo->GetDescription();
			Desc.InputChord = CommandInfo->GetInputText().ToString();
			Desc.bIsActionMapped = false;

			StaticCommandCatalog.Add(Desc.SemanticId, Desc);
		}
	}
}

TArray<FVAILCommandDescriptor> FVAILIdentityRegistry::FindCommandsByQuery(const FString& Query, const FString& ContextScope) const
{
	TArray<FVAILCommandDescriptor> Results;
	const FString LowerQuery = Query.ToLower();

	for (const auto& Pair : StaticCommandCatalog)
	{
		const FVAILCommandDescriptor& Desc = Pair.Value;

		if (!ContextScope.IsEmpty() && !Desc.ContextName.ToString().Contains(ContextScope))
		{
			continue;
		}

		if (Desc.SemanticId.ToLower().Contains(LowerQuery) ||
			Desc.Label.ToString().ToLower().Contains(LowerQuery) ||
			Desc.Description.ToString().ToLower().Contains(LowerQuery))
		{
			FVAILCommandDescriptor ResultDesc = Desc;
			if (const TWeakPtr<FUICommandList>* FoundList = ActiveCommandLists.Find(Desc.ContextName))
			{
				ResultDesc.bIsActionMapped = FoundList->IsValid();
			}
			Results.Add(ResultDesc);
		}
	}

	return Results;
}

bool FVAILIdentityRegistry::GetCommandDescriptor(const FString& SemanticId, FVAILCommandDescriptor& OutDescriptor) const
{
	if (const FVAILCommandDescriptor* Found = StaticCommandCatalog.Find(SemanticId))
	{
		OutDescriptor = *Found;
		if (const TWeakPtr<FUICommandList>* FoundList = ActiveCommandLists.Find(Found->ContextName))
		{
			OutDescriptor.bIsActionMapped = FoundList->IsValid();
		}
		return true;
	}
	return false;
}

bool FVAILIdentityRegistry::ExecuteCommand(const FString& SemanticId, FString& OutErrorMessage)
{
	check(IsInGameThread());

	FString ContextStr;
	FString CommandStr;
	if (!SemanticId.Split(TEXT("."), &ContextStr, &CommandStr))
	{
		OutErrorMessage = FString::Printf(TEXT("Invalid SemanticId format: %s. Expected 'Context.Command'"), *SemanticId);
		return false;
	}

	const FName ContextName(*ContextStr);
	const FName CommandName(*CommandStr);

	const TSharedPtr<FUICommandInfo> CommandInfo = FInputBindingManager::Get().FindCommandInContext(ContextName, CommandName);
	if (!CommandInfo.IsValid())
	{
		OutErrorMessage = FString::Printf(TEXT("Command '%s' not found in context '%s'"), *CommandStr, *ContextStr);
		return false;
	}

	const TWeakPtr<FUICommandList>* FoundList = ActiveCommandLists.Find(ContextName);
	if (!FoundList || !FoundList->IsValid())
	{
		OutErrorMessage = FString::Printf(TEXT("No active FUICommandList currently registered for context '%s'"), *ContextStr);
		return false;
	}

	const TSharedPtr<FUICommandList> CommandList = FoundList->Pin();
	if (!CommandList->CanExecuteAction(CommandInfo.ToSharedRef()))
	{
		OutErrorMessage = FString::Printf(TEXT("Command '%s' cannot be executed in current editor state (CanExecuteAction = false)"), *SemanticId);
		return false;
	}

	const bool bSuccess = CommandList->ExecuteAction(CommandInfo.ToSharedRef());
	if (!bSuccess)
	{
		OutErrorMessage = FString::Printf(TEXT("ExecuteAction returned false for command '%s'"), *SemanticId);
		return false;
	}

	return true;
}
