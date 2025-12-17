// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint//IUserObjectListEntry.h"
#include "FGLobbyPlayerEntry.generated.h"

/**
 * 
 */
UCLASS()
class FIGHTGAME_API UFGLobbyPlayerEntry : public UUserWidget, public IUserObjectListEntry
{
	GENERATED_BODY()
	
private:
	UPROPERTY()
	TObjectPtr<class AFGLobbyPlayerController> VFGLobbyPlayerController;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UTextBlock> PlayerNameTextBlock;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UWrapBox> SelectCharacterWrapBox;

	UPROPERTY()
	TObjectPtr<class AFGPlayerState> VFGPlayerState;

	UPROPERTY()
	TObjectPtr<class UFGLobbyRightClickMenu> VFGLobbyRightClickMenu;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	void CreateFGLobbyRightClickMenuWidget(FVector2D ClickPosition, const FPointerEvent& InMouseEvent);

public:
	UFUNCTION()
	void UpdateSelectCharacterWrapBox();
};
