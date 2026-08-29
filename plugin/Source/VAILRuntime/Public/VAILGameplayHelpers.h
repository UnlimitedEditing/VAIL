// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "VAILGameplayHelpers.generated.h"

/**
 * Small runtime-callable helpers that let a headlessly-built Blueprint graph do things
 * (like activating an Enhanced Input mapping context) with a single CallFunction node,
 * instead of needing several chained nodes -- e.g. Get Controller -> Get Local Player ->
 * Get Subsystem -> Add Mapping Context -- that VAIL's generic node spawner can't
 * assemble in one shot. Lives in VAILRuntime (a Runtime-type module) rather than
 * VAILCore/UnrealMCP (both Editor-type) because an Editor-module UFUNCTION is rejected
 * by the Blueprint compiler when called from a runtime (gameplay) Blueprint.
 */
UCLASS()
class VAILRUNTIME_API UVAILGameplayHelpers : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Activates an Input Mapping Context (by asset path) on the Enhanced Input subsystem of
	 * the local player controlling the given Pawn. No-ops quietly if the Pawn isn't
	 * player-controlled or the path doesn't resolve to a valid UInputMappingContext.
	 */
	UFUNCTION(BlueprintCallable, Category = "VAIL")
	static void AddInputMappingContextByPath(APawn* Pawn, const FString& ContextAssetPath, int32 Priority);

	/**
	 * Finds the closest actor of the given class to Origin, within MaxRange (0 = unlimited).
	 * Returns nullptr if none found. Avoids needing a Blueprint ForEachLoop macro (a different,
	 * untested-by-VAIL node category) for a simple nearest-target search.
	 */
	UFUNCTION(BlueprintCallable, Category = "VAIL", meta = (WorldContext = "WorldContextObject"))
	static AActor* FindNearestActorOfClass(UObject* WorldContextObject, AActor* Origin, TSubclassOf<AActor> ActorClass, float MaxRange);
};
