#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "UBPowerPrototypeSubsystem.generated.h"

enum class EUBPowerType : uint8;
class APawn;
class APlayerController;
class AUBPrototypeTargetCar;
class AStaticMeshActor;
class UMaterialInterface;
class UStaticMesh;
class UStaticMeshComponent;
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
	bool bSpawnedPrototypeTrack = false;
	bool bSpawnedPrototypePickups = false;
	bool bSpawnedPrototypeTargets = false;
	bool bLoggedVehicleDiagnostics = false;
	bool bHasPrototypeTrackStart = false;
	FVector PrototypeTrackStartLocation = FVector::ZeroVector;
	FRotator PrototypeTrackStartRotation = FRotator::ZeroRotator;
	TArray<FVector> PrototypePickupLocations;
	TArray<FTransform> PrototypeTargetTransforms;

	void TryInventoryHotkeys(APlayerController* PlayerController) const;
	void DisplayInventory(APlayerController* PlayerController) const;
	void DisplayVehicleDiagnostics(APlayerController* PlayerController);
	bool IsImportedDriftTrackWorld() const;
	void SetupImportedDriftTrack(APawn* AnchorPawn);
	void SpawnPrototypeTrack(APawn* AnchorPawn);
	AStaticMeshActor* SpawnTrackBlock(UStaticMesh* Mesh, UMaterialInterface* BaseMaterial, const FVector& Location, const FRotator& Rotation, const FVector& Scale, const FLinearColor& Color, bool bCollisionEnabled, const FString& ActorLabel) const;
	void ApplyTrackMaterial(UStaticMeshComponent* MeshComponent, UMaterialInterface* BaseMaterial, const FLinearColor& Color, float EmissiveStrength) const;
	FVector TransformTrackPoint(const APawn* AnchorPawn, const FVector2D& LocalPoint, float RoadTopZ) const;
	void AddTrackGameplayPoints(const FVector& SegmentStart, const FVector& SegmentEnd, const FVector& Right, float RoadTopZ, int32 SegmentIndex);
	void MovePawnToPrototypeTrackStart(APawn* AnchorPawn) const;
	void SpawnPrototypePickups(APawn* AnchorPawn);
	FVector FindPickupLocation(APawn* AnchorPawn, int32 PickupIndex) const;
	void SpawnPrototypeTargets(APawn* AnchorPawn);
	FTransform FindTargetCarTransform(APawn* AnchorPawn, int32 TargetIndex) const;
	EUBPowerType PickRandomPower() const;
	void PrintInstructions();
};
