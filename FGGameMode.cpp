// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/FGGameMode.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"
#include "Character/FGCharacterPlayer.h"
#include "Player/FGPlayerState.h"
#include "Game/FGGameInstance.h"
#include "Player/FGPlayerController.h"
#include "UserWidgetHUD/FGSelectedCharacterHpMp.h"
#include "Game/FGGameState.h"

//디폴트 폰 스폰 위치
AActor* AFGGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
    UFGGameInstance* FGGameInstance = GetGameInstance<UFGGameInstance>();
    if (!IsValid(FGGameInstance)) return nullptr;

    AFGPlayerState* FGPlayerState = Player->GetPlayerState<AFGPlayerState>();
    if (!IsValid(FGPlayerState)) return nullptr;

    if (const FSaveSelectedCharacterStruct* SaveSelectedCharacterStruct = FGGameInstance->FindSaveSelectedCharacterStructMap(RetrunPlayerNumber(FGPlayerState)))
    {
        if (SaveSelectedCharacterStruct->TeamType == 0)
        {
            return GetTeamPlayerStart(0);
        }
        else if (SaveSelectedCharacterStruct->TeamType == 1)
        {
            return GetTeamPlayerStart(1);
        }
    }

    return Super::ChoosePlayerStart_Implementation(Player);
}

int32 AFGGameMode::RetrunPlayerNumber(AFGPlayerState* InFGPlayerState)
{
    FString Name = InFGPlayerState->GetName();
    FString Left, Right;
    if (!Name.Split(TEXT("_"), &Left, &Right, ESearchCase::IgnoreCase, ESearchDir::FromEnd)) return int32();

    return FCString::Atoi(*Right);
}

APlayerStart* AFGGameMode::GetTeamPlayerStart(int32 InTeamIndex)
{
    APlayerStart* TeamAPlayerStart = nullptr;
    APlayerStart* TeamBPlayerStart = nullptr;

    TArray<AActor*> PlayerStarts;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerStart::StaticClass(), PlayerStarts);
    for (AActor* Actor : PlayerStarts)
    {
        APlayerStart* PlayerStart = Cast<APlayerStart>(Actor);
        if (PlayerStart->PlayerStartTag == TEXT("TeamA"))
        {
            TeamAPlayerStart = PlayerStart;
        }
        else if (PlayerStart->PlayerStartTag == TEXT("TeamB"))
        {
            TeamBPlayerStart = PlayerStart;
        }
    }

    return InTeamIndex == 0 ? TeamAPlayerStart : TeamBPlayerStart;
}

void AFGGameMode::SpawnSelectedCharacter(AFGPlayerController* InFGPlayerController, int32 InPlayerNumber, TArray<TObjectPtr<UTexture2D>> InSelectedCharacterImageArray, TArray<TSubclassOf<AFGCharacterPlayer>> InFGCharacterPlayerArray, APlayerStart* InPlayerStart, int32 InTeamIndex)
{
    FVector SpawnLocation = InPlayerStart->GetActorLocation();
    FRotator SpawnRotation = InPlayerStart->GetActorRotation();
    FTransform SpawnTransform = FTransform(SpawnRotation, SpawnLocation);

    for (int32 i = 0; i < InFGCharacterPlayerArray.Num(); ++i)
    {
        if (!IsValid(InFGCharacterPlayerArray[i])) continue;

        AFGCharacterPlayer* FGCharacterPlayer = GetWorld()->SpawnActorDeferred<AFGCharacterPlayer>(InFGCharacterPlayerArray[i], SpawnTransform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
        if (IsValid(FGCharacterPlayer))
        {
            FGCharacterPlayer->SetSpawnFGPlayerController(InFGPlayerController);
            FGCharacterPlayer->SetCharacterImage(InSelectedCharacterImageArray[i]);
            FGCharacterPlayer->SetCharacterPlayerClass(InFGCharacterPlayerArray[i]);
            FGCharacterPlayer->SetOwnPlayerNumber(InPlayerNumber);
            FGCharacterPlayer->SetTeamIndex(InTeamIndex);
            FGCharacterPlayer->SetSelectedCharacterNumber(i);

            if (InTeamIndex == 0)
            {
                VFGGameState->AddTeamACharacterPlayerArray(FGCharacterPlayer);
            }
            else
            {
                VFGGameState->AddTeamBCharacterPlayerArray(FGCharacterPlayer);
            }

            FSelectedCharacterPlayerStruct SelectedCharacterPlayerStruct;
            SelectedCharacterPlayerStruct.SelectedCharacterNumber = i;
            SelectedCharacterPlayerStruct.SelectedCharacter = FGCharacterPlayer;
            InFGPlayerController->GetSelectedCharacterPlayerStructArray().Add(SelectedCharacterPlayerStruct);

            UGameplayStatics::FinishSpawningActor(FGCharacterPlayer, SpawnTransform);

            FGCharacterPlayer->SetPlayerState(InFGPlayerController->PlayerState);
            //PlayerName 얻기 위해
        }
    }

    if (InTeamIndex == 0)
    {
        VFGGameState->AddTeamAPlayerControllerArray(InFGPlayerController);
    }
    else
    {
        VFGGameState->AddTeamBPlayerControllerArray(InFGPlayerController);
    }

    InFGPlayerController->Client_SelectCharacter1();
}

void AFGGameMode::SpawnAllCharacters(AFGPlayerController* InFGPlayerController)
{
    UFGGameInstance* FGGameInstance = GetGameInstance<UFGGameInstance>();
    if (!IsValid(FGGameInstance)) return;

    VFGGameState = GetWorld()->GetGameState<AFGGameState>();
    if (!IsValid(VFGGameState)) return;

    AFGPlayerState* FGPlayerState = InFGPlayerController->GetPlayerState<AFGPlayerState>();
    if (!IsValid(FGPlayerState)) return;

    int32 PlayerNumber = RetrunPlayerNumber(FGPlayerState);

    if (const FSaveSelectedCharacterStruct* SaveSelectedCharacterStruct = FGGameInstance->FindSaveSelectedCharacterStructMap(PlayerNumber))
    {
        if (SaveSelectedCharacterStruct->TeamType == 0)
        {
            InFGPlayerController->SetOwnPlayerNumber(PlayerNumber);
            InFGPlayerController->SetTeamIndex(0);
            FGPlayerState->SetOwnPlayerNumber(PlayerNumber);
            FGPlayerState->SetTeamIndex(0);

            SpawnSelectedCharacter(InFGPlayerController, PlayerNumber, SaveSelectedCharacterStruct->SaveSelectedCharacterImageArray, SaveSelectedCharacterStruct->SaveSelectedCharacterClassArray, GetTeamPlayerStart(0), 0);
        }
        else if (SaveSelectedCharacterStruct->TeamType == 1)
        {
            InFGPlayerController->SetOwnPlayerNumber(PlayerNumber);
            InFGPlayerController->SetTeamIndex(1);
            FGPlayerState->SetOwnPlayerNumber(PlayerNumber);
            FGPlayerState->SetTeamIndex(1);

            SpawnSelectedCharacter(InFGPlayerController, PlayerNumber, SaveSelectedCharacterStruct->SaveSelectedCharacterImageArray, SaveSelectedCharacterStruct->SaveSelectedCharacterClassArray, GetTeamPlayerStart(1), 1);
        }
    }
}
