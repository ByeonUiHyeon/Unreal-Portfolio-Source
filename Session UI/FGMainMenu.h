// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FGMainMenu.generated.h"

/**
 * 
 */
UCLASS()
class FIGHTGAME_API UFGMainMenu : public UUserWidget
{
	GENERATED_BODY()
	
protected:
	UPROPERTY()
	TObjectPtr<class UFGGameInstance> VFGGameInstance;

	UPROPERTY()
	TObjectPtr<class AFGEOSPlayerController> VFGEOSPlayerController;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class USizeBox> LoginSizeBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UComboBoxString> LoginComboBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USizeBox> InputLoginSizeBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UEditableTextBox> IdEditableTextBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UEditableTextBox> TokenEditableTextBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UButton> LoginButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USizeBox> LogoutSizeBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> LogoutButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USizeBox> ChannelSizeBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UComboBoxString> CannelComboBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USizeBox> CreateSessionSizeBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UEditableTextBox> RoomNameEditableTextBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UCheckBox> PasswordCheckBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UEditableTextBox> PasswordEditableTextBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> CreateSessionButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USizeBox> FindSessionSizeBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> FindSessionsButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UVerticalBox> JoinSessionVerticalBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UScrollBox> LobbyListScrollBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UHorizontalBox> InputPasswordHorizontalBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UEditableTextBox> InputPasswordEditableTextBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> JoinLobbyButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UTextBlock> PlayerNicknameTextBlock;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> SettingButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> QuitGameButton;

	int32 SelectedLobbyEntryIndex = -1;

protected:
	virtual void NativeConstruct() override;

	void OnlineStatusState(bool bOnline);
	void UpdateSessionList(const int32& InLobbyIndex);

	UFUNCTION()
	void OnLoginComboBoxSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	UFUNCTION()
	void OnLoginButtonClicked();

	UFUNCTION()
	void OnLogoutButtonClicked();

	UFUNCTION()
	void OnChannelComboBoxSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	UFUNCTION()
	void OnRoomNameChanged(const FText& InText);

	UFUNCTION()
	void OnCheckBoxStateChanged(bool bIsChecked);

	UFUNCTION()
	void OnPasswordChanged(const FText& InText);

	UFUNCTION()
	void OnCreateSessionButtonClicked();

	UFUNCTION()
	void OnFindSessionButtonsClicked();

	void OnFindSessionsCompletedFunction(const TArray<FOnlineSessionSearchResult>& SearchResults);

	UFUNCTION()
	void LobbyEntrySelected(const FString& InCheckPasswordParam, int32 EntryLobbyIndex);

	UFUNCTION()
	void OnInputPasswordChanged(const FText& InText);

	UFUNCTION()
	void OnJoinLobbyButtonClicked();

	UFUNCTION()
	void OnSettingButtonClicked();

	UFUNCTION()
	void OnQuitButtonClicked();
};
