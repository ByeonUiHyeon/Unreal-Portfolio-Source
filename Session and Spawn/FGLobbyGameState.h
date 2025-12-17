// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "FGLobbyGameState.generated.h"

/**
 * 
 */
UCLASS()
class FIGHTGAME_API AFGLobbyGameState : public AGameState
{
	GENERATED_BODY()
	
private:
	UPROPERTY(EditDefaultsOnly, Replicated, Category = "EOS")
	TObjectPtr<class UFGSelectCharacterDataAsset> FGSelectCharacterDataAsset;

	UPROPERTY(Replicated)
	FString RoomNameString;

public:
	FORCEINLINE const FString& GetRoomNameString() const { return RoomNameString; }
	FORCEINLINE void SetRoomNameString(const FString& InRoomNameString) { RoomNameString = InRoomNameString; }
	FORCEINLINE const UFGSelectCharacterDataAsset* GetFGSelectCharacterDataAsset() const { return FGSelectCharacterDataAsset; }

public:
	AFGLobbyGameState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
