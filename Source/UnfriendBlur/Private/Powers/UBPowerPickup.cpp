#include "Powers/UBPowerPickup.h"

#include "Components/BillboardComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Net/UnrealNetwork.h"
#include "Powers/UBPowerInventoryComponent.h"
#include "Powers/UBPowerVisuals.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

AUBPowerPickup::AUBPowerPickup()
{
	PrimaryActorTick.bCanEverTick = true;
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

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Collision);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetRelativeLocation(FVector(0.0f, 0.0f, 28.0f));
	Mesh->SetRelativeRotation(FRotator(45.0f, 45.0f, 0.0f));
	Mesh->SetRelativeScale3D(FVector(0.13f, 0.13f, 0.32f));

	IconBillboard = CreateDefaultSubobject<UBillboardComponent>(TEXT("IconBillboard"));
	IconBillboard->SetupAttachment(Collision);
	IconBillboard->SetRelativeLocation(FVector(0.0f, 0.0f, 86.0f));
	IconBillboard->SetRelativeScale3D(FVector(0.16f));
	IconBillboard->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	PointLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("PointLight"));
	PointLight->SetupAttachment(Collision);
	PointLight->SetIntensity(760.0f);
	PointLight->SetAttenuationRadius(275.0f);
	PointLight->bUseInverseSquaredFalloff = false;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		Mesh->SetStaticMesh(CubeMesh.Object);
	}

	AvailablePowers =
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
}

void AUBPowerPickup::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RefreshVisuals();
}

void AUBPowerPickup::BeginPlay()
{
	Super::BeginPlay();

	Collision->OnComponentBeginOverlap.AddDynamic(this, &AUBPowerPickup::HandleOverlap);
	RefreshVisuals();
}

void AUBPowerPickup::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bAvailable && Mesh)
	{
		const float Pulse = 1.0f + FMath::Sin(GetWorld()->GetTimeSeconds() * 5.0f) * 0.08f;
		Mesh->AddLocalRotation(FRotator(0.0f, 90.0f * DeltaSeconds, 0.0f));
		Mesh->SetRelativeScale3D(FVector(0.13f, 0.13f, 0.32f) * Pulse);

		if (IconBillboard)
		{
			IconBillboard->SetWorldRotation(FRotator(0.0f, 180.0f, 0.0f));
			IconBillboard->SetRelativeScale3D(FVector(0.16f + Pulse * 0.01f));
		}

		if (PointLight)
		{
			PointLight->SetIntensity(760.0f * Pulse);
		}
	}
}

void AUBPowerPickup::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AUBPowerPickup, FixedPower);
	DOREPLIFETIME(AUBPowerPickup, bIsSuperPickup);
	DOREPLIFETIME(AUBPowerPickup, bRespawns);
}

void AUBPowerPickup::HandleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority() || !bAvailable || !OtherActor)
	{
		return;
	}

	const APawn* OtherPawn = Cast<APawn>(OtherActor);
	if (!OtherPawn || !OtherPawn->IsPlayerControlled())
	{
		return;
	}

	UUBPowerInventoryComponent* PowerComponent = UUBPowerInventoryComponent::FindOrCreatePowerComponent(OtherActor);
	if (!PowerComponent)
	{
		return;
	}

	const EUBPowerType ChosenPower = ChoosePower();
	const bool bGiveSuper = bIsSuperPickup && ChosenPower != EUBPowerType::None;
	if (PowerComponent->GivePowerSlot(FUBPowerSlot(ChosenPower, bGiveSuper)))
	{
		SetAvailable(false);
		if (bRespawns && RespawnSeconds > 0.0f)
		{
			GetWorldTimerManager().SetTimer(RespawnTimerHandle, this, &AUBPowerPickup::Respawn, RespawnSeconds, false);
		}
		else
		{
			Destroy();
		}
	}
}

EUBPowerType AUBPowerPickup::ChoosePower() const
{
	if (FixedPower != EUBPowerType::None)
	{
		return FixedPower;
	}

	if (AvailablePowers.Num() == 0)
	{
		return EUBPowerType::Boost;
	}

	return AvailablePowers[FMath::RandRange(0, AvailablePowers.Num() - 1)];
}

EUBPowerType AUBPowerPickup::GetVisualPower() const
{
	if (FixedPower != EUBPowerType::None)
	{
		return FixedPower;
	}

	return EUBPowerType::None;
}

void AUBPowerPickup::OnRep_PickupVisualState()
{
	RefreshVisuals();
}

void AUBPowerPickup::RefreshVisuals()
{
	const EUBPowerType VisualPower = GetVisualPower();
	const FLinearColor PowerColor = VisualPower == EUBPowerType::None ? FLinearColor(1.0f, 0.9f, 0.25f, 1.0f) : UBPowerTypeLinearColor(VisualPower);

	if (Mesh)
	{
		UBApplyPowerMaterial(Mesh, VisualPower == EUBPowerType::None ? EUBPowerType::Boost : VisualPower, 7.0f);
	}

	if (IconBillboard)
	{
		UTexture2D* IconTexture = UBLoadPowerIcon(VisualPower, bIsSuperPickup && VisualPower != EUBPowerType::None);
		IconBillboard->SetSprite(IconTexture);
		IconBillboard->SetHiddenInGame(VisualPower == EUBPowerType::None || !IconTexture);
	}

	if (PointLight)
	{
		PointLight->SetLightColor(PowerColor);
		PointLight->SetIntensity(bIsSuperPickup ? 1180.0f : 760.0f);
	}
}

void AUBPowerPickup::SetAvailable(bool bNewAvailable)
{
	bAvailable = bNewAvailable;
	SetActorHiddenInGame(!bAvailable);
	SetActorEnableCollision(bAvailable);
}

void AUBPowerPickup::Respawn()
{
	SetAvailable(true);
}
