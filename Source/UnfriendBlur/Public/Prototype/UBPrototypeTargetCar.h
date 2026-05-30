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

	UFUNCTION(BlueprintCallable, Category = "UnfriendBlur|Prototype")
	void InitializePrototypeTarget(int32 InTargetIndex);

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
};
