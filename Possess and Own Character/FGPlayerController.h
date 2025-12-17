// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Interface/FGRTSInterface.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "InputActionValue.h"
#include "EnhancedInputComponent.h"
#include "Data/FGGameDatas.h"
#include "FGPlayerController.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FOnUpdateKeySetting, ECharacterControlType);

USTRUCT(BlueprintType)
struct FSelectedCharacterPlayerStruct
{
	GENERATED_BODY()

	UPROPERTY()
	int32 SelectedCharacterNumber;

	UPROPERTY()
	TObjectPtr<class AFGCharacterPlayer> SelectedCharacter;

	FSelectedCharacterPlayerStruct() = default;

	FSelectedCharacterPlayerStruct
	(
		int32 InSelectedCharacterNumber, 
		AFGCharacterPlayer* InSelectedCharacter
	) :
		SelectedCharacterNumber(InSelectedCharacterNumber),
		SelectedCharacter(InSelectedCharacter)
	{
	}
};

/**
 * 
 */
UCLASS()
class FIGHTGAME_API AFGPlayerController : public APlayerController
{
	GENERATED_BODY()
	
protected:
	UPROPERTY()
	TObjectPtr<class UFGGameInstance> VFGGameInstance;

	UPROPERTY(EditDefaultsOnly, Category = "GAS_Input")
	TMap<ECharacterControlType, TObjectPtr<class UInputMappingContext>> ControlChangeMap;

	UPROPERTY(EditDefaultsOnly, Category = "GAS_Input")
	TMap<FName, TObjectPtr<UInputAction>> InputActionMap;

	UPROPERTY(EditDefaultsOnly, Category = "GAS_Input")
	TArray<TObjectPtr<UInputAction>> InventoryInputActionArray;

	UPROPERTY(EditDefaultsOnly, Category = "GAS")
	FGameplayTag MoveOwnedTag;

	UPROPERTY(EditDefaultsOnly, Category = "GAS")
	FGameplayTagContainer MoveBlockedTags;

	UPROPERTY()
	TObjectPtr<class APawn> RTSPawn;

	UPROPERTY(Replicated)
	TObjectPtr<class AFGCharacterPlayer> VFGCharacterPlayer;

	UPROPERTY()
	TObjectPtr<class UEnhancedInputComponent> VEnhancedInputComponent;

	UPROPERTY(Replicated)
	TObjectPtr<class UFGAbilitySystemComponent> VFGAbilitySystemComponent;

	UPROPERTY(EditDefaultsOnly, Category = "GAS")
	FGameplayTag LeftMousePressedOwnedTag;

	UPROPERTY(EditDefaultsOnly, Category = "GAS")
	FGameplayTagContainer LeftMousePressedBlockedTags;

	UPROPERTY(EditDefaultsOnly, Category = "GAS_AI")
	TObjectPtr<class UBehaviorTree> BTATtack;

	UPROPERTY(EditDefaultsOnly, Category = "GAS_AI")
	TObjectPtr<class UBehaviorTree> BTHold;

	UPROPERTY(EditDefaultsOnly, Category = "GAS_AI")
	TObjectPtr<class UBehaviorTree> BTPatrol;

	UPROPERTY(Replicated)
	bool bCharacterMode;

	UPROPERTY(EditDefaultsOnly, Category = "GAS")
	FGameplayTag ChangePossessOwnedTag;

	UPROPERTY(EditDefaultsOnly, Category = "GAS")
	FGameplayTagContainer ChangePossessBlockedTags;

	UPROPERTY(EditDefaultsOnly, Category = "GAS")
	FGameplayTagContainer RTSAttackBlockedTags;

	UPROPERTY()
	TObjectPtr<class UFGCommonHUD> VFGCommonHUD;

	UPROPERTY(Replicated)
	TObjectPtr<class UFGSelectedCharacterHpMp> VFGSelectedCharacterHpMp;

	UPROPERTY()
	TObjectPtr<class AFGPlayerState> VFGPlayerState;

	UPROPERTY(EditDefaultsOnly, Category = "GAS_Change")
	float FGRTSCameraBlendChnageTime = 0.2f;

	UPROPERTY(Replicated)
	TArray<FSelectedCharacterPlayerStruct> SelectedCharacterPlayerStructArray;

	UPROPERTY()
	TObjectPtr<class AFGHUD> VFGHUD;

	UPROPERTY(EditDefaultsOnly, Category = "GAS_UI")
	TMap<FName, TSubclassOf<UUserWidget>> UserWidgetClassMap;

	UPROPERTY()
	TObjectPtr<class UFGSettings> VFGSettings;
	
	UPROPERTY(EditDefaultsOnly, Category = "GAS_Component")
	TObjectPtr<class UFGChatComponent> VFGChatComponent;

	UPROPERTY()
	TObjectPtr<class UEnhancedInputUserSettings> VEnhancedInputUserSettings;
	
	UPROPERTY()
	TObjectPtr<class AGameplayAbilityTargetActor> VGameplayAbilityTargetActor;

	UPROPERTY()
	TMap<int32, bool> HaveCompletedEventMap;

	UPROPERTY()
	TObjectPtr<class UFGShopHUD> VFGShopHUD;

	UPROPERTY()
	TObjectPtr<class UTextureRenderTarget2D> VTextureRenderTarget2D;

	UPROPERTY(Replicated)
	int32 OwnPlayerNumber;

	UPROPERTY(Replicated)
	int32 TeamIndex = -1;

	UPROPERTY()
	TObjectPtr<class AFGGameState> VFGGameState;

	UPROPERTY(EditDefaultsOnly, Category = "GAS_UI")
	TSubclassOf<class AFGClickMarker> ClickMarkerClass;

	UPROPERTY()
	TObjectPtr<AActor> HoveredActor;

	UPROPERTY(EditDefaultsOnly, Category = "GAS_UI")
	TObjectPtr<class UMaterialInterface> OulineMaterial;

	FOnUpdateKeySetting OnUpdateKeySetting;

	TArray<uint32> BindingHandles;

	IFGRTSInterface* VFGRTSInterface;

	FTimerHandle InFGRTSCameraBlendTimer;
	FTimerHandle InFGCharacterPlayerBlendTimer;
	FTimerHandle RTSAttackTimer;
	FTimerHandle AppendTriangulatedPolygonTimer;
	FTimerHandle HaveCompletedEventTimer;
	FTimerHandle SetMouseCursorPositionTimer;
	FTimerHandle ChangePossessStateTimer;
	FTimerHandle SelectCharacter1TimerHandle;
	FTimerHandle MouseHoverTimerHandle;

	int32 SelectCharacterCount1 = 0;
	int32 SelectCharacterCount2 = 0;
	int32 SelectCharacterCount3 = 0;

	uint32 bLeftMousePressed : 1;
	uint32 bRightMousePressed : 1;
	uint32 bLookAt : 1;
	uint32 bCreateFGSettingsWidget : 1;
	uint32 bShopClicked : 1;
	uint32 bHover : 1;

public:
	FORCEINLINE const TMap<ECharacterControlType, TObjectPtr<UInputMappingContext>>& GetControlChangeMap() const { return ControlChangeMap; }
	FORCEINLINE const bool GetbCharacterMode() const { return bCharacterMode; }
	FORCEINLINE UBehaviorTree* GetBTATtack() const { return BTATtack; }
	FORCEINLINE TArray<FSelectedCharacterPlayerStruct>& GetSelectedCharacterPlayerStructArray() { return SelectedCharacterPlayerStructArray; };
	FORCEINLINE UFGSelectedCharacterHpMp* GetFGSelectedCharacterHpMp() const { return VFGSelectedCharacterHpMp; }
	FORCEINLINE AFGCharacterPlayer* GetFGCharacterPlayer() const { return VFGCharacterPlayer; }
	FORCEINLINE const TArray<TObjectPtr<UInputAction>>& GetInventoryInputActionArray() const { return InventoryInputActionArray; };
	FORCEINLINE UFGChatComponent* GetFGChatComponent() const { return VFGChatComponent; }
	FORCEINLINE FOnUpdateKeySetting& GetOnUpdateKeySetting() { return OnUpdateKeySetting; }
	FORCEINLINE int32 GetOwnPlayerNumber() const { return OwnPlayerNumber; }
	FORCEINLINE void SetOwnPlayerNumber(int32 InOwnPlayerNumber) { OwnPlayerNumber = InOwnPlayerNumber; }
	FORCEINLINE int32 GetTeamIndex() const { return TeamIndex; }
	FORCEINLINE void SetTeamIndex(int32 InTeamIndex) { TeamIndex = InTeamIndex; }

public:
	AFGPlayerController();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void SetFGCharacterPlayer(AFGCharacterPlayer* InFGCharacterPlayer);
	void SetFGCharacterPlayerFunction(AFGCharacterPlayer* InFGCharacterPlayer);

	UFUNCTION(Server, Reliable)
	void Server_SetFGCharacterPlayer(AFGCharacterPlayer* InFGCharacterPlayer);

protected:
	virtual void BeginPlay() override;

	virtual void OnRep_PlayerState() override;

	void LoadFunction();

	virtual void SetupInputComponent() override;

public:
	void AddAbilityInputComponent(const TMap<struct FGameplayTag, struct FAbilityInfo>& InSetInputAbilities);

protected:
	void ControllerInputPressed(int32 InputId);
	void ControllerInputPressedFunction(int32 InputId);

	UFUNCTION(Server, Reliable)
	void Server_ControllerInputPressed(int32 InputId);

	void ControllerInputReleased(int32 InputId);
	void ControllerInputReleasedFunction(int32 InputId);

	UFUNCTION(Server, Reliable)
	void Server_ControllerInputReleased(int32 InputId);

	void SetMouseCursorPosition(AFGCharacterPlayer* InFGCharacterPlayer);
	void SetMouseCursorPositionFunction(AFGCharacterPlayer* InFGCharacterPlayer, FVector InVector);

	UFUNCTION(Server, Reliable)
	void Server_SetMouseCursorPosition(AFGCharacterPlayer* InFGCharacterPlayer, FVector InVector);

	virtual void Tick(float DeltaTime) override;

	void LeftMousePressed();

	void ConfirmTargetingFunction();

	UFUNCTION(Server, Reliable)
	void Server_ConfirmTargetingFunction();

	void LeftMouseReleased();

	void RightMousePressed();

	void CancelTargetingFunctionBundle();

	void CancelTargetingFunction();

	UFUNCTION(Server, Reliable)
	void Server_CancelTargetingFunction();

public:
	void RightMousePressedContents(const FVector& InMouseLocation);

protected:
	void RightMousePressedFunction(AFGCharacterPlayer* InFGCharacterPlayer, FVector InLocation);

	UFUNCTION(Server, Reliable)
	void Server_RightMousePressed(AFGCharacterPlayer* InFGCharacterPlayer, FVector InLocation);

	void SpawnClickMarker(const FVector& InMouseLocation, const FLinearColor& InLinearColor);

	void RightMouseReleased();

	UFUNCTION(Server, Reliable)
	void Server_RightMouseReleased();

public:
	void ChangePossess();

protected:
	void ChangePossessFunction(AFGCharacterPlayer* InFGCharacterPlayer, APawn* InFGRTSCamera);

	UFUNCTION(Server, Reliable)
	void Server_ChangePossess(AFGCharacterPlayer* InFGCharacterPlayer, APawn* InFGRTSCamera);

	UFUNCTION(Client, Reliable)
	void Client_RemoveConsoleCommand(AFGCharacterPlayer* InFGCharacterPlayer);

	UFUNCTION(Client, Reliable)
	void Client_CreateConsoleCommand(AFGCharacterPlayer* InFGCharacterPlayer);

	void RTSCameraPossess(AFGCharacterPlayer* InFGCharacterPlayer, APawn* InFGRTSCamera);

	void RTSAttack();

public:
	void RTSAttackContents(const FVector& InMouseLocation);

protected:
	void RTSAttackFunction(AFGCharacterPlayer* InFGCharacterPlayer, FVector InLocation);

	UFUNCTION(Server, Reliable)
	void Server_RTSAttack(AFGCharacterPlayer* InFGCharacterPlayer, FVector InLocation);

	void RTSStop();
	void RTSStopFunction(AFGCharacterPlayer* InFGCharacterPlayer);

	UFUNCTION(Server, Reliable)
	void Server_RTSStop(AFGCharacterPlayer* InFGCharacterPlayer);

	void RTSHold();
	void RTSHoldFunction(AFGCharacterPlayer* InFGCharacterPlayer);

	UFUNCTION(Server, Reliable)
	void Server_RTSHold(AFGCharacterPlayer* InFGCharacterPlayer);

	void RTSPatrol();

public:
	void RTSPatrolContents(const FVector& InMouseLocation);

protected:
	void RTSPatrolFunction(AFGCharacterPlayer* InFGCharacterPlayer, const FVector& InMouseLocation);

	UFUNCTION(Server, Reliable)
	void Server_RTSPatrol(AFGCharacterPlayer* InFGCharacterPlayer, const FVector& InMouseLocation);

	void FocusCharacter();

public:
	void RTSPawnPossessDueToDead();

protected:
	UFUNCTION(Server, Reliable)
	void Server_RTSCameraPossess(AFGCharacterPlayer* InFGCharacterPlayer, APawn* InFGRTSCamera);

public:
	void LookAtAllFGChildWidgetComponent(bool InbLookAt);

	void LookAtAllFGChildWidgetComponentFunction(bool InbLookAt);

protected:
	UFUNCTION(Server, Reliable)
	void Server_LookAtAllFGChildWidgetComponent(bool InbLookAt);

	UFUNCTION(Client, Reliable)
	void Client_LookAtAllFGChildWidgetComponent(bool InbLookAt);

public:
	void DeadLookAtAllFGChildWidgetComponent();

protected:
	void SelectCharacter1();

public:
	UFUNCTION(Client, Reliable)
	void Client_SelectCharacter1();

protected:
	void SelectCharacter2();
	void SelectCharacter3();
	void SelectAllCharacter();

	void CreatePlayerWidgets();

public:
	void StartCharacterRespawnCoolTimeSetting(const int32& InSelectCharacterNumber, const float& InCharacterRespawnTime);

protected:
	UFUNCTION(Client, Reliable)
	void Client_StartCharacterRespawnCoolTimeSetting(const int32& InSelectCharacterNumber, const float& InCharacterRespawnTime);

public:
	void BuyItem(class AFGShop* InFGShop, AFGCharacterPlayer* InFGCharacterPlayer, int32 InSlotIndex, int32 InDropIndex = -1, int32 InItemQuantity = 1);

protected:
	UFUNCTION(Server, Reliable)
	void Server_BuyItem(AFGShop* InFGShop, AFGCharacterPlayer* InFGCharacterPlayer, int32 InSlotIndex, int32 InDropIndex = -1, int32 InItemQuantity = 1);

public:
	void SellItem(AFGShop* InFGShop, AFGCharacterPlayer* InFGCharacterPlayer, int32 InSlotIndex, int32 InItemQuantity = 1);

protected:
	UFUNCTION(Server, Reliable)
	void Server_SellItem(AFGShop* InFGShop, AFGCharacterPlayer* InFGCharacterPlayer, int32 InSlotIndex, int32 InItemQuantity = 1);

	void OnUseInventoryItemAction(const FInputActionInstance& Instance);

public:
	void UseInventoryItem(AFGCharacterPlayer* InFGCharacterPlayer, int32 InSlotIndex);

protected:
	UFUNCTION(Server, Reliable)
	void Server_UseInventoryItem(AFGCharacterPlayer* InFGCharacterPlayer, int32 InSlotIndex);

public:
	UEnhancedInputUserSettings* GetEnhancedInputUserSettings() const;

protected:
	void RegisterInputMappingContextSetting();

public:
	void CreateFGSettingsWidget();

public:
	void InstantCastSetting(UFGAbilitySystemComponent* InFGAbilitySystemComponent, TSubclassOf<UGameplayAbility> InGameplayAbilityClass, bool InbInstantCast);

protected:
	void InstantCastSettingFunction(UFGAbilitySystemComponent* InFGAbilitySystemComponent, TSubclassOf<UGameplayAbility> InGameplayAbilityClass, bool InbInstantCast);

	UFUNCTION(Server, Reliable)
	void Server_InstantCastSetting(UFGAbilitySystemComponent* InFGAbilitySystemComponent, TSubclassOf<UGameplayAbility> InGameplayAbilityClass, bool InbInstantCast);

	void IncreaseSensitivity();
	void DecreaseSensitivity();

public:
	UFUNCTION(Client, Reliable)
	void Client_TeamScoreWidgetFunction(float InTeamAScore, float InTeamBScore, int32 InTeamIndex, UTexture2D* InKillerImage, UTexture2D* InTargetImage, const FString& InKillerNickname, const FString& InTargetNickname);

	TSubclassOf<UUserWidget> GetUserWidgetClassByName(const FName& InName) const;
	UInputAction* GetInputActionByName(const FName& InName) const;

	void RenderTargetSetting();

	UFUNCTION(Client, Reliable)
	void Client_UpdateVision(const TArray<uint8>& InFogOfWar, const TArray<AActor*>& InOtherActorArray, float InGridSize, int32 InColumns);

protected:
	UFUNCTION(Server, Reliable)
	void Server_NotifyLoadingComplete();

public:
	void InputModeGameAndUIFunction();

protected:
	void MouseHover();
 };
