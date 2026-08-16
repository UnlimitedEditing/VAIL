// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWindow.h"
#include "AssetCompilingManager.h"

struct FVAILSettleResult
{
	bool bSettled;
	float SettleDurationMs;
	int32 QuiescentFrames;
	FString SettleFailureReason;
};

class VAILCORE_API FVAILSettleEngine
{
public:
	static FVAILSettleEngine& Get();

	/** Checks Tier 1: Slate layout and attribute invalidation status */
	bool IsSlateQuiescent() const;

	/** Checks Tier 2: Background compilation and asynchronous asset pipelines */
	bool IsAsyncSettled(UBlueprint* ScopedBlueprint = nullptr) const;

	/** Checks Tier 3: Modal windows and popup context menus */
	bool IsModalClear() const;

	/** Returns true if all 3 tiers evaluate to true on the current frame */
	bool IsFullySettled(UBlueprint* ScopedBlueprint = nullptr) const;

	/**
	 * Blocks on the Game Thread until the editor settles for the required consecutive frames
	 * or until the timeout is reached.
	 */
	FVAILSettleResult WaitForSettle(
		float TimeoutSeconds = 5.0f,
		int32 RequiredQuiescentFrames = 2,
		UBlueprint* ScopedBlueprint = nullptr
	);

private:
	FVAILSettleEngine();
	~FVAILSettleEngine();
};
