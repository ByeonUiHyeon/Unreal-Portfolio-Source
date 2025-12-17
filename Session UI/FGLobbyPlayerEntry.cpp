// Fill out your copyright notice in the Description page of Project Settings.


#include "Session/FGLobbyPlayerEntry.h"
#include "Player/FGPlayerState.h"
#include "Components/TextBlock.h"
#include "Game/FGGameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Session/FGSelectCharacter.h"
#include "Components/WrapBox.h"
#include "Components/Button.h"
#include "Player/FGLobbyPlayerController.h"
#include "Session/FGLobbyRightClickMenu.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/CanvasPanelSlot.h"

void UFGLobbyPlayerEntry::NativeConstruct()
{
	Super::NativeConstruct();

	VFGLobbyPlayerController = Cast<AFGLobbyPlayerController>(GetOwningPlayer());
	if (!IsValid(VFGLobbyPlayerController)) return;
}

void UFGLobbyPlayerEntry::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	IUserObjectListEntry::NativeOnListItemObjectSet(ListItemObject);

	VFGPlayerState = Cast<AFGPlayerState>(ListItemObject);
	if (IsValid(VFGPlayerState))
	{
		PlayerNameTextBlock->SetText(FText::FromString(VFGPlayerState->GetPlayerName()));

		VFGPlayerState->GetOnSelectCharacterClicked().RemoveAll(this);
		VFGPlayerState->GetOnSelectCharacterClicked().AddUObject(this, &UFGLobbyPlayerEntry::UpdateSelectCharacterWrapBox);

		UpdateSelectCharacterWrapBox();
	}
}

FReply UFGLobbyPlayerEntry::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (GetOwningPlayer()->HasAuthority())
	{
		if (VFGLobbyPlayerController->PlayerState == VFGPlayerState) return FReply::Unhandled();

		if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
		{
			//UListView는 리스트 마다 위젯 따로 생기므로 배열로 처리
			for (int32 i = VFGLobbyPlayerController->FGLobbyRightClickMenuArray.Num() - 1; i >= 0; --i)
			{
				UFGLobbyRightClickMenu* FGLobbyRightClickMenu = VFGLobbyPlayerController->FGLobbyRightClickMenuArray[i];
				if (IsValid(FGLobbyRightClickMenu))
				{
					FGLobbyRightClickMenu->RemoveFromParent();

					VFGLobbyPlayerController->FGLobbyRightClickMenuArray.RemoveAt(i);
				}

			}

			CreateFGLobbyRightClickMenuWidget(InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition()), InMouseEvent);

			return FReply::Handled();
		}
	}

	return FReply::Unhandled();
}

void UFGLobbyPlayerEntry::CreateFGLobbyRightClickMenuWidget(FVector2D ClickPosition, const FPointerEvent& InMouseEvent)
{
	if (!VFGLobbyPlayerController->GetFGLobbyRightClickMenuClass()) return;

	VFGLobbyRightClickMenu = CreateWidget<UFGLobbyRightClickMenu>(VFGLobbyPlayerController, VFGLobbyPlayerController->GetFGLobbyRightClickMenuClass());
	if (IsValid(VFGLobbyRightClickMenu))
	{
		VFGLobbyPlayerController->FGLobbyRightClickMenuArray.Add(VFGLobbyRightClickMenu);
		VFGLobbyRightClickMenu->SetFGPlayerState(VFGPlayerState);
		VFGLobbyRightClickMenu->AddToViewport();

		//마우스 위치에 생성
		//위젯 앵커 왼쪽 상단으로 해야함
		//종료 버튼 추가하려면 왼쪽 상단 뚫어서 넓혀야한다
		FVector2D MousePosition;
		if (VFGLobbyPlayerController->GetMousePosition(MousePosition.X, MousePosition.Y))
		{
			VFGLobbyRightClickMenu->SetPositionInViewport(MousePosition);
		}
	}
}

void UFGLobbyPlayerEntry::UpdateSelectCharacterWrapBox()
{
	SelectCharacterWrapBox->ClearChildren();

	if (!VFGLobbyPlayerController->GetFGSelectCharacterClass()) return;

	for (int32 i = 0; i < 3; i++)
	{
		UFGSelectCharacter* FGSelectCharacter = CreateWidget<UFGSelectCharacter>(VFGLobbyPlayerController, VFGLobbyPlayerController->GetFGSelectCharacterClass());
		if (!IsValid(FGSelectCharacter)) continue;

		FGSelectCharacter->SetIndex(i);
		FGSelectCharacter->SetFGSelectCharacterParentType(EFGSelectCharacterParentType::FGLobbyPlayerEntryType);

		AFGPlayerState* LocalPlayerState = GetOwningPlayer()->GetPlayerState<AFGPlayerState>();
		if (!IsValid(LocalPlayerState)) continue;

		//자기 캐릭터 리스트만 버튼 클릭 가능
		if (LocalPlayerState == VFGPlayerState)
		{
			FGSelectCharacter->SetbClickable(true);
		}
		else
		{
			FGSelectCharacter->SetbClickable(false);
		}

		if (VFGPlayerState->GetSelectedCharacterStructArray().IsValidIndex(i))
		{
			FGSelectCharacter->SetCharacterTexture2D(VFGPlayerState->GetSelectedCharacterStructArray()[i].SelectedCharacterTexture2D);
		}

		SelectCharacterWrapBox->AddChildToWrapBox(FGSelectCharacter);

		FMargin FGSelectCharacterPadding(0.f, 0.f, 20.f, 0.f);
		FGSelectCharacter->SetPadding(FGSelectCharacterPadding);
	}
}
