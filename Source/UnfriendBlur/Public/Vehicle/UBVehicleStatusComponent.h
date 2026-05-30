#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Powers/UBPowerTypes.h"
#include "UBVehicleStatusComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FUBVehicleSlowChangedSignature, bool, bIsSlowed, float, SlowStrength, float, RemainingSeconds);

UCLASS(ClassGroup = (UnfriendBlur), meta = (BlueprintSpawnableComponent))
class UNFRIENDBLUR_API UUBVehicleStatusComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UUBVehicleStatusComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnfriendBlur|Vehicle Status")
	float InstantSlowVelocityLossScale = 0.55f;

	UPROPERTY(BlueprintAssignable, Category = "UnfriendBlur|Vehicle Status")
	FUBVehicleSlowChangedSignature OnSlowChanged;

	UFUNCTION(BlueprintCallable, Category = "UnfriendBlur|Vehicle Status")
	void ApplySlow(float Strength, float DurationSeconds, AActor* SourceActor, EUBPowerType SourcePower);

	UFUNCTION(BlueprintCallable, Category = "UnfriendBlur|Vehicle Status")
	void ClearNegativeStatus();

	UFUNCTION(BlueprintPure, Category = "UnfriendBlur|Vehicle Status")
	bool IsSlowed() const { return bSlowed; }

	UFUNCTION(BlueprintPure, Category = "UnfriendBlur|Vehicle Status")
	float GetSlowStrength() const { return SlowStrength; }

	UFUNCTION(BlueprintPure, Category = "UnfriendBlur|Vehicle Status")
	float GetSlowTimeRemaining() const { return SlowTimeRemaining; }

	UFUNCTION(BlueprintPure, Category = "UnfriendBlur|Vehicle Status")
	float GetSpeedScale() const { return bSlowed ? FMath::Clamp(1.0f - SlowStrength, 0.1f, 1.0f) : 1.0f; }

	UFUNCTION(BlueprintCallable, Category = "UnfriendBlur|Vehicle Status", meta = (DefaultToSelf = "OwnerActor"))
	static UUBVehicleStatusComponent* FindOrCreateStatusComponent(AActor* OwnerActor);

protected:
	UPROPERTY(ReplicatedUsing = OnRep_Slow)
	bool bSlowed = false;

	UPROPERTY(ReplicatedUsing = OnRep_Slow)
	float SlowStrength = 0.0f;

	UPROPERTY(ReplicatedUsing = OnRep_Slow)
	float SlowTimeRemaining = 0.0f;

	UPROPERTY(Replicated)
	EUBPowerType LastSlowPower = EUBPowerType::None;

	UFUNCTION()
	void OnRep_Slow();

	void ApplyInstantVelocityPenalty(float Strength);
	void SetSlowState(bool bNewSlowed, float NewStrength, float NewTimeRemaining);
};
