#include "Vehicle/UBArcadeVehicleAssistComponent.h"

#include "ChaosVehicleMovementComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/FileManager.h"
#include "InputCoreTypes.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

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
	BindHitHandler();
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
	UChaosVehicleMovementComponent* VehicleMovement = CachedVehicleMovement.Get();
	if (!VehicleMovement)
	{
		VehicleMovement = FindVehicleMovement();
		CachedVehicleMovement = VehicleMovement;
	}

	ApplyChaosVehicleInput(VehicleMovement);

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

	if (bEnablePhysicsThrottleFallback)
	{
		ApplyThrottleBrakeAssist(Primitive, DeltaTime, bGrounded);
	}
	ApplyDownforce(Primitive, Speed, bGrounded);
	ApplyAntiFlipStability(Primitive, bGrounded);
	ApplyCurbLaunchClamp(Primitive, bGrounded, GroundDistance);
	if (bEnableArcadeSteeringAssist)
	{
		ApplyArcadeSteering(Primitive, DeltaTime, Speed, bGrounded);
	}
	if (bEnableDriftAssist)
	{
		ApplySteeringAndDriftAssist(Primitive, DeltaTime, Speed);
	}
	if (bEnableNormalGripAssist)
	{
		ApplyNormalGripAssist(Primitive, DeltaTime);
	}
	ApplyVelocitySafetyClamp(Primitive);
	RecordTelemetry(DeltaTime, Primitive, bGrounded, GroundDistance);

	bWasGrounded = bGrounded;
}

void UUBArcadeVehicleAssistComponent::SetAssistEnabled(bool bEnabled)
{
	bAssistEnabled = bEnabled;
	SetComponentTickEnabled(bAssistEnabled);

	if (bAssistEnabled)
	{
		CachedPrimitive = FindBestPrimitive();
		CachedVehicleMovement = FindVehicleMovement();
		BindHitHandler();
		if (CachedPrimitive)
		{
			CachedPrimitive->WakeAllRigidBodies();
		}
		if (CachedVehicleMovement)
		{
			CachedVehicleMovement->SetSleeping(false);
			CachedVehicleMovement->SetParked(false);
			CachedVehicleMovement->SetUseAutomaticGears(true);
		}

		InitializeTelemetry();
	}
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

void UUBArcadeVehicleAssistComponent::BindHitHandler()
{
	UPrimitiveComponent* Primitive = CachedPrimitive.Get();
	if (!Primitive)
	{
		return;
	}

	Primitive->SetNotifyRigidBodyCollision(true);
	Primitive->OnComponentHit.AddUniqueDynamic(this, &UUBArcadeVehicleAssistComponent::HandleOwnerComponentHit);
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

UChaosVehicleMovementComponent* UUBArcadeVehicleAssistComponent::FindVehicleMovement() const
{
	const AActor* OwnerActor = GetOwner();
	return OwnerActor ? OwnerActor->FindComponentByClass<UChaosVehicleMovementComponent>() : nullptr;
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
	Steering += PlayerController->GetInputAnalogKeyState(EKeys::Gamepad_LeftX);
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

float UUBArcadeVehicleAssistComponent::GetThrottleInput() const
{
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	const APlayerController* PlayerController = OwnerPawn ? Cast<APlayerController>(OwnerPawn->GetController()) : nullptr;
	if (!PlayerController || !PlayerController->IsLocalController())
	{
		return 0.0f;
	}

	float Throttle = PlayerController->GetInputAnalogKeyState(EKeys::Gamepad_RightTriggerAxis);
	if (PlayerController->IsInputKeyDown(EKeys::W) || PlayerController->IsInputKeyDown(EKeys::Up))
	{
		Throttle += 1.0f;
	}

	return FMath::Clamp(Throttle, 0.0f, 1.0f);
}

float UUBArcadeVehicleAssistComponent::GetBrakeInput() const
{
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	const APlayerController* PlayerController = OwnerPawn ? Cast<APlayerController>(OwnerPawn->GetController()) : nullptr;
	if (!PlayerController || !PlayerController->IsLocalController())
	{
		return 0.0f;
	}

	float Brake = PlayerController->GetInputAnalogKeyState(EKeys::Gamepad_LeftTriggerAxis);
	if (PlayerController->IsInputKeyDown(EKeys::S) || PlayerController->IsInputKeyDown(EKeys::Down))
	{
		Brake += 1.0f;
	}

	return FMath::Clamp(Brake, 0.0f, 1.0f);
}

bool UUBArcadeVehicleAssistComponent::IsDriftInputDown() const
{
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	const APlayerController* PlayerController = OwnerPawn ? Cast<APlayerController>(OwnerPawn->GetController()) : nullptr;
	return PlayerController
		&& PlayerController->IsLocalController()
		&& PlayerController->IsInputKeyDown(EKeys::SpaceBar);
}

void UUBArcadeVehicleAssistComponent::ApplyChaosVehicleInput(UChaosVehicleMovementComponent* VehicleMovement) const
{
	if (!VehicleMovement || !bDriveChaosVehicleInput)
	{
		return;
	}

	VehicleMovement->SetSleeping(false);
	VehicleMovement->SetParked(false);
	VehicleMovement->SetUseAutomaticGears(true);
	VehicleMovement->SetThrottleInput(GetThrottleInput());
	VehicleMovement->SetBrakeInput(GetBrakeInput());
	VehicleMovement->SetSteeringInput(GetSteeringInput());
	VehicleMovement->SetHandbrakeInput(IsDriftInputDown());
}

void UUBArcadeVehicleAssistComponent::ApplyThrottleBrakeAssist(UPrimitiveComponent* Primitive, float DeltaTime, bool bGrounded) const
{
	const AActor* OwnerActor = GetOwner();
	if (!Primitive || !OwnerActor || !bGrounded)
	{
		return;
	}

	const FVector Forward = OwnerActor->GetActorForwardVector().GetSafeNormal2D();
	const FVector Velocity = Primitive->GetPhysicsLinearVelocity();
	const float ForwardSpeed = FVector::DotProduct(Velocity, Forward);
	const float Throttle = GetThrottleInput();
	const float Brake = GetBrakeInput();

	if (Throttle > 0.01f && ForwardSpeed < AssistMaxForwardSpeed)
	{
		const float SpeedScale = FMath::Clamp(1.0f - ForwardSpeed / AssistMaxForwardSpeed, 0.18f, 1.0f);
		Primitive->AddForce(Forward * Primitive->GetMass() * ThrottleAssistAcceleration * Throttle * SpeedScale, NAME_None, false);
	}

	if (Brake > 0.01f && ForwardSpeed > 60.0f)
	{
		const float BrakeDelta = BrakeAssistAcceleration * Brake * DeltaTime;
		const float NewForwardSpeed = FMath::Max(0.0f, ForwardSpeed - BrakeDelta);
		const FVector Right = OwnerActor->GetActorRightVector().GetSafeNormal2D();
		const float SideSpeed = FVector::DotProduct(Velocity, Right);
		const FVector VerticalVelocity = FVector::UpVector * FVector::DotProduct(Velocity, FVector::UpVector);
		Primitive->SetPhysicsLinearVelocity(Forward * NewForwardSpeed + Right * SideSpeed * 0.82f + VerticalVelocity, false);
	}
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
	const float SpeedAlpha = FMath::Clamp(HorizontalSpeed / 3600.0f, 0.42f, 1.0f);
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

	const float SpeedAlpha = FMath::Clamp(Speed / 3800.0f, 0.35f, 1.0f);
	const FVector Velocity = Primitive->GetPhysicsLinearVelocity();
	const FVector HorizontalVelocity(Velocity.X, Velocity.Y, 0.0f);
	const float DriftTurnDegrees = Steering * DriftYawRateDegrees * SpeedAlpha * DeltaTime;
	const FVector DriftedHorizontalVelocity = HorizontalVelocity.RotateAngleAxis(DriftTurnDegrees * DriftVelocityTurnBlend, FVector::UpVector);

	Primitive->SetPhysicsLinearVelocity(FVector(DriftedHorizontalVelocity.X, DriftedHorizontalVelocity.Y, Velocity.Z), false);
	const FVector AngularVelocity = Primitive->GetPhysicsAngularVelocityInRadians();
	const FVector RollPitchVelocity = AngularVelocity - FVector::UpVector * FVector::DotProduct(AngularVelocity, FVector::UpVector);
	const float TargetYawVelocity = FMath::DegreesToRadians(Steering * DriftYawRateDegrees * SpeedAlpha);
	Primitive->SetPhysicsAngularVelocityInRadians(RollPitchVelocity + FVector::UpVector * TargetYawVelocity, false);
	Primitive->AddTorqueInRadians(FVector::UpVector * Steering * DriftYawTorque, NAME_None, true);
	Primitive->AddForce(OwnerActor->GetActorRightVector() * Steering * Primitive->GetMass() * DriftSideAcceleration * SpeedAlpha, NAME_None, false);
}

void UUBArcadeVehicleAssistComponent::ApplyNormalGripAssist(UPrimitiveComponent* Primitive, float DeltaTime) const
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
	const float FrameDamping = FMath::Pow(NormalGripLateralDamping, FMath::Max(DeltaTime, 0.0f) * 60.0f);
	const FVector StabilizedVelocity = Forward * ForwardSpeed + Right * SideSpeed * FrameDamping + VerticalVelocity;
	Primitive->SetPhysicsLinearVelocity(StabilizedVelocity, false);
}

void UUBArcadeVehicleAssistComponent::ApplyVelocitySafetyClamp(UPrimitiveComponent* Primitive) const
{
	const AActor* OwnerActor = GetOwner();
	if (!Primitive || !OwnerActor)
	{
		return;
	}

	FVector Velocity = Primitive->GetPhysicsLinearVelocity();
	FVector HorizontalVelocity(Velocity.X, Velocity.Y, 0.0f);
	const float HorizontalSpeed = HorizontalVelocity.Size();
	if (HorizontalSpeed > MaxPlanarSpeed)
	{
		HorizontalVelocity = HorizontalVelocity.GetSafeNormal() * MaxPlanarSpeed;
	}

	const FVector Forward = OwnerActor->GetActorForwardVector().GetSafeNormal2D();
	const FVector Right = OwnerActor->GetActorRightVector().GetSafeNormal2D();
	const float ForwardSpeed = FVector::DotProduct(HorizontalVelocity, Forward);
	const float SideSpeed = FVector::DotProduct(HorizontalVelocity, Right);
	const float SideSpeedLimit = IsDriftInputDown() ? MaxDriftSideSpeed : MaxNormalSideSpeed;
	const float ClampedSideSpeed = FMath::Clamp(SideSpeed, -SideSpeedLimit, SideSpeedLimit);
	const float ClampedUpSpeed = FMath::Clamp(Velocity.Z, -MaxDownVelocity, MaxNearGroundUpVelocity);

	if (!FMath::IsNearlyEqual(ClampedSideSpeed, SideSpeed, 0.1f) || !FMath::IsNearlyEqual(ClampedUpSpeed, Velocity.Z, 0.1f) || HorizontalSpeed > MaxPlanarSpeed)
	{
		Primitive->SetPhysicsLinearVelocity(Forward * ForwardSpeed + Right * ClampedSideSpeed + FVector::UpVector * ClampedUpSpeed, false);
	}
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

bool UUBArcadeVehicleAssistComponent::ShouldRecordTelemetry() const
{
	if (!bTelemetryEnabled)
	{
		return false;
	}

	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	const APlayerController* PlayerController = OwnerPawn ? Cast<APlayerController>(OwnerPawn->GetController()) : nullptr;
	return PlayerController && PlayerController->IsLocalController();
}

void UUBArcadeVehicleAssistComponent::InitializeTelemetry()
{
	if (bTelemetryInitialized || !ShouldRecordTelemetry())
	{
		return;
	}

	TelemetryFilePath = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("HandlingTelemetry.csv"));
	const FString Header = TEXT("time,actor,speed_kmh,forward_speed_kmh,side_speed_kmh,steering,throttle,brake,drift,grounded,ground_distance,slip_angle_deg,yaw_rate_deg_s,roll_deg,pitch_deg,up_velocity\n");
	FFileHelper::SaveStringToFile(Header, *TelemetryFilePath, FFileHelper::EEncodingOptions::AutoDetect, &IFileManager::Get());
	bTelemetryInitialized = true;
	UE_LOG(LogTemp, Log, TEXT("[UnfriendBlur Vehicle] Handling telemetry recording to %s"), *TelemetryFilePath);
}

void UUBArcadeVehicleAssistComponent::RecordTelemetry(float DeltaTime, UPrimitiveComponent* Primitive, bool bGrounded, float GroundDistance)
{
	if (!Primitive || !ShouldRecordTelemetry())
	{
		return;
	}

	InitializeTelemetry();
	if (TelemetryFilePath.IsEmpty())
	{
		return;
	}

	TelemetryAccumulator += DeltaTime;
	if (TelemetryAccumulator < FMath::Max(0.02f, TelemetrySampleInterval))
	{
		return;
	}
	TelemetryAccumulator = 0.0f;

	const AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
	{
		return;
	}

	const FVector Velocity = Primitive->GetPhysicsLinearVelocity();
	const FVector HorizontalVelocity(Velocity.X, Velocity.Y, 0.0f);
	const FVector Forward = OwnerActor->GetActorForwardVector().GetSafeNormal2D();
	const FVector Right = OwnerActor->GetActorRightVector().GetSafeNormal2D();
	const float ForwardSpeed = FVector::DotProduct(Velocity, Forward);
	const float SideSpeed = FVector::DotProduct(Velocity, Right);
	const float SpeedKmh = HorizontalVelocity.Size() * 0.036f;
	const float ForwardSpeedKmh = ForwardSpeed * 0.036f;
	const float SideSpeedKmh = SideSpeed * 0.036f;
	const float SlipAngleDegrees = HorizontalVelocity.SizeSquared2D() > FMath::Square(20.0f)
		? FMath::RadiansToDegrees(FMath::Atan2(SideSpeed, FMath::Abs(ForwardSpeed)))
		: 0.0f;
	const FRotator Rotation = OwnerActor->GetActorRotation();
	const float TimeSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	float ActorYawRateDegrees = 0.0f;
	if (bHasTelemetryYawSample)
	{
		const float TimeDelta = FMath::Max(TimeSeconds - LastTelemetryTimeSeconds, KINDA_SMALL_NUMBER);
		ActorYawRateDegrees = FMath::FindDeltaAngleDegrees(LastTelemetryYawDegrees, Rotation.Yaw) / TimeDelta;
	}
	LastTelemetryTimeSeconds = TimeSeconds;
	LastTelemetryYawDegrees = Rotation.Yaw;
	bHasTelemetryYawSample = true;

	const FString Row = FString::Printf(
		TEXT("%.3f,%s,%.2f,%.2f,%.2f,%.3f,%.3f,%.3f,%d,%d,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f\n"),
		TimeSeconds,
		*OwnerActor->GetName(),
		SpeedKmh,
		ForwardSpeedKmh,
		SideSpeedKmh,
		GetSteeringInput(),
		GetThrottleInput(),
		GetBrakeInput(),
		IsDriftInputDown() ? 1 : 0,
		bGrounded ? 1 : 0,
		GroundDistance,
		SlipAngleDegrees,
		ActorYawRateDegrees,
		Rotation.Roll,
		Rotation.Pitch,
		Velocity.Z);

	FFileHelper::SaveStringToFile(Row, *TelemetryFilePath, FFileHelper::EEncodingOptions::AutoDetect, &IFileManager::Get(), EFileWrite::FILEWRITE_Append);
}
