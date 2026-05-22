#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Powers/UBPowerTypes.h"
#include "UBPowerPickup.generated.h"

class USphereComponent;
class UBillboardComponent;
class UPointLightComponent;
class UStaticMeshComponent;

UCLASS()
class UNFRIENDBLUR_API AUBPowerPickup : public AActor
{
	GENERATED_BODY()

public:
	AUBPowerPickup();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UnfriendBlur|Powers")
	TObjectPtr<USphereComponent> Collision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UnfriendBlur|Powers")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UnfriendBlur|Powers")
	TObjectPtr<UBillboardComponent> IconBillboard;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UnfriendBlur|Powers")
	TObjectPtr<UPointLightComponent> PointLight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, ReplicatedUsing = OnRep_PickupVisualState, Category = "UnfriendBlur|Powers")
	EUBPowerType FixedPower = EUBPowerType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, ReplicatedUsing = OnRep_PickupVisualState, Category = "UnfriendBlur|Powers")
	bool bIsSuperPickup = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnfriendBlur|Powers")
	TArray<EUBPowerType> AvailablePowers;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnfriendBlur|Powers")
	float RespawnSeconds = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "UnfriendBlur|Powers")
	bool bRespawns = true;

	UFUNCTION(BlueprintCallable, Category = "UnfriendBlur|Powers")
	void RefreshVisuals();

protected:
	bool bAvailable = true;
	FTimerHandle RespawnTimerHandle;

	UFUNCTION()
	void HandleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnRep_PickupVisualState();

	EUBPowerType ChoosePower() const;
	EUBPowerType GetVisualPower() const;
	void SetAvailable(bool bNewAvailable);
	void Respawn();
};
