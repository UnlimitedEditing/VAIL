// Copyright Epic Games, Inc. All Rights Reserved.

#include "VAILGameplayHelpers.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "EnhancedInputSubsystems.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"

void UVAILGameplayHelpers::AddInputMappingContextByPath(APawn* Pawn, const FString& ContextAssetPath, int32 Priority)
{
	if (!Pawn)
	{
		return;
	}

	APlayerController* PC = Cast<APlayerController>(Pawn->GetController());
	if (!PC)
	{
		return;
	}

	ULocalPlayer* LocalPlayer = PC->GetLocalPlayer();
	if (!LocalPlayer)
	{
		return;
	}

	UEnhancedInputLocalPlayerSubsystem* Subsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	if (!Subsystem)
	{
		return;
	}

	UInputMappingContext* Context = LoadObject<UInputMappingContext>(nullptr, *ContextAssetPath);
	if (!Context)
	{
		return;
	}

	Subsystem->AddMappingContext(Context, Priority);
}

AActor* UVAILGameplayHelpers::FindNearestActorOfClass(UObject* WorldContextObject, AActor* Origin, TSubclassOf<AActor> ActorClass, float MaxRange)
{
	if (!Origin || !ActorClass)
	{
		return nullptr;
	}

	UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : Origin->GetWorld();
	if (!World)
	{
		return nullptr;
	}

	const FVector OriginLocation = Origin->GetActorLocation();
	const float MaxRangeSq = (MaxRange > 0.0f) ? (MaxRange * MaxRange) : -1.0f;

	AActor* Nearest = nullptr;
	float NearestDistSq = FLT_MAX;

	for (TActorIterator<AActor> It(World, ActorClass); It; ++It)
	{
		AActor* Candidate = *It;
		if (!Candidate || Candidate == Origin)
		{
			continue;
		}

		const float DistSq = FVector::DistSquared(OriginLocation, Candidate->GetActorLocation());
		if (MaxRangeSq >= 0.0f && DistSq > MaxRangeSq)
		{
			continue;
		}

		if (DistSq < NearestDistSq)
		{
			NearestDistSq = DistSq;
			Nearest = Candidate;
		}
	}

	return Nearest;
}
