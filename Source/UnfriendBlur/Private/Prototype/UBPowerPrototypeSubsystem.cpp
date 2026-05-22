#include "Prototype/UBPowerPrototypeSubsystem.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "Powers/UBPowerInventoryComponent.h"
#include "Powers/UBPowerPickup.h"
#include "Powers/UBPowerTypes.h"
#include "Prototype/UBPrototypeTargetCar.h"
#include "Stats/Stats.h"

void UUBPowerPrototypeSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	bPrintedInstructions = false;
	bSpawnedPrototypePickups = false;
	bSpawnedPrototypeTargets = false;
}

void UUBPowerPrototypeSubsystem::Tick(float DeltaTime)
{
	UWorld* World = GetWorld();
	if (!World || !World->IsGameWorld() || World->GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	if (!bPrintedInstructions)
	{
		PrintInstructions();
		bPrintedInstructions = true;
	}

	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PlayerController = It->Get();
		if (!PlayerController || !PlayerController->IsLocalController() || !PlayerController->GetPawn())
		{
			continue;
		}

		if (!bSpawnedPrototypePickups && World->GetNetMode() != NM_Client)
		{
			SpawnPrototypePickups(PlayerController->GetPawn());
		}

		if (!bSpawnedPrototypeTargets && World->GetNetMode() != NM_Client)
		{
			SpawnPrototypeTargets(PlayerController->GetPawn());
		}

		TryInventoryHotkeys(PlayerController);
		DisplayInventory(PlayerController);
	}
}

TStatId UUBPowerPrototypeSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UUBPowerPrototypeSubsystem, STATGROUP_Tickables);
}

void UUBPowerPrototypeSubsystem::TryInventoryHotkeys(APlayerController* PlayerController) const
{
	if (!PlayerController)
	{
		return;
	}

	APawn* Pawn = PlayerController->GetPawn();
	if (!Pawn)
	{
		return;
	}

	UUBPowerInventoryComponent* PowerComponent = UUBPowerInventoryComponent::FindOrCreatePowerComponent(Pawn);
	if (!PowerComponent)
	{
		return;
	}

	if (PlayerController->WasInputKeyJustPressed(EKeys::F))
	{
		PowerComponent->SelectNextPowerSlot();
	}

	if (PlayerController->WasInputKeyJustPressed(EKeys::B))
	{
		PowerComponent->UseSelectedPower(true);
	}

	if (PlayerController->WasInputKeyJustPressed(EKeys::N))
	{
		PowerComponent->UseSelectedPower(false);
	}

	if (PlayerController->WasInputKeyJustPressed(EKeys::G))
	{
		PowerComponent->DropSelectedPower();
	}
}

void UUBPowerPrototypeSubsystem::DisplayInventory(APlayerController* PlayerController) const
{
	if (!GEngine || !PlayerController)
	{
		return;
	}

	APawn* Pawn = PlayerController->GetPawn();
	if (!Pawn)
	{
		return;
	}

	const UUBPowerInventoryComponent* PowerComponent = Pawn->FindComponentByClass<UUBPowerInventoryComponent>();
	if (!PowerComponent)
	{
		GEngine->AddOnScreenDebugMessage(9001, 0.05f, FColor::White, TEXT("Powers: [empty] | collect pickups | F select | B front | N back | G drop"));
		return;
	}

	const TArray<FUBPowerSlot>& Slots = PowerComponent->GetPowerSlots();
	FString InventoryText = TEXT("Powers ");
	for (int32 Index = 0; Index < 3; ++Index)
	{
		const FString SlotText = Slots.IsValidIndex(Index)
			? FString::Printf(TEXT("%s%s"), Slots[Index].bIsSuper ? TEXT("S-") : TEXT(""), *UBPowerTypeShortCode(Slots[Index].PowerType))
			: TEXT("---");
		if (Index == PowerComponent->GetSelectedSlotIndex() && Slots.IsValidIndex(Index))
		{
			InventoryText += FString::Printf(TEXT("[>%s<] "), *SlotText);
		}
		else
		{
			InventoryText += FString::Printf(TEXT("[%s] "), *SlotText);
		}
	}

	InventoryText += TEXT("| F select | B front | N back | G drop");
	GEngine->AddOnScreenDebugMessage(9001, 0.05f, FColor::White, InventoryText);
}

void UUBPowerPrototypeSubsystem::SpawnPrototypePickups(APawn* AnchorPawn)
{
	UWorld* World = GetWorld();
	if (!World || !AnchorPawn || bSpawnedPrototypePickups)
	{
		return;
	}

	bSpawnedPrototypePickups = true;

	constexpr int32 PickupCount = 32;
	for (int32 Index = 0; Index < PickupCount; ++Index)
	{
		const FVector SpawnLocation = FindPickupLocation(AnchorPawn, Index);
		const FRotator SpawnRotation = FRotator::ZeroRotator;

		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		AUBPowerPickup* Pickup = World->SpawnActor<AUBPowerPickup>(AUBPowerPickup::StaticClass(), SpawnLocation, SpawnRotation, SpawnParams);
		if (!Pickup)
		{
			continue;
		}

		static const EUBPowerType PrototypePowerCycle[] =
		{
			EUBPowerType::Boost,
			EUBPowerType::Shield,
			EUBPowerType::Repair,
			EUBPowerType::Barge,
			EUBPowerType::Bolt,
			EUBPowerType::Shunt,
			EUBPowerType::Mine,
			EUBPowerType::Shock
		};

		Pickup->FixedPower = PrototypePowerCycle[Index % UE_ARRAY_COUNT(PrototypePowerCycle)];
		Pickup->RefreshVisuals();
	}

	UE_LOG(LogTemp, Log, TEXT("[UnfriendBlur Powers] Spawned %d random prototype pickups"), PickupCount);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 6.0f, FColor::Cyan, TEXT("Random power pickups spawned on the map"));
	}
}

FVector UUBPowerPrototypeSubsystem::FindPickupLocation(APawn* AnchorPawn, int32 PickupIndex) const
{
	UWorld* World = GetWorld();
	if (!World || !AnchorPawn)
	{
		return FVector::ZeroVector;
	}

	const FVector Forward = AnchorPawn->GetActorForwardVector().GetSafeNormal();
	const FVector Right = AnchorPawn->GetActorRightVector().GetSafeNormal();
	const int32 RingIndex = PickupIndex / 8;
	const int32 SegmentIndex = PickupIndex % 8;
	const float AngleDegrees = static_cast<float>(SegmentIndex) * 45.0f + static_cast<float>(RingIndex) * 18.0f + FMath::RandRange(-12.0f, 12.0f);
	const float AngleRadians = FMath::DegreesToRadians(AngleDegrees);
	const float Radius = 2200.0f + static_cast<float>(RingIndex) * 1750.0f + FMath::RandRange(-260.0f, 260.0f);
	const FVector PlanarOffset = Forward * FMath::Cos(AngleRadians) * Radius + Right * FMath::Sin(AngleRadians) * Radius;
	const FVector Candidate = AnchorPawn->GetActorLocation() + PlanarOffset;

	FHitResult Hit;
	const FVector TraceStart = Candidate + FVector::UpVector * 1500.0f;
	const FVector TraceEnd = Candidate - FVector::UpVector * 5000.0f;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(UBPrototypePickupGroundTrace), false, AnchorPawn);
	if (World->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, QueryParams))
	{
		return Hit.Location + FVector::UpVector * 120.0f;
	}

	return Candidate + FVector::UpVector * 120.0f;
}

void UUBPowerPrototypeSubsystem::SpawnPrototypeTargets(APawn* AnchorPawn)
{
	UWorld* World = GetWorld();
	if (!World || !AnchorPawn || bSpawnedPrototypeTargets)
	{
		return;
	}

	bSpawnedPrototypeTargets = true;

	constexpr int32 TargetCount = 2;
	for (int32 Index = 0; Index < TargetCount; ++Index)
	{
		const FVector SpawnLocation = FindTargetCarLocation(AnchorPawn, Index);
		const FRotator SpawnRotation(0.0f, AnchorPawn->GetActorRotation().Yaw, 0.0f);

		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		AUBPrototypeTargetCar* TargetCar = World->SpawnActor<AUBPrototypeTargetCar>(AUBPrototypeTargetCar::StaticClass(), SpawnLocation, SpawnRotation, SpawnParams);
		if (TargetCar)
		{
			TargetCar->InitializePrototypeTarget(Index);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[UnfriendBlur Powers] Spawned %d prototype target cars"), TargetCount);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 6.0f, FColor::Orange, TEXT("2 target cars spawned ahead for power testing"));
	}
}

FVector UUBPowerPrototypeSubsystem::FindTargetCarLocation(APawn* AnchorPawn, int32 TargetIndex) const
{
	UWorld* World = GetWorld();
	if (!World || !AnchorPawn)
	{
		return FVector::ZeroVector;
	}

	const FVector Forward = AnchorPawn->GetActorForwardVector().GetSafeNormal();
	const FVector Right = AnchorPawn->GetActorRightVector().GetSafeNormal();
	const float ForwardDistance = TargetIndex == 0 ? 1550.0f : 2650.0f;
	const float LateralOffset = TargetIndex == 0 ? -360.0f : 430.0f;
	const FVector Candidate = AnchorPawn->GetActorLocation() + Forward * ForwardDistance + Right * LateralOffset;

	FHitResult Hit;
	const FVector TraceStart = Candidate + FVector::UpVector * 1500.0f;
	const FVector TraceEnd = Candidate - FVector::UpVector * 5000.0f;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(UBPrototypeTargetGroundTrace), false, AnchorPawn);
	if (World->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, QueryParams))
	{
		return Hit.Location + FVector::UpVector * 85.0f;
	}

	return Candidate + FVector::UpVector * 85.0f;
}

EUBPowerType UUBPowerPrototypeSubsystem::PickRandomPower() const
{
	static const EUBPowerType Powers[] =
	{
		EUBPowerType::Boost,
		EUBPowerType::Shield,
		EUBPowerType::Repair,
		EUBPowerType::Barge,
		EUBPowerType::Bolt,
		EUBPowerType::Shunt,
		EUBPowerType::Mine,
		EUBPowerType::Shock
	};

	return Powers[FMath::RandRange(0, UE_ARRAY_COUNT(Powers) - 1)];
}

void UUBPowerPrototypeSubsystem::PrintInstructions()
{
	const FString Message = TEXT("UnfriendBlur: native vehicle control | F slot | B front power | N back power | G drop");
	UE_LOG(LogTemp, Log, TEXT("%s"), *Message);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 8.0f, FColor::Green, Message);
	}
}
