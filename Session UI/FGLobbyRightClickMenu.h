// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FGLobbyRightClickMenu.generated.h"

/**
 * 
 */
UCLASS()
class FIGHTGAME_API UFGLobbyRightClickMenu : public UUserWidget
{
	GENERATED_BODY()
	
private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UButton> CloseButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> KickOutButton;

	UPROPERTY()
	TObjectPtr<class AFGPlayerState> VFGPlayerState;

public:
	FORCEINLINE void SetFGPlayerState(AFGPlayerState* InFGPlayerState) { VFGPlayerState = InFGPlayerState; }

public:
	UFGLobbyRightClickMenu(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void NativeConstruct() override;

	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	UFUNCTION()
	void OnCloseButtonClicked();

	UFUNCTION()
	void OnKickOutButtonClicked();
};
