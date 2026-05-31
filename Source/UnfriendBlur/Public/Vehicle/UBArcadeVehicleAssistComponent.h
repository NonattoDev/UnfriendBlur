#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UBArcadeVehicleAssistComponent.generated.h"

class UPrimitiveComponent;

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnfriendBlur|Vehicle Assist|Throttle")
	float ThrottleAssistAcceleration = 1800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnfriendBlur|Vehicle Assist|Throttle")
	float BrakeAssistAcceleration = 6200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnfriendBlur|Vehicle Assist|Throttle")
	float AssistMaxForwardSpeed = 7200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnfriendBlur|Vehicle Assist|Downforce")
	float BaseDownforceAcceleration = 720.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnfriendBlur|Vehicle Assist|Downforce")
	float SpeedDownforceCoefficient = 0.00018f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnfriendBlur|Vehicle Assist|Downforce")
	float MaxDownforceAcceleration = 4800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnfriendBlur|Vehicle Assist|Downforce")
	float AirDownforceAcceleration = 6800.0f;

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
	float ArcadeSteeringTurnRateDegrees = 152.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnfriendBlur|Vehicle Assist|Steering")
	float ArcadeSteeringVelocityTurnBlend = 0.74f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnfriendBlur|Vehicle Assist|Drift")
	float DriftMinSpeed = 850.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnfriendBlur|Vehicle Assist|Drift")
	float DriftYawRateDegrees = 168.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnfriendBlur|Vehicle Assist|Drift")
	float DriftYawTorque = 9200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnfriendBlur|Vehicle Assist|Drift")
	float DriftSideAcceleration = 620.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnfriendBlur|Vehicle Assist|Drift")
	float DriftVelocityTurnBlend = 0.28f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnfriendBlur|Vehicle Assist|Grip")
	float NormalGripLateralDamping = 0.935f;

	UFUNCTION(BlueprintCallable, Category = "UnfriendBlur|Vehicle Assist")
	void SetAssistEnabled(bool bEnabled);

	UFUNCTION(BlueprintCallable, Category = "UnfriendBlur|Vehicle Assist", meta = (DefaultToSelf = "OwnerActor"))
	static UUBArcadeVehicleAssistComponent* FindOrCreateAssistComponent(AActor* OwnerActor);

private:
	UPROPERTY(Transient)
	TObjectPtr<UPrimitiveComponent> CachedPrimitive;

	bool bWasGrounded = false;

	UPrimitiveComponent* FindBestPrimitive() const;
	bool IsGrounded(float& OutGroundDistance) const;
	float GetSteeringInput() const;
	float GetThrottleInput() const;
	float GetBrakeInput() const;
	bool IsDriftInputDown() const;
	void BindHitHandler();
	void ApplyThrottleBrakeAssist(UPrimitiveComponent* Primitive, float DeltaTime, bool bGrounded) const;
	void ApplyDownforce(UPrimitiveComponent* Primitive, float Speed, bool bGrounded) const;
	void ApplyAntiFlipStability(UPrimitiveComponent* Primitive, bool bGrounded) const;
	void ApplyCurbLaunchClamp(UPrimitiveComponent* Primitive, bool bGrounded, float GroundDistance) const;
	void ApplyArcadeSteering(UPrimitiveComponent* Primitive, float DeltaTime, float Speed, bool bGrounded) const;
	void ApplySteeringAndDriftAssist(UPrimitiveComponent* Primitive, float DeltaTime, float Speed) const;
	void ApplyNormalGripAssist(UPrimitiveComponent* Primitive, float DeltaTime) const;

	UFUNCTION()
	void HandleOwnerComponentHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
};
