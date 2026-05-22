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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnfriendBlur|Vehicle Assist|Downforce")
	float BaseDownforceAcceleration = 260.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnfriendBlur|Vehicle Assist|Downforce")
	float SpeedDownforceCoefficient = 0.00008f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnfriendBlur|Vehicle Assist|Downforce")
	float MaxDownforceAcceleration = 2400.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnfriendBlur|Vehicle Assist|Downforce")
	float AirDownforceAcceleration = 2800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnfriendBlur|Vehicle Assist|Stability")
	float RollPitchAngularDamping = 0.92f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnfriendBlur|Vehicle Assist|Stability")
	float AirRollPitchAngularDamping = 0.88f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnfriendBlur|Vehicle Assist|Stability")
	float UprightTorqueStrength = 3200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnfriendBlur|Vehicle Assist|Stability")
	float StrongUprightTorqueStrength = 9800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnfriendBlur|Vehicle Assist|Curb")
	float GroundProbeDistance = 245.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnfriendBlur|Vehicle Assist|Curb")
	float MaxNearGroundUpVelocity = 720.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnfriendBlur|Vehicle Assist|Curb")
	float WallHitNormalDamping = 0.45f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnfriendBlur|Vehicle Assist|Steering")
	float ArcadeSteeringMinSpeed = 250.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnfriendBlur|Vehicle Assist|Steering")
	float ArcadeSteeringTurnRateDegrees = 82.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnfriendBlur|Vehicle Assist|Steering")
	float ArcadeSteeringVelocityTurnBlend = 0.82f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnfriendBlur|Vehicle Assist|Drift")
	float DriftMinSpeed = 1450.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnfriendBlur|Vehicle Assist|Drift")
	float DriftYawTorque = 3600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnfriendBlur|Vehicle Assist|Drift")
	float DriftSideAcceleration = 140.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnfriendBlur|Vehicle Assist|Grip")
	float NormalGripLateralDamping = 0.985f;

	UFUNCTION(BlueprintCallable, Category = "UnfriendBlur|Vehicle Assist", meta = (DefaultToSelf = "OwnerActor"))
	static UUBArcadeVehicleAssistComponent* FindOrCreateAssistComponent(AActor* OwnerActor);

private:
	UPROPERTY(Transient)
	TObjectPtr<UPrimitiveComponent> CachedPrimitive;

	bool bWasGrounded = false;

	UPrimitiveComponent* FindBestPrimitive() const;
	bool IsGrounded(float& OutGroundDistance) const;
	float GetSteeringInput() const;
	bool IsDriftInputDown() const;
	void ApplyDownforce(UPrimitiveComponent* Primitive, float Speed, bool bGrounded) const;
	void ApplyAntiFlipStability(UPrimitiveComponent* Primitive, bool bGrounded) const;
	void ApplyCurbLaunchClamp(UPrimitiveComponent* Primitive, bool bGrounded, float GroundDistance) const;
	void ApplyArcadeSteering(UPrimitiveComponent* Primitive, float DeltaTime, float Speed, bool bGrounded) const;
	void ApplySteeringAndDriftAssist(UPrimitiveComponent* Primitive, float DeltaTime, float Speed) const;
	void ApplyNormalGripAssist(UPrimitiveComponent* Primitive) const;

	UFUNCTION()
	void HandleOwnerComponentHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
};
