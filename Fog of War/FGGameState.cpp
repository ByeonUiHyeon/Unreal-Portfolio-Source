// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/FGGameState.h"
#include "Net/UnrealNetwork.h"
#include "Player/FGPlayerState.h"
#include "Player/FGPlayerController.h"
#include "Character/FGCharacterPlayer.h"
#include "Kismet/GameplayStatics.h"
#include "Grid/FGGridComponent.h"
#include "Actors/FGNexus.h"
#include "Game/FGGameMode.h"
#include "Game/FGGameInstance.h"

AFGGameState::AFGGameState()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	SetReplicates(true);

	IntTime = 0;
}

void AFGGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AFGGameState, IntTime);
	DOREPLIFETIME(AFGGameState, Rows);
	DOREPLIFETIME(AFGGameState, Columns);
	DOREPLIFETIME(AFGGameState, TeamACharacterPlayerArray);
	DOREPLIFETIME(AFGGameState, TeamBCharacterPlayerArray);
	DOREPLIFETIME(AFGGameState, TeamAPlayerControllerArray);
	DOREPLIFETIME(AFGGameState, TeamBPlayerControllerArray);
	DOREPLIFETIME(AFGGameState, BlockedArray);
}

void AFGGameState::BeginPlay()
{
	Super::BeginPlay();

	//Fog of War
	GridSetting();

	if (HasAuthority())
	{
		GetWorld()->GetTimerManager().SetTimer(UpdateFogOfWarTimer, this, &AFGGameState::UpdatingFogOfWar, FogOfWarPeriod, true);
	}
}

void AFGGameState::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	
	//시간
	if (HasAuthority())
	{
		FloatTime += DeltaSeconds;
		if (FloatTime >= 1.f)
		{
			++IntTime;

			FloatTime = 0.f;
		}
	}
}

//팀 스코어 및 킬로그
void AFGGameState::AddTeamScroeAndKilllog(AFGCharacterPlayer* InKiller, AFGCharacterPlayer* InTarget)
{
	if (!IsValid(InKiller) || !IsValid(InTarget)) return;

	int32 TeamIndexValue = InKiller->GetTeamIndex();

	UTexture2D* KillerTexture2D = InKiller->GetCharacterImage();
	if(!IsValid(KillerTexture2D)) return;

	UTexture2D* TargetTexture2D = InTarget->GetCharacterImage();
	if (!IsValid(TargetTexture2D)) return;

	if (HasAuthority())
	{
		if (TeamIndexValue == 0)
		{
			++TeamAScore;
		}
		else
		{
			++TeamBScore;
		}

		for (const auto& EPlayerState : PlayerArray)
		{
			AFGPlayerState* CFGPlayerState = Cast<AFGPlayerState>(EPlayerState);
			if (!IsValid(CFGPlayerState)) continue;

			if (CFGPlayerState->GetOwnPlayerNumber() == InKiller->GetOwnPlayerNumber())
			{
				KillerNickname = CFGPlayerState->GetPlayerName();
			}
			else if (CFGPlayerState->GetOwnPlayerNumber() == InTarget->GetOwnPlayerNumber())
			{
				TargetNickname = CFGPlayerState->GetPlayerName();
			}
		}

		for (const auto& ETeamAPlayerController : TeamAPlayerControllerArray)
		{
			if (IsValid(ETeamAPlayerController))
			{
				ETeamAPlayerController->Client_TeamScoreWidgetFunction(TeamAScore, TeamBScore, TeamIndexValue, KillerTexture2D, TargetTexture2D, KillerNickname, TargetNickname);
			}
		}

		for (const auto& ETeamBPlayerController : TeamBPlayerControllerArray)
		{
			if (IsValid(ETeamBPlayerController))
			{
				ETeamBPlayerController->Client_TeamScoreWidgetFunction(TeamAScore, TeamBScore, TeamIndexValue, KillerTexture2D, TargetTexture2D, KillerNickname, TargetNickname);
			}
		}
	}
}

//Fog of War
//맵의 바닥, 벽, 물체등에 FGGridComponent를 붙여 시야 블럭 처리한다.
//바닥은 필수 설정
void AFGGameState::GridSetting()
{
	if (!HasAuthority()) return;

	BlockedArray.Empty();

	//바닥 으로 Rows, Columns 정보 얻을라면 사용
	//벽은 일자형이여야 한다.ㄱ자형은 ㅁ형으로 설정된다.
	TArray<AActor*> OutActorArray;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AActor::StaticClass(), OutActorArray);

	//바닥 처리
	if (bUseValue)
	{
		//값
		Columns = FMath::CeilToInt(XSize / GridSize);
		Rows = FMath::CeilToInt(YSize / GridSize);
	}
	else
	{
		//컴포넌트
		for (AActor* EOutActor : OutActorArray)
		{
			UActorComponent* GActorComponent = EOutActor->GetComponentByClass(UFGGridComponent::StaticClass());
			if (!IsValid(GActorComponent)) continue;

			UFGGridComponent* CFGGridComponent = Cast<UFGGridComponent>(GActorComponent);
			if (!IsValid(CFGGridComponent)) continue;

			if (CFGGridComponent->ComponentHasTag(CFGGridComponent->GetNameByEnum(EGridType::Floor)))
			{
				CFGGridComponent->GetActorGirdRowsAndColumns(GridSize, Rows, Columns);

				break;
			}
		}
	}

	//블럭 초기화
	if (Rows > 0 && Columns > 0)
	{
		BlockedArray.Init(0, Rows * Columns);
	}

	//벽 및 물체 처리
	for (AActor* EOutActor : OutActorArray)
	{
		UActorComponent* GActorComponent = EOutActor->GetComponentByClass(UFGGridComponent::StaticClass());
		if (!IsValid(GActorComponent)) continue;

		UFGGridComponent* CFGGridComponent = Cast<UFGGridComponent>(GActorComponent);
		if (!IsValid(CFGGridComponent)) continue;

		if (CFGGridComponent->ComponentHasTag(CFGGridComponent->GetNameByEnum(EGridType::Nexus)))
		{
			CFGGridComponent->GetNexusGridLocation(NexusArray);
		}
		else if (CFGGridComponent->ComponentHasTag(CFGGridComponent->GetNameByEnum(EGridType::ComplexObject)))
		{
			CFGGridComponent->GetActorGridComplexHeight(GridSize, Rows, Columns, BlockedArray);
			//대각선 배치나 복잡한 형태는 이거 사용
		}
		else if (CFGGridComponent->ComponentHasTag(CFGGridComponent->GetNameByEnum(EGridType::StaticActor)))
		{
			CFGGridComponent->GetActorGridLocation(StaticActorArray);
		}
		else if (CFGGridComponent->ComponentHasTag(CFGGridComponent->GetNameByEnum(EGridType::Wall)))
		{
			CFGGridComponent->GetActorGridHeight(GridSize, Rows, Columns, BlockedArray);
		}
	}

	//시야 정보 초기화
	if (Rows > 0 && Columns > 0)
	{
		TeamAFogOfWar.Init(0, Rows * Columns);
		TeamBFogOfWar.Init(0, Rows * Columns);
	}

	//팀마다 다른 액터 정보 담기
	for (const auto& ENexus : NexusArray)
	{
		if (ENexus->GetTeamIndex() == 0)
		{
			OtherActorOfTeamB.Add(ENexus.Get());
		}
		else
		{
			OtherActorOfTeamA.Add(ENexus.Get());
		}
	}

	for (const auto& EStaticActor : StaticActorArray)
	{
		OtherActorOfTeamA.Add(EStaticActor);
		OtherActorOfTeamB.Add(EStaticActor);
	}

	//틱 시작
	SetActorTickEnabled(true);
}

//Fog of War
void AFGGameState::UpdateFogOfWar(int32 InTeamIndex, const TArray<TObjectPtr<AFGCharacterPlayer>>& OurTeamCharacterPlayerArray, const TArray<TObjectPtr<AActor>>& InOtherActorArray, const TArray<TObjectPtr<AFGPlayerController>>& InTeamPlayerControllerArray)
{
	TArray<uint8>& SelectFogOfWar = (InTeamIndex == 0) ? TeamAFogOfWar : TeamBFogOfWar;
	//팀별 배열 선택

	FMemory::Memzero(SelectFogOfWar.GetData(), SelectFogOfWar.Num());
	//어둡게 초기화

	TSet<int32> VisitedGrids;
	//시야 밝힌곳 중복 제거

	//같은 팀 시야 밝히기
	for (const auto& ETeamCharacterPlayer : OurTeamCharacterPlayerArray)
	{
		if (IsValid(ETeamCharacterPlayer))
		{
			if (ETeamCharacterPlayer->GetbDead()) continue;

			int32 VisionRadiusGrid = FMath::FloorToInt(ETeamCharacterPlayer->GetVisionRadius() / GridSize);
			//시야 반지름 그리드로 변환

			FVector ActorLocation = ETeamCharacterPlayer->GetActorLocation();
			//캐릭터 위치 그리드로 변환

			//캐릭터 높이 그리드로 변환
			FVector Origin, BoxExtent;
			ETeamCharacterPlayer->GetActorBounds(true, Origin, BoxExtent);
			int32 ActorGridZ = FMath::CeilToInt((Origin + BoxExtent).Z / GridSize);

			UpdateFogOfWarFunction(VisionRadiusGrid, ActorLocation, ActorGridZ, SelectFogOfWar, VisitedGrids);
		}
	}

	//넥서스 시야
	for (const auto& ENexus : NexusArray)
	{
		if (IsValid(ENexus))
		{
			if (ENexus->GetTeamIndex() == InTeamIndex)
			{
				int32 VisionRadiusGrid = FMath::FloorToInt(ENexus->GetVisionRadius() / GridSize);

				FVector ActorLocation = ENexus->GetActorLocation();

				UpdateFogOfWarFunction(VisionRadiusGrid, ActorLocation, 4, SelectFogOfWar, VisitedGrids);
			}
		}
	}

	//각 플레이어에게 전달
	for (const auto& ETeamPlayerController : InTeamPlayerControllerArray)
	{
		if (IsValid(ETeamPlayerController))
		{
			ETeamPlayerController->Client_UpdateVision(SelectFogOfWar, InOtherActorArray, GridSize, Columns);
		}
	}
}

//Fog of War
void AFGGameState::UpdateFogOfWarFunction(int32 InVisionRadiusGrid, FVector InActorLocation, int32 InActorGridZ, TArray<uint8>& InFogOfWar, TSet<int32>& InVisitedGrids)
{
	int32 ActorGridX = FMath::FloorToInt(InActorLocation.X / GridSize);
	int32 ActorGridY = FMath::FloorToInt(InActorLocation.Y / GridSize);

	if (ActorGridX < 0 || ActorGridY < 0 || ActorGridX >= Columns || ActorGridY >= Rows) return;

	//중심 밝히기
	int32 ActorIndex = ActorGridY * Columns + ActorGridX;
	if (InFogOfWar.IsValidIndex(ActorIndex))
	{
		InFogOfWar[ActorIndex] = 1;

		InVisitedGrids.Add(ActorIndex);
	}

	//내부 함수
	auto BresenhamSightFunction = [this, ActorIndex, ActorGridX, ActorGridY, InActorGridZ, &InFogOfWar, &InVisitedGrids](int32 InXOffset, int32 InYOffset)
		{
			BresenhamSight(ActorIndex, ActorGridX, ActorGridY, ActorGridX + InXOffset, ActorGridY + InYOffset, InActorGridZ, InFogOfWar, InVisitedGrids);
			if (InXOffset != 0) BresenhamSight(ActorIndex, ActorGridX, ActorGridY, ActorGridX - InXOffset, ActorGridY + InYOffset, InActorGridZ, InFogOfWar, InVisitedGrids);
			if (InYOffset != 0) BresenhamSight(ActorIndex, ActorGridX, ActorGridY, ActorGridX + InXOffset, ActorGridY - InYOffset, InActorGridZ, InFogOfWar, InVisitedGrids);
			if (InXOffset != 0 && InYOffset != 0) BresenhamSight(ActorIndex, ActorGridX, ActorGridY, ActorGridX - InXOffset, ActorGridY - InYOffset, InActorGridZ, InFogOfWar, InVisitedGrids);
		};

	//원 끝 반지름 찾기
	//1사분면만 찾고 모든 사분면 돌린다.
	int32 PreviousMaxY = 0;
	//중복 감지 방지
	int32 VisionRadiusGridValue = InVisionRadiusGrid + 1;
	//처음 시작 부분만 튀어 나와서 끝점 보정 하기 위해 사용 반지름 결과는 InVisionRadiusGrid가 된다.
	for (int32 x = VisionRadiusGridValue; x >= 0; --x)
	{
		int32 MaxY = FMath::FloorToInt(FMath::Sqrt(static_cast<float>(VisionRadiusGridValue * VisionRadiusGridValue - x * x)));
		//원형 반지름 범위 안

		for (int32 y = PreviousMaxY; y <= MaxY; ++y)
		{
			//끝점 보정
			if (x == VisionRadiusGridValue)
			{
				BresenhamSightFunction(InVisionRadiusGrid , y);

				continue;
			}

			if (x == 0)
			{
				BresenhamSightFunction(x, InVisionRadiusGrid);

				continue;
			}

			BresenhamSightFunction(x, y);
			//끝점 까지 시야 검사
		}

		PreviousMaxY = MaxY;
		//중복 방지를 위해 다음 루프 때 쓰임
	}
}

//Fog of War
void AFGGameState::BresenhamSight(int32 InCenter, int32 StartX, int32 StartY, int32 EndX, int32 EndY, int32 InCharacterZ, TArray<uint8>& InFogOfWar, TSet<int32>& InVisitedGrids)
{
	//현재 값
	int32 CurrentX = StartX;
	int32 CurrentY = StartY;

	//증감 결정
	int32 StepX = (StartX < EndX) ? 1 : -1;
	int32 StepY = (StartY < EndY) ? 1 : -1;

	//이동할 거리
	int32 DeltaX = FMath::Abs(EndX - StartX);
	int32 DeltaY = FMath::Abs(EndY - StartY);

	int32 Error = DeltaX - DeltaY;
	//(StartX, StartY)에서 (EndX, EndY)까지 직선을 그리면
	//정수 Tile로만 이동할 수 있으므로, 실제 직선 좌표와 Tile 중심 좌표가 완전히 일치하지 않는다.
	//Error = DeltaX - DeltaY 는 X방향 이동과 Y방향 이동 중 어느 쪽을 먼저 해야 직선과 최대한 가까운 경로가 되는지 결정하는 기준
	//Error가 양수면 X로 이동, 음수면 Y로 이동

	//이미 자기 자신 위치 밝혀서 1증가 시킨 후 while 돌린다.
	//원래 Bresenham은 1부터 증가 시키지 않는다.
	int32 Error2 = 2 * Error;
	//float로 계산하면 currentY = slope * currentX 이런 식으로 오차를 계산해야 하는데
	//정수 연산만 쓰기 위해 모든 값을 2배해서 비교하는 trick을 쓴다.

	//X 방향으로 이동
	//화면 좌표는 -y가 위 맵은 x가 위
	if (Error2 > -DeltaY)
	{
		Error -= DeltaY;
		CurrentX += StepX;
	}

	//Y 방향으로 이동
	//화면 좌표는 x가 오른쪽 맵은 y가 오른쪽
	if (Error2 < DeltaX)
	{
		Error += DeltaX;
		CurrentY += StepY;
	}

	while (true)
	{
		if (CurrentX < 0 || CurrentY < 0 || CurrentX >= Columns || CurrentY >= Rows) break;

		//유효한 인덱스 검사
		int32 GridIndex = CurrentY * Columns + CurrentX;
		if (!InFogOfWar.IsValidIndex(GridIndex)) break;

		//if (GridIndex != InCenter && BlockedArray[GridIndex] >= InCharacterZ) break;
		if (BlockedArray[GridIndex] >= InCharacterZ) break;
		//벽이 있으면 시야 차단
		//벽이랑 겹칠 경우 제외

		if (!InVisitedGrids.Contains(GridIndex))
		{
			InFogOfWar[GridIndex] = 1;
			//시야 밝히기

			InVisitedGrids.Add(GridIndex);
		}

		if (CurrentX == EndX && CurrentY == EndY) break;
		//끝점 도달 시 종료

		//증감
		Error2 = 2 * Error;

		if (Error2 > -DeltaY)
		{
			Error -= DeltaY;
			CurrentX += StepX;
		}

		if (Error2 < DeltaX)
		{
			Error += DeltaX;
			CurrentY += StepY;
		}
	}
}

//Fog of War
void AFGGameState::UpdatingFogOfWar()
{
	UpdateFogOfWar(0, TeamACharacterPlayerArray, OtherActorOfTeamA, TeamAPlayerControllerArray);
	UpdateFogOfWar(1, TeamBCharacterPlayerArray, OtherActorOfTeamB, TeamBPlayerControllerArray);
}

//준비 완료되면 스폰
void AFGGameState::NotifyPlayerLoadingComplete(AFGPlayerController* InFGPlayerController)
{
	LoadedPlayers.Add(InFGPlayerController);

	UFGGameInstance* GFGGameInstance = GetGameInstance<UFGGameInstance>();
	if (!IsValid(GFGGameInstance)) return;

	//모든 플레이어가 로딩 완료
	if (LoadedPlayers.Num() == GFGGameInstance->GetAllPlayerNum())
	{
		GetWorld()->GetTimerManager().SetTimer(SpawnDelayTimerHandle, FTimerDelegate::CreateLambda([&]()
			{
				AFGGameMode* GFGGameMode = GetWorld()->GetAuthGameMode<AFGGameMode>();
				if (IsValid(GFGGameMode))
				{
					for (AFGPlayerController* LoadedPlayer : LoadedPlayers)
					{
						GFGGameMode->SpawnAllCharacters(LoadedPlayer);
					}
				}

				NetMulticast_CreateFGChildWidgetComponent();

				GetWorld()->GetTimerManager().ClearTimer(SpawnDelayTimerHandle);
			}
		), SpawnDelay, false);
	}
}

//넥서스 위젯컴포넌트 설정
void AFGGameState::NetMulticast_CreateFGChildWidgetComponent_Implementation()
{
	TArray<AActor*> OutActorArray;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), FGNexusClass, OutActorArray);
	for (AActor* EOutActor : OutActorArray)
	{
		AFGNexus* CFGNexus = Cast<AFGNexus>(EOutActor);
		if (!IsValid(CFGNexus)) continue;

		CFGNexus->CreateFGChildWidgetComponent();
	}
}
