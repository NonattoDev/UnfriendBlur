#pragma once

#include "CoreMinimal.h"
#include "Powers/UBPowerTypes.h"

class UMeshComponent;
class UMaterialInstanceDynamic;
class UTexture2D;

UNFRIENDBLUR_API UMaterialInstanceDynamic* UBApplyPowerMaterial(UMeshComponent* MeshComponent, EUBPowerType PowerType, float EmissiveStrength = 4.0f);
UNFRIENDBLUR_API UTexture2D* UBLoadPowerIcon(EUBPowerType PowerType, bool bIsSuper = false);
