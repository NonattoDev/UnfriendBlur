#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "UBPrototypeTargetCar.generated.h"

class UBoxComponent;
class UPointLightComponent;
class UStaticMeshComponent;
class UUBPowerInventoryComponent;
class UUBVehicleHealthComponent;
class UUBVehicleStatusComponent;

UCLASS()
class UNFRIENDBLUR_API AUBPrototypeTargetCar : public APawn
{
	GENERATED_BODY()

public:
	AUBPrototypeTargetCar();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable, Category = "UnfriendBlur|Prototype")
	void InitializePrototypeTarget(int32 InTargetIndex);

	void InitializePrototypeTarget(int32 InTargetIndex, const TArray<FVector>& InDrivingWaypoints);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UnfriendBlur|Prototype")
	TObjectPtr<UBoxComponent> Collision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UnfriendBlur|Prototype")
	TObjectPtr<UStaticMeshComponent> CarMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UnfriendBlur|Prototype")
	TObjectPtr<UPointLightComponent> MarkerLight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UnfriendBlur|Prototype")
	TObjectPtr<UUBPowerInventoryComponent> PowerInventory;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UnfriendBlur|Prototype")
	TObjectPtr<UUBVehicleHealthComponent> Health;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UnfriendBlur|Prototype")
	TObjectPtr<UUBVehicleStatusComponent> Status;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnfriendBlur|Prototype|Driving")
	float DesiredCruiseSpeed = 2150.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnfriendBlur|Prototype|Driving")
	float WaypointAcceptanceRadius = 620.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnfriendBlur|Prototype|Driving")
	float AccelerationResponse = 2.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnfriendBlur|Prototype|Driving")
	float TurnResponse = 4.6f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnfriendBlur|Prototype|Driving")
	float StuckRecoverySeconds = 1.6f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnfriendBlur|Prototype|Driving")
	float ImpactRecoveryHoldSeconds = 1.25f;

	UPROPERTY(Transient)
	TArray<FVector> DrivingWaypoints;

	int32 TargetIndex = 0;
	int32 CurrentWaypointIndex = 0;
	float LowSpeedSeconds = 0.0f;
	float ImpactRecoveryTimeRemaining = 0.0f;

	int32 FindClosestWaypointIndex() const;
	void AdvanceWaypointIfNeeded();
};
