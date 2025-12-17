// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "Interface/FGRTSInterface.h"
#include "FGHUD.generated.h"

/**
 * 
 */
UCLASS()
class FIGHTGAME_API AFGHUD : public AHUD, public IFGRTSInterface
{
	GENERATED_BODY()
	
protected:
	UPROPERTY()
	TArray<TObjectPtr<class AFGCharacterPlayer>> OwnFGCharacterPlayerArray;

	UPROPERTY()
	TObjectPtr<class AFGPlayerController> VFGPlayerController;

	UPROPERTY()
	TObjectPtr<AFGCharacterPlayer> VFGCharacterPlayer;
	
	FTimerHandle CastFGPlayerControllerTimer;

	FVector2D CurrentMousePosition;
	FVector2D StartMousePosition;

	uint32 bDrawing : 1;
	uint32 bMiniMapEnter : 1;

	int32 SelectedCharacterNumber;

public:
	FORCEINLINE void SetbMiniMapEnter(const bool InbMiniMapEnter) { bMiniMapEnter = InbMiniMapEnter; }

protected:
	virtual void BeginPlay() override;

public:
	virtual void DrawHUD() override;

	void HUDSelectCharacter(const int32& InSelectedCharacterNumber);
	void HUDSelectAllCharacter();

//Interface
protected:
	virtual void MarqueePressed() override;
	virtual void MarqueeReleased() override;
	virtual void MarqueeHeld() override;
	virtual TArray<AFGCharacterPlayer*> SelectedCharacterArray() override;
};
