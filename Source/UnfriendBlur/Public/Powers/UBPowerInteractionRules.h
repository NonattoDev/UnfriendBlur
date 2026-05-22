#pragma once

#include "CoreMinimal.h"
#include "Powers/UBPowerTypes.h"

class UNFRIENDBLUR_API FUBPowerInteractionRules
{
public:
	static int32 GetProjectileDamageAgainstShunt(EUBPowerType IncomingPowerType);
	static bool ShouldMineDestroyShunt();
	static bool ShouldRadialPowerDestroyShunt(EUBPowerType IncomingPowerType);
	static bool ShouldRadialPowerIgnoreShunt(EUBPowerType IncomingPowerType);
	static bool ShouldCounterSpawnAreaPulse(EUBPowerType IncomingPowerType);
};
