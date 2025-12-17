// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/GA/FGGA_PushBack.h"
#include "Character/FGCharacterBase.h"
#include "Player/FGPlayerController.h"

UFGGA_PushBack::UFGGA_PushBack()
{
	bSkillMana = true;
	SkillMana = 5.f;
	bSkillStrength = false;
	bSkillBarrier = false;
	bUseSkillDuration = false;
	bUseSkillCooldown = true;
	SkillCooldown = 10.f;
	bSkillDamage = true;
	SkillDamage = 10.f;
	bHpMp = false;
	bDisableMovementTime = true;
	bPushStrength = true;
	PushStrength = 2000.f;
}

void UFGGA_PushBack::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	//부채꼴 트레이스 설정
	TraceStart = VFGCharacterBase->GetActorLocation() + FVector(0.f, 0.f, TraceHeight);
	ForwardVector = VFGCharacterBase->GetActorForwardVector();
	ActorTransform = VFGCharacterBase->GetActorTransform();

	StepAngle = TraceConeAngle / (TraceCount - 1);

	TArray<FVector2D> Vertices2D;
	Vertices2D.Add(FVector2D(0.f, 0.f));
	//부채꼴 중심점 추가

	for (int32 i = 0; i < TraceCount; ++i)
	{
		//트레이스 당 각도
		float Angle = TraceConeAngle / 2.f - StepAngle * i;

		//ForwardVector 벡터 기준 회전
		FRotator Rotated(0.f, Angle, 0.f);
		FVector Direction = Rotated.RotateVector(ForwardVector);
		FVector End = TraceStart + Direction * TraceLength;

		//로컬 좌표 변환
		FVector InverseTransformPositionValue = ActorTransform.InverseTransformPosition(End);
		Vertices2D.Add(FVector2D(InverseTransformPositionValue.X, InverseTransformPositionValue.Y));
	}

	SpellIndicatorFunction(TEXT("PushBack"), Vertices2D);
}

void UFGGA_PushBack::OnTargetDataReady(const FGameplayAbilityTargetDataHandle& Data)
{
	Super::OnTargetDataReady(Data);

	//스킬 범위 표시 후 스킬 사용할때 SetFGCharacterPlayerRotation로 회전 변화 때문에 최신화 시킨다.
	TraceStart = VFGCharacterBase->GetActorLocation() + FVector(0.f, 0.f, TraceHeight);
	ForwardVector = VFGCharacterBase->GetActorForwardVector();

	PlayMontageAndWaitFunction(TEXT("PushBack"));
}

void UFGGA_PushBack::OnEventReceived(FGameplayEventData Payload)
{
	Super::OnEventReceived(Payload);

	SetByCallerManaFunction();
	SetByCallerCooldownFunction();

	//트레이스
	AFGPlayerController* CFGPlayerController = Cast<AFGPlayerController>(VFGCharacterBase->GetController());
	if (IsValid(CFGPlayerController))
	{
		FRotator ControlRotation = CFGPlayerController->GetControlRotation();
		FVector ControlForwardVector = ControlRotation.Vector();

		ConeTraceFunction(ControlForwardVector);
	}
	else
	{
		ConeTraceFunction(ForwardVector);
	}
}

//부채꼴 트레이스
void UFGGA_PushBack::ConeTraceFunction(const FVector& InFVector)
{
	for (int32 i = 0; i < TraceCount; ++i)
	{
		float Angle = TraceConeAngle / 2.f - StepAngle * i;

		FRotator Rotated(0.f, Angle, 0.f);
		FVector Direction = Rotated.RotateVector(InFVector);
		FVector End = TraceStart + Direction * TraceLength;

		UseWaitTraceByName(TEXT("PushBack"), TraceStart, End, false, false);
	}
}

void UFGGA_PushBack::MiddleTraceResultFunction(const FHitResult& InHitResult, AActor* InActor)
{
	AlreadyHitActors.Add(InActor);

	SetByCallerDamageFunction(InActor, InHitResult);

	TargetDisableMovement(InActor);
	PushCharacter(InActor);
}
