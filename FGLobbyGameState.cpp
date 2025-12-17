// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/FGLobbyGameState.h"
#include "Net/UnrealNetwork.h"

AFGLobbyGameState::AFGLobbyGameState()
{
	SetReplicates(true);
}

void AFGLobbyGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(AFGLobbyGameState, RoomNameString, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(AFGLobbyGameState, FGSelectCharacterDataAsset, COND_None, REPNOTIFY_Always);
	//DOREPLIFETIME_CONDITION_NOTIFY : 항상 OnRep 호출 (UI 갱신용)
	//COND_None : 항상 복제
	//REPNOTIFY_Always : 값이 같아도 무조건 OnRep_ 호출
}
