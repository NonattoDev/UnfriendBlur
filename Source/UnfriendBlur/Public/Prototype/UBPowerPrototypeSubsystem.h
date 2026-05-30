#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "UBPowerPrototypeSubsystem.generated.h"

enum class EUBPowerType : uint8;
class APawn;
class APlayerController;
class AUBPrototypeTargetCar;
struct FKey;

UCLASS()
class UNFRIENDBLUR_API UUBPowerPrototypeSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

private:
	bool bPrintedInstructions = false;
	bool bSpawnedPrototypePickups = false;
	bool bSpawnedPrototypeTargets = false;
	bool bLoggedVehicleDiagnostics = false;

	void TryInventoryHotkeys(APlayerController* PlayerController) const;
	void DisplayInventory(APlayerController* PlayerController) const;
	void DisplayVehicleDiagnostics(APlayerController* PlayerController);
	void SpawnPrototypePickups(APawn* AnchorPawn);
	FVector FindPickupLocation(APawn* AnchorPawn, int32 PickupIndex) const;
	void SpawnPrototypeTargets(APawn* AnchorPawn);
	FVector FindTargetCarLocation(APawn* AnchorPawn, int32 TargetIndex) const;
	EUBPowerType PickRandomPower() const;
	void PrintInstructions();
};
