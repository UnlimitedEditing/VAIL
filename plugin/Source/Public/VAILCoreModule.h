// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

DECLARE_LOG_CATEGORY_EXTERN(LogVAILCore, Log, All);

class FVAILCoreModule : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	/** Get the module instance */
	static FVAILCoreModule& Get()
	{
		return FModuleManager::LoadModuleChecked<FVAILCoreModule>("VAILCore");
	}

	/** Check if the module is loaded */
	static bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded("VAILCore");
	}
};
