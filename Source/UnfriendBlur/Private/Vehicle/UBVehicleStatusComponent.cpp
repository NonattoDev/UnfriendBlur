#include "Vehicle/UBVehicleStatusComponent.h"

#include "Components/PrimitiveComponent.h"
#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"

UUBVehicleStatusComponent::UUBVehicleStatusComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	SetIsReplicatedByDefault(true);
}

void UUBVehicleStatusComponent::BeginPlay()
{
	Super::BeginPlay();
	SetComponentTickEnabled(GetOwner() && GetOwner()->HasAuthority() && bSlowed);
}

void UUBVehicleStatusComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority() || !bSlowed)
	{
		return;
	}

	SlowTimeRemaining = FMath::Max(0.0f, SlowTimeRemaining - DeltaTime);
	if (SlowTimeRemaining <= 0.0f)
	{
		SetSlowState(false, 0.0f, 0.0f);
		LastSlowPower = EUBPowerType::None;
		SetComponentTickEnabled(false);
		OwnerActor->ForceNetUpdate();
	}
}

void UUBVehicleStatusComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UUBVehicleStatusComponent, bSlowed);
	DOREPLIFETIME(UUBVehicleStatusComponent, SlowStrength);
	DOREPLIFETIME(UUBVehicleStatusComponent, SlowTimeRemaining);
	DOREPLIFETIME(UUBVehicleStatusComponent, LastSlowPower);
}

void UUBVehicleStatusComponent::ApplySlow(float Strength, float DurationSeconds, AActor* SourceActor, EUBPowerType SourcePower)
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority() || Strength <= 0.0f || DurationSeconds <= 0.0f)
	{
		return;
	}

	const float ClampedStrength = FMath::Clamp(Strength, 0.0f, 0.85f);
	const bool bShouldReplace = !bSlowed || ClampedStrength >= SlowStrength || DurationSeconds > SlowTimeRemaining;
	if (!bShouldReplace)
	{
		return;
	}

	SetSlowState(true, FMath::Max(ClampedStrength, SlowStrength), FMath::Max(DurationSeconds, SlowTimeRemaining));
	LastSlowPower = SourcePower;
	ApplyInstantVelocityPenalty(ClampedStrength);
	SetComponentTickEnabled(true);
	OwnerActor->ForceNetUpdate();
}

void UUBVehicleStatusComponent::ClearNegativeStatus()
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority())
	{
		return;
	}

	SetSlowState(false, 0.0f, 0.0f);
	LastSlowPower = EUBPowerType::None;
	SetComponentTickEnabled(false);
	OwnerActor->ForceNetUpdate();
}

UUBVehicleStatusComponent* UUBVehicleStatusComponent::FindOrCreateStatusComponent(AActor* OwnerActor)
{
	if (!OwnerActor)
	{
		return nullptr;
	}

	if (UUBVehicleStatusComponent* ExistingComponent = OwnerActor->FindComponentByClass<UUBVehicleStatusComponent>())
	{
		return ExistingComponent;
	}

	UUBVehicleStatusComponent* NewComponent = NewObject<UUBVehicleStatusComponent>(OwnerActor, UUBVehicleStatusComponent::StaticClass(), TEXT("UBVehicleStatus"));
	if (!NewComponent)
	{
		return nullptr;
	}

	OwnerActor->AddInstanceComponent(NewComponent);
	NewComponent->SetIsReplicated(true);
	NewComponent->RegisterComponent();
	return NewComponent;
}

void UUBVehicleStatusComponent::OnRep_Slow()
{
	OnSlowChanged.Broadcast(bSlowed, SlowStrength, SlowTimeRemaining);
}

void UUBVehicleStatusComponent::ApplyInstantVelocityPenalty(float Strength)
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

	if (!Primitive)
	{
		return;
	}

	const FVector Velocity = Primitive->GetPhysicsLinearVelocity();
	const float Multiplier = FMath::Clamp(1.0f - Strength * InstantSlowVelocityLossScale, 0.35f, 1.0f);
	Primitive->SetPhysicsLinearVelocity(FVector(Velocity.X * Multiplier, Velocity.Y * Multiplier, Velocity.Z), false);
}

void UUBVehicleStatusComponent::SetSlowState(bool bNewSlowed, float NewStrength, float NewTimeRemaining)
{
	bSlowed = bNewSlowed;
	SlowStrength = bNewSlowed ? NewStrength : 0.0f;
	SlowTimeRemaining = bNewSlowed ? NewTimeRemaining : 0.0f;
	OnSlowChanged.Broadcast(bSlowed, SlowStrength, SlowTimeRemaining);
}
