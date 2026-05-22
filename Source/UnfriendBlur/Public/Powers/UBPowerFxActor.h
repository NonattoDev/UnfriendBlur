#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Powers/UBPowerTypes.h"
#include "UBPowerFxActor.generated.h"

class UPointLightComponent;
class USceneComponent;
class UStaticMeshComponent;

UCLASS()
class UNFRIENDBLUR_API AUBPowerFxActor : public AActor
{
	GENERATED_BODY()

public:
	AUBPowerFxActor();

	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable, Category = "UnfriendBlur|Powers|FX")
	void InitializePowerFx(EUBPowerType InPowerType, FVector InDirection, float InLifeSeconds, float InVisualScale, bool bInIsSuper = false);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UnfriendBlur|Powers|FX")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UnfriendBlur|Powers|FX")
	TObjectPtr<UStaticMeshComponent> CoreMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UnfriendBlur|Powers|FX")
	TObjectPtr<UStaticMeshComponent> AxisMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UnfriendBlur|Powers|FX")
	TObjectPtr<UPointLightComponent> PointLight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UnfriendBlur|Powers|FX")
	TArray<TObjectPtr<UStaticMeshComponent>> OrbitMeshes;

	UPROPERTY(ReplicatedUsing = OnRep_VisualState)
	EUBPowerType PowerType = EUBPowerType::Boost;

	UPROPERTY(ReplicatedUsing = OnRep_VisualState)
	FVector Direction = FVector::ForwardVector;

	UPROPERTY(ReplicatedUsing = OnRep_VisualState)
	float LifeSeconds = 1.0f;

	UPROPERTY(ReplicatedUsing = OnRep_VisualState)
	float VisualScale = 1.0f;

	UPROPERTY(ReplicatedUsing = OnRep_VisualState)
	bool bIsSuper = false;

	float AgeSeconds = 0.0f;

	UFUNCTION()
	void OnRep_VisualState();

	void ApplyVisualState();
	void UpdateOrbitMeshes(float Alpha);
};
