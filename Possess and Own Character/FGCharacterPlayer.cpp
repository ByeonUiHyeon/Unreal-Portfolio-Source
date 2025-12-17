// Copyright Epic Games, Inc. All Rights Reserved.

#include "Character/FGCharacterPlayer.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameplayTagContainer.h"
#include "Tag/FGGameplayTag.h"
#include "AbilitySystem/FGAbilitySystemComponent.h"
#include "UserWidgetHUD/FGUserWidgetHUD.h"
#include "Components/DecalComponent.h"
#include "Player/FGPlayerController.h"
#include "UI/FGHUD.h"
#include "HpMpBar/FGChildWidgetComponent.h"
#include "AI/FGAIController.h"
#include "UObject/FastReferenceCollector.h"
#include "Net/UnrealNetwork.h"
#include "PaperSpriteComponent.h"
#include "Game/FGGameMode.h"
#include "UserWidgetHUD/FGSkillHotbar.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Physics/FGCollision.h"
#include "Attribute/FGAttributeSet.h"
#include "Inventory/FGInventoryComponent.h"
#include "Components/DynamicMeshComponent.h"
#include "GeometryScript/MeshPrimitiveFunctions.h"
#include "UserWidgetHUD/FGSelectedCharacterHpMp.h"
#include "Game/FGGameState.h"
#include "Grid/FGGridComponent.h"

AFGCharacterPlayer::AFGCharacterPlayer()
{
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = -90.f;
	CameraBoom->SetRelativeLocation(FVector(0.f, 0.f, 80.f));
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bInheritPitch = true;
	CameraBoom->bInheritYaw = true;
	CameraBoom->bInheritRoll = true;
	CameraBoom->bDoCollisionTest = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	TeamPaperSpriteComponent = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("TeamPaperSpriteComponent"));
	TeamPaperSpriteComponent->SetupAttachment(GetMesh());
	TeamPaperSpriteComponent->SetRelativeLocation(FVector(0.f, 0.f, 3000.f));
	TeamPaperSpriteComponent->SetRelativeRotation(FRotator(0.f, -90.f, 90.f));
	TeamPaperSpriteComponent->SetRelativeScale3D(FVector(0.5f, 0.5f, 0.5f));
	TeamPaperSpriteComponent->SetAbsolute(false, true, false);
	TeamPaperSpriteComponent->SetCollisionProfileName(TEXT("NoCollision"));
	TeamPaperSpriteComponent->bVisibleInSceneCaptureOnly = true;

	VFGInventoryComponent = CreateDefaultSubobject<UFGInventoryComponent>(TEXT("VFGInventoryComponent"));

	//UDynamicMeshComponent 사용하기 위해
	//플러그인에서 Geometry Script 추가
	//Build.cs에서 "GeometryFramework" 추가

	//UGeometryScriptLibrary_MeshPrimitiveFunctions::AppendTriangulatedPolygon(); 사용하기 위해
	//Build.cs에서 "GeometryScriptingCore" 추가
	VDynamicMeshComponent = CreateDefaultSubobject<UDynamicMeshComponent>(TEXT("VDynamicMeshComponent"));
	VDynamicMeshComponent->SetupAttachment(RootComponent);
	VDynamicMeshComponent->SetIsReplicated(true);
	//클라이언트에서 위치이동되게 하기위해 설정

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> CMaterialInterface(TEXT("/Script/Engine.Material'/Game/Blueprints/AbilitySystem/TA/Materials/M_SpellIndicatorRing.M_SpellIndicatorRing'"));
	if (CMaterialInterface.Succeeded())
	{
		VDynamicMeshComponent->SetMaterial(0, CMaterialInterface.Object);
	}

	//캐릭터 주변 범위
	VAroundDynamicMeshComponent = CreateDefaultSubobject<UDynamicMeshComponent>(TEXT("VAroundDynamicMeshComponent"));
	VAroundDynamicMeshComponent->SetupAttachment(RootComponent);
	VAroundDynamicMeshComponent->SetIsReplicated(true);

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> CAroundMaterialInterface(TEXT("/Script/Engine.Material'/Game/Blueprints/AbilitySystem/TA/Materials/M_AroundSpellIndicatorRing1.M_AroundSpellIndicatorRing1'"));
	if (CAroundMaterialInterface.Succeeded())
	{
		VAroundDynamicMeshComponent->SetMaterial(0, CAroundMaterialInterface.Object);
	}

	VFGGridComponent->GridType = EGridType::Player;
}

void AFGCharacterPlayer::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(AFGCharacterPlayer, SpawnFGPlayerController);
	DOREPLIFETIME(AFGCharacterPlayer, CharacterImage);
	DOREPLIFETIME(AFGCharacterPlayer, CharacterPlayerClass);
	DOREPLIFETIME(AFGCharacterPlayer, OwnPlayerNumber);
	DOREPLIFETIME(AFGCharacterPlayer, TeamIndex);
	DOREPLIFETIME(AFGCharacterPlayer, SelectedCharacterNumber);
	DOREPLIFETIME(AFGCharacterPlayer, VGameplayAbilityTargetActor);
	DOREPLIFETIME(AFGCharacterPlayer, Vertices2D);
	DOREPLIFETIME(AFGCharacterPlayer, AroundRadius);
	DOREPLIFETIME(AFGCharacterPlayer, bAroundSpellIndicator);
}

void AFGCharacterPlayer::BeginPlay()
{
	Super::BeginPlay();

	RespawnTransform = GetActorTransform();

	GetMesh()->SetVisibility(false, true);

	VFGGameState = GetWorld()->GetGameState<AFGGameState>();

	VFGAbilitySystemComponent->StatLevelSetting(FGCharacterStatTable, 1);
	VFGInventoryComponent->InitialInventoryItemAbility(this);

	VisionRadius = UnpossessVisionRadius;
	DynamicMeshComponentDefaultLocation = VDynamicMeshComponent->GetRelativeLocation();

	SetGameplayTagPairs();

	SettingAbility();

	SetPaperSprite();

	//캐릭터 스폰시 숨긴 상태에서 스폰하고 캐릭터 생성되면 같은 팀만 보이게 하기
	GetWorld()->GetTimerManager().SetTimer(CharacterVisibilityTimerHandle, FTimerDelegate::CreateLambda([&]()
		{
			GetMesh()->SetVisibility(true, true);

			GetWorld()->GetTimerManager().ClearTimer(CharacterVisibilityTimerHandle);
		}
	), 0.5f, false);

	if (IsValid(SpawnFGPlayerController) && SpawnFGPlayerController->IsLocalPlayerController())
	{
		CreateUserWidgetHUD();
		//위젯 생성

		//키세팅 최신화
		SpawnFGPlayerController->GetOnUpdateKeySetting().RemoveAll(this);
		SpawnFGPlayerController->GetOnUpdateKeySetting().AddUObject(this, &AFGCharacterPlayer::KeySettingSkillHotbarBroadcast);
	}
}

void AFGCharacterPlayer::SetDead(AFGCharacterBase* Killer)
{
	Super::SetDead(Killer);

	if (!IsValid(Killer)) return;

	AFGPlayerController* CFGPlayerController = Cast<AFGPlayerController>(GetController());
	if (IsValid(CFGPlayerController) && CFGPlayerController->IsLocalPlayerController())
	{
		SetControl(ECharacterControlType::WeakPossession);
	}

	//리스폰 시 남은 시간 표시
	//SpawnFGPlayerController->IsLocalPlayerController()를 사용하지 않는다
	//SetDead는 AttributeSet에서 브로드캐스트 하는건데 서버에서만 실행된다
	//그래서 IsLocalPlayerController()는 fasle가 된다.
	if (IsValid(SpawnFGPlayerController))
	{
		SpawnFGPlayerController->StartCharacterRespawnCoolTimeSetting(SelectedCharacterNumber, CharacterRespawnTime);
	}

	//경험치 및 골드 획득 
	AFGCharacterPlayer* CFGCharacterPlayer = Cast<AFGCharacterPlayer>(Killer);
	if (IsValid(CFGCharacterPlayer))
	{
		//킬 스코어 및 킬로그
		if (IsValid(VFGGameState) && IsValid(this))
		{
			VFGGameState->AddTeamScroeAndKilllog(CFGCharacterPlayer, this);
		}

		UFGAbilitySystemComponent* CFGAbilitySystemComponent = Cast<UFGAbilitySystemComponent>(CFGCharacterPlayer->GetAbilitySystemComponent());
		if (IsValid(CFGAbilitySystemComponent))
		{
			float KillXPValue = VFGAbilitySystemComponent->GetNumericAttribute(UFGAttributeSet::GetKillXPAttribute());
			float KillGoldValue = VFGAbilitySystemComponent->GetNumericAttribute(UFGAttributeSet::GetKillGoldAttribute());

			CFGAbilitySystemComponent->AddXP(CFGCharacterPlayer->GetFGCharacterStatTable(), KillXPValue);
			CFGAbilitySystemComponent->AddGold(KillGoldValue);
		}
	}
}

void AFGCharacterPlayer::SetDeadFunction(AFGCharacterBase* Killer)
{
	Super::SetDeadFunction(Killer);

	//리스폰
	TeamPaperSpriteComponent->SetHiddenInGame(true);

	GetWorld()->GetTimerManager().SetTimer(RespawnTimer, FTimerDelegate::CreateLambda([&]()
		{
			bDead = false;

			if (IsValid(VFGAbilitySystemComponent))
			{
				if (VFGAbilitySystemComponent->HasMatchingGameplayTag(CHARACTER_CHARACTER_ISDEAD))
				{
					VFGAbilitySystemComponent->RemoveLooseGameplayTag(CHARACTER_CHARACTER_ISDEAD);
					VFGAbilitySystemComponent->RemoveReplicatedLooseGameplayTag(CHARACTER_CHARACTER_ISDEAD);
				}

				if (VFGAbilitySystemComponent->HasMatchingGameplayTag(CHARACTER_STATE_ISHEALTHREGEN))
				{
					VFGAbilitySystemComponent->RemoveLooseGameplayTag(CHARACTER_STATE_ISHEALTHREGEN);
					VFGAbilitySystemComponent->RemoveReplicatedLooseGameplayTag(CHARACTER_STATE_ISHEALTHREGEN);
				}

				if (VFGAbilitySystemComponent->HasMatchingGameplayTag(CHARACTER_STATE_ISMANAREGEN))
				{
					VFGAbilitySystemComponent->RemoveLooseGameplayTag(CHARACTER_STATE_ISMANAREGEN);
					VFGAbilitySystemComponent->RemoveReplicatedLooseGameplayTag(CHARACTER_STATE_ISMANAREGEN);
				}

				if (IsValid(VFGAttributeSet))
				{
					VFGAttributeSet->SetHealth(VFGAbilitySystemComponent->GetNumericAttribute(UFGAttributeSet::GetMaxHealthAttribute()));
					VFGAttributeSet->SetMana(VFGAbilitySystemComponent->GetNumericAttribute(UFGAttributeSet::GetMaxManaAttribute()));
				}
			}

			VChildActorComponent->SetHiddenInGame(false);
			VPaperSpriteComponent->SetHiddenInGame(false);
			SelectCharacterDecal->SetHiddenInGame(false);
			TeamPaperSpriteComponent->SetHiddenInGame(false);

			GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Walking);

			UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
			if (IsValid(AnimInstance))
			{
				AnimInstance->StopAllMontages(0.f);
			}

			SetActorLocation(RespawnTransform.GetLocation());
			SetActorRotation(RespawnTransform.GetRotation());

			GetWorld()->GetTimerManager().ClearTimer(RespawnTimer);
		}
	), CharacterRespawnTime, false);
}

//AFGCharacterBase::SetGiveAbilities와 중복되는 어빌리티 있는지 확인하기 중복 있을 경우 두번 실행 됨
void AFGCharacterPlayer::SettingAbility()
{
	if (!IsValid(VFGAbilitySystemComponent)) return;

	VFGAbilitySystemComponent->OnActiveGameplayEffectAddedDelegateToSelf.AddUObject(this, &AFGCharacterPlayer::OnGameplayEffectAdded);

	if (HasAuthority())
	{
		SetGiveAbilities();

		int32 i = 0;
		for (const auto & SetInputAbility : SetInputAbilities)
		{
			FGameplayAbilitySpec StartSpec(SetInputAbility.Value.GameplayAbility);
			StartSpec.InputID = i;

			VFGAbilitySystemComponent->GiveAbility(StartSpec);

			++i;
		}
	}
}

void AFGCharacterPlayer::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (IsValid(VFGAbilitySystemComponent) && IsValid(PlayerInputComponent))
	{
		UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent);

		EnhancedInputComponent->BindAction(FirstMoveAction, ETriggerEvent::Triggered, this, &AFGCharacterPlayer::FirstMove);
		EnhancedInputComponent->BindAction(FirstMoveAction, ETriggerEvent::Completed, this, &AFGCharacterPlayer::StopMove);
		EnhancedInputComponent->BindAction(FirstLookAction, ETriggerEvent::Triggered, this, &AFGCharacterPlayer::FirstLook);
	}
}

void AFGCharacterPlayer::FirstMove(const FInputActionValue& Value)
{
	if (!IsValid(VFGAbilitySystemComponent)) return;

	for (const FGameplayTag& Tag : MoveBlockedTags)
	{
		if (VFGAbilitySystemComponent->HasMatchingGameplayTag(Tag))
		{
			return;
		}
	}

	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		AddMovementInput(ForwardDirection, MovementVector.X);
		AddMovementInput(RightDirection, MovementVector.Y);
	}

	if (!VFGAbilitySystemComponent->HasMatchingGameplayTag(MoveOwnedTag))
	{
		VFGAbilitySystemComponent->AddLooseGameplayTag(MoveOwnedTag);
		VFGAbilitySystemComponent->AddReplicatedLooseGameplayTag(MoveOwnedTag);
	}
}

void AFGCharacterPlayer::StopMove()
{
	if (IsValid(VFGAbilitySystemComponent) && VFGAbilitySystemComponent->HasMatchingGameplayTag(MoveOwnedTag))
	{
		VFGAbilitySystemComponent->RemoveLooseGameplayTag(MoveOwnedTag);
		VFGAbilitySystemComponent->RemoveReplicatedLooseGameplayTag(MoveOwnedTag);
	}
}

void AFGCharacterPlayer::FirstLook(const FInputActionValue& Value)
{
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		AddControllerYawInput(LookAxisVector.X * MouseSensitivity);
		AddControllerPitchInput(LookAxisVector.Y * MouseSensitivity);
	}
}

void AFGCharacterPlayer::SetControl(ECharacterControlType NewCharacterControlType)
{
	if (!IsValid(SpawnFGPlayerController) || !SpawnFGPlayerController->IsLocalPlayerController()) return;

	UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(SpawnFGPlayerController->GetLocalPlayer());
	if (Subsystem)
	{
		Subsystem->ClearAllMappings();

		const TObjectPtr<UInputMappingContext>* FoundMapping = SpawnFGPlayerController->GetControlChangeMap().Find(NewCharacterControlType);
		if (FoundMapping)
		{
			UInputMappingContext* NewMappingContext = *FoundMapping;
			if (NewMappingContext)
			{
				Subsystem->AddMappingContext(NewMappingContext, 0);
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("FoundMapping is not valid"));
		}
	}
}


//AIController가 포제스 되어도 실행된다.
//캐릭터에 포제스할때 먼저 AIController가 먼저 언포제스 후 PlayerController가 포제스 된다
//언포제스 시 PlayerController가 언포제스 되고 VFGAIController->Possess(this)로 PossessedBy가 실행된다.
void AFGCharacterPlayer::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
		
	PossessPlayerController = Cast<APlayerController>(NewController);
	if (IsValid(PossessPlayerController))
	{
		PossessFunction();
	}
}

//PossessedBy는 서버에서만 실행돼서
//클라이언트에서 Controller 변화로 포제스, 언포제스 변화 설정
void AFGCharacterPlayer::OnRep_Controller()
{
	Super::OnRep_Controller();
	
	APlayerController* CPlayerController = Cast<APlayerController>(Controller);
	if (IsValid(CPlayerController)) //포제스
	{
		PossessFunction();
	}
	else //언포제스
	{
		UnpossessFunction();
	}
}

//AIController가 언포제스 되어도 실행된다.
void AFGCharacterPlayer::UnPossessed()
{
	Super::UnPossessed();

	if (IsValid(PossessPlayerController))
	{
		PossessPlayerController = nullptr;

		UnpossessFunction();
	}
}

void AFGCharacterPlayer::PossessFunction()
{
	SetControl(ECharacterControlType::FirstPossession);
	//AddMappingContext 변경

	AddCrosshair();
	//크로스헤어 추가

	VisibleUserWidgetHUD();
	//위젯 보이기

	VisionRadius = PossessVisionRadius;

	if (IsValid(SpawnFGPlayerController) && SpawnFGPlayerController->IsLocalPlayerController())
	{
		SpawnFGPlayerController->GetOnUpdateKeySetting().Broadcast(ECharacterControlType::None);
		//키세팅 업데이트
	}
}

void AFGCharacterPlayer::UnpossessFunction()
{
	//설정해야 언포제스 후 AI 조종 가능
	if (IsValid(VFGAIController))
	{
		VFGAIController->Possess(this);
	}

	//클라이언트는 점프중 언포제스하면 EndAbility가 실행 안돼서 CancelAbilitiesWithTags 설정
	if (IsValid(VFGAbilitySystemComponent))
	{
		VFGAbilitySystemComponent->CancelAbilitiesWithTags(FGameplayTagContainer(DEFAULT_ABILITYTAG_JUMP));
	}

	SetControl(ECharacterControlType::WeakPossession);
	//AddMappingContext 변경

	//크로스헤어 숨김
	if (IsValid(Crosshair) && Crosshair->IsInViewport())
	{
		Crosshair->SetVisibility(ESlateVisibility::Collapsed);
	}

	VisionRadius = UnpossessVisionRadius;

	if (IsValid(SpawnFGPlayerController) && SpawnFGPlayerController->IsLocalPlayerController())
	{
		SpawnFGPlayerController->GetOnUpdateKeySetting().Broadcast(ECharacterControlType::None);
		//키세팅 업데이트
	}
}

void AFGCharacterPlayer::WeakPossess()
{
	SetControl(ECharacterControlType::WeakPossession);

	VisibleUserWidgetHUD();
}

void AFGCharacterPlayer::WeakUnpossess()
{
	SetControl(ECharacterControlType::WeakUnpossession);

	if (IsValid(VFGUserWidgetHUD) && VFGUserWidgetHUD->IsInViewport())
	{
		VFGUserWidgetHUD->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void AFGCharacterPlayer::AddCrosshair()
{
	if (!IsValid(SpawnFGPlayerController) || !SpawnFGPlayerController->IsLocalPlayerController()) return;

	if (IsValid(CrosshairClass))
	{
		if (IsValid(Crosshair) && Crosshair->IsInViewport())
		{
			Crosshair->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			Crosshair = CreateWidget<UUserWidget>(SpawnFGPlayerController, CrosshairClass);
			if (IsValid(Crosshair) && !Crosshair->IsInViewport())
			{
				Crosshair->AddToViewport();
			}
		}
	}
}

void AFGCharacterPlayer::CreateUserWidgetHUD()
{
	SetControl(ECharacterControlType::WeakPossession);
	//AddMappingContext 후에 위젯에서 QueryKeysMappedToAction를 해야 키를 찾을 수 있다

	if (!IsValid(SpawnFGPlayerController) || !SpawnFGPlayerController->IsLocalPlayerController()) return;

	if (IsValid(FGUserWidgetHUDClass))
	{
		VFGUserWidgetHUD = CreateWidget<UFGUserWidgetHUD>(SpawnFGPlayerController, FGUserWidgetHUDClass);
		if (IsValid(VFGUserWidgetHUD))
		{
			VFGUserWidgetHUD->SkillHotbarSetting(this);
			VFGUserWidgetHUD->GetBindToAttributes(VFGAbilitySystemComponent, VFGAttributeSet, FGCharacterStatTable);
			VFGUserWidgetHUD->CharacterImageSettingFunction(60.f, CharacterImage);
			VFGUserWidgetHUD->InventorySetting(VFGInventoryComponent);
			VFGUserWidgetHUD->AddToViewport();
			VFGUserWidgetHUD->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("FGUserWidgetHUDClass is not valid"));
	}

	//좌측 캐릭터 창 위젯 설정
	UFGSelectedCharacterHpMp* GFGSelectedCharacterHpMp = SpawnFGPlayerController->GetFGSelectedCharacterHpMp();
	if (IsValid(GFGSelectedCharacterHpMp))
	{
		GFGSelectedCharacterHpMp->SelectedCharacterSkillHotbarSetting(SelectedCharacterNumber, this);
		GFGSelectedCharacterHpMp->SelectedCharacterBindToAttributes(SelectedCharacterNumber, 40.f, CharacterImage, VFGAbilitySystemComponent, VFGAttributeSet);
	}

	SkillHotbarBroadcast();
}

void AFGCharacterPlayer::VisibleUserWidgetHUD()
{
	if (IsValid(VFGUserWidgetHUD) && VFGUserWidgetHUD->IsInViewport())
	{
		VFGUserWidgetHUD->SetVisibility(ESlateVisibility::Visible);
	}
}

void AFGCharacterPlayer::SetGameplayTagPairs()
{
	FGameplayTagPairs.Add(Skill1);
	FGameplayTagPairs.Add(Skill2);
	FGameplayTagPairs.Add(Skill3);
	FGameplayTagPairs.Add(Skill4);
}

void AFGCharacterPlayer::SkillHotbarBroadcast()
{
	GetWorld()->GetTimerManager().SetTimer(SkillHotbarBroadcastTimer, FTimerDelegate::CreateLambda([&]()
		{
			int32 i = 0;
			for (const FGameplayTagPair& GameplayTagPair : FGameplayTagPairs)
			{
				const FAbilityInfo* AbilityData = SetInputAbilities.Find(GameplayTagPair.DefaultAbilityTag);
				if (AbilityData != nullptr)
				{
					OnSkillHotbar.Broadcast(i, VFGAbilitySystemComponent, AbilityData->GameplayAbility, AbilityData->InputAction);
				}

				++i;
			}

			GetWorld()->GetTimerManager().ClearTimer(SkillHotbarBroadcastTimer);
		}
	), 0.05f, false);
}

void AFGCharacterPlayer::KeySettingSkillHotbarBroadcast(ECharacterControlType InECharacterControlType)
{
	SkillHotbarBroadcast();
}

void AFGCharacterPlayer::OnGameplayEffectAdded(UAbilitySystemComponent* ASC, const FGameplayEffectSpec& Spec, FActiveGameplayEffectHandle Handle)
{
	FGameplayTagContainer GrantedTags = Spec.Def->InheritableOwnedTagsContainer.CombinedTags;
	if (GrantedTags.Num() > 0)
	{
		for (const FGameplayTag& Tag : GrantedTags)
		{
			for (int32 i = 0; i < FGameplayTagPairs.Num(); ++i)
			{
				if (!FGameplayTagPairs[i].bCooldown)
				{
					if (FGameplayTagPairs[i].DurationTag == Tag)
					{
						GameplayEffectAddedDurationFunction(i, ASC, Spec, Handle);
					}

					if (FGameplayTagPairs[i].CooldownTag == Tag)
					{
						GameplayEffectAddedCooldownFunction(i, ASC, Spec, Handle);
					}
				}
			}
		}
	}
}

void AFGCharacterPlayer::GameplayEffectAddedDurationFunction(int32 Number, UAbilitySystemComponent* ASC, const FGameplayEffectSpec& Spec, FActiveGameplayEffectHandle Handle)
{
	if (!FGameplayTagPairs.IsValidIndex(Number) || !ASC || !Handle.IsValid()) return;

	FOnActiveGameplayEffectTimeChange* DurationChangeDelegate = ASC->OnGameplayEffectTimeChangeDelegate(Handle);
	if (DurationChangeDelegate)
	{
		DurationChangeDelegate->AddLambda([this, Number](FActiveGameplayEffectHandle InEffectHandle, float InRemainingTime, float InGetDuration)
			{
				if (HasAuthority())
				{
					NetMulticast_DurationTimeChangedDelegate(Number, InEffectHandle, InRemainingTime, InGetDuration);
				}
			});
	}

	GetWorld()->GetTimerManager().SetTimer(FGameplayTagPairs[Number].TagDurationTimer, FTimerDelegate::CreateLambda([this, Number, ASC, Handle]()
			{
				const FActiveGameplayEffect* DurationActiveEffect = ASC->GetActiveGameplayEffect(Handle);
				if (DurationActiveEffect)
				{
					float DurationRemainingTime = DurationActiveEffect->GetTimeRemaining(GetWorld()->GetTimeSeconds());
					float DurationTime = DurationActiveEffect->GetDuration();

					FOnActiveGameplayEffectTimeChange* BroadcastDurationChangeDelegate = ASC->OnGameplayEffectTimeChangeDelegate(Handle);
					if (BroadcastDurationChangeDelegate)
					{
						BroadcastDurationChangeDelegate->Broadcast(Handle, DurationRemainingTime, DurationTime);
					}
				}
			}
	), 0.2f, true);

	ASC->OnAnyGameplayEffectRemovedDelegate().AddLambda([this, Number, ASC, Handle](const FActiveGameplayEffect& InGameplayEffect)
		{
			if (InGameplayEffect.Handle == Handle && ASC && Handle.IsValid())
			{
				const FActiveGameplayEffect* DurationActiveEffect = ASC->GetActiveGameplayEffect(Handle);
				if (DurationActiveEffect)
				{
					float DurationRemainingTime = DurationActiveEffect->GetTimeRemaining(GetWorld()->GetTimeSeconds());
					float DurationTime = DurationActiveEffect->GetDuration();

					FOnActiveGameplayEffectTimeChange* EndDurationChangeDelegate = ASC->OnGameplayEffectTimeChangeDelegate(Handle);
					if (EndDurationChangeDelegate)
					{
						EndDurationChangeDelegate->Broadcast(Handle, DurationRemainingTime, DurationTime);
					}
				}

				if (HasAuthority())
				{
					NetMulticast_TagDurationTimerClearTimer(Number);
					NetMulticast_CollapsedDurationProgressBar(Number);
				}

				//클라이언트에서 언포제스 후 GA 사용 후 포제스하면 2개 쌓여 있어서 직접 제거
				FGameplayTagContainer RemoveDurationTags;
				InGameplayEffect.Spec.GetAllGrantedTags(RemoveDurationTags);
				for (const FGameplayTag& RemoveDurationTag : RemoveDurationTags)
				{
					VFGAbilitySystemComponent->RemoveLooseGameplayTag(RemoveDurationTag);
					VFGAbilitySystemComponent->RemoveReplicatedLooseGameplayTag(RemoveDurationTag);
				}
			}
		});
}

void AFGCharacterPlayer::NetMulticast_DurationTimeChangedDelegate_Implementation(int32 Number, FActiveGameplayEffectHandle InEffectHandle, float InRemainingTime, float InGetDuration)
{
	DurationTimeChangedDelegate.Broadcast(Number, InEffectHandle, InRemainingTime, InGetDuration);
}

void AFGCharacterPlayer::NetMulticast_TagDurationTimerClearTimer_Implementation(int32 Number)
{
	GetWorld()->GetTimerManager().ClearTimer(FGameplayTagPairs[Number].TagDurationTimer);
}

void AFGCharacterPlayer::NetMulticast_CollapsedDurationProgressBar_Implementation(int32 Number)
{
	OnCollapsedDurationProgressBar.Broadcast(Number);
}

void AFGCharacterPlayer::GameplayEffectAddedCooldownFunction(int32 Number, UAbilitySystemComponent* ASC, const FGameplayEffectSpec& Spec, FActiveGameplayEffectHandle Handle)
{
	if (!FGameplayTagPairs.IsValidIndex(Number) || !ASC || !Handle.IsValid()) return;

	FGameplayTagPairs[Number].bCooldown = true;

	FOnActiveGameplayEffectTimeChange* CooldownChangeDelegate = ASC->OnGameplayEffectTimeChangeDelegate(Handle);
	if (CooldownChangeDelegate)
	{
		CooldownChangeDelegate->AddLambda([this, Number](FActiveGameplayEffectHandle InEffectHandle, float InRemainingTime, float InDuration)
			{
				if (HasAuthority())
				{
					NetMulticast_CooldownTimeChangedDelegate(Number, InEffectHandle, InRemainingTime, InDuration);
				}
			});
	}

	GetWorld()->GetTimerManager().SetTimer(FGameplayTagPairs[Number].TagCooldownTimer, FTimerDelegate::CreateLambda([this, Number, ASC, Handle]()
			{				
				const FActiveGameplayEffect* CooldownActiveEffect = ASC->GetActiveGameplayEffect(Handle);
				if (CooldownActiveEffect)
				{
					float CooldownRemainingTime = CooldownActiveEffect->GetTimeRemaining(GetWorld()->GetTimeSeconds());
					float CooldownTime = CooldownActiveEffect->GetDuration();

					FOnActiveGameplayEffectTimeChange* BroadcastCooldownChangeDelegate = ASC->OnGameplayEffectTimeChangeDelegate(Handle);
					if (BroadcastCooldownChangeDelegate)
					{
						BroadcastCooldownChangeDelegate->Broadcast(Handle, CooldownRemainingTime, CooldownTime);
					}
				}
			}
	), 0.2f, true);

	ASC->OnAnyGameplayEffectRemovedDelegate().AddLambda([this, Number, ASC, Handle](const FActiveGameplayEffect& InGameplayEffect)
		{
			if (InGameplayEffect.Handle == Handle && ASC && Handle.IsValid())
			{
				const FActiveGameplayEffect* CooldownActiveEffect = ASC->GetActiveGameplayEffect(Handle);
				if (CooldownActiveEffect)
				{
					float CooldownRemainingTime = CooldownActiveEffect->GetTimeRemaining(GetWorld()->GetTimeSeconds());
					float CooldownTime = CooldownActiveEffect->GetDuration();

					FOnActiveGameplayEffectTimeChange* EndCooldownChangeDelegate = ASC->OnGameplayEffectTimeChangeDelegate(Handle);
					if (EndCooldownChangeDelegate)
					{
						EndCooldownChangeDelegate->Broadcast(Handle, CooldownRemainingTime, CooldownTime);
					}
				}

				if (HasAuthority())
				{
					NetMulticast_TagCooldownTimerClearTimer(Number);
				}

				//클라이언트에서 언포제스 후 GA 사용 후 포제스하면 2개 쌓여 있어서 직접 제거
				FGameplayTagContainer RemoveCooldownTags;
				InGameplayEffect.Spec.GetAllGrantedTags(RemoveCooldownTags);
				for (const FGameplayTag& RemoveCooldownTag : RemoveCooldownTags)
				{
					VFGAbilitySystemComponent->RemoveLooseGameplayTag(RemoveCooldownTag);
					VFGAbilitySystemComponent->RemoveReplicatedLooseGameplayTag(RemoveCooldownTag);
				}
			}
		});
}

void AFGCharacterPlayer::NetMulticast_CooldownTimeChangedDelegate_Implementation(int32 Number, FActiveGameplayEffectHandle InEffectHandle, float InRemainingTime, float InGetDuration)
{
	CooldownTimeChangedDelegate.Broadcast(Number, InEffectHandle, InRemainingTime, InGetDuration);
}

void AFGCharacterPlayer::NetMulticast_TagCooldownTimerClearTimer_Implementation(int32 Number)
{
	FGameplayTagPairs[Number].bCooldown = false;

	GetWorld()->GetTimerManager().ClearTimer(FGameplayTagPairs[Number].TagCooldownTimer);
}

void AFGCharacterPlayer::SetPaperSprite()
{
	if (TeamIndex == 0)
	{
		TeamPaperSpriteComponent->SetSprite(TeamAPaperSprite);
	}
	else if (TeamIndex == 1)
	{
		TeamPaperSpriteComponent->SetSprite(TeamBPaperSprite);
	}
}

void AFGCharacterPlayer::AppendTriangulatedPolygonFunction()
{
	if (IsValid(SpawnFGPlayerController) && SpawnFGPlayerController->IsLocalPlayerController() && IsValid(VDynamicMeshComponent))
	{
		VDynamicMesh = VDynamicMeshComponent->GetDynamicMesh();
		if (IsValid(VDynamicMesh))
		{
			//DynamicMesh 위치 설정
			FTransform TransformValue; //FTransform::Identity
			//TransformValue.SetLocation(FVector(0.f, 0.f, 150.f));
			TransformValue.SetLocation(FVector(0.f, 0.f, -96.f));
			FRotator RotatorValue(45.f, 0.f, 0.f);
			TransformValue.SetRotation(RotatorValue.Quaternion());

			UGeometryScriptLibrary_MeshPrimitiveFunctions::AppendTriangulatedPolygon(VDynamicMesh, FGeometryScriptPrimitiveOptions{}, FTransform::Identity, Vertices2D);
			//메시 모양 설정
		}
	}
}

void AFGCharacterPlayer::ResetDynamicMesh()
{
	if (IsValid(VDynamicMesh))
	{
		VDynamicMesh->Reset();
	}
}

void AFGCharacterPlayer::AroundAppendTriangulatedPolygonFunction()
{
	if (IsValid(SpawnFGPlayerController) && SpawnFGPlayerController->IsLocalPlayerController() && IsValid(VAroundDynamicMeshComponent))
	{
		VAroundDynamicMesh = VAroundDynamicMeshComponent->GetDynamicMesh();
		if (IsValid(VAroundDynamicMesh))
		{
			TArray<FVector2D> Vertices2DValue;

			const int32 NumVertices = 64;
			for (int32 i = 0; i < NumVertices; ++i)
			{
				float Angle = (2.f * PI * i) / NumVertices;
				float X = FMath::Cos(Angle) * AroundRadius;
				float Y = FMath::Sin(Angle) * AroundRadius;

				Vertices2DValue.Add(FVector2D(X, Y));
			}

			UGeometryScriptLibrary_MeshPrimitiveFunctions::AppendTriangulatedPolygon(VAroundDynamicMesh, FGeometryScriptPrimitiveOptions{}, FTransform::Identity, Vertices2DValue);
		}
	}
}

void AFGCharacterPlayer::ResetAroundDynamicMesh()
{
	if (IsValid(VAroundDynamicMesh))
	{
		VAroundDynamicMesh->Reset();
	}
}

void AFGCharacterPlayer::SetDynamicMeshComponentDefaultLocation()
{
	VDynamicMeshComponent->SetRelativeLocation(DynamicMeshComponentDefaultLocation);
}

void AFGCharacterPlayer::SetDynamicMeshComponentLocation()
{
	VDynamicMeshComponent->SetWorldLocation(MouseCursorLocation + FVector(0.f, 0.f, 42.f));
}

void AFGCharacterPlayer::OnChildWidgetCreated(AFGChildWidgetComponent* InFGChildWidgetComponent)
{
	if (IsValid(InFGChildWidgetComponent))
	{
		InFGChildWidgetComponent->CreateHpMpWidget(true, OwnPlayerNumber, TeamIndex, VFGAbilitySystemComponent, VFGAttributeSet, this);
	}
}

//Interface
void AFGCharacterPlayer::SetFGSkillHotbar(UFGSkillHotbar* InFGSkillHotbar)
{
	if (InFGSkillHotbar)
	{
		OnSkillHotbar.AddUObject(InFGSkillHotbar, &UFGSkillHotbar::UpdateSkillHotbar);
		OnCollapsedDurationProgressBar.AddUObject(InFGSkillHotbar, &UFGSkillHotbar::CollapsedDurationProgressBarVisibility);
		DurationTimeChangedDelegate.AddUObject(InFGSkillHotbar, &UFGSkillHotbar::UpdateSkillHotbarDuration);
		CooldownTimeChangedDelegate.AddUObject(InFGSkillHotbar, &UFGSkillHotbar::UpdateSkillHotbarCoolTime);
	}
}

void AFGCharacterPlayer::SetSelectCharacterDecalVisibility(bool bVisibility)
{
	SelectCharacterDecal->SetVisibility(bVisibility);
}

EMouseCursor::Type AFGCharacterPlayer::ChildMouseHover(int32 InTeamIndex, int32 InOwnPlayerNumber)
{
	if (InTeamIndex == TeamIndex)
	{
		if (InOwnPlayerNumber == OwnPlayerNumber)
		{
			return EMouseCursor::Crosshairs;
		}

		return EMouseCursor::Custom;
	}

	return EMouseCursor::EyeDropper;
}
