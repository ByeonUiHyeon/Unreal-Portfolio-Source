
#pragma once

#include "CoreMinimal.h"
#include "FGSlotData.generated.h"

USTRUCT(BlueprintType)
struct FSlotItemInfo
{
    GENERATED_BODY()

    UPROPERTY(EditDefaultsOnly)
    FName ItemID = NAME_None;

    UPROPERTY(EditDefaultsOnly)
    int32 ItemQuantity = 0;

    FSlotItemInfo() = default;

    FSlotItemInfo(FName InItemID, int32 InItemQuantity) :
        ItemID(InItemID),
        ItemQuantity(InItemQuantity)
    {
    }
};

UENUM(BlueprintType)
enum class ESlotType : uint8
{
    Inventory UMETA(DisplayName = "Inventory"),
    Shop      UMETA(DisplayName = "Shop"),
    Equipment UMETA(DisplayName = "Equipment"),
    Storage   UMETA(DisplayName = "Storage")
};
