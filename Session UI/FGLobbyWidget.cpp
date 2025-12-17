// Fill out your copyright notice in the Description page of Project Settings.


#include "Session/FGLobbyWidget.h"
#include "Game/FGGameInstance.h"
#include "Components/TextBlock.h"
#include "Game/FGLobbyGameState.h"
#include "Components/ListView.h"
#include "Player/FGPlayerState.h"
#include "Components/Button.h"
#include "Kismet/GameplayStatics.h"
#include "Game/FGGameInstance.h"
#include "Player/FGLobbyPlayerController.h"
#include "Session/FGSelectCharacter.h"
#include "Components/WrapBox.h"
#include "Game/FGGameInstance.h"
#include "Data/FGSelectCharacterDataAsset.h"
#include "Player/FGLobbyPlayerController.h"

void UFGLobbyWidget::NativeConstruct()
{
	Super::NativeConstruct();

    VFGGameInstance = GetGameInstance<UFGGameInstance>();
    if (!IsValid(VFGGameInstance)) return;

    VFGLobbyGameState = GetWorld()->GetGameState<AFGLobbyGameState>();
	if (IsValid(VFGLobbyGameState))
	{
        LobbyNameTextBlock->SetText(FText::FromString(VFGLobbyGameState->GetRoomNameString()));
	}

    VFGLobbyPlayerController = Cast<AFGLobbyPlayerController>(GetOwningPlayer());
    if (!IsValid(VFGLobbyPlayerController)) return;

    if (!GetOwningPlayer()->HasAuthority())
    {
        StartButton->SetVisibility(ESlateVisibility::Collapsed);
    }

    if (TeamASelect->OnClicked.IsBound()) TeamASelect->OnClicked.RemoveAll(this);
    TeamASelect->OnClicked.AddDynamic(this, &UFGLobbyWidget::SetTeamA);

    if (TeamBSelect->OnClicked.IsBound()) TeamBSelect->OnClicked.RemoveAll(this);
    TeamBSelect->OnClicked.AddDynamic(this, &UFGLobbyWidget::SetTeamB);

    if (StartButton->OnClicked.IsBound()) StartButton->OnClicked.RemoveAll(this);
    StartButton->OnClicked.AddDynamic(this, &UFGLobbyWidget::StartGameFunction);

    if (ExitButton->OnClicked.IsBound()) ExitButton->OnClicked.RemoveAll(this);
    ExitButton->OnClicked.AddDynamic(this, &UFGLobbyWidget::OnExitButtonClicked);

    SelectCharacterArray();

    GetWorld()->GetTimerManager().SetTimer(PlayerListUpdateTimer, this, &UFGLobbyWidget::UpdateTeamListView, 1.f, true);
}

void UFGLobbyWidget::UpdateTeamListView()
{
    //리스트 업데이트
    if (!IsValid(TeamAListView) || !IsValid(TeamBListView)) return;

    TeamAPlayers.Empty();
    TeamBPlayers.Empty();

    if (VFGLobbyGameState->PlayerArray.Num() > 0)
    {
        for (APlayerState* PlayerState : VFGLobbyGameState->PlayerArray)
        {
            AFGPlayerState* FGPlayerState = Cast<AFGPlayerState>(PlayerState);
            if (!FGPlayerState) continue;

            if (FGPlayerState->GetTeamIndex() == 0)
            {
                TeamAPlayers.Add(FGPlayerState);
            }
            else if (FGPlayerState->GetTeamIndex() == 1)
            {
                TeamBPlayers.Add(FGPlayerState);
            }
        }

        TeamAListView->SetListItems(TeamAPlayers);
        TeamBListView->SetListItems(TeamBPlayers);
    }

    //게임 시작 활성화
    bool bCanStartGame = true;

    for (const auto& TeamAPlayer : TeamAPlayers)
    {
        if (TeamAPlayer->GetSelectedCharacterStructArray().Num() != 3)
        {
            bCanStartGame = false;

            break;
        }
    }

    for (const auto& TeamBPlayer : TeamBPlayers)
    {
        if (TeamBPlayer->GetSelectedCharacterStructArray().Num() != 3)
        {
            bCanStartGame = false;

            break;
        }
    }

    if (TeamAListView->GetNumItems() == 0 || TeamBListView->GetNumItems() == 0)
    {
        bCanStartGame = false;
    }

    StartButton->SetIsEnabled(bCanStartGame);
}

void UFGLobbyWidget::SelectCharacterArray()
{
    if (VFGLobbyPlayerController->GetFGSelectCharacterClass())
    {
        if (VFGLobbyGameState->GetFGSelectCharacterDataAsset()->SelectCharacterMap.Num() <= SelectCharacterArraySize)
        {
            //캐릭터부터 위젯 생성
            SelectCharacterArrayFunction();

            //나머지 빈칸 위젯 생성
            for (int32 i = VFGLobbyGameState->GetFGSelectCharacterDataAsset()->SelectCharacterMap.Num(); i < SelectCharacterArraySize; i++)
            {
                UFGSelectCharacter* FGSelectCharacter = CreateWidget<UFGSelectCharacter>(VFGLobbyPlayerController, VFGLobbyPlayerController->GetFGSelectCharacterClass());
                if (IsValid(FGSelectCharacter))
                {
                    FGSelectCharacter->SetFGSelectCharacterParentType(EFGSelectCharacterParentType::FGLobbyWidgetType);

                    SelectCharacterWrapBox->AddChildToWrapBox(FGSelectCharacter);

                    FMargin FGSelectCharacterPadding(10.f, 10.f, 10.f, 10.f);
                    FGSelectCharacter->SetPadding(FGSelectCharacterPadding);
                }
            }
        }
        else
        {
            SelectCharacterArrayFunction();
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("FGSelectCharacterClass is not valid"));
    }
}

void UFGLobbyWidget::SelectCharacterArrayFunction()
{
    if (VFGLobbyGameState->GetFGSelectCharacterDataAsset()->SelectCharacterMap.Num() > 0)
    {
        for (auto& SelectCharacterElement : VFGLobbyGameState->GetFGSelectCharacterDataAsset()->SelectCharacterMap)
        {
            UFGSelectCharacter* FGSelectCharacter = CreateWidget<UFGSelectCharacter>(VFGLobbyPlayerController, VFGLobbyPlayerController->GetFGSelectCharacterClass());
            if (IsValid(FGSelectCharacter))
            {
                FGSelectCharacter->SetFGSelectCharacterParentType(EFGSelectCharacterParentType::FGLobbyWidgetType);

                FGSelectCharacter->SetCharacterTexture2D(SelectCharacterElement.Key);
                FGSelectCharacter->SetFGCharacterPlayerClass(SelectCharacterElement.Value);

                SelectCharacterWrapBox->AddChildToWrapBox(FGSelectCharacter);

                FMargin FGSelectCharacterPadding(10.f, 10.f, 10.f, 10.f);
                FGSelectCharacter->SetPadding(FGSelectCharacterPadding);
            }
        }
    }
}

void UFGLobbyWidget::SetTeamA()
{
    AFGPlayerState* FGPlayerState = GetOwningPlayer()->GetPlayerState<AFGPlayerState>();
    if (IsValid(FGPlayerState))
    {
        FGPlayerState->SetTeamIndex(0);
    }
}

void UFGLobbyWidget::SetTeamB()
{
    AFGPlayerState* FGPlayerState = GetOwningPlayer()->GetPlayerState<AFGPlayerState>();
    if (IsValid(FGPlayerState))
    {
        FGPlayerState->SetTeamIndex(1);
    }
}

void UFGLobbyWidget::StartGameFunction()
{
    VFGGameInstance->StartGameLevel();
}

void UFGLobbyWidget::OnExitButtonClicked()
{
    VFGGameInstance->DestroySessionFunction();
}
