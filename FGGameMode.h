// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "FGGameMode.generated.h"

class AFGPlayerController;
class AFGCharacterPlayer;
class UTexture2D;

/**
 * 
 */
UCLASS()
class FIGHTGAME_API AFGGameMode : public AGameMode
{
	GENERATED_BODY()
	
protected:
	UPROPERTY()
	TObjectPtr<class AFGGameState> VFGGameState;

public:
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

protected:
	int32 RetrunPlayerNumber(class AFGPlayerState* InFGPlayerState);
	APlayerStart* GetTeamPlayerStart(int32 InTeamIndex);

	void SpawnSelectedCharacter(AFGPlayerController* InFGPlayerController, int32 InPlayerNumber, TArray<TObjectPtr<UTexture2D>> InSelectedCharacterImageArray, TArray<TSubclassOf<AFGCharacterPlayer>> InFGCharacterPlayerArray, APlayerStart* InPlayerStart, int32 InTeamIndex);

public:
	void SpawnAllCharacters(AFGPlayerController* InFGPlayerController);
};
