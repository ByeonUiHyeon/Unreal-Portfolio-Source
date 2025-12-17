// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/FGLobbyPlayerController.h"
#include "Chat/FGChatComponent.h"
#include "Session/FGLobbyWidget.h"
#include "Game/FGGameInstance.h"

AFGLobbyPlayerController::AFGLobbyPlayerController()
{
	VFGChatComponent = CreateDefaultSubobject<UFGChatComponent>(TEXT("VFGChatComponent"));
	//에디터에서 BP_FGChatComponent로 변경하기
}

void AFGLobbyPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalPlayerController()) return;

	if (IsValid(FGLobbyWidgetClass))
	{
		UFGLobbyWidget* FGLobbyWidget = CreateWidget<UFGLobbyWidget>(this, FGLobbyWidgetClass);
		if (FGLobbyWidget)
		{
			FGLobbyWidget->AddToViewport();
		}

		SetShowMouseCursor(true);

		FInputModeGameAndUI InputModeGameAndUI;
		InputModeGameAndUI.SetHideCursorDuringCapture(false);
		SetInputMode(InputModeGameAndUI);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("FGLobbyWidgetClass is not valid"));
	}
}

void AFGLobbyPlayerController::KickPlayer(const FName& InSessionName)
{
	if (HasAuthority())
	{
		Client_KickPlayer(InSessionName);
	}
}

void AFGLobbyPlayerController::Client_KickPlayer_Implementation(const FName& InSessionName)
{
	IOnlineSubsystem* VOnlineSubSystem = IOnlineSubsystem::Get(EOS_SUBSYSTEM);
	if (VOnlineSubSystem)
	{
		IOnlineSessionPtr SessionPtr = VOnlineSubSystem->GetSessionInterface();
		if (SessionPtr.IsValid())
		{
			SessionPtr->DestroySession(InSessionName);
		}
	}
}
