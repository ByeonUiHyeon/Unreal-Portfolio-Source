// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/GA/FGGameplayAbility.h"
#include "Character/FGCharacterBase.h"
#include "Character/FGCharacterPlayer.h"
#include "Character/FGCharacterSniper.h"
#include "AI/FGAIController.h"
#include "AbilitySystem/FGAbilitySystemComponent.h"
#include "Attribute/FGAttributeSet.h"
#include "AbilitySystem/FGGameplayEffectContext.h"
#include "AbilitySystem/AT/FGAT_WaitTargetData.h"
#include "AbilitySystem/TA/FGTA_SpellIndicator.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystem/AT/FGAT_WaitTrace.h"
#include "AbilitySystem/AT/FGAT_WaitLineTrace.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystem/GE/FGGE_SetByCallerMana.h"
#include "AbilitySystem/GE/FGGE_SetByCallerStrength.h"
#include "AbilitySystem/GE/FGGE_SetByCallerBarrier.h"
#include "AbilitySystem/GE/FGGE_SetByCallerDamage.h"
#include "AbilitySystem/GE/FGGE_SetByCallerHpMp.h"
#include "NiagaraSystem.h"
#include "Abilities/Tasks/AbilityTask_WaitInputPress.h"
#include "Abilities/Tasks/AbilityTask_WaitInputRelease.h"
#include "Actors/FGProjectile.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Player/FGPlayerController.h"

UFGGameplayAbility::UFGGameplayAbility()
{
	//지속시간 사용할 경우 ActivationOwnedTags에 태그 추가하지 않는다.
	
	//스폰이나 어빌리티 실행(TryActivateAbilitiesByTag)은 if (HasAuthority(&CurrentActivationInfo))로 서버에서 실행한다.(중복 실행 방지)

	//프로젝타일, TryActivateAbilitiesByTag 각각 3개 이상 사용되면 공용함수 만들기

	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	//EGameplayAbilityInstancingPolicy : Gameplay Ability가 어떻게 생성되고 관리될지를 결정
	//NonInstanced	: Ability가 인스턴스화되지 않고 공유(모든 액터가 같은 Ability 사용)
	//InstancedPerActor	: 각 액터마다 새로운 Ability 인스턴스를 생성
	//InstancedPerExecution	: Ability를 실행할 때마다 새로운 인스턴스를 생성
}

void UFGGameplayAbility::OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnGiveAbility(ActorInfo, Spec);

	VFGCharacterBase = ActorInfo->AvatarActor.IsValid() ? Cast<AFGCharacterBase>(ActorInfo->AvatarActor.Get()) : nullptr;
	VFGCharacterPlayer = IsValid(VFGCharacterBase) ? Cast<AFGCharacterPlayer>(VFGCharacterBase) : nullptr;
	VFGCharacterSniper = IsValid(VFGCharacterPlayer) ? Cast<AFGCharacterSniper>(VFGCharacterPlayer) : nullptr;
	VFGAIController = IsValid(VFGCharacterBase->GetController()) ? Cast<AFGAIController>(VFGCharacterBase->GetController()) : nullptr;
	VPathFollowingComponent = IsValid(VFGAIController) ? VFGAIController->GetPathFollowingComponent() : nullptr;
	VFGAbilitySystemComponent = ActorInfo->AbilitySystemComponent.IsValid() ? Cast<UFGAbilitySystemComponent>(ActorInfo->AbilitySystemComponent.Get()) : nullptr;
	VFGAttributeSet = IsValid(VFGAbilitySystemComponent) ? VFGAbilitySystemComponent->GetSet<UFGAttributeSet>() : nullptr;

	if (IsValid(VFGAttributeSet))
	{
		VFGAttributeSet->OnBarrierDestroyedDelegate.RemoveAll(this);
		VFGAttributeSet->OnBarrierDestroyedDelegate.AddDynamic(this, &UFGGameplayAbility::OnBarrierDestroyedFunction);
	}

	ContextHandle = IsValid(VFGAbilitySystemComponent) ? VFGAbilitySystemComponent->MakeEffectContext() : FGameplayEffectContextHandle();
	VFGGameplayEffectContext = ContextHandle.IsValid() ? static_cast<FFGGameplayEffectContext*>(ContextHandle.Get()) : nullptr;
	//FGGameplayEffectContext를 상속 받은 커스텀 FGGameplayEffectContext 제작
	//GA에서 GC에 여러 값들을 전달하기 위해 만듬
	//비트마스크 사용
	//FGAbilitySystemComponent에서 MakeEffectContext를 오버라이드 후 Super 사용 안하고 새로 만든다.
}

void UFGGameplayAbility::OnRemoveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnRemoveAbility(ActorInfo, Spec);

	VFGCharacterBase = nullptr;
	VFGCharacterPlayer = nullptr;
	VFGCharacterSniper = nullptr;
	VFGAIController = nullptr;
	VPathFollowingComponent = nullptr;
	VFGAbilitySystemComponent = nullptr;

	if (IsValid(VFGAttributeSet))
	{
		VFGAttributeSet->OnBarrierDestroyedDelegate.RemoveAll(this);
	}

	VFGAttributeSet = nullptr;
	ContextHandle = FGameplayEffectContextHandle();
	VFGGameplayEffectContext = nullptr;
}

//시작 전 체크
bool UFGGameplayAbility::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, OUT FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags)) return false;

	if (!IsValid(VFGCharacterBase)) return false;
	if (!IsValid(VFGAbilitySystemComponent)) return false;
	if (!IsValid(VFGAttributeSet)) return false;
	if (!ContextHandle.IsValid()) return false;
	if (!VFGGameplayEffectContext) return false;

	//VFGAIController는 CanActivateAbility에서 유효성 검사 안된다.
	//if (!IsValid(VFGCharacterPlayer)) return false;
	//몹일 경우 사용안하므로 넣지 않는다.
	//if (!IsValid(VFGCharacterSniper)) return false;
	//대부분 VFGCharacterSniper가 아닌경우가 많아서 넣지 않는다.
	//if (!IsValid(VFGAIController)) return false;
	//if (!IsValid(VPathFollowingComponent)) return false;

	return CanUseSkillMana();
}

//종료
void UFGGameplayAbility::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	AlreadyHitActors.Empty();

	bEventReceived = false;
	MontageName = NAME_None;
	TraceName = NAME_None;

	if (HasAuthority(&CurrentActivationInfo))
	{
		if (IsValid(VFGProjectile))
		{
			VFGProjectile->GetOnProjectileHitActor().RemoveAll(this);
			VFGProjectile->GetOnProjectileHitActorFail().RemoveAll(this);
			VFGProjectile = nullptr;
		}
	}

	if (bUseInputRelease)
	{
		bUseInputRelease = false;

		GetWorld()->GetTimerManager().ClearTimer(UseInputReleaseTimer);
	}

	if (!bInstantCast)
	{
		GetWorld()->GetTimerManager().ClearTimer(CharacterPlayerRotationTimer);
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

//사용 가능 마나
bool UFGGameplayAbility::CanUseSkillMana(bool InbUseParameter, float InSkillMana) const
{
	if (!InbUseParameter)
	{
		if (!bSkillMana) return true;
	}

	float CurrentMana = VFGAbilitySystemComponent->GetNumericAttribute(UFGAttributeSet::GetManaAttribute());
	float ManaCost = InbUseParameter ? FMath::Abs(InSkillMana) : FMath::Abs(SkillMana);

	return CurrentMana >= ManaCost;
}

//즉시 시전 or 스킬 범위
void UFGGameplayAbility::SpellIndicatorFunction(const FName& TaskInstanceName, const TArray<FVector2D>& InFVector2D)
{
	if (bInstantCast)
	{
		OnTargetDataReady(FGameplayAbilityTargetDataHandle());
	}
	else
	{
		if (!IsValid(VFGCharacterPlayer))
		{
			EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);

			return;
		}

		AFGPlayerController* CFGPlayerController = Cast<AFGPlayerController>(VFGCharacterBase->GetController());
		if (IsValid(CFGPlayerController))
		{
			OnTargetDataReady(FGameplayAbilityTargetDataHandle());
		}
		else
		{
			//스킬 범위
			//EGameplayTargetingConfirmation::Instant : 추가 입력 없이 즉시 타겟 데이터 생성
			//EGameplayTargetingConfirmation::UserConfirmed : 유저 입력(버튼, 마우스 클릭 등)으로 확정
			//EGameplayTargetingConfirmation::Custom/EGameplayTargetingConfirmation::CustomMulti : 어빌리티 또는 특별 로직에서 직접 확정
			UFGAT_WaitTargetData* WaitTargetDataFunction = UFGAT_WaitTargetData::WaitTargetDataFunction(this, TaskInstanceName, EGameplayTargetingConfirmation::UserConfirmed, AFGTA_SpellIndicator::StaticClass());
			if (IsValid(WaitTargetDataFunction))
			{
				WaitTargetDataFunction->ValidData.AddDynamic(this, &UFGGameplayAbility::OnTargetDataReady);
				WaitTargetDataFunction->Cancelled.AddDynamic(this, &UFGGameplayAbility::OnTargetDataCancelled);
				WaitTargetDataFunction->ReadyForActivation();

				AddTagFunction(CHARACTER_STATE_ISSPELLINDICATOR);
				//태그 추가

				//스킬 범위 모양 설정
				VFGCharacterPlayer->SetDynamicMeshComponentDefaultLocation();
				//SetDynamicMeshComponentLocation 사용 시 다시 원점으로 되돌려 놓는다.
				VFGCharacterPlayer->SetVertices2D(InFVector2D);
				VFGCharacterPlayer->SetGameplayAbilityTargetActor(WaitTargetDataFunction->GetTargetActor());

				//제한 범위 표시
				if (bUseLocation)
				{
					VFGCharacterPlayer->SetbAroundSpellIndicator(true);
					VFGCharacterPlayer->SetAroundRadius(SpellIndicatorClampRadius);
				}

				//스킬 범위 마우스 방향으로 회전
				GetWorld()->GetTimerManager().SetTimer(CharacterPlayerRotationTimer, FTimerDelegate::CreateLambda([&]()
					{
						VFGCharacterPlayer->SetFGCharacterPlayerRotation();

						//마우스 위치 사용시
						if (bUseLocation)
						{
							VFGCharacterPlayer->SetDynamicMeshComponentLocation();
						}
					}
				), 0.05f, true);
			}
		}
	}
}

//SpellIndicator 확인
void UFGGameplayAbility::OnTargetDataReady(const FGameplayAbilityTargetDataHandle& Data)
{
	GetWorld()->GetTimerManager().ClearTimer(CharacterPlayerRotationTimer);
	//캐릭 회전 타이머 제거

	RemoveTagFunction(CHARACTER_STATE_ISSPELLINDICATOR);
	//태그 제거

	StopAndRotationSetting();
	//캐릭터 마우스 또는 화면 방향 회전
}

//캐릭터 마우스 또는 화면 방향 회전
void UFGGameplayAbility::StopAndRotationSetting(bool InbUseStop, bool InbUseRotation, bool InbUseInputRelease)
{
	//스킬 사용 시 멈춤
	if (InbUseStop)
	{
		VFGCharacterBase->GetCharacterMovement()->StopMovementImmediately();
	}

	if (!InbUseRotation) return;

	AFGPlayerController* CFGPlayerController = Cast<AFGPlayerController>(VFGCharacterBase->GetController());
	if (IsValid(CFGPlayerController))
	{
		//화면 방향으로 회전
		FRotator LookAtRotator(0.f, VFGCharacterBase->GetControlRotation().Yaw, 0.f);
		VFGCharacterBase->SetActorRotation(LookAtRotator);
	}
	else
	{
		if (InbUseInputRelease)
		{
			bUseInputRelease = true;

			GetWorld()->GetTimerManager().SetTimer(UseInputReleaseTimer, FTimerDelegate::CreateLambda([&]()
				{
					VFGCharacterBase->SetFGCharacterPlayerRotation();
				}
			), 0.05f, true);
		}

		VFGCharacterBase->SetFGCharacterPlayerRotation();
		//마우스 방향으로 회전

		MouseCursorLocationValue = VFGCharacterBase->GetMouseCursorLocation();
		//마우스 위치 저장
	}
}

//스킬 사용 가능 범위 안인지 확인
//SpellIndicator Location 사용시 사용한다.
//스킬 사용 가능 범위 밖에서 스킬 사용시에
//스킬 사용 가능 범위 안까지 움직이고 도달하면 스킬 사용
//bUseLocation 사용시 OnAIMoveRequestFinished 오버라이드 후 if (Result.IsSuccess())에 내용 추가
bool UFGGameplayAbility::CanUseSkillWithinRange(const FName& InTraceRadiusName)
{
	FVector CharacterLocation = VFGCharacterBase->GetActorLocation();

	FVector Direction = MouseCursorLocationValue - CharacterLocation;
	float Distance = Direction.Length();
	if (Distance > SpellIndicatorClampRadius)
	{
		FVector MoveTarget;

		if (FSkillTraceStruct* FIndSkillTraceStruct = SkillTraceMap.Find(InTraceRadiusName))
		{
			float TraceRadiusValue = FIndSkillTraceStruct->TraceRadius;
			MoveTarget = MouseCursorLocationValue - Direction.GetSafeNormal() * (SpellIndicatorClampRadius - TraceRadiusValue);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("InTraceRadiusName is not found in CanUseSkillWithinRange of %s"), *GetNameSafe(this));

			EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);

			return false;
			//필요 없는 값
		}

		FAIMoveRequest MoveRequest;
		MoveRequest.SetGoalLocation(MoveTarget);
		MoveRequest.SetAcceptanceRadius(5.0f);

		if (IsValid(VFGAIController))
		{
			VFGAIController->StopAI();

			FPathFollowingRequestResult Result = VFGAIController->MoveTo(MoveRequest);
			if (Result.Code == EPathFollowingRequestResult::RequestSuccessful)
			{
				if (IsValid(VPathFollowingComponent))
				{
					VPathFollowingComponent->OnRequestFinished.RemoveAll(this);
					VPathFollowingComponent->OnRequestFinished.AddUObject(this, &UFGGameplayAbility::OnAIMoveRequestFinished);
				}
			}
		}

		return false;
	}

	return true;
}

//AI 목표까지 이동했는지 결과 확인
void UFGGameplayAbility::OnAIMoveRequestFinished(FAIRequestID RequestID, const FPathFollowingResult& Result)
{
	if (IsValid(VPathFollowingComponent))
	{
		VPathFollowingComponent->OnRequestFinished.RemoveAll(this);
	}

	if (!Result.IsSuccess())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
	}
}

//SpellIndicator 취소
void UFGGameplayAbility::OnTargetDataCancelled(const FGameplayAbilityTargetDataHandle& Data)
{
	GetWorld()->GetTimerManager().ClearTimer(CharacterPlayerRotationTimer);
	//캐릭 회전 타이머 제거

	RemoveTagFunction(CHARACTER_STATE_ISSPELLINDICATOR);
	//태그 제거

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

//몽타주 실행
void UFGGameplayAbility::PlayMontageAndWaitFunction(const FName& TaskInstanceName)
{
	if (!bSkillMontageMap) return;

	bEventReceived = false;
	MontageName = TaskInstanceName;

	if (const FSkillMontageStruct* FindSkillMontageStruct = SkillMontageMap.Find(TaskInstanceName))
	{
		UAnimMontage* GAnimMontage = FindSkillMontageStruct->SkillMontage;
		if (IsValid(GAnimMontage))
		{
			UAbilityTask_PlayMontageAndWait* PlayMontageAndWait = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, TaskInstanceName, GAnimMontage);
			if (IsValid(PlayMontageAndWait))
			{
				PlayMontageAndWait->OnInterrupted.AddDynamic(this, &UFGGameplayAbility::OnPlayMontageInterrupted);
				PlayMontageAndWait->OnCompleted.AddDynamic(this, &UFGGameplayAbility::OnPlayMontageCompleted);
				PlayMontageAndWait->ReadyForActivation();
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("SkillMontage is not valid in PlayMontageAndWaitFunction of %s"), *GetNameSafe(this));

			EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);

			return;
		}

		if (!FindSkillMontageStruct->bNotifyTag) return;
		
		FGameplayTag GTag = FindSkillMontageStruct->NotifyTag;
		if (GTag.IsValid())
		{
			UAbilityTask_WaitGameplayEvent* NotifyWaitGameplayEvent = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, GTag, nullptr, true);
			if (IsValid(NotifyWaitGameplayEvent))
			{
				NotifyWaitGameplayEvent->EventReceived.AddDynamic(this, &UFGGameplayAbility::OnEventReceived);
				NotifyWaitGameplayEvent->ReadyForActivation();
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("NotifyTag is not valid in PlayMontageAndWaitFunction of %s"), *GetNameSafe(this));

			EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("TaskInstanceName is not found in PlayMontageAndWaitFunction of %s"), *GetNameSafe(this));

		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

//몽타주 방해
//if (MontageName == TEXT("Name"))으로 분기한다.
//몽타주 연속 실행 시 이전 몽타주에 OnPlayMontageInterrupted이 실행되는데
//MontageName = TaskInstanceName;으로 인해 마지막 몽타주로 분기 하지 않게 하기 위해
//다음 실행할 몽타주 맨 앞에 노티파이 추가하고 OnPlayMontageInterruptedEventReceived에서 코드 설정
void UFGGameplayAbility::OnPlayMontageInterrupted()
{
	//OnEventReceived가 실행 되기 전에 발동되면 끝내기
	if (bEventReceived)
	{
		OnPlayMontageInterruptedEventReceived();
	}
	else
	{
		SkillDurationEarlyEnd();

		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
	}
}

void UFGGameplayAbility::OnPlayMontageInterruptedEventReceived()
{
}

//몽타주 종료
void UFGGameplayAbility::OnPlayMontageCompleted()
{
	if (bUseSkillDuration) return;
	//지속 시간 있으면 몽타주 종료 후 바로 종료 되기 때문에 설정

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

//이벤트(태그) 기다리기
void UFGGameplayAbility::WaitGameplayEventFunction(const FName& InName)
{
	if (const FGameplayTag* FindGameplayTag = AddGameplayTagMap.Find(InName))
	{
		if (FindGameplayTag->IsValid())
		{
			UAbilityTask_WaitGameplayEvent* NotifyWaitGameplayEvent = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, *FindGameplayTag, nullptr, true);
			if (IsValid(NotifyWaitGameplayEvent))
			{
				NotifyWaitGameplayEvent->EventReceived.AddDynamic(this, &UFGGameplayAbility::OnEventReceived);
				NotifyWaitGameplayEvent->ReadyForActivation();
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("FindGameplayTag is not valid in WaitGameplayEventFunction of %s"), *GetNameSafe(this));

			EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);

			return;
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("InName is not found in WaitGameplayEventFunction of %s"), *GetNameSafe(this));

		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}


//이벤트(태그) 실행
void UFGGameplayAbility::OnEventReceived(FGameplayEventData Payload)
{
	bEventReceived = true;
	//OnEventReceived가 실행되면 OnPlayMontageInterrupted 발동 안되게 하기
}

//기본 트레이스
void UFGGameplayAbility::WaitTraceFunction(const FVector& InStart, const FVector& InEnd, bool InbLineTrace, float InTraceRadius, bool InbDebugDraw, bool InSingle, float InSplitSize, float InLifeTime)
{
	if (InbLineTrace)
	{
		UFGAT_WaitLineTrace* WaitLineTrace = UFGAT_WaitLineTrace::WaitLineTrace(this, InStart, InEnd, InbDebugDraw, InSingle, InSplitSize, InLifeTime);
		if (IsValid(WaitLineTrace))
		{
			WaitLineTrace->OnComplete.AddDynamic(this, &UFGGameplayAbility::OnTraceResultFunction);
			WaitLineTrace->ReadyForActivation();
		}
	}
	else
	{
		UFGAT_WaitTrace* WaitTrace = UFGAT_WaitTrace::WaitTrace(this, InStart, InEnd, InTraceRadius, InbDebugDraw, InSingle, InSplitSize, InLifeTime);
		if (IsValid(WaitTrace))
		{
			WaitTrace->OnComplete.AddDynamic(this, &UFGGameplayAbility::OnTraceResultFunction);
			WaitTrace->ReadyForActivation();
		}
	}
}

//네임 사용 트레이스
void UFGGameplayAbility::UseWaitTraceByName(const FName& InName, const FVector& InStart, const FVector& InEnd, bool InbLineTrace, bool InSingle, float InSplitSize, float InLifeTime)
{
	if (!bSkillTraceMap) return;

	TraceName = InName;

	if (FSkillTraceStruct* FIndSkillTraceStruct = SkillTraceMap.Find(InName))
	{
		bool bDrawDebugValue = FIndSkillTraceStruct->bDrawDebug;

		if (InbLineTrace)
		{
			UFGAT_WaitLineTrace* WaitLineTrace = UFGAT_WaitLineTrace::WaitLineTrace(this, InStart, InEnd, bDrawDebugValue, InSingle, InSplitSize, InLifeTime);
			if (IsValid(WaitLineTrace))
			{
				WaitLineTrace->OnComplete.AddDynamic(this, &UFGGameplayAbility::OnTraceResultFunction);
				WaitLineTrace->ReadyForActivation();
			}
		}
		else
		{
			float TraceRadiusValue = FIndSkillTraceStruct->TraceRadius;

			UFGAT_WaitTrace* WaitTrace = UFGAT_WaitTrace::WaitTrace(this, InStart, InEnd, TraceRadiusValue, bDrawDebugValue, InSingle, InSplitSize, InLifeTime);
			if (IsValid(WaitTrace))
			{
				WaitTrace->OnComplete.AddDynamic(this, &UFGGameplayAbility::OnTraceResultFunction);
				WaitTrace->ReadyForActivation();
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("InName is not found in UseWaitTraceByName of %s"), *GetNameSafe(this));

		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

//트레이스 결과
void UFGGameplayAbility::OnTraceResultFunction(const FGameplayAbilityTargetDataHandle& TargetDataHandle)
{
	int32 TargetDataHandleNum = TargetDataHandle.Data.Num();
	if (TargetDataHandleNum > 0)
	{
		FrontTraceResultFunction();

		//데미지 및 효과
		for (int32 i = 0; i < TargetDataHandleNum; ++i)
		{
			if (UAbilitySystemBlueprintLibrary::TargetDataHasHitResult(TargetDataHandle, i))
			{
				FHitResult HitResult = UAbilitySystemBlueprintLibrary::GetHitResultFromTargetData(TargetDataHandle, i);
				if (!HitResult.bBlockingHit) continue;

				AActor* HitActor = HitResult.GetActor();
				if (!IsValid(HitActor)) continue;

				MiddleTraceResultFunction(HitResult, HitActor);
			}
		}

		BackTraceResultFunction();
	}
}

void UFGGameplayAbility::FrontTraceResultFunction()
{
}

void UFGGameplayAbility::MiddleTraceResultFunction(const FHitResult& InHitResult, AActor* InActor)
{
}

void UFGGameplayAbility::BackTraceResultFunction()
{
}

//InputPress
//한번만 적용됨
//계속 사용할라면 GA의 InputPressed 사용하거나 다시 InputPressFunction 설정
void UFGGameplayAbility::InputPressFunction()
{
	UAbilityTask_WaitInputPress* WaitForPress = UAbilityTask_WaitInputPress::WaitInputPress(this);
	if (IsValid(WaitForPress))
	{
		WaitForPress->OnPress.AddDynamic(this, &UFGGameplayAbility::OnInputPress);
		WaitForPress->ReadyForActivation();
	}
}

void UFGGameplayAbility::OnInputPress(float TimeWaited)
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

//InputRelease
//한번만 적용됨
//계속 사용할라면 GA의 InputReleased 사용하거나 다시 InputReleaseFunction 설정
void UFGGameplayAbility::InputReleaseFunction()
{
	UAbilityTask_WaitInputRelease* WaitForRelease = UAbilityTask_WaitInputRelease::WaitInputRelease(this);
	if (IsValid(WaitForRelease))
	{
		WaitForRelease->OnRelease.AddDynamic(this, &UFGGameplayAbility::OnInputRelease);
		WaitForRelease->ReadyForActivation();
	}
}

void UFGGameplayAbility::OnInputRelease(float TimeHeld)
{
	SkillDurationEarlyEnd();

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

//태그 추가
//LooseGameplayTag는 ActivationOwnedTags와는 다른 태그다(ActivationOwnedTags는 RemoveLooseGameplayTag로 지울수 없음)
void UFGGameplayAbility::AddTagFunction(const FGameplayTag& InGameplayTag)
{
	if (!InGameplayTag.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("InGameplayTag is not valid in AddTagFunction of %s"), *GetNameSafe(this));

		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);

		return;
	}

	if (!VFGAbilitySystemComponent->HasMatchingGameplayTag(InGameplayTag))
	{
		VFGAbilitySystemComponent->AddLooseGameplayTag(InGameplayTag);
		VFGAbilitySystemComponent->AddReplicatedLooseGameplayTag(InGameplayTag);
	}
}

//네임으로 태그 추가
void UFGGameplayAbility::AddTagByName(const FName& InName)
{
	if (!bAddGameplayTagMap) return;

	if (const FGameplayTag* FindGameplayTag = AddGameplayTagMap.Find(InName))
	{
		if (FindGameplayTag->IsValid())
		{
			AddTagFunction(*FindGameplayTag);

			return;
		}
	}

	UE_LOG(LogTemp, Error, TEXT("InName is not found in AddTagByName of %s"), *GetNameSafe(this));

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

//태그 제거
void UFGGameplayAbility::RemoveTagFunction(const FGameplayTag& InGameplayTag)
{
	if (!InGameplayTag.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("InGameplayTag is not valid in RemoveTagFunction of %s"), *GetNameSafe(this));

		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);

		return;
	}

	if (VFGAbilitySystemComponent->HasMatchingGameplayTag(InGameplayTag))
	{
		VFGAbilitySystemComponent->RemoveLooseGameplayTag(InGameplayTag);
		VFGAbilitySystemComponent->RemoveReplicatedLooseGameplayTag(InGameplayTag);
	}
}

//네임으로 태그 제거
void UFGGameplayAbility::RemoveTagByName(const FName& InName)
{
	if (const FGameplayTag* FindGameplayTag = AddGameplayTagMap.Find(InName))
	{
		if (FindGameplayTag->IsValid())
		{
			RemoveTagFunction(*FindGameplayTag);

			return;
		}
	}

	UE_LOG(LogTemp, Error, TEXT("InName is not found in RemoveTagByName of %s"), *GetNameSafe(this));

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

//네임으로 저장한 태그 찾기
FGameplayTag UFGGameplayAbility::GetTagByName(const FName& InName) const
{
	if (const FGameplayTag* FindGameplayTag = AddGameplayTagMap.Find(InName))
	{
		if (FindGameplayTag->IsValid())
		{
			return *FindGameplayTag;
		}
	}

	UE_LOG(LogTemp, Error, TEXT("InName is not found in GetTagByName of %s"), *GetNameSafe(this));

	return FGameplayTag();
}

//네임으로 큐 태그 찾기
FGameplayTag UFGGameplayAbility::GetCueTagByName(const FName& InName) const
{
	if (const FSkillEffectStruct* FindSkillEffectStruct = SkillEffectMap.Find(InName))
	{
		FGameplayTag GGameplayTag = FindSkillEffectStruct->GameplayCueTag;
		if (GGameplayTag.IsValid())
		{
			return GGameplayTag;
		}
	}

	UE_LOG(LogTemp, Error, TEXT("InName is not found in GetCueTagByName of %s"), *GetNameSafe(this));

	return FGameplayTag();
}

//태그로 Ability 실행
bool UFGGameplayAbility::HasTagsTryAbilities(const FGameplayTagContainer& InHasAllMatchingGameplayTags, const FGameplayTag& InTryActivateAbilitiesByTag)
{
	if (VFGAbilitySystemComponent->HasAllMatchingGameplayTags(InHasAllMatchingGameplayTags) && HasAuthority(&CurrentActivationInfo))
	{
		VFGAbilitySystemComponent->TryActivateAbilitiesByTag(FGameplayTagContainer(InTryActivateAbilitiesByTag));

		return true;
	}

	return false;
}

//태그로 Ability 취소
void UFGGameplayAbility::HasTagCancelAbilities(const FGameplayTag& InHasMatchingGameplayTag, const FGameplayTag& InTryActivateAbilitiesByTag)
{
	if (VFGAbilitySystemComponent->HasMatchingGameplayTag(InHasMatchingGameplayTag) && HasAuthority(&CurrentActivationInfo))
	{
		VFGAbilitySystemComponent->CancelAbilitiesWithTags(FGameplayTagContainer(InTryActivateAbilitiesByTag));
	}
}

//마나
void UFGGameplayAbility::SetByCallerManaFunction(bool InbUseParameter, float InSkillMana)
{
	if (!InbUseParameter)
	{
		if (!bSkillMana) return;
	}

	float SkillManaValue = InbUseParameter ? InSkillMana : SkillMana;

	FGameplayEffectSpecHandle ManaSpecHandle = MakeOutgoingGameplayEffectSpec(UFGGE_SetByCallerMana::StaticClass());
	if (ManaSpecHandle.IsValid())
	{
		ManaSpecHandle.Data->SetSetByCallerMagnitude(SETBYCALLER_TAG_MANA, -SkillManaValue);
		ApplyGameplayEffectSpecToOwner(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, ManaSpecHandle);
	}
}

//힘
//추가한거 제거해주기
void UFGGameplayAbility::SetByCallerStrengthFunction(bool InbBuff)
{
	if (!bSkillStrength) return;

	float SkillStrengthValue = InbBuff ? SkillStrength : -SkillStrength;

	FGameplayEffectSpecHandle StrengthSpecHandle = MakeOutgoingGameplayEffectSpec(UFGGE_SetByCallerStrength::StaticClass());
	if (StrengthSpecHandle.IsValid())
	{
		StrengthSpecHandle.Data->SetSetByCallerMagnitude(SETBYCALLER_TAG_STRENGTH, SkillStrengthValue);
		ApplyGameplayEffectSpecToOwner(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, StrengthSpecHandle);
	}
}

//베리어
//추가한거 제거해주기
void UFGGameplayAbility::SetByCallerBarrierFunction(bool InbBuff)
{
	if (!bSkillBarrier) return;

	float CurrentBarrier = VFGAbilitySystemComponent->GetNumericAttribute(UFGAttributeSet::GetBarrierAttribute());
	if (!InbBuff && CurrentBarrier <= 0.f) return;

	float SkillBarrierValue = InbBuff ? SkillBarrier : -CurrentBarrier;

	FGameplayEffectSpecHandle BarrierSpecHandle = MakeOutgoingGameplayEffectSpec(UFGGE_SetByCallerBarrier::StaticClass());
	if (BarrierSpecHandle.IsValid())
	{
		BarrierSpecHandle.Data->SetSetByCallerMagnitude(SETBYCALLER_TAG_BARRIER, SkillBarrierValue);
		ApplyGameplayEffectSpecToOwner(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, BarrierSpecHandle);
	}
}

void UFGGameplayAbility::OnBarrierDestroyedFunction()
{
	SkillDurationEarlyEnd();

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

//지속 시간
//쿨다운보다 앞에 둔다.
//SkillDurationEnd을 override 해서 내용 추가하기
//SkillDurationEarlyEnd도 설정하기
void UFGGameplayAbility::SetByCallerDurationFunction()
{
	if (!bUseSkillDuration) return;

	if (!IsValid(SetByCallerDurationGE))
	{
		UE_LOG(LogTemp, Error, TEXT("SetByCallerDurationGE is not valid in SetByCallerDurationFunction of %s"), *GetNameSafe(this));

		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);

		return;
	}

	FGameplayEffectSpecHandle DurationSpecHandle = MakeOutgoingGameplayEffectSpec(SetByCallerDurationGE);
	if (DurationSpecHandle.IsValid())
	{
		DurationSpecHandle.Data->SetSetByCallerMagnitude(SETBYCALLER_TAG_DURATION, SkillDuration);
	}

	FActiveGameplayEffectHandle DurationEffectHandle = ApplyGameplayEffectSpecToOwner(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, DurationSpecHandle);

	if (!DurationActivationOwnedTag.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("DurationActivationOwnedTag is not valid in SetByCallerDurationFunction of %s"), *GetNameSafe(this));

		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);

		return;
	}

	VFGCharacterBase->AddTagDurationEffectHandleMap(DurationActivationOwnedTag, DurationEffectHandle);

	GetWorld()->GetTimerManager().SetTimer(SkillDurationTimer, this, &UFGGameplayAbility::SkillDurationEnd, SkillDuration, false);
	//지속 시간 타이머
}

//지속 시간 종료
//SkillDurationEnd가 동작하기 전에 EndAbility가 실행되면 동작하지 않는다.
void UFGGameplayAbility::SkillDurationEnd()
{
	VFGCharacterBase->RemoveTagDurationEffectHandleMap(DurationActivationOwnedTag);

	//바로 EndAbility 실행하면 자식에서 함수가 실행 안돼므로 딜레이 설정
	GetWorld()->GetTimerManager().SetTimer(SkillDurationEndAbilitytimer, FTimerDelegate::CreateLambda([&]()
		{
			EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);

			GetWorld()->GetTimerManager().ClearTimer(SkillDurationEndAbilitytimer);
		}
	), 0.5f, false);

	GetWorld()->GetTimerManager().ClearTimer(SkillDurationTimer);
}

//스킬 발동으로 지속 시간 일찍 제거
void UFGGameplayAbility::SkillDurationEarlyEnd()
{
	if (!bUseSkillDuration) return;

	//지속 시간 종료
	//FActiveGameplayEffectHandle DurationEffectHandle = VFGCharacterBase->FindTagDurationEffectHandleMap(DurationActivationOwnedTag);
	//if (DurationEffectHandle.IsValid())
	//{
	//	VFGAbilitySystemComponent->RemoveActiveGameplayEffect(DurationEffectHandle);
	//}

	if (const FActiveGameplayEffectHandle* DurationEffectHandle = VFGCharacterBase->FindTagDurationEffectHandleMap(DurationActivationOwnedTag))
	{
		if (DurationEffectHandle->IsValid())
		{
			VFGAbilitySystemComponent->RemoveActiveGameplayEffect(*DurationEffectHandle);
		}
	}

	VFGCharacterBase->RemoveTagDurationEffectHandleMap(DurationActivationOwnedTag);

	GetWorld()->GetTimerManager().ClearTimer(SkillDurationTimer);
}

//쿨다운
//에디터의 CooldownGameplayEffectClass에 클래스 추가하기
void UFGGameplayAbility::SetByCallerCooldownFunction(bool InbUseParameter, float InSkillCooldown)
{
	if (!InbUseParameter)
	{
		if (!bUseSkillCooldown) return;
	}

	if (!IsValid(SetByCallerCooldownGE))
	{
		UE_LOG(LogTemp, Error, TEXT("SetByCallerCooldownGE is not valid in SetByCallerCooldownFunction of %s"), *GetNameSafe(this));

		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);

		return;
	}

	float SkillCooldownValue = InbUseParameter ? InSkillCooldown : SkillCooldown;

	FGameplayEffectSpecHandle CooldownSpecHandle = MakeOutgoingGameplayEffectSpec(SetByCallerCooldownGE);
	if (CooldownSpecHandle.IsValid())
	{
		CooldownSpecHandle.Data->SetSetByCallerMagnitude(SETBYCALLER_TAG_COOLDWON, SkillCooldownValue);
		ApplyGameplayEffectSpecToOwner(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, CooldownSpecHandle);
	}
}

//데미지
//TargetASC를 사용해서 데미지 전달하거면 FGameplayAbilityTargetDataHandle()를 집어 넣는다.
void UFGGameplayAbility::SetByCallerDamageFunction(AActor* InHitActor, const FHitResult& InHitResult, bool InbUseHeadShot, bool InbUseCue, bool InbUseParameter, float InSkillDamage, const FGameplayTag& GameplayCueTag)
{
	if (!InbUseParameter)
	{
		if (!bSkillDamage) return;
	}

	FGameplayEffectSpecHandle DamageSpecHandle = MakeOutgoingGameplayEffectSpec(UFGGE_SetByCallerDamage::StaticClass());
	if (DamageSpecHandle.IsValid())
	{
		if (InbUseHeadShot && InHitResult.BoneName.ToString().Equals(TEXT("Head"), ESearchCase::IgnoreCase))
		{
			DamageSpecHandle.Data->SetSetByCallerMagnitude(SETBYCALLER_TAG_DAMAGE, 99999.f);
		}
		else
		{
			float SkillDamageValue = InbUseParameter ? InSkillDamage : SkillDamage;
			float StrengthValue = VFGAbilitySystemComponent->GetNumericAttribute(UFGAttributeSet::GetStrengthAttribute());
			float TotalDamage = SkillDamageValue + StrengthValue;

			DamageSpecHandle.Data->SetSetByCallerMagnitude(SETBYCALLER_TAG_DAMAGE, TotalDamage);
		}
	}

	if (!IsValid(InHitActor))
	{
		UE_LOG(LogTemp, Error, TEXT("InHitActor is not valid in SetByCallerDamageFunction of %s"), *GetNameSafe(this));

		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);

		return;
	}

	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(InHitActor);
	if (IsValid(TargetASC))
	{
		VFGAbilitySystemComponent->ApplyGameplayEffectSpecToTarget(*DamageSpecHandle.Data, TargetASC);
		//UAbilitySystemBlueprintLibrary::TargetDataHasHitResult(TargetDataHandle, 0);로 데미지 전달하지 않는다.
		//TargetDataHasHitResult는 데미지 전달을 한명이든 여러명이든 한번에 전달해서
		//TargetDataHandle.Data.Num()로 데미지 순환 할 경우 중복 데미지 전달된다.
		//UAbilitySystemBlueprintLibrary::TargetDataHasHitResult(TargetDataHandle, i); 및 VFGAbilitySystemComponent->ApplyGameplayEffectSpecToTarget로 전달해야 데미지 뿐만 아니라 다른 효과도 여러명에게 전달 가능하다.
		//ApplyGameplayEffectSpecToTarget(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, DamageSpecHandle, TargetDataHandle);는 사용하지 않는다.

		//몽타주, 히트 이펙트
		//다른 이펙트 사용하고 싶을 경우 FGGC_AttackHitEffect를 부모로 블루프린트 생성해서 새로운 이펙트 및 태그 설정
		if (InbUseCue)
		{
			FGameplayEffectContextHandle CueContextHandle = UAbilitySystemBlueprintLibrary::GetEffectContext(DamageSpecHandle);
			if (CueContextHandle.IsValid() && InHitResult.bBlockingHit)
			{
				CueContextHandle.AddHitResult(InHitResult);
			}

			FGameplayCueParameters CueParam;
			CueParam.EffectContext = CueContextHandle;

			if (!GameplayCueTag.IsValid())
			{
				UE_LOG(LogTemp, Error, TEXT("GameplayCueTag is not valid in SetByCallerDamageFunction of %s"), *GetNameSafe(this));

				EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);

				return;
			}

			TargetASC->ExecuteGameplayCue(GameplayCueTag, CueParam);
		}
	}
}

//체력, 마력 변화
void UFGGameplayAbility::SetByCallerHpMpFunction(bool InbUseParameter, float InHp, float InMp)
{
	if (!InbUseParameter)
	{
		if (!bHpMp) return;
	}

	FGameplayEffectSpecHandle HpMpSpecHandle = MakeOutgoingGameplayEffectSpec(UFGGE_SetByCallerHpMp::StaticClass());
	if (HpMpSpecHandle.IsValid())
	{
		float HpValue = InbUseParameter ? InbUseParameter : HealthValue;
		if (HpValue != 0)
		{
			HpMpSpecHandle.Data->SetSetByCallerMagnitude(SETBYCALLER_TAG_HEALTH, HpValue);
		}

		float MpValue = InbUseParameter ? InbUseParameter : ManaValue;
		if (MpValue != 0)
		{
			HpMpSpecHandle.Data->SetSetByCallerMagnitude(SETBYCALLER_TAG_MANA, MpValue);
		}

		ApplyGameplayEffectSpecToOwner(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, HpMpSpecHandle);
	}
}

//타겟 움직임 제한
void UFGGameplayAbility::TargetDisableMovement(AActor* InHitActor)
{
	if (!bDisableMovementTime) return;

	if (!IsValid(InHitActor))
	{
		UE_LOG(LogTemp, Error, TEXT("InHitActor is not valid in TargetDisableMovement of %s"), *GetNameSafe(this));

		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);

		return;
	}

	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(InHitActor);
	if (IsValid(TargetASC) && !TargetASC->HasMatchingGameplayTag(CHARACTER_STATE_DISABLEMOVEMENT))
	{
		TargetASC->AddLooseGameplayTag(CHARACTER_STATE_DISABLEMOVEMENT);
		TargetASC->AddReplicatedLooseGameplayTag(CHARACTER_STATE_DISABLEMOVEMENT);

		TWeakObjectPtr<UFGGameplayAbility> WeakThis(this);
		FTimerHandle LocalDisableMovementTimer;
		GetWorld()->GetTimerManager().SetTimer(LocalDisableMovementTimer, FTimerDelegate::CreateLambda([WeakThis, TargetASC]()
			{
				if (!WeakThis.IsValid()) return;

				if (TargetASC->HasMatchingGameplayTag(CHARACTER_STATE_DISABLEMOVEMENT))
				{
					TargetASC->RemoveLooseGameplayTag(CHARACTER_STATE_DISABLEMOVEMENT);
					TargetASC->RemoveReplicatedLooseGameplayTag(CHARACTER_STATE_DISABLEMOVEMENT);
				}
			}
		), DisableMovementTime, false);
	}
}

//밀어내기
void UFGGameplayAbility::PushCharacter(AActor* InHitActor)
{
	if (!bPushStrength) return;

	if (!IsValid(InHitActor))
	{
		UE_LOG(LogTemp, Error, TEXT("InHitActor is not valid in PushCharacter of %s"), *GetNameSafe(this));

		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);

		return;
	}

	AFGCharacterBase* TargetFGCharacterBase = Cast<AFGCharacterBase>(InHitActor);
	if (TargetFGCharacterBase)
	{
		FVector Direction = (TargetFGCharacterBase->GetActorLocation() - VFGCharacterBase->GetActorLocation()).GetSafeNormal();
		TargetFGCharacterBase->LaunchCharacter(Direction * PushStrength, true, true);
	}
}

//사운드 Cue
//AddGameplayCue 사용 시 RemoveGameplayCue로 제거
void UFGGameplayAbility::SoundBaseGameplayCue(const FName & InName, bool InbUseAdd, float InStartTime, const FVector & InLocation, const FRotator & InRotation, float InVolumeMultiplier, float InPitchMultiplier)
{
	if (!bSkillEffectMap) return;

	if (const FSkillEffectStruct* FindSkillEffectStruct = SkillEffectMap.Find(InName))
	{
		FGameplayCueParameters SoundBaseCueParameters;

		USoundBase* GSoundBase = FindSkillEffectStruct->SkillSoundBase;
		if (IsValid(GSoundBase))
		{
			VFGGameplayEffectContext->SetSoundBaseLocation(InLocation);
			VFGGameplayEffectContext->SetSoundBaseRotation(InRotation);
			VFGGameplayEffectContext->SetSoundBaseVolumeMultiplier(InVolumeMultiplier);
			VFGGameplayEffectContext->SetSoundBasePitchMultiplier(InPitchMultiplier);
			VFGGameplayEffectContext->SetSoundBaseStartTime(InStartTime);

			SoundBaseCueParameters.SourceObject = GSoundBase;
			SoundBaseCueParameters.EffectContext = ContextHandle;
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("GSoundBase is not valid in SoundBaseGameplayCue of %s"), *GetNameSafe(this));

			EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);

			return;
		}

		FGameplayTag GGameplayTag = FindSkillEffectStruct->GameplayCueTag;
		if (GGameplayTag.IsValid())
		{
			if (InbUseAdd)
			{
				VFGAbilitySystemComponent->AddGameplayCue(GGameplayTag, SoundBaseCueParameters);
			}
			else
			{
				VFGAbilitySystemComponent->ExecuteGameplayCue(GGameplayTag, SoundBaseCueParameters);
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("GGameplayTag is not valid in SoundBaseGameplayCue of %s"), *GetNameSafe(this));

			EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);

			return;
		}

		VFGGameplayEffectContext->ResetSoundValues();
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("InName is not found in SoundBaseGameplayCue of %s"), *GetNameSafe(this));

		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

//파티클 시스템 Cue
//AddGameplayCue 사용 시 RemoveGameplayCue로 제거
void UFGGameplayAbility::ParticleSystemGameplayCue(const FName& InName, bool InbUseAdd, const FVector& InLocation, const FRotator& InRotation, const FVector& InScale, bool InbUseAttached, USceneComponent* InAttachToComponent, const FName& InAttachPointName, EAttachLocation::Type InLocationType)
{
	if (!bSkillEffectMap) return;

	if (const FSkillEffectStruct* FindSkillEffectStruct = SkillEffectMap.Find(InName))
	{
		FGameplayCueParameters ParticleSystemCueParameters;

		UParticleSystem* GParticleSystem = FindSkillEffectStruct->SkillParticleSystem;
		if (IsValid(GParticleSystem))
		{
			VFGGameplayEffectContext->SetParticleSystemLocation(InLocation);
			VFGGameplayEffectContext->SetParticleSystemRotation(InRotation);
			VFGGameplayEffectContext->SetParticleSystemScale(InScale);
			VFGGameplayEffectContext->SetbParticleSystemUseSpawnEmitterAttached(InbUseAttached);
			VFGGameplayEffectContext->SetParticleSystemAttachToComponent(InAttachToComponent);
			VFGGameplayEffectContext->SetParticleSystemAttachPointName(InAttachPointName);
			VFGGameplayEffectContext->SetParticleSystemLocationType(InLocationType);

			ParticleSystemCueParameters.SourceObject = GParticleSystem;
			ParticleSystemCueParameters.EffectContext = ContextHandle;
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("GParticleSystem is not valid in ParticleSystemGameplayCue of %s"), *GetNameSafe(this));

			EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);

			return;
		}

		FGameplayTag GGameplayTag = FindSkillEffectStruct->GameplayCueTag;
		if (GGameplayTag.IsValid())
		{
			if (InbUseAdd)
			{
				VFGAbilitySystemComponent->AddGameplayCue(GGameplayTag, ParticleSystemCueParameters);
			}
			else
			{
				VFGAbilitySystemComponent->ExecuteGameplayCue(GGameplayTag, ParticleSystemCueParameters);
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("GGameplayTag is not valid in ParticleSystemGameplayCue of %s"), *GetNameSafe(this));

			EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);

			return;
		}

		VFGGameplayEffectContext->ResetSoundValues();
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("InName is not found in ParticleSystemGameplayCue of %s"), *GetNameSafe(this));

		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

//나이아가라 시스템 Cue
//AddGameplayCue 사용 시 RemoveGameplayCue로
void UFGGameplayAbility::NiagaraSystemGameplayCue(const FName& InName, bool InbUseAdd, const FVector& InLocation, const FRotator& InRotation, const FVector& InScale, bool InbUseAttached, USceneComponent* InAttachToComponent, const FName& InAttachPointName, EAttachLocation::Type InLocationType)
{
	if (!bSkillEffectMap) return;

	if (const FSkillEffectStruct* FindSkillEffectStruct = SkillEffectMap.Find(InName))
	{
		FGameplayCueParameters NiagaraSystemCueParameters;

		UNiagaraSystem* GNiagaraSystem = FindSkillEffectStruct->SkillNiagaraSystem;
		if (IsValid(GNiagaraSystem))
		{
			VFGGameplayEffectContext->SetNiagaraSystemLocation(InLocation);
			VFGGameplayEffectContext->SetNiagaraSystemRotation(InRotation);
			VFGGameplayEffectContext->SetNiagaraSystemScale(InScale);
			VFGGameplayEffectContext->SetbNiagaraSystemUseSpawnSystemAttached(InbUseAttached);
			VFGGameplayEffectContext->SetNiagaraSystemAttachToComponent(InAttachToComponent);
			VFGGameplayEffectContext->SetNiagaraSystemAttachPointName(InAttachPointName);
			VFGGameplayEffectContext->SetNiagaraSystemLocationType(InLocationType);

			NiagaraSystemCueParameters.SourceObject = GNiagaraSystem;
			NiagaraSystemCueParameters.EffectContext = ContextHandle;
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("GNiagaraSystem is not valid in NiagaraSystemGameplayCue of %s"), *GetNameSafe(this));

			EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);

			return;
		}

		FGameplayTag GGameplayTag = FindSkillEffectStruct->GameplayCueTag;
		if (GGameplayTag.IsValid())
		{
			if (InbUseAdd)
			{
				VFGAbilitySystemComponent->AddGameplayCue(GGameplayTag, NiagaraSystemCueParameters);
			}
			else
			{
				VFGAbilitySystemComponent->ExecuteGameplayCue(GGameplayTag, NiagaraSystemCueParameters);
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("GGameplayTag is not valid in NiagaraSystemGameplayCue of %s"), *GetNameSafe(this));

			EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);

			return;
		}

		VFGGameplayEffectContext->ResetSoundValues();
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("InName is not found in NiagaraSystemGameplayCue of %s"), *GetNameSafe(this));

		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

//게임플레이 큐 제거
void UFGGameplayAbility::RemoveGameplayCueFunction(const FName& InName)
{
	if (const FSkillEffectStruct* FindSkillEffectStruct = SkillEffectMap.Find(InName))
	{
		FGameplayTag GGameplayTag = FindSkillEffectStruct->GameplayCueTag;
		if (GGameplayTag.IsValid())
		{
			if (VFGAbilitySystemComponent->IsGameplayCueActive(GGameplayTag))
			{
				VFGAbilitySystemComponent->RemoveGameplayCue(GGameplayTag);
			}

			return;
		}
	}

	UE_LOG(LogTemp, Error, TEXT("InName is not found in RemoveGameplayCueFunction of %s"), *GetNameSafe(this));

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

//프로젝타일
void UFGGameplayAbility::SpawnProjectile(const FTransform& InSpawnTransform, float InInitialSpeed, float InMaxSpeed, const FVector& InProjectileDirection, bool InbHoming, AActor* InHitActor)
{
	if (!bProjectile) return;

	if (!IsValid(FGProjectileClass))
	{
		UE_LOG(LogTemp, Error, TEXT("FGProjectileClass is not valid in SpawnProjectile of %s"), *GetNameSafe(this));

		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);

		return;
	}

	if (HasAuthority(&CurrentActivationInfo))
	{
		VFGProjectile = GetWorld()->SpawnActorDeferred<AFGProjectile>(FGProjectileClass, InSpawnTransform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
		if (IsValid(VFGProjectile))
		{
			if (InbHoming)
			{
				VFGProjectile->GetOnProjectileHitActor().RemoveAll(this);
				VFGProjectile->GetOnProjectileHitActor().AddUObject(this, &UFGGameplayAbility::OnProjectileHitActorFunction);
				VFGProjectile->GetOnProjectileHitActorFail().RemoveAll(this);
				VFGProjectile->GetOnProjectileHitActorFail().AddUObject(this, &UFGGameplayAbility::OnProjectileHitActorFailFunction);
				VFGProjectile->SetProjectileTarget(InHitActor);
				VFGProjectile->SetbUseOnProjectileHitActor(true);
			}

			UProjectileMovementComponent* GProjectileMovementComponent = VFGProjectile->GetVProjectileMovementComponent();
			if (IsValid(GProjectileMovementComponent))
			{
				GProjectileMovementComponent->InitialSpeed = InInitialSpeed;
				GProjectileMovementComponent->MaxSpeed = InMaxSpeed;
			}

			UGameplayStatics::FinishSpawningActor(VFGProjectile, InSpawnTransform);

			if (!InbHoming)
			{
				VFGProjectile->EndPointDirection(InProjectileDirection);
			}
		}
	}
}

void UFGGameplayAbility::OnProjectileHitActorFunction()
{
	if (HasAuthority(&CurrentActivationInfo))
	{
		if (IsValid(VFGProjectile))
		{
			VFGProjectile->SetbHitTargetActor(true);
		}
	}
}

void UFGGameplayAbility::OnProjectileHitActorFailFunction()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

//Get, Set
TArray<float> UFGGameplayAbility::GetManaValue() const
{
	if (bSkillMana)
	{
		TArray<float> ManaArray;
		ManaArray.Add(SkillMana);

		return ManaArray;
	}

	return TArray<float>();
}

TArray<float> UFGGameplayAbility::GetCooldownValue() const
{
	if (bUseSkillCooldown)
	{
		TArray<float> CooldownArray;
		CooldownArray.Add(SkillCooldown);

		return CooldownArray;
	}

	return TArray<float>();
}
