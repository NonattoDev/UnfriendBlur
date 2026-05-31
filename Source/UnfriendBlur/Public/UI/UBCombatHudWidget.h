#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "Fonts/SlateFontInfo.h"
#include "Powers/UBPowerTypes.h"
#include "Styling/SlateBrush.h"
#include "UBCombatHudWidget.generated.h"

class AUBCombatHud;
class AUBPrototypeTargetCar;
class UTexture2D;

UCLASS()
class UNFRIENDBLUR_API UUBCombatHudWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	explicit UUBCombatHudWidget(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void NativeConstruct() override;
	virtual int32 NativePaint(
		const FPaintArgs& Args,
		const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FWidgetStyle& InWidgetStyle,
		bool bParentEnabled) const override;

private:
	struct FUBHudRaceEntry
	{
		TWeakObjectPtr<AActor> Actor;
		float Progress = 0.0f;
		bool bIsPlayer = false;
	};

	FPaintGeometry MakePaintGeometry(const FGeometry& AllottedGeometry, const FVector2D& Position, const FVector2D& Size) const;
	void DrawBox(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FVector2D& Position, const FVector2D& Size, const FSlateBrush& Brush, const FLinearColor& Tint) const;
	void DrawPanel(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FVector2D& Position, const FVector2D& Size, const FLinearColor& FillColor, const FLinearColor& BorderColor) const;
	void DrawLine(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FVector2D& Start, const FVector2D& End, const FLinearColor& Tint, float Thickness = 1.0f) const;
	void DrawPolyline(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId, const TArray<FVector2D>& Points, const FLinearColor& Tint, float Thickness = 1.0f) const;
	void DrawTextGlow(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FString& Text, const FSlateFontInfo& Font, const FVector2D& Position, const FLinearColor& TextColor, const FLinearColor& GlowColor, FVector2D TextBoxSize = FVector2D(360.0f, 60.0f)) const;
	void DrawMiniTriangle(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FVector2D& Center, float Size, float YawDegrees, const FLinearColor& Tint) const;

	int32 DrawRearViewMirror(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId, AUBCombatHud* Hud, APawn* PlayerPawn, const TArray<AUBPrototypeTargetCar*>& TargetCars, const FVector2D& ViewSize) const;
	int32 DrawPositionPanel(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId, AUBCombatHud* Hud, APawn* PlayerPawn, const TArray<AUBPrototypeTargetCar*>& TargetCars, const TArray<FVector>& RaceWaypoints, const FVector2D& ViewSize) const;
	int32 DrawPowerSlots(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId, AUBCombatHud* Hud, APawn* PlayerPawn, const FVector2D& ViewSize) const;
	int32 DrawMiniMap(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId, AUBCombatHud* Hud, APawn* PlayerPawn, const TArray<AUBPrototypeTargetCar*>& TargetCars, const TArray<FVector>& RaceWaypoints, const FVector2D& ViewSize) const;
	int32 DrawSpeedCluster(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId, AUBCombatHud* Hud, APawn* PlayerPawn, const FVector2D& ViewSize) const;

	FSlateBrush* GetPowerIconBrush(AUBCombatHud* Hud, EUBPowerType PowerType, bool bIsSuper) const;

	mutable TMap<int32, FSlateBrush> PowerIconBrushCache;
	mutable FSlateBrush RearViewBrush;

	FSlateBrush PanelBrush;
	FSlateBrush SoftBrush;
	FSlateBrush SlotBrush;
	FSlateBrush DotBrush;
	FSlateBrush GlowBrush;

	FSlateFontInfo TinyFont;
	FSlateFontInfo LabelFont;
	FSlateFontInfo MediumFont;
	FSlateFontInfo LargeFont;
	FSlateFontInfo SpeedFont;
};
