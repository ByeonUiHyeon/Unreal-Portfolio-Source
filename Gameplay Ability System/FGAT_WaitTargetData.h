// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask_WaitTargetData.h"
#include "FGAT_WaitTargetData.generated.h"

/**
 * 
 */
UCLASS()
class FIGHTGAME_API UFGAT_WaitTargetData : public UAbilityTask_WaitTargetData
{
	GENERATED_BODY()

public:
	FORCEINLINE AGameplayAbilityTargetActor* GetTargetActor() const { return TargetActor; }

public:
    UFUNCTION(BlueprintCallable, meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "true", HideSpawnParms = "Instigator"), Category = "Ability|Tasks")
    static UFGAT_WaitTargetData* WaitTargetDataFunction(UGameplayAbility* OwningAbility, FName TaskInstanceName, TEnumAsByte<EGameplayTargetingConfirmation::Type> ConfirmationType, TSubclassOf<AGameplayAbilityTargetActor> Class);

	virtual void Activate() override;
};
