
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "FGItemDataTable.generated.h"

USTRUCT(BlueprintType)
struct FFGItemDataTable : public FTableRowBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FText ItemName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FText Description;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	int32 MaxStackQuantity;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	int32 Price;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float Health;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float HealthRegen;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float Mana;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float ManaRegen;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float Strength;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FSoftObjectPath DataAssetPath;
};
