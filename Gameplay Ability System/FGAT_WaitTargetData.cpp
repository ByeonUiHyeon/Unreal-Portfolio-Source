// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/AT/FGAT_WaitTargetData.h"
#include "AbilitySystemComponent.h"

UFGAT_WaitTargetData* UFGAT_WaitTargetData::WaitTargetDataFunction(UGameplayAbility* OwningAbility, FName TaskInstanceName, TEnumAsByte<EGameplayTargetingConfirmation::Type> ConfirmationType, TSubclassOf<AGameplayAbilityTargetActor> Class)
{
    UFGAT_WaitTargetData* NWaitTargetData = NewAbilityTask<UFGAT_WaitTargetData>(OwningAbility, TaskInstanceName);
    if (IsValid(NWaitTargetData))
    {
        NWaitTargetData->TargetClass = Class;
        NWaitTargetData->TargetActor = nullptr;
        NWaitTargetData->ConfirmationType = ConfirmationType;

        return NWaitTargetData;
    }

    return nullptr;
}

void UFGAT_WaitTargetData::Activate()
{
    Super::Activate();

    AGameplayAbilityTargetActor* GroundTraceActor = nullptr;
    if (BeginSpawningActor(Ability, TargetClass, GroundTraceActor))
    {
        FinishSpawningActor(Ability, GroundTraceActor);
    }

    SetWaitingOnAvatar();
}
