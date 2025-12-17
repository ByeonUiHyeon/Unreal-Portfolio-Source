// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BTService_ReturnDistance.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AI/FGAI.h"
#include "Interface/FGAIInterface.h"

struct FReturnDistance
{
    TWeakObjectPtr<APawn> WPawn;
    float SReturnDistance = 0.f;
    TWeakObjectPtr<UBlackboardComponent> WBlackboardComponent;
};

UBTService_ReturnDistance::UBTService_ReturnDistance()
{
    bNotifyBecomeRelevant = true;
    //OnBecomeRelevant 사용 가능하게 함
    bNotifyCeaseRelevant = true;
    //OnCeaseRelevant 사용 가능하게 함

	Interval = 0.1f;
}

//FReturnDistance의 사이즈를 리턴 해 NodeMemory의 크기를 알 수 있게 한다.
uint16 UBTService_ReturnDistance::GetInstanceMemorySize() const
{
    return sizeof(FReturnDistance);
}

//서비스 활성화될 때 한번 실행
void UBTService_ReturnDistance::OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    //AI 인스턴스별로 독립적인 상태 관리 하기 위해 NodeMemory에 저장
    FReturnDistance* NReturnDistance = new(NodeMemory) FReturnDistance();
    if (NReturnDistance)
    {
        AAIController* GAIController = OwnerComp.GetAIOwner();
        if (!IsValid(GAIController)) return;

        APawn* GPawn = GAIController->GetPawn();
        if (!IsValid(GPawn)) return;

        NReturnDistance->WPawn = GPawn;

        IFGAIInterface* CFGAIInterface = Cast<IFGAIInterface>(GPawn);
        if (!CFGAIInterface) return;

        NReturnDistance->SReturnDistance = CFGAIInterface->GetAIReturnDistance();

        NReturnDistance->WBlackboardComponent = OwnerComp.GetBlackboardComponent();
    }
}

//서비스 종료될 때 실행
void UBTService_ReturnDistance::OnCeaseRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    FReturnDistance* CReturnDistance = reinterpret_cast<FReturnDistance*>(NodeMemory);
    if (CReturnDistance)
    {
        CReturnDistance->WPawn = nullptr;
        CReturnDistance->SReturnDistance = 0.f;
        CReturnDistance->WBlackboardComponent = nullptr;
    }
}

void UBTService_ReturnDistance::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

    FReturnDistance* CReturnDistance = reinterpret_cast<FReturnDistance*>(NodeMemory);
    if (!CReturnDistance) return;

    if (!CReturnDistance->WPawn.IsValid()) return;
    APawn* GPawn = CReturnDistance->WPawn.Get();

    FVector OwnerLocation = GPawn->GetActorLocation();

    float ReturnDistanceValue = CReturnDistance->SReturnDistance;

    if (!CReturnDistance->WBlackboardComponent.IsValid()) return;
    UBlackboardComponent* GBlackboardComponent = CReturnDistance->WBlackboardComponent.Get();

    FVector HomePosition = GBlackboardComponent->GetValueAsVector(BBKEY_HOMEPOS);

    float Distance = FVector::Dist(OwnerLocation, HomePosition);
    if (Distance < ReturnDistanceValue)
    {
        GBlackboardComponent->SetValueAsBool(BBKEY_RETRUNHOMEPOS, false);
    }
    else
    {
        GBlackboardComponent->SetValueAsBool(BBKEY_RETRUNHOMEPOS, true);
        GBlackboardComponent->SetValueAsObject(BBKEY_TARGET, nullptr);
    }
}
