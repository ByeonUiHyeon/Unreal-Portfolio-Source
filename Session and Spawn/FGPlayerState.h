// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "FGPlayerState.generated.h"

class UTexture2D;
class AFGCharacterPlayer;

DECLARE_MULTICAST_DELEGATE(FOnSelectCharacterClicked);

USTRUCT(BlueprintType)
struct FSelectedCharacterData
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UTexture2D> SelectedCharacterTexture2D = nullptr;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<AFGCharacterPlayer> SelectedCharacterClass = nullptr;

	FSelectedCharacterData() = default;

	FSelectedCharacterData(UTexture2D* InSelectedCharacterTexture2D, TSubclassOf<AFGCharacterPlayer> InSelectedCharacterClass) :
		SelectedCharacterTexture2D(InSelectedCharacterTexture2D),
		SelectedCharacterClass(InSelectedCharacterClass)
	{
	}
};

/**
 * 
 */
UCLASS()
class FIGHTGAME_API AFGPlayerState : public APlayerState
{
	GENERATED_BODY()
	
private:
	UPROPERTY(Replicated)
	TArray<FSelectedCharacterData> SelectedCharacterStructArray;

	UPROPERTY(Replicated)
	int32 OwnPlayerNumber;

	UPROPERTY(Replicated)
	int32 TeamIndex = 0;

	FOnSelectCharacterClicked OnSelectCharacterClicked;

	FTimerHandle FGPlayerStateTimer;

public:
	FORCEINLINE TArray<FSelectedCharacterData>& GetSelectedCharacterStructArray() { return SelectedCharacterStructArray; }
	FORCEINLINE int32 GetTeamIndex() const { return TeamIndex; }
	FORCEINLINE FOnSelectCharacterClicked& GetOnSelectCharacterClicked() { return OnSelectCharacterClicked; }
	FORCEINLINE int32 GetOwnPlayerNumber() const { return OwnPlayerNumber; }
	FORCEINLINE void SetOwnPlayerNumber(const int32& InOwnPlayerNumber)  { OwnPlayerNumber = InOwnPlayerNumber; }

public:
	AFGPlayerState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void SetTeamIndex(const int32& InTeamIndex);

protected:
	UFUNCTION(Server, Reliable)
	void Server_SetTeamIndex(const int32& InTeamIndex);

public:
	void UpdateSelectedCharacter();

protected:
	UFUNCTION(Server, Reliable)
	void Server_UpdateSelectedCharacter();

	UFUNCTION(NetMulticast, Reliable)
	void NetMulticast_UpdateSelectedCharacter();

public:
	void SetSelectedCharacterStructArray(FSelectedCharacterData InFSelectedCharacterData);

protected:
	UFUNCTION(Server, Reliable)
	void Server_SetSelectedCharacterStructArray(FSelectedCharacterData InFSelectedCharacterData);

	UFUNCTION(NetMulticast, Reliable)
	void NetMulticast_SetSelectedCharacterStructArray(FSelectedCharacterData InFSelectedCharacterData);

public:
	void RemoveAtSelectedCharacterStructArray(int32 InIndex);

protected:
	UFUNCTION(Server, Reliable)
	void Server_RemoveAtSelectedCharacterStructArray(int32 InIndex);

	UFUNCTION(NetMulticast, Reliable)
	void NetMulticast_RemoveAtSelectedCharacterStructArray(int32 InIndex);
};
