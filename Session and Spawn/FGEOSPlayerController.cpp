// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/FGEOSPlayerController.h"
#include "Session/FGMainMenu.h"
#include "Game/FGGameInstance.h"
#include "EnhancedInputSubsystems.h"
#include "UserSettings/EnhancedInputUserSettings.h"
#include "UI/FGSettings.h"
#include "GameFramework/PlayerState.h"
#include "Save/FGLocalPlayerSaveGame.h"
#include "Game/FGGameInstance.h"

AFGEOSPlayerController::AFGEOSPlayerController()
{
	bReplicates = true;
}

void AFGEOSPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalPlayerController()) return;

	VFGGameInstance = GetGameInstance<UFGGameInstance>();
	if (IsValid(VFGGameInstance))
	{
		VFGGameInstance->GetOnOnlineStatusState().RemoveAll(this);
		VFGGameInstance->GetOnOnlineStatusState().AddUObject(this, &AFGEOSPlayerController::OnlineStatusState);
	}

	RegisterInputMappingContextSetting();

	UFGMainMenu* FGMainMenu = CreateWidget<UFGMainMenu>(this, GetUserWidgetClassByName(TEXT("UFGMainMenu")));
	if (IsValid(FGMainMenu))
	{
		FGMainMenu->AddToViewport();
	}

	SetShowMouseCursor(true);

	FInputModeUIOnly InputModeUIOnly;
	SetInputMode(InputModeUIOnly);
}

void AFGEOSPlayerController::OnlineStatusState(bool bOnline)
{
	if (!IsValid(VFGGameInstance)) return;

	//불러오기
	FString InSlotName = FString::Printf(TEXT("PlayerSaveSlot_%s"), *VFGGameInstance->GetUniquePlayerIdFunction());

	UFGLocalPlayerSaveGame* FGLocalPlayerSaveGame = Cast<UFGLocalPlayerSaveGame>(UFGLocalPlayerSaveGame::LoadOrCreateSaveGameForLocalPlayer(UFGLocalPlayerSaveGame::StaticClass(), GetLocalPlayer(), InSlotName));
	if (IsValid(FGLocalPlayerSaveGame))
	{
		FGLocalPlayerSaveGame->LoadSaveGameData();
	}
}

//플레이어 컨트롤러가 네트워크에서 제거될 때 호출
void AFGEOSPlayerController::OnNetCleanup(UNetConnection* Connection)
{
	if (!IsLocalPlayerController()) return;

	UFGGameInstance* FGGameInstance = GetGameInstance<UFGGameInstance>();
	if (FGGameInstance)
	{
		FGGameInstance->DestroySessionFunction();
	}

	Super::OnNetCleanup(Connection);
}

UEnhancedInputUserSettings* AFGEOSPlayerController::GetEnhancedInputUserSettings() const
{
	ULocalPlayer* LocalPlayer = GetLocalPlayer();
	if (IsValid(LocalPlayer))
	{
		UEnhancedInputLocalPlayerSubsystem* EnhancedInputLocalPlayerSubsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer);
		if (IsValid(EnhancedInputLocalPlayerSubsystem))
		{
			//에디터에서 프로젝트 세팅 - 향상된 입력 - User Settings - 사용자 세팅 활성화 체크 해줘야 사용 가능
			UEnhancedInputUserSettings* EnhancedInputUserSettings = EnhancedInputLocalPlayerSubsystem->GetUserSettings();
			if (EnhancedInputUserSettings)
			{
				return EnhancedInputUserSettings;
			}
		}
	}

	return nullptr;
}

void AFGEOSPlayerController::RegisterInputMappingContextSetting()
{
	UEnhancedInputUserSettings* EnhancedInputUserSettings = GetEnhancedInputUserSettings();
	if (!IsValid(EnhancedInputUserSettings)) return;

	for (const auto& ControlChange : ControlChangeMap)
	{
		if (!EnhancedInputUserSettings->IsMappingContextRegistered(ControlChange.Value))
		{
			EnhancedInputUserSettings->RegisterInputMappingContext(ControlChange.Value);
		}
	}

}

void AFGEOSPlayerController::CreateFGSettingsWidget()
{
	if (!bCreateFGSettingsWidget)
	{
		bCreateFGSettingsWidget = true;

		if (IsValid(VFGSettings) && VFGSettings->IsInViewport())
		{
			VFGSettings->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			VFGSettings = CreateWidget<UFGSettings>(this, GetUserWidgetClassByName(TEXT("UFGSettings")));
			if (IsValid(VFGSettings) && !VFGSettings->IsInViewport())
			{
				VFGSettings->AddToViewport(999);
			}
		}
	}
	else
	{
		bCreateFGSettingsWidget = false;

		if (IsValid(VFGSettings) && VFGSettings->IsInViewport())
		{
			VFGSettings->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

TSubclassOf<UUserWidget> AFGEOSPlayerController::GetUserWidgetClassByName(const FName& InName) const
{
	if (const TSubclassOf<UUserWidget>* FindUserWidgetClass = UserWidgetClassMap.Find(InName))
	{
		return *FindUserWidgetClass;
	}

	UE_LOG(LogTemp, Error, TEXT("InName is not found in GetUserWidgetClassByName of %s"), *GetNameSafe(this));

	return nullptr;
}
