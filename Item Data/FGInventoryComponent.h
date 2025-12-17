// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/FGSlotData.h"
#include "FGInventoryComponent.generated.h"

class AFGCharacterPlayer;

DECLARE_MULTICAST_DELEGATE(FOnUpdateInventorySlot);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class FIGHTGAME_API UFGInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

private:
	UPROPERTY(ReplicatedUsing = OnRep_InventorySlots, EditDefaultsOnly, Category = "GAS_Inventory")
	TArray<FSlotItemInfo> InventorySlots;

	UPROPERTY(EditDefaultsOnly, Category = "GAS_Inventory")
	int32 SlotQuantity = 6;

	UPROPERTY()
	TObjectPtr<class UDataTable> VDataTable;

	FOnUpdateInventorySlot OnUpdateInventorySlot;

	FTimerHandle UsePotionTimer;
	FTimerHandle EndUsePotionTimer;

public:
	FORCEINLINE const TArray<FSlotItemInfo>& GetInventorySlots() const { return InventorySlots; }
	FORCEINLINE FOnUpdateInventorySlot& GetOnUpdateInventorySlot() { return OnUpdateInventorySlot; }

public:	
	UFGInventoryComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnRep_InventorySlots();

public:
	bool CheckBuy(FName InItemID, int32 InItemQuantity);
	void BuyItem(AFGCharacterPlayer* InFGCharacterPlayer, FName InItemID, int32 InDropIndex, int32 InItemQuantity);
	void EmptySlotStack(FName InItemID, int32 InMaxStackQuantityValue, int32 InRemainItemQuantity);

	void SellItem(class AFGShop* InFGShop, AFGCharacterPlayer* InFGCharacterPlayer, int32 InSlotIndex, int32 InItemQuantity);

	void SwapItem(int32 FromSlotIndex, int32 ToSlotIndex);

	void UseItem(AFGCharacterPlayer* InFGCharacterPlayer, int32 InSlotIndex);

	void InitialInventoryItemAbility(AFGCharacterPlayer* InFGCharacterPlayer);
	void InventoryItemAbility(FName InItemID, AFGCharacterPlayer* InFGCharacterPlayer, bool bBuy);
};
