#include "Powers/UBPowerInventoryComponent.h"

#include "Components/PrimitiveComponent.h"
#include "Engine/Engine.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "Net/UnrealNetwork.h"
#include "Powers/UBPowerFxActor.h"
#include "Powers/UBPowerInteractionRules.h"
#include "Powers/UBPowerMine.h"
#include "Powers/UBPowerPickup.h"
#include "Powers/UBPowerProjectile.h"
#include "Vehicle/UBVehicleHealthComponent.h"
#include "Vehicle/UBVehicleStatusComponent.h"
#include "TimerManager.h"

UUBPowerInventoryComponent::UUBPowerInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	SetIsReplicatedByDefault(true);
}

void UUBPowerInventoryComponent::BeginPlay()
{
	Super::BeginPlay();

	if (UPrimitiveComponent* OwnerPrimitive = FindBestPrimitive(GetOwner()))
	{
		OwnerPrimitive->SetNotifyRigidBodyCollision(true);
		OwnerPrimitive->OnComponentHit.AddUniqueDynamic(this, &UUBPowerInventoryComponent::HandleOwnerComponentHit);
	}

	if (AActor* OwnerActor = GetOwner(); OwnerActor && OwnerActor->HasAuthority())
	{
		UUBVehicleHealthComponent::FindOrCreateHealthComponent(OwnerActor);
		UUBVehicleStatusComponent::FindOrCreateStatusComponent(OwnerActor);
	}
}

void UUBPowerInventoryComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (GetOwner() && GetOwner()->HasAuthority() && bBoostRamActive)
	{
		DetectBoostRamTargets();
	}
}

void UUBPowerInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UUBPowerInventoryComponent, Slots);
	DOREPLIFETIME(UUBPowerInventoryComponent, bShieldActive);
	DOREPLIFETIME(UUBPowerInventoryComponent, bSuperShieldReflectReady);
	DOREPLIFETIME(UUBPowerInventoryComponent, bBoostRamActive);
	DOREPLIFETIME(UUBPowerInventoryComponent, bSuperBoostRamActive);
	DOREPLIFETIME(UUBPowerInventoryComponent, SelectedSlotIndex);
}

bool UUBPowerInventoryComponent::GivePower(EUBPowerType PowerType)
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || PowerType == EUBPowerType::None)
	{
		return false;
	}

	if (!OwnerActor->HasAuthority())
	{
		ServerGivePower(PowerType);
		return true;
	}

	if (Slots.Num() >= MaxSlots)
	{
		if (TryMergeIncomingPower(PowerType))
		{
			ClampSelectedSlot();
			NotifySlotsChanged();
			NotifySelectedSlotChanged();
			ShowPowerMessage(FString::Printf(TEXT("SUPER %s created"), *UBPowerTypeToString(PowerType)));
			return true;
		}

		ShowPowerMessage(TEXT("Power inventory is full"));
		return false;
	}

	Slots.Add(FUBPowerSlot(PowerType, false));
	const bool bMerged = TryMergeMatchingPowers();
	ClampSelectedSlot();
	NotifySlotsChanged();
	NotifySelectedSlotChanged();
	ShowPowerMessage(bMerged
		? FString::Printf(TEXT("SUPER %s created"), *UBPowerTypeToString(PowerType))
		: FString::Printf(TEXT("Picked up %s"), *UBPowerTypeToString(PowerType)));
	return true;
}

bool UUBPowerInventoryComponent::GivePowerSlot(const FUBPowerSlot& PowerSlot)
{
	if (PowerSlot.PowerType == EUBPowerType::None)
	{
		return false;
	}

	if (!PowerSlot.bIsSuper)
	{
		return GivePower(PowerSlot.PowerType);
	}

	AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
	{
		return false;
	}

	if (!OwnerActor->HasAuthority())
	{
		ShowPowerMessage(TEXT("Super pickup must be granted by the server"));
		return false;
	}

	if (Slots.Num() >= MaxSlots)
	{
		ShowPowerMessage(TEXT("Power inventory is full"));
		return false;
	}

	Slots.Add(PowerSlot);
	ClampSelectedSlot();
	NotifySlotsChanged();
	NotifySelectedSlotChanged();
	ShowPowerMessage(FString::Printf(TEXT("Picked up SUPER %s"), *UBPowerTypeToString(PowerSlot.PowerType)));
	return true;
}

void UUBPowerInventoryComponent::ServerGivePower_Implementation(EUBPowerType PowerType)
{
	GivePower(PowerType);
}

bool UUBPowerInventoryComponent::UsePowerSlot(int32 SlotIndex)
{
	return UsePowerSlotDirected(SlotIndex, true);
}

void UUBPowerInventoryComponent::ServerUsePowerSlot_Implementation(int32 SlotIndex)
{
	UsePowerSlotDirected(SlotIndex, true);
}

bool UUBPowerInventoryComponent::UsePowerSlotDirected(int32 SlotIndex, bool bFireForward)
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
	{
		return false;
	}

	if (!OwnerActor->HasAuthority())
	{
		ServerUsePowerSlotDirected(SlotIndex, bFireForward);
		return true;
	}

	if (!Slots.IsValidIndex(SlotIndex))
	{
		return false;
	}

	const FUBPowerSlot PowerSlot = Slots[SlotIndex];
	Slots.RemoveAt(SlotIndex);
	ClampSelectedSlot();
	NotifySlotsChanged();
	NotifySelectedSlotChanged();
	ActivatePower(PowerSlot.PowerType, bFireForward, PowerSlot.bIsSuper);
	return true;
}

void UUBPowerInventoryComponent::ServerUsePowerSlotDirected_Implementation(int32 SlotIndex, bool bFireForward)
{
	UsePowerSlotDirected(SlotIndex, bFireForward);
}

bool UUBPowerInventoryComponent::UseSelectedPower(bool bFireForward)
{
	return UsePowerSlotDirected(SelectedSlotIndex, bFireForward);
}

bool UUBPowerInventoryComponent::DropSelectedPower()
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
	{
		return false;
	}

	if (!OwnerActor->HasAuthority())
	{
		ServerDropSelectedPower();
		return true;
	}

	if (!Slots.IsValidIndex(SelectedSlotIndex))
	{
		ShowPowerMessage(TEXT("No selected power to drop"));
		return false;
	}

	const FUBPowerSlot DroppedSlot = Slots[SelectedSlotIndex];
	Slots.RemoveAt(SelectedSlotIndex);
	SpawnDroppedPowerPickup(DroppedSlot);
	ClampSelectedSlot();
	NotifySlotsChanged();
	NotifySelectedSlotChanged();
	ShowPowerMessage(FString::Printf(TEXT("Dropped %s%s"), DroppedSlot.bIsSuper ? TEXT("SUPER ") : TEXT(""), *UBPowerTypeToString(DroppedSlot.PowerType)));
	return true;
}

void UUBPowerInventoryComponent::ServerDropSelectedPower_Implementation()
{
	DropSelectedPower();
}

void UUBPowerInventoryComponent::SelectNextPowerSlot()
{
	if (Slots.Num() == 0)
	{
		SelectedSlotIndex = 0;
		NotifySelectedSlotChanged();
		ShowPowerMessage(TEXT("No powers in inventory"));
		return;
	}

	SelectedSlotIndex = (SelectedSlotIndex + 1) % Slots.Num();
	NotifySelectedSlotChanged();
	ShowPowerMessage(FString::Printf(TEXT("Selected slot %d: %s"), SelectedSlotIndex + 1, *FormatPowerSlot(Slots[SelectedSlotIndex])));
}

void UUBPowerInventoryComponent::SetSelectedPowerSlot(int32 SlotIndex)
{
	SelectedSlotIndex = SlotIndex;
	ClampSelectedSlot();
	NotifySelectedSlotChanged();
}

void UUBPowerInventoryComponent::ClearPowers()
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
	{
		return;
	}

	if (!OwnerActor->HasAuthority())
	{
		return;
	}

	Slots.Reset();
	SelectedSlotIndex = 0;
	NotifySlotsChanged();
	NotifySelectedSlotChanged();
}

bool UUBPowerInventoryComponent::DebugActivatePower(EUBPowerType PowerType)
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || PowerType == EUBPowerType::None)
	{
		return false;
	}

	if (!OwnerActor->HasAuthority())
	{
		ServerDebugActivatePower(PowerType);
		return true;
	}

	ActivatePower(PowerType, true, false);
	return true;
}

void UUBPowerInventoryComponent::ServerDebugActivatePower_Implementation(EUBPowerType PowerType)
{
	DebugActivatePower(PowerType);
}

EUBPowerType UUBPowerInventoryComponent::GetSelectedPower() const
{
	return Slots.IsValidIndex(SelectedSlotIndex) ? Slots[SelectedSlotIndex].PowerType : EUBPowerType::None;
}

bool UUBPowerInventoryComponent::IsSelectedPowerSuper() const
{
	return Slots.IsValidIndex(SelectedSlotIndex) && Slots[SelectedSlotIndex].bIsSuper;
}

bool UUBPowerInventoryComponent::TryBlockIncomingPower(AActor* SourceActor, EUBPowerType PowerType)
{
	if (!bShieldActive)
	{
		return false;
	}

	ShowPowerMessage(FString::Printf(TEXT("Shield blocked %s"), *UBPowerTypeToString(PowerType)));
	if (AActor* OwnerActor = GetOwner())
	{
		SpawnPowerFx(EUBPowerType::Shield, OwnerActor->GetActorLocation() + FVector::UpVector * 56.0f, OwnerActor->GetActorForwardVector(), 0.5f, 0.72f, false, bSuperShieldReflectReady);

		if (bSuperShieldReflectReady && SourceActor)
		{
			bSuperShieldReflectReady = false;
			if (UUBPowerInventoryComponent* SourcePowerComponent = SourceActor->FindComponentByClass<UUBPowerInventoryComponent>())
			{
				SourcePowerComponent->ApplyPowerHit(EUBPowerType::Shield, OwnerActor, 2600.0f);
				ShowPowerMessage(TEXT("SUPER Shield reflected the hit"));
			}
		}
	}
	return true;
}

void UUBPowerInventoryComponent::ApplyPowerHit(EUBPowerType PowerType, AActor* SourceActor, float DeltaVelocity)
{
	ApplyPowerHitInternal(PowerType, SourceActor, DeltaVelocity, false);
}

void UUBPowerInventoryComponent::ApplyPowerHitWithContext(EUBPowerType PowerType, AActor* SourceActor, float DeltaVelocity, bool bWeakenedHit)
{
	ApplyPowerHitInternal(PowerType, SourceActor, DeltaVelocity, bWeakenedHit);
}

void UUBPowerInventoryComponent::ApplyPowerHitInternal(EUBPowerType PowerType, AActor* SourceActor, float DeltaVelocity, bool bWeakenedHit)
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
	{
		return;
	}

	if (TryBlockIncomingPower(SourceActor, PowerType))
	{
		return;
	}

	if (OwnerActor->HasAuthority())
	{
		ApplyPowerGameplayOutcome(PowerType, SourceActor, bWeakenedHit);
	}

	UPrimitiveComponent* Primitive = Cast<UPrimitiveComponent>(OwnerActor->GetRootComponent());
	if (!Primitive || !Primitive->IsSimulatingPhysics())
	{
		Primitive = OwnerActor->FindComponentByClass<UPrimitiveComponent>();
	}

	if (Primitive)
	{
		const FVector SourceLocation = SourceActor ? SourceActor->GetActorLocation() : OwnerActor->GetActorLocation() - OwnerActor->GetActorForwardVector();
		const FVector Direction = (OwnerActor->GetActorLocation() - SourceLocation).GetSafeNormal();
		if (PowerType == EUBPowerType::Shunt && !bWeakenedHit)
		{
			const FVector Impulse = Direction * DeltaVelocity + FVector::UpVector * (DeltaVelocity * 0.92f);
			Primitive->AddImpulse(Impulse, NAME_None, true);
			Primitive->AddAngularImpulseInRadians((OwnerActor->GetActorRightVector() * DeltaVelocity * 0.5f) + (OwnerActor->GetActorForwardVector() * DeltaVelocity * 0.35f), NAME_None, true);

			if (UWorld* World = GetWorld())
			{
				World->GetTimerManager().ClearTimer(HeavyShuntRecoveryTimerHandle);
				World->GetTimerManager().SetTimer(HeavyShuntRecoveryTimerHandle, this, &UUBPowerInventoryComponent::RecoverFromHeavyShuntHit, 2.4f, false);
			}
		}
		else if (PowerType == EUBPowerType::Shunt && bWeakenedHit)
		{
			const FVector Right = OwnerActor->GetActorRightVector();
			const float SideSign = FVector::DotProduct(Direction, Right) < 0.0f ? -1.0f : 1.0f;
			const FVector Impulse = Direction * (DeltaVelocity * 0.55f) + Right * (SideSign * DeltaVelocity * 0.45f) + FVector::UpVector * (DeltaVelocity * 0.08f);
			Primitive->AddImpulse(Impulse, NAME_None, true);
			Primitive->AddAngularImpulseInRadians(OwnerActor->GetActorUpVector() * SideSign * DeltaVelocity * 0.32f, NAME_None, true);
		}
		else
		{
			const FVector Impulse = Direction * DeltaVelocity + FVector::UpVector * (DeltaVelocity * 0.35f);
			Primitive->AddImpulse(Impulse, NAME_None, true);
		}
	}

	if (PowerType == EUBPowerType::Shunt && bWeakenedHit)
	{
		ShowPowerMessage(TEXT("Hit by weakened Shunt"));
	}
	else
	{
		ShowPowerMessage(FString::Printf(TEXT("Hit by %s"), *UBPowerTypeToString(PowerType)));
	}
}

void UUBPowerInventoryComponent::ApplyPowerGameplayOutcome(EUBPowerType PowerType, AActor* SourceActor, bool bWeakenedHit)
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority())
	{
		return;
	}

	const float Damage = GetDamageForPowerHit(PowerType, bWeakenedHit);
	if (Damage > 0.0f)
	{
		if (UUBVehicleHealthComponent* HealthComponent = UUBVehicleHealthComponent::FindOrCreateHealthComponent(OwnerActor))
		{
			HealthComponent->ApplyDamage(Damage, SourceActor, PowerType);
		}
	}

	const float SlowStrength = GetSlowStrengthForPowerHit(PowerType, bWeakenedHit);
	const float SlowDuration = GetSlowDurationForPowerHit(PowerType, bWeakenedHit);
	if (SlowStrength > 0.0f && SlowDuration > 0.0f)
	{
		if (UUBVehicleStatusComponent* StatusComponent = UUBVehicleStatusComponent::FindOrCreateStatusComponent(OwnerActor))
		{
			StatusComponent->ApplySlow(SlowStrength, SlowDuration, SourceActor, PowerType);
		}
	}
}

float UUBPowerInventoryComponent::GetDamageForPowerHit(EUBPowerType PowerType, bool bWeakenedHit) const
{
	switch (PowerType)
	{
	case EUBPowerType::Boost:
		return 20.0f;
	case EUBPowerType::Shield:
		return 12.0f;
	case EUBPowerType::Barge:
		return 16.0f;
	case EUBPowerType::Bolt:
		return 18.0f;
	case EUBPowerType::Shunt:
		return bWeakenedHit ? 24.0f : 45.0f;
	case EUBPowerType::Mine:
		return 34.0f;
	case EUBPowerType::Shock:
		return 30.0f;
	default:
		return 0.0f;
	}
}

float UUBPowerInventoryComponent::GetSlowStrengthForPowerHit(EUBPowerType PowerType, bool bWeakenedHit) const
{
	switch (PowerType)
	{
	case EUBPowerType::Boost:
		return 0.28f;
	case EUBPowerType::Shield:
		return 0.18f;
	case EUBPowerType::Barge:
		return 0.25f;
	case EUBPowerType::Bolt:
		return 0.22f;
	case EUBPowerType::Shunt:
		return bWeakenedHit ? 0.34f : 0.62f;
	case EUBPowerType::Mine:
		return 0.46f;
	case EUBPowerType::Shock:
		return 0.68f;
	default:
		return 0.0f;
	}
}

float UUBPowerInventoryComponent::GetSlowDurationForPowerHit(EUBPowerType PowerType, bool bWeakenedHit) const
{
	switch (PowerType)
	{
	case EUBPowerType::Boost:
		return 0.8f;
	case EUBPowerType::Shield:
		return 0.5f;
	case EUBPowerType::Barge:
		return 0.9f;
	case EUBPowerType::Bolt:
		return 1.0f;
	case EUBPowerType::Shunt:
		return bWeakenedHit ? 1.4f : 2.5f;
	case EUBPowerType::Mine:
		return 2.0f;
	case EUBPowerType::Shock:
		return 2.8f;
	default:
		return 0.0f;
	}
}

void UUBPowerInventoryComponent::RecoverFromHeavyShuntHit()
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority())
	{
		return;
	}

	UPrimitiveComponent* Primitive = Cast<UPrimitiveComponent>(OwnerActor->GetRootComponent());
	if (!Primitive || !Primitive->IsSimulatingPhysics())
	{
		Primitive = OwnerActor->FindComponentByClass<UPrimitiveComponent>();
	}

	if (Primitive)
	{
		Primitive->SetPhysicsAngularVelocityInRadians(Primitive->GetPhysicsAngularVelocityInRadians() * 0.18f, false);
		Primitive->SetPhysicsLinearVelocity(Primitive->GetPhysicsLinearVelocity() * 0.82f, false);
	}

	if (OwnerActor->GetActorUpVector().Z < 0.15f)
	{
		const FRotator CurrentRotation = OwnerActor->GetActorRotation();
		const FRotator UprightRotation(0.0f, CurrentRotation.Yaw, 0.0f);
		OwnerActor->SetActorLocation(OwnerActor->GetActorLocation() + FVector::UpVector * 80.0f, false, nullptr, ETeleportType::TeleportPhysics);
		OwnerActor->SetActorRotation(UprightRotation, ETeleportType::TeleportPhysics);
	}
}

void UUBPowerInventoryComponent::BeginBoostRamWindow(bool bFireForward, bool bIsSuper)
{
	AActor* OwnerActor = GetOwner();
	UWorld* World = GetWorld();
	if (!OwnerActor || !OwnerActor->HasAuthority() || !World)
	{
		return;
	}

	BoostRamDirection = bFireForward ? OwnerActor->GetActorForwardVector() : -OwnerActor->GetActorForwardVector();
	BoostRamDirection = BoostRamDirection.GetSafeNormal();
	if (BoostRamDirection.IsNearlyZero())
	{
		BoostRamDirection = FVector::ForwardVector;
	}

	BoostRamHitActors.Reset();
	bBoostRamActive = true;
	bSuperBoostRamActive = bIsSuper;
	SetComponentTickEnabled(true);

	World->GetTimerManager().ClearTimer(BoostRamTimerHandle);
	World->GetTimerManager().SetTimer(BoostRamTimerHandle, this, &UUBPowerInventoryComponent::EndBoostRamWindow, bIsSuper ? SuperBoostRamDuration : BoostRamDuration, false);
}

void UUBPowerInventoryComponent::EndBoostRamWindow()
{
	bBoostRamActive = false;
	bSuperBoostRamActive = false;
	BoostRamHitActors.Reset();
	SetComponentTickEnabled(false);
}

void UUBPowerInventoryComponent::DetectBoostRamTargets()
{
	AActor* OwnerActor = GetOwner();
	UWorld* World = GetWorld();
	if (!OwnerActor || !World)
	{
		return;
	}

	TArray<FOverlapResult> Overlaps;
	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_PhysicsBody);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Vehicle);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(UBBoostRam), false, OwnerActor);
	const FVector Origin = OwnerActor->GetActorLocation() + BoostRamDirection * 230.0f + FVector::UpVector * 55.0f;
	World->OverlapMultiByObjectType(Overlaps, Origin, FQuat::Identity, ObjectQueryParams, FCollisionShape::MakeSphere(BoostRamRadius), QueryParams);

	for (const FOverlapResult& Overlap : Overlaps)
	{
		if (!bBoostRamActive)
		{
			return;
		}

		TryApplyBoostRamHit(Overlap.GetActor());
	}
}

bool UUBPowerInventoryComponent::TryApplyBoostRamHit(AActor* OtherActor)
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority() || !bBoostRamActive || !OtherActor || OtherActor == OwnerActor || BoostRamHitActors.Contains(OtherActor))
	{
		return false;
	}

	const FVector ToTarget = OtherActor->GetActorLocation() - OwnerActor->GetActorLocation();
	const float DirectionDot = FVector::DotProduct(BoostRamDirection, ToTarget.GetSafeNormal());
	if (DirectionDot < 0.18f)
	{
		return false;
	}

	const FVector OwnerVelocity = GetActorPhysicsVelocity(OwnerActor);
	const FVector TargetVelocity = GetActorPhysicsVelocity(OtherActor);
	const float RelativeSpeed = FVector::DotProduct(OwnerVelocity - TargetVelocity, BoostRamDirection);
	const float MinimumRelativeSpeed = bSuperBoostRamActive ? SuperBoostRamMinimumRelativeSpeed : BoostRamMinimumRelativeSpeed;
	if (RelativeSpeed < MinimumRelativeSpeed)
	{
		return false;
	}

	BoostRamHitActors.Add(OtherActor);
	const float RamDeltaVelocity = bSuperBoostRamActive ? SuperBoostRamDeltaVelocity : BoostRamDeltaVelocity;
	const FVector HitLocation = OtherActor->GetActorLocation() + FVector::UpVector * 80.0f;

	if (UUBPowerInventoryComponent* TargetPowerComponent = OtherActor->FindComponentByClass<UUBPowerInventoryComponent>())
	{
		TargetPowerComponent->ApplyPowerHitWithContext(EUBPowerType::Boost, OwnerActor, RamDeltaVelocity, false);
	}
	else if (UPrimitiveComponent* TargetPrimitive = FindBestPrimitive(OtherActor))
	{
		const FVector HitDirection = (OtherActor->GetActorLocation() - OwnerActor->GetActorLocation()).GetSafeNormal();
		TargetPrimitive->AddImpulse(HitDirection * RamDeltaVelocity + FVector::UpVector * (RamDeltaVelocity * (bSuperBoostRamActive ? 0.28f : 0.16f)), NAME_None, true);
	}
	else
	{
		return false;
	}

	SpawnPowerFx(EUBPowerType::Boost, HitLocation, BoostRamDirection, bSuperBoostRamActive ? 0.72f : 0.46f, bSuperBoostRamActive ? 1.25f : 0.78f, false, bSuperBoostRamActive);
	ApplyBoostRamSelfPenalty();
	ShowPowerMessage(bSuperBoostRamActive ? TEXT("SUPER Boost ram hit") : TEXT("Boost ram hit"));

	if (!bSuperBoostRamActive)
	{
		EndBoostRamWindow();
	}

	return true;
}

void UUBPowerInventoryComponent::ApplyBoostRamSelfPenalty()
{
	AActor* OwnerActor = GetOwner();
	UPrimitiveComponent* OwnerPrimitive = FindBestPrimitive(OwnerActor);
	if (!OwnerPrimitive)
	{
		return;
	}

	const FVector CurrentVelocity = OwnerPrimitive->GetPhysicsLinearVelocity();
	const float ForwardSpeed = FVector::DotProduct(CurrentVelocity, BoostRamDirection);
	if (ForwardSpeed <= 0.0f)
	{
		return;
	}

	const float SpeedMultiplier = bSuperBoostRamActive ? SuperBoostRamSelfSpeedMultiplier : BoostRamSelfSpeedMultiplier;
	const FVector LateralVelocity = CurrentVelocity - BoostRamDirection * ForwardSpeed;
	OwnerPrimitive->SetPhysicsLinearVelocity(LateralVelocity + BoostRamDirection * ForwardSpeed * SpeedMultiplier, false);
}

UPrimitiveComponent* UUBPowerInventoryComponent::FindBestPrimitive(AActor* Actor) const
{
	if (!Actor)
	{
		return nullptr;
	}

	UPrimitiveComponent* Primitive = Cast<UPrimitiveComponent>(Actor->GetRootComponent());
	if (Primitive && Primitive->IsSimulatingPhysics())
	{
		return Primitive;
	}

	for (UActorComponent* Component : Actor->GetComponents())
	{
		UPrimitiveComponent* Candidate = Cast<UPrimitiveComponent>(Component);
		if (Candidate && Candidate->IsSimulatingPhysics())
		{
			return Candidate;
		}
	}

	return Primitive ? Primitive : Actor->FindComponentByClass<UPrimitiveComponent>();
}

FVector UUBPowerInventoryComponent::GetActorPhysicsVelocity(AActor* Actor) const
{
	if (UPrimitiveComponent* Primitive = FindBestPrimitive(Actor))
	{
		return Primitive->GetPhysicsLinearVelocity();
	}

	return Actor ? Actor->GetVelocity() : FVector::ZeroVector;
}

void UUBPowerInventoryComponent::HandleOwnerComponentHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (GetOwner() && GetOwner()->HasAuthority() && bBoostRamActive)
	{
		TryApplyBoostRamHit(OtherActor);
	}
}

UUBPowerInventoryComponent* UUBPowerInventoryComponent::FindOrCreatePowerComponent(AActor* OwnerActor)
{
	if (!OwnerActor)
	{
		return nullptr;
	}

	if (UUBPowerInventoryComponent* ExistingComponent = OwnerActor->FindComponentByClass<UUBPowerInventoryComponent>())
	{
		return ExistingComponent;
	}

	UUBPowerInventoryComponent* NewComponent = NewObject<UUBPowerInventoryComponent>(OwnerActor, UUBPowerInventoryComponent::StaticClass(), TEXT("UBPowerInventory"));
	if (!NewComponent)
	{
		return nullptr;
	}

	OwnerActor->AddInstanceComponent(NewComponent);
	NewComponent->SetIsReplicated(true);
	NewComponent->RegisterComponent();
	return NewComponent;
}

void UUBPowerInventoryComponent::OnRep_Slots()
{
	NotifySlotsChanged();
}

void UUBPowerInventoryComponent::OnRep_ShieldActive()
{
	OnShieldChanged.Broadcast(bShieldActive);
}

void UUBPowerInventoryComponent::OnRep_SelectedSlotIndex()
{
	NotifySelectedSlotChanged();
}

void UUBPowerInventoryComponent::NotifySlotsChanged()
{
	OnPowerSlotsChanged.Broadcast(Slots);
}

void UUBPowerInventoryComponent::NotifySelectedSlotChanged()
{
	OnSelectedPowerSlotChanged.Broadcast(SelectedSlotIndex, GetSelectedPower(), IsSelectedPowerSuper());
}

void UUBPowerInventoryComponent::ClampSelectedSlot()
{
	if (Slots.Num() == 0)
	{
		SelectedSlotIndex = 0;
		return;
	}

	SelectedSlotIndex = FMath::Clamp(SelectedSlotIndex, 0, Slots.Num() - 1);
}

bool UUBPowerInventoryComponent::TryMergeMatchingPowers()
{
	for (int32 Index = 0; Index < Slots.Num(); ++Index)
	{
		const EUBPowerType CandidatePower = Slots[Index].PowerType;
		if (CandidatePower == EUBPowerType::None || Slots[Index].bIsSuper)
		{
			continue;
		}

		TArray<int32> MatchingIndices;
		for (int32 CompareIndex = 0; CompareIndex < Slots.Num(); ++CompareIndex)
		{
			if (Slots[CompareIndex].PowerType == CandidatePower && !Slots[CompareIndex].bIsSuper)
			{
				MatchingIndices.Add(CompareIndex);
			}
		}

		if (MatchingIndices.Num() < 3)
		{
			continue;
		}

		MatchingIndices.Sort([](int32 A, int32 B) { return A > B; });
		for (int32 RemoveIndex = 0; RemoveIndex < 3; ++RemoveIndex)
		{
			Slots.RemoveAt(MatchingIndices[RemoveIndex]);
		}

		Slots.Insert(FUBPowerSlot(CandidatePower, true), 0);
		SelectedSlotIndex = 0;
		return true;
	}

	return false;
}

bool UUBPowerInventoryComponent::TryMergeIncomingPower(EUBPowerType PowerType)
{
	if (PowerType == EUBPowerType::None)
	{
		return false;
	}

	TArray<int32> MatchingIndices;
	for (int32 Index = 0; Index < Slots.Num(); ++Index)
	{
		if (Slots[Index].PowerType == PowerType && !Slots[Index].bIsSuper)
		{
			MatchingIndices.Add(Index);
		}
	}

	if (MatchingIndices.Num() < 2)
	{
		return false;
	}

	MatchingIndices.Sort([](int32 A, int32 B) { return A > B; });
	Slots.RemoveAt(MatchingIndices[0]);
	Slots.RemoveAt(MatchingIndices[1]);
	Slots.Insert(FUBPowerSlot(PowerType, true), 0);
	SelectedSlotIndex = 0;
	return true;
}

FString UUBPowerInventoryComponent::FormatPowerSlot(const FUBPowerSlot& Slot) const
{
	if (Slot.PowerType == EUBPowerType::None)
	{
		return TEXT("---");
	}

	return Slot.bIsSuper
		? FString::Printf(TEXT("SUPER %s"), *UBPowerTypeToString(Slot.PowerType))
		: UBPowerTypeToString(Slot.PowerType);
}

void UUBPowerInventoryComponent::SetShieldActive(bool bNewShieldActive)
{
	if (bShieldActive == bNewShieldActive)
	{
		return;
	}

	bShieldActive = bNewShieldActive;
	OnShieldChanged.Broadcast(bShieldActive);
}

void UUBPowerInventoryComponent::FinishShield()
{
	SetShieldActive(false);
	bSuperShieldReflectReady = false;
	ShowPowerMessage(TEXT("Shield ended"));
}

void UUBPowerInventoryComponent::ActivatePower(EUBPowerType PowerType, bool bFireForward, bool bIsSuper)
{
	OnPowerActivated.Broadcast(PowerType, GetOwner(), bFireForward, bIsSuper);
	ShowPowerMessage(FString::Printf(TEXT("Using %s%s %s"), bIsSuper ? TEXT("SUPER ") : TEXT(""), *UBPowerTypeToString(PowerType), bFireForward ? TEXT("forward") : TEXT("backward")));

	switch (PowerType)
	{
	case EUBPowerType::Boost:
		ActivateBoost(bFireForward, bIsSuper);
		break;
	case EUBPowerType::Shield:
		ActivateShield(bIsSuper);
		break;
	case EUBPowerType::Repair:
		ActivateRepair(bIsSuper);
		break;
	case EUBPowerType::Barge:
		ActivateBarge(bFireForward, bIsSuper);
		break;
	case EUBPowerType::Bolt:
		ActivateBolt(bFireForward, bIsSuper);
		break;
	case EUBPowerType::Shunt:
		ActivateShunt(bFireForward, bIsSuper);
		break;
	case EUBPowerType::Mine:
		ActivateMine(bFireForward, bIsSuper);
		break;
	case EUBPowerType::Shock:
		ActivateShock(bFireForward, bIsSuper);
		break;
	default:
		break;
	}
}

void UUBPowerInventoryComponent::ActivateBoost(bool bFireForward, bool bIsSuper)
{
	AddForwardDeltaVelocity(BoostDeltaVelocity * (bIsSuper ? 2.35f : 1.0f), bFireForward);
	BeginBoostRamWindow(bFireForward, bIsSuper);

	if (const AActor* OwnerActor = GetOwner())
	{
		const FVector Direction = bFireForward ? OwnerActor->GetActorForwardVector() : -OwnerActor->GetActorForwardVector();
		SpawnPowerFx(EUBPowerType::Boost, OwnerActor->GetActorLocation() - Direction * 120.0f, Direction, bIsSuper ? 1.1f : 0.55f, bIsSuper ? 1.25f : 0.7f, false, bIsSuper);
	}
}

void UUBPowerInventoryComponent::ActivateShield(bool bIsSuper)
{
	SetShieldActive(true);
	bSuperShieldReflectReady = bIsSuper;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ShieldTimerHandle);
		World->GetTimerManager().SetTimer(ShieldTimerHandle, this, &UUBPowerInventoryComponent::FinishShield, ShieldDuration * (bIsSuper ? 1.35f : 1.0f), false);
	}

	if (const AActor* OwnerActor = GetOwner())
	{
		SpawnPowerFx(EUBPowerType::Shield, OwnerActor->GetActorLocation() + FVector::UpVector * 52.0f, OwnerActor->GetActorForwardVector(), ShieldDuration * (bIsSuper ? 1.35f : 1.0f), bIsSuper ? 1.18f : 0.88f, true, bIsSuper);
	}
}

void UUBPowerInventoryComponent::ActivateRepair(bool bIsSuper)
{
	AActor* OwnerActor = GetOwner();
	if (OwnerActor && OwnerActor->HasAuthority())
	{
		if (UUBVehicleHealthComponent* HealthComponent = UUBVehicleHealthComponent::FindOrCreateHealthComponent(OwnerActor))
		{
			const float Repaired = HealthComponent->RepairDamage(bIsSuper ? 48.0f : 26.0f, OwnerActor);
			ShowPowerMessage(FString::Printf(TEXT("%sRepair restored %.0f health"), bIsSuper ? TEXT("SUPER ") : TEXT(""), Repaired));
		}

		if (bIsSuper)
		{
			if (UUBVehicleStatusComponent* StatusComponent = UUBVehicleStatusComponent::FindOrCreateStatusComponent(OwnerActor))
			{
				StatusComponent->ClearNegativeStatus();
			}
		}
	}

	if (const AActor* FxOwnerActor = GetOwner())
	{
		SpawnPowerFx(EUBPowerType::Repair, FxOwnerActor->GetActorLocation() + FVector::UpVector * 58.0f, FxOwnerActor->GetActorForwardVector(), bIsSuper ? 1.35f : 0.9f, bIsSuper ? 1.05f : 0.68f, false, bIsSuper);
	}
}

void UUBPowerInventoryComponent::ActivateBarge(bool bFireForward, bool bIsSuper)
{
	ApplyRadialDeltaVelocity(BargeRadius * (bIsSuper ? 1.65f : 1.0f), BargeDeltaVelocity * (bIsSuper ? 1.75f : 1.0f), bIsSuper ? 950.0f : 650.0f, EUBPowerType::Barge, bFireForward);

	if (const AActor* OwnerActor = GetOwner())
	{
		const FVector Origin = OwnerActor->GetActorLocation() + (bFireForward ? OwnerActor->GetActorForwardVector() : -OwnerActor->GetActorForwardVector()) * 260.0f;
		const FVector Direction = bFireForward ? OwnerActor->GetActorForwardVector() : -OwnerActor->GetActorForwardVector();
		SpawnPowerFx(EUBPowerType::Barge, Origin + FVector::UpVector * 44.0f, Direction, bIsSuper ? 1.0f : 0.6f, bIsSuper ? 1.25f : 0.82f, false, bIsSuper);
	}
}

void UUBPowerInventoryComponent::ActivateBolt(bool bFireForward, bool bIsSuper)
{
	if (!bIsSuper)
	{
		SpawnProjectile(EUBPowerType::Bolt, false, bFireForward);
		return;
	}

	AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
	{
		return;
	}

	const FVector BaseDirection = bFireForward ? OwnerActor->GetActorForwardVector() : -OwnerActor->GetActorForwardVector();
	SpawnProjectileDirected(EUBPowerType::Bolt, false, BaseDirection.RotateAngleAxis(-8.0f, FVector::UpVector));
	SpawnProjectileDirected(EUBPowerType::Bolt, false, BaseDirection);
	SpawnProjectileDirected(EUBPowerType::Bolt, false, BaseDirection.RotateAngleAxis(8.0f, FVector::UpVector));
}

void UUBPowerInventoryComponent::ActivateShunt(bool bFireForward, bool bIsSuper)
{
	if (!bIsSuper)
	{
		SpawnProjectile(EUBPowerType::Shunt, true, bFireForward);
		return;
	}

	AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
	{
		return;
	}

	const FVector BaseDirection = bFireForward ? OwnerActor->GetActorForwardVector() : -OwnerActor->GetActorForwardVector();
	SpawnProjectileDirected(EUBPowerType::Shunt, true, BaseDirection.RotateAngleAxis(-10.0f, FVector::UpVector));
	SpawnProjectileDirected(EUBPowerType::Shunt, true, BaseDirection.RotateAngleAxis(10.0f, FVector::UpVector));
}

void UUBPowerInventoryComponent::ActivateMine(bool bFireForward, bool bIsSuper)
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
	{
		return;
	}

	const FVector Direction = bFireForward ? OwnerActor->GetActorForwardVector() : -OwnerActor->GetActorForwardVector();
	if (!bIsSuper)
	{
		SpawnMineDirected(Direction, 0.0f);
		return;
	}

	SpawnMineDirected(Direction, -190.0f);
	SpawnMineDirected(Direction, 0.0f);
	SpawnMineDirected(Direction, 190.0f);
}

void UUBPowerInventoryComponent::ActivateShock(bool bFireForward, bool bIsSuper)
{
	ApplyRadialDeltaVelocity(ShockRadius * (bIsSuper ? 1.35f : 1.0f), ShockDeltaVelocity * (bIsSuper ? 1.8f : 1.0f), bIsSuper ? 1350.0f : 950.0f, EUBPowerType::Shock, bFireForward);

	if (const AActor* OwnerActor = GetOwner())
	{
		const FVector Origin = OwnerActor->GetActorLocation() + (bFireForward ? OwnerActor->GetActorForwardVector() : -OwnerActor->GetActorForwardVector()) * 420.0f;
		const FVector Direction = bFireForward ? OwnerActor->GetActorForwardVector() : -OwnerActor->GetActorForwardVector();
		SpawnPowerFx(EUBPowerType::Shock, Origin + FVector::UpVector * 68.0f, Direction, bIsSuper ? 1.35f : 0.95f, bIsSuper ? 1.48f : 1.05f, false, bIsSuper);
		if (bIsSuper)
		{
			SpawnPowerFx(EUBPowerType::Shock, Origin + Direction * 500.0f + FVector::UpVector * 72.0f, Direction, 1.2f, 1.15f, false, true);
			SpawnPowerFx(EUBPowerType::Shock, Origin + Direction * 1000.0f + FVector::UpVector * 72.0f, Direction, 1.1f, 1.0f, false, true);
		}
	}
}

void UUBPowerInventoryComponent::AddForwardDeltaVelocity(float DeltaVelocity, bool bFireForward) const
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
	{
		return;
	}

	UPrimitiveComponent* Primitive = Cast<UPrimitiveComponent>(OwnerActor->GetRootComponent());
	if (!Primitive || !Primitive->IsSimulatingPhysics())
	{
		Primitive = OwnerActor->FindComponentByClass<UPrimitiveComponent>();
	}

	if (Primitive)
	{
		const FVector Direction = bFireForward ? OwnerActor->GetActorForwardVector() : -OwnerActor->GetActorForwardVector();
		Primitive->AddImpulse(Direction * DeltaVelocity, NAME_None, true);
	}
}

void UUBPowerInventoryComponent::ApplyRadialDeltaVelocity(float Radius, float DeltaVelocity, float UpwardVelocity, EUBPowerType PowerType, bool bFireForward) const
{
	AActor* OwnerActor = GetOwner();
	UWorld* World = GetWorld();
	if (!OwnerActor || !World)
	{
		return;
	}

	TArray<FOverlapResult> Overlaps;
	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_PhysicsBody);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Vehicle);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(UBPowerRadial), false, OwnerActor);
	const FCollisionShape Sphere = FCollisionShape::MakeSphere(Radius);
	const FVector Origin = OwnerActor->GetActorLocation() + (bFireForward ? OwnerActor->GetActorForwardVector() : -OwnerActor->GetActorForwardVector()) * 320.0f;
	World->OverlapMultiByObjectType(Overlaps, Origin, FQuat::Identity, ObjectQueryParams, Sphere, QueryParams);

	TSet<AActor*> HitActors;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* OtherActor = Overlap.GetActor();
		if (!OtherActor || OtherActor == OwnerActor || HitActors.Contains(OtherActor))
		{
			continue;
		}

		HitActors.Add(OtherActor);

		if (AUBPowerProjectile* Projectile = Cast<AUBPowerProjectile>(OtherActor))
		{
			if (Projectile->IsShuntProjectile())
			{
				if (FUBPowerInteractionRules::ShouldRadialPowerDestroyShunt(PowerType))
				{
					Projectile->DestroyByPowerCounter(PowerType, OwnerActor, FUBPowerInteractionRules::ShouldCounterSpawnAreaPulse(PowerType));
				}
				else if (FUBPowerInteractionRules::ShouldRadialPowerIgnoreShunt(PowerType))
				{
					continue;
				}
			}

			continue;
		}

		if (const UPrimitiveComponent* RootPrimitive = Cast<UPrimitiveComponent>(OtherActor->GetRootComponent()))
		{
			if (RootPrimitive->GetCollisionObjectType() == ECC_WorldDynamic)
			{
				continue;
			}
		}

		if (UUBPowerInventoryComponent* OtherPowerComponent = OtherActor->FindComponentByClass<UUBPowerInventoryComponent>())
		{
			OtherPowerComponent->ApplyPowerHitWithContext(PowerType, OwnerActor, DeltaVelocity, false);
			continue;
		}

		UPrimitiveComponent* Primitive = Cast<UPrimitiveComponent>(OtherActor->GetRootComponent());
		if (!Primitive || !Primitive->IsSimulatingPhysics())
		{
			Primitive = OtherActor->FindComponentByClass<UPrimitiveComponent>();
		}

		if (Primitive)
		{
			const FVector Away = (OtherActor->GetActorLocation() - Origin).GetSafeNormal();
			Primitive->AddImpulse(Away * DeltaVelocity + FVector::UpVector * UpwardVelocity, NAME_None, true);
		}
	}
}

void UUBPowerInventoryComponent::SpawnProjectile(EUBPowerType PowerType, bool bHoming, bool bFireForward) const
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
	{
		return;
	}

	const FVector Direction = bFireForward ? OwnerActor->GetActorForwardVector() : -OwnerActor->GetActorForwardVector();
	SpawnProjectileDirected(PowerType, bHoming, Direction);
}

void UUBPowerInventoryComponent::SpawnProjectileDirected(EUBPowerType PowerType, bool bHoming, const FVector& Direction) const
{
	AActor* OwnerActor = GetOwner();
	UWorld* World = GetWorld();
	if (!OwnerActor || !World)
	{
		return;
	}

	const FVector SafeDirection = Direction.GetSafeNormal();
	const FVector SpawnLocation = OwnerActor->GetActorLocation() + SafeDirection * 280.0f + FVector::UpVector * 80.0f;
	const FRotator SpawnRotation = SafeDirection.Rotation();
	const bool bFireForward = FVector::DotProduct(SafeDirection, OwnerActor->GetActorForwardVector()) >= 0.0f;

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = OwnerActor;
	SpawnParams.Instigator = Cast<APawn>(OwnerActor);
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	if (AUBPowerProjectile* Projectile = World->SpawnActor<AUBPowerProjectile>(AUBPowerProjectile::StaticClass(), SpawnLocation, SpawnRotation, SpawnParams))
	{
		Projectile->InitializeProjectile(PowerType, OwnerActor, bHoming, bFireForward);
	}
}

void UUBPowerInventoryComponent::SpawnMineDirected(const FVector& Direction, float LateralOffset) const
{
	AActor* OwnerActor = GetOwner();
	UWorld* World = GetWorld();
	if (!OwnerActor || !World)
	{
		return;
	}

	const FVector SafeDirection = Direction.GetSafeNormal();
	const FVector Right = FVector::CrossProduct(FVector::UpVector, SafeDirection).GetSafeNormal();
	const FVector SpawnLocation = OwnerActor->GetActorLocation() + SafeDirection * 320.0f + Right * LateralOffset + FVector::UpVector * 45.0f;
	const FRotator SpawnRotation = SafeDirection.Rotation();

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = OwnerActor;
	SpawnParams.Instigator = Cast<APawn>(OwnerActor);
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	if (AUBPowerMine* Mine = World->SpawnActor<AUBPowerMine>(AUBPowerMine::StaticClass(), SpawnLocation, SpawnRotation, SpawnParams))
	{
		Mine->InitializeMine(OwnerActor);
	}
}

void UUBPowerInventoryComponent::SpawnDroppedPowerPickup(const FUBPowerSlot& PowerSlot) const
{
	AActor* OwnerActor = GetOwner();
	UWorld* World = GetWorld();
	if (!OwnerActor || !World || PowerSlot.PowerType == EUBPowerType::None)
	{
		return;
	}

	const FVector Backward = -OwnerActor->GetActorForwardVector().GetSafeNormal();
	const FVector Right = OwnerActor->GetActorRightVector().GetSafeNormal();
	const FVector CandidateLocation = OwnerActor->GetActorLocation() + Backward * 360.0f + Right * 120.0f;
	FVector SpawnLocation = CandidateLocation + FVector::UpVector * 58.0f;
	FHitResult GroundHit;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(UBDroppedPowerGroundTrace), false, OwnerActor);
	if (World->LineTraceSingleByChannel(GroundHit, CandidateLocation + FVector::UpVector * 700.0f, CandidateLocation - FVector::UpVector * 2200.0f, ECC_Visibility, QueryParams))
	{
		SpawnLocation = GroundHit.Location + FVector::UpVector * 58.0f;
	}
	const FRotator SpawnRotation = FRotator::ZeroRotator;

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = OwnerActor;
	SpawnParams.Instigator = Cast<APawn>(OwnerActor);
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AUBPowerPickup* Pickup = World->SpawnActor<AUBPowerPickup>(AUBPowerPickup::StaticClass(), SpawnLocation, SpawnRotation, SpawnParams);
	if (!Pickup)
	{
		return;
	}

	Pickup->FixedPower = PowerSlot.PowerType;
	Pickup->bIsSuperPickup = PowerSlot.bIsSuper;
	Pickup->bRespawns = false;
	Pickup->RespawnSeconds = 0.0f;
	Pickup->RefreshVisuals();
}

void UUBPowerInventoryComponent::SpawnPowerFx(EUBPowerType PowerType, const FVector& Location, const FVector& Direction, float LifeSeconds, float VisualScale, bool bAttachToOwner, bool bIsSuper) const
{
	AActor* OwnerActor = GetOwner();
	UWorld* World = GetWorld();
	if (!OwnerActor || !World || !OwnerActor->HasAuthority())
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = OwnerActor;
	SpawnParams.Instigator = Cast<APawn>(OwnerActor);
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AUBPowerFxActor* FxActor = World->SpawnActor<AUBPowerFxActor>(AUBPowerFxActor::StaticClass(), Location, Direction.Rotation(), SpawnParams);
	if (!FxActor)
	{
		return;
	}

	FxActor->InitializePowerFx(PowerType, Direction, LifeSeconds, VisualScale, bIsSuper);

	if (bAttachToOwner)
	{
		FxActor->AttachToActor(OwnerActor, FAttachmentTransformRules::KeepWorldTransform);
	}
}

void UUBPowerInventoryComponent::ShowPowerMessage(const FString& Message) const
{
	UE_LOG(LogTemp, Log, TEXT("[UnfriendBlur Powers] %s"), *Message);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Yellow, Message);
	}
}
