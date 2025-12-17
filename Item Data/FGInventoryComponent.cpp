// Fill out your copyright notice in the Description page of Project Settings.


#include "Inventory/FGInventoryComponent.h"
#include "Net/UnrealNetwork.h"
#include "Game/FGGameInstance.h"
#include "Data/FGItemDataTable.h"
#include "Shop/FGShopComponent.h"
#include "Shop/FGShop.h"
#include "AbilitySystem/FGAbilitySystemComponent.h"
#include "Attribute/FGAttributeSet.h"
#include "Character/FGCharacterPlayer.h"
#include "Data/FGItemDataAsset.h"
#include "Tag/FGGameplayTag.h"

UFGInventoryComponent::UFGInventoryComponent()
{
	SetIsReplicatedByDefault(true);
	//액터 컴포넌트에서 사용
	//정적

	//SetIsReplicated(true);
	//액터와 컴포넌트에서 모두 사용
	//동적
}

void UFGInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UFGInventoryComponent, InventorySlots);
}

void UFGInventoryComponent::BeginPlay()
{
	Super::BeginPlay();

	InventorySlots.SetNum(SlotQuantity);

	UFGGameInstance* FGGameInstance = GetWorld()->GetGameInstance<UFGGameInstance>();
	if (!IsValid(FGGameInstance)) return;

	VDataTable = FGGameInstance->GetLinkItemDataTable();
	if (!IsValid(VDataTable)) return;
}

void UFGInventoryComponent::OnRep_InventorySlots()
{
	OnUpdateInventorySlot.Broadcast();
}

bool UFGInventoryComponent::CheckBuy(FName InItemID, int32 InItemQuantity)
{
	FFGItemDataTable* FGItemDataTable = VDataTable->FindRow<FFGItemDataTable>(InItemID, TEXT("ItemData"));
	if (!FGItemDataTable) return false;

	int32 MaxStackQuantityValue = FGItemDataTable->MaxStackQuantity;
	int32 RemainItemQuantity = InItemQuantity;

	//같은 아이템 찾고 쌓을 수 있는지 확인
	for (const FSlotItemInfo& InventorySlot : InventorySlots)
	{
		if (InventorySlot.ItemID == InItemID)
		{
			if (InventorySlot.ItemQuantity + RemainItemQuantity <= MaxStackQuantityValue)
			{
				RemainItemQuantity = 0;

				return true;
			}
			else
			{
				RemainItemQuantity = InventorySlot.ItemQuantity + RemainItemQuantity - MaxStackQuantityValue;
			}
		}
	}

	//같은 아이템 없을 경우 빈 슬롯에 쌓을 수 있는지 확인
	if (RemainItemQuantity > 0)
	{
		for (const FSlotItemInfo& InventorySlot : InventorySlots)
		{
			if (InventorySlot.ItemID.IsNone())
			{
				if (RemainItemQuantity <= MaxStackQuantityValue)
				{
					RemainItemQuantity = 0;

					return true;
				}
				else
				{
					RemainItemQuantity -= MaxStackQuantityValue;
				}
			}
		}
	}

	//풀 인벤토리
	return false;
}

void UFGInventoryComponent::BuyItem(AFGCharacterPlayer* InFGCharacterPlayer, FName InItemID, int32 InDropIndex, int32 InItemQuantity)
{
	FFGItemDataTable* FGItemDataTable = VDataTable->FindRow<FFGItemDataTable>(InItemID, TEXT("ItemData"));
	if (!FGItemDataTable) return;

	int32 MaxStackQuantityValue = FGItemDataTable->MaxStackQuantity;
	int32 RemainItemQuantity = InItemQuantity;

	//같은 아이템 있을 경우 같은 아이템에 쌓기
	for (FSlotItemInfo& InventorySlot : InventorySlots)
	{
		if (InventorySlot.ItemID == InItemID)
		{
			if (InventorySlot.ItemQuantity + RemainItemQuantity <= MaxStackQuantityValue)
			{
				InventorySlot.ItemQuantity += RemainItemQuantity;

				RemainItemQuantity = 0;

				break;
			}
			else
			{
				RemainItemQuantity = InventorySlot.ItemQuantity + RemainItemQuantity - MaxStackQuantityValue;

				InventorySlot.ItemQuantity = MaxStackQuantityValue;
			}
		}
	}

	//같은 아이템 없을 경우 빈 슬롯에 쌓기
	if (RemainItemQuantity > 0)
	{
		if (InDropIndex < 0)
		{
			EmptySlotStack(InItemID, MaxStackQuantityValue, RemainItemQuantity);
		}
		else
		{
			if (InventorySlots[InDropIndex].ItemID.IsNone())
			{
				InventorySlots[InDropIndex].ItemID = InItemID;

				if (RemainItemQuantity <= MaxStackQuantityValue)
				{
					InventorySlots[InDropIndex].ItemQuantity += RemainItemQuantity;

					RemainItemQuantity = 0;
				}
				else
				{
					InventorySlots[InDropIndex].ItemQuantity = MaxStackQuantityValue;

					RemainItemQuantity -= MaxStackQuantityValue;

					EmptySlotStack(InItemID, MaxStackQuantityValue, RemainItemQuantity);
				}
			}
			else
			{
				EmptySlotStack(InItemID, MaxStackQuantityValue, RemainItemQuantity);
			}
		}
	}

	//능력치 업데이트
	InventoryItemAbility(InItemID, InFGCharacterPlayer, true);

	OnUpdateInventorySlot.Broadcast();
}

void UFGInventoryComponent::EmptySlotStack(FName InItemID, int32 InMaxStackQuantityValue, int32 InRemainItemQuantity)
{
	int32 RemainItemQuantity = InRemainItemQuantity;

	for (FSlotItemInfo& InventorySlot : InventorySlots)
	{
		if (InventorySlot.ItemID.IsNone())
		{
			InventorySlot.ItemID = InItemID;

			if (RemainItemQuantity <= InMaxStackQuantityValue)
			{
				InventorySlot.ItemQuantity += InRemainItemQuantity;

				RemainItemQuantity = 0;

				break;
			}
			else
			{
				InventorySlot.ItemQuantity = InMaxStackQuantityValue;

				RemainItemQuantity -= InMaxStackQuantityValue;
			}
		}
	}
}

void UFGInventoryComponent::SellItem(AFGShop* InFGShop, AFGCharacterPlayer* InFGCharacterPlayer, int32 InSlotIndex, int32 InItemQuantity)
{
	//아이템 없으면 종료
	if (InventorySlots[InSlotIndex].ItemQuantity <= 0) return;

	//골드 감가
	FFGItemDataTable* FGItemDataTable = VDataTable->FindRow<FFGItemDataTable>(InventorySlots[InSlotIndex].ItemID, TEXT("ItemData"));
	if (!FGItemDataTable) return;

	int32 ItemPrice = FGItemDataTable->Price;

	float DepreciationRate = (1.f - InFGShop->GetDepreciationRate() / 100.f);

	int32 DepreciationItemPrice = FMath::FloorToInt(ItemPrice * DepreciationRate);
	//소수점 버리기
	//int32 DepreciationItemPrice = FMath::RoundToInt(ItemPrice * DepreciationRate);
	//반올림 할 경우

	int32 TotalItemPrice = DepreciationItemPrice * InItemQuantity;

	UFGAbilitySystemComponent* FGAbilitySystemComponent = InFGCharacterPlayer->GetFGAbilitySystemComponent();
	if (!IsValid(FGAbilitySystemComponent)) return;

	int32 PlayerGold = FGAbilitySystemComponent->GetNumericAttributeBase(UFGAttributeSet::GetGoldAttribute());

	PlayerGold += TotalItemPrice;

	//인벤토리에서 감소
	InventorySlots[InSlotIndex].ItemQuantity -= InItemQuantity;

	//골드 증가
	FGAbilitySystemComponent->SetNumericAttributeBase(UFGAttributeSet::GetGoldAttribute(), PlayerGold);

	//능력치 업데이트
	InventoryItemAbility(InventorySlots[InSlotIndex].ItemID, InFGCharacterPlayer, false);

	//상점에 물품 판매
	UFGShopComponent* FGShopComponent = InFGShop->GetFGShopComponent();
	if (!IsValid(FGShopComponent)) return;

	FGShopComponent->SellItem(InventorySlots[InSlotIndex].ItemID, InItemQuantity);

	//슬롯 업데이트
	if (InventorySlots[InSlotIndex].ItemQuantity <= 0)
	{
		InventorySlots[InSlotIndex].ItemID = NAME_None;
		InventorySlots[InSlotIndex].ItemQuantity = 0;
	}

	OnUpdateInventorySlot.Broadcast();
}

void UFGInventoryComponent::SwapItem(int32 FromSlotIndex, int32 ToSlotIndex)
{
	if (InventorySlots.IsValidIndex(FromSlotIndex) && InventorySlots.IsValidIndex(ToSlotIndex))
	{
		InventorySlots.Swap(FromSlotIndex, ToSlotIndex);

		OnUpdateInventorySlot.Broadcast();
	}
}

void UFGInventoryComponent::UseItem(AFGCharacterPlayer* InFGCharacterPlayer, int32 InSlotIndex)
{
	if (InventorySlots[InSlotIndex].ItemQuantity <= 0) return;

	UFGAbilitySystemComponent* FGAbilitySystemComponent = InFGCharacterPlayer->GetFGAbilitySystemComponent();
	if (!IsValid(FGAbilitySystemComponent)) return;

	float CurrentHealth = FGAbilitySystemComponent->GetNumericAttribute(UFGAttributeSet::GetHealthAttribute());
	float MaxHealth = FGAbilitySystemComponent->GetNumericAttribute(UFGAttributeSet::GetMaxHealthAttribute());

	if (CurrentHealth >= MaxHealth) return;

	FFGItemDataTable* FGItemDataTable = VDataTable->FindRow<FFGItemDataTable>(InventorySlots[InSlotIndex].ItemID, TEXT("ItemData"));
	if (!FGItemDataTable) return;

	TSoftObjectPtr<UFGItemDataAsset> AssetRef(FGItemDataTable->DataAssetPath);
	UFGItemDataAsset* FGItemDataAsset = AssetRef.LoadSynchronous();
	if (!IsValid(FGItemDataAsset)) return;

	FGameplayEffectContextHandle ContextHandle = FGAbilitySystemComponent->MakeEffectContext();

	TSubclassOf<UGameplayEffect> GameplayEffect = FGItemDataAsset->ItemEffect;
	if (!IsValid(GameplayEffect)) return;

	FGameplayEffectSpecHandle SpecHandle = FGAbilitySystemComponent->MakeOutgoingSpec(GameplayEffect, 1.f, ContextHandle);
	if (!SpecHandle.IsValid()) return;

	FString ItemIDString = InventorySlots[InSlotIndex].ItemID.ToString();

	if (ItemIDString[0] == 'P')
	{
		if (ItemIDString[1] == 'H')
		{
			//아이템 소모
			InventorySlots[InSlotIndex].ItemQuantity -= 1;

			if (InventorySlots[InSlotIndex].ItemQuantity <= 0)
			{
				InventorySlots[InSlotIndex].ItemID = NAME_None;
				InventorySlots[InSlotIndex].ItemQuantity = 0;
			}

			OnUpdateInventorySlot.Broadcast();

			//포션 사용
			float PotionValue = FGItemDataTable->Health;
			float PotionRecoveryPeriod = 0.5f;
			float EndPotionRecoveryTime = 3.f;
			float PotionRecoveryTimeInterval = EndPotionRecoveryTime / PotionRecoveryPeriod;
			float PotionRecoveryInterval = PotionValue / PotionRecoveryTimeInterval;

			GetWorld()->GetTimerManager().SetTimer(UsePotionTimer, FTimerDelegate::CreateLambda([this, FGAbilitySystemComponent, SpecHandle, PotionRecoveryInterval]()
				{
					FGameplayEffectSpec* Spec = SpecHandle.Data.Get();
					if (!Spec) return;

					Spec->SetSetByCallerMagnitude(SETBYCALLER_TAG_HEALTH, PotionRecoveryInterval);

					FGAbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*Spec);
				}
			), PotionRecoveryPeriod, true);

			GetWorld()->GetTimerManager().SetTimer(EndUsePotionTimer, FTimerDelegate::CreateLambda([&]()
				{
					GetWorld()->GetTimerManager().ClearTimer(UsePotionTimer);

					GetWorld()->GetTimerManager().ClearTimer(EndUsePotionTimer);
				}
			), EndPotionRecoveryTime, false);
		}
	}
}

void UFGInventoryComponent::InitialInventoryItemAbility(AFGCharacterPlayer* InFGCharacterPlayer)
{
	UFGAbilitySystemComponent* FGAbilitySystemComponent = InFGCharacterPlayer->GetFGAbilitySystemComponent();
	if (!IsValid(FGAbilitySystemComponent)) return;

	for (const FSlotItemInfo& InventorySlot : InventorySlots)
	{
		FString ItemIDString = InventorySlot.ItemID.ToString();

		if (ItemIDString.StartsWith("E"))
		{
			FFGItemDataTable* FGItemDataTable = VDataTable->FindRow<FFGItemDataTable>(InventorySlot.ItemID, TEXT("ItemData"));
			if (!FGItemDataTable) return;

			TSoftObjectPtr<UFGItemDataAsset> AssetRef(FGItemDataTable->DataAssetPath);
			UFGItemDataAsset* FGItemDataAsset = AssetRef.LoadSynchronous();
			if (!IsValid(FGItemDataAsset)) return;

			FGameplayEffectContextHandle ContextHandle = FGAbilitySystemComponent->MakeEffectContext();

			TSubclassOf<UGameplayEffect> GameplayEffect = FGItemDataAsset->ItemEffect;
			if (!IsValid(GameplayEffect)) return;

			FGameplayEffectSpecHandle SpecHandle = FGAbilitySystemComponent->MakeOutgoingSpec(GameplayEffect, 1.f, ContextHandle);
			if (!SpecHandle.IsValid()) return;

			FGameplayEffectSpec* Spec = SpecHandle.Data.Get();
			if (!Spec) return;

			Spec->SetSetByCallerMagnitude(SETBYCALLER_TAG_MAXHEALTH, FGItemDataTable->Health);
			Spec->SetSetByCallerMagnitude(SETBYCALLER_TAG_HEALTHREGEN, FGItemDataTable->HealthRegen);
			Spec->SetSetByCallerMagnitude(SETBYCALLER_TAG_MAXMANA, FGItemDataTable->Mana);
			Spec->SetSetByCallerMagnitude(SETBYCALLER_TAG_MANAREGEN, FGItemDataTable->ManaRegen);
			Spec->SetSetByCallerMagnitude(SETBYCALLER_TAG_STRENGTH, FGItemDataTable->Strength);

			FGAbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*Spec);
		}
	}
}

//FGCharacterStat.h
//FGItemDataTable.h
//FGAttributeSet.h
//FGGameplayTag.h
//FGEquipmentItemTooltipWidget.h
//FGStatWidget.h
// /All/Game/Blueprints/AbilitySystem/GE/SetByCaller/BPGE_SetByCaller_ItemAbility 설정
//연관
void UFGInventoryComponent::InventoryItemAbility(FName InItemID, AFGCharacterPlayer* InFGCharacterPlayer, bool bBuy)
{
	FString ItemIDString = InItemID.ToString();

	if (ItemIDString.StartsWith("E"))
	{
		FFGItemDataTable* FGItemDataTable = VDataTable->FindRow<FFGItemDataTable>(InItemID, TEXT("ItemData"));
		if (!FGItemDataTable) return;

		TSoftObjectPtr<UFGItemDataAsset> AssetRef(FGItemDataTable->DataAssetPath);
		UFGItemDataAsset* FGItemDataAsset = AssetRef.LoadSynchronous();
		if (!IsValid(FGItemDataAsset)) return;

		UFGAbilitySystemComponent* FGAbilitySystemComponent = InFGCharacterPlayer->GetFGAbilitySystemComponent();
		if (!IsValid(FGAbilitySystemComponent)) return;

		FGameplayEffectContextHandle ContextHandle = FGAbilitySystemComponent->MakeEffectContext();

		TSubclassOf<UGameplayEffect> GameplayEffect = FGItemDataAsset->ItemEffect;
		if (!IsValid(GameplayEffect)) return;

		FGameplayEffectSpecHandle SpecHandle = FGAbilitySystemComponent->MakeOutgoingSpec(GameplayEffect, 1.f, ContextHandle);
		if (!SpecHandle.IsValid()) return;

		FGameplayEffectSpec* Spec = SpecHandle.Data.Get();
		if (!Spec) return;

		int32 BuyOrSell = bBuy ? 1 : -1;

		Spec->SetSetByCallerMagnitude(SETBYCALLER_TAG_MAXHEALTH, BuyOrSell * FGItemDataTable->Health);
		Spec->SetSetByCallerMagnitude(SETBYCALLER_TAG_HEALTHREGEN, BuyOrSell * FGItemDataTable->HealthRegen);
		Spec->SetSetByCallerMagnitude(SETBYCALLER_TAG_MAXMANA, BuyOrSell * FGItemDataTable->Mana);
		Spec->SetSetByCallerMagnitude(SETBYCALLER_TAG_MANAREGEN, BuyOrSell * FGItemDataTable->ManaRegen);
		Spec->SetSetByCallerMagnitude(SETBYCALLER_TAG_STRENGTH, BuyOrSell * FGItemDataTable->Strength);

		FGAbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*Spec);
	}
}
