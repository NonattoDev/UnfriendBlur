#include "Vehicle/UBArcadeVehicleAssistComponent.h"

#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"

UUBArcadeVehicleAssistComponent::UUBArcadeVehicleAssistComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	SetIsReplicatedByDefault(false);
}

void UUBArcadeVehicleAssistComponent::BeginPlay()
{
	Super::BeginPlay();

	SetComponentTickEnabled(bAssistEnabled);
	if (!bAssistEnabled)
	{
		return;
	}

	CachedPrimitive = FindBestPrimitive();
	if (CachedPrimitive)
	{
		CachedPrimitive->SetNotifyRigidBodyCollision(true);
		CachedPrimitive->OnComponentHit.AddUniqueDynamic(this, &UUBArcadeVehicleAssistComponent::HandleOwnerComponentHit);
	}
}

void UUBArcadeVehicleAssistComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	AActor* OwnerActor = GetOwner();
	if (!bAssistEnabled || !OwnerActor || !OwnerActor->HasAuthority())
	{
		return;
	}

	UPrimitiveComponent* Primitive = CachedPrimitive.Get();
	if (!Primitive || !Primitive->IsSimulatingPhysics())
	{
		Primitive = FindBestPrimitive();
		CachedPrimitive = Primitive;
	}

	if (!Primitive || !Primitive->IsSimulatingPhysics())
	{
		return;
	}

	float GroundDistance = 0.0f;
	const bool bGrounded = IsGrounded(GroundDistance);
	const float Speed = Primitive->GetPhysicsLinearVelocity().Size2D();

	ApplyDownforce(Primitive, Speed, bGrounded);
	ApplyAntiFlipStability(Primitive, bGrounded);
	ApplyCurbLaunchClamp(Primitive, bGrounded, GroundDistance);
	ApplyArcadeSteering(Primitive, DeltaTime, Speed, bGrounded);
	ApplySteeringAndDriftAssist(Primitive, DeltaTime, Speed);

	bWasGrounded = bGrounded;
}

UUBArcadeVehicleAssistComponent* UUBArcadeVehicleAssistComponent::FindOrCreateAssistComponent(AActor* OwnerActor)
{
	if (!OwnerActor)
	{
		return nullptr;
	}

	if (UUBArcadeVehicleAssistComponent* ExistingComponent = OwnerActor->FindComponentByClass<UUBArcadeVehicleAssistComponent>())
	{
		return ExistingComponent;
	}

	UUBArcadeVehicleAssistComponent* NewComponent = NewObject<UUBArcadeVehicleAssistComponent>(OwnerActor, UUBArcadeVehicleAssistComponent::StaticClass(), TEXT("UBArcadeVehicleAssist"));
	if (!NewComponent)
	{
		return nullptr;
	}

	OwnerActor->AddInstanceComponent(NewComponent);
	NewComponent->RegisterComponent();
	return NewComponent;
}

UPrimitiveComponent* UUBArcadeVehicleAssistComponent::FindBestPrimitive() const
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
	{
		return nullptr;
	}

	UPrimitiveComponent* Primitive = Cast<UPrimitiveComponent>(OwnerActor->GetRootComponent());
	if (Primitive && Primitive->IsSimulatingPhysics())
	{
		return Primitive;
	}

	for (UActorComponent* Component : OwnerActor->GetComponents())
	{
		UPrimitiveComponent* Candidate = Cast<UPrimitiveComponent>(Component);
		if (Candidate && Candidate->IsSimulatingPhysics())
		{
			return Candidate;
		}
	}

	return Primitive ? Primitive : OwnerActor->FindComponentByClass<UPrimitiveComponent>();
}

bool UUBArcadeVehicleAssistComponent::IsGrounded(float& OutGroundDistance) const
{
	OutGroundDistance = GroundProbeDistance;

	const AActor* OwnerActor = GetOwner();
	const UWorld* World = GetWorld();
	if (!OwnerActor || !World)
	{
		return false;
	}

	FHitResult Hit;
	const FVector Start = OwnerActor->GetActorLocation() + FVector::UpVector * 40.0f;
	const FVector End = Start - FVector::UpVector * GroundProbeDistance;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(UBArcadeVehicleGroundProbe), false, OwnerActor);
	const bool bHitGround = World->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, QueryParams);
	if (bHitGround)
	{
		OutGroundDistance = Hit.Distance;
	}

	return bHitGround;
}

float UUBArcadeVehicleAssistComponent::GetSteeringInput() const
{
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	const APlayerController* PlayerController = OwnerPawn ? Cast<APlayerController>(OwnerPawn->GetController()) : nullptr;
	if (!PlayerController || !PlayerController->IsLocalController())
	{
		return 0.0f;
	}

	float Steering = 0.0f;
	if (PlayerController->IsInputKeyDown(EKeys::A) || PlayerController->IsInputKeyDown(EKeys::Left))
	{
		Steering -= 1.0f;
	}

	if (PlayerController->IsInputKeyDown(EKeys::D) || PlayerController->IsInputKeyDown(EKeys::Right))
	{
		Steering += 1.0f;
	}

	return FMath::Clamp(Steering, -1.0f, 1.0f);
}

bool UUBArcadeVehicleAssistComponent::IsDriftInputDown() const
{
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	const APlayerController* PlayerController = OwnerPawn ? Cast<APlayerController>(OwnerPawn->GetController()) : nullptr;
	return PlayerController
		&& PlayerController->IsLocalController()
		&& PlayerController->IsInputKeyDown(EKeys::SpaceBar);
}

void UUBArcadeVehicleAssistComponent::ApplyDownforce(UPrimitiveComponent* Primitive, float Speed, bool bGrounded) const
{
	if (!Primitive)
	{
		return;
	}

	const float GroundedDownforce = FMath::Clamp(BaseDownforceAcceleration + Speed * Speed * SpeedDownforceCoefficient, 0.0f, MaxDownforceAcceleration);
	const float DownforceAcceleration = bGrounded ? GroundedDownforce : AirDownforceAcceleration;
	Primitive->AddForce(-FVector::UpVector * Primitive->GetMass() * DownforceAcceleration, NAME_None, false);
}

void UUBArcadeVehicleAssistComponent::ApplyAntiFlipStability(UPrimitiveComponent* Primitive, bool bGrounded) const
{
	const AActor* OwnerActor = GetOwner();
	if (!Primitive || !OwnerActor)
	{
		return;
	}

	const FVector Up = OwnerActor->GetActorUpVector();
	const float UprightDot = FVector::DotProduct(Up, FVector::UpVector);
	const FVector CorrectionAxis = FVector::CrossProduct(Up, FVector::UpVector);
	if (!CorrectionAxis.IsNearlyZero())
	{
		const float TorqueStrength = UprightDot < 0.55f ? StrongUprightTorqueStrength : UprightTorqueStrength;
		Primitive->AddTorqueInRadians(CorrectionAxis.GetSafeNormal() * TorqueStrength * FMath::Clamp(1.0f - UprightDot, 0.0f, 1.5f), NAME_None, true);
	}

	const FVector AngularVelocity = Primitive->GetPhysicsAngularVelocityInRadians();
	const FVector YawAngularVelocity = FVector::UpVector * FVector::DotProduct(AngularVelocity, FVector::UpVector);
	const FVector RollPitchAngularVelocity = AngularVelocity - YawAngularVelocity;
	const float Damping = bGrounded ? RollPitchAngularDamping : AirRollPitchAngularDamping;
	Primitive->SetPhysicsAngularVelocityInRadians(YawAngularVelocity + RollPitchAngularVelocity * Damping, false);
}

void UUBArcadeVehicleAssistComponent::ApplyCurbLaunchClamp(UPrimitiveComponent* Primitive, bool bGrounded, float GroundDistance) const
{
	if (!Primitive || !bGrounded || GroundDistance > GroundProbeDistance * 0.72f)
	{
		return;
	}

	FVector Velocity = Primitive->GetPhysicsLinearVelocity();
	if (Velocity.Z > MaxNearGroundUpVelocity)
	{
		Velocity.Z = MaxNearGroundUpVelocity;
		Primitive->SetPhysicsLinearVelocity(Velocity, false);
	}
}

void UUBArcadeVehicleAssistComponent::ApplyArcadeSteering(UPrimitiveComponent* Primitive, float DeltaTime, float Speed, bool bGrounded) const
{
	const AActor* OwnerActor = GetOwner();
	if (!Primitive || !OwnerActor || !bGrounded || Speed < ArcadeSteeringMinSpeed || IsDriftInputDown())
	{
		return;
	}

	const float Steering = GetSteeringInput();
	if (FMath::IsNearlyZero(Steering, 0.05f))
	{
		return;
	}

	const FVector Velocity = Primitive->GetPhysicsLinearVelocity();
	const FVector HorizontalVelocity(Velocity.X, Velocity.Y, 0.0f);
	const float HorizontalSpeed = HorizontalVelocity.Size();
	if (HorizontalSpeed < ArcadeSteeringMinSpeed)
	{
		return;
	}

	const FVector Forward = OwnerActor->GetActorForwardVector().GetSafeNormal2D();
	const float ForwardSpeedSign = FVector::DotProduct(HorizontalVelocity.GetSafeNormal(), Forward) >= 0.0f ? 1.0f : -1.0f;
	const float SpeedAlpha = FMath::Clamp(HorizontalSpeed / 5200.0f, 0.18f, 1.0f);
	const float TurnDegrees = Steering * ForwardSpeedSign * ArcadeSteeringTurnRateDegrees * SpeedAlpha * DeltaTime;

	const FVector TurnedHorizontalVelocity = HorizontalVelocity.RotateAngleAxis(TurnDegrees * ArcadeSteeringVelocityTurnBlend, FVector::UpVector);
	Primitive->SetPhysicsLinearVelocity(FVector(TurnedHorizontalVelocity.X, TurnedHorizontalVelocity.Y, Velocity.Z), false);

	const FVector AngularVelocity = Primitive->GetPhysicsAngularVelocityInRadians();
	const float TargetYawVelocity = FMath::DegreesToRadians(Steering * ForwardSpeedSign * ArcadeSteeringTurnRateDegrees * SpeedAlpha);
	const FVector RollPitchVelocity = AngularVelocity - FVector::UpVector * FVector::DotProduct(AngularVelocity, FVector::UpVector);
	Primitive->SetPhysicsAngularVelocityInRadians(RollPitchVelocity + FVector::UpVector * TargetYawVelocity, false);
}

void UUBArcadeVehicleAssistComponent::ApplySteeringAndDriftAssist(UPrimitiveComponent* Primitive, float DeltaTime, float Speed) const
{
	const AActor* OwnerActor = GetOwner();
	if (!Primitive || !OwnerActor)
	{
		return;
	}

	if (Speed < DriftMinSpeed || !IsDriftInputDown())
	{
		return;
	}

	const float Steering = GetSteeringInput();
	if (FMath::IsNearlyZero(Steering, 0.05f))
	{
		return;
	}

	const float SpeedAlpha = FMath::Clamp(Speed / 4200.0f, 0.0f, 1.0f);
	Primitive->AddTorqueInRadians(FVector::UpVector * Steering * DriftYawTorque, NAME_None, true);
	Primitive->AddForce(OwnerActor->GetActorRightVector() * Steering * Primitive->GetMass() * DriftSideAcceleration * SpeedAlpha, NAME_None, false);
}

void UUBArcadeVehicleAssistComponent::ApplyNormalGripAssist(UPrimitiveComponent* Primitive) const
{
	const AActor* OwnerActor = GetOwner();
	if (!Primitive || !OwnerActor || IsDriftInputDown())
	{
		return;
	}

	const FVector Velocity = Primitive->GetPhysicsLinearVelocity();
	const FVector Forward = OwnerActor->GetActorForwardVector();
	const FVector Right = OwnerActor->GetActorRightVector();
	const float ForwardSpeed = FVector::DotProduct(Velocity, Forward);
	const float SideSpeed = FVector::DotProduct(Velocity, Right);
	const FVector VerticalVelocity = FVector::UpVector * FVector::DotProduct(Velocity, FVector::UpVector);
	const FVector StabilizedVelocity = Forward * ForwardSpeed + Right * SideSpeed * NormalGripLateralDamping + VerticalVelocity;
	Primitive->SetPhysicsLinearVelocity(StabilizedVelocity, false);
}

void UUBArcadeVehicleAssistComponent::HandleOwnerComponentHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (!bAssistEnabled || !HitComponent || !GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	if (Hit.ImpactNormal.Z > 0.62f)
	{
		return;
	}

	FVector Velocity = HitComponent->GetPhysicsLinearVelocity();
	const float IntoWallSpeed = FVector::DotProduct(Velocity, -Hit.ImpactNormal);
	if (IntoWallSpeed > 250.0f)
	{
		Velocity += Hit.ImpactNormal * IntoWallSpeed * WallHitNormalDamping;
	}

	if (Velocity.Z > MaxNearGroundUpVelocity)
	{
		Velocity.Z = MaxNearGroundUpVelocity;
	}

	HitComponent->SetPhysicsLinearVelocity(Velocity, false);
	HitComponent->SetPhysicsAngularVelocityInRadians(HitComponent->GetPhysicsAngularVelocityInRadians() * 0.78f, false);
}
