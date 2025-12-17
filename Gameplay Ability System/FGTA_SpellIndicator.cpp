// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/TA/FGTA_SpellIndicator.h"

AFGTA_SpellIndicator::AFGTA_SpellIndicator()
{
    bReplicates = true;
    ShouldProduceTargetDataOnServer = true;
    //멀티플레이어+서버권한타겟팅(RTS, MOBA)에서 사용
}
