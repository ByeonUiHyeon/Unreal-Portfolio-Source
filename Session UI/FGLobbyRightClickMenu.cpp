// Fill out your copyright notice in the Description page of Project Settings.


#include "Session/FGLobbyRightClickMenu.h"
#include "Components/Button.h"
#include "Game/FGGameInstance.h"

UFGLobbyRightClickMenu::UFGLobbyRightClickMenu(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	bIsFocusable = true;
}

void UFGLobbyRightClickMenu::NativeConstruct()
{
	Super::NativeConstruct();

	if (CloseButton->OnClicked.IsBound()) CloseButton->OnClicked.RemoveAll(this);
	CloseButton->OnClicked.AddDynamic(this, &UFGLobbyRightClickMenu::OnCloseButtonClicked);

	if (KickOutButton->OnClicked.IsBound()) KickOutButton->OnClicked.RemoveAll(this);
	KickOutButton->OnClicked.AddDynamic(this, &UFGLobbyRightClickMenu::OnKickOutButtonClicked);

	SetKeyboardFocus();
}

FReply UFGLobbyRightClickMenu::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Escape)
	{
		RemoveFromParent();

		return FReply::Handled();
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UFGLobbyRightClickMenu::OnCloseButtonClicked()
{
	UE_LOG(LogTemp, Log, TEXT("OnCloseButtonClicked"));

	RemoveFromParent();
}

void UFGLobbyRightClickMenu::OnKickOutButtonClicked()
{
	UFGGameInstance* FGGameInstance = GetGameInstance<UFGGameInstance>();
	if (IsValid(FGGameInstance))
	{
		FGGameInstance->RemovePlayerFromSessionFunction(VFGPlayerState);

		RemoveFromParent();
	}
}
