// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ScopedTransaction.h"

enum class EVAILScopeType : uint8
{
	DetailsPanel,
	Toolbar,
	ContentBrowser,
	ActivePanel
};

class VAILCORE_API FVAILSessionManager
{
public:
	static FVAILSessionManager& Get();

	/** Set active attention scope and target object */
	bool SetScope(const FString& ScopeName, const FString& TargetIdentifier, FString& OutErrorMessage);

	/** Get current scope type */
	EVAILScopeType GetCurrentScopeType() const { return CurrentScopeType; }

	/** Get currently scoped object */
	UObject* GetScopedObject() const;

	/** Get currently scoped object identifier */
	FString GetScopedTargetIdentifier() const { return ScopedTargetIdentifier; }

	/** Begin a compound transaction batch */
	bool BeginBatch(const FString& BatchTitle, FString& OutErrorMessage);

	/** End and commit active compound transaction batch */
	bool EndBatch(FString& OutErrorMessage);

	/** Roll back active transaction (Auto-rollback on failure) */
	bool RollbackCurrentTransaction(FString& OutErrorMessage);

	/** Utility helper to find a UObject by actor label, name, or asset path */
	UObject* ResolveObject(const FString& Identifier) const;

private:
	FVAILSessionManager();
	~FVAILSessionManager();

	void CheckBatchTimeout();

	EVAILScopeType CurrentScopeType;
	FString ScopedTargetIdentifier;
	TWeakObjectPtr<UObject> ScopedObject;

	TUniquePtr<FScopedTransaction> ActiveBatchTransaction;
	FString ActiveBatchTitle;
	double BatchStartTime;
	const double BatchTimeoutSeconds = 15.0;
};
