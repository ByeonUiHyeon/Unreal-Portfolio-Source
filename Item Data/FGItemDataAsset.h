// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "FGItemDataAsset.generated.h"

/**
 * 
 */
UCLASS()
class FIGHTGAME_API UFGItemDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<class UTexture2D> ItemImage = nullptr;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<class UGameplayEffect> ItemEffect;
};
