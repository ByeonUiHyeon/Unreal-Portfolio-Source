// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "FGGameState.generated.h"

/**
 * 
 */
UCLASS()
class FIGHTGAME_API AFGGameState : public AGameState
{
	GENERATED_BODY()
	
private:
	UPROPERTY(Replicated)
	TArray<TObjectPtr<class AFGCharacterPlayer>> TeamACharacterPlayerArray;

	UPROPERTY(Replicated)
	TArray<TObjectPtr<AFGCharacterPlayer>> TeamBCharacterPlayerArray;

	UPROPERTY(Replicated)
	TArray<TObjectPtr<class AFGPlayerController>> TeamAPlayerControllerArray;

	UPROPERTY(Replicated)
	TArray<TObjectPtr<AFGPlayerController>> TeamBPlayerControllerArray;

	UPROPERTY(Replicated)
	int32 IntTime;

	UPROPERTY(EditDefaultsOnly, Category = "GAS_FOW")
	TObjectPtr<class UMaterialInterface> FogOfWarMaterialInterface;

	UPROPERTY(EditDefaultsOnly, Category = "GAS_FOW")
	float FogOfWarPeriod = 0.4f;

	UPROPERTY(EditDefaultsOnly, Category = "GAS_FOW")
	float GridSize = 100.f;

	UPROPERTY(EditDefaultsOnly, Category = "GAS_FOW")
	bool bUseValue;

	UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "bUseValue", EditConditionHides, ClampMin = "0.0"), Category = "GAS_FOW")
	float XSize;

	UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "bUseValue", EditConditionHides, ClampMin = "0.0"), Category = "GAS_FOW")
	float YSize;

	UPROPERTY(EditDefaultsOnly, Category = "GAS_MiniMap")
	TObjectPtr<class UMaterialInterface> MiniMapMaterialInterface;

	UPROPERTY(EditDefaultsOnly, Category = "GAS_Spawn")
	float SpawnDelay = 1.f;

	UPROPERTY(Replicated)
	int32 Rows;

	UPROPERTY(Replicated)
	int32 Columns;

	UPROPERTY(Replicated)
	TArray<int32> BlockedArray;

	UPROPERTY()
	TArray<TObjectPtr<class AFGNexus>> NexusArray;

	UPROPERTY()
	TArray<TObjectPtr<AActor>> StaticActorArray;

	UPROPERTY()
	TArray<TObjectPtr<AActor>> OtherActorOfTeamA;

	UPROPERTY()
	TArray<TObjectPtr<AActor>> OtherActorOfTeamB;

	UPROPERTY()
	TSet<TObjectPtr<AFGPlayerController>> LoadedPlayers;

	UPROPERTY(EditDefaultsOnly, Category = "GAS")
	TSubclassOf<AFGNexus> FGNexusClass;

	FTimerHandle UpdateFogOfWarTimer;
	FTimerHandle SpawnDelayTimerHandle;

	TArray<uint8> TeamAFogOfWar;
	TArray<uint8> TeamBFogOfWar;

	FString KillerNickname;
	FString TargetNickname;

	float FloatTime = 0;

	int32 TeamAScore = 0;
	int32 TeamBScore = 0;

public:
	FORCEINLINE int32 GetIntTime() const { return IntTime; }
	FORCEINLINE UMaterialInterface* GetFogOfWarMaterialInterface() const { return FogOfWarMaterialInterface; }
	FORCEINLINE UMaterialInterface* GetMiniMapMaterialInterface() const { return MiniMapMaterialInterface; }
	FORCEINLINE int32 GetRows() const { return Rows; }
	FORCEINLINE int32 GetColumns() const { return Columns; }
	FORCEINLINE float GetGridSize() const { return GridSize; }
	FORCEINLINE void AddTeamACharacterPlayerArray(AFGCharacterPlayer* InFGCharacterPlayer) { if (!TeamACharacterPlayerArray.Contains(InFGCharacterPlayer)) { TeamACharacterPlayerArray.Add(InFGCharacterPlayer); } }
	FORCEINLINE void AddTeamBCharacterPlayerArray(AFGCharacterPlayer* InFGCharacterPlayer) { if (!TeamBCharacterPlayerArray.Contains(InFGCharacterPlayer)) { TeamBCharacterPlayerArray.Add(InFGCharacterPlayer); } }
	FORCEINLINE void AddTeamAPlayerControllerArray(AFGPlayerController* InFGPlayerController) { if (!TeamAPlayerControllerArray.Contains(InFGPlayerController)) { TeamAPlayerControllerArray.Add(InFGPlayerController); } }
	FORCEINLINE void AddTeamBPlayerControllerArray(AFGPlayerController* InFGPlayerController) { if (!TeamBPlayerControllerArray.Contains(InFGPlayerController)) { TeamBPlayerControllerArray.Add(InFGPlayerController); } }
	FORCEINLINE void AddOtherActorOfTeamA(AActor* InActor) { if (!OtherActorOfTeamA.Contains(InActor)) { OtherActorOfTeamA.Add(InActor); } }
	FORCEINLINE void AddOtherActorOfTeamB(AActor* InActor) { if (!OtherActorOfTeamB.Contains(InActor)) { OtherActorOfTeamB.Add(InActor); } }
	FORCEINLINE void RemoveOtherActorOfTeamA(AActor* InActor) { OtherActorOfTeamA.Remove(InActor); }
	FORCEINLINE void RemoveOtherActorOfTeamB(AActor* InActor) { OtherActorOfTeamB.Remove(InActor);}

public:
	AFGGameState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaSeconds) override;

	void AddTeamScroeAndKilllog(AFGCharacterPlayer* InKiller, AFGCharacterPlayer* InTarget);

//Fog Of War
protected:
	void GridSetting();
	void UpdateFogOfWar(int32 InTeamIndex, const TArray<TObjectPtr<AFGCharacterPlayer>>& OurTeamCharacterPlayerArray, const TArray<TObjectPtr<AActor>>& InOtherActorArray, const TArray<TObjectPtr<AFGPlayerController>>& InTeamPlayerControllerArray);
	void UpdateFogOfWarFunction(int32 InVisionRadiusGrid, FVector InActorLocation, int32 InActorGridZ, TArray<uint8>& InFogOfWar, TSet<int32>& InVisitedGrids);
	void BresenhamSight(int32 InCenter, int32 StartX, int32 StartY, int32 EndX, int32 EndY, int32 InCharacterZ, TArray<uint8>& InFogOfWar, TSet<int32>& InVisitedGrids);
	void UpdatingFogOfWar();

public:
	void NotifyPlayerLoadingComplete(AFGPlayerController* InFGPlayerController);

protected:
	UFUNCTION(NetMulticast, Reliable)
	void NetMulticast_CreateFGChildWidgetComponent();
};
