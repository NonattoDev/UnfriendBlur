#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UBArcadeVehicleAssistComponent.generated.h"

class UPrimitiveComponent;
class UChaosVehicleMovementComponent;

UCLASS(ClassGroup = (UnfriendBlur), meta = (BlueprintSpawnableComponent))
class UNFRIENDBLUR_API UUBArcadeVehicleAssistComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UUBArcadeVehicleAssistComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnfriendBlur|Vehicle Assist")
	bool bAssistEnabled = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnfriendBlur|Vehicle Assist|Input")
	bool bDriveChaosVehicleInput = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnfriendBlur|Vehicle Assist|Input")
	bool bEnablePhysicsThrottleFallback = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnfriendBlur|Vehicle Assist|Steering")
	bool bEnableArcadeSteeringAssist = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnfriendBlur|Vehicle Assist|Drift")
	bool bEnableDriftAssist = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnfriendBlur|Vehicle Assist|Grip")
	bool bEnableNormalGripAssist = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnfriendBlur|Vehicle Assist|Telemetry")
	bool bTelemetryEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnfriendBlur|Vehicle Assist|Telemetry")
	float TelemetrySampleInterval = 0.10f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnfriendBlur|Vehicle Assist|Throttle")
	float ThrottleAssistAcceleration = 1750.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnfriendBlur|Vehicle Assist|Throttle")
	float BrakeAssistAcceleration = 5200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnfriendBlur|Vehicle Assist|Throttle")
	float AssistMaxForwardSpeed = 7600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnfriendBlur|Vehicle Assist|Downforce")
	float BaseDownforceAcceleration = 420.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnfriendBlur|Vehicle Assist|Downforce")
	float SpeedDownforceCoefficient = 0.00007f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnfriendBlur|Vehicle Assist|Downforce")
	float MaxDownforceAcceleration = 2600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnfriendBlur|Vehicle Assist|Downforce")
	float AirDownforceAcceleration = 1800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnfriendBlur|Vehicle Assist|Stability")
	float RollPitchAngularDamping = 0.92f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnfriendBlur|Vehicle Assist|Stability")
	float AirRollPitchAngularDamping = 0.88f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnfriendBlur|Vehicle Assist|Stability")
	float UprightTorqueStrength = 3200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnfriendBlur|Vehicle Assist|Stability")
	float StrongUprightTorqueStrength = 9800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnfriendBlur|Vehicle Assist|Curb")
	float GroundProbeDistance = 340.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnfriendBlur|Vehicle Assist|Curb")
	float MaxNearGroundUpVelocity = 360.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnfriendBlur|Vehicle Assist|Curb")
	float WallHitNormalDamping = 0.72f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnfriendBlur|Vehicle Assist|Steering")
	float ArcadeSteeringMinSpeed = 60.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnfriendBlur|Vehicle Assist|Steering")
	float ArcadeSteeringTurnRateDegrees = 136.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnfriendBlur|Vehicle Assist|Steering")
	float ArcadeSteeringVelocityTurnBlend = 0.74f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnfriendBlur|Vehicle Assist|Drift")
	float DriftMinSpeed = 850.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnfriendBlur|Vehicle Assist|Drift")
	float DriftYawRateDegrees = 132.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnfriendBlur|Vehicle Assist|Drift")
	float DriftYawTorque = 4200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnfriendBlur|Vehicle Assist|Drift")
	float DriftSideAcceleration = 240.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnfriendBlur|Vehicle Assist|Drift")
	float DriftVelocityTurnBlend = 0.28f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnfriendBlur|Vehicle Assist|Grip")
	float NormalGripLateralDamping = 0.956f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnfriendBlur|Vehicle Assist|Safety")
	float MaxPlanarSpeed = 9000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnfriendBlur|Vehicle Assist|Safety")
	float MaxNormalSideSpeed = 2200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnfriendBlur|Vehicle Assist|Safety")
	float MaxDriftSideSpeed = 3800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnfriendBlur|Vehicle Assist|Safety")
	float MaxDownVelocity = 3600.0f;

	UFUNCTION(BlueprintCallable, Category = "UnfriendBlur|Vehicle Assist")
	void SetAssistEnabled(bool bEnabled);

	UFUNCTION(BlueprintCallable, Category = "UnfriendBlur|Vehicle Assist", meta = (DefaultToSelf = "OwnerActor"))
	static UUBArcadeVehicleAssistComponent* FindOrCreateAssistComponent(AActor* OwnerActor);

private:
	UPROPERTY(Transient)
	TObjectPtr<UPrimitiveComponent> CachedPrimitive;

	UPROPERTY(Transient)
	TObjectPtr<UChaosVehicleMovementComponent> CachedVehicleMovement;

	bool bWasGrounded = false;
	bool bTelemetryInitialized = false;
	bool bHasTelemetryYawSample = false;
	float TelemetryAccumulator = 0.0f;
	float LastTelemetryTimeSeconds = 0.0f;
	float LastTelemetryYawDegrees = 0.0f;
	FString TelemetryFilePath;

	UPrimitiveComponent* FindBestPrimitive() const;
	UChaosVehicleMovementComponent* FindVehicleMovement() const;
	bool IsGrounded(float& OutGroundDistance) const;
	float GetSteeringInput() const;
	float GetThrottleInput() const;
	float GetBrakeInput() const;
	bool IsDriftInputDown() const;
	void BindHitHandler();
	void ApplyChaosVehicleInput(UChaosVehicleMovementComponent* VehicleMovement) const;
	void ApplyThrottleBrakeAssist(UPrimitiveComponent* Primitive, float DeltaTime, bool bGrounded) const;
	void ApplyDownforce(UPrimitiveComponent* Primitive, float Speed, bool bGrounded) const;
	void ApplyAntiFlipStability(UPrimitiveComponent* Primitive, bool bGrounded) const;
	void ApplyCurbLaunchClamp(UPrimitiveComponent* Primitive, bool bGrounded, float GroundDistance) const;
	void ApplyArcadeSteering(UPrimitiveComponent* Primitive, float DeltaTime, float Speed, bool bGrounded) const;
	void ApplySteeringAndDriftAssist(UPrimitiveComponent* Primitive, float DeltaTime, float Speed) const;
	void ApplyNormalGripAssist(UPrimitiveComponent* Primitive, float DeltaTime) const;
	void ApplyVelocitySafetyClamp(UPrimitiveComponent* Primitive) const;
	bool ShouldRecordTelemetry() const;
	void InitializeTelemetry();
	void RecordTelemetry(float DeltaTime, UPrimitiveComponent* Primitive, bool bGrounded, float GroundDistance);

	UFUNCTION()
	void HandleOwnerComponentHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
};
