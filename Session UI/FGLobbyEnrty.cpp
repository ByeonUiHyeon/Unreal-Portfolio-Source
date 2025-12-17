// Fill out your copyright notice in the Description page of Project Settings.


#include "Session/FGLobbyEnrty.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"

void UFGLobbyEnrty::NativeConstruct()
{
	Super::NativeConstruct();

	if (LobbyButton->OnClicked.IsBound()) LobbyButton->OnClicked.RemoveAll(this);
	LobbyButton->OnClicked.AddDynamic(this, &UFGLobbyEnrty::LobbyButtonClicked);
}

void UFGLobbyEnrty::LobbyButtonClicked()
{
	OnLobbyEntrySelected.Broadcast(CheckPasswordString, LobbyIndex);
}

void UFGLobbyEnrty::SetLobbyEntry(const FString& InLobbyName, const FString& InCheckPassword, int32 EntryLobbyIndex)
{
	LobbyTextBlock->SetText(FText::FromString(InLobbyName));
	CheckPasswordString = InCheckPassword;
	LobbyIndex = EntryLobbyIndex;
}
