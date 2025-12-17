// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/FGPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Interface/FGRTSInterface.h"
#include "UI/FGHUD.h"
#include "Character/FGCharacterPlayer.h"
#include "Character/FGCharacterMob.h"
//#include "Navigation/CrowdFollowingComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "AbilitySystem/FGAbilitySystemComponent.h"
//#include "DetourCrowdAIController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AI/FGAIController.h"
#include "Net/UnrealNetwork.h"
#include "UserWidgetHUD/FGCommonHUD.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "Physics/FGCollision.h"
#include "Engine/OverlapResult.h"
#include "Actors/FGNexus.h"
#include "HpMpBar/FGChildWidgetComponent.h"
#include "UserWidgetHUD/FGSelectedCharacterHpMp.h"
#include "Interface/FGSelectedCharacterHpMpInterface.h"
#include "Actors/FGMine.h"
#include "Shop/FGShop.h"
#include "Shop/FGShopComponent.h"
#include "Inventory/FGInventoryComponent.h"
#include "Chat/FGChatComponent.h"
#include "UI/FGSettings.h"
#include "Save/FGLocalPlayerSaveGame.h"
#include "Game/FGGameInstance.h"
#include "AbilitySystem/AT/FGAT_WaitTargetData.h"
#include "AbilitySystem/GA/FGGameplayAbility.h"
#include "Interface/FGMouseSensitivityInterface.h"
#include "Shop/FGShopHUD.h"
#include "Game/FGGameState.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/Canvas.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Engine/DecalActor.h"
#include "Interface/FGVisibilityInterface.h"
#include "Actors/FGClickMarker.h"

AFGPlayerController::AFGPlayerController()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	VFGChatComponent = CreateDefaultSubobject<UFGChatComponent>(TEXT("VFGChatComponent"));
	//에디터에서 BP_FGChatComponent로 변경하기

	//AIControllerClass = ADetourCrowdAIController::StaticClass();

	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;

	bReplicates = true;
}

void AFGPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(AFGPlayerController, VFGCharacterPlayer);
	DOREPLIFETIME(AFGPlayerController, bCharacterMode);
	DOREPLIFETIME(AFGPlayerController, VFGAbilitySystemComponent);
	DOREPLIFETIME(AFGPlayerController, VFGSelectedCharacterHpMp);
	DOREPLIFETIME(AFGPlayerController, SelectedCharacterPlayerStructArray);
	DOREPLIFETIME(AFGPlayerController, OwnPlayerNumber);
	DOREPLIFETIME(AFGPlayerController, TeamIndex);
}

void AFGPlayerController::SetFGCharacterPlayer(AFGCharacterPlayer* InFGCharacterPlayer)
{
	if (HasAuthority())
	{
		SetFGCharacterPlayerFunction(InFGCharacterPlayer);
	}
	else
	{
		Server_SetFGCharacterPlayer(InFGCharacterPlayer);
	}
}

void AFGPlayerController::SetFGCharacterPlayerFunction(AFGCharacterPlayer* InFGCharacterPlayer)
{
	if (InFGCharacterPlayer)
	{
		VFGCharacterPlayer = InFGCharacterPlayer;
		VFGAbilitySystemComponent = VFGCharacterPlayer->GetFGAbilitySystemComponent();
	}
	else
	{
		VFGCharacterPlayer = nullptr;
		VFGAbilitySystemComponent = nullptr;
	}
}

void AFGPlayerController::Server_SetFGCharacterPlayer_Implementation(AFGCharacterPlayer* InFGCharacterPlayer)
{
	SetFGCharacterPlayerFunction(InFGCharacterPlayer);
}

void AFGPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalPlayerController()) return;

	VFGGameState = GetWorld()->GetGameState<AFGGameState>();

	if (HasAuthority())
	{
		if (IsValid(VFGGameState))
		{
			VFGGameState->NotifyPlayerLoadingComplete(this);
		}
	}
	else
	{
		Server_NotifyLoadingComplete();
	}

	//인풋 매핑 등록
	//키세팅 변경하기 위해 무조건 추가
	RegisterInputMappingContextSetting();

	//FInputModeGameAndUI GameAndUIInputMode;
	//GameAndUIInputMode.SetHideCursorDuringCapture(false);
	//GameAndUIInputMode.SetLockMouseToViewportBehavior(EMouseLockMode::LockAlways);
	//SetInputMode(GameAndUIInputMode);

	InputModeGameAndUIFunction();

	VFGGameInstance = GetGameInstance<UFGGameInstance>();

	RTSPawn = GetPawn();

	VFGHUD = Cast<AFGHUD>(GetHUD());
	if (IsValid(VFGHUD))
	{
		VFGRTSInterface = Cast<IFGRTSInterface>(VFGHUD);
	}

	CreatePlayerWidgets();
	//위젯 생성

	//키세팅 저장된거 불러오기
	if (HasAuthority())
	{
		LoadFunction();
	}
	
	RenderTargetSetting();

	SetActorTickEnabled(true);

	GetWorld()->GetTimerManager().SetTimer(MouseHoverTimerHandle, this, &AFGPlayerController::MouseHover, 0.1f, true);
}

void AFGPlayerController::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	//키세팅 저장된거 불러오기
	//클라이언트에서도 설정되게 OnRep_PlayerState에 추가 설정
	LoadFunction();
}

void AFGPlayerController::LoadFunction()
{
	if (!IsValid(VFGGameInstance)) return;

	FString InSlotName = FString::Printf(TEXT("PlayerSaveSlot_%s"), *VFGGameInstance->GetUniquePlayerIdFunction());

	UFGLocalPlayerSaveGame* FGLocalPlayerSaveGame = Cast<UFGLocalPlayerSaveGame>(UFGLocalPlayerSaveGame::LoadOrCreateSaveGameForLocalPlayer(UFGLocalPlayerSaveGame::StaticClass(), GetLocalPlayer(), InSlotName));
	if (IsValid(FGLocalPlayerSaveGame))
	{
		FGLocalPlayerSaveGame->LoadSaveGameData();
	}
}

void AFGPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	VEnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent);
	if (VEnhancedInputComponent)
	{
		VEnhancedInputComponent->BindAction(GetInputActionByName(TEXT("RTSLeftMouse")), ETriggerEvent::Started, this, &AFGPlayerController::LeftMousePressed);
		VEnhancedInputComponent->BindAction(GetInputActionByName(TEXT("RTSLeftMouse")), ETriggerEvent::Completed, this, &AFGPlayerController::LeftMouseReleased);
		VEnhancedInputComponent->BindAction(GetInputActionByName(TEXT("RTSRightMouse")), ETriggerEvent::Triggered, this, &AFGPlayerController::RightMousePressed);
		VEnhancedInputComponent->BindAction(GetInputActionByName(TEXT("RTSRightMouse")), ETriggerEvent::Completed, this, &AFGPlayerController::RightMouseReleased);

		VEnhancedInputComponent->BindAction(GetInputActionByName(TEXT("ChangePossess")), ETriggerEvent::Started, this, &AFGPlayerController::ChangePossess);
		VEnhancedInputComponent->BindAction(GetInputActionByName(TEXT("RTSAttack")), ETriggerEvent::Started, this, &AFGPlayerController::RTSAttack);
		VEnhancedInputComponent->BindAction(GetInputActionByName(TEXT("RTSStop")), ETriggerEvent::Started, this, &AFGPlayerController::RTSStop);
		VEnhancedInputComponent->BindAction(GetInputActionByName(TEXT("RTSHold")), ETriggerEvent::Started, this, &AFGPlayerController::RTSHold);
		VEnhancedInputComponent->BindAction(GetInputActionByName(TEXT("RTSPatrol")), ETriggerEvent::Started, this, &AFGPlayerController::RTSPatrol);
		VEnhancedInputComponent->BindAction(GetInputActionByName(TEXT("FocusCharacter")), ETriggerEvent::Triggered, this, &AFGPlayerController::FocusCharacter);

		VEnhancedInputComponent->BindAction(GetInputActionByName(TEXT("SelectCharacter1")), ETriggerEvent::Started, this, &AFGPlayerController::SelectCharacter1);
		VEnhancedInputComponent->BindAction(GetInputActionByName(TEXT("SelectCharacter2")), ETriggerEvent::Started, this, &AFGPlayerController::SelectCharacter2);
		VEnhancedInputComponent->BindAction(GetInputActionByName(TEXT("SelectCharacter3")), ETriggerEvent::Started, this, &AFGPlayerController::SelectCharacter3);
		VEnhancedInputComponent->BindAction(GetInputActionByName(TEXT("SelectAllCharacter")), ETriggerEvent::Started, this, &AFGPlayerController::SelectAllCharacter);

		VEnhancedInputComponent->BindAction(GetInputActionByName(TEXT("Settings")), ETriggerEvent::Started, this, &AFGPlayerController::CreateFGSettingsWidget);

		VEnhancedInputComponent->BindAction(GetInputActionByName(TEXT("IncreaseSensitivity")), ETriggerEvent::Triggered, this, &AFGPlayerController::IncreaseSensitivity);
		VEnhancedInputComponent->BindAction(GetInputActionByName(TEXT("DecreaseSensitivity")), ETriggerEvent::Triggered, this, &AFGPlayerController::DecreaseSensitivity);

		for (const auto& InventoryInputAction : InventoryInputActionArray)
		{
			VEnhancedInputComponent->BindAction(InventoryInputAction, ETriggerEvent::Started, this, &AFGPlayerController::OnUseInventoryItemAction);
		}
	}
}

void AFGPlayerController::AddAbilityInputComponent(const TMap<FGameplayTag, FAbilityInfo>& InSetInputAbilities)
{
	if (BindingHandles.Num() > 0)
	{
		for (const auto& BindingHandle : BindingHandles)
		{
			VEnhancedInputComponent->RemoveBindingByHandle(BindingHandle);
		}

		BindingHandles.Reset();
	}

	int32 i = 0;
	for (const auto & SetInputAbility : InSetInputAbilities)
	{
		for (const auto& TriggerEventAndInputPressed : SetInputAbility.Value.TriggerEventAndInputPressedMap)
		{
			//ControllerInputReleased 사용하는지 확인해서 SetMouseCursorPosition 틱 사용하기 위해 추가
			if (!TriggerEventAndInputPressed.Value)
			{
				HaveCompletedEventMap.Add(i, true);
			}

			void (AFGPlayerController:: * InputActionFunction)(int32) = TriggerEventAndInputPressed.Value ? &AFGPlayerController::ControllerInputPressed : &AFGPlayerController::ControllerInputReleased;

			FInputBindingHandle InputBindingHandle = VEnhancedInputComponent->BindAction(SetInputAbility.Value.InputAction, TriggerEventAndInputPressed.Key, this, InputActionFunction, i);

			BindingHandles.Add(InputBindingHandle.GetHandle());
		}

		++i;
	}
}

void AFGPlayerController::ControllerInputPressed(int32 InputId)
{	
	if (!IsLocalPlayerController()) return;

	if (IsValid(VFGAbilitySystemComponent) && VFGAbilitySystemComponent->HasMatchingGameplayTag(ChangePossessOwnedTag)) return;
	//ChangePossess 중에 실행 안되게 하기

	CancelTargetingFunctionBundle();
	//SpellIndicator 중에 다른 GA 사용할 경우 취소하고 다른 GA 실행

	if (!bCharacterMode)
	{
		//RTSStop();
		//각각 GA에서 StopMovementImmediately 추가

		SetMouseCursorPosition(nullptr);

		//스킬 누르고 있을때 캐릭터 회전 사용하기 위해
		bool* bHaveCompletedEvent = HaveCompletedEventMap.Find(InputId);
		if (bHaveCompletedEvent && *bHaveCompletedEvent)
		{
			GetWorld()->GetTimerManager().SetTimer(HaveCompletedEventTimer, FTimerDelegate::CreateLambda([&]()
				{
					SetMouseCursorPosition(nullptr);
				}
			), 0.05f, true);
		}
	}

	if (HasAuthority())
	{
		ControllerInputPressedFunction(InputId);
	}
	else
	{
		Server_ControllerInputPressed(InputId);
	}

	//스킬 사용전 범위 표시
	//UFGAT_WaitTargetData 사용할때 실행
	//바로 사용하면 실행 안돼서 딜레이 추가
	//포제스 중 SpellIndicator 사용 안되게 bSpellIndicator 설정
	if (!IsValid(VGameplayAbilityTargetActor))
	{
		GetWorld()->GetTimerManager().SetTimer(AppendTriangulatedPolygonTimer, FTimerDelegate::CreateLambda([&]()
			{
				if (IsValid(VFGCharacterPlayer))
				{
					VGameplayAbilityTargetActor = VFGCharacterPlayer->GetVGameplayAbilityTargetActor();
					if (IsValid(VGameplayAbilityTargetActor))
					{
						VFGCharacterPlayer->AppendTriangulatedPolygonFunction();
						//스킬 범위

						//스킬 사용 가능 범위
						if (VFGCharacterPlayer->GetbAroundSpellIndicator())
						{
							VFGCharacterPlayer->AroundAppendTriangulatedPolygonFunction();
						}

						GetWorld()->GetTimerManager().SetTimer(SetMouseCursorPositionTimer, FTimerDelegate::CreateLambda([&]()
							{
								SetMouseCursorPosition(nullptr);
							}
						), 0.05f, true);
					}
				}

				GetWorld()->GetTimerManager().ClearTimer(AppendTriangulatedPolygonTimer);
			}
		), 0.3f, false);
	}
}

void AFGPlayerController::ControllerInputPressedFunction(int32 InputId)
{
	if (VFGAbilitySystemComponent)
	{	
		FGameplayAbilitySpec* Spec = VFGAbilitySystemComponent->FindAbilitySpecFromInputID(InputId);
		if (Spec)
		{
			Spec->InputPressed = true;
			if (Spec->IsActive())
			{
				VFGAbilitySystemComponent->AbilitySpecInputPressed(*Spec);
			}
			else
			{
				VFGAbilitySystemComponent->TryActivateAbility(Spec->Handle);
			}
		}
	}
}

void AFGPlayerController::Server_ControllerInputPressed_Implementation(int32 InputId)
{
	ControllerInputPressedFunction(InputId);
}

void AFGPlayerController::ControllerInputReleased(int32 InputId)
{
	if (!IsLocalPlayerController()) return;

	if (!bCharacterMode)
	{
		SetMouseCursorPosition(nullptr);

		GetWorld()->GetTimerManager().ClearTimer(HaveCompletedEventTimer);
	}

	if (HasAuthority())
	{
		ControllerInputReleasedFunction(InputId);
	}
	else
	{
		Server_ControllerInputReleased(InputId);
	}
}

void AFGPlayerController::ControllerInputReleasedFunction(int32 InputId)
{
	if (VFGAbilitySystemComponent)
	{
		FGameplayAbilitySpec* Spec = VFGAbilitySystemComponent->FindAbilitySpecFromInputID(InputId);
		if (Spec)
		{
			Spec->InputPressed = false;
			if (Spec->IsActive())
			{
				VFGAbilitySystemComponent->AbilitySpecInputReleased(*Spec);
			}
		}
	}
}

void AFGPlayerController::Server_ControllerInputReleased_Implementation(int32 InputId)
{
	ControllerInputReleasedFunction(InputId);
}

void AFGPlayerController::SetMouseCursorPosition(AFGCharacterPlayer* InFGCharacterPlayer)
{
	FHitResult HitResult;
	bool bHit = GetHitResultUnderCursorByChannel(UEngineTypes::ConvertToTraceType(ECC_Visibility), true, HitResult);
	FVector MouseLocation = HitResult.Location;

	if (HasAuthority())
	{
		SetMouseCursorPositionFunction(InFGCharacterPlayer, MouseLocation);
	}
	else
	{
		Server_SetMouseCursorPosition(InFGCharacterPlayer, MouseLocation);
	}
}

void AFGPlayerController::SetMouseCursorPositionFunction(AFGCharacterPlayer* InFGCharacterPlayer, FVector InVector)
{
	if (InFGCharacterPlayer)
	{
		FRotator NewRotation = (InVector - InFGCharacterPlayer->GetActorLocation()).Rotation();
		NewRotation.Pitch = 0.f;
		NewRotation.Roll = 0.f;

		InFGCharacterPlayer->SetMouseCursorLocation(InVector);
		InFGCharacterPlayer->SetMouseCursorRotator(NewRotation);

		return;
	}

	if (VFGCharacterPlayer)
	{
		FRotator NewRotation = (InVector - VFGCharacterPlayer->GetActorLocation()).Rotation();
		NewRotation.Pitch = 0.f;
		NewRotation.Roll = 0.f;

		VFGCharacterPlayer->SetMouseCursorLocation(InVector);
		VFGCharacterPlayer->SetMouseCursorRotator(NewRotation);
	}
}

void AFGPlayerController::Server_SetMouseCursorPosition_Implementation(AFGCharacterPlayer* InFGCharacterPlayer, FVector InVector)
{
	SetMouseCursorPositionFunction(InFGCharacterPlayer, InVector);
}

void AFGPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	//마우스 클릭 드래그 시 박스 그리기
	if (VFGRTSInterface && bLeftMousePressed)
	{
		VFGRTSInterface->MarqueeHeld();
	}
}

void AFGPlayerController::LeftMousePressed()
{
	//SpellIndicator 이후 실행
	//AGameplayAbilityTargetActor의 ConfirmTargeting를 실행시키기 위한 설정
	if (IsValid(VGameplayAbilityTargetActor))
	{
		if (HasAuthority())
		{
			ConfirmTargetingFunction();
		}
		else
		{
			Server_ConfirmTargetingFunction();
		}

		//DynamicMesh 제거
		if (IsValid(VFGCharacterPlayer))
		{
			VFGCharacterPlayer->ResetDynamicMesh();
			VFGCharacterPlayer->ResetAroundDynamicMesh();
		}

		GetWorld()->GetTimerManager().ClearTimer(SetMouseCursorPositionTimer);
		//스킬 범위 표시기 회전 타이머 제거

		VGameplayAbilityTargetActor = nullptr;
		//초기화
	}

	if (VFGCharacterPlayer)
	{
		for (const FGameplayTag& Tag : LeftMousePressedBlockedTags)
		{
			if (VFGAbilitySystemComponent && VFGAbilitySystemComponent->HasMatchingGameplayTag(Tag)) return;
		}
	}

	bLeftMousePressed = true;

	VFGRTSInterface->MarqueePressed();

	//상점
	if (bShopClicked && IsLocalPlayerController())
	{
		FHitResult HitResult;
		bool bHit = GetHitResultUnderCursorByChannel(UEngineTypes::ConvertToTraceType(ECC_Visibility), true, HitResult);
		if (bHit && HitResult.bBlockingHit)
		{
			AFGShop* CFGShop = Cast<AFGShop>(HitResult.GetActor());
			if (IsValid(CFGShop))
			{
				if (IsValid(VFGShopHUD) && VFGShopHUD->IsInViewport())
				{
					VFGShopHUD->SetVisibility(ESlateVisibility::Visible);
				}
				else
				{
					VFGShopHUD = CreateWidget<UFGShopHUD>(this, GetUserWidgetClassByName(TEXT("UFGShopHUD")));
					if (IsValid(VFGShopHUD) && !VFGShopHUD->IsInViewport())
					{
						UFGShopComponent* GFGShopComponent = CFGShop->GetFGShopComponent();
						if (!IsValid(GFGShopComponent)) return;

						VFGShopHUD->ShopSetting(GFGShopComponent, CFGShop);
						VFGShopHUD->AddToViewport();
					}
				}
			}
		}
	}
	
	if (VFGCharacterPlayer && VFGAbilitySystemComponent && !VFGAbilitySystemComponent->HasMatchingGameplayTag(LeftMousePressedOwnedTag))
	{
		VFGAbilitySystemComponent->AddLooseGameplayTag(LeftMousePressedOwnedTag);
		VFGAbilitySystemComponent->AddReplicatedLooseGameplayTag(LeftMousePressedOwnedTag);
	}
}

void AFGPlayerController::ConfirmTargetingFunction()
{
	if (IsValid(VFGCharacterPlayer))
	{
		AGameplayAbilityTargetActor* GGameplayAbilityTargetActor = VFGCharacterPlayer->GetVGameplayAbilityTargetActor();
		if (IsValid(GGameplayAbilityTargetActor))
		{
			GGameplayAbilityTargetActor->ConfirmTargeting();
			VFGCharacterPlayer->SetGameplayAbilityTargetActor(nullptr);
			VFGCharacterPlayer->SetbAroundSpellIndicator(false);
			//사용 가능 범위 bool값 초기화

			return;
		}
	}
}

void AFGPlayerController::Server_ConfirmTargetingFunction_Implementation()
{
	ConfirmTargetingFunction();
}

void AFGPlayerController::LeftMouseReleased()
{
	bLeftMousePressed = false;

	VFGRTSInterface->MarqueeReleased();

	if (VFGCharacterPlayer && VFGAbilitySystemComponent && VFGAbilitySystemComponent->HasMatchingGameplayTag(LeftMousePressedOwnedTag))
	{
		VFGAbilitySystemComponent->RemoveLooseGameplayTag(LeftMousePressedOwnedTag);
		VFGAbilitySystemComponent->RemoveReplicatedLooseGameplayTag(LeftMousePressedOwnedTag);
	}
}

void AFGPlayerController::RightMousePressed()
{
	CancelTargetingFunctionBundle();
	//SpellIndicator 이후 취소

	FHitResult HitResult;
	bool bHit = GetHitResultUnderCursorByChannel(UEngineTypes::ConvertToTraceType(ECC_Visibility), true, HitResult);
	FVector RightClickLocation = HitResult.Location;

	RightMousePressedContents(RightClickLocation);
}

void AFGPlayerController::CancelTargetingFunctionBundle()
{
	if (!IsValid(VGameplayAbilityTargetActor)) return;

	//AGameplayAbilityTargetActor의 CancelTargeting를 실행시키기 위한 설정
	if (HasAuthority())
	{
		CancelTargetingFunction();
	}
	else
	{
		Server_CancelTargetingFunction();
	}

	//DynamicMesh 제거
	if (IsValid(VFGCharacterPlayer))
	{
		VFGCharacterPlayer->ResetDynamicMesh();
		VFGCharacterPlayer->ResetAroundDynamicMesh();
	}

	GetWorld()->GetTimerManager().ClearTimer(SetMouseCursorPositionTimer);
	//스킬 범위 표시기 회전 타이머 제거

	VGameplayAbilityTargetActor = nullptr;
	//초기화
}

void AFGPlayerController::CancelTargetingFunction()
{
	if (IsValid(VFGCharacterPlayer))
	{
		AGameplayAbilityTargetActor* GGameplayAbilityTargetActor = VFGCharacterPlayer->GetVGameplayAbilityTargetActor();
		if (IsValid(GGameplayAbilityTargetActor))
		{
			GGameplayAbilityTargetActor->CancelTargeting();
			VFGCharacterPlayer->SetGameplayAbilityTargetActor(nullptr);
			VFGCharacterPlayer->SetbAroundSpellIndicator(false);
			//사용 가능 범위 bool값 초기화

			return;
		}
	}
}

void AFGPlayerController::Server_CancelTargetingFunction_Implementation()
{
	CancelTargetingFunction();
}

void AFGPlayerController::RightMousePressedContents(const FVector& InMouseLocation)
{
	if (bLeftMousePressed) return;

	if (VFGCharacterPlayer)
	{
		for (const FGameplayTag& Tag : MoveBlockedTags)
		{
			if (VFGAbilitySystemComponent && VFGAbilitySystemComponent->HasMatchingGameplayTag(Tag)) return;
		}
	}

	//이펙트 추가
	if (!bRightMousePressed)
	{
		bRightMousePressed = true;

		SpawnClickMarker(InMouseLocation, FLinearColor::Blue);
	}

	for (const auto& SelectedCharacterElement : VFGRTSInterface->SelectedCharacterArray())
	{
		if (HasAuthority())
		{
			RightMousePressedFunction(SelectedCharacterElement, InMouseLocation);
		}
		else
		{
			Server_RightMousePressed(SelectedCharacterElement, InMouseLocation);
		}
	}

	if (VFGCharacterPlayer && VFGAbilitySystemComponent && !VFGAbilitySystemComponent->HasMatchingGameplayTag(MoveOwnedTag))
	{
		VFGAbilitySystemComponent->AddLooseGameplayTag(MoveOwnedTag);
		VFGAbilitySystemComponent->AddReplicatedLooseGameplayTag(MoveOwnedTag);
	}
}

void AFGPlayerController::RightMousePressedFunction(AFGCharacterPlayer* InFGCharacterPlayer, FVector InLocation)
{	
	if (!InFGCharacterPlayer) return;

	AFGAIController* AIController = Cast<AFGAIController>(InFGCharacterPlayer->GetController());
	if (AIController)
	{
		FAIMoveRequest MoveRequest;
		MoveRequest.SetGoalLocation(InLocation);
		MoveRequest.SetAcceptanceRadius(5.0f);

		FNavPathSharedPtr Path;
		AIController->StopAI();
		AIController->MoveTo(MoveRequest, &Path);
	}
}

void AFGPlayerController::Server_RightMousePressed_Implementation(AFGCharacterPlayer* InFGCharacterPlayer, FVector InLocation)
{
	RightMousePressedFunction(InFGCharacterPlayer, InLocation);
}

void AFGPlayerController::SpawnClickMarker(const FVector& InMouseLocation, const FLinearColor& InLinearColor)
{
	if (!IsLocalPlayerController()) return;

	FTransform SpawnTransform(FRotator::ZeroRotator, InMouseLocation);
	AFGClickMarker* SFGClickMarker = GetWorld()->SpawnActorDeferred<AFGClickMarker>(ClickMarkerClass, SpawnTransform, this, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (IsValid(SFGClickMarker))
	{
		SFGClickMarker->ClickMarkerLinearColor = InLinearColor;

		UGameplayStatics::FinishSpawningActor(SFGClickMarker, SpawnTransform);
	}
}

void AFGPlayerController::RightMouseReleased()
{
	bRightMousePressed = false;

	//if (HasAuthority())
	//{
	//}
	//else
	//{
	//	Server_RightMouseReleased();
	//}

	if (VFGCharacterPlayer && VFGAbilitySystemComponent && VFGAbilitySystemComponent->HasMatchingGameplayTag(MoveOwnedTag))
	{
		VFGAbilitySystemComponent->RemoveLooseGameplayTag(MoveOwnedTag);
		VFGAbilitySystemComponent->AddReplicatedLooseGameplayTag(MoveOwnedTag);
	}
}

void AFGPlayerController::Server_RightMouseReleased_Implementation()
{
}

void AFGPlayerController::ChangePossess()
{
	if (!VFGCharacterPlayer || !RTSPawn || !(VFGRTSInterface->SelectedCharacterArray().Num() == 1) || VFGCharacterPlayer->GetbDead() == true) return;

	if (VFGCharacterPlayer)
	{
		for (const FGameplayTag& Tag : ChangePossessBlockedTags)
		{
			if (VFGAbilitySystemComponent && VFGAbilitySystemComponent->HasMatchingGameplayTag(Tag)) return;
		}
	}

	//ChangePossess 상태 태그
	if (IsValid(VFGAbilitySystemComponent) && !VFGAbilitySystemComponent->HasMatchingGameplayTag(ChangePossessOwnedTag))
	{
		VFGAbilitySystemComponent->AddLooseGameplayTag(ChangePossessOwnedTag);
		VFGAbilitySystemComponent->AddReplicatedLooseGameplayTag(ChangePossessOwnedTag);
	}

	GetWorld()->GetTimerManager().SetTimer(ChangePossessStateTimer, FTimerDelegate::CreateLambda([&]()
		{
			if (IsValid(VFGAbilitySystemComponent) && VFGAbilitySystemComponent->HasMatchingGameplayTag(ChangePossessOwnedTag))
			{
				VFGAbilitySystemComponent->RemoveLooseGameplayTag(ChangePossessOwnedTag);
				VFGAbilitySystemComponent->AddReplicatedLooseGameplayTag(ChangePossessOwnedTag);
			}

			GetWorld()->GetTimerManager().ClearTimer(ChangePossessStateTimer);
		}
	), FGRTSCameraBlendChnageTime, false);

	//ChangePossess
	if (bCharacterMode)
	{
		FocusCharacter();

		//FInputModeGameAndUI GameAndUIInputMode;
		//GameAndUIInputMode.SetHideCursorDuringCapture(false);
		//GameAndUIInputMode.SetLockMouseToViewportBehavior(EMouseLockMode::LockAlways);
		//SetInputMode(GameAndUIInputMode);
		//SetShowMouseCursor(true);

		InputModeGameAndUIFunction();

		if (VFGCommonHUD)
		{
			VFGCommonHUD->SetMiniMapVisibility(ESlateVisibility::Visible);
		}
	}
	else
	{
		FInputModeGameOnly GameOnlyInputMode;
		SetInputMode(GameOnlyInputMode);
		SetShowMouseCursor(false);

		if (VFGCommonHUD)
		{
			VFGCommonHUD->SetMiniMapVisibility(ESlateVisibility::Collapsed);
		}
	}

	if (HasAuthority())
	{
		ChangePossessFunction(VFGCharacterPlayer, RTSPawn);
	}
	else
	{
		Server_ChangePossess(VFGCharacterPlayer, RTSPawn);
	}

	if (!bCharacterMode)
	{
		SetControlRotation(VFGCharacterPlayer->GetActorRotation());
	}
}

void AFGPlayerController::ChangePossessFunction(AFGCharacterPlayer* InFGCharacterPlayer, APawn* InFGRTSCamera)
{
	if (!IsValid(InFGCharacterPlayer) || !IsValid(InFGRTSCamera)) return;

	if (bCharacterMode)
	{
		RTSCameraPossess(InFGCharacterPlayer, InFGRTSCamera);
	}
	else
	{
		SetViewTargetWithBlend(InFGCharacterPlayer, FGRTSCameraBlendChnageTime);

		GetWorld()->GetTimerManager().SetTimer(InFGCharacterPlayerBlendTimer, FTimerDelegate::CreateLambda([this, InFGCharacterPlayer]()
			{
				AFGAIController* AIController = Cast<AFGAIController>(InFGCharacterPlayer->GetController());
				if (AIController)
				{
					AIController->StopAI();
				}

				Possess(InFGCharacterPlayer);

				if (HasAuthority())
				{
					Client_CreateConsoleCommand(InFGCharacterPlayer);
				}

				bCharacterMode = true;

				bLookAt = true;
				LookAtAllFGChildWidgetComponent(bLookAt);

				GetWorld()->GetTimerManager().ClearTimer(InFGCharacterPlayerBlendTimer);
			}
		), FGRTSCameraBlendChnageTime, false);
	}
}

void AFGPlayerController::Server_ChangePossess_Implementation(AFGCharacterPlayer* InFGCharacterPlayer, APawn* InFGRTSCamera)
{
	ChangePossessFunction(InFGCharacterPlayer, InFGRTSCamera);
}

void AFGPlayerController::Client_RemoveConsoleCommand_Implementation(AFGCharacterPlayer* InFGCharacterPlayer)
{
	if (IsValid(InFGCharacterPlayer) && InFGCharacterPlayer->GetbGASDebug())
	{
		ConsoleCommand(TEXT("showdebug none"));
	}
}

void AFGPlayerController::Client_CreateConsoleCommand_Implementation(AFGCharacterPlayer* InFGCharacterPlayer)
{
	if (IsValid(InFGCharacterPlayer) && InFGCharacterPlayer->GetbGASDebug())
	{
		ConsoleCommand(TEXT("showdebug abilitysystem"));
	}
}

void AFGPlayerController::RTSCameraPossess(AFGCharacterPlayer* InFGCharacterPlayer, APawn* InFGRTSCamera)
{
	if (!IsValid(InFGCharacterPlayer) || !IsValid(InFGRTSCamera)) return;

	SetViewTargetWithBlend(InFGRTSCamera, FGRTSCameraBlendChnageTime);

	GetWorld()->GetTimerManager().SetTimer(InFGRTSCameraBlendTimer, FTimerDelegate::CreateLambda([this, InFGCharacterPlayer, InFGRTSCamera]()
		{
			Possess(InFGRTSCamera);

			if (HasAuthority())
			{
				Client_RemoveConsoleCommand(InFGCharacterPlayer);
			}

			bCharacterMode = false;

			bLookAt = false;
			LookAtAllFGChildWidgetComponent(bLookAt);

			GetWorld()->GetTimerManager().ClearTimer(InFGRTSCameraBlendTimer);
		}
	), FGRTSCameraBlendChnageTime, false);
}

void AFGPlayerController::RTSAttack()
{
	FHitResult HitResult;
	bool bHit = GetHitResultUnderCursorByChannel(UEngineTypes::ConvertToTraceType(ECC_Visibility), true, HitResult);
	FVector AttackLocation = HitResult.Location;

	RTSAttackContents(AttackLocation);
}

void AFGPlayerController::RTSAttackContents(const FVector& InMouseLocation)
{
	if (VFGCharacterPlayer)
	{
		for (const FGameplayTag& Tag : RTSAttackBlockedTags)
		{
			if (VFGAbilitySystemComponent && VFGAbilitySystemComponent->HasMatchingGameplayTag(Tag)) return;
		}
	}

	//이펙트 추가
	SpawnClickMarker(InMouseLocation, FLinearColor::Red);

	for (const auto& SelectedCharacterElement : VFGRTSInterface->SelectedCharacterArray())
	{
		SetMouseCursorPosition(SelectedCharacterElement);

		if (HasAuthority())
		{
			RTSAttackFunction(SelectedCharacterElement, InMouseLocation);
		}
		else
		{
			Server_RTSAttack(SelectedCharacterElement, InMouseLocation);
		}
	}
}

void AFGPlayerController::RTSAttackFunction(AFGCharacterPlayer* InFGCharacterPlayer, FVector InLocation)
{
	AFGAIController* AIController = Cast<AFGAIController>(InFGCharacterPlayer->GetController());
	if (AIController)
	{
		AIController->StopAI();
		AIController->SetMouseLocation(InLocation);
		AIController->RunAI(BTATtack);
	}
}

void AFGPlayerController::Server_RTSAttack_Implementation(AFGCharacterPlayer* InFGCharacterPlayer, FVector InLocation)
{
	RTSAttackFunction(InFGCharacterPlayer, InLocation);
}

void AFGPlayerController::RTSStop()
{
	for (const auto& SelectedCharacterElement : VFGRTSInterface->SelectedCharacterArray())
	{
		if (HasAuthority())
		{
			RTSStopFunction(SelectedCharacterElement);
		}
		else
		{
			Server_RTSStop(SelectedCharacterElement);
		}
	}
}

void AFGPlayerController::RTSStopFunction(AFGCharacterPlayer* InFGCharacterPlayer)
{
	AFGAIController* AIController = Cast<AFGAIController>(InFGCharacterPlayer->GetController());
	if (AIController)
	{
		AIController->StopAI();
		AIController->StopMovement();
	}
}

void AFGPlayerController::Server_RTSStop_Implementation(AFGCharacterPlayer* InFGCharacterPlayer)
{
	RTSStopFunction(InFGCharacterPlayer);
}

void AFGPlayerController::RTSHold()
{
	if (VFGCharacterPlayer)
	{
		for (const FGameplayTag& Tag : RTSAttackBlockedTags)
		{
			if (VFGAbilitySystemComponent && VFGAbilitySystemComponent->HasMatchingGameplayTag(Tag)) return;
		}
	}

	for (const auto& SelectedCharacterElement : VFGRTSInterface->SelectedCharacterArray())
	{
		if (HasAuthority())
		{
			RTSHoldFunction(SelectedCharacterElement);
		}
		else
		{
			Server_RTSHold(SelectedCharacterElement);
		}
	}
}

void AFGPlayerController::RTSHoldFunction(AFGCharacterPlayer* InFGCharacterPlayer)
{
	AFGAIController* AIController = Cast<AFGAIController>(InFGCharacterPlayer->GetController());
	if (AIController)
	{
		AIController->StopAI();
		AIController->StopMovement();
		AIController->RunAI(BTHold);
	}
}

void AFGPlayerController::Server_RTSHold_Implementation(AFGCharacterPlayer* InFGCharacterPlayer)
{
	RTSHoldFunction(InFGCharacterPlayer);
}

void AFGPlayerController::RTSPatrol()
{
	FHitResult HitResult;
	bool bHit = GetHitResultUnderCursorByChannel(UEngineTypes::ConvertToTraceType(ECC_Visibility), true, HitResult);
	FVector AttackLocation = HitResult.Location;

	RTSPatrolContents(AttackLocation);
}

void AFGPlayerController::RTSPatrolContents(const FVector& InMouseLocation)
{
	if (VFGCharacterPlayer)
	{
		for (const FGameplayTag& Tag : RTSAttackBlockedTags)
		{
			if (VFGAbilitySystemComponent && VFGAbilitySystemComponent->HasMatchingGameplayTag(Tag)) return;
		}
	}

	//이펙트 추가
	SpawnClickMarker(InMouseLocation, FLinearColor::Yellow);

	for (const auto& SelectedCharacterElement : VFGRTSInterface->SelectedCharacterArray())
	{
		SetMouseCursorPosition(SelectedCharacterElement);

		if (HasAuthority())
		{
			RTSPatrolFunction(SelectedCharacterElement, InMouseLocation);
		}
		else
		{
			Server_RTSPatrol(SelectedCharacterElement, InMouseLocation);
		}
	}
}

void AFGPlayerController::RTSPatrolFunction(AFGCharacterPlayer* InFGCharacterPlayer, const FVector& InMouseLocation)
{
	AFGAIController* AIController = Cast<AFGAIController>(InFGCharacterPlayer->GetController());
	if (AIController)
	{
		AIController->StopAI();
		AIController->SetMouseLocation(InMouseLocation);
		AIController->RunAI(BTPatrol);
	}
}

void AFGPlayerController::Server_RTSPatrol_Implementation(AFGCharacterPlayer* InFGCharacterPlayer, const FVector& InMouseLocation)
{
	RTSPatrolFunction(InFGCharacterPlayer, InMouseLocation);
}

void AFGPlayerController::FocusCharacter()
{
	if (VFGCharacterPlayer)
	{
		FVector FGCharacterPlayerLocation = VFGCharacterPlayer->GetActorLocation();
		if (RTSPawn)
		{
			RTSPawn->SetActorLocation(FGCharacterPlayerLocation);
		}
	}
}

void AFGPlayerController::RTSPawnPossessDueToDead()
{
	if (bCharacterMode)
	{
		FocusCharacter();

		if (VFGCommonHUD)
		{
			VFGCommonHUD->SetMiniMapVisibility(ESlateVisibility::Visible);
		}

		if (HasAuthority())
		{
			RTSCameraPossess(VFGCharacterPlayer, RTSPawn);
		}
		else
		{
			Server_RTSCameraPossess(VFGCharacterPlayer, RTSPawn);
		}
	}
}

void AFGPlayerController::Server_RTSCameraPossess_Implementation(AFGCharacterPlayer* InFGCharacterPlayer, APawn* InFGRTSCamera)
{
	RTSCameraPossess(InFGCharacterPlayer, InFGRTSCamera);
}

void AFGPlayerController::LookAtAllFGChildWidgetComponent(bool InbLookAt)
{
	if (HasAuthority())
	{
		Client_LookAtAllFGChildWidgetComponent(InbLookAt);
	}
	else
	{
		Server_LookAtAllFGChildWidgetComponent(InbLookAt);
	}
}

void AFGPlayerController::LookAtAllFGChildWidgetComponentFunction(bool InbLookAt)
{
	for (TActorIterator<AFGCharacterBase> It(GetWorld()); It; ++It)
	{
		AFGCharacterBase* FGCharacterBase = *It;
		if (FGCharacterBase)
		{
			AFGChildWidgetComponent* FGChildWidgetComponent = FGCharacterBase->GetFGChildWidgetComponent();
			if (FGChildWidgetComponent)
			{
				FGChildWidgetComponent->SetbLookAt(InbLookAt);
				FGChildWidgetComponent->SetAbsoluteWidgetComponent(InbLookAt);
			}
		}
	}

	for (TActorIterator<AFGNexus> It(GetWorld()); It; ++It)
	{
		AFGNexus* FGNexus = *It;
		if (FGNexus)
		{
			AFGChildWidgetComponent* FGChildWidgetComponent = FGNexus->GetFGChildWidgetComponent();
			if (FGChildWidgetComponent)
			{
				FGChildWidgetComponent->SetbLookAt(InbLookAt);
				FGChildWidgetComponent->SetAbsoluteWidgetComponent(InbLookAt);
			}
		}
	}
}

void AFGPlayerController::Server_LookAtAllFGChildWidgetComponent_Implementation(bool InbLookAt)
{
	Client_LookAtAllFGChildWidgetComponent(InbLookAt);
}

void AFGPlayerController::Client_LookAtAllFGChildWidgetComponent_Implementation(bool InbLookAt)
{
	LookAtAllFGChildWidgetComponentFunction(InbLookAt);
}

void AFGPlayerController::DeadLookAtAllFGChildWidgetComponent()
{
	LookAtAllFGChildWidgetComponent(bLookAt);
}

void AFGPlayerController::SelectCharacter1()
{
	if (!IsLocalPlayerController()) return;

	SelectCharacterCount1++;
	SelectCharacterCount2 = 0;
	SelectCharacterCount3 = 0;

	if (IsValid(VFGHUD))
	{
		VFGHUD->HUDSelectCharacter(0);
	}

	if (IsValid(VFGSelectedCharacterHpMp))
	{
		VFGSelectedCharacterHpMp->SelectCharacter(0);
	}

	if (SelectCharacterCount1 >= 2)
	{
		FocusCharacter();

		SelectCharacterCount1 = 0;
	}
}

void AFGPlayerController::Client_SelectCharacter1_Implementation()
{
	GetWorld()->GetTimerManager().SetTimer(SelectCharacter1TimerHandle, FTimerDelegate::CreateLambda([&]()
		{
			SelectCharacter1();

			GetWorld()->GetTimerManager().ClearTimer(SelectCharacter1TimerHandle);
		}
	), 0.5f, false);
}

void AFGPlayerController::SelectCharacter2()
{
	if (!IsLocalPlayerController()) return;

	SelectCharacterCount1 = 0;
	SelectCharacterCount2++;
	SelectCharacterCount3 = 0;

	if (IsValid(VFGHUD))
	{
		VFGHUD->HUDSelectCharacter(1);
	}

	if (IsValid(VFGSelectedCharacterHpMp))
	{
		VFGSelectedCharacterHpMp->SelectCharacter(1);
	}

	if (SelectCharacterCount2 >= 2)
	{
		FocusCharacter();

		SelectCharacterCount2 = 0;
	}
}

void AFGPlayerController::SelectCharacter3()
{
	if (!IsLocalPlayerController()) return;

	SelectCharacterCount1 = 0;
	SelectCharacterCount2 = 0;
	SelectCharacterCount3++;

	if (IsValid(VFGHUD))
	{
		VFGHUD->HUDSelectCharacter(2);
	}

	if (IsValid(VFGSelectedCharacterHpMp))
	{
		VFGSelectedCharacterHpMp->SelectCharacter(2);
	}

	if (SelectCharacterCount3 >= 2)
	{
		FocusCharacter();

		SelectCharacterCount3 = 0;
	}
}

void AFGPlayerController::SelectAllCharacter()
{
	if (!IsLocalPlayerController()) return;

	if (IsValid(VFGHUD))
	{
		VFGHUD->HUDSelectAllCharacter();
	}

	if (IsValid(VFGSelectedCharacterHpMp))
	{
		VFGSelectedCharacterHpMp->SelectAllCharacter();
	}
}

void AFGPlayerController::CreatePlayerWidgets()
{
	if (!IsLocalPlayerController()) return;

	//미니맵 위젯
	VFGCommonHUD = CreateWidget<UFGCommonHUD>(this, GetUserWidgetClassByName(TEXT("UFGCommonHUD")));
	if (IsValid(VFGCommonHUD))
	{
		VFGCommonHUD->AddToViewport(10);
	}

	//왼쪽 캐릭터 상태
	VFGSelectedCharacterHpMp = CreateWidget<UFGSelectedCharacterHpMp>(this, GetUserWidgetClassByName(TEXT("UFGSelectedCharacterHpMp")));
	if (IsValid(VFGSelectedCharacterHpMp))
	{
		VFGSelectedCharacterHpMp->AddToViewport(10);
	}
}

void AFGPlayerController::StartCharacterRespawnCoolTimeSetting(const int32& InSelectCharacterNumber, const float& InCharacterRespawnTime)
{
	if (HasAuthority())
	{
		Client_StartCharacterRespawnCoolTimeSetting(InSelectCharacterNumber, InCharacterRespawnTime);
	}
}

void AFGPlayerController::Client_StartCharacterRespawnCoolTimeSetting_Implementation(const int32& InSelectCharacterNumber, const float& InCharacterRespawnTime)
{
	IFGSelectedCharacterHpMpInterface* FGSelectedCharacterHpMpInterface = Cast<IFGSelectedCharacterHpMpInterface>(VFGSelectedCharacterHpMp);
	if (FGSelectedCharacterHpMpInterface)
	{
		FGSelectedCharacterHpMpInterface->CharacterRespawnCoolTimeSettingInterface(InSelectCharacterNumber, InCharacterRespawnTime);
	}
}

void AFGPlayerController::BuyItem(AFGShop* InFGShop, AFGCharacterPlayer* InFGCharacterPlayer, int32 InSlotIndex, int32 InDropIndex, int32 InItemQuantity)
{
	if (HasAuthority())
	{
		UFGShopComponent* FGShopComponent = InFGShop->GetFGShopComponent();
		if (IsValid(FGShopComponent))
		{
			FGShopComponent->BuyItem(InFGCharacterPlayer, InSlotIndex, InDropIndex, InItemQuantity);
		}
	}
	else
	{
		Server_BuyItem(InFGShop, InFGCharacterPlayer, InSlotIndex, InDropIndex, InItemQuantity);
	}
}

void AFGPlayerController::Server_BuyItem_Implementation(AFGShop* InFGShop, AFGCharacterPlayer* InFGCharacterPlayer, int32 InSlotIndex, int32 InDropIndex, int32 InItemQuantity)
{
	UFGShopComponent* FGShopComponent = InFGShop->GetFGShopComponent();
	if (IsValid(FGShopComponent))
	{
		FGShopComponent->BuyItem(InFGCharacterPlayer, InSlotIndex, InDropIndex, InItemQuantity);
	}
}

void AFGPlayerController::SellItem(AFGShop* InFGShop, AFGCharacterPlayer* InFGCharacterPlayer, int32 InSlotIndex, int32 InItemQuantity)
{
	if (HasAuthority())
	{
		UFGInventoryComponent* FGInventoryComponent = InFGCharacterPlayer->GetFGInventoryComponent();
		if (IsValid(FGInventoryComponent))
		{
			FGInventoryComponent->SellItem(InFGShop, InFGCharacterPlayer, InSlotIndex, InItemQuantity);
		}
	}
	else
	{
		Server_SellItem(InFGShop, InFGCharacterPlayer, InSlotIndex, InItemQuantity);
	}
}

void AFGPlayerController::Server_SellItem_Implementation(AFGShop* InFGShop, AFGCharacterPlayer* InFGCharacterPlayer, int32 InSlotIndex, int32 InItemQuantity)
{
	UFGInventoryComponent* FGInventoryComponent = InFGCharacterPlayer->GetFGInventoryComponent();
	if (IsValid(FGInventoryComponent))
	{
		FGInventoryComponent->SellItem(InFGShop, InFGCharacterPlayer, InSlotIndex, InItemQuantity);
	}
}

void AFGPlayerController::OnUseInventoryItemAction(const FInputActionInstance& Instance)
{
	const UInputAction* InputAction = Instance.GetSourceAction();
	if (IsValid(InputAction))
	{
		int32 i = 0;
		for (const auto& InventoryInputAction : InventoryInputActionArray)
		{
			if (InputAction == InventoryInputAction)
			{
				UseInventoryItem(VFGCharacterPlayer, i);

				return;
			}

			++i;
		}
	}
}

void AFGPlayerController::UseInventoryItem(AFGCharacterPlayer* InFGCharacterPlayer, int32 InSlotIndex)
{
	if (HasAuthority())
	{
		UFGInventoryComponent* FGInventoryComponent = InFGCharacterPlayer->GetFGInventoryComponent();
		if (IsValid(FGInventoryComponent))
		{
			FGInventoryComponent->UseItem(InFGCharacterPlayer, InSlotIndex);
		}
	}
	else
	{
		Server_UseInventoryItem(InFGCharacterPlayer, InSlotIndex);
	}
}

void AFGPlayerController::Server_UseInventoryItem_Implementation(AFGCharacterPlayer* InFGCharacterPlayer, int32 InSlotIndex)
{
	UFGInventoryComponent* FGInventoryComponent = InFGCharacterPlayer->GetFGInventoryComponent();
	if (IsValid(FGInventoryComponent))
	{
		FGInventoryComponent->UseItem(InFGCharacterPlayer, InSlotIndex);
	}
}

UEnhancedInputUserSettings* AFGPlayerController::GetEnhancedInputUserSettings() const
{
	ULocalPlayer* LocalPlayer = GetLocalPlayer();
	if (IsValid(LocalPlayer))
	{
		UEnhancedInputLocalPlayerSubsystem* EnhancedInputLocalPlayerSubsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer);
		if (IsValid(EnhancedInputLocalPlayerSubsystem))
		{
			//에디터에서 프로젝트 세팅 - 향상된 입력 - User Settings - 사용자 세팅 활성화 체크 해줘야 사용 가능
			UEnhancedInputUserSettings* EnhancedInputUserSettings = EnhancedInputLocalPlayerSubsystem->GetUserSettings();
			if (EnhancedInputUserSettings)
			{
				return EnhancedInputUserSettings;
			}
		}
	}

	return nullptr;
}

void AFGPlayerController::RegisterInputMappingContextSetting()
{
	UEnhancedInputUserSettings* EnhancedInputUserSettings = GetEnhancedInputUserSettings();
	if (!IsValid(EnhancedInputUserSettings)) return;

	for (const auto& ControlChange : ControlChangeMap)
	{
		if (!EnhancedInputUserSettings->IsMappingContextRegistered(ControlChange.Value))
		{
			EnhancedInputUserSettings->RegisterInputMappingContext(ControlChange.Value);
		}
	}
}

void AFGPlayerController::CreateFGSettingsWidget()
{
	if (!bCreateFGSettingsWidget)
	{
		bCreateFGSettingsWidget = true;

		if (IsLocalPlayerController())
		{
			if (IsValid(VFGSettings) && VFGSettings->IsInViewport())
			{
				VFGSettings->SetVisibility(ESlateVisibility::Visible);
			}
			else
			{
				VFGSettings = CreateWidget<UFGSettings>(this, GetUserWidgetClassByName(TEXT("UFGSettings")));
				if (IsValid(VFGSettings) && !VFGSettings->IsInViewport())
				{
					VFGSettings->AddToViewport(999);
				}
			}
		}

		FInputModeUIOnly InputModeUIOnly;
		InputModeUIOnly.SetWidgetToFocus(VFGSettings->TakeWidget());
		InputModeUIOnly.SetLockMouseToViewportBehavior(EMouseLockMode::LockAlways);
		SetInputMode(InputModeUIOnly);
		SetShowMouseCursor(true);
	}
	else
	{
		bCreateFGSettingsWidget = false;

		if (IsValid(VFGSettings) && VFGSettings->IsInViewport())
		{
			VFGSettings->SetVisibility(ESlateVisibility::Collapsed);
		}

		if (bCharacterMode)
		{
			FInputModeGameOnly GameOnlyInputMode;
			SetInputMode(GameOnlyInputMode);
			SetShowMouseCursor(false);
		}
		else
		{
			//버튼으로 종료 후 다시 위젯 생성 시 두번 눌러야 해서 GameOnly 설정 후 GameAndUI로 전환
			FInputModeGameOnly GameOnlyInputMode;
			SetInputMode(GameOnlyInputMode);

			//FInputModeGameAndUI GameAndUIInputMode;
			//GameAndUIInputMode.SetHideCursorDuringCapture(false);
			//GameAndUIInputMode.SetWidgetToFocus(nullptr);
			//GameAndUIInputMode.SetLockMouseToViewportBehavior(EMouseLockMode::LockAlways);
			//SetInputMode(GameAndUIInputMode);

			InputModeGameAndUIFunction();
		}
	}
}

void AFGPlayerController::InstantCastSetting(UFGAbilitySystemComponent* InFGAbilitySystemComponent, TSubclassOf<UGameplayAbility> InGameplayAbilityClass, bool InbInstantCast)
{
	if (HasAuthority())
	{
		InstantCastSettingFunction(InFGAbilitySystemComponent, InGameplayAbilityClass, InbInstantCast);
	}
	else
	{
		Server_InstantCastSetting(InFGAbilitySystemComponent, InGameplayAbilityClass, InbInstantCast);
	}
}

void AFGPlayerController::InstantCastSettingFunction(UFGAbilitySystemComponent* InFGAbilitySystemComponent, TSubclassOf<UGameplayAbility> InGameplayAbilityClass, bool InbInstantCast)
{
	if (!IsValid(InGameplayAbilityClass) || !IsValid(InFGAbilitySystemComponent)) return;

	FGameplayAbilitySpec* Spec = InFGAbilitySystemComponent->FindAbilitySpecFromClass(InGameplayAbilityClass);
	if (!Spec) return;

	UGameplayAbility* GGameplayAbility = Spec->GetPrimaryInstance();
	if (!IsValid(GGameplayAbility)) return;

	UFGGameplayAbility* CFGGameplayAbility = Cast<UFGGameplayAbility>(GGameplayAbility);
	if (IsValid(CFGGameplayAbility))
	{
		CFGGameplayAbility->SetbInstantCast(InbInstantCast);
	}
}

void AFGPlayerController::Server_InstantCastSetting_Implementation(UFGAbilitySystemComponent* InFGAbilitySystemComponent, TSubclassOf<UGameplayAbility> InGameplayAbilityClass, bool InbInstantCast)
{
	InstantCastSettingFunction(InFGAbilitySystemComponent, InGameplayAbilityClass, InbInstantCast);
}

void AFGPlayerController::IncreaseSensitivity()
{
	APawn* CurrentPawn = GetPawn();
	if (IsValid(CurrentPawn))
	{
		if (IFGMouseSensitivityInterface* CFGMouseSensitivityInterface = Cast<IFGMouseSensitivityInterface>(CurrentPawn))
		{
			float MouseSensitivityValue = FMath::Clamp(CFGMouseSensitivityInterface->GetMouseSensitivity() + 0.1f, 0.1f, 1.9f);
			CFGMouseSensitivityInterface->SetMouseSensitivity(MouseSensitivityValue);

			if (IsValid(VFGSelectedCharacterHpMp))
			{
				VFGSelectedCharacterHpMp->SensitivityWidgetSetting(MouseSensitivityValue);
			}
		}
	}
}

void AFGPlayerController::DecreaseSensitivity()
{
	APawn* CurrentPawn = GetPawn();
	if (IsValid(CurrentPawn))
	{
		if (IFGMouseSensitivityInterface* CFGMouseSensitivityInterface = Cast<IFGMouseSensitivityInterface>(CurrentPawn))
		{
			float MouseSensitivityValue = FMath::Clamp(CFGMouseSensitivityInterface->GetMouseSensitivity() - 0.1f, 0.1f, 1.9f);
			CFGMouseSensitivityInterface->SetMouseSensitivity(MouseSensitivityValue);

			if (IsValid(VFGSelectedCharacterHpMp))
			{
				VFGSelectedCharacterHpMp->SensitivityWidgetSetting(MouseSensitivityValue);
			}
		}
	}
}

void AFGPlayerController::Client_TeamScoreWidgetFunction_Implementation(float InTeamAScore, float InTeamBScore, int32 InTeamIndex, UTexture2D* InKillerImage, UTexture2D* InTargetImage, const FString& InKillerNickname, const FString& InTargetNickname)
{
	if (IsValid(VFGSelectedCharacterHpMp))
	{
		VFGSelectedCharacterHpMp->TeamScoreWidgetSetting(InTeamAScore, InTeamBScore);
		VFGSelectedCharacterHpMp->AddFGKilllogWidgetSetting(InTeamIndex, InKillerImage, InTargetImage, InKillerNickname, InTargetNickname);
	}
}

TSubclassOf<UUserWidget> AFGPlayerController::GetUserWidgetClassByName(const FName& InName) const
{
	if (const TSubclassOf<UUserWidget>* FindUserWidgetClass = UserWidgetClassMap.Find(InName))
	{
		return *FindUserWidgetClass;
	}

	UE_LOG(LogTemp, Error, TEXT("InName is not found in GetUserWidgetClassByName of %s"), *GetNameSafe(this));

	return nullptr;
}

UInputAction* AFGPlayerController::GetInputActionByName(const FName& InName) const
{
	if (const auto* FindInputAction = InputActionMap.Find(InName))
	{
		return *FindInputAction;
	}

	UE_LOG(LogTemp, Error, TEXT("InName is not found in GetInputActionMap of %s"), *GetNameSafe(this));

	return nullptr;
}

//Fog of War
void AFGPlayerController::RenderTargetSetting()
{
	if (!IsValid(VFGGameState)) return;

	int32 RowsValue = VFGGameState->GetRows();
	int32 ColumnsValue = VFGGameState->GetColumns();
	if (RowsValue < 1 || ColumnsValue < 1)
	{
		UE_LOG(LogTemp, Error, TEXT("RowsValue, ColumnsValue are not valid in RenderTargetSetting of %s"), *GetNameSafe(this));

		return;
	}

	VTextureRenderTarget2D = NewObject<UTextureRenderTarget2D>(this);
	if (IsValid(VTextureRenderTarget2D))
	{
		VTextureRenderTarget2D->InitCustomFormat(ColumnsValue, RowsValue, PF_B8G8R8A8, false);
		VTextureRenderTarget2D->ClearColor = FLinearColor::Black;
	}

	//Fog of War
	UMaterialInterface* GMaterialInterface = VFGGameState->GetFogOfWarMaterialInterface();
	if (IsValid(GMaterialInterface))
	{
		UMaterialInstanceDynamic* CMaterialInstanceDynamic = UMaterialInstanceDynamic::Create(GMaterialInterface, this);
		if (IsValid(CMaterialInstanceDynamic))
		{
			CMaterialInstanceDynamic->SetTextureParameterValue(TEXT("FogOfWar"), VTextureRenderTarget2D);
			CMaterialInstanceDynamic->SetScalarParameterValue(TEXT("Rows"), static_cast<float>(RowsValue));
			CMaterialInstanceDynamic->SetScalarParameterValue(TEXT("Columns"), static_cast<float>(ColumnsValue));
			CMaterialInstanceDynamic->SetScalarParameterValue(TEXT("GridSize"), VFGGameState->GetGridSize());

			//포스트 프로세스
			//맵의 머티리얼 넣는곳에 어무것도 넣으면 안된다.(코드로 해결해야 설정됨)
			AActor* PostProcessActor = UGameplayStatics::GetActorOfClass(GetWorld(), APostProcessVolume::StaticClass());
			if (!IsValid(PostProcessActor)) return;

			APostProcessVolume* CPostProcessVolume = Cast<APostProcessVolume>(PostProcessActor);
			if (!IsValid(CPostProcessVolume)) return;

			CPostProcessVolume->Settings.AddBlendable(CMaterialInstanceDynamic, 1.f);

			//데칼
			//AActor* GDecalActor = UGameplayStatics::GetActorOfClass(GetWorld(), ADecalActor::StaticClass());
			//if (!IsValid(GDecalActor)) return;

			//ADecalActor* CDecalActor = Cast<ADecalActor>(GDecalActor);
			//if (!IsValid(CDecalActor)) return;

			//CDecalActor->SetDecalMaterial(CMaterialInstanceDynamic);
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("GMaterialInterface is not valid in RenderTargetSetting of %s"), *GetNameSafe(this));

		return;
	}

	//미니맵 렌더타겟으로 설정
	//사용할거면 미니맵 위젯에서 미니맵 카메라에서 불러온 머티리얼 주석처리
	//미니맵 머티리얼에 좌표 반전 설정 연결
	//UMaterialInterface* MiniMapMaterialInterface = VFGGameState->GetMiniMapMaterialInterface();
	//if (IsValid(MiniMapMaterialInterface))
	//{
	//	UMaterialInstanceDynamic* CMaterialInstanceDynamic = UMaterialInstanceDynamic::Create(MiniMapMaterialInterface, this);
	//	if (IsValid(CMaterialInstanceDynamic))
	//	{
	//		CMaterialInstanceDynamic->SetTextureParameterValue(TEXT("MiniMapTexture"), VTextureRenderTarget2D);

	//		if (IsValid(VFGCommonHUD))
	//		{
	//			VFGCommonHUD->SetMiniMapMaterial(CMaterialInstanceDynamic);
	//		}
	//	}
	//}
	//else
	//{
	//	UE_LOG(LogTemp, Error, TEXT("GMaterialInterface is not valid in RenderTargetSetting of %s"), *GetNameSafe(this));
	//}
}

//Fog of War
void AFGPlayerController::Client_UpdateVision_Implementation(const TArray<uint8>& InFogOfWar, const TArray<AActor*>& InOtherActorArray, float InGridSize, int32 InColumns)
{
	//텍스쳐 그리기
	if (!IsValid(VTextureRenderTarget2D)) return;

	UKismetRenderingLibrary::ClearRenderTarget2D(this, VTextureRenderTarget2D);

	UCanvas* Canvas = nullptr;
	FVector2D Size;
	FDrawToRenderTargetContext Context;
	UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(this, VTextureRenderTarget2D, Canvas, Size, Context);
	if (Canvas)
	{
		for (int32 i = 0; i < InFogOfWar.Num(); ++i)
		{
			if (InFogOfWar[i] == 1)
			{
				FVector2D ScreenPositionValue = FVector2D(static_cast<float>(i % InColumns), static_cast<float>(i / InColumns));

				Canvas->K2_DrawTexture(nullptr, ScreenPositionValue, FVector2D(1.f, 1.f), FVector2D::ZeroVector);
			}
		}
	}

	UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(this, Context);

	//Visibility 설정
	if (!IsLocalPlayerController()) return;

	for (const auto& InOtherActor : InOtherActorArray)
	{
		if (!IsValid(InOtherActor)) continue;

		if (IFGVisibilityInterface* CFGVisibilityInterface = Cast<IFGVisibilityInterface>(InOtherActor))
		{
			FVector ActorLocation = InOtherActor->GetActorLocation();
			int32 ActorLocationGridX = FMath::FloorToInt(ActorLocation.X / InGridSize);
			int32 ActorLocationGridY = FMath::FloorToInt(ActorLocation.Y / InGridSize);

			int32 Index = ActorLocationGridY * InColumns + ActorLocationGridX;
			if (InFogOfWar.IsValidIndex(Index))
			{
				CFGVisibilityInterface->VisibilitySetting(InFogOfWar[Index] == 1, TeamIndex);

				AFGShop* CFGShop = Cast<AFGShop>(InOtherActor);
				if (IsValid(CFGShop))
				{
					bShopClicked = InFogOfWar[Index] == 1;
				}
			}
		}
	}
}

void AFGPlayerController::Server_NotifyLoadingComplete_Implementation()
{
	//직접 Server_NotifyLoadingComplete안에서 AFGGameState 캐스트 하기
	AFGGameState* GFGGameState = GetWorld()->GetGameState<AFGGameState>();
	if (IsValid(GFGGameState))
	{
		GFGGameState->NotifyPlayerLoadingComplete(this);
	}
}

void AFGPlayerController::InputModeGameAndUIFunction()
{
	if (IsLocalPlayerController())
	{
		FInputModeGameAndUI GameAndUIInputMode;
		GameAndUIInputMode.SetHideCursorDuringCapture(false);
		GameAndUIInputMode.SetLockMouseToViewportBehavior(EMouseLockMode::LockAlways);
		SetInputMode(GameAndUIInputMode);
		SetShowMouseCursor(true);
	}
}

void AFGPlayerController::MouseHover()
{
	FHitResult HitResult;
	bool bHit = GetHitResultUnderCursorByChannel(UEngineTypes::ConvertToTraceType(ECC_Visibility), true, HitResult);

	AActor* HitActor = (bHit && HitResult.bBlockingHit) ? HitResult.GetActor() : nullptr;

	if (IsValid(HoveredActor) && HoveredActor != HitActor)
	{
		if (IFGVisibilityInterface* CFGVisibilityInterface = Cast<IFGVisibilityInterface>(HoveredActor))
		{
			CFGVisibilityInterface->MouseUnhover();
		}

		bHover = false;
	}

	if (IsValid(HitActor) && HoveredActor != HitActor && !bHover)
	{
		if (IFGVisibilityInterface* CFGVisibilityInterface = Cast<IFGVisibilityInterface>(HitActor))
		{
			CurrentMouseCursor = CFGVisibilityInterface->MouseHover(TeamIndex, OwnPlayerNumber, OulineMaterial);

			bHover = true;
		}
		else
		{
			CurrentMouseCursor = EMouseCursor::Default;
		}
	}

	HoveredActor = HitActor;
}
