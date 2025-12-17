// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/FGPlayerState.h"
#include "Net/UnrealNetwork.h"
#include "Player/FGPlayerController.h"

AFGPlayerState::AFGPlayerState()
{
	SetReplicates(true);
}

void AFGPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AFGPlayerState, OwnPlayerNumber);
	DOREPLIFETIME(AFGPlayerState, TeamIndex);
	DOREPLIFETIME(AFGPlayerState, SelectedCharacterStructArray);
}

void AFGPlayerState::SetTeamIndex(const int32& InTeamIndex)
{
	if (HasAuthority())
	{
		TeamIndex = InTeamIndex;
	}
	else
	{
		Server_SetTeamIndex(InTeamIndex);
	}
}

void AFGPlayerState::Server_SetTeamIndex_Implementation(const int32& InTeamIndex)
{
	TeamIndex = InTeamIndex;
}

void AFGPlayerState::UpdateSelectedCharacter()
{
	if (HasAuthority())
	{
		NetMulticast_UpdateSelectedCharacter();
	}
	else
	{
		Server_UpdateSelectedCharacter();
	}
}

void AFGPlayerState::Server_UpdateSelectedCharacter_Implementation()
{
	NetMulticast_UpdateSelectedCharacter();
}

void AFGPlayerState::NetMulticast_UpdateSelectedCharacter_Implementation()
{
	OnSelectCharacterClicked.Broadcast();
}

void AFGPlayerState::SetSelectedCharacterStructArray(FSelectedCharacterData InFSelectedCharacterData)
{
	if (HasAuthority())
	{
		NetMulticast_SetSelectedCharacterStructArray(InFSelectedCharacterData);
	}
	else
	{
		Server_SetSelectedCharacterStructArray(InFSelectedCharacterData);
	}
}

void AFGPlayerState::Server_SetSelectedCharacterStructArray_Implementation(FSelectedCharacterData InFSelectedCharacterData)
{
	NetMulticast_SetSelectedCharacterStructArray(InFSelectedCharacterData);
}

void AFGPlayerState::NetMulticast_SetSelectedCharacterStructArray_Implementation(FSelectedCharacterData InFSelectedCharacterData)
{
	SelectedCharacterStructArray.Add(InFSelectedCharacterData);
}

void AFGPlayerState::RemoveAtSelectedCharacterStructArray(int32 InIndex)
{
	if (HasAuthority())
	{
		NetMulticast_RemoveAtSelectedCharacterStructArray(InIndex);
	}
	else
	{
		Server_RemoveAtSelectedCharacterStructArray(InIndex);
	}
}

void AFGPlayerState::Server_RemoveAtSelectedCharacterStructArray_Implementation(int32 InIndex)
{
	NetMulticast_RemoveAtSelectedCharacterStructArray(InIndex);
}

void AFGPlayerState::NetMulticast_RemoveAtSelectedCharacterStructArray_Implementation(int32 InIndex)
{
	SelectedCharacterStructArray.RemoveAt(InIndex);
}
