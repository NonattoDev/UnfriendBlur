#include "Vehicle/UBVehicleHealthComponent.h"

#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"

UUBVehicleHealthComponent::UUBVehicleHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UUBVehicleHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	if (GetOwner() && GetOwner()->HasAuthority())
	{
		CurrentHealth = FMath::Clamp(CurrentHealth <= 0.0f ? MaxHealth : CurrentHealth, 0.0f, MaxHealth);
		bDestroyed = CurrentHealth <= 0.0f;
	}
}

void UUBVehicleHealthComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UUBVehicleHealthComponent, CurrentHealth);
	DOREPLIFETIME(UUBVehicleHealthComponent, bDestroyed);
}

float UUBVehicleHealthComponent::ApplyDamage(float DamageAmount, AActor* SourceActor, EUBPowerType DamagePower)
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority() || DamageAmount <= 0.0f || bDestroyed)
	{
		return 0.0f;
	}

	const float PreviousHealth = CurrentHealth;
	CurrentHealth = FMath::Clamp(CurrentHealth - DamageAmount, 0.0f, MaxHealth);
	const float AppliedDamage = PreviousHealth - CurrentHealth;
	if (AppliedDamage <= 0.0f)
	{
		return 0.0f;
	}

	OnDamaged.Broadcast(AppliedDamage, SourceActor, DamagePower, CurrentHealth);
	NotifyHealthChanged(SourceActor);

	if (CurrentHealth <= 0.0f && !bDestroyed)
	{
		bDestroyed = true;
		OnDestroyed.Broadcast(SourceActor);
	}

	OwnerActor->ForceNetUpdate();
	return AppliedDamage;
}

float UUBVehicleHealthComponent::RepairDamage(float RepairAmount, AActor* SourceActor)
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority() || RepairAmount <= 0.0f)
	{
		return 0.0f;
	}

	const float PreviousHealth = CurrentHealth;
	CurrentHealth = FMath::Clamp(CurrentHealth + RepairAmount, 0.0f, MaxHealth);
	if (CurrentHealth > 0.0f)
	{
		bDestroyed = false;
	}

	const float AppliedRepair = CurrentHealth - PreviousHealth;
	if (AppliedRepair <= 0.0f)
	{
		return 0.0f;
	}

	OnRepaired.Broadcast(AppliedRepair, CurrentHealth);
	NotifyHealthChanged(SourceActor);
	OwnerActor->ForceNetUpdate();
	return AppliedRepair;
}

void UUBVehicleHealthComponent::ResetHealth()
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority())
	{
		return;
	}

	CurrentHealth = MaxHealth;
	bDestroyed = false;
	NotifyHealthChanged(nullptr);
	OwnerActor->ForceNetUpdate();
}

float UUBVehicleHealthComponent::GetHealthNormalized() const
{
	return MaxHealth > 0.0f ? CurrentHealth / MaxHealth : 0.0f;
}

UUBVehicleHealthComponent* UUBVehicleHealthComponent::FindOrCreateHealthComponent(AActor* OwnerActor)
{
	if (!OwnerActor)
	{
		return nullptr;
	}

	if (UUBVehicleHealthComponent* ExistingComponent = OwnerActor->FindComponentByClass<UUBVehicleHealthComponent>())
	{
		return ExistingComponent;
	}

	UUBVehicleHealthComponent* NewComponent = NewObject<UUBVehicleHealthComponent>(OwnerActor, UUBVehicleHealthComponent::StaticClass(), TEXT("UBVehicleHealth"));
	if (!NewComponent)
	{
		return nullptr;
	}

	OwnerActor->AddInstanceComponent(NewComponent);
	NewComponent->SetIsReplicated(true);
	NewComponent->RegisterComponent();
	return NewComponent;
}

void UUBVehicleHealthComponent::OnRep_CurrentHealth()
{
	NotifyHealthChanged(nullptr);
}

void UUBVehicleHealthComponent::OnRep_Destroyed()
{
	if (bDestroyed)
	{
		OnDestroyed.Broadcast(nullptr);
	}
}

void UUBVehicleHealthComponent::NotifyHealthChanged(AActor* SourceActor)
{
	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth, SourceActor);
}
