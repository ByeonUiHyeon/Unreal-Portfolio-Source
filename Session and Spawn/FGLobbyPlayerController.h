// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "FGLobbyPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class FIGHTGAME_API AFGLobbyPlayerController : public APlayerController
{
	GENERATED_BODY()
	
private:
	UPROPERTY(EditDefaultsOnly, Category = "EOS_UI")
	TSubclassOf<class UFGLobbyWidget> FGLobbyWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "EOS_UI")
	TSubclassOf<class UFGSelectCharacter> FGSelectCharacterClass;

	UPROPERTY(EditDefaultsOnly, Category = "EOS_UI")
	TSubclassOf<class UFGLobbyRightClickMenu> FGLobbyRightClickMenuClass;

	UPROPERTY(EditDefaultsOnly, Category = "EOS_Component")
	TObjectPtr<class UFGChatComponent> VFGChatComponent;

public:
	UPROPERTY()
	TArray<TObjectPtr<UFGLobbyRightClickMenu>> FGLobbyRightClickMenuArray;

public:
	FORCEINLINE TSubclassOf<UFGSelectCharacter> GetFGSelectCharacterClass() const { return FGSelectCharacterClass; }
	FORCEINLINE TSubclassOf<UFGLobbyRightClickMenu> GetFGLobbyRightClickMenuClass() const { return FGLobbyRightClickMenuClass; }
	FORCEINLINE UFGChatComponent* GetFGChatComponent() const { return VFGChatComponent; }

public:
	AFGLobbyPlayerController();

protected:
	virtual void BeginPlay() override;

public:
	void KickPlayer(const FName& InSessionName);

protected:
	UFUNCTION(Client, Reliable)
	void Client_KickPlayer(const FName& InSessionName);
};
