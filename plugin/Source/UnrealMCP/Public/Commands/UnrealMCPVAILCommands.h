// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

/**
 * Command handler for VAIL JSON-RPC commands received over the TCP socket bridge.
 * Adapts incoming JSON requests into native VAILCore C++ calls and formats response envelopes.
 */
class UNREALMCP_API FUnrealMCPVAILCommands
{
public:
	FUnrealMCPVAILCommands();
	~FUnrealMCPVAILCommands();

	/** Dispatches an incoming vail_* command to its corresponding handler */
	TSharedPtr<FJsonObject> HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params);

private:
	TSharedPtr<FJsonObject> HandleSetScope(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleGetTree(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleFind(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleExecuteCommand(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleSetProperty(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleWaitFor(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleBeginBatch(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleEndBatch(const TSharedPtr<FJsonObject>& Params);

	// Phase 2: Graph Data-Channel and Asset Management
	TSharedPtr<FJsonObject> HandleGraphGetTopology(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleGraphAddNode(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleGraphConnectPins(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleGraphDeleteNode(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleAssetCreate(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleAssetQuery(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleAssetSave(const TSharedPtr<FJsonObject>& Params);

	// Phase 3: Spatial, UMG, Sequencer, and Sensory Telemetry Handlers
	TSharedPtr<FJsonObject> HandleLevelSpawnActor(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleLevelQueryActors(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleLevelDeleteActor(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleViewportFrame(const TSharedPtr<FJsonObject>& Params);

	TSharedPtr<FJsonObject> HandleWidgetTreeGet(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleWidgetAddElement(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleWidgetSetSlot(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleWidgetBindEvent(const TSharedPtr<FJsonObject>& Params);

	TSharedPtr<FJsonObject> HandleSequencerQuery(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleSequencerAddTrack(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleSequencerAddKey(const TSharedPtr<FJsonObject>& Params);

	TSharedPtr<FJsonObject> HandleSenseOptical(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleSenseSpatial(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleSenseMesh(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleSenseShader(const TSharedPtr<FJsonObject>& Params);

	// Phase 4: Subsystems & Specialized Production Tooling Handlers
	TSharedPtr<FJsonObject> HandleLandscapeCreate(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleLandscapeSculpt(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleLandscapePaint(const TSharedPtr<FJsonObject>& Params);

	TSharedPtr<FJsonObject> HandleFoliageScatter(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleFoliageQuery(const TSharedPtr<FJsonObject>& Params);

	TSharedPtr<FJsonObject> HandleControlRigSetTransform(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleControlRigQuery(const TSharedPtr<FJsonObject>& Params);

	TSharedPtr<FJsonObject> HandleAudioPlay(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleAudioSetParameter(const TSharedPtr<FJsonObject>& Params);

	TSharedPtr<FJsonObject> HandleProjectBuild(const TSharedPtr<FJsonObject>& Params);
};


