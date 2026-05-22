#include "Powers/UBPowerProjectile.h"

#include "Components/PointLightComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "Powers/UBPowerFxActor.h"
#include "Powers/UBPowerInventoryComponent.h"
#include "Powers/UBPowerInteractionRules.h"
#include "Powers/UBPowerVisuals.h"
#include "UObject/ConstructorHelpers.h"

AUBPowerProjectile::AUBPowerProjectile()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicateMovement(true);

	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	SetRootComponent(Collision);
	Collision->InitSphereRadius(30.0f);
	Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Collision->SetCollisionObjectType(ECC_WorldDynamic);
	Collision->SetCollisionResponseToAllChannels(ECR_Ignore);
	Collision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	Collision->SetCollisionResponseToChannel(ECC_Vehicle, ECR_Overlap);
	Collision->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Overlap);
	Collision->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Collision);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetRelativeRotation(FRotator(45.0f, 45.0f, 0.0f));
	Mesh->SetRelativeScale3D(FVector(0.16f, 0.16f, 0.34f));

	TrailMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TrailMesh"));
	TrailMesh->SetupAttachment(Collision);
	TrailMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TrailMesh->SetRelativeLocation(FVector(-86.0f, 0.0f, 0.0f));
	TrailMesh->SetRelativeScale3D(FVector(0.95f, 0.08f, 0.08f));

	PointLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("PointLight"));
	PointLight->SetupAttachment(Collision);
	PointLight->SetIntensity(1800.0f);
	PointLight->SetAttenuationRadius(420.0f);
	PointLight->bUseInverseSquaredFalloff = false;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		Mesh->SetStaticMesh(CubeMesh.Object);
		TrailMesh->SetStaticMesh(CubeMesh.Object);
	}

	Movement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Movement"));
	Movement->InitialSpeed = BoltSpeed;
	Movement->MaxSpeed = BoltSpeed;
	Movement->ProjectileGravityScale = 0.0f;
	Movement->bRotationFollowsVelocity = true;

	InitialLifeSpan = 5.0f;
}

void AUBPowerProjectile::BeginPlay()
{
	Super::BeginPlay();

	Collision->OnComponentBeginOverlap.AddDynamic(this, &AUBPowerProjectile::HandleOverlap);
	ApplyVisualStyle();

	if (HasAuthority())
	{
		AcquireHomingTarget();
	}
}

void AUBPowerProjectile::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (HasAuthority() && bHoming && Movement && !Movement->HomingTargetComponent.IsValid())
	{
		AcquireHomingTarget();
	}

	if (Mesh)
	{
		Mesh->AddLocalRotation(FRotator(0.0f, 0.0f, DeltaSeconds * 720.0f));
	}

	if (TrailMesh)
	{
		const float Pulse = 1.0f + FMath::Sin(GetWorld()->GetTimeSeconds() * 24.0f) * 0.08f;
		const float WeakenedScale = bWeakenedShunt ? 0.58f : 1.0f;
		TrailMesh->SetRelativeScale3D((bHoming ? FVector(0.92f, 0.12f, 0.12f) : FVector(1.08f, 0.08f, 0.08f)) * Pulse * WeakenedScale);
	}
}

void AUBPowerProjectile::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AUBPowerProjectile, PowerType);
	DOREPLIFETIME(AUBPowerProjectile, SourceActor);
	DOREPLIFETIME(AUBPowerProjectile, bHoming);
	DOREPLIFETIME(AUBPowerProjectile, bFireForward);
	DOREPLIFETIME(AUBPowerProjectile, CurrentCounterHealth);
	DOREPLIFETIME(AUBPowerProjectile, bWeakenedShunt);
}

void AUBPowerProjectile::InitializeProjectile(EUBPowerType InPowerType, AActor* InSourceActor, bool bInHoming, bool bInFireForward)
{
	if (!HasAuthority())
	{
		return;
	}

	PowerType = InPowerType;
	SourceActor = InSourceActor;
	bHoming = bInHoming;
	bFireForward = bInFireForward;
	CurrentCounterHealth = PowerType == EUBPowerType::Shunt ? ShuntCounterHealth : 0;
	bWeakenedShunt = false;
	SetOwner(InSourceActor);
	ApplyVisualStyle();

	if (SourceActor)
	{
		Collision->IgnoreActorWhenMoving(SourceActor, true);
	}

	Collision->SetSphereRadius(bHoming ? 38.0f : 28.0f, true);

	const float Speed = bHoming ? ShuntSpeed : BoltSpeed;
	Movement->InitialSpeed = Speed;
	Movement->MaxSpeed = Speed;
	Movement->Velocity = GetActorForwardVector() * Speed;

	HitDeltaVelocity = bHoming ? 3600.0f : 2600.0f;
	AcquireHomingTarget();
}

bool AUBPowerProjectile::IsShuntProjectile() const
{
	return PowerType == EUBPowerType::Shunt;
}

bool AUBPowerProjectile::ApplyProjectileCounterHit(EUBPowerType IncomingPowerType, AActor* IncomingSourceActor)
{
	const int32 CounterDamage = FUBPowerInteractionRules::GetProjectileDamageAgainstShunt(IncomingPowerType);
	if (!HasAuthority() || !IsShuntProjectile() || CounterDamage <= 0)
	{
		return false;
	}

	CurrentCounterHealth = FMath::Max(0, CurrentCounterHealth - CounterDamage);
	if (CurrentCounterHealth <= 0)
	{
		SpawnCounterFx(IncomingPowerType, true);
		Destroy();
		return true;
	}

	bWeakenedShunt = true;
	HitDeltaVelocity = WeakenedShuntHitDeltaVelocity;
	if (Movement)
	{
		Movement->MaxSpeed = ShuntSpeed * 0.78f;
		Movement->HomingAccelerationMagnitude = HomingAcceleration * 0.62f;
	}

	ApplyVisualStyle();
	SpawnCounterFx(IncomingPowerType, false);
	return true;
}

bool AUBPowerProjectile::DestroyByPowerCounter(EUBPowerType CounterPowerType, AActor* CounterSourceActor, bool bSpawnAreaPulse)
{
	if (!HasAuthority() || !IsShuntProjectile())
	{
		return false;
	}

	SpawnCounterFx(CounterPowerType, true);
	if (bSpawnAreaPulse)
	{
		ApplyDestructionPulse(CounterPowerType, CounterSourceActor);
	}

	Destroy();
	return true;
}

void AUBPowerProjectile::HandleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority() || !OtherActor || OtherActor == this || OtherActor == SourceActor)
	{
		return;
	}

	if (AUBPowerProjectile* OtherProjectile = Cast<AUBPowerProjectile>(OtherActor))
	{
		if (OtherProjectile->GetSourceActor() == SourceActor)
		{
			return;
		}

		if (PowerType == EUBPowerType::Bolt && OtherProjectile->IsShuntProjectile())
		{
			if (OtherProjectile->ApplyProjectileCounterHit(PowerType, SourceActor ? SourceActor : this))
			{
				SpawnImpactFx();
				Destroy();
			}
			return;
		}

		return;
	}

	if (UUBPowerInventoryComponent* PowerComponent = OtherActor->FindComponentByClass<UUBPowerInventoryComponent>())
	{
		PowerComponent->ApplyPowerHitWithContext(PowerType, SourceActor ? SourceActor : this, GetEffectiveHitDeltaVelocity(), bWeakenedShunt);
		SpawnImpactFx();
		Destroy();
		return;
	}

	UPrimitiveComponent* Primitive = OtherComp ? OtherComp : Cast<UPrimitiveComponent>(OtherActor->GetRootComponent());
	if (Primitive)
	{
		const FVector Direction = (OtherActor->GetActorLocation() - GetActorLocation()).GetSafeNormal();
		const float EffectiveDeltaVelocity = GetEffectiveHitDeltaVelocity();
		Primitive->AddImpulse(Direction * EffectiveDeltaVelocity + FVector::UpVector * (EffectiveDeltaVelocity * (bWeakenedShunt ? 0.08f : 0.25f)), NAME_None, true);
	}

	SpawnImpactFx();
	Destroy();
}

void AUBPowerProjectile::AcquireHomingTarget()
{
	if (!bHoming || !Movement)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	AActor* BestTarget = nullptr;
	float BestScore = TNumericLimits<float>::Lowest();
	const FVector Start = GetActorLocation();
	const FVector Forward = GetActorForwardVector();

	for (TActorIterator<APawn> It(World); It; ++It)
	{
		APawn* Candidate = *It;
		if (!Candidate || Candidate == SourceActor)
		{
			continue;
		}

		USceneComponent* CandidateRoot = Candidate->GetRootComponent();
		if (!CandidateRoot)
		{
			continue;
		}

		const FVector ToCandidate = Candidate->GetActorLocation() - Start;
		const float Distance = ToCandidate.Size();
		if (Distance <= KINDA_SMALL_NUMBER || Distance > HomingRange)
		{
			continue;
		}

		const float ForwardDot = FVector::DotProduct(Forward, ToCandidate / Distance);
		if (ForwardDot < 0.2f)
		{
			continue;
		}

		const float Score = ForwardDot * 100000.0f - Distance;
		if (Score > BestScore)
		{
			BestScore = Score;
			BestTarget = Candidate;
		}
	}

	if (BestTarget && BestTarget->GetRootComponent())
	{
		Movement->bIsHomingProjectile = true;
		Movement->HomingAccelerationMagnitude = HomingAcceleration;
		Movement->HomingTargetComponent = BestTarget->GetRootComponent();
	}
}

void AUBPowerProjectile::ApplyVisualStyle()
{
	if (!Mesh)
	{
		return;
	}

	const FLinearColor PowerColor = UBPowerTypeLinearColor(PowerType);
	const bool bWeakenedVisual = IsShuntProjectile() && bWeakenedShunt;
	const float WeakenedScale = bWeakenedVisual ? 0.72f : 1.0f;
	UBApplyPowerMaterial(Mesh, PowerType, 12.0f);
	UBApplyPowerMaterial(TrailMesh, PowerType, 8.0f);
	Mesh->SetRelativeScale3D((bHoming ? FVector(0.2f, 0.2f, 0.42f) : FVector(0.15f, 0.15f, 0.32f)) * WeakenedScale);

	if (PointLight)
	{
		PointLight->SetLightColor(PowerColor);
		PointLight->SetIntensity((bHoming ? 2600.0f : 1900.0f) * (bWeakenedVisual ? 0.52f : 1.0f));
		PointLight->SetAttenuationRadius((bHoming ? 520.0f : 390.0f) * (bWeakenedVisual ? 0.68f : 1.0f));
	}
}

void AUBPowerProjectile::OnRep_VisualStyle()
{
	ApplyVisualStyle();
}

void AUBPowerProjectile::SpawnImpactFx()
{
	if (!HasAuthority())
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = SourceActor ? SourceActor : this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AUBPowerFxActor* FxActor = World->SpawnActor<AUBPowerFxActor>(AUBPowerFxActor::StaticClass(), GetActorLocation(), GetActorRotation(), SpawnParams);
	if (FxActor)
	{
		FxActor->InitializePowerFx(PowerType, GetVelocity().GetSafeNormal(), bHoming ? 0.7f : 0.5f, bHoming ? 0.95f : 0.7f, false);
	}
}

void AUBPowerProjectile::SpawnCounterFx(EUBPowerType CounterPowerType, bool bDestroyed)
{
	if (!HasAuthority())
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = SourceActor ? SourceActor : this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	const FVector Direction = GetVelocity().IsNearlyZero() ? GetActorForwardVector() : GetVelocity().GetSafeNormal();
	const float LifeSeconds = bDestroyed ? 0.82f : 0.45f;
	const float VisualScale = bDestroyed ? 1.1f : 0.62f;

	AUBPowerFxActor* ShuntFx = World->SpawnActor<AUBPowerFxActor>(AUBPowerFxActor::StaticClass(), GetActorLocation(), Direction.Rotation(), SpawnParams);
	if (ShuntFx)
	{
		ShuntFx->InitializePowerFx(PowerType, Direction, LifeSeconds, VisualScale, bDestroyed);
	}

	AUBPowerFxActor* CounterFx = World->SpawnActor<AUBPowerFxActor>(AUBPowerFxActor::StaticClass(), GetActorLocation() + FVector::UpVector * 35.0f, Direction.Rotation(), SpawnParams);
	if (CounterFx)
	{
		CounterFx->InitializePowerFx(CounterPowerType, -Direction, bDestroyed ? 0.58f : 0.35f, bDestroyed ? 0.78f : 0.48f, false);
	}
}

void AUBPowerProjectile::ApplyDestructionPulse(EUBPowerType CounterPowerType, AActor* CounterSourceActor)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	constexpr float PulseRadius = 650.0f;
	constexpr float PulseDeltaVelocity = 1800.0f;
	TArray<FOverlapResult> Overlaps;
	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_PhysicsBody);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Vehicle);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(UBPowerProjectileCounterPulse), false, this);
	World->OverlapMultiByObjectType(Overlaps, GetActorLocation(), FQuat::Identity, ObjectQueryParams, FCollisionShape::MakeSphere(PulseRadius), QueryParams);

	TSet<AActor*> HitActors;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* OtherActor = Overlap.GetActor();
		if (!OtherActor || HitActors.Contains(OtherActor))
		{
			continue;
		}

		HitActors.Add(OtherActor);
		if (UUBPowerInventoryComponent* PowerComponent = OtherActor->FindComponentByClass<UUBPowerInventoryComponent>())
		{
			PowerComponent->ApplyPowerHitWithContext(CounterPowerType, CounterSourceActor ? CounterSourceActor : this, PulseDeltaVelocity, false);
			continue;
		}

		UPrimitiveComponent* Primitive = Cast<UPrimitiveComponent>(OtherActor->GetRootComponent());
		if (!Primitive || !Primitive->IsSimulatingPhysics())
		{
			Primitive = OtherActor->FindComponentByClass<UPrimitiveComponent>();
		}

		if (Primitive)
		{
			const FVector Direction = (OtherActor->GetActorLocation() - GetActorLocation()).GetSafeNormal();
			Primitive->AddImpulse(Direction * PulseDeltaVelocity + FVector::UpVector * (PulseDeltaVelocity * 0.25f), NAME_None, true);
		}
	}
}

float AUBPowerProjectile::GetEffectiveHitDeltaVelocity() const
{
	return bWeakenedShunt ? WeakenedShuntHitDeltaVelocity : HitDeltaVelocity;
}
