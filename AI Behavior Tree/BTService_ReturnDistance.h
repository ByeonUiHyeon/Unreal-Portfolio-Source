// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_ReturnDistance.generated.h"

/**
 * 
 */
UCLASS()
class FIGHTGAME_API UBTService_ReturnDistance : public UBTService
{
	GENERATED_BODY()
	
public:
	UBTService_ReturnDistance();

protected:
	virtual uint16 GetInstanceMemorySize() const override;
	virtual void OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void OnCeaseRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
};
