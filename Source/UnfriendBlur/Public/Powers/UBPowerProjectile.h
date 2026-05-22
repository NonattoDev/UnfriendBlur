#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Powers/UBPowerTypes.h"
#include "UBPowerProjectile.generated.h"

class UProjectileMovementComponent;
class UPointLightComponent;
class USphereComponent;
class UStaticMeshComponent;

UCLASS()
class UNFRIENDBLUR_API AUBPowerProjectile : public AActor
{
	GENERATED_BODY()

public:
	AUBPowerProjectile();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UnfriendBlur|Powers")
	TObjectPtr<USphereComponent> Collision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UnfriendBlur|Powers")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UnfriendBlur|Powers")
	TObjectPtr<UStaticMeshComponent> TrailMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UnfriendBlur|Powers")
	TObjectPtr<UPointLightComponent> PointLight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UnfriendBlur|Powers")
	TObjectPtr<UProjectileMovementComponent> Movement;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnfriendBlur|Powers")
	float BoltSpeed = 6500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnfriendBlur|Powers")
	float ShuntSpeed = 5200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnfriendBlur|Powers")
	float HitDeltaVelocity = 2800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnfriendBlur|Powers")
	float HomingRange = 9000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnfriendBlur|Powers")
	float HomingAcceleration = 14000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnfriendBlur|Powers|Counters")
	int32 ShuntCounterHealth = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnfriendBlur|Powers|Counters")
	float WeakenedShuntHitDeltaVelocity = 1800.0f;

	UFUNCTION(BlueprintCallable, Category = "UnfriendBlur|Powers")
	void InitializeProjectile(EUBPowerType InPowerType, AActor* InSourceActor, bool bInHoming, bool bInFireForward);

	UFUNCTION(BlueprintPure, Category = "UnfriendBlur|Powers")
	bool IsShuntProjectile() const;

	UFUNCTION(BlueprintPure, Category = "UnfriendBlur|Powers")
	bool IsWeakenedShunt() const { return bWeakenedShunt; }

	UFUNCTION(BlueprintPure, Category = "UnfriendBlur|Powers")
	AActor* GetSourceActor() const { return SourceActor; }

	bool ApplyProjectileCounterHit(EUBPowerType IncomingPowerType, AActor* IncomingSourceActor);
	bool DestroyByPowerCounter(EUBPowerType CounterPowerType, AActor* CounterSourceActor, bool bSpawnAreaPulse);

protected:
	UPROPERTY(ReplicatedUsing = OnRep_VisualStyle)
	EUBPowerType PowerType = EUBPowerType::Bolt;

	UPROPERTY(Replicated)
	TObjectPtr<AActor> SourceActor;

	UPROPERTY(ReplicatedUsing = OnRep_VisualStyle)
	bool bHoming = false;

	UPROPERTY(ReplicatedUsing = OnRep_VisualStyle)
	bool bFireForward = true;

	UPROPERTY(ReplicatedUsing = OnRep_VisualStyle)
	int32 CurrentCounterHealth = 0;

	UPROPERTY(ReplicatedUsing = OnRep_VisualStyle)
	bool bWeakenedShunt = false;

	UFUNCTION()
	void OnRep_VisualStyle();

	UFUNCTION()
	void HandleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	void AcquireHomingTarget();
	void ApplyVisualStyle();
	void SpawnImpactFx();
	void SpawnCounterFx(EUBPowerType CounterPowerType, bool bDestroyed);
	void ApplyDestructionPulse(EUBPowerType CounterPowerType, AActor* CounterSourceActor);
	float GetEffectiveHitDeltaVelocity() const;
};
