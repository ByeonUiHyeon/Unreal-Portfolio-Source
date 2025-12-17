// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BTService_MouseDetect.h"
#include "AIController.h"
#include "Character/FGCharacterPlayer.h"
#include "Character/FGCharacterMob.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Interface/FGAIInterface.h"
#include "AI/FGAI.h"
#include "Engine/OverlapResult.h"
#include "Physics/FGCollision.h"
#include "Actors/FGNexus.h"

struct FMouseDetect
{
	TWeakObjectPtr<AFGCharacterBase> WFGCharacterBase;
	TWeakObjectPtr<AFGCharacterPlayer> WFGCharacterPlayer;
	TWeakObjectPtr<UBlackboardComponent> WBlackboardComponent;
	FVector SMouseLocation = FVector::ZeroVector;
	float SDetectRadius = 0.f;
	float SAttackRadius = 0.f;
};

UBTService_MouseDetect::UBTService_MouseDetect()
{
	bNotifyBecomeRelevant = true;
	bNotifyCeaseRelevant = true;

	Interval = 0.1f;
}

uint16 UBTService_MouseDetect::GetInstanceMemorySize() const
{
	return sizeof(FMouseDetect);
}

void UBTService_MouseDetect::OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	FMouseDetect* NMouseDetect = new(NodeMemory) FMouseDetect();
	if (NMouseDetect)
	{
		AAIController* GAIController = OwnerComp.GetAIOwner();
		if (!IsValid(GAIController)) return;

		APawn* GPawn = GAIController->GetPawn();
		if (!IsValid(GPawn)) return;

		NMouseDetect->WFGCharacterBase = Cast<AFGCharacterBase>(GPawn);
		NMouseDetect->WFGCharacterPlayer = Cast<AFGCharacterPlayer>(GPawn);

		UBlackboardComponent* GBlackboardComponent = OwnerComp.GetBlackboardComponent();
		if (!IsValid(GBlackboardComponent)) return;

		NMouseDetect->WBlackboardComponent = GBlackboardComponent;
		NMouseDetect->SMouseLocation = GBlackboardComponent->GetValueAsVector(BBKEY_TARGETPOS);

		IFGAIInterface* CFGAIInterface = Cast<IFGAIInterface>(GPawn);
		if (!CFGAIInterface) return;

		NMouseDetect->SDetectRadius = CFGAIInterface->GetAIDetectRadius();
		NMouseDetect->SAttackRadius = CFGAIInterface->GetAIAttackRadius();
	}
}

void UBTService_MouseDetect::OnCeaseRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	FMouseDetect* CMouseDetect = reinterpret_cast<FMouseDetect*>(NodeMemory);
	if (CMouseDetect)
	{
		if (!bMob && !bPatrol)
		{
			if (!CMouseDetect->WBlackboardComponent.IsValid()) return;
			UBlackboardComponent* GBlackboardComponent = CMouseDetect->WBlackboardComponent.Get();

			GBlackboardComponent->SetValueAsObject(BBKEY_TARGET, nullptr);
		}

		CMouseDetect->WFGCharacterBase = nullptr;
		CMouseDetect->WFGCharacterPlayer = nullptr;
		CMouseDetect->WBlackboardComponent = nullptr;
		CMouseDetect->SMouseLocation = FVector::ZeroVector;
		CMouseDetect->SDetectRadius = 0.f;
		CMouseDetect->SAttackRadius = 0.f;
	}
}

void UBTService_MouseDetect::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	//값 불러오기
	FMouseDetect* CMouseDetect = reinterpret_cast<FMouseDetect*>(NodeMemory);
	if (!CMouseDetect) return;

	if (!CMouseDetect->WFGCharacterBase.IsValid()) return;
	AFGCharacterBase* GFGCharacterBase = CMouseDetect->WFGCharacterBase.Get();
	if (GFGCharacterBase->GetbDead()) return;

	FVector CharacterLocation = GFGCharacterBase->GetActorLocation();

	AFGCharacterPlayer* GFGCharacterPlayer = nullptr;

	if (!bMob)
	{
		if (!CMouseDetect->WFGCharacterPlayer.IsValid()) return;
		GFGCharacterPlayer = CMouseDetect->WFGCharacterPlayer.Get();
	}

	if (!CMouseDetect->WBlackboardComponent.IsValid()) return;
	UBlackboardComponent* GBlackboardComponent = CMouseDetect->WBlackboardComponent.Get();

	FVector MouseLocation = CMouseDetect->SMouseLocation;

	float DetectRadiusValue = CMouseDetect->SDetectRadius;
	float AttackRadiusValue = CMouseDetect->SAttackRadius;

	//초기 타겟 설정
	AActor* TargetActor = Cast<AActor>(GBlackboardComponent->GetValueAsObject(BBKEY_TARGET));
	if (IsValid(TargetActor))
	{
		AFGCharacterBase* TargetCharacter = Cast<AFGCharacterBase>(TargetActor);
		if (IsValid(TargetCharacter) && TargetCharacter->GetbDead())
		{
			GBlackboardComponent->SetValueAsObject(BBKEY_TARGET, nullptr);
		}

		FVector TargetLocation = TargetActor->GetActorLocation();

		float DistanceToTargetActor = FVector::Dist(CharacterLocation, TargetLocation);
		if (DistanceToTargetActor > DetectRadiusValue)
		{
			if (bMob || bPatrol)
			{
				FVector Direction = (TargetLocation - CharacterLocation).GetSafeNormal();
				FVector MoveToAttackRadius = TargetLocation - Direction * (AttackRadiusValue - AttackRadiusAlign);

				GBlackboardComponent->SetValueAsVector(BBKEY_ATTACKPOS, MoveToAttackRadius);
			}
			else
			{
				GBlackboardComponent->SetValueAsObject(BBKEY_TARGET, nullptr);
			}
		}

		if (!bMob)
		{
			AFGNexus* TagetNexus = Cast<AFGNexus>(TargetActor);
			if (IsValid(TagetNexus) && TagetNexus->GetbGameOver())
			{
				GBlackboardComponent->SetValueAsObject(BBKEY_TARGET, nullptr);
			}
		}
	}

	//오버랩
	TArray<FOverlapResult> OverlapResults;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(MouseDetect), false, GFGCharacterBase);

	bool bOverlap = GetWorld()->OverlapMultiByChannel
	(
		OverlapResults,
		CharacterLocation,
		FQuat::Identity,
		CHANNEL_FGOVERLAP,
		FCollisionShape::MakeSphere(DetectRadiusValue),
		QueryParams
	);

	if (bOverlap)
	{
		float NearestDistance = FLT_MAX;
		AActor* NearestActor = nullptr;

		for (auto const& OverlapResult : OverlapResults)
		{
			if (!OverlapResult.bBlockingHit) continue;

			AActor* OverlapActor = Cast<AActor>(OverlapResult.GetActor());
			if (!IsValid(OverlapActor)) continue;

			AFGCharacterPlayer* OverlapCharacterPlayer = Cast<AFGCharacterPlayer>(OverlapActor);
			if (IsValid(OverlapCharacterPlayer))
			{
				if (!bMob)
				{
					if (OverlapCharacterPlayer->GetTeamIndex() == GFGCharacterPlayer->GetTeamIndex()) continue;
				}

				if (OverlapCharacterPlayer->GetbDead()) continue;
			}

			AFGNexus* OverlapNexus = Cast<AFGNexus>(OverlapActor);
			if (bMob)
			{
				if (IsValid(OverlapNexus)) continue;
			}
			else
			{
				if (IsValid(OverlapNexus))
				{
					if (OverlapNexus->GetTeamIndex() == GFGCharacterPlayer->GetTeamIndex()) continue;
					if (OverlapNexus->GetbGameOver()) continue;
				}
			}

			FVector SelectLocation = bUseMouse ? MouseLocation : CharacterLocation;

			float MouseDistance = FVector::Dist(SelectLocation, OverlapActor->GetActorLocation());
			if (MouseDistance < NearestDistance)
			{
				NearestDistance = MouseDistance;
				NearestActor = OverlapActor;
			}
		}

		if (bDrawDebug)
		{
			DrawDebugSphere(GetWorld(), CharacterLocation, DetectRadiusValue, 16, FColor::Green, false, 0.2f);
		}

		//벽 있으면 차단
		if (IsValid(NearestActor))
		{
			FHitResult HitResult;
			FVector End = NearestActor->GetActorLocation();

			bool bHit = GetWorld()->LineTraceSingleByChannel
			(
				HitResult,
				CharacterLocation,
				End,
				CHANNEL_FGTRACE,
				QueryParams
			);

			if (bHit && HitResult.bBlockingHit && HitResult.GetActor() == NearestActor)
			{
				//최종 타겟 설정
				GBlackboardComponent->SetValueAsObject(BBKEY_TARGET, NearestActor);

				//어택 가능 범위까지 이동
				FVector HitImpactPoint = HitResult.ImpactPoint;

				float DistanceToNearestActor = FVector::Dist(CharacterLocation, HitImpactPoint);
				if (DistanceToNearestActor > AttackRadiusValue)
				{
					FVector Direction = (HitImpactPoint - CharacterLocation).GetSafeNormal();
					FVector MoveToAttackRadius = HitImpactPoint - Direction * (AttackRadiusValue - AttackRadiusAlign);

					GBlackboardComponent->SetValueAsVector(BBKEY_ATTACKPOS, MoveToAttackRadius);
				}

				if (bDrawDebug)
				{
					DrawDebugLine(GetWorld(), CharacterLocation, HitImpactPoint, FColor::Green, false, 0.2f);
				}
			}
		}

		return;
	}

	if (bDrawDebug)
	{
		DrawDebugSphere(GetWorld(), CharacterLocation, DetectRadiusValue, 16, FColor::Red, false, 0.2f);
	}
}
