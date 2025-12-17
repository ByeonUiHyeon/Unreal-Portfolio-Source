// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "FGLobbyGameMode.generated.h"

/**
 * 
 */
UCLASS()
class FIGHTGAME_API AFGLobbyGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	virtual void PostLogin(APlayerController* NewPlayer) override;

	virtual void InitGameState() override;
};
