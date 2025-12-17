// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FGLobbyEnrty.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLobbyEntrySelected, const FString&, CheckPasswordParam, int32, EntryLobbyIndex);

/**
 * 
 */
UCLASS()
class FIGHTGAME_API UFGLobbyEnrty : public UUserWidget
{
	GENERATED_BODY()
	
protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UButton> LobbyButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UTextBlock> LobbyTextBlock;

	FOnLobbyEntrySelected OnLobbyEntrySelected;

	FString CheckPasswordString;
	int32 LobbyIndex = -1;

public:
	FORCEINLINE FOnLobbyEntrySelected& GetOnLobbyEntrySelected() { return OnLobbyEntrySelected; }

protected:
	virtual void NativeConstruct() override;

	UFUNCTION()
	void LobbyButtonClicked();

public:
	void SetLobbyEntry(const FString& InLobbyName, const FString& InCheckPassword, int32 EntryLobbyIndex);
};
