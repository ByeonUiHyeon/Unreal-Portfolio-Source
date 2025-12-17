// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/FGLobbyGameMode.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Game/FGGameInstance.h"
#include "Game/FGLobbyGameState.h"

#include "OnlineSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Player/FGLobbyPlayerController.h"
#include "Player/FGPlayerState.h"

void AFGLobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);

    if (!IsValid(NewPlayer)) return;

    //플레이어가 세션에 들어왔다는것을 알리는 코드
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////

    UFGGameInstance* FGGameInstance = GetGameInstance<UFGGameInstance>();
    if (!IsValid(FGGameInstance)) return;

    FUniqueNetIdRepl UniqueNetIdRepl;
    if (NewPlayer->IsLocalPlayerController())
    {
        ULocalPlayer* LocalPlayer = NewPlayer->GetLocalPlayer();
        if (IsValid(LocalPlayer))
        {
            UniqueNetIdRepl = LocalPlayer->GetPreferredUniqueNetId();
        }
        else
        {
            UNetConnection* NetConnection = Cast<UNetConnection>(NewPlayer->Player);
            if (IsValid(NetConnection))
            {
                UniqueNetIdRepl = NetConnection->PlayerId;
            }
        }
    }
    else
    {
        UNetConnection* NetConnection = Cast<UNetConnection>(NewPlayer->Player);
        if (IsValid(NetConnection))
        {
            UniqueNetIdRepl = NetConnection->PlayerId;
        }
    }

    if (UniqueNetIdRepl.IsValid())
    {
        IOnlineSubsystem* OnlineSubsystem = Online::GetSubsystem(NewPlayer->GetWorld());
        if (!OnlineSubsystem) return;
        //GetSubsystem() : EOS/Steam 같은 온라인 서브시스템 가져오기

        IOnlineSessionPtr OnlineSessionPtr = OnlineSubsystem->GetSessionInterface();
        if (!OnlineSessionPtr.IsValid()) return;
        //GetSessionInterface() : 세션 관리 객체 가져오기

        TSharedPtr<const FUniqueNetId> UniqueNetId = UniqueNetIdRepl.GetUniqueNetId();
        if (!UniqueNetId.IsValid()) return;

        //RegisterPlayer() : 해당 세션에 유저 등록(JoinSession 할 경우 등록됨)
        //MainSession이라는 이름의 세션에 플레이어 UniqueNetId 등록
        //RegisterPlayer의 3번 째 매개변수는
        //true -> 플레이어가 초대를 통해 참가한 경우
        //false -> 그냥 일반적으로 접속한 경우
        bool bRegisterPlayer = OnlineSessionPtr->RegisterPlayer(FGGameInstance->GetGameSessionName(), *UniqueNetId, false);
        if (bRegisterPlayer)
        {
            UE_LOG(LogTemp, Warning, TEXT("Registration Successful"));
        }
    }

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////
}

void AFGLobbyGameMode::InitGameState()
{
	Super::InitGameState();

    AFGLobbyGameState* CFGLobbyGameState = Cast<AFGLobbyGameState>(GameState);
    if (!IsValid(CFGLobbyGameState)) return;

	UFGGameInstance* FGGameInstance = GetGameInstance<UFGGameInstance>();
	if (IsValid(FGGameInstance))
	{
        CFGLobbyGameState->SetRoomNameString(FGGameInstance->GetRoomNameString());
	}
}
