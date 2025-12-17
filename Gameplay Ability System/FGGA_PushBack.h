// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/GA/FGGameplayAbility.h"
#include "FGGA_PushBack.generated.h"

/**
 * 
 */
UCLASS()
class FIGHTGAME_API UFGGA_PushBack : public UFGGameplayAbility
{
	GENERATED_BODY()
	
private:
	UPROPERTY(EditDefaultsOnly, meta = (ClampMin = "0.0", ClampMax = "1000.0", UIMin = "0.0", UIMax = "1000.0"), Category = "GAS_Trace")
	float TraceHeight = 20.f;

	UPROPERTY(EditDefaultsOnly, meta = (ClampMin = "0.0", ClampMax = "1000.0", UIMin = "0.0", UIMax = "1000.0"), Category = "GAS_Trace")
	float TraceLength = 250.f;

	UPROPERTY(EditDefaultsOnly, meta = (ClampMin = "0.0", ClampMax = "1000.0", UIMin = "0.0", UIMax = "1000.0"), Category = "GAS_Trace")
	float TraceConeAngle = 60.f;

	UPROPERTY(EditDefaultsOnly, meta = (ClampMin = "0", ClampMax = "100", UIMin = "0", UIMax = "100"), Category = "GAS_Trace")
	int32 TraceCount = 6;

	FVector TraceStart;
	FVector ForwardVector;
	FTransform ActorTransform;

	float StepAngle;

public:
	UFGGA_PushBack();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	virtual void OnTargetDataReady(const FGameplayAbilityTargetDataHandle& Data) override;

	virtual void OnEventReceived(FGameplayEventData Payload) override;

	void ConeTraceFunction(const FVector& InFVector);

	virtual void MiddleTraceResultFunction(const FHitResult& InHitResult, AActor* InActor) override;
};
