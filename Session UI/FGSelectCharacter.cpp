// Fill out your copyright notice in the Description page of Project Settings.


#include "Session/FGSelectCharacter.h"
#include "Components/Button.h"
#include "Game/FGGameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Character/FGCharacterPlayer.h"
#include "Components/Image.h"
#include "Player/FGPlayerState.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/Border.h"

UFGSelectCharacter::UFGSelectCharacter(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	bClickable = true;
}

void UFGSelectCharacter::NativeConstruct()
{
	Super::NativeConstruct();

	if (CharacterTexture2D)
	{
		SelectCharacterButton->SetVisibility(ESlateVisibility::Visible);
		CharacterImage->SetBrushFromTexture(CharacterTexture2D);
		CharacterImage->SetVisibility(ESlateVisibility::Visible);
		EmptyCharacterBorder->SetVisibility(ESlateVisibility::Collapsed);
	}
	else
	{
		SelectCharacterButton->SetVisibility(ESlateVisibility::Collapsed);
		CharacterImage->SetBrushFromTexture(nullptr);
		CharacterImage->SetVisibility(ESlateVisibility::Collapsed);
		EmptyCharacterBorder->SetVisibility(ESlateVisibility::Visible);
	}

	if (bClickable)
	{
		if (SelectCharacterButton->OnClicked.IsBound()) SelectCharacterButton->OnClicked.RemoveAll(this);
		SelectCharacterButton->OnClicked.AddDynamic(this, &UFGSelectCharacter::OnSelectCharacterButtonClicked);
	}
}

void UFGSelectCharacter::OnSelectCharacterButtonClicked()
{
	AFGPlayerState* FGPlayerState = GetOwningPlayer()->GetPlayerState<AFGPlayerState>();
	if (!IsValid(FGPlayerState)) return;

	if (FGSelectCharacterParentType == EFGSelectCharacterParentType::FGLobbyWidgetType)
	{
		if (FGPlayerState->GetSelectedCharacterStructArray().Num() < 3)
		{
			if (!IsValid(FGCharacterPlayerClass) || !IsValid(CharacterTexture2D)) return;

			FSelectedCharacterData SelectedCharacterData;
			SelectedCharacterData.SelectedCharacterTexture2D = CharacterTexture2D;
			SelectedCharacterData.SelectedCharacterClass = FGCharacterPlayerClass;

			//캐릭터 3개 이상 있을 경우 맵으로 변경 //PlayerState, GameInstance, GameMode, CharacterPlayer
			//중복 선택 안돼게 하기 위해
			FGPlayerState->SetSelectedCharacterStructArray(SelectedCharacterData);
		}
	}
	else if (FGSelectCharacterParentType == EFGSelectCharacterParentType::FGLobbyPlayerEntryType)
	{
		//캐릭터 여러개 있을 경우 맵으로 변경
		FGPlayerState->RemoveAtSelectedCharacterStructArray(Index);
	}

	FGPlayerState->UpdateSelectedCharacter();
}
