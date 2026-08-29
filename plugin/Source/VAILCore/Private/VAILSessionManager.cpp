// Copyright Epic Games, Inc. All Rights Reserved.

#include "VAILSessionManager.h"
#include "VAILCoreModule.h"
#include "Editor.h"
#include "Engine/Selection.h"
#include "EngineUtils.h"
#include "HAL/PlatformTime.h"
#include "Engine/Blueprint.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#include "GameFramework/Actor.h"
#include "Components/ActorComponent.h"

FVAILSessionManager& FVAILSessionManager::Get()
{
	static FVAILSessionManager Singleton;
	return Singleton;
}

FVAILSessionManager::FVAILSessionManager()
	: CurrentScopeType(EVAILScopeType::DetailsPanel)
	, BatchStartTime(0.0)
{
}

FVAILSessionManager::~FVAILSessionManager()
{
	if (ActiveBatchTransaction.IsValid())
	{
		ActiveBatchTransaction.Reset();
	}
}

bool FVAILSessionManager::SetScope(const FString& ScopeName, const FString& TargetIdentifier, FString& OutErrorMessage)
{
	check(IsInGameThread());

	CheckBatchTimeout();

	const FString LowerScope = ScopeName.ToLower();
	if (LowerScope == TEXT("detailspanel") || LowerScope == TEXT("details"))
	{
		CurrentScopeType = EVAILScopeType::DetailsPanel;
	}
	else if (LowerScope == TEXT("toolbar"))
	{
		CurrentScopeType = EVAILScopeType::Toolbar;
	}
	else if (LowerScope == TEXT("contentbrowser"))
	{
		CurrentScopeType = EVAILScopeType::ContentBrowser;
	}
	else
	{
		CurrentScopeType = EVAILScopeType::ActivePanel;
	}

	ScopedTargetIdentifier = TargetIdentifier;

	if (!TargetIdentifier.IsEmpty())
	{
		UObject* Resolved = ResolveObject(TargetIdentifier);
		if (!Resolved)
		{
			OutErrorMessage = FString::Printf(TEXT("Could not resolve target object '%s' for scope '%s'"), *TargetIdentifier, *ScopeName);
			return false;
		}
		ScopedObject = Resolved;
	}
	else
	{
		// Fallback to active editor selection if no explicit target
		if (GEditor && GEditor->GetSelectedActorCount() > 0)
		{
			ScopedObject = GEditor->GetSelectedActors()->GetTop<UObject>();
			if (ScopedObject.IsValid())
			{
				ScopedTargetIdentifier = ScopedObject->GetName();
			}
		}
		else
		{
			ScopedObject = nullptr;
		}
	}

	return true;
}

UObject* FVAILSessionManager::GetScopedObject() const
{
	if (ScopedObject.IsValid())
	{
		return ScopedObject.Get();
	}

	// Fallback to active selection
	if (GEditor && GEditor->GetSelectedActorCount() > 0)
	{
		return GEditor->GetSelectedActors()->GetTop<UObject>();
	}

	return nullptr;
}

bool FVAILSessionManager::BeginBatch(const FString& BatchTitle, FString& OutErrorMessage)
{
	check(IsInGameThread());

	CheckBatchTimeout();

	if (ActiveBatchTransaction.IsValid())
	{
		ActiveBatchTransaction.Reset();
	}

	ActiveBatchTitle = FString::Printf(TEXT("VAIL: %s"), *BatchTitle);
	ActiveBatchTransaction = MakeUnique<FScopedTransaction>(FText::FromString(ActiveBatchTitle));
	BatchStartTime = FPlatformTime::Seconds();

	UE_LOG(LogVAILCore, Log, TEXT("VAILSessionManager: Began batch transaction '%s'"), *ActiveBatchTitle);
	return true;
}

bool FVAILSessionManager::EndBatch(FString& OutErrorMessage)
{
	check(IsInGameThread());

	if (!ActiveBatchTransaction.IsValid())
	{
		OutErrorMessage = TEXT("No active batch transaction to end");
		return false;
	}

	ActiveBatchTransaction.Reset();
	UE_LOG(LogVAILCore, Log, TEXT("VAILSessionManager: Committed batch transaction '%s'"), *ActiveBatchTitle);
	ActiveBatchTitle.Empty();
	BatchStartTime = 0.0;
	return true;
}

bool FVAILSessionManager::RollbackCurrentTransaction(FString& OutErrorMessage)
{
	check(IsInGameThread());

	if (ActiveBatchTransaction.IsValid())
	{
		ActiveBatchTransaction->Cancel();
		ActiveBatchTransaction.Reset();
	}

	if (GEditor)
	{
		const bool bUndone = GEditor->UndoTransaction();
		if (!bUndone)
		{
			OutErrorMessage = TEXT("GEditor->UndoTransaction() returned false");
			return false;
		}
	}

	UE_LOG(LogVAILCore, Warning, TEXT("VAILSessionManager: Rolled back transaction successfully"));
	return true;
}

void FVAILSessionManager::CheckBatchTimeout()
{
	if (ActiveBatchTransaction.IsValid() && BatchStartTime > 0.0)
	{
		if (FPlatformTime::Seconds() - BatchStartTime > BatchTimeoutSeconds)
		{
			UE_LOG(LogVAILCore, Warning, TEXT("VAILSessionManager: Batch transaction '%s' timed out after %.1f seconds. Auto-committing."), *ActiveBatchTitle, BatchTimeoutSeconds);
			ActiveBatchTransaction.Reset();
			ActiveBatchTitle.Empty();
			BatchStartTime = 0.0;
		}
	}
}

UObject* FVAILSessionManager::ResolveObject(const FString& Identifier) const
{
	if (Identifier.IsEmpty())
	{
		return nullptr;
	}

	// 0. "<BlueprintAssetPath>::<ComponentVariableName>" addresses a component's default-value
	// template inside a Blueprint's Components tree (SCS) -- lets vail_set_property configure
	// a component (SpringArm length, Camera FOV, etc.) with no Blueprint editor window open.
	FString BlueprintPath, ComponentName;
	if (Identifier.Split(TEXT("::"), &BlueprintPath, &ComponentName))
	{
		if (UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, *BlueprintPath))
		{
			// "::Self" (or "::CDO") addresses the Blueprint's own actor-level defaults --
			// e.g. bUseControllerRotationYaw on a Character -- as opposed to any component.
			if (ComponentName.Equals(TEXT("Self"), ESearchCase::IgnoreCase) ||
				ComponentName.Equals(TEXT("CDO"), ESearchCase::IgnoreCase))
			{
				if (Blueprint->GeneratedClass)
				{
					return Blueprint->GeneratedClass->GetDefaultObject();
				}
				return nullptr;
			}

			if (Blueprint->SimpleConstructionScript)
			{
				for (USCS_Node* Node : Blueprint->SimpleConstructionScript->GetAllNodes())
				{
					if (Node->GetVariableName().ToString().Equals(ComponentName, ESearchCase::IgnoreCase))
					{
						return Node->ComponentTemplate;
					}
				}
			}

			// Not an SCS node -- fall back to a component inherited from the native C++ parent
			// class (e.g. Character's CapsuleComponent/CharacterMovement/Mesh), read off the
			// class default object so its property defaults are still editable headlessly.
			if (Blueprint->GeneratedClass)
			{
				if (AActor* CDO = Cast<AActor>(Blueprint->GeneratedClass->GetDefaultObject()))
				{
					// Native default subobjects rarely carry the friendly name callers will
					// guess (e.g. ACharacter's movement component object is "CharMoveComp",
					// not "CharacterMovement") -- so match on object name first, then fall
					// back to the component's class name, which is what users actually know.
					UActorComponent* ClassNameMatch = nullptr;
					for (UActorComponent* Comp : CDO->GetComponents())
					{
						if (!Comp)
						{
							continue;
						}
						if (Comp->GetName().Equals(ComponentName, ESearchCase::IgnoreCase))
						{
							return Comp;
						}
						if (!ClassNameMatch && Comp->GetClass()->GetName().Contains(ComponentName, ESearchCase::IgnoreCase))
						{
							ClassNameMatch = Comp;
						}
					}
					if (ClassNameMatch)
					{
						return ClassNameMatch;
					}
				}
			}
		}
		return nullptr;
	}

	// 1. Check if Identifier is a direct Asset / Package path (e.g. "/Game/Blueprints/BP_Player")
	if (Identifier.StartsWith(TEXT("/")) || Identifier.Contains(TEXT(".")))
	{
		UObject* LoadedAsset = StaticLoadObject(UObject::StaticClass(), nullptr, *Identifier);
		if (LoadedAsset)
		{
			return LoadedAsset;
		}
	}

	// 2. Search actors in active Editor World
	if (GEditor)
	{
		UWorld* World = GEditor->GetEditorWorldContext().World();
		if (World)
		{
			for (TActorIterator<AActor> It(World); It; ++It)
			{
				AActor* Actor = *It;
				if (Actor->GetActorLabel() == Identifier || Actor->GetName() == Identifier)
				{
					return Actor;
				}
			}
		}
	}

	return nullptr;
}
