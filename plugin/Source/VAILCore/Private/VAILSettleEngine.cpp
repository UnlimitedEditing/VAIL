// Copyright Epic Games, Inc. All Rights Reserved.

#include "VAILSettleEngine.h"
#include "VAILCoreModule.h"
#include "Editor.h"
#include "Engine/Blueprint.h"
#include "HAL/PlatformTime.h"

#include "Engine/Engine.h"

FVAILSettleEngine& FVAILSettleEngine::Get()
{
	static FVAILSettleEngine Singleton;
	return Singleton;
}

FVAILSettleEngine::FVAILSettleEngine()
{
}

FVAILSettleEngine::~FVAILSettleEngine()
{
}

void FVAILSettleEngine::CaptureActiveScreenWarnings(TArray<FString>& OutWarnings) const
{
	// Passive warning capture hook
}

bool FVAILSettleEngine::IsSlateQuiescent() const
{
	if (!FSlateApplication::IsInitialized())
	{
		return true;
	}

	const TArray<TSharedRef<SWindow>> Windows = FSlateApplication::Get().GetTopLevelWindows();
	for (const TSharedRef<SWindow>& Window : Windows)
	{
		if (Window->NeedsSlowPath() || Window->IsProcessingAttributeUpdate())
		{
			return false;
		}
	}

	return true;
}

bool FVAILSettleEngine::IsAsyncSettled(UBlueprint* ScopedBlueprint) const
{
	if (FAssetCompilingManager::Get().GetNumRemainingAssets() > 0)
	{
		return false;
	}

	if (ScopedBlueprint && ScopedBlueprint->bBeingCompiled)
	{
		return false;
	}

	return true;
}

bool FVAILSettleEngine::IsModalClear() const
{
	if (!FSlateApplication::IsInitialized())
	{
		return true;
	}

	if (FSlateApplication::Get().GetActiveModalWindow().IsValid())
	{
		return false;
	}

	if (FSlateApplication::Get().AnyMenusVisible())
	{
		return false;
	}

	return true;
}

bool FVAILSettleEngine::IsFullySettled(UBlueprint* ScopedBlueprint) const
{
	return IsSlateQuiescent() && IsAsyncSettled(ScopedBlueprint) && IsModalClear();
}

FVAILSettleResult FVAILSettleEngine::WaitForSettle(
	float TimeoutSeconds,
	int32 RequiredQuiescentFrames,
	UBlueprint* ScopedBlueprint)
{
	check(IsInGameThread());

	FVAILSettleResult Result;
	Result.bSettled = false;
	Result.QuiescentFrames = 0;

	const double StartTime = FPlatformTime::Seconds();
	const double EndTime = StartTime + TimeoutSeconds;

	while (FPlatformTime::Seconds() < EndTime)
	{
		// Pump Slate ticks if needed
		if (FSlateApplication::IsInitialized())
		{
			FSlateApplication::Get().PumpMessages();
		}

		if (IsFullySettled(ScopedBlueprint))
		{
			Result.QuiescentFrames++;
			if (Result.QuiescentFrames >= RequiredQuiescentFrames)
			{
				Result.bSettled = true;
				Result.SettleDurationMs = static_cast<float>((FPlatformTime::Seconds() - StartTime) * 1000.0);
				CaptureActiveScreenWarnings(Result.ScreenWarnings);
				return Result;
			}
		}
		else
		{
			Result.QuiescentFrames = 0;
		}

		FPlatformProcess::Sleep(0.005f); // 5ms tick interval
	}

	Result.SettleDurationMs = static_cast<float>((FPlatformTime::Seconds() - StartTime) * 1000.0);
	CaptureActiveScreenWarnings(Result.ScreenWarnings);
	
	if (!IsSlateQuiescent())
	{
		Result.SettleFailureReason = TEXT("Slate layout pending slow-path invalidation");
	}
	else if (!IsAsyncSettled(ScopedBlueprint))
	{
		Result.SettleFailureReason = TEXT("Async asset or blueprint compilation in progress");
	}
	else if (!IsModalClear())
	{
		Result.SettleFailureReason = TEXT("Modal dialog or popup menu blocking UI");
	}
	else
	{
		Result.SettleFailureReason = TEXT("Timed out before required quiescent frame threshold");
	}

	return Result;
}
