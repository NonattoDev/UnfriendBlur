#include "Prototype/UBPrototypeTargetCar.h"

#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Powers/UBPowerInventoryComponent.h"
#include "Vehicle/UBVehicleHealthComponent.h"
#include "Vehicle/UBVehicleStatusComponent.h"
#include "UObject/ConstructorHelpers.h"

AUBPrototypeTargetCar::AUBPrototypeTargetCar()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	bReplicates = true;
	SetReplicateMovement(true);

	Collision = CreateDefaultSubobject<UBoxComponent>(TEXT("Collision"));
	SetRootComponent(Collision);
	Collision->SetBoxExtent(FVector(245.0f, 108.0f, 58.0f));
	Collision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Collision->SetCollisionObjectType(ECC_Vehicle);
	Collision->SetCollisionResponseToAllChannels(ECR_Block);
	Collision->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	Collision->SetGenerateOverlapEvents(true);
	Collision->SetSimulatePhysics(true);
	Collision->SetEnableGravity(true);
	Collision->SetLinearDamping(0.7f);
	Collision->SetAngularDamping(1.15f);
	Collision->SetNotifyRigidBodyCollision(true);

	CarMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CarMesh"));
	CarMesh->SetupAttachment(Collision);
	CarMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CarMesh->SetGenerateOverlapEvents(false);

	MarkerLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("MarkerLight"));
	MarkerLight->SetupAttachment(Collision);
	MarkerLight->SetRelativeLocation(FVector(0.0f, 0.0f, 130.0f));
	MarkerLight->SetIntensity(750.0f);
	MarkerLight->SetAttenuationRadius(520.0f);
	MarkerLight->bUseInverseSquaredFalloff = false;

	PowerInventory = CreateDefaultSubobject<UUBPowerInventoryComponent>(TEXT("PowerInventory"));
	Health = CreateDefaultSubobject<UUBVehicleHealthComponent>(TEXT("Health"));
	Status = CreateDefaultSubobject<UUBVehicleStatusComponent>(TEXT("Status"));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SportsCarMesh(TEXT("/Game/Vehicles/SportsCar/SM_SportsCar.SM_SportsCar"));
	if (SportsCarMesh.Succeeded())
	{
		CarMesh->SetStaticMesh(SportsCarMesh.Object);
	}

	AutoPossessAI = EAutoPossessAI::Disabled;
}

void AUBPrototypeTargetCar::BeginPlay()
{
	Super::BeginPlay();

	if (Collision)
	{
		Collision->SetMassOverrideInKg(NAME_None, 1450.0f, true);
	}
}

void AUBPrototypeTargetCar::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!HasAuthority() || !Collision || !Collision->IsSimulatingPhysics() || DrivingWaypoints.Num() < 2)
	{
		return;
	}

	if (Health && Health->IsDestroyed())
	{
		Collision->SetPhysicsLinearVelocity(Collision->GetPhysicsLinearVelocity() * 0.88f, false);
		Collision->SetPhysicsAngularVelocityInRadians(Collision->GetPhysicsAngularVelocityInRadians() * 0.72f, false);
		return;
	}

	const FVector Velocity = Collision->GetPhysicsLinearVelocity();
	const FVector HorizontalVelocity(Velocity.X, Velocity.Y, 0.0f);
	const FVector AngularVelocity = Collision->GetPhysicsAngularVelocityInRadians();
	if (Velocity.Z > 720.0f || AngularVelocity.Size() > 5.8f)
	{
		ImpactRecoveryTimeRemaining = FMath::Max(ImpactRecoveryTimeRemaining, ImpactRecoveryHoldSeconds);
	}

	if (ImpactRecoveryTimeRemaining > 0.0f)
	{
		ImpactRecoveryTimeRemaining = FMath::Max(0.0f, ImpactRecoveryTimeRemaining - DeltaSeconds);
		Collision->AddForce(-FVector::UpVector * Collision->GetMass() * 520.0f, NAME_None, false);
		return;
	}

	AdvanceWaypointIfNeeded();

	const FVector Location = GetActorLocation();
	FVector ToTarget = DrivingWaypoints[CurrentWaypointIndex] - Location;
	ToTarget.Z = 0.0f;
	if (ToTarget.IsNearlyZero())
	{
		return;
	}

	const FVector Direction = ToTarget.GetSafeNormal();
	const float StatusSpeedScale = Status ? Status->GetSpeedScale() : 1.0f;
	const float TargetSpeed = DesiredCruiseSpeed * FMath::Clamp(StatusSpeedScale, 0.25f, 1.15f);
	const FVector DesiredHorizontalVelocity = Direction * TargetSpeed;
	FVector NewHorizontalVelocity = FMath::VInterpTo(HorizontalVelocity, DesiredHorizontalVelocity, DeltaSeconds, AccelerationResponse);
	NewHorizontalVelocity = NewHorizontalVelocity.GetClampedToMaxSize(TargetSpeed * 1.18f);

	const float CurrentSpeed = HorizontalVelocity.Size();
	if (CurrentSpeed < 240.0f)
	{
		LowSpeedSeconds += DeltaSeconds;
	}
	else
	{
		LowSpeedSeconds = 0.0f;
	}

	if (LowSpeedSeconds > StuckRecoverySeconds)
	{
		CurrentWaypointIndex = (CurrentWaypointIndex + 1) % DrivingWaypoints.Num();
		NewHorizontalVelocity = Direction * TargetSpeed * 0.65f;
		LowSpeedSeconds = 0.0f;
	}

	const float ClampedVerticalSpeed = FMath::Clamp(Velocity.Z, -1800.0f, 420.0f);
	Collision->SetPhysicsLinearVelocity(FVector(NewHorizontalVelocity.X, NewHorizontalVelocity.Y, ClampedVerticalSpeed), false);
	Collision->AddForce(-FVector::UpVector * Collision->GetMass() * 950.0f, NAME_None, false);

	const FRotator TargetRotation = Direction.Rotation();
	const FRotator NewRotation = FMath::RInterpTo(GetActorRotation(), TargetRotation, DeltaSeconds, TurnResponse);
	SetActorRotation(FRotator(0.0f, NewRotation.Yaw, 0.0f), ETeleportType::TeleportPhysics);

	const FVector YawVelocity = FVector::UpVector * FVector::DotProduct(AngularVelocity, FVector::UpVector);
	Collision->SetPhysicsAngularVelocityInRadians(YawVelocity * 0.48f, false);
}

void AUBPrototypeTargetCar::InitializePrototypeTarget(int32 InTargetIndex)
{
	InitializePrototypeTarget(InTargetIndex, TArray<FVector>());
}

void AUBPrototypeTargetCar::InitializePrototypeTarget(int32 InTargetIndex, const TArray<FVector>& InDrivingWaypoints)
{
	const FLinearColor MarkerColor = (InTargetIndex % 2 == 0)
		? FLinearColor(1.0f, 0.08f, 0.02f)
		: FLinearColor(0.1f, 0.45f, 1.0f);

	TargetIndex = InTargetIndex;
	DesiredCruiseSpeed = 1980.0f + static_cast<float>(InTargetIndex % 4) * 180.0f;
	DrivingWaypoints = InDrivingWaypoints;
	if (DrivingWaypoints.Num() > 1)
	{
		const int32 ClosestIndex = FindClosestWaypointIndex();
		CurrentWaypointIndex = (ClosestIndex + 1 + InTargetIndex % 2) % DrivingWaypoints.Num();
	}

	if (MarkerLight)
	{
		MarkerLight->SetLightColor(MarkerColor);
	}
}

int32 AUBPrototypeTargetCar::FindClosestWaypointIndex() const
{
	int32 BestIndex = 0;
	float BestDistanceSquared = TNumericLimits<float>::Max();
	for (int32 Index = 0; Index < DrivingWaypoints.Num(); ++Index)
	{
		const float DistanceSquared = FVector::DistSquared2D(GetActorLocation(), DrivingWaypoints[Index]);
		if (DistanceSquared < BestDistanceSquared)
		{
			BestDistanceSquared = DistanceSquared;
			BestIndex = Index;
		}
	}

	return BestIndex;
}

void AUBPrototypeTargetCar::AdvanceWaypointIfNeeded()
{
	if (DrivingWaypoints.Num() < 2)
	{
		return;
	}

	for (int32 Guard = 0; Guard < 3; ++Guard)
	{
		const float DistanceSquared = FVector::DistSquared2D(GetActorLocation(), DrivingWaypoints[CurrentWaypointIndex]);
		if (DistanceSquared > FMath::Square(WaypointAcceptanceRadius))
		{
			return;
		}

		CurrentWaypointIndex = (CurrentWaypointIndex + 1) % DrivingWaypoints.Num();
	}
}
