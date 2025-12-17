// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Data/FGGameDatas.h"
#include "FGEOSPlayerController.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FOnEOSUpdateKeySetting, ECharacterControlType);

/**
 * 
 */
UCLASS()
class FIGHTGAME_API AFGEOSPlayerController : public APlayerController
{
	GENERATED_BODY()
	
private:
	UPROPERTY()
	TObjectPtr<class UFGGameInstance> VFGGameInstance;

	UPROPERTY(EditDefaultsOnly, Category = "EOS_Input")
	TMap<ECharacterControlType, TObjectPtr<class UInputMappingContext>> ControlChangeMap;

	UPROPERTY()
	TObjectPtr<class UFGSettings> VFGSettings;

	UPROPERTY(EditDefaultsOnly, Category = "EOS_UI")
	TMap<FName, TSubclassOf<UUserWidget>> UserWidgetClassMap;

	FOnEOSUpdateKeySetting OnEOSUpdateKeySetting;

	uint32 bCreateFGSettingsWidget : 1;

public:
	FORCEINLINE const TMap<ECharacterControlType, TObjectPtr<UInputMappingContext>>& GetControlChangeMap() const { return ControlChangeMap; }
	FORCEINLINE FOnEOSUpdateKeySetting& GetOnEOSUpdateKeySetting() { return OnEOSUpdateKeySetting; }

public:
	AFGEOSPlayerController();

protected:
	virtual void BeginPlay() override;

	void OnlineStatusState(bool bOnline);

public:
	virtual void OnNetCleanup(class UNetConnection* Connection) override;

	class UEnhancedInputUserSettings* GetEnhancedInputUserSettings() const;

protected:
	void RegisterInputMappingContextSetting();

public:
	void CreateFGSettingsWidget();

	TSubclassOf<UUserWidget> GetUserWidgetClassByName(const FName& InName) const;
};
