#include "Prototype/UBPowerPrototypeSubsystem.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Components/PrimitiveComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "PhysicsEngine/BodySetup.h"
#include "Powers/UBPowerInventoryComponent.h"
#include "Powers/UBPowerPickup.h"
#include "Powers/UBPowerTypes.h"
#include "Prototype/UBPrototypeTargetCar.h"
#include "Vehicle/UBVehicleHealthComponent.h"
#include "Vehicle/UBVehicleStatusComponent.h"
#include "Stats/Stats.h"

void UUBPowerPrototypeSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	bPrintedInstructions = false;
	bSpawnedPrototypeTrack = false;
	bSpawnedPrototypePickups = false;
	bSpawnedPrototypeTargets = false;
	bLoggedVehicleDiagnostics = false;
	bHasPrototypeTrackStart = false;
	PrototypeTrackStartLocation = FVector::ZeroVector;
	PrototypeTrackStartRotation = FRotator::ZeroRotator;
	PrototypePickupLocations.Reset();
	PrototypeTargetTransforms.Reset();
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

		if (!bSpawnedPrototypeTrack && World->GetNetMode() != NM_Client)
		{
			if (IsImportedDriftTrackWorld())
			{
				SetupImportedDriftTrack(PlayerController->GetPawn());
			}
			else
			{
				SpawnPrototypeTrack(PlayerController->GetPawn());
			}
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
		DisplayVehicleDiagnostics(PlayerController);
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

void UUBPowerPrototypeSubsystem::DisplayVehicleDiagnostics(APlayerController* PlayerController)
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

	const bool bW = PlayerController->IsInputKeyDown(EKeys::W);
	const bool bA = PlayerController->IsInputKeyDown(EKeys::A);
	const bool bS = PlayerController->IsInputKeyDown(EKeys::S);
	const bool bD = PlayerController->IsInputKeyDown(EKeys::D);
	const bool bSpace = PlayerController->IsInputKeyDown(EKeys::SpaceBar);
	const bool bShift = PlayerController->IsInputKeyDown(EKeys::LeftShift) || PlayerController->IsInputKeyDown(EKeys::RightShift);

	UPrimitiveComponent* RootPrimitive = Cast<UPrimitiveComponent>(Pawn->GetRootComponent());
	const FVector Velocity = RootPrimitive ? RootPrimitive->GetPhysicsLinearVelocity() : Pawn->GetVelocity();
	const float SpeedKmh = Velocity.Size() * 0.036f;
	const float SteerRaw = (bD ? 1.0f : 0.0f) - (bA ? 1.0f : 0.0f);
	const UUBVehicleHealthComponent* HealthComponent = Pawn->FindComponentByClass<UUBVehicleHealthComponent>();
	const UUBVehicleStatusComponent* StatusComponent = Pawn->FindComponentByClass<UUBVehicleStatusComponent>();
	const FString HealthText = HealthComponent
		? FString::Printf(TEXT("hp %.0f/%.0f"), HealthComponent->GetCurrentHealth(), HealthComponent->MaxHealth)
		: TEXT("hp ---");
	const FString StatusText = StatusComponent && StatusComponent->IsSlowed()
		? FString::Printf(TEXT("slow %.0f%% %.1fs"), StatusComponent->GetSlowStrength() * 100.0f, StatusComponent->GetSlowTimeRemaining())
		: TEXT("slow off");

	FString MovementComponents;
	for (const UActorComponent* Component : Pawn->GetComponents())
	{
		if (!Component)
		{
			continue;
		}

		const FString ComponentClassName = Component->GetClass()->GetName();
		if (ComponentClassName.Contains(TEXT("Movement")) || ComponentClassName.Contains(TEXT("Vehicle")))
		{
			if (!MovementComponents.IsEmpty())
			{
				MovementComponents += TEXT(", ");
			}
			MovementComponents += ComponentClassName;
		}
	}

	if (MovementComponents.IsEmpty())
	{
		MovementComponents = TEXT("none found");
	}

	const FString InputText = FString::Printf(
		TEXT("Vehicle debug | %s | W:%d A:%d S:%d D:%d Space:%d Shift:%d | steer raw %.0f | %.0f km/h | yaw %.0f | %s | %s"),
		*Pawn->GetClass()->GetName(),
		bW ? 1 : 0,
		bA ? 1 : 0,
		bS ? 1 : 0,
		bD ? 1 : 0,
		bSpace ? 1 : 0,
		bShift ? 1 : 0,
		SteerRaw,
		SpeedKmh,
		Pawn->GetActorRotation().Yaw,
		*HealthText,
		*StatusText);

	GEngine->AddOnScreenDebugMessage(9002, 0.05f, FColor::Yellow, InputText);

	if (!bLoggedVehicleDiagnostics)
	{
		bLoggedVehicleDiagnostics = true;
		UE_LOG(LogTemp, Log, TEXT("[UnfriendBlur Vehicle Debug] Pawn=%s Components=%s Root=%s"),
			*Pawn->GetClass()->GetName(),
			*MovementComponents,
			Pawn->GetRootComponent() ? *Pawn->GetRootComponent()->GetClass()->GetName() : TEXT("none"));
	}
}

bool UUBPowerPrototypeSubsystem::IsImportedDriftTrackWorld() const
{
	const UWorld* World = GetWorld();
	return World && World->GetMapName().Contains(TEXT("L_DriftTrackShort"));
}

void UUBPowerPrototypeSubsystem::SetupImportedDriftTrack(APawn* AnchorPawn)
{
	UWorld* World = GetWorld();
	if (!World || !AnchorPawn || bSpawnedPrototypeTrack)
	{
		return;
	}

	bSpawnedPrototypeTrack = true;
	bHasPrototypeTrackStart = true;
	PrototypePickupLocations.Reset();
	PrototypeTargetTransforms.Reset();

	for (TActorIterator<AStaticMeshActor> It(World); It; ++It)
	{
		AStaticMeshActor* MeshActor = *It;
		UStaticMeshComponent* MeshComponent = MeshActor ? MeshActor->GetStaticMeshComponent() : nullptr;
		UStaticMesh* Mesh = MeshComponent ? MeshComponent->GetStaticMesh() : nullptr;
		if (!Mesh || !Mesh->GetName().Equals(TEXT("SM_DriftTrackShort")))
		{
			continue;
		}

		if (UBodySetup* BodySetup = Mesh->GetBodySetup())
		{
			BodySetup->CollisionTraceFlag = CTF_UseComplexAsSimple;
		}

		MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		MeshComponent->SetCollisionResponseToAllChannels(ECR_Block);
		MeshComponent->RecreatePhysicsState();
	}

	const FVector StartLocation = AnchorPawn->GetActorLocation() + FVector::UpVector * 40.0f;
	const FVector Forward = AnchorPawn->GetActorForwardVector().GetSafeNormal2D();
	const FVector Right = AnchorPawn->GetActorRightVector().GetSafeNormal2D();
	PrototypeTrackStartLocation = StartLocation;
	PrototypeTrackStartRotation = FRotator(0.0f, AnchorPawn->GetActorRotation().Yaw, 0.0f);

	const float LateralPattern[] = { -420.0f, 0.0f, 420.0f, -180.0f, 220.0f };
	for (int32 Index = 0; Index < 40; ++Index)
	{
		const float ForwardDistance = 1600.0f + static_cast<float>(Index) * 420.0f;
		const float LateralDistance = LateralPattern[Index % UE_ARRAY_COUNT(LateralPattern)];
		const FVector Candidate = StartLocation + Forward * ForwardDistance + Right * LateralDistance;
		PrototypePickupLocations.Add(Candidate + FVector::UpVector * 80.0f);
	}

	for (int32 Index = 0; Index < 5; ++Index)
	{
		const float ForwardDistance = 2600.0f + static_cast<float>(Index) * 900.0f;
		const float LateralDistance = Index % 2 == 0 ? -330.0f : 330.0f;
		const FVector Candidate = StartLocation + Forward * ForwardDistance + Right * LateralDistance + FVector::UpVector * 90.0f;
		PrototypeTargetTransforms.Add(FTransform(PrototypeTrackStartRotation, Candidate, FVector::OneVector));
	}

	MovePawnToPrototypeTrackStart(AnchorPawn);

	UE_LOG(LogTemp, Log, TEXT("[UnfriendBlur Track] Using imported Drift Track Short map with %d pickup points and %d target points"), PrototypePickupLocations.Num(), PrototypeTargetTransforms.Num());

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 9.0f, FColor::Cyan, TEXT("DRIFT TRACK SHORT loaded with power pickups and target cars"));
	}
}

void UUBPowerPrototypeSubsystem::SpawnPrototypeTrack(APawn* AnchorPawn)
{
	UWorld* World = GetWorld();
	if (!World || !AnchorPawn || bSpawnedPrototypeTrack)
	{
		return;
	}

	bSpawnedPrototypeTrack = true;
	bHasPrototypeTrackStart = false;
	PrototypePickupLocations.Reset();
	PrototypeTargetTransforms.Reset();

	UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	UMaterialInterface* BaseMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (!CubeMesh)
	{
		UE_LOG(LogTemp, Warning, TEXT("[UnfriendBlur Track] Could not load cube mesh for prototype track"));
		return;
	}

	const FVector AnchorLocation = AnchorPawn->GetActorLocation();
	FHitResult GroundHit;
	const FVector TraceStart = AnchorLocation + FVector::UpVector * 1500.0f;
	const FVector TraceEnd = AnchorLocation - FVector::UpVector * 5000.0f;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(UBPrototypeTrackGroundTrace), false, AnchorPawn);
	const float RoadTopZ = World->LineTraceSingleByChannel(GroundHit, TraceStart, TraceEnd, ECC_Visibility, QueryParams)
		? GroundHit.Location.Z + 70.0f
		: AnchorLocation.Z - 70.0f;

	const TArray<FVector2D> LocalPath =
	{
		FVector2D(-1200.0f, 0.0f),
		FVector2D(1400.0f, 0.0f),
		FVector2D(3400.0f, 420.0f),
		FVector2D(5400.0f, 1450.0f),
		FVector2D(7900.0f, 1450.0f),
		FVector2D(10100.0f, 520.0f),
		FVector2D(12100.0f, -720.0f),
		FVector2D(14900.0f, -720.0f),
		FVector2D(17200.0f, 240.0f),
		FVector2D(19500.0f, 1500.0f),
		FVector2D(22400.0f, 1500.0f)
	};

	constexpr float RoadWidth = 1420.0f;
	constexpr float RoadThickness = 34.0f;
	const FLinearColor AsphaltColor(0.055f, 0.058f, 0.064f, 1.0f);
	const FLinearColor ShoulderColor(0.085f, 0.09f, 0.1f, 1.0f);
	const FLinearColor RailColor(0.16f, 0.17f, 0.19f, 1.0f);
	const FLinearColor LeftNeonColor(0.0f, 0.55f, 1.0f, 1.0f);
	const FLinearColor RightNeonColor(1.0f, 0.12f, 0.02f, 1.0f);
	const FLinearColor CenterLineColor(1.0f, 0.78f, 0.12f, 1.0f);
	const FLinearColor StartGateColor(0.08f, 0.9f, 1.0f, 1.0f);

	for (int32 Index = 0; Index < LocalPath.Num() - 1; ++Index)
	{
		const FVector SegmentStart = TransformTrackPoint(AnchorPawn, LocalPath[Index], RoadTopZ);
		const FVector SegmentEnd = TransformTrackPoint(AnchorPawn, LocalPath[Index + 1], RoadTopZ);
		const FVector Segment = SegmentEnd - SegmentStart;
		const float SegmentLength = Segment.Size2D();
		if (SegmentLength <= KINDA_SMALL_NUMBER)
		{
			continue;
		}

		const FVector Direction = Segment.GetSafeNormal2D();
		const FVector Right = FVector::CrossProduct(FVector::UpVector, Direction).GetSafeNormal();
		const FVector Midpoint = (SegmentStart + SegmentEnd) * 0.5f;
		const FRotator Rotation = Direction.Rotation();
		const FVector RoadCenter = FVector(Midpoint.X, Midpoint.Y, RoadTopZ - RoadThickness * 0.5f);

		SpawnTrackBlock(CubeMesh, BaseMaterial, RoadCenter, Rotation, FVector(SegmentLength / 100.0f, RoadWidth / 100.0f, RoadThickness / 100.0f), AsphaltColor, true, TEXT("UB_Road"));
		SpawnTrackBlock(CubeMesh, BaseMaterial, RoadCenter + Right * (RoadWidth * 0.5f + 150.0f), Rotation, FVector(SegmentLength / 100.0f, 2.7f, RoadThickness / 100.0f), ShoulderColor, true, TEXT("UB_Shoulder_R"));
		SpawnTrackBlock(CubeMesh, BaseMaterial, RoadCenter - Right * (RoadWidth * 0.5f + 150.0f), Rotation, FVector(SegmentLength / 100.0f, 2.7f, RoadThickness / 100.0f), ShoulderColor, true, TEXT("UB_Shoulder_L"));

		const FVector RailScale(SegmentLength / 100.0f, 0.28f, 1.15f);
		SpawnTrackBlock(CubeMesh, BaseMaterial, FVector(Midpoint.X, Midpoint.Y, RoadTopZ + 58.0f) + Right * (RoadWidth * 0.5f + 315.0f), Rotation, RailScale, RailColor, true, TEXT("UB_Rail_R"));
		SpawnTrackBlock(CubeMesh, BaseMaterial, FVector(Midpoint.X, Midpoint.Y, RoadTopZ + 58.0f) - Right * (RoadWidth * 0.5f + 315.0f), Rotation, RailScale, RailColor, true, TEXT("UB_Rail_L"));

		const FVector NeonScale(SegmentLength / 100.0f, 0.055f, 0.075f);
		SpawnTrackBlock(CubeMesh, BaseMaterial, FVector(Midpoint.X, Midpoint.Y, RoadTopZ + 18.0f) + Right * (RoadWidth * 0.5f - 54.0f), Rotation, NeonScale, RightNeonColor, false, TEXT("UB_Neon_R"));
		SpawnTrackBlock(CubeMesh, BaseMaterial, FVector(Midpoint.X, Midpoint.Y, RoadTopZ + 18.0f) - Right * (RoadWidth * 0.5f - 54.0f), Rotation, NeonScale, LeftNeonColor, false, TEXT("UB_Neon_L"));

		const int32 DashCount = FMath::Max(1, FMath::FloorToInt(SegmentLength / 650.0f));
		for (int32 DashIndex = 0; DashIndex < DashCount; ++DashIndex)
		{
			if ((DashIndex + Index) % 2 != 0)
			{
				continue;
			}

			const float Alpha = (static_cast<float>(DashIndex) + 0.5f) / static_cast<float>(DashCount);
			const FVector DashLocation = FMath::Lerp(SegmentStart, SegmentEnd, Alpha) + FVector::UpVector * 24.0f;
			SpawnTrackBlock(CubeMesh, BaseMaterial, DashLocation, Rotation, FVector(3.2f, 0.085f, 0.045f), CenterLineColor, false, TEXT("UB_CenterDash"));
		}

		AddTrackGameplayPoints(SegmentStart, SegmentEnd, Right, RoadTopZ, Index);
	}

	const FVector StartLocation = TransformTrackPoint(AnchorPawn, LocalPath[0], RoadTopZ);
	const FVector StartNext = TransformTrackPoint(AnchorPawn, LocalPath[1], RoadTopZ);
	const FVector StartDirection = (StartNext - StartLocation).GetSafeNormal2D();
	const FVector StartRight = FVector::CrossProduct(FVector::UpVector, StartDirection).GetSafeNormal();
	const FRotator StartRotation = StartDirection.Rotation();
	PrototypeTrackStartLocation = StartLocation + StartDirection * 650.0f + FVector::UpVector * 165.0f;
	PrototypeTrackStartRotation = StartRotation;
	bHasPrototypeTrackStart = true;

	SpawnTrackBlock(CubeMesh, BaseMaterial, StartLocation + FVector::UpVector * 170.0f + StartRight * 720.0f, StartRotation, FVector(0.32f, 0.24f, 3.3f), StartGateColor, false, TEXT("UB_StartGate_R"));
	SpawnTrackBlock(CubeMesh, BaseMaterial, StartLocation + FVector::UpVector * 170.0f - StartRight * 720.0f, StartRotation, FVector(0.32f, 0.24f, 3.3f), StartGateColor, false, TEXT("UB_StartGate_L"));
	SpawnTrackBlock(CubeMesh, BaseMaterial, StartLocation + FVector::UpVector * 335.0f, StartRotation, FVector(0.32f, 15.4f, 0.2f), StartGateColor, false, TEXT("UB_StartGate_Top"));
	SpawnTrackBlock(CubeMesh, BaseMaterial, PrototypeTrackStartLocation + FVector::UpVector * 360.0f, StartRotation, FVector(0.18f, 9.0f, 0.18f), FLinearColor(1.0f, 0.22f, 0.05f, 1.0f), false, TEXT("UB_OverheadMarker"));

	MovePawnToPrototypeTrackStart(AnchorPawn);

	UE_LOG(LogTemp, Log, TEXT("[UnfriendBlur Track] Spawned combat test track with %d pickup points and %d target points"), PrototypePickupLocations.Num(), PrototypeTargetTransforms.Num());

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 9.0f, FColor::Cyan, TEXT("COMBAT TEST TRACK spawned and car moved to the new start gate"));
	}
}

AStaticMeshActor* UUBPowerPrototypeSubsystem::SpawnTrackBlock(UStaticMesh* Mesh, UMaterialInterface* BaseMaterial, const FVector& Location, const FRotator& Rotation, const FVector& Scale, const FLinearColor& Color, bool bCollisionEnabled, const FString& ActorLabel) const
{
	UWorld* World = GetWorld();
	if (!World || !Mesh)
	{
		return nullptr;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AStaticMeshActor* BlockActor = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), Location, Rotation, SpawnParams);
	if (!BlockActor)
	{
		return nullptr;
	}

	BlockActor->SetActorScale3D(Scale);
#if WITH_EDITOR
	BlockActor->SetActorLabel(ActorLabel);
#endif

	UStaticMeshComponent* MeshComponent = BlockActor->GetStaticMeshComponent();
	if (MeshComponent)
	{
		MeshComponent->SetStaticMesh(Mesh);
		MeshComponent->SetMobility(EComponentMobility::Movable);
		MeshComponent->SetCollisionEnabled(bCollisionEnabled ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
		MeshComponent->SetCollisionResponseToAllChannels(bCollisionEnabled ? ECR_Block : ECR_Ignore);
		MeshComponent->SetCanEverAffectNavigation(false);
		ApplyTrackMaterial(MeshComponent, BaseMaterial, Color, bCollisionEnabled ? 0.0f : 5.5f);
	}

	BlockActor->SetReplicates(false);
	return BlockActor;
}

void UUBPowerPrototypeSubsystem::ApplyTrackMaterial(UStaticMeshComponent* MeshComponent, UMaterialInterface* BaseMaterial, const FLinearColor& Color, float EmissiveStrength) const
{
	if (!MeshComponent)
	{
		return;
	}

	if (BaseMaterial)
	{
		MeshComponent->SetMaterial(0, BaseMaterial);
	}

	UMaterialInstanceDynamic* DynamicMaterial = MeshComponent->CreateAndSetMaterialInstanceDynamic(0);
	if (!DynamicMaterial)
	{
		return;
	}

	DynamicMaterial->SetVectorParameterValue(TEXT("Color"), Color);
	DynamicMaterial->SetVectorParameterValue(TEXT("BaseColor"), Color);
	DynamicMaterial->SetVectorParameterValue(TEXT("EmissiveColor"), Color * EmissiveStrength);
	DynamicMaterial->SetScalarParameterValue(TEXT("EmissiveStrength"), EmissiveStrength);
}

FVector UUBPowerPrototypeSubsystem::TransformTrackPoint(const APawn* AnchorPawn, const FVector2D& LocalPoint, float RoadTopZ) const
{
	if (!AnchorPawn)
	{
		return FVector(LocalPoint.X, LocalPoint.Y, RoadTopZ);
	}

	const FVector Forward = AnchorPawn->GetActorForwardVector().GetSafeNormal2D();
	const FVector Right = AnchorPawn->GetActorRightVector().GetSafeNormal2D();
	const FVector Origin = AnchorPawn->GetActorLocation();
	return FVector(Origin.X, Origin.Y, RoadTopZ) + Forward * LocalPoint.X + Right * LocalPoint.Y;
}

void UUBPowerPrototypeSubsystem::AddTrackGameplayPoints(const FVector& SegmentStart, const FVector& SegmentEnd, const FVector& Right, float RoadTopZ, int32 SegmentIndex)
{
	const FVector Direction = (SegmentEnd - SegmentStart).GetSafeNormal2D();
	const FRotator TargetRotation = Direction.Rotation();
	const float LateralPattern[4] = { -420.0f, 0.0f, 420.0f, -120.0f };

	for (int32 PickupIndex = 0; PickupIndex < 4; ++PickupIndex)
	{
		const float Alpha = 0.18f + static_cast<float>(PickupIndex) * 0.2f;
		const float Lateral = LateralPattern[(PickupIndex + SegmentIndex) % UE_ARRAY_COUNT(LateralPattern)];
		PrototypePickupLocations.Add(FMath::Lerp(SegmentStart, SegmentEnd, Alpha) + Right * Lateral + FVector::UpVector * 118.0f);
	}

	if (SegmentIndex >= 1 && SegmentIndex % 2 == 1)
	{
		const float Lateral = SegmentIndex % 4 == 1 ? -280.0f : 310.0f;
		const FVector TargetLocation = FMath::Lerp(SegmentStart, SegmentEnd, 0.58f) + Right * Lateral + FVector::UpVector * 88.0f;
		PrototypeTargetTransforms.Add(FTransform(TargetRotation, TargetLocation, FVector::OneVector));
	}
}

void UUBPowerPrototypeSubsystem::MovePawnToPrototypeTrackStart(APawn* AnchorPawn) const
{
	if (!AnchorPawn || !bHasPrototypeTrackStart)
	{
		return;
	}

	if (UPrimitiveComponent* RootPrimitive = Cast<UPrimitiveComponent>(AnchorPawn->GetRootComponent()))
	{
		if (RootPrimitive->IsSimulatingPhysics())
		{
			RootPrimitive->SetPhysicsLinearVelocity(FVector::ZeroVector, false);
			RootPrimitive->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector, false);
		}
	}

	AnchorPawn->SetActorLocationAndRotation(PrototypeTrackStartLocation, PrototypeTrackStartRotation, false, nullptr, ETeleportType::TeleportPhysics);
	AnchorPawn->ForceNetUpdate();
}

void UUBPowerPrototypeSubsystem::SpawnPrototypePickups(APawn* AnchorPawn)
{
	UWorld* World = GetWorld();
	if (!World || !AnchorPawn || bSpawnedPrototypePickups)
	{
		return;
	}

	bSpawnedPrototypePickups = true;

	const int32 PickupCount = PrototypePickupLocations.Num() > 0 ? PrototypePickupLocations.Num() : 32;
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

	UE_LOG(LogTemp, Log, TEXT("[UnfriendBlur Powers] Spawned %d prototype pickups"), PickupCount);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 6.0f, FColor::Cyan, TEXT("Power pickups spawned along the combat track"));
	}
}

FVector UUBPowerPrototypeSubsystem::FindPickupLocation(APawn* AnchorPawn, int32 PickupIndex) const
{
	UWorld* World = GetWorld();
	if (!World || !AnchorPawn)
	{
		return FVector::ZeroVector;
	}

	if (PrototypePickupLocations.IsValidIndex(PickupIndex))
	{
		return PrototypePickupLocations[PickupIndex];
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

	const int32 TargetCount = PrototypeTargetTransforms.Num() > 0 ? PrototypeTargetTransforms.Num() : 2;
	for (int32 Index = 0; Index < TargetCount; ++Index)
	{
		const FTransform SpawnTransform = FindTargetCarTransform(AnchorPawn, Index);

		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		AUBPrototypeTargetCar* TargetCar = World->SpawnActor<AUBPrototypeTargetCar>(AUBPrototypeTargetCar::StaticClass(), SpawnTransform, SpawnParams);
		if (TargetCar)
		{
			TargetCar->InitializePrototypeTarget(Index);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[UnfriendBlur Powers] Spawned %d prototype target cars"), TargetCount);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 6.0f, FColor::Orange, TEXT("Target cars spawned along the combat track"));
	}
}

FTransform UUBPowerPrototypeSubsystem::FindTargetCarTransform(APawn* AnchorPawn, int32 TargetIndex) const
{
	UWorld* World = GetWorld();
	if (!World || !AnchorPawn)
	{
		return FTransform::Identity;
	}

	if (PrototypeTargetTransforms.IsValidIndex(TargetIndex))
	{
		return PrototypeTargetTransforms[TargetIndex];
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
		return FTransform(FRotator(0.0f, AnchorPawn->GetActorRotation().Yaw, 0.0f), Hit.Location + FVector::UpVector * 85.0f, FVector::OneVector);
	}

	return FTransform(FRotator(0.0f, AnchorPawn->GetActorRotation().Yaw, 0.0f), Candidate + FVector::UpVector * 85.0f, FVector::OneVector);
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
	const FString Message = TEXT("UnfriendBlur: combat test track | F slot | B front power | N back power | G drop");
	UE_LOG(LogTemp, Log, TEXT("%s"), *Message);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 8.0f, FColor::Green, Message);
	}
}
