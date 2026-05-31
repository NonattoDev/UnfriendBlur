#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "Powers/UBPowerTypes.h"
#include "UBCombatHud.generated.h"

class AUBPrototypeTargetCar;
class USceneCaptureComponent2D;
class UTexture;
class UTexture2D;
class UTextureRenderTarget2D;

UCLASS()
class UNFRIENDBLUR_API AUBCombatHud : public AHUD
{
	GENERATED_BODY()

public:
	AUBCombatHud();
	virtual void BeginPlay() override;
	virtual void DrawHUD() override;

private:
	struct FUBRaceEntry
	{
		TWeakObjectPtr<AActor> Actor;
		float Progress = 0.0f;
		bool bIsPlayer = false;
	};

	void DrawRearViewMirror(APawn* PlayerPawn, const TArray<AUBPrototypeTargetCar*>& TargetCars);
	void DrawPositionPanel(APawn* PlayerPawn, const TArray<AUBPrototypeTargetCar*>& TargetCars, const TArray<FVector>& RaceWaypoints);
	void DrawPowerSlots(APawn* PlayerPawn);
	void DrawMiniMap(APawn* PlayerPawn, const TArray<AUBPrototypeTargetCar*>& TargetCars, const TArray<FVector>& RaceWaypoints);
	void DrawSpeedCluster(APawn* PlayerPawn);
	void DrawGlassPanel(float X, float Y, float Width, float Height, const FLinearColor& FillColor, const FLinearColor& BorderColor, float BorderThickness = 2.0f);
	void DrawSoftBar(float X, float Y, float Width, float Height, float FillAlpha, const FLinearColor& FillColor, const FLinearColor& BackColor);
	void DrawTextureFit(UTexture2D* Texture, float X, float Y, float Width, float Height, const FLinearColor& TintColor);
	void DrawTextureFit(UTexture* Texture, float X, float Y, float Width, float Height, const FLinearColor& TintColor);
	void GatherTargetCars(TArray<AUBPrototypeTargetCar*>& OutTargetCars) const;
	float GetActorSpeedKmh(const AActor* Actor) const;
	float ComputeRouteProgress(const AActor* Actor, const TArray<FVector>& RaceWaypoints) const;
	FVector2D ProjectToMiniMap(const FVector& WorldLocation, const TArray<FVector>& RaceWaypoints, const FVector2D& Center, float Radius) const;
	FString FormatOrdinal(int32 Position) const;
	UTexture2D* GetCachedPowerIcon(EUBPowerType PowerType, bool bIsSuper);
	void UpdateRearViewCapture(APawn* PlayerPawn);

	UPROPERTY(Transient)
	TObjectPtr<USceneCaptureComponent2D> RearViewCapture;

	UPROPERTY(Transient)
	TObjectPtr<UTextureRenderTarget2D> RearViewRenderTarget;

	UPROPERTY(Transient)
	TMap<int32, TObjectPtr<UTexture2D>> PowerIconCache;
};
