// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_MouseDetect.generated.h"

/**
 * 
 */
UCLASS()
class FIGHTGAME_API UBTService_MouseDetect : public UBTService
{
	GENERATED_BODY()
	
protected:
	UPROPERTY(EditAnywhere, Category = "GAS")
	bool bMob = false;

	UPROPERTY(EditAnywhere, Category = "GAS")
	bool bUseMouse = true;

	UPROPERTY(EditAnywhere, Category = "GAS")
	bool bPatrol = false;

	UPROPERTY(EditAnywhere, Category = "GAS")
	bool bDrawDebug;

	UPROPERTY(EditAnywhere, Category = "GAS")
	float AttackRadiusAlign = 75.f;

public:
	UBTService_MouseDetect();

protected:
	virtual uint16 GetInstanceMemorySize() const override;
	virtual void OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void OnCeaseRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
};
