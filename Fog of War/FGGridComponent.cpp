// Fill out your copyright notice in the Description page of Project Settings.


#include "Grid/FGGridComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Actors/FGNexus.h"
#include "Game/FGGameState.h"
#include "Character/FGCharacterPlayer.h"

#include "PhysicsEngine/BodySetup.h"
#include "Chaos/HeightField.h"

void UFGGridComponent::BeginPlay()
{
	Super::BeginPlay();

	ComponentTags.Add(GetNameByEnum(GridType));

    VFGGameState = GetWorld()->GetGameState<AFGGameState>();
    if (IsValid(VFGGameState))
    {
        if (GridType == EGridType::Player)
        {
            AFGCharacterPlayer* CFGCharacterPlayer = Cast<AFGCharacterPlayer>(GetOwner());
            if (IsValid(CFGCharacterPlayer))
            {
                if (CFGCharacterPlayer->GetTeamIndex() == 0)
                {
                    VFGGameState->AddOtherActorOfTeamB(GetOwner());
                }
                else if (CFGCharacterPlayer->GetTeamIndex() == 1)
                {
                    VFGGameState->AddOtherActorOfTeamA(GetOwner());
                }
            }
        }
        else if (GridType == EGridType::DynamicActor)
        {
            VFGGameState->AddOtherActorOfTeamA(GetOwner());
            VFGGameState->AddOtherActorOfTeamB(GetOwner());
        }
    }
}

void UFGGridComponent::OnComponentDestroyed(bool bDestroyingHierarchy)
{
    Super::OnComponentDestroyed(bDestroyingHierarchy);

    if (IsValid(VFGGameState) && GridType == EGridType::DynamicActor)
    {
        VFGGameState->RemoveOtherActorOfTeamA(GetOwner());
        VFGGameState->RemoveOtherActorOfTeamB(GetOwner());
    }
}

FName UFGGridComponent::GetNameByEnum(EGridType InGridType)
{
	switch (InGridType)
	{
	case EGridType::Floor: return FName("Floor");
	case EGridType::Nexus:  return FName("Nexus");
	case EGridType::ComplexObject:  return FName("ComplexObject");
	case EGridType::StaticActor:  return FName("StaticActor");
	case EGridType::DynamicActor:  return FName("DynamicActor");
	case EGridType::Player:  return FName("Player");
	case EGridType::Wall:  return FName("Wall");
	default:               return NAME_None;
	}
}

void UFGGridComponent::GetActorGirdRowsAndColumns(float InGridSize, int32& OutRows, int32& OutColumns)
{
	FVector OriginValue;
	FVector BoxExtentValue;
	GetOwner()->GetActorBounds(true, OriginValue, BoxExtentValue);

	FVector MinSize = OriginValue - BoxExtentValue;
	FVector MaxSize = OriginValue + BoxExtentValue;

	float WidthValue = MaxSize.X - MinSize.X;
	float HeightValue = MaxSize.Y - MinSize.Y;

	OutColumns = FMath::CeilToInt(WidthValue / InGridSize);
	OutRows = FMath::CeilToInt(HeightValue / InGridSize);
}

void UFGGridComponent::GetActorGridHeight(float InGridSize, int32 InRows, int32 InColumns, TArray<int32>& OutBlockedArray)
{
    FVector OriginValue;
    FVector BoxExtentValue;
    GetOwner()->GetActorBounds(true, OriginValue, BoxExtentValue);

    FVector MinSize = OriginValue - BoxExtentValue;
    FVector MaxSize = OriginValue + BoxExtentValue;

    int32 MinGridX = FMath::FloorToInt(MinSize.X / InGridSize);
    int32 MaxGridX = FMath::FloorToInt(MaxSize.X / InGridSize);

    int32 MinGridY = FMath::FloorToInt(MinSize.Y / InGridSize);
    int32 MaxGridY = FMath::FloorToInt(MaxSize.Y / InGridSize);

    int32 MinGridZ = FMath::FloorToInt(MinSize.Z / InGridSize);
    int32 MaxGridZ = FMath::FloorToInt(MaxSize.Z / InGridSize);

    for (int32 x = MinGridX; x <= MaxGridX; ++x)
    {
        for (int32 y = MinGridY; y <= MaxGridY; ++y)
        {
            int32 ClampedX = FMath::Clamp(x, 0, InColumns - 1);
            int32 ClampedY = FMath::Clamp(y, 0, InRows - 1);
            int32 GridIndex = ClampedY * InColumns + ClampedX;
            if (OutBlockedArray.IsValidIndex(GridIndex))
            {
                OutBlockedArray[GridIndex] = MaxGridZ;
            }
        }
    }
}

void UFGGridComponent::GetActorGridComplexHeight(float InGridSize, int32 InRows, int32 InColumns, TArray<int32>& OutBlockedArray)
{
    AActor* Owner = GetOwner();
    if (!IsValid(Owner)) return;

	//오너의 바운드 크기 구함
	//대각선으로 배치할 경우 정면 기준 다시 바운드를 잡아 범위가 커진다.
	//그래서 로컬 크기를 구하고 로컬 크기 안에 포함되는지 조건을 찾아 넣는다.
    FVector OriginValue;
    FVector BoxExtentValue;
    Owner->GetActorBounds(true, OriginValue, BoxExtentValue);

    FVector MinSize = OriginValue - BoxExtentValue;
    FVector MaxSize = OriginValue + BoxExtentValue;

    int32 MinGridX = FMath::FloorToInt(MinSize.X / InGridSize);
    int32 MaxGridX = FMath::FloorToInt(MaxSize.X / InGridSize);

    int32 MinGridY = FMath::FloorToInt(MinSize.Y / InGridSize);
    int32 MaxGridY = FMath::FloorToInt(MaxSize.Y / InGridSize);

    int32 MinGridZ = FMath::FloorToInt(MinSize.Z / InGridSize);
    int32 MaxGridZ = FMath::FloorToInt(MaxSize.Z / InGridSize);

    UPrimitiveComponent* CPrimitiveComponent = Cast<UPrimitiveComponent>(Owner->GetRootComponent());
    if (!IsValid(CPrimitiveComponent)) return;

	//연결한 액터의 로컬 크기를 구한다.
    FBoxSphereBounds GBoxSphereBounds = CPrimitiveComponent->GetLocalBounds();
    FVector LocalOrigin = GBoxSphereBounds.Origin;
    FVector LocalExtent = GBoxSphereBounds.BoxExtent;

    FTransform GComponentTransform = CPrimitiveComponent->GetComponentTransform();

    for (int32 x = MinGridX; x <= MaxGridX; ++x)
    {
        for (int32 y = MinGridY; y <= MaxGridY; ++y)
        {
            int32 ClampedX = FMath::Clamp(x, 0, InColumns - 1);
            int32 ClampedY = FMath::Clamp(y, 0, InRows - 1);

            int32 GridIndex = ClampedY * InColumns + ClampedX;
            if (!OutBlockedArray.IsValidIndex(GridIndex)) continue;

            FVector GridCenter((x + 0.5f) * InGridSize, (y + 0.5f) * InGridSize, OriginValue.Z);
			//그리드 중심

            FVector LocalPoint = GComponentTransform.InverseTransformPosition(GridCenter);
			//로컬 좌표로 변환

            FVector Delta = LocalPoint - LocalOrigin;
			//그리드 중심점이 박스 중심으로부터 얼마나 떨어져 있는지 확인하기위해 설정
			//LocalOrigin는 보통 0

			//Extent는 -x < 0 < +x 범위이기 때문에 절대값으로 판단
            if (FMath::Abs(Delta.X) <= LocalExtent.X && FMath::Abs(Delta.Y) <= LocalExtent.Y && FMath::Abs(Delta.Z) <= LocalExtent.Z)
            {
                OutBlockedArray[GridIndex] = MaxGridZ;
            }
        }
    }
}

void UFGGridComponent::GetActorGridLocation(TArray<TObjectPtr<AActor>>& InActorGridArray)
{
    InActorGridArray.Add(GetOwner());
}

void UFGGridComponent::GetNexusGridLocation(TArray<TObjectPtr<AFGNexus>>& InNexusArray)
{
    AFGNexus* CFGNexus = Cast<AFGNexus>(GetOwner());
    if (IsValid(CFGNexus))
    {
        InNexusArray.Add(CFGNexus);
    }
}
