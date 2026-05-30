#include "Powers/UBPowerFxActor.h"

#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"
#include "Powers/UBPowerVisuals.h"
#include "UObject/ConstructorHelpers.h"

AUBPowerFxActor::AUBPowerFxActor()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicateMovement(true);

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	CoreMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CoreMesh"));
	CoreMesh->SetupAttachment(SceneRoot);
	CoreMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	AxisMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AxisMesh"));
	AxisMesh->SetupAttachment(SceneRoot);
	AxisMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	PointLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("PointLight"));
	PointLight->SetupAttachment(SceneRoot);
	PointLight->SetAttenuationRadius(620.0f);
	PointLight->SetIntensity(2400.0f);
	PointLight->bUseInverseSquaredFalloff = false;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));

	if (CubeMesh.Succeeded())
	{
		CoreMesh->SetStaticMesh(CubeMesh.Object);
		AxisMesh->SetStaticMesh(CubeMesh.Object);
	}

	for (int32 Index = 0; Index < 14; ++Index)
	{
		const FName ComponentName(*FString::Printf(TEXT("OrbitSpark_%02d"), Index));
		UStaticMeshComponent* OrbitMesh = CreateDefaultSubobject<UStaticMeshComponent>(ComponentName);
		OrbitMesh->SetupAttachment(SceneRoot);
		OrbitMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		if (CubeMesh.Succeeded())
		{
			OrbitMesh->SetStaticMesh(CubeMesh.Object);
		}
		OrbitMeshes.Add(OrbitMesh);
	}

	InitialLifeSpan = 1.0f;
}

void AUBPowerFxActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	AgeSeconds += DeltaSeconds;
	const float Alpha = LifeSeconds > KINDA_SMALL_NUMBER ? FMath::Clamp(AgeSeconds / LifeSeconds, 0.0f, 1.0f) : 1.0f;
	const float Pulse = 1.0f + FMath::Sin((AgeSeconds * 18.0f) + static_cast<float>(PowerType) * 0.37f) * 0.12f;
	const float SuperScale = bIsSuper ? 1.22f : 1.0f;

	CoreMesh->SetRelativeRotation(FRotator(AgeSeconds * 90.0f, AgeSeconds * 130.0f, AgeSeconds * 40.0f));
	CoreMesh->SetRelativeScale3D(FVector(0.16f, 0.16f, 0.32f) * VisualScale * Pulse * SuperScale);
	AxisMesh->SetRelativeRotation(FRotator(AgeSeconds * 130.0f, AgeSeconds * 95.0f, AgeSeconds * 67.0f));
	AxisMesh->SetRelativeScale3D(FVector(0.035f * VisualScale, 0.82f * VisualScale, 0.035f * VisualScale) * (1.0f + Alpha * 0.25f) * SuperScale);

	UpdateOrbitMeshes(Alpha);

	if (PointLight)
	{
		PointLight->SetIntensity(FMath::Lerp(bIsSuper ? 4200.0f : 2200.0f, 0.0f, Alpha) * FMath::Max(0.15f, Pulse));
		PointLight->SetAttenuationRadius((bIsSuper ? 500.0f : 340.0f) * VisualScale * (1.0f + Alpha * 0.42f));
	}
}

void AUBPowerFxActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AUBPowerFxActor, PowerType);
	DOREPLIFETIME(AUBPowerFxActor, Direction);
	DOREPLIFETIME(AUBPowerFxActor, LifeSeconds);
	DOREPLIFETIME(AUBPowerFxActor, VisualScale);
	DOREPLIFETIME(AUBPowerFxActor, bIsSuper);
}

void AUBPowerFxActor::InitializePowerFx(EUBPowerType InPowerType, FVector InDirection, float InLifeSeconds, float InVisualScale, bool bInIsSuper)
{
	PowerType = InPowerType;
	bIsSuper = bInIsSuper;
	Direction = InDirection.GetSafeNormal();
	if (Direction.IsNearlyZero())
	{
		Direction = FVector::ForwardVector;
	}

	LifeSeconds = FMath::Max(0.1f, InLifeSeconds);
	VisualScale = FMath::Max(0.1f, InVisualScale);
	InitialLifeSpan = LifeSeconds;
	SetLifeSpan(LifeSeconds);
	SetActorRotation(Direction.Rotation());
	ApplyVisualState();
}

void AUBPowerFxActor::OnRep_VisualState()
{
	ApplyVisualState();
}

void AUBPowerFxActor::ApplyVisualState()
{
	const FLinearColor PowerColor = UBPowerTypeLinearColor(PowerType);

	UBApplyPowerMaterial(CoreMesh, PowerType, bIsSuper ? 16.0f : 8.0f);
	UBApplyPowerMaterial(AxisMesh, PowerType, bIsSuper ? 12.0f : 6.0f);

	for (UStaticMeshComponent* OrbitMesh : OrbitMeshes)
	{
		UBApplyPowerMaterial(OrbitMesh, PowerType, bIsSuper ? 18.0f : 10.0f);
	}

	if (PointLight)
	{
		PointLight->SetLightColor(PowerColor);
		PointLight->SetIntensity(bIsSuper ? 4200.0f : 2200.0f);
		PointLight->SetAttenuationRadius((bIsSuper ? 520.0f : 350.0f) * VisualScale);
	}
}

void AUBPowerFxActor::UpdateOrbitMeshes(float Alpha)
{
	const float SuperScale = bIsSuper ? 1.25f : 1.0f;
	const float BaseRadius = FMath::Lerp(48.0f, 145.0f, Alpha) * VisualScale * SuperScale;
	const float SparkScale = FMath::Lerp(bIsSuper ? 0.075f : 0.055f, bIsSuper ? 0.026f : 0.018f, Alpha) * VisualScale;

	for (int32 Index = 0; Index < OrbitMeshes.Num(); ++Index)
	{
		UStaticMeshComponent* OrbitMesh = OrbitMeshes[Index];
		if (!OrbitMesh)
		{
			continue;
		}

		const float Phase = static_cast<float>(Index) / static_cast<float>(OrbitMeshes.Num());
		const float Angle = AgeSeconds * (2.4f + Phase * 1.9f) + Phase * UE_TWO_PI;
		const float Height = FMath::Sin(Angle * 1.7f) * 38.0f * VisualScale;
		const float Radius = BaseRadius * (0.72f + 0.28f * FMath::Sin(Angle * 0.63f));
		const FVector LocalLocation(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, Height);

		OrbitMesh->SetRelativeLocation(LocalLocation);
		OrbitMesh->SetRelativeScale3D(FVector(SparkScale));
		OrbitMesh->SetVisibility(Alpha < 0.96f, true);
	}
}
