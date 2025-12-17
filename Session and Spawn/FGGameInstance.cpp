// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/FGGameInstance.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "Interfaces/OnlinePresenceInterface.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Kismet/GameplayStatics.h"
#include "Online/OnlineSessionNames.h"
#include "Player/FGEOSPlayerController.h"
#include "Game/FGLobbyGameState.h"
#include "Player/FGPlayerState.h"
#include "Player/FGLobbyPlayerController.h"
#include "Player/FGPlayerController.h"

void UFGGameInstance::Init()
{
	Super::Init();

	FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UFGGameInstance::OnPostLoadMap);
	//맵 열리고 바로 실행시키기 위한 딜리게이트

	if (IOnlineSubsystem* GOnlineSubSystem = IOnlineSubsystem::Get(EOS_SUBSYSTEM))
	{
		IdentityPtr = GOnlineSubSystem->GetIdentityInterface();
		if (IdentityPtr.IsValid())
		{
			IdentityPtr->OnLoginCompleteDelegates->AddUObject(this, &UFGGameInstance::OnLoginCompleted);
			IdentityPtr->OnLogoutCompleteDelegates->AddUObject(this, &UFGGameInstance::OnLogoutCompleted);
		}

		PresencePtr = GOnlineSubSystem->GetPresenceInterface();
		if (PresencePtr.IsValid())
		{
			PresencePtr->OnPresenceReceivedDelegates.AddUObject(this, &UFGGameInstance::OnPresenceCompleted);
		}

		SessionPtr = GOnlineSubSystem->GetSessionInterface();
		if (SessionPtr.IsValid())
		{
			SessionPtr->OnCreateSessionCompleteDelegates.AddUObject(this, &UFGGameInstance::OnCreateSessionCompleted);
			SessionPtr->OnFindSessionsCompleteDelegates.AddUObject(this, &UFGGameInstance::OnFindSessionsCompleted);
			SessionPtr->OnJoinSessionCompleteDelegates.AddUObject(this, &UFGGameInstance::OnJoinSessionCompleted);
			SessionPtr->OnDestroySessionCompleteDelegates.AddUObject(this, &UFGGameInstance::OnDestroySessionCompleted);
		}
	}
}

//온라인 상태
void UFGGameInstance::OnPresenceCompleted(const FUniqueNetId& UserId, const TSharedRef<class FOnlineUserPresence>& Presence)
{
	//switch (Presence->Status.State)
	//{
	//case EOnlinePresenceState::Online:
	//	OnOnlineStatusState.Broadcast(true);
	//	UE_LOG(LogTemp, Log, TEXT("Player %s is Online"), *UserId.ToString());
	//	break;

	//case EOnlinePresenceState::Away:
	//	UE_LOG(LogTemp, Log, TEXT("Player %s is Away"), *UserId.ToString());
	//	break;

	//case EOnlinePresenceState::Offline:
	//	OnOnlineStatusState.Broadcast(false);
	//	UE_LOG(LogTemp, Log, TEXT("Player %s is Offline"), *UserId.ToString());
	//	break;

	//case EOnlinePresenceState::DoNotDisturb:
	//	UE_LOG(LogTemp, Log, TEXT("Player %s is DoNotDisturb"), *UserId.ToString());
	//	break;

	//default:
	//	break;
	//}
}

//일반 로그인
void UFGGameInstance::LoginFunction()
{
	if (IdentityPtr.IsValid())
	{
		//FOnlineAccountCredentials OnlineAccountCredentials("AccountPortal", "", "");
		//아래와 같다

		FOnlineAccountCredentials OnlineAccountCredentials;
		OnlineAccountCredentials.Type = "AccountPortal";
		OnlineAccountCredentials.Id = "";
		OnlineAccountCredentials.Token = "";

		IdentityPtr->Login(0, OnlineAccountCredentials);
	}
}

//EOS 개발자 로그인
void UFGGameInstance::DeveloperLoginFunction(const FString& InDevId, const FString& InDevToken)
{
	if (IdentityPtr.IsValid())
	{
		FOnlineAccountCredentials OnlineAccountCredentials;
		OnlineAccountCredentials.Type = "Developer";
		OnlineAccountCredentials.Id = FString::Printf(TEXT("127.0.0.1:%s"), *InDevId);
		OnlineAccountCredentials.Token = InDevToken;

		IdentityPtr->Login(0, OnlineAccountCredentials);
	}
}

void UFGGameInstance::OnLoginCompleted(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& UserId, const FString& Error)
{
	if (bWasSuccessful)
	{
		bLoginCompleted = true;

		OnOnlineStatusState.Broadcast(true);

		UE_LOG(LogTemp, Log, TEXT("%d Uwer Login Success, UserId : %s"), LocalUserNum, *UserId.ToString());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Login Fail : %s"), *Error);
	}
}

//로그아웃
void UFGGameInstance::LogoutFunction()
{
	if (IdentityPtr.IsValid())
	{
		IdentityPtr->Logout(0);
	}
}

void UFGGameInstance::OnLogoutCompleted(int32 LocalUserNum, bool bWasSuccessful)
{
	if (bWasSuccessful)
	{
		bLoginCompleted = false;

		OnOnlineStatusState.Broadcast(false);

		UE_LOG(LogTemp, Log, TEXT("%d Uwer Login Success"), LocalUserNum);
	}
}

//닉네임
FString UFGGameInstance::GetPlayerNicknameFunction()
{
	if (IdentityPtr)
	{
		if (IdentityPtr->GetLoginStatus(0) == ELoginStatus::LoggedIn)
		{
			return IdentityPtr->GetPlayerNickname(0);
		}
	}

	return FString();
}

//유니크 아이디
FString UFGGameInstance::GetUniquePlayerIdFunction()
{
	if (IdentityPtr)
	{
		if (IdentityPtr->GetLoginStatus(0) == ELoginStatus::LoggedIn)
		{
			TSharedPtr<const FUniqueNetId> UniqueId = IdentityPtr->GetUniquePlayerId(0);
			if (UniqueId.IsValid())
			{
				FString UniqueIdString = UniqueId->ToString();
				//UniqueIdString.ReplaceInline(TEXT("|"), TEXT("_"));
				UniqueIdString.ReplaceInline(TEXT("|"), TEXT(""));

				return UniqueIdString;
			}
		}
	}

	return FString();
}

//세션 생성
void UFGGameInstance::CreateSessionFunction(const FString& InRoomName, bool bIsChecked, const FString& InPassword)
{
	if (!bLoginCompleted || !SessionPtr.IsValid()) return;

	SessionSettings = MakeShareable(new FOnlineSessionSettings());
	if (!SessionSettings.IsValid()) return;

	SessionSettings->bIsDedicated = false;
	//true : 전용 서버에서 실행(서버만 실행, 플레이어는 없음)
	//false : 일반 클라이언트가 호스트를 겸함(Listen Server 방식)
	SessionSettings->bIsLANMatch = false;
	//true : 인터넷 대신 로컬 네트워크에서만 탐색/참가 가능
	//false : 온라인 서비스(스팀, EOS 등)를 통해 검색
	SessionSettings->NumPublicConnections = 6;
	//공개 슬롯 수(플레이어 최대 참가 가능 인원)
	SessionSettings->bUsesPresence = true;
	//온라인 플랫폼(스팀, EOS, PSN 등)의 상태 표시
	SessionSettings->bUseLobbiesIfAvailable = true;
	//로비 시스템 사용 여부
	SessionSettings->bAllowInvites = true;
	//초대 허용 여부
	SessionSettings->bAllowJoinViaPresence = true;
	//Presence를 통한 친구 초대/참가 허용 여부
	//Presence : 온라인 플랫폼(스팀, EOS, PSN 등)
	//SessionSettings->bAllowJoinViaPresenceFriendsOnly = true;
	//친구만 초대 가능
	SessionSettings->bShouldAdvertise = true;
	//세션을 공개적으로 광고할지 여부
	SessionSettings->bAllowJoinInProgress = false;
	//게임이 이미 시작된 후 참가 가능한지 여부

	SessionSettings->Set(ChannelName, ChannelNumber, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	//어느 채널에 만들지 설정

	RoomNameString = InRoomName;
	//표시 방 이름

	SessionSettings->Set(RoomName, InRoomName, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	//방이름 설정

	//내부 방 이름 설정
	int32 RandomNumber = FMath::RandRange(0, 99999);
	int64 Timestamp = FDateTime::UtcNow().ToUnixTimestamp();
	FString GameSessionNameString = FString::Printf(TEXT("%s_%d_%lld"), *InRoomName, RandomNumber, Timestamp);
	GameSessionName = *GameSessionNameString;
	SessionSettings->Set(UniqueGameSessionName, GameSessionNameString, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

	//비밀 번호 있는지 정하기
	FString bCheckPassword = bIsChecked ? TEXT("true") : TEXT("false");
	SessionSettings->Set(CheckPasswordName, bCheckPassword, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

	if (bIsChecked)
	{
		SessionSettings->Set(PasswordName, InPassword, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
		//비밀번호 있으면 비밀번호 설정
	}

	SessionPtr->CreateSession(0, GameSessionName, *SessionSettings);
}















void UFGGameInstance::OnCreateSessionCompleted(FName InName, bool bSuccessful)
{
	if (bSuccessful)
	{
		LoadLevelAndListen(LobbyLevel);
	}
}

void UFGGameInstance::LoadLevelAndListen(TSoftObjectPtr<UWorld> LevelToLoad)
{
	if (!LevelToLoad.IsValid())
	{
		LevelToLoad.LoadSynchronous();
	}

	if (LevelToLoad.IsValid())
	{
		const FName LevelName = FName(*FPackageName::ObjectPathToPackageName(LevelToLoad.ToString()));
		GetWorld()->ServerTravel(LevelName.ToString() + "?listen");
	}
}









//세션 찾기
void UFGGameInstance::FindSessionsFunction()
{
	if (SessionPtr.IsValid())
	{
		if (SessionSearchPtr.IsValid())
		{
			SessionSearchPtr.Reset();
		}

		SessionSearchPtr = MakeShareable(new FOnlineSessionSearch());
		if (SessionSearchPtr.IsValid())
		{
			SessionSearchPtr->bIsLanQuery = false;
			//true : 로컬 네트워크(LAN)에서만 세션 찾음
			//false : 온라인 서비스(스팀, EOS 등)를 통해 세션 찾음
			SessionSearchPtr->MaxSearchResults = 100;
			//검색 결과 최대 개수 제한

			SessionSearchPtr->QuerySettings.Set(SEARCH_LOBBIES, true, EOnlineComparisonOp::Equals);
			//로비 타입 세션만 검색

			SessionSearchPtr->QuerySettings.Set(ChannelName, ChannelNumber, EOnlineComparisonOp::Equals);
			//채널 검색
			//이 키, 값이 맞는게 하나라도 있어야 세션들을 찾을 수 있다.

			SessionPtr->FindSessions(0, SessionSearchPtr.ToSharedRef());
		}
	}
}

void UFGGameInstance::OnFindSessionsCompleted(bool bSuccessful)
{
	if (bSuccessful)
	{
		if (SessionSearchPtr->SearchResults.Num() > 0)
		{
			OnSessionSearchCompleted.Broadcast(SessionSearchPtr->SearchResults);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("SessionSearchPtr->SearchResults.Num() is None"));
		}
	}
}










//세션 참여
void UFGGameInstance::JoinSessionFunctionIndex(const FString& InPassword, int32 Index)
{
	if (!SessionSearchPtr->SearchResults[Index].IsValid()) return;

	if (SessionPtr.IsValid())
	{
		FString UniqueGameSessionNameString;
		SessionSearchPtr->SearchResults[Index].Session.SessionSettings.Get(UniqueGameSessionName, UniqueGameSessionNameString);
		GameSessionName = *UniqueGameSessionNameString;

		FString CheckPasswordString;
		SessionSearchPtr->SearchResults[Index].Session.SessionSettings.Get(CheckPasswordName, CheckPasswordString);
		if (CheckPasswordString == TEXT("true"))
		{
			FString InputPassword;
			SessionSearchPtr->SearchResults[Index].Session.SessionSettings.Get(PasswordName, InputPassword);

			if (InputPassword == InPassword)
			{
				SessionPtr->JoinSession(0, GameSessionName, SessionSearchPtr->SearchResults[Index]);
			}
		}
		else
		{
			SessionPtr->JoinSession(0, GameSessionName, SessionSearchPtr->SearchResults[Index]);
		}
	}
}

void UFGGameInstance::OnJoinSessionCompleted(FName InSessionName, EOnJoinSessionCompleteResult::Type Result)
{
	if (Result == EOnJoinSessionCompleteResult::Success)
	{
		if (SessionPtr.IsValid())
		{
			FString JoinAddress;
			SessionPtr->GetResolvedConnectString(InSessionName, JoinAddress);

			AFGEOSPlayerController* FGEOSPlayerController = Cast<AFGEOSPlayerController>(GetFirstLocalPlayerController());
			if (FGEOSPlayerController)
			{
				FGEOSPlayerController->ClientTravel(JoinAddress, ETravelType::TRAVEL_Absolute);
			}
		}
	}
	else
	{
		//세션 유효하지 않으면 지우기
		OnUpdateSessionList.Broadcast(SelectedLobbyIndex);

		UE_LOG(LogTemp, Error, TEXT("JoinSessionCompleted fail - Reason: %d"), (int32)Result);
	}
}














//세션 파괴(나가기)
void UFGGameInstance::DestroySessionFunction()
{
	//세션 파괴 후 검색 되지 않게 채널 0으로 수정
	AFGLobbyPlayerController* FGEOSPlayerController = Cast<AFGLobbyPlayerController>(GetFirstLocalPlayerController());
	if (IsValid(FGEOSPlayerController))
	{
		if (FGEOSPlayerController->HasAuthority())
		{
			FString DestroyChannelNumber = TEXT("0");
			SessionSettings->Set(ChannelName, DestroyChannelNumber, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
			SessionPtr->UpdateSession(GameSessionName, *SessionSettings, true);
		}
	}

	AFGPlayerController* FGPlayerController = Cast<AFGPlayerController>(GetFirstLocalPlayerController());
	if (IsValid(FGPlayerController))
	{
		if (FGPlayerController->HasAuthority())
		{
			FString DestroyChannelNumber = TEXT("0");
			SessionSettings->Set(ChannelName, DestroyChannelNumber, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
			SessionPtr->UpdateSession(GameSessionName, *SessionSettings, true);
		}
	}

	if (SessionPtr.IsValid())
	{
		SessionPtr->DestroySession(GameSessionName);
	}
}

void UFGGameInstance::OnDestroySessionCompleted(FName InSessionName, bool bSuccessful)
{
	if (bSuccessful)
	{
		UGameplayStatics::OpenLevel(this, FName("L_FGMainMenu"));

		UE_LOG(LogTemp, Warning, TEXT("%s Session Destroyed"), *InSessionName.ToString());
	}
}

//로그인 상태면 로그인 후 상태 만들기
void UFGGameInstance::OnPostLoadMap(UWorld* InWorld)
{
	if (bLoginCompleted)
	{
		OnOnlineStatusState.Broadcast(true);
	}
}


//강퇴
void UFGGameInstance::RemovePlayerFromSessionFunction(class AFGPlayerState* InFGPlayerState)
{
	if (!IsValid(InFGPlayerState) || !InFGPlayerState->GetUniqueId().IsValid()) return;

	//SessionPtr->RemovePlayerFromSession(0, GameSessionName, *InFGPlayerState->GetUniqueId().GetUniqueNetId().ToSharedRef());
	//재참여 불가

	AFGLobbyPlayerController* FGLobbyPlayerController = Cast<AFGLobbyPlayerController>(InFGPlayerState->GetOwner());
	if (IsValid(FGLobbyPlayerController))
	{
		FGLobbyPlayerController->KickPlayer(GameSessionName);

		FString FGMainMenuAddress = TEXT("/Game/Maps/Main_Menu/L_FGMainMenu.L_FGMainMenu");
		FGLobbyPlayerController->ClientTravel(FGMainMenuAddress, ETravelType::TRAVEL_Absolute);
	}
}




void UFGGameInstance::StartGameLevel()
{
	AFGLobbyGameState* GFGLobbyGameState = GetWorld()->GetGameState<AFGLobbyGameState>();
	if (!IsValid(GFGLobbyGameState)) return;
	
	AllPlayerNum = GFGLobbyGameState->PlayerArray.Num();

	for (APlayerState* EPlayerState : GFGLobbyGameState->PlayerArray)
	{
		if (!IsValid(EPlayerState)) continue;

		AFGPlayerState* CFGPlayerState = Cast<AFGPlayerState>(EPlayerState);
		if (!IsValid(CFGPlayerState)) continue;

		FString Name = CFGPlayerState->GetName();
		FString Left, Right;
		if (!Name.Split(TEXT("_"), &Left, &Right, ESearchCase::IgnoreCase, ESearchDir::FromEnd)) continue;
		//ESearchCase::IgnoreCase -> 문자열 비교·검색 시 대소문자를 무시
		//ESearchCase::CaseSensitive -> 대소문자를 구분
		//ESearchDir::FromEnd -> 특정 값을 뒤에서부터 찾거나 분리하는 기준
		//ESearchDir::FromStart -> 특정 값을 앞에서부터 찾거나 분리하는 기준

		int32 PlayerNumber = FCString::Atoi(*Right);
		//FCString : 저수준 문자열 처리 클래스
		//Atoi : 문자열을 정수로 변환

		//FUniqueNetIdRepl UniqueId = FGPlayerState->GetUniqueId();
		//if (UniqueId.IsValid())
		//{
		//	FString PlayerIdStr = UniqueId->ToString(); // 네트워크 고유 ID
		//	UE_LOG(LogTemp, Log, TEXT("PlayerIdStr %s"), *PlayerIdStr);
		//}

		if (CFGPlayerState->GetTeamIndex() == 0)
		{
			FSaveSelectedCharacterStruct SaveSelectedCharacterStruct;
			SaveSelectedCharacterStruct.TeamType = 0;

			for (const auto& SelectedCharacterStructElement : CFGPlayerState->GetSelectedCharacterStructArray())
			{
				SaveSelectedCharacterStruct.SaveSelectedCharacterImageArray.Add(SelectedCharacterStructElement.SelectedCharacterTexture2D);
				SaveSelectedCharacterStruct.SaveSelectedCharacterClassArray.Add(SelectedCharacterStructElement.SelectedCharacterClass);
			}

			SaveSelectedCharacterStructMap.Add(PlayerNumber, SaveSelectedCharacterStruct);
		}
		else if (CFGPlayerState->GetTeamIndex() == 1)
		{
			FSaveSelectedCharacterStruct SaveSelectedCharacterStruct;
			SaveSelectedCharacterStruct.TeamType = 1;

			for (const auto& SelectedCharacterStructElement : CFGPlayerState->GetSelectedCharacterStructArray())
			{
				SaveSelectedCharacterStruct.SaveSelectedCharacterImageArray.Add(SelectedCharacterStructElement.SelectedCharacterTexture2D);
				SaveSelectedCharacterStruct.SaveSelectedCharacterClassArray.Add(SelectedCharacterStructElement.SelectedCharacterClass);
			}

			SaveSelectedCharacterStructMap.Add(PlayerNumber, SaveSelectedCharacterStruct);
		}
	}

	LoadLevelAndListen(GameLevel);
}

const FSaveSelectedCharacterStruct* UFGGameInstance::FindSaveSelectedCharacterStructMap(int32 InPlayerNumber) const
{
	if (const FSaveSelectedCharacterStruct* FindSaveSelectedCharacterStruct = SaveSelectedCharacterStructMap.Find(InPlayerNumber))
	{
		return FindSaveSelectedCharacterStruct;
	}

	return nullptr;
}
