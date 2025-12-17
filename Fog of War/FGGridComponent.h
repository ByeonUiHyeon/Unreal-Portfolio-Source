// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FGGridComponent.generated.h"

class AFGNexus;

UENUM(BlueprintType)
enum class EGridType : uint8
{
    None = 0,
	Floor,
	Nexus,
	ComplexObject,
	StaticActor,
	DynamicActor,
	Player,
	Wall
};

UCLASS(Blueprintable, ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class FIGHTGAME_API UFGGridComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
	EGridType GridType;
	
	UPROPERTY()
	TObjectPtr<class AFGGameState> VFGGameState;

protected:
	virtual void BeginPlay() override;

public:
	virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;

	static FName GetNameByEnum(EGridType InGridType);

	void GetActorGirdRowsAndColumns(float InGridSize, int32& OutRows, int32& OutColumns);
	void GetActorGridHeight(float InGridSize, int32 InRows, int32 InColumns, TArray<int32>& OutBlockedArray);
	void GetActorGridComplexHeight(float InGridSize, int32 InRows, int32 InColumns, TArray<int32>& OutBlockedArray);
	void GetActorGridLocation(TArray<TObjectPtr<AActor>>& InActorGridArray);
	void GetNexusGridLocation(TArray<TObjectPtr<AFGNexus>>& InNexusArray);
};
