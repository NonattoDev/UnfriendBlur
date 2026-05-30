#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Powers/UBPowerTypes.h"
#include "UBVehicleHealthComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FUBVehicleHealthChangedSignature, float, CurrentHealth, float, MaxHealth, AActor*, SourceActor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FUBVehicleDamagedSignature, float, DamageAmount, AActor*, SourceActor, EUBPowerType, DamagePower, float, CurrentHealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FUBVehicleRepairedSignature, float, RepairAmount, float, CurrentHealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FUBVehicleDestroyedSignature, AActor*, SourceActor);

UCLASS(ClassGroup = (UnfriendBlur), meta = (BlueprintSpawnableComponent))
class UNFRIENDBLUR_API UUBVehicleHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UUBVehicleHealthComponent();

	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnfriendBlur|Vehicle Health")
	float MaxHealth = 100.0f;

	UPROPERTY(BlueprintAssignable, Category = "UnfriendBlur|Vehicle Health")
	FUBVehicleHealthChangedSignature OnHealthChanged;

	UPROPERTY(BlueprintAssignable, Category = "UnfriendBlur|Vehicle Health")
	FUBVehicleDamagedSignature OnDamaged;

	UPROPERTY(BlueprintAssignable, Category = "UnfriendBlur|Vehicle Health")
	FUBVehicleRepairedSignature OnRepaired;

	UPROPERTY(BlueprintAssignable, Category = "UnfriendBlur|Vehicle Health")
	FUBVehicleDestroyedSignature OnDestroyed;

	UFUNCTION(BlueprintCallable, Category = "UnfriendBlur|Vehicle Health")
	float ApplyDamage(float DamageAmount, AActor* SourceActor, EUBPowerType DamagePower);

	UFUNCTION(BlueprintCallable, Category = "UnfriendBlur|Vehicle Health")
	float RepairDamage(float RepairAmount, AActor* SourceActor);

	UFUNCTION(BlueprintCallable, Category = "UnfriendBlur|Vehicle Health")
	void ResetHealth();

	UFUNCTION(BlueprintPure, Category = "UnfriendBlur|Vehicle Health")
	float GetCurrentHealth() const { return CurrentHealth; }

	UFUNCTION(BlueprintPure, Category = "UnfriendBlur|Vehicle Health")
	float GetHealthNormalized() const;

	UFUNCTION(BlueprintPure, Category = "UnfriendBlur|Vehicle Health")
	bool IsDestroyed() const { return bDestroyed; }

	UFUNCTION(BlueprintCallable, Category = "UnfriendBlur|Vehicle Health", meta = (DefaultToSelf = "OwnerActor"))
	static UUBVehicleHealthComponent* FindOrCreateHealthComponent(AActor* OwnerActor);

protected:
	UPROPERTY(ReplicatedUsing = OnRep_CurrentHealth)
	float CurrentHealth = 100.0f;

	UPROPERTY(ReplicatedUsing = OnRep_Destroyed)
	bool bDestroyed = false;

	UFUNCTION()
	void OnRep_CurrentHealth();

	UFUNCTION()
	void OnRep_Destroyed();

	void NotifyHealthChanged(AActor* SourceActor);
};
