// Copyright Epic Games, Inc. All Rights Reserved.

#include "VAILCoreModule.h"
#include "VAILIdentityRegistry.h"

DEFINE_LOG_CATEGORY(LogVAILCore);

#define LOCTEXT_NAMESPACE "FVAILCoreModule"

void FVAILCoreModule::StartupModule()
{
	UE_LOG(LogVAILCore, Log, TEXT("VAILCore: Module starting up"));
	
	// Initialize Identity Registry and subscribe to Slate command list delegations
	FVAILIdentityRegistry::Get().Initialize();
}

void FVAILCoreModule::ShutdownModule()
{
	UE_LOG(LogVAILCore, Log, TEXT("VAILCore: Module shutting down"));
	
	FVAILIdentityRegistry::Get().Shutdown();
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FVAILCoreModule, VAILCore)
