// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FGLobbyWidget.generated.h"

/**
 * 
 */
UCLASS()
class FIGHTGAME_API UFGLobbyWidget : public UUserWidget
{
	GENERATED_BODY()
	
private:
	UPROPERTY()
	TObjectPtr<class UFGGameInstance> VFGGameInstance;

	UPROPERTY()
	TObjectPtr<class AFGLobbyGameState> VFGLobbyGameState;

	UPROPERTY()
	TObjectPtr<class AFGLobbyPlayerController> VFGLobbyPlayerController;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UTextBlock> LobbyNameTextBlock;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UListView> TeamAListView;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UListView> TeamBListView;

	UPROPERTY()
	TArray<TObjectPtr<class AFGPlayerState>> TeamAPlayers;

	UPROPERTY()
	TArray<TObjectPtr<AFGPlayerState>> TeamBPlayers;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UButton> TeamASelect;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> TeamBSelect;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> StartButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UWrapBox> SelectCharacterWrapBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ExitButton;

	UPROPERTY(EditDefaultsOnly, Category = "EOS")
	int32 SelectCharacterArraySize = 30;

	FTimerHandle PlayerListUpdateTimer;

protected:
	virtual void NativeConstruct() override;

	void UpdateTeamListView();

	void SelectCharacterArray();
	void SelectCharacterArrayFunction();

	UFUNCTION()
	void SetTeamA();

	UFUNCTION()
	void SetTeamB();

	UFUNCTION()
	void StartGameFunction();

	UFUNCTION()
	void OnExitButtonClicked();
};
