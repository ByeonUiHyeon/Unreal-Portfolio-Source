// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Character/FGCharacterBase.h"
#include "InputActionValue.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "Components/WidgetComponent.h"
#include "EnhancedInputComponent.h"
#include "Interface/FGHUDInterface.h"
#include "Interface/FGRTSInterface.h"
#include "Interface/FGFogOfWallInterface.h"
#include "Interface/FGSelectCharacterInterface.h"
#include "Interface/FGMouseSensitivityInterface.h"
#include "Data/FGGameDatas.h"
#include "GeometryScript/MeshPrimitiveFunctions.h"
#include "FGCharacterPlayer.generated.h"

class UFGAbilitySystemComponent;
class UGameplayAbility;
class UInputAction;

DECLARE_MULTICAST_DELEGATE_FourParams(FOnSkillHotbarDelegate, int32, UFGAbilitySystemComponent*, TSubclassOf<UGameplayAbility>, const UInputAction*);
DECLARE_MULTICAST_DELEGATE_FourParams(FDurationTimeChangedDelegate, int32, FActiveGameplayEffectHandle, float, float);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnCollapsedDurationProgressBar, int32);
DECLARE_MULTICAST_DELEGATE_FourParams(FCooldownTimeChangedDelegate, int32, FActiveGameplayEffectHandle, float, float);

USTRUCT(BlueprintType)
struct FAbilityInfo
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UGameplayAbility> GameplayAbility = nullptr;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UInputAction> InputAction = nullptr;

	UPROPERTY(EditDefaultsOnly)
	TMap<ETriggerEvent, bool> TriggerEventAndInputPressedMap;

	FAbilityInfo() = default;
};

USTRUCT(BlueprintType)
struct FGameplayTagPair
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	FGameplayTag DefaultAbilityTag;

	UPROPERTY()
	bool bCooldown;

	UPROPERTY(EditDefaultsOnly)
	FGameplayTag DurationTag;

	UPROPERTY(EditDefaultsOnly)
	FGameplayTag CooldownTag;

	UPROPERTY()
	FTimerHandle TagDurationTimer;

	UPROPERTY()
	FTimerHandle TagCooldownTimer;

	FGameplayTagPair() = default;
};

UCLASS()
class AFGCharacterPlayer : public AFGCharacterBase, public IFGHUDInterface, public IFGSelectCharacterInterface, public IFGMouseSensitivityInterface
{
	GENERATED_BODY()

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UCameraComponent> FollowCamera;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS_Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> FirstMoveAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS_Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> FirstLookAction;

	UPROPERTY(EditDefaultsOnly, Category = "GAS_Tag")
	FGameplayTag MoveOwnedTag;

	UPROPERTY(EditDefaultsOnly, Category = "GAS_Tag")
	FGameplayTagContainer MoveBlockedTags;

	UPROPERTY()
	TArray<FGameplayTagPair> FGameplayTagPairs;

	UPROPERTY(EditDefaultsOnly, Category = "GAS_Hotbar")
	FGameplayTagPair Skill1;

	UPROPERTY(EditDefaultsOnly, Category = "GAS_Hotbar")
	FGameplayTagPair Skill2;

	UPROPERTY(EditDefaultsOnly, Category = "GAS_Hotbar")
	FGameplayTagPair Skill3;

	UPROPERTY(EditDefaultsOnly, Category = "GAS_Hotbar")
	FGameplayTagPair Skill4;

	UPROPERTY(EditDefaultsOnly, Category = "GAS_Important")
	TMap<FGameplayTag, FAbilityInfo> SetInputAbilities;

	UPROPERTY(EditDefaultsOnly, Category = "GAS_Debug")
	uint32 bGASDebug : 1;

	UPROPERTY(EditDefaultsOnly, Category = "GAS_UI")
	TSubclassOf<class UFGUserWidgetHUD> FGUserWidgetHUDClass;

	UPROPERTY()
	TObjectPtr<UFGUserWidgetHUD> VFGUserWidgetHUD;

	UPROPERTY(EditDefaultsOnly, Category = "GAS_UI")
	TSubclassOf<class UUserWidget> CrosshairClass;

	UPROPERTY()
	TObjectPtr<UUserWidget> Crosshair;

	UPROPERTY(Replicated)
	TObjectPtr<class AFGPlayerController> SpawnFGPlayerController;

	UPROPERTY(Replicated)
	TObjectPtr<class UTexture2D> CharacterImage;;

	UPROPERTY(Replicated)
	TSubclassOf<AFGCharacterPlayer> CharacterPlayerClass;

	UPROPERTY(Replicated)
	int32 OwnPlayerNumber;

	UPROPERTY(Replicated)
	int32 TeamIndex = -1;

	UPROPERTY(Replicated)
	int32 SelectedCharacterNumber;

	UPROPERTY()
	FTransform RespawnTransform;

	UPROPERTY()
	TObjectPtr<class UPaperSpriteComponent> TeamPaperSpriteComponent;

	UPROPERTY(EditDefaultsOnly, Category = "GAS_UI")
	TObjectPtr<class UPaperSprite> TeamAPaperSprite;

	UPROPERTY(EditDefaultsOnly, Category = "GAS_UI")
	TObjectPtr<UPaperSprite> TeamBPaperSprite;

	UPROPERTY(EditDefaultsOnly, Category = "GAS_Stat")
	float CharacterRespawnTime = 10.f;

	UPROPERTY(EditDefaultsOnly, Category = "GAS_Stat")
	float UnpossessVisionRadius = 1000.f;

	UPROPERTY(EditDefaultsOnly, Category = "GAS_Stat")
	float PossessVisionRadius = 2000.f;

	UPROPERTY(EditDefaultsOnly, Category = "GAS_Stat")
	float MouseSensitivity = 1.f;

	UPROPERTY(EditDefaultsOnly, Category = "GAS_Component")
	TObjectPtr<class UFGInventoryComponent> VFGInventoryComponent;

	UPROPERTY()
	TObjectPtr<class UDynamicMeshComponent> VDynamicMeshComponent;

	UPROPERTY()
	TObjectPtr<class UDynamicMesh> VDynamicMesh;

	UPROPERTY()
	TObjectPtr<class UDynamicMeshComponent> VAroundDynamicMeshComponent;

	UPROPERTY()
	TObjectPtr<class UDynamicMesh> VAroundDynamicMesh;

	UPROPERTY(Replicated)
	TObjectPtr<class AGameplayAbilityTargetActor> VGameplayAbilityTargetActor;

	UPROPERTY(Replicated)
	TArray<FVector2D> Vertices2D;

	UPROPERTY()
	TObjectPtr<APlayerController> PossessPlayerController;

	UPROPERTY(Replicated)
	float AroundRadius;

	UPROPERTY(Replicated)
	bool bAroundSpellIndicator;

	UPROPERTY()
	TObjectPtr<class AFGGameState> VFGGameState;

	FOnSkillHotbarDelegate OnSkillHotbar;
	FOnCollapsedDurationProgressBar OnCollapsedDurationProgressBar;
	FCooldownTimeChangedDelegate DurationTimeChangedDelegate;
	FCooldownTimeChangedDelegate CooldownTimeChangedDelegate;

	FTimerHandle CharacterVisibilityTimerHandle;
	FTimerHandle SkillHotbarBroadcastTimer;
	FTimerHandle RespawnTimer;

	FVector DynamicMeshComponentDefaultLocation;

	float VisionRadius;

public:
	FORCEINLINE bool GetbGASDebug() const { return bGASDebug; }
	FORCEINLINE const TMap<FGameplayTag, FAbilityInfo>& GetSetInputAbilities() const { return SetInputAbilities; }
	FORCEINLINE UFGAbilitySystemComponent* GetFGAbilitySystemComponent() const { return VFGAbilitySystemComponent; }
	FORCEINLINE UFGAttributeSet* GetFGAttributeSet() const { return VFGAttributeSet; }
	FORCEINLINE int32 GetTeamIndex() const { return TeamIndex; }
	FORCEINLINE void SetTeamIndex(int32 InTeamIndex) { TeamIndex = InTeamIndex; }
	FORCEINLINE int32 GetOwnPlayerNumber() const { return OwnPlayerNumber; }
	FORCEINLINE void SetOwnPlayerNumber(const int32& InOwnPlayerNumber) { OwnPlayerNumber = InOwnPlayerNumber; }
	FORCEINLINE void SetCharacterPlayerClass(TSubclassOf<AFGCharacterPlayer> InCharacterPlayerClass) { CharacterPlayerClass = InCharacterPlayerClass; };
	FORCEINLINE int32 GetSelectedCharacterNumber() const { return SelectedCharacterNumber; }
	FORCEINLINE void SetSelectedCharacterNumber(int32 InSelectedCharacterNumber) { SelectedCharacterNumber = InSelectedCharacterNumber; }
	FORCEINLINE UTexture2D* GetCharacterImage() const { return CharacterImage; }
	FORCEINLINE void SetCharacterImage(UTexture2D* InCharacterImage) { CharacterImage = InCharacterImage; }
	FORCEINLINE AFGPlayerController* GetSpawnFGPlayerController() const { return SpawnFGPlayerController; }
	FORCEINLINE void SetSpawnFGPlayerController(AFGPlayerController* InSpawnFGPlayerController) { SpawnFGPlayerController = InSpawnFGPlayerController; }
	FORCEINLINE UPaperSpriteComponent* GetTeamPaperSpriteComponent() const { return TeamPaperSpriteComponent; }
	FORCEINLINE float GetVisionRadius() const { return VisionRadius; }
	FORCEINLINE UFGInventoryComponent* GetFGInventoryComponent() const { return VFGInventoryComponent; }
	FORCEINLINE AGameplayAbilityTargetActor* GetVGameplayAbilityTargetActor() const { return VGameplayAbilityTargetActor; }
	FORCEINLINE void SetGameplayAbilityTargetActor(AGameplayAbilityTargetActor* InGameplayAbilityTargetActor) { VGameplayAbilityTargetActor = InGameplayAbilityTargetActor; }
	FORCEINLINE const TArray<FVector2D>& GetVertices2D() const { return Vertices2D; }
	FORCEINLINE void SetVertices2D(const TArray<FVector2D>& InVertices2D) { Vertices2D = InVertices2D; }
	FORCEINLINE UFGUserWidgetHUD* GetVFGUserWidgetHUD() const { return VFGUserWidgetHUD; }
	FORCEINLINE void SetAroundRadius(const float& InAroundRadius) { AroundRadius = InAroundRadius; }
	FORCEINLINE bool GetbAroundSpellIndicator() const { return bAroundSpellIndicator; }
	FORCEINLINE void SetbAroundSpellIndicator(bool InbAroundSpellIndicator) { bAroundSpellIndicator = InbAroundSpellIndicator; }

public:
	AFGCharacterPlayer();
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;

	virtual void SetDead(AFGCharacterBase* Killer) override;

	virtual void SetDeadFunction(AFGCharacterBase* Killer) override;

	void SettingAbility();

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	void FirstMove(const FInputActionValue& Value);
	void StopMove();
	void FirstLook(const FInputActionValue& Value);

public:
	void SetControl(ECharacterControlType NewCharacterControlType);

	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_Controller() override;
	virtual void UnPossessed() override;

protected:
	void PossessFunction();

public:
	void UnpossessFunction();

	void WeakPossess();
	void WeakUnpossess();

protected:
	void AddCrosshair();
	void CreateUserWidgetHUD();
	void VisibleUserWidgetHUD();

	void SetGameplayTagPairs();

public:
	void SkillHotbarBroadcast();
	void KeySettingSkillHotbarBroadcast(ECharacterControlType InECharacterControlType);

protected:
	UFUNCTION()
	void OnGameplayEffectAdded(UAbilitySystemComponent* ASC, const FGameplayEffectSpec& Spec, FActiveGameplayEffectHandle Handle);

	void GameplayEffectAddedDurationFunction(int32 Number, UAbilitySystemComponent* ASC, const FGameplayEffectSpec& Spec, FActiveGameplayEffectHandle Handle);

	UFUNCTION(NetMulticast, Reliable)
	void NetMulticast_DurationTimeChangedDelegate(int32 Number, FActiveGameplayEffectHandle InEffectHandle, float InRemainingTime, float InGetDuration);

	UFUNCTION(NetMulticast, Reliable)
	void NetMulticast_TagDurationTimerClearTimer(int32 Number);

	UFUNCTION(NetMulticast, Reliable)
	void NetMulticast_CollapsedDurationProgressBar(int32 Number);

	void GameplayEffectAddedCooldownFunction(int32 Number, UAbilitySystemComponent* ASC, const FGameplayEffectSpec& Spec, FActiveGameplayEffectHandle Handle);

	UFUNCTION(NetMulticast, Reliable)
	void NetMulticast_CooldownTimeChangedDelegate(int32 Number, FActiveGameplayEffectHandle InEffectHandle, float InRemainingTime, float InGetDuration);

	UFUNCTION(NetMulticast, Reliable)
	void NetMulticast_TagCooldownTimerClearTimer(int32 Number);

	void SetPaperSprite();

public:
	void AppendTriangulatedPolygonFunction();
	void ResetDynamicMesh();

	void AroundAppendTriangulatedPolygonFunction();
	void ResetAroundDynamicMesh();

	void SetDynamicMeshComponentDefaultLocation();
	void SetDynamicMeshComponentLocation();

protected:
	virtual void OnChildWidgetCreated(AFGChildWidgetComponent* InFGChildWidgetComponent) override;

//Interface
protected:
	virtual void SetFGSkillHotbar(class UFGSkillHotbar* InFGSkillHotbar) override;

	virtual void SetSelectCharacterDecalVisibility(bool bVisibility) override;

	virtual float GetMouseSensitivity() const override { return MouseSensitivity; }
	virtual void SetMouseSensitivity(float InSensitivity) override { MouseSensitivity = InSensitivity; }

	virtual EMouseCursor::Type ChildMouseHover(int32 InTeamIndex, int32 InOwnPlayerNumber) override;
};
