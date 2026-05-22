#include "Powers/UBPowerInteractionRules.h"

int32 FUBPowerInteractionRules::GetProjectileDamageAgainstShunt(EUBPowerType IncomingPowerType)
{
	return IncomingPowerType == EUBPowerType::Bolt ? 1 : 0;
}

bool FUBPowerInteractionRules::ShouldMineDestroyShunt()
{
	return true;
}

bool FUBPowerInteractionRules::ShouldRadialPowerDestroyShunt(EUBPowerType IncomingPowerType)
{
	// Barge is the temporary PEM/EMP-style pulse until that power becomes its own type.
	return IncomingPowerType == EUBPowerType::Barge;
}

bool FUBPowerInteractionRules::ShouldRadialPowerIgnoreShunt(EUBPowerType IncomingPowerType)
{
	return IncomingPowerType == EUBPowerType::Shock;
}

bool FUBPowerInteractionRules::ShouldCounterSpawnAreaPulse(EUBPowerType IncomingPowerType)
{
	return IncomingPowerType == EUBPowerType::Barge;
}
