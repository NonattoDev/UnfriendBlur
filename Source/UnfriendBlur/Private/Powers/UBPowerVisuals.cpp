#include "Powers/UBPowerVisuals.h"

#include "Components/MeshComponent.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

UMaterialInstanceDynamic* UBApplyPowerMaterial(UMeshComponent* MeshComponent, EUBPowerType PowerType, float EmissiveStrength)
{
	if (!MeshComponent)
	{
		return nullptr;
	}

	if (!MeshComponent->GetMaterial(0))
	{
		if (UMaterialInterface* BasicMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")))
		{
			MeshComponent->SetMaterial(0, BasicMaterial);
		}
	}

	UMaterialInstanceDynamic* DynamicMaterial = MeshComponent->CreateAndSetMaterialInstanceDynamic(0);
	if (!DynamicMaterial)
	{
		return nullptr;
	}

	const FLinearColor PowerColor = UBPowerTypeLinearColor(PowerType);
	DynamicMaterial->SetVectorParameterValue(TEXT("Color"), PowerColor);
	DynamicMaterial->SetVectorParameterValue(TEXT("BaseColor"), PowerColor);
	DynamicMaterial->SetVectorParameterValue(TEXT("EmissiveColor"), PowerColor * EmissiveStrength);
	DynamicMaterial->SetScalarParameterValue(TEXT("EmissiveStrength"), EmissiveStrength);
	return DynamicMaterial;
}

UTexture2D* UBLoadPowerIcon(EUBPowerType PowerType, bool bIsSuper)
{
	const TCHAR* IconPath = nullptr;

	switch (PowerType)
	{
	case EUBPowerType::Boost:
		IconPath = bIsSuper ? TEXT("/Game/UnfriendBlur/Art/PowerIcons/Icon_Super_Boost.Icon_Super_Boost") : TEXT("/Game/UnfriendBlur/Art/PowerIcons/Icon_Boost.Icon_Boost");
		break;
	case EUBPowerType::Shield:
		IconPath = bIsSuper ? TEXT("/Game/UnfriendBlur/Art/PowerIcons/Icon_Super_Shield.Icon_Super_Shield") : TEXT("/Game/UnfriendBlur/Art/PowerIcons/Icon_Shield.Icon_Shield");
		break;
	case EUBPowerType::Repair:
		IconPath = bIsSuper ? TEXT("/Game/UnfriendBlur/Art/PowerIcons/Icon_Super_Repair.Icon_Super_Repair") : TEXT("/Game/UnfriendBlur/Art/PowerIcons/Icon_Repair.Icon_Repair");
		break;
	case EUBPowerType::Barge:
		IconPath = bIsSuper ? TEXT("/Game/UnfriendBlur/Art/PowerIcons/Icon_Super_Barge.Icon_Super_Barge") : TEXT("/Game/UnfriendBlur/Art/PowerIcons/Icon_Barge.Icon_Barge");
		break;
	case EUBPowerType::Bolt:
		IconPath = bIsSuper ? TEXT("/Game/UnfriendBlur/Art/PowerIcons/Icon_Super_Bolt.Icon_Super_Bolt") : TEXT("/Game/UnfriendBlur/Art/PowerIcons/Icon_Bolt.Icon_Bolt");
		break;
	case EUBPowerType::Shunt:
		IconPath = bIsSuper ? TEXT("/Game/UnfriendBlur/Art/PowerIcons/Icon_Super_Shunt.Icon_Super_Shunt") : TEXT("/Game/UnfriendBlur/Art/PowerIcons/Icon_Shunt.Icon_Shunt");
		break;
	case EUBPowerType::Mine:
		IconPath = bIsSuper ? TEXT("/Game/UnfriendBlur/Art/PowerIcons/Icon_Super_Mine.Icon_Super_Mine") : TEXT("/Game/UnfriendBlur/Art/PowerIcons/Icon_Mine.Icon_Mine");
		break;
	case EUBPowerType::Shock:
		IconPath = bIsSuper ? TEXT("/Game/UnfriendBlur/Art/PowerIcons/Icon_Super_Shock.Icon_Super_Shock") : TEXT("/Game/UnfriendBlur/Art/PowerIcons/Icon_Shock.Icon_Shock");
		break;
	default:
		break;
	}

	return IconPath ? LoadObject<UTexture2D>(nullptr, IconPath) : nullptr;
}
