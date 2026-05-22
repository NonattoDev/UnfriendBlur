#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UBPowerMine.generated.h"

class USphereComponent;
class UPointLightComponent;
class UStaticMeshComponent;

UCLASS()
class UNFRIENDBLUR_API AUBPowerMine : public AActor
{
	GENERATED_BODY()

public:
	AUBPowerMine();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UnfriendBlur|Powers")
	TObjectPtr<USphereComponent> Collision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UnfriendBlur|Powers")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UnfriendBlur|Powers")
	TObjectPtr<UPointLightComponent> PointLight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnfriendBlur|Powers")
	float ArmedDelay = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnfriendBlur|Powers")
	float HitDeltaVelocity = 3400.0f;

	UFUNCTION(BlueprintCallable, Category = "UnfriendBlur|Powers")
	void InitializeMine(AActor* InSourceActor);

	UFUNCTION(BlueprintPure, Category = "UnfriendBlur|Powers")
	AActor* GetSourceActor() const { return SourceActor; }

protected:
	UPROPERTY(Replicated)
	TObjectPtr<AActor> SourceActor;

	bool bArmed = false;

	FTimerHandle ArmedTimerHandle;

	void ArmMine();
	void SpawnExplosionFx();

	UFUNCTION()
	void HandleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
};
