#include "Powers/UBPowerMine.h"

#include "Components/PointLightComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"
#include "Powers/UBPowerFxActor.h"
#include "Powers/UBPowerInventoryComponent.h"
#include "Powers/UBPowerInteractionRules.h"
#include "Powers/UBPowerProjectile.h"
#include "Powers/UBPowerTypes.h"
#include "Powers/UBPowerVisuals.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

AUBPowerMine::AUBPowerMine()
{
	bReplicates = true;
	SetReplicateMovement(true);

	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	SetRootComponent(Collision);
	Collision->InitSphereRadius(88.0f);
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
	Mesh->SetRelativeScale3D(FVector(0.42f, 0.42f, 0.14f));

	PointLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("PointLight"));
	PointLight->SetupAttachment(Collision);
	PointLight->SetLightColor(UBPowerTypeLinearColor(EUBPowerType::Mine));
	PointLight->SetIntensity(900.0f);
	PointLight->SetAttenuationRadius(340.0f);
	PointLight->bUseInverseSquaredFalloff = false;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		Mesh->SetStaticMesh(CubeMesh.Object);
	}

	InitialLifeSpan = 24.0f;
	PrimaryActorTick.bCanEverTick = true;
}

void AUBPowerMine::BeginPlay()
{
	Super::BeginPlay();

	Collision->OnComponentBeginOverlap.AddDynamic(this, &AUBPowerMine::HandleOverlap);
	UBApplyPowerMaterial(Mesh, EUBPowerType::Mine, 10.0f);

	if (HasAuthority())
	{
		GetWorldTimerManager().SetTimer(ArmedTimerHandle, this, &AUBPowerMine::ArmMine, ArmedDelay, false);
	}
}

void AUBPowerMine::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const float Pulse = 1.0f + FMath::Sin(GetWorld()->GetTimeSeconds() * (bArmed ? 9.0f : 3.5f)) * (bArmed ? 0.12f : 0.04f);
	Mesh->AddLocalRotation(FRotator(0.0f, DeltaSeconds * 90.0f, 0.0f));
	Mesh->SetRelativeScale3D(FVector(0.42f, 0.42f, 0.14f) * Pulse);

	if (PointLight)
	{
		PointLight->SetIntensity((bArmed ? 1400.0f : 650.0f) * Pulse);
	}
}

void AUBPowerMine::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AUBPowerMine, SourceActor);
}

void AUBPowerMine::InitializeMine(AActor* InSourceActor)
{
	if (!HasAuthority())
	{
		return;
	}

	SourceActor = InSourceActor;
	SetOwner(InSourceActor);

	if (SourceActor)
	{
		Collision->IgnoreActorWhenMoving(SourceActor, true);
	}
}

void AUBPowerMine::ArmMine()
{
	bArmed = true;
}

void AUBPowerMine::HandleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority() || !bArmed || !OtherActor || OtherActor == this || OtherActor == SourceActor)
	{
		return;
	}

	if (AUBPowerProjectile* Projectile = Cast<AUBPowerProjectile>(OtherActor))
	{
		if (Projectile->IsShuntProjectile() && Projectile->GetSourceActor() != SourceActor && FUBPowerInteractionRules::ShouldMineDestroyShunt())
		{
			Projectile->DestroyByPowerCounter(EUBPowerType::Mine, SourceActor ? SourceActor : this, false);
			SpawnExplosionFx();
			Destroy();
		}
		return;
	}

	if (UUBPowerInventoryComponent* PowerComponent = OtherActor->FindComponentByClass<UUBPowerInventoryComponent>())
	{
		PowerComponent->ApplyPowerHit(EUBPowerType::Mine, SourceActor ? SourceActor : this, HitDeltaVelocity);
		SpawnExplosionFx();
		Destroy();
		return;
	}

	UPrimitiveComponent* Primitive = OtherComp ? OtherComp : Cast<UPrimitiveComponent>(OtherActor->GetRootComponent());
	if (Primitive)
	{
		const FVector Direction = (OtherActor->GetActorLocation() - GetActorLocation()).GetSafeNormal();
		Primitive->AddImpulse(Direction * HitDeltaVelocity + FVector::UpVector * (HitDeltaVelocity * 0.45f), NAME_None, true);
	}

	SpawnExplosionFx();
	Destroy();
}

void AUBPowerMine::SpawnExplosionFx()
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

	AUBPowerFxActor* FxActor = World->SpawnActor<AUBPowerFxActor>(AUBPowerFxActor::StaticClass(), GetActorLocation() + FVector::UpVector * 80.0f, GetActorRotation(), SpawnParams);
	if (FxActor)
	{
		FxActor->InitializePowerFx(EUBPowerType::Mine, FVector::UpVector, 0.7f, 1.0f);
	}
}
