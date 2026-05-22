#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Powers/UBPowerTypes.h"
#include "UBPowerInventoryComponent.generated.h"

class UPrimitiveComponent;

USTRUCT(BlueprintType)
struct UNFRIENDBLUR_API FUBPowerSlot
{
	GENERATED_BODY()

	FUBPowerSlot() = default;

	FUBPowerSlot(EUBPowerType InPowerType, bool bInIsSuper)
		: PowerType(InPowerType)
		, bIsSuper(bInIsSuper)
	{
	}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnfriendBlur|Powers")
	EUBPowerType PowerType = EUBPowerType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnfriendBlur|Powers")
	bool bIsSuper = false;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FUBPowerSlotsChangedSignature, const TArray<FUBPowerSlot>&, Slots);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FUBShieldChangedSignature, bool, bIsShieldActive);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FUBSelectedPowerSlotChangedSignature, int32, SelectedSlotIndex, EUBPowerType, SelectedPower, bool, bIsSuper);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FUBPowerActivatedSignature, EUBPowerType, PowerType, AActor*, SourceActor, bool, bFireForward, bool, bIsSuper);

UCLASS(ClassGroup = (UnfriendBlur), meta = (BlueprintSpawnableComponent))
class UNFRIENDBLUR_API UUBPowerInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UUBPowerInventoryComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnfriendBlur|Powers")
	int32 MaxSlots = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnfriendBlur|Powers|Boost")
	float BoostDeltaVelocity = 3500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnfriendBlur|Powers|Boost")
	float BoostRamDuration = 1.05f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnfriendBlur|Powers|Boost")
	float BoostRamRadius = 290.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnfriendBlur|Powers|Boost")
	float BoostRamMinimumRelativeSpeed = 2300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnfriendBlur|Powers|Boost")
	float BoostRamDeltaVelocity = 2700.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnfriendBlur|Powers|Boost")
	float BoostRamSelfSpeedMultiplier = 0.52f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnfriendBlur|Powers|Boost")
	float SuperBoostRamDuration = 1.55f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnfriendBlur|Powers|Boost")
	float SuperBoostRamMinimumRelativeSpeed = 1650.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnfriendBlur|Powers|Boost")
	float SuperBoostRamDeltaVelocity = 4300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnfriendBlur|Powers|Boost")
	float SuperBoostRamSelfSpeedMultiplier = 0.88f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnfriendBlur|Powers|Shield")
	float ShieldDuration = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnfriendBlur|Powers|Barge")
	float BargeRadius = 520.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnfriendBlur|Powers|Barge")
	float BargeDeltaVelocity = 2400.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnfriendBlur|Powers|Shock")
	float ShockRadius = 950.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnfriendBlur|Powers|Shock")
	float ShockDeltaVelocity = 1800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnfriendBlur|Powers|Super")
	float SuperPowerMultiplier = 1.75f;

	UPROPERTY(BlueprintAssignable, Category = "UnfriendBlur|Powers")
	FUBPowerSlotsChangedSignature OnPowerSlotsChanged;

	UPROPERTY(BlueprintAssignable, Category = "UnfriendBlur|Powers")
	FUBShieldChangedSignature OnShieldChanged;

	UPROPERTY(BlueprintAssignable, Category = "UnfriendBlur|Powers")
	FUBSelectedPowerSlotChangedSignature OnSelectedPowerSlotChanged;

	UPROPERTY(BlueprintAssignable, Category = "UnfriendBlur|Powers")
	FUBPowerActivatedSignature OnPowerActivated;

	UFUNCTION(BlueprintCallable, Category = "UnfriendBlur|Powers")
	bool GivePower(EUBPowerType PowerType);

	UFUNCTION(BlueprintCallable, Category = "UnfriendBlur|Powers")
	bool GivePowerSlot(const FUBPowerSlot& PowerSlot);

	UFUNCTION(Server, Reliable)
	void ServerGivePower(EUBPowerType PowerType);

	UFUNCTION(BlueprintCallable, Category = "UnfriendBlur|Powers")
	bool UsePowerSlot(int32 SlotIndex);

	UFUNCTION(Server, Reliable)
	void ServerUsePowerSlot(int32 SlotIndex);

	UFUNCTION(BlueprintCallable, Category = "UnfriendBlur|Powers")
	bool UsePowerSlotDirected(int32 SlotIndex, bool bFireForward);

	UFUNCTION(Server, Reliable)
	void ServerUsePowerSlotDirected(int32 SlotIndex, bool bFireForward);

	UFUNCTION(BlueprintCallable, Category = "UnfriendBlur|Powers")
	bool UseSelectedPower(bool bFireForward);

	UFUNCTION(BlueprintCallable, Category = "UnfriendBlur|Powers")
	bool DropSelectedPower();

	UFUNCTION(Server, Reliable)
	void ServerDropSelectedPower();

	UFUNCTION(BlueprintCallable, Category = "UnfriendBlur|Powers")
	void SelectNextPowerSlot();

	UFUNCTION(BlueprintCallable, Category = "UnfriendBlur|Powers")
	void SetSelectedPowerSlot(int32 SlotIndex);

	UFUNCTION(BlueprintCallable, Category = "UnfriendBlur|Powers")
	void ClearPowers();

	UFUNCTION(BlueprintCallable, Category = "UnfriendBlur|Powers|Debug")
	bool DebugActivatePower(EUBPowerType PowerType);

	UFUNCTION(Server, Reliable)
	void ServerDebugActivatePower(EUBPowerType PowerType);

	UFUNCTION(BlueprintPure, Category = "UnfriendBlur|Powers")
	const TArray<FUBPowerSlot>& GetPowerSlots() const { return Slots; }

	UFUNCTION(BlueprintPure, Category = "UnfriendBlur|Powers")
	int32 GetSelectedSlotIndex() const { return SelectedSlotIndex; }

	UFUNCTION(BlueprintPure, Category = "UnfriendBlur|Powers")
	EUBPowerType GetSelectedPower() const;

	UFUNCTION(BlueprintPure, Category = "UnfriendBlur|Powers")
	bool IsSelectedPowerSuper() const;

	UFUNCTION(BlueprintPure, Category = "UnfriendBlur|Powers")
	bool IsShieldActive() const { return bShieldActive; }

	UFUNCTION(BlueprintCallable, Category = "UnfriendBlur|Powers")
	bool TryBlockIncomingPower(AActor* SourceActor, EUBPowerType PowerType);

	UFUNCTION(BlueprintCallable, Category = "UnfriendBlur|Powers")
	void ApplyPowerHit(EUBPowerType PowerType, AActor* SourceActor, float DeltaVelocity);

	UFUNCTION(BlueprintCallable, Category = "UnfriendBlur|Powers")
	void ApplyPowerHitWithContext(EUBPowerType PowerType, AActor* SourceActor, float DeltaVelocity, bool bWeakenedHit);

	UFUNCTION(BlueprintCallable, Category = "UnfriendBlur|Powers", meta = (DefaultToSelf = "OwnerActor"))
	static UUBPowerInventoryComponent* FindOrCreatePowerComponent(AActor* OwnerActor);

protected:
	UPROPERTY(ReplicatedUsing = OnRep_Slots)
	TArray<FUBPowerSlot> Slots;

	UPROPERTY(ReplicatedUsing = OnRep_ShieldActive)
	bool bShieldActive = false;

	UPROPERTY(Replicated)
	bool bSuperShieldReflectReady = false;

	UPROPERTY(Replicated)
	bool bBoostRamActive = false;

	UPROPERTY(Replicated)
	bool bSuperBoostRamActive = false;

	UPROPERTY(ReplicatedUsing = OnRep_SelectedSlotIndex)
	int32 SelectedSlotIndex = 0;

	FTimerHandle ShieldTimerHandle;
	FTimerHandle HeavyShuntRecoveryTimerHandle;
	FTimerHandle BoostRamTimerHandle;
	FVector BoostRamDirection = FVector::ForwardVector;
	TSet<TWeakObjectPtr<AActor>> BoostRamHitActors;

	UFUNCTION()
	void OnRep_Slots();

	UFUNCTION()
	void OnRep_ShieldActive();

	UFUNCTION()
	void OnRep_SelectedSlotIndex();

	void NotifySlotsChanged();
	void NotifySelectedSlotChanged();
	void ClampSelectedSlot();
	bool TryMergeMatchingPowers();
	bool TryMergeIncomingPower(EUBPowerType PowerType);
	FString FormatPowerSlot(const FUBPowerSlot& Slot) const;
	void SetShieldActive(bool bNewShieldActive);
	void FinishShield();
	void ActivatePower(EUBPowerType PowerType, bool bFireForward, bool bIsSuper);
	void ActivateBoost(bool bFireForward, bool bIsSuper);
	void ActivateShield(bool bIsSuper);
	void ActivateRepair(bool bIsSuper);
	void ActivateBarge(bool bFireForward, bool bIsSuper);
	void ActivateBolt(bool bFireForward, bool bIsSuper);
	void ActivateShunt(bool bFireForward, bool bIsSuper);
	void ActivateMine(bool bFireForward, bool bIsSuper);
	void ActivateShock(bool bFireForward, bool bIsSuper);
	void AddForwardDeltaVelocity(float DeltaVelocity, bool bFireForward) const;
	void ApplyRadialDeltaVelocity(float Radius, float DeltaVelocity, float UpwardVelocity, EUBPowerType PowerType, bool bFireForward) const;
	void SpawnProjectile(EUBPowerType PowerType, bool bHoming, bool bFireForward) const;
	void SpawnProjectileDirected(EUBPowerType PowerType, bool bHoming, const FVector& Direction) const;
	void SpawnMineDirected(const FVector& Direction, float LateralOffset) const;
	void SpawnDroppedPowerPickup(const FUBPowerSlot& PowerSlot) const;
	void SpawnPowerFx(EUBPowerType PowerType, const FVector& Location, const FVector& Direction, float LifeSeconds, float VisualScale, bool bAttachToOwner = false, bool bIsSuper = false) const;
	void ShowPowerMessage(const FString& Message) const;
	void ApplyPowerHitInternal(EUBPowerType PowerType, AActor* SourceActor, float DeltaVelocity, bool bWeakenedHit);
	void RecoverFromHeavyShuntHit();
	void BeginBoostRamWindow(bool bFireForward, bool bIsSuper);
	void EndBoostRamWindow();
	void DetectBoostRamTargets();
	bool TryApplyBoostRamHit(AActor* OtherActor);
	void ApplyBoostRamSelfPenalty();
	UPrimitiveComponent* FindBestPrimitive(AActor* Actor) const;
	FVector GetActorPhysicsVelocity(AActor* Actor) const;

	UFUNCTION()
	void HandleOwnerComponentHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
};
