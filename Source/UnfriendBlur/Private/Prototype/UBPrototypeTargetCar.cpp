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
	PrimaryActorTick.bCanEverTick = false;
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
	Collision->SetMassOverrideInKg(NAME_None, 1450.0f, true);
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

void AUBPrototypeTargetCar::InitializePrototypeTarget(int32 InTargetIndex)
{
	const FLinearColor MarkerColor = (InTargetIndex % 2 == 0)
		? FLinearColor(1.0f, 0.08f, 0.02f)
		: FLinearColor(0.1f, 0.45f, 1.0f);

	if (MarkerLight)
	{
		MarkerLight->SetLightColor(MarkerColor);
	}
}
