// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FGSelectCharacter.generated.h"


UENUM(BlueprintType)
enum class EFGSelectCharacterParentType : uint8
{
	FGLobbyWidgetType = 0,
	FGLobbyPlayerEntryType
};

/**
 * 
 */
UCLASS()
class FIGHTGAME_API UFGSelectCharacter : public UUserWidget
{
	GENERATED_BODY()
	
private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UButton> SelectCharacterButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UImage> CharacterImage;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UBorder> EmptyCharacterBorder;

	UPROPERTY()
	TObjectPtr<class UTexture2D> CharacterTexture2D;

	UPROPERTY()
	TSubclassOf<class AFGCharacterPlayer> FGCharacterPlayerClass;


	EFGSelectCharacterParentType FGSelectCharacterParentType;

	int32 Index;

	uint32 bClickable : 1;

public:
	FORCEINLINE void SetFGSelectCharacterParentType(EFGSelectCharacterParentType InEFGSelectCharacterParentType) { FGSelectCharacterParentType = InEFGSelectCharacterParentType; }
	FORCEINLINE void SetCharacterTexture2D(UTexture2D* InTexture2D) { CharacterTexture2D = InTexture2D; }
	FORCEINLINE void SetFGCharacterPlayerClass(TSubclassOf<AFGCharacterPlayer> InFGCharacterPlayer) { FGCharacterPlayerClass = InFGCharacterPlayer; }
	FORCEINLINE void SetIndex(const int32& InIndex) { Index = InIndex; }
	FORCEINLINE void SetbClickable(const bool& InbClickable) { bClickable = InbClickable; }

public:
	UFGSelectCharacter(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void NativeConstruct() override;

	UFUNCTION()
	void OnSelectCharacterButtonClicked();
};
