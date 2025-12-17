// Fill out your copyright notice in the Description page of Project Settings.


#include "Session/FGMainMenu.h"
#include "Game/FGGameInstance.h"
#include "Player/FGEOSPlayerController.h"
#include "Components/SizeBox.h"
#include "Components/ComboBoxString.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/CheckBox.h"
#include "Components/ScrollBox.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "Components/TextBlock.h"
#include "Session/FGLobbyEnrty.h"
#include "Kismet/KismetSystemLibrary.h"

void UFGMainMenu::NativeConstruct()
{
	Super::NativeConstruct();

	VFGGameInstance = GetGameInstance<UFGGameInstance>();
	if (!IsValid(VFGGameInstance)) return;

	VFGEOSPlayerController = Cast<AFGEOSPlayerController>(GetOwningPlayer());
	if (!IsValid(VFGEOSPlayerController)) return;

	VFGGameInstance->GetOnOnlineStatusState().AddUObject(this, &UFGMainMenu::OnlineStatusState);
	VFGGameInstance->GetOnUpdateSessionList().AddUObject(this, &UFGMainMenu::UpdateSessionList);

	LogoutSizeBox->SetVisibility(ESlateVisibility::Collapsed);
	ChannelSizeBox->SetVisibility(ESlateVisibility::Collapsed);
	CreateSessionSizeBox->SetVisibility(ESlateVisibility::Collapsed);
	FindSessionSizeBox->SetVisibility(ESlateVisibility::Collapsed);
	JoinSessionVerticalBox->SetVisibility(ESlateVisibility::Collapsed);
	PlayerNicknameTextBlock->SetVisibility(ESlateVisibility::Collapsed);
	SettingButton->SetVisibility(ESlateVisibility::Collapsed);

	LoginComboBox->OnSelectionChanged.RemoveAll(this);
	LoginComboBox->OnSelectionChanged.AddDynamic(this, &UFGMainMenu::OnLoginComboBoxSelectionChanged);
	LoginComboBox->AddOption(TEXT("Login"));
	LoginComboBox->AddOption(TEXT("Developer Login"));
	LoginComboBox->SetSelectedOption(TEXT("Login"));

	LoginButton->OnClicked.RemoveAll(this);
	LoginButton->OnClicked.AddDynamic(this, &UFGMainMenu::OnLoginButtonClicked);

	LogoutButton->OnClicked.RemoveAll(this);
	LogoutButton->OnClicked.AddDynamic(this, &UFGMainMenu::OnLogoutButtonClicked);

	CannelComboBox->OnSelectionChanged.RemoveAll(this);
	CannelComboBox->OnSelectionChanged.AddDynamic(this, &UFGMainMenu::OnChannelComboBoxSelectionChanged);
	CannelComboBox->AddOption(TEXT("1"));
	CannelComboBox->AddOption(TEXT("2"));
	CannelComboBox->AddOption(TEXT("3"));
	CannelComboBox->SetSelectedOption(TEXT("1"));

	RoomNameEditableTextBox->OnTextChanged.RemoveAll(this);
	RoomNameEditableTextBox->OnTextChanged.AddDynamic(this, &UFGMainMenu::OnRoomNameChanged);
	PasswordCheckBox->OnCheckStateChanged.AddDynamic(this, &UFGMainMenu::OnCheckBoxStateChanged);

	PasswordEditableTextBox->OnTextChanged.RemoveAll(this);
	PasswordEditableTextBox->OnTextChanged.AddDynamic(this, &UFGMainMenu::OnPasswordChanged);
	PasswordEditableTextBox->SetIsEnabled(false);

	CreateSessionButton->OnClicked.RemoveAll(this);
	CreateSessionButton->OnClicked.AddDynamic(this, &UFGMainMenu::OnCreateSessionButtonClicked);
	CreateSessionButton->SetIsEnabled(false);

	FindSessionsButton->OnClicked.RemoveAll(this);
	FindSessionsButton->OnClicked.AddDynamic(this, &UFGMainMenu::OnFindSessionButtonsClicked);
	VFGGameInstance->GetOnSessionSearchCompleted().AddUObject(this, &UFGMainMenu::OnFindSessionsCompletedFunction);
	//FindSessions 성공 시 호출

	InputPasswordEditableTextBox->OnTextChanged.RemoveAll(this);
	InputPasswordEditableTextBox->OnTextChanged.AddDynamic(this, &UFGMainMenu::OnInputPasswordChanged);

	JoinLobbyButton->OnClicked.RemoveAll(this);
	JoinLobbyButton->OnClicked.AddDynamic(this, &UFGMainMenu::OnJoinLobbyButtonClicked);
	JoinLobbyButton->SetIsEnabled(false);

	SettingButton->OnClicked.RemoveAll(this);
	SettingButton->OnClicked.AddDynamic(this, &UFGMainMenu::OnSettingButtonClicked);

	QuitGameButton->OnClicked.RemoveAll(this);
	QuitGameButton->OnClicked.AddDynamic(this, &UFGMainMenu::OnQuitButtonClicked);
}

void UFGMainMenu::OnlineStatusState(bool bOnline)
{
	if (bOnline)
	{
		LoginSizeBox->SetVisibility(ESlateVisibility::Collapsed);
		InputLoginSizeBox->SetVisibility(ESlateVisibility::Collapsed);
		LogoutSizeBox->SetVisibility(ESlateVisibility::Visible);
		ChannelSizeBox->SetVisibility(ESlateVisibility::Visible);
		CreateSessionSizeBox->SetVisibility(ESlateVisibility::Visible);
		FindSessionSizeBox->SetVisibility(ESlateVisibility::Visible);
		JoinSessionVerticalBox->SetVisibility(ESlateVisibility::Visible);
		InputPasswordHorizontalBox->SetVisibility(ESlateVisibility::Collapsed);
		PlayerNicknameTextBlock->SetVisibility(ESlateVisibility::Visible);
		SettingButton->SetVisibility(ESlateVisibility::Visible);

		PlayerNicknameTextBlock->SetText(FText::FromString(VFGGameInstance->GetPlayerNicknameFunction()));
	}
	else
	{
		LoginSizeBox->SetVisibility(ESlateVisibility::Visible);
		InputLoginSizeBox->SetVisibility(ESlateVisibility::Visible);
		LogoutSizeBox->SetVisibility(ESlateVisibility::Collapsed);
		ChannelSizeBox->SetVisibility(ESlateVisibility::Collapsed);
		CreateSessionSizeBox->SetVisibility(ESlateVisibility::Collapsed);
		FindSessionSizeBox->SetVisibility(ESlateVisibility::Collapsed);
		JoinSessionVerticalBox->SetVisibility(ESlateVisibility::Collapsed);
		PlayerNicknameTextBlock->SetVisibility(ESlateVisibility::Collapsed);
		SettingButton->SetVisibility(ESlateVisibility::Collapsed);

		PlayerNicknameTextBlock->SetText(FText::GetEmpty());
	}
}

void UFGMainMenu::UpdateSessionList(const int32& InLobbyIndex)
{
	LobbyListScrollBox->RemoveChildAt(InLobbyIndex);

	JoinLobbyButton->SetIsEnabled(false);
}

void UFGMainMenu::OnLoginComboBoxSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	if (SelectedItem == TEXT("Login"))
	{
		IdEditableTextBox->SetVisibility(ESlateVisibility::Collapsed);
		TokenEditableTextBox->SetVisibility(ESlateVisibility::Collapsed);
	}
	else if (SelectedItem == TEXT("Developer Login"))
	{
		IdEditableTextBox->SetVisibility(ESlateVisibility::Visible);
		TokenEditableTextBox->SetVisibility(ESlateVisibility::Visible);
	}
}

void UFGMainMenu::OnLoginButtonClicked()
{
	if (LoginComboBox->GetSelectedOption() == TEXT("Login"))
	{
		VFGGameInstance->LoginFunction();
	}
	else if (LoginComboBox->GetSelectedOption() == TEXT("Developer Login"))
	{
		FString DevId = IdEditableTextBox->GetText().ToString();
		FString DevToken = TokenEditableTextBox->GetText().ToString();

		if (DevId.IsEmpty() || DevToken.IsEmpty()) return;

		VFGGameInstance->DeveloperLoginFunction(DevId, DevToken);
	}
}

void UFGMainMenu::OnLogoutButtonClicked()
{
	VFGGameInstance->LogoutFunction();
}

void UFGMainMenu::OnChannelComboBoxSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	VFGGameInstance->SetChannelNumber(SelectedItem);

	if (LobbyListScrollBox->GetChildrenCount() != 0)
	{
		LobbyListScrollBox->ClearChildren();
	}

	VFGGameInstance->FindSessionsFunction();
}

void UFGMainMenu::OnRoomNameChanged(const FText& InText)
{
	CreateSessionButton->SetIsEnabled(!InText.IsEmpty());
}

void UFGMainMenu::OnCheckBoxStateChanged(bool bIsChecked)
{
	PasswordEditableTextBox->SetIsEnabled(bIsChecked);
}

void UFGMainMenu::OnPasswordChanged(const FText& InText)
{
	CreateSessionButton->SetIsEnabled(!InText.IsEmpty());
}

void UFGMainMenu::OnCreateSessionButtonClicked()
{
	VFGGameInstance->CreateSessionFunction(RoomNameEditableTextBox->GetText().ToString(), PasswordCheckBox->IsChecked(), PasswordEditableTextBox->GetText().ToString());
}

void UFGMainMenu::OnFindSessionButtonsClicked()
{
	if (LobbyListScrollBox->GetChildrenCount() != 0)
	{
		LobbyListScrollBox->ClearChildren();
	}

	VFGGameInstance->FindSessionsFunction();
}

void UFGMainMenu::OnFindSessionsCompletedFunction(const TArray<FOnlineSessionSearchResult>& SearchResults)
{
	if (LobbyListScrollBox->GetChildrenCount() != 0)
	{
		LobbyListScrollBox->ClearChildren();
	}

	int32 i = 0;
	for (const FOnlineSessionSearchResult& SearchResult : SearchResults)
	{
		UFGLobbyEnrty* VFGLobbyEnrty = CreateWidget<UFGLobbyEnrty>(LobbyListScrollBox, VFGEOSPlayerController->GetUserWidgetClassByName(TEXT("UFGLobbyEnrty")));
		if (IsValid(VFGLobbyEnrty))
		{
			VFGLobbyEnrty->GetOnLobbyEntrySelected().AddDynamic(this, &UFGMainMenu::LobbyEntrySelected);

			FString RoomNameString;
			SearchResult.Session.SessionSettings.Get(VFGGameInstance->GetRoomName(), RoomNameString);

			FString CheckPasswordNameString;
			SearchResult.Session.SessionSettings.Get(VFGGameInstance->GetCheckPasswordName(), CheckPasswordNameString);

			VFGLobbyEnrty->SetLobbyEntry(RoomNameString, CheckPasswordNameString, i);

			LobbyListScrollBox->AddChild(VFGLobbyEnrty);
		}

		++i;
	}
}

void UFGMainMenu::LobbyEntrySelected(const FString& InCheckPasswordParam, int32 EntryLobbyIndex)
{
	SelectedLobbyEntryIndex = EntryLobbyIndex;
	VFGGameInstance->SetSelectedLobbyIndex(EntryLobbyIndex);

	if (InCheckPasswordParam == TEXT("true"))
	{
		InputPasswordHorizontalBox->SetVisibility(ESlateVisibility::Visible);
	}
	else
	{
		if (SelectedLobbyEntryIndex != -1)
		{
			JoinLobbyButton->SetIsEnabled(true);
		}

		InputPasswordHorizontalBox->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UFGMainMenu::OnInputPasswordChanged(const FText& InText)
{
	JoinLobbyButton->SetIsEnabled(!InText.IsEmpty());
}

void UFGMainMenu::OnJoinLobbyButtonClicked()
{
	VFGGameInstance->JoinSessionFunctionIndex(InputPasswordEditableTextBox->GetText().ToString(), SelectedLobbyEntryIndex);
}

void UFGMainMenu::OnSettingButtonClicked()
{
	VFGEOSPlayerController->CreateFGSettingsWidget();
}

void UFGMainMenu::OnQuitButtonClicked()
{
	UKismetSystemLibrary::QuitGame(GetWorld(), VFGEOSPlayerController, EQuitPreference::Quit, true);
}
