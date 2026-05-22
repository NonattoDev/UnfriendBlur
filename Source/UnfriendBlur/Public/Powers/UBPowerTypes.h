#pragma once

#include "CoreMinimal.h"
#include "UBPowerTypes.generated.h"

UENUM(BlueprintType)
enum class EUBPowerType : uint8
{
	None UMETA(DisplayName = "None"),
	Boost UMETA(DisplayName = "Boost / Nitro"),
	Shield UMETA(DisplayName = "Shield"),
	Repair UMETA(DisplayName = "Repair"),
	Barge UMETA(DisplayName = "Barge"),
	Bolt UMETA(DisplayName = "Bolt"),
	Shunt UMETA(DisplayName = "Shunt"),
	Mine UMETA(DisplayName = "Mine"),
	Shock UMETA(DisplayName = "Shock")
};

UNFRIENDBLUR_API FString UBPowerTypeToString(EUBPowerType PowerType);
UNFRIENDBLUR_API FString UBPowerTypeShortCode(EUBPowerType PowerType);
UNFRIENDBLUR_API FColor UBPowerTypeColor(EUBPowerType PowerType);
UNFRIENDBLUR_API FLinearColor UBPowerTypeLinearColor(EUBPowerType PowerType);
