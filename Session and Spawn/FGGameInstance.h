// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "OnlineSessionClient.h"
#include "FGGameInstance.generated.h"

class UTexture2D;
class AFGCharacterPlayer;
class AFGPlayerState;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnSessionSearchCompleted, const TArray<FOnlineSessionSearchResult>& /*SearchResults*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnOnlineStatusState, bool /*bOnline*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnUpdateSessionList, const int32& /*InLobbyIndex*/);

USTRUCT(BlueprintType)
struct FSaveSelectedCharacterStruct
{
	GENERATED_BODY()

	UPROPERTY()
	int32 TeamType = 0;

	UPROPERTY()
	TArray<TObjectPtr<UTexture2D>> SaveSelectedCharacterImageArray;

	UPROPERTY()
	TArray<TSubclassOf<AFGCharacterPlayer>> SaveSelectedCharacterClassArray;

	FSaveSelectedCharacterStruct() = default;

	FSaveSelectedCharacterStruct
	(
		int32 InTeamType,
		TArray<TObjectPtr<UTexture2D>> InSaveSelectedCharacterImageArray,
		TArray<TSubclassOf<AFGCharacterPlayer>> InSaveSelectedCharacterClassArray
	) :
		TeamType(InTeamType),
		SaveSelectedCharacterImageArray(InSaveSelectedCharacterImageArray),
		SaveSelectedCharacterClassArray(InSaveSelectedCharacterClassArray)
	{
	}
};

/**
 * 
 */
UCLASS()
class FIGHTGAME_API UFGGameInstance : public UGameInstance
{
	GENERATED_BODY()
	
private:
	UPROPERTY(EditDefaultsOnly)
	TSoftObjectPtr<UWorld> GameLevel;
	
	UPROPERTY(EditDefaultsOnly)
	TSoftObjectPtr<UWorld> LobbyLevel;

	UPROPERTY()
	TMap<int32, FSaveSelectedCharacterStruct> SaveSelectedCharacterStructMap;

	UPROPERTY(EditDefaultsOnly, Category = "GAS_Inventory")
	TObjectPtr<class UDataTable> LinkItemDataTable;

	TSharedPtr<class IOnlineIdentity, ESPMode::ThreadSafe> IdentityPtr;
	TSharedPtr<class IOnlinePresence, ESPMode::ThreadSafe> PresencePtr;
	TSharedPtr<class IOnlineSession, ESPMode::ThreadSafe> SessionPtr;
	TSharedPtr<FOnlineSessionSettings> SessionSettings;
	TSharedPtr<class FOnlineSessionSearch> SessionSearchPtr;

	FOnSessionSearchCompleted OnSessionSearchCompleted;
	FOnOnlineStatusState OnOnlineStatusState;
	FOnUpdateSessionList OnUpdateSessionList;

	const FName ChannelName = TEXT("Channel");
	const FName RoomName = TEXT("RoomName");
	const FName UniqueGameSessionName = TEXT("UniqueGameSessionName");
	const FName CheckPasswordName = TEXT("CheckPassword");
	const FName PasswordName = TEXT("Password");

	FName GameSessionName = TEXT("GameSession");

	FString RoomNameString;
	FString ChannelNumber = TEXT("1");

	int32 SelectedLobbyIndex;
	int32 AllPlayerNum;

	uint32 bLoginCompleted : 1;

public:
	FORCEINLINE const FString& GetRoomNameString() const { return RoomNameString; }
	FORCEINLINE const FName& GetRoomName() const { return RoomName; }
	FORCEINLINE const FName& GetGameSessionName() const { return GameSessionName; }
	FORCEINLINE const FName& GetCheckPasswordName() const { return CheckPasswordName; }
	FORCEINLINE void SetChannelNumber(const FString& InChannelNumber) { ChannelNumber = InChannelNumber; }
	FORCEINLINE FOnSessionSearchCompleted& GetOnSessionSearchCompleted() { return OnSessionSearchCompleted; }
	FORCEINLINE FOnOnlineStatusState& GetOnOnlineStatusState() { return OnOnlineStatusState; }
	FORCEINLINE FOnUpdateSessionList& GetOnUpdateSessionList() { return OnUpdateSessionList; }
	FORCEINLINE UDataTable* GetLinkItemDataTable() const { return LinkItemDataTable; }
	FORCEINLINE void SetSelectedLobbyIndex(const int32& InSelectedLobbyIndex) { SelectedLobbyIndex = InSelectedLobbyIndex; }
	FORCEINLINE int32 GetAllPlayerNum() const { return AllPlayerNum; }

public:
	virtual void Init() override;

protected:
	void OnPresenceCompleted(const FUniqueNetId& UserId, const TSharedRef<class FOnlineUserPresence>& Presence);

public:
	void LoginFunction();
	void DeveloperLoginFunction(const FString& InDevId, const FString& InDevToken);

protected:
	void OnLoginCompleted(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& UserId, const FString& Error);

public:
	void LogoutFunction();

protected:
	void OnLogoutCompleted(int32 LocalUserNum, bool bWasSuccessful);

public:
	FString GetPlayerNicknameFunction();
	FString GetUniquePlayerIdFunction();

public:
	void CreateSessionFunction(const FString& InRoomName, bool bIsChecked, const FString& InPassword);

protected:
	void OnCreateSessionCompleted(FName InName, bool bSuccessful);

	void LoadLevelAndListen(TSoftObjectPtr<UWorld> LevelToLoad);

public:
	void FindSessionsFunction();

protected:
	void OnFindSessionsCompleted(bool bSuccessful);

public:
	void JoinSessionFunctionIndex(const FString& InPassword, int32 Index);

protected:
	void OnJoinSessionCompleted(FName InSessionName, EOnJoinSessionCompleteResult::Type Result);

public:
	void DestroySessionFunction();

protected:
	void OnDestroySessionCompleted(FName InSessionName, bool bSuccessful);

	void OnPostLoadMap(UWorld* InWorld);

public:
	void RemovePlayerFromSessionFunction(AFGPlayerState* InFGPlayerState);

	void StartGameLevel();

	const FSaveSelectedCharacterStruct* FindSaveSelectedCharacterStructMap(int32 InPlayerNumber) const;
};
