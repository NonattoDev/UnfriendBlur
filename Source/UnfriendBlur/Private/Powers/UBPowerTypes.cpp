#include "Powers/UBPowerTypes.h"

FString UBPowerTypeToString(EUBPowerType PowerType)
{
	switch (PowerType)
	{
	case EUBPowerType::Boost:
		return TEXT("Boost / Nitro");
	case EUBPowerType::Shield:
		return TEXT("Shield");
	case EUBPowerType::Repair:
		return TEXT("Repair");
	case EUBPowerType::Barge:
		return TEXT("Barge");
	case EUBPowerType::Bolt:
		return TEXT("Bolt");
	case EUBPowerType::Shunt:
		return TEXT("Shunt");
	case EUBPowerType::Mine:
		return TEXT("Mine");
	case EUBPowerType::Shock:
		return TEXT("Shock");
	default:
		return TEXT("None");
	}
}

FString UBPowerTypeShortCode(EUBPowerType PowerType)
{
	switch (PowerType)
	{
	case EUBPowerType::Boost:
		return TEXT("BST");
	case EUBPowerType::Shield:
		return TEXT("SHD");
	case EUBPowerType::Repair:
		return TEXT("REP");
	case EUBPowerType::Barge:
		return TEXT("BRG");
	case EUBPowerType::Bolt:
		return TEXT("BLT");
	case EUBPowerType::Shunt:
		return TEXT("SHT");
	case EUBPowerType::Mine:
		return TEXT("MIN");
	case EUBPowerType::Shock:
		return TEXT("SHK");
	default:
		return TEXT("?");
	}
}

FColor UBPowerTypeColor(EUBPowerType PowerType)
{
	switch (PowerType)
	{
	case EUBPowerType::Boost:
		return FColor(255, 156, 28);
	case EUBPowerType::Shield:
		return FColor(40, 220, 255);
	case EUBPowerType::Repair:
		return FColor(44, 235, 120);
	case EUBPowerType::Barge:
		return FColor(255, 78, 48);
	case EUBPowerType::Bolt:
		return FColor(255, 232, 42);
	case EUBPowerType::Shunt:
		return FColor(210, 80, 255);
	case EUBPowerType::Mine:
		return FColor(255, 55, 80);
	case EUBPowerType::Shock:
		return FColor(95, 145, 255);
	default:
		return FColor(255, 255, 255);
	}
}

FLinearColor UBPowerTypeLinearColor(EUBPowerType PowerType)
{
	return FLinearColor(UBPowerTypeColor(PowerType));
}
