// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "Tag/FGGameplayTag.h"
#include "Data/FGMacro.h"
#include "Navigation/PathFollowingComponent.h"
#include "FGGameplayAbility.generated.h"

struct FFGGameplayEffectContext;

USTRUCT(BlueprintType)
struct FSkillMontageStruct
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<class UAnimMontage> SkillMontage;

	UPROPERTY(EditDefaultsOnly, meta = (InlineEditConditionToggle))
	bool bNotifyTag;

	UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "bNotifyTag"))
	FGameplayTag NotifyTag;

	FSkillMontageStruct() = default;
};

USTRUCT(BlueprintType)
struct FSkillTraceStruct
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, meta = (ClampMin = "0.0", ClampMax = "1000.0", UIMin = "0.0", UIMax = "1000.0"))
	float TraceRadius;

	UPROPERTY(EditDefaultsOnly)
	bool bDrawDebug;

	FSkillTraceStruct() = default;
};

USTRUCT(BlueprintType)
struct FSkillEffectStruct
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	FGameplayTag GameplayCueTag;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<USoundBase> SkillSoundBase;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<class UParticleSystem> SkillParticleSystem;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<class UNiagaraSystem> SkillNiagaraSystem;

	FSkillEffectStruct() = default;
};

/**
 * 
 */
UCLASS()
class FIGHTGAME_API UFGGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()
	
protected:
	UPROPERTY()
	TObjectPtr<class AFGCharacterBase> VFGCharacterBase;

	UPROPERTY()
	TObjectPtr<class AFGCharacterPlayer> VFGCharacterPlayer;

	UPROPERTY()
	TObjectPtr<class AFGCharacterSniper> VFGCharacterSniper;

	UPROPERTY()
	TObjectPtr<class AFGAIController> VFGAIController;

	UPROPERTY()
	TObjectPtr<class UPathFollowingComponent> VPathFollowingComponent;

	UPROPERTY()
	TObjectPtr<class UFGAbilitySystemComponent> VFGAbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<const class UFGAttributeSet> VFGAttributeSet;

	UPROPERTY()
	TSet<TObjectPtr<AActor>> AlreadyHitActors;

	UPROPERTY(EditDefaultsOnly, meta = (InlineEditConditionToggle))
	bool bUseLocation = false;

	UPROPERTY(VisibleDefaultsOnly, meta = (EditCondition = "bUseLocation"), Category = "GAS_SpellIndicator")
	bool bUseLocationToggle;

	UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "bUseLocation", EditConditionHides, ClampMin = "0.0", ClampMax = "10000.0", UIMin = "0.0", UIMax = "10000.0"), Category = "GAS_SpellIndicator")
	float SpellIndicatorClampRadius = 0.f;

	UPROPERTY(EditDefaultsOnly, meta = (InlineEditConditionToggle))
	bool bSkillMana = false;

	UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "bSkillMana", ClampMin = "0.0", ClampMax = "1000.0", UIMin = "0.0", UIMax = "1000.0"), Category = "GAS_Value")
	float SkillMana = 0;

	UPROPERTY(EditDefaultsOnly, meta = (InlineEditConditionToggle))
	bool bSkillBarrier = false;

	UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "bSkillBarrier", ClampMin = "0.0", ClampMax = "1000.0", UIMin = "0.0", UIMax = "1000.0"), Category = "GAS_Value")
	float SkillBarrier = 0;

	UPROPERTY(EditDefaultsOnly, meta = (InlineEditConditionToggle))
	bool bSkillStrength = false;

	UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "bSkillStrength", ClampMin = "0.0", ClampMax = "1000.0", UIMin = "0.0", UIMax = "1000.0"), Category = "GAS_Value")
	float SkillStrength = 0;

	UPROPERTY(EditDefaultsOnly, meta = (InlineEditConditionToggle))
	bool bUseSkillDuration = false;

	UPROPERTY(VisibleDefaultsOnly, meta = (EditCondition = "bUseSkillDuration"), Category = "GAS_Value")
	bool bUseSkillDurationToggle;

	UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "bUseSkillDuration", EditConditionHides), Category = "GAS_Value")
	float SkillDuration = 0;

	UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "bUseSkillDuration", EditConditionHides), Category = "GAS_Value")
	TSubclassOf<class UGameplayEffect> SetByCallerDurationGE;

	UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "bUseSkillDuration", EditConditionHides), Category = "GAS_Value")
	FGameplayTag DurationActivationOwnedTag;

	UPROPERTY(EditDefaultsOnly, meta = (InlineEditConditionToggle))
	bool bUseSkillCooldown = false;

	UPROPERTY(VisibleDefaultsOnly, meta = (EditCondition = "bUseSkillCooldown"), Category = "GAS_Value")
	bool bUseSkillCooldownToggle;

	UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "bUseSkillCooldown", EditConditionHides, ClampMin = "0.0", ClampMax = "100.0", UIMin = "0.0", UIMax = "100.0"), Category = "GAS_Value")
	float SkillCooldown = 0;

	UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "bUseSkillCooldown", EditConditionHides), Category = "GAS_Value")
	TSubclassOf<UGameplayEffect> SetByCallerCooldownGE;

	UPROPERTY(EditDefaultsOnly, meta = (InlineEditConditionToggle))
	bool bSkillDamage = false;

	UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "bSkillDamage", ClampMin = "0.0", ClampMax = "10000.0", UIMin = "0.0", UIMax = "10000.0"), Category = "GAS_Value")
	float SkillDamage = 0;

	UPROPERTY(EditDefaultsOnly, meta = (InlineEditConditionToggle))
	bool bHpMp = false;

	UPROPERTY(VisibleDefaultsOnly, meta = (EditCondition = "bHpMp"), Category = "GAS_Value")
	bool bHpMpToggle;

	UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "bHpMp", EditConditionHides, ClampMin = "-1000.0", ClampMax = "1000.0", UIMin = "-1000.0", UIMax = "1000.0"), Category = "GAS_Value")
	float HealthValue = 0.f;

	UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "bHpMp", EditConditionHides, ClampMin = "-1000.0", ClampMax = "1000.0", UIMin = "-1000.0", UIMax = "1000.0"), Category = "GAS_Value")
	float ManaValue = 0.f;

	UPROPERTY(EditDefaultsOnly, meta = (InlineEditConditionToggle))
	bool bDisableMovementTime = false;

	UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "bDisableMovementTime", ClampMin = "0.0", ClampMax = "10.0", UIMin = "0.0", UIMax = "10.0"), Category = "GAS_Value")
	float DisableMovementTime = 0.2f;

	UPROPERTY(EditDefaultsOnly, meta = (InlineEditConditionToggle))
	bool bPushStrength = false;

	UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "bPushStrength", ClampMin = "0.0", ClampMax = "10000.0", UIMin = "0.0", UIMax = "10000.0"), Category = "GAS_Value")
	float PushStrength = 0;

	UPROPERTY(EditDefaultsOnly, meta = (InlineEditConditionToggle))
	bool bSkillImage = false;

	UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "bSkillImage"), Category = "GAS_Object")
	TObjectPtr<class UTexture2D> SkillImage;

	UPROPERTY(EditDefaultsOnly, meta = (InlineEditConditionToggle))
	bool bSkillMontageMap = false;

	UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "bSkillMontageMap"), Category = "GAS_Object")
	TMap<FName, FSkillMontageStruct> SkillMontageMap;

	UPROPERTY(EditDefaultsOnly, meta = (InlineEditConditionToggle))
	bool bSkillEffectMap = false;

	UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "bSkillEffectMap"), Category = "GAS_Object")
	TMap<FName, FSkillEffectStruct> SkillEffectMap;

	UPROPERTY(EditDefaultsOnly, meta = (InlineEditConditionToggle))
	bool bAddGameplayTagMap = false;

	UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "bAddGameplayTagMap"), Category = "GAS_Tag")
	TMap<FName, FGameplayTag> AddGameplayTagMap;

	UPROPERTY(EditDefaultsOnly, meta = (InlineEditConditionToggle))
	bool bSkillTraceMap = false;

	UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "bSkillTraceMap"), Category = "GAS_Trace")
	TMap<FName, FSkillTraceStruct> SkillTraceMap;

	UPROPERTY(EditDefaultsOnly, meta = (InlineEditConditionToggle))
	bool bProjectile = false;

	UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "bProjectile"), Category = "GAS_Projectile")
	TSubclassOf<class AFGProjectile> FGProjectileClass;

	UPROPERTY()
	TObjectPtr<AFGProjectile> VFGProjectile;

	FGameplayEffectContextHandle ContextHandle;

	FFGGameplayEffectContext* VFGGameplayEffectContext;

	FTimerHandle CharacterPlayerRotationTimer;
	FTimerHandle SkillDurationTimer;
	FTimerHandle SkillDurationEndAbilitytimer;
	FTimerHandle UseInputReleaseTimer;

	FVector MouseCursorLocationValue;

	FName MontageName;
	FName TraceName;

	uint32 bInstantCast : 1;
	uint32 bEventReceived : 1;
	uint32 bUseInputRelease : 1;

public:
	GET_OBJECT(UTexture2D, SkillImage);
	SET_COPY(bool, bInstantCast);

public:
	UFGGameplayAbility();

protected:
	virtual void OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
	virtual void OnRemoveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr, OUT FGameplayTagContainer* OptionalRelevantTags = nullptr) const;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	bool CanUseSkillMana(bool InbUseParameter = false, float InSkillMana = 0.f) const;

	void SpellIndicatorFunction(const FName& TaskInstanceName, const TArray<FVector2D>& InFVector2D);

	UFUNCTION()
	virtual void OnTargetDataReady(const FGameplayAbilityTargetDataHandle& Data);

	void StopAndRotationSetting(bool InbUseStop = true, bool InbUseRotation = true, bool InbUseInputRelease = false);

	bool CanUseSkillWithinRange(const FName& InTraceRadiusName);

	virtual void OnAIMoveRequestFinished(FAIRequestID RequestID, const FPathFollowingResult& Result);

	UFUNCTION()
	virtual void OnTargetDataCancelled(const FGameplayAbilityTargetDataHandle& Data);

	void PlayMontageAndWaitFunction(const FName& TaskInstanceName);

	UFUNCTION()
	virtual void OnPlayMontageInterrupted();

	virtual void OnPlayMontageInterruptedEventReceived();

	UFUNCTION()
	virtual void OnPlayMontageCompleted();

	void WaitGameplayEventFunction(const FName& InName);

	UFUNCTION()
	virtual void OnEventReceived(FGameplayEventData Payload);


	void WaitTraceFunction(const FVector& InStart, const FVector& InEnd, bool InbLineTrace = true, float InTraceRadius = 0, bool InbDebugDraw = false, bool InSingle = true, float InSplitSize = 50.f, float InLifeTime = 3.f);
	void UseWaitTraceByName(const FName& InName, const FVector& InStart, const FVector& InEnd, bool InbLineTrace = false, bool InSingle = true, float InSplitSize = 50.f, float InLifeTime = 3.f);

	UFUNCTION()
	void OnTraceResultFunction(const FGameplayAbilityTargetDataHandle& TargetDataHandle);

	virtual void FrontTraceResultFunction();
	virtual void MiddleTraceResultFunction(const FHitResult& InHitResult, AActor* InActor);
	virtual void BackTraceResultFunction();

	void InputPressFunction();

	UFUNCTION()
	virtual void OnInputPress(float TimeWaited);

	void InputReleaseFunction();

	UFUNCTION()
	virtual void OnInputRelease(float TimeHeld);

	void AddTagFunction(const FGameplayTag& InGameplayTag);
	void AddTagByName(const FName& InName);
	void RemoveTagFunction(const FGameplayTag& InGameplayTag);
	void RemoveTagByName(const FName& InName);
	FGameplayTag GetTagByName(const FName& InName) const;
	FGameplayTag GetCueTagByName(const FName& InName) const;

	bool HasTagsTryAbilities(const FGameplayTagContainer& InHasAllMatchingGameplayTags, const FGameplayTag& InTryActivateAbilitiesByTag);
	void HasTagCancelAbilities(const FGameplayTag& InHasMatchingGameplayTag, const FGameplayTag& InTryActivateAbilitiesByTag);

	void SetByCallerManaFunction(bool InbUseParameter = false, float InSkillMana = 0.f);
	void SetByCallerStrengthFunction(bool InbBuff = true);
	void SetByCallerBarrierFunction(bool InbBuff = true);

	UFUNCTION()
	virtual void OnBarrierDestroyedFunction();

	void SetByCallerDurationFunction();
	virtual void SkillDurationEnd();
	void SkillDurationEarlyEnd();
	void SetByCallerCooldownFunction(bool InbUseParameter = false, float InSkillCooldown = 0.f);
	void SetByCallerDamageFunction(AActor* InHitActor, const FHitResult& InHitResult = FHitResult(), bool InbUseHeadShot = false, bool InbUseCue = true, bool InbUseParameter = false, float InSkillDamage = 0.f, const FGameplayTag& GameplayCueTag = GAMEPLAYCUE_ATTACK_HITEFFECT);
	void SetByCallerHpMpFunction(bool InbUseParameter = false, float InHp = 0.f, float InMp = 0.f);

	void TargetDisableMovement(AActor* InHitActor);

	void PushCharacter(AActor* InHitActor);

	void SoundBaseGameplayCue
	(
		const FName& InName,
		bool InbUseAdd = true,
		float InStartTime = 0.f,
		const FVector& InLocation = FVector::ZeroVector,
		const FRotator& InRotation = FRotator::ZeroRotator,
		float InVolumeMultiplier = 1.f,
		float InPitchMultiplier = 1.f
	);

	void ParticleSystemGameplayCue
	(
		const FName& InName,
		bool InbUseAdd = true,
		const FVector& InLocation = FVector::ZeroVector,
		const FRotator& InRotation = FRotator::ZeroRotator,
		const FVector& InScale = FVector::OneVector,
		bool InbUseAttached = false,
		USceneComponent* InAttachToComponent = nullptr,
		const FName& InAttachPointName = NAME_None,
		EAttachLocation::Type InLocationType = EAttachLocation::KeepRelativeOffset
	);

	void NiagaraSystemGameplayCue
	(
		const FName& InName,
		bool InbUseAdd = true,
		const FVector& InLocation = FVector::ZeroVector,
		const FRotator& InRotation = FRotator::ZeroRotator,
		const FVector& InScale = FVector::OneVector,
		bool InbUseAttached = false,
		USceneComponent* InAttachToComponent = nullptr,
		const FName& InAttachPointName = NAME_None,
		EAttachLocation::Type InLocationType = EAttachLocation::KeepRelativeOffset
	);

	void RemoveGameplayCueFunction(const FName& InName);

	void SpawnProjectile(const FTransform& InSpawnTransform, float InInitialSpeed, float InMaxSpeed, const FVector& InProjectileDirection = FVector::ZeroVector, bool InbHoming = false, AActor* InHitActor = nullptr);
	virtual void OnProjectileHitActorFunction();
	virtual void OnProjectileHitActorFailFunction();

//Get, Set
public:
	virtual TArray<float> GetManaValue() const;
	virtual TArray<float> GetCooldownValue() const;
};
