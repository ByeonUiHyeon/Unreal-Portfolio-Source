// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/FGHUD.h"
#include "UserWidgetHUD/FGUserWidgetHUD.h"
#include "HpMpBar/FGHpMpUserWidget.h"
#include "Player/FGPlayerController.h"
#include "Character/FGCharacterPlayer.h"
#include "Interface/FGRTSInterface.h"
#include "Player/FGPlayerState.h"
#include "UserWidgetHUD/FGSelectedCharacterHpMp.h"
#include "Interface/FGSelectCharacterInterface.h"

void AFGHUD::BeginPlay()
{
    Super::BeginPlay();

    GetWorld()->GetTimerManager().SetTimer(CastFGPlayerControllerTimer, FTimerDelegate::CreateLambda([&]()
        {
            VFGPlayerController = Cast<AFGPlayerController>(GetOwningPlayerController());
            if (IsValid(VFGPlayerController))
            {
                GetWorld()->GetTimerManager().ClearTimer(CastFGPlayerControllerTimer);
            }
        }
    ), 0.5f, true);
}

void AFGHUD::DrawHUD()
{
    if (!bDrawing || !IsValid(VFGPlayerController)) return;

    FLinearColor Color(0.f, 0.f, 1.f, 0.2f);
    DrawRect(Color, CurrentMousePosition.X, CurrentMousePosition.Y, StartMousePosition.X - CurrentMousePosition.X, StartMousePosition.Y - CurrentMousePosition.Y);

    TArray<AActor*> SelectedActors;
    GetActorsInSelectionRectangle(AFGCharacterPlayer::StaticClass(), CurrentMousePosition, StartMousePosition, SelectedActors, false, true);

    TArray<AFGCharacterPlayer*> SelectedCharacters;
    TArray<AFGCharacterPlayer*> SelectedOwnFGCharacterPlayers;

    for (const auto& SelectedActor : SelectedActors)
    {
        AFGCharacterPlayer* SelectedCharacter = Cast<AFGCharacterPlayer>(SelectedActor);
        if (IsValid(SelectedCharacter))
        {
            SelectedCharacters.Add(SelectedCharacter);
        }

        if (IsValid(SelectedCharacter) && !SelectedCharacter->GetbDead() && SelectedCharacter->GetOwnPlayerNumber() == VFGPlayerController->GetOwnPlayerNumber())
        {
            SelectedOwnFGCharacterPlayers.Add(SelectedCharacter);
        }
    }

    for (const auto& SelectedOwnFGCharacterPlayer : SelectedOwnFGCharacterPlayers)
    {
        if (!IsValid(SelectedOwnFGCharacterPlayer)) continue;

        IFGSelectCharacterInterface* FGSelectCharacterInterface = Cast<IFGSelectCharacterInterface>(SelectedOwnFGCharacterPlayer);
        if (FGSelectCharacterInterface)
        {
            FGSelectCharacterInterface->SetSelectCharacterDecalVisibility(true);
        }

        if (IsValid(VFGPlayerController->GetFGSelectedCharacterHpMp()))
        {
            VFGPlayerController->GetFGSelectedCharacterHpMp()->VisibleSelctedCharacterBorderVisibility(SelectedOwnFGCharacterPlayer->GetSelectedCharacterNumber());
        }

        OwnFGCharacterPlayerArray.AddUnique(SelectedOwnFGCharacterPlayer);
    }

    for (int32 i = OwnFGCharacterPlayerArray.Num() - 1; i >= 0; --i)
    {
        AFGCharacterPlayer* OwnFGCharacterPlayer = OwnFGCharacterPlayerArray[i];
        if (!SelectedCharacters.Contains(OwnFGCharacterPlayer))
        {
            IFGSelectCharacterInterface* FGSelectCharacterInterface = Cast<IFGSelectCharacterInterface>(OwnFGCharacterPlayer);
            if (FGSelectCharacterInterface)
            {
                FGSelectCharacterInterface->SetSelectCharacterDecalVisibility(false);
            }

            if (IsValid(VFGPlayerController->GetFGSelectedCharacterHpMp()))
            {
                VFGPlayerController->GetFGSelectedCharacterHpMp()->CollapsedSelctedCharacterBorderVisibility(OwnFGCharacterPlayer->GetSelectedCharacterNumber());
            }

            OwnFGCharacterPlayerArray.RemoveAt(i);
        }
    }

    if (OwnFGCharacterPlayerArray.Num() == 1)
    {
        if (IsValid(VFGCharacterPlayer))
        {
            VFGCharacterPlayer->WeakUnpossess();
            VFGCharacterPlayer = nullptr;
        }

        VFGCharacterPlayer = OwnFGCharacterPlayerArray[0];
        if (IsValid(VFGCharacterPlayer))
        {
            VFGPlayerController->SetFGCharacterPlayer(VFGCharacterPlayer);
            VFGPlayerController->AddAbilityInputComponent(VFGCharacterPlayer->GetSetInputAbilities());

            VFGCharacterPlayer->WeakPossess();

            SelectedCharacterNumber = VFGCharacterPlayer->GetSelectedCharacterNumber();
        }
    }
    else if (OwnFGCharacterPlayerArray.Num() > 1)
    {
        if (!bMiniMapEnter)
        {
            if (IsValid(VFGCharacterPlayer))
            {
                VFGCharacterPlayer->WeakUnpossess();
                VFGCharacterPlayer = nullptr;
            }

            VFGPlayerController->SetFGCharacterPlayer(nullptr);
        }
    }
    else
    {
        HUDSelectCharacter(SelectedCharacterNumber);

        if (IsValid(VFGPlayerController->GetFGSelectedCharacterHpMp()))
        {
            VFGPlayerController->GetFGSelectedCharacterHpMp()->VisibleSelctedCharacterBorderVisibility(SelectedCharacterNumber);
        }
    }
}

void AFGHUD::HUDSelectCharacter(const int32& InSelectedCharacterNumber)
{
    if (!IsValid(VFGPlayerController)) return;

    SelectedCharacterNumber = InSelectedCharacterNumber;

    for (const auto& OwnFGCharacterPlayer : OwnFGCharacterPlayerArray)
    {
        if (!IsValid(OwnFGCharacterPlayer)) continue;

        IFGSelectCharacterInterface* FGSelectCharacterInterface = Cast<IFGSelectCharacterInterface>(OwnFGCharacterPlayer);
        if (FGSelectCharacterInterface)
        {
            FGSelectCharacterInterface->SetSelectCharacterDecalVisibility(false);
        }
    }

    OwnFGCharacterPlayerArray.Reset();

    if (IsValid(VFGCharacterPlayer))
    {
        VFGCharacterPlayer->WeakUnpossess();
        VFGCharacterPlayer = nullptr;
    }

    for (const auto& SelectedCharacterStruct : VFGPlayerController->GetSelectedCharacterPlayerStructArray())
    {
        if (SelectedCharacterStruct.SelectedCharacterNumber == InSelectedCharacterNumber && SelectedCharacterStruct.SelectedCharacter && SelectedCharacterStruct.SelectedCharacter->GetbDead() == false)
        {
            VFGCharacterPlayer = SelectedCharacterStruct.SelectedCharacter;
            if (IsValid(VFGCharacterPlayer))
            {
                IFGSelectCharacterInterface* FGSelectCharacterInterface = Cast<IFGSelectCharacterInterface>(VFGCharacterPlayer);
                if (FGSelectCharacterInterface)
                {
                    FGSelectCharacterInterface->SetSelectCharacterDecalVisibility(true);
                }

                VFGPlayerController->SetFGCharacterPlayer(VFGCharacterPlayer);
                VFGPlayerController->AddAbilityInputComponent(VFGCharacterPlayer->GetSetInputAbilities());

                VFGCharacterPlayer->WeakPossess();

                OwnFGCharacterPlayerArray.AddUnique(VFGCharacterPlayer);
            }
        }
    }
}

void AFGHUD::HUDSelectAllCharacter()
{
    if (!IsValid(VFGPlayerController)) return;

    for (const auto& OwnFGCharacterPlayer : OwnFGCharacterPlayerArray)
    {
        if (!IsValid(OwnFGCharacterPlayer)) continue;

        IFGSelectCharacterInterface* FGSelectCharacterInterface = Cast<IFGSelectCharacterInterface>(OwnFGCharacterPlayer);
        if (FGSelectCharacterInterface)
        {
            FGSelectCharacterInterface->SetSelectCharacterDecalVisibility(false);
        }
    }

    OwnFGCharacterPlayerArray.Reset();

    if (IsValid(VFGCharacterPlayer))
    {
        VFGCharacterPlayer->WeakUnpossess();
        VFGCharacterPlayer = nullptr;
    }

    VFGPlayerController->SetFGCharacterPlayer(nullptr);

    for (const auto& SelectedCharacterStruct : VFGPlayerController->GetSelectedCharacterPlayerStructArray())
    {
        if (SelectedCharacterStruct.SelectedCharacter && SelectedCharacterStruct.SelectedCharacter->GetbDead() == true) continue;

        AFGCharacterPlayer* OwnFGCharacterPlayer = SelectedCharacterStruct.SelectedCharacter;
        if (IsValid(OwnFGCharacterPlayer))
        {
            IFGSelectCharacterInterface* FGSelectCharacterInterface = Cast<IFGSelectCharacterInterface>(OwnFGCharacterPlayer);
            if (FGSelectCharacterInterface)
            {
                FGSelectCharacterInterface->SetSelectCharacterDecalVisibility(true);
            }

            OwnFGCharacterPlayerArray.AddUnique(OwnFGCharacterPlayer);
        }
    }
}

//Interface
void AFGHUD::MarqueePressed()
{
	bDrawing = true;

    FVector2D MousePos = FVector2D::ZeroVector;

    if (IsValid(VFGPlayerController))
    {
        VFGPlayerController->GetMousePosition(MousePos.X, MousePos.Y);
    }

    CurrentMousePosition = MousePos;

    StartMousePosition = CurrentMousePosition;
}

void AFGHUD::MarqueeReleased()
{
	bDrawing = false;
}

void AFGHUD::MarqueeHeld()
{
    FVector2D MousePos = FVector2D::ZeroVector;

    if (IsValid(VFGPlayerController))
    {
        VFGPlayerController->GetMousePosition(MousePos.X, MousePos.Y);
    }

    CurrentMousePosition = MousePos;
}

TArray<AFGCharacterPlayer*> AFGHUD::SelectedCharacterArray()
{
    return OwnFGCharacterPlayerArray;
}
