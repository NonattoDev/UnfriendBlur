#include "UI/UBCombatHudWidget.h"

#include "Brushes/SlateRoundedBoxBrush.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Powers/UBPowerInventoryComponent.h"
#include "Powers/UBPowerTypes.h"
#include "Prototype/UBPowerPrototypeSubsystem.h"
#include "Prototype/UBPrototypeTargetCar.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "UI/UBCombatHud.h"

namespace
{
	FVector2f ToVector2f(const FVector2D& Vector)
	{
		return FVector2f(static_cast<float>(Vector.X), static_cast<float>(Vector.Y));
	}

	FLinearColor WithAlpha(FLinearColor Color, float Alpha)
	{
		Color.A = Alpha;
		return Color;
	}
}

UUBCombatHudWidget::UUBCombatHudWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, TinyFont(FCoreStyle::GetDefaultFontStyle("Regular", 11.0f))
	, LabelFont(FCoreStyle::GetDefaultFontStyle("Bold", 13.0f))
	, MediumFont(FCoreStyle::GetDefaultFontStyle("Bold", 22.0f))
	, LargeFont(FCoreStyle::GetDefaultFontStyle("Bold", 48.0f))
	, SpeedFont(FCoreStyle::GetDefaultFontStyle("Bold", 42.0f))
{
	SetIsFocusable(false);
	PanelBrush = FSlateRoundedBoxBrush(FLinearColor::White, 14.0f, FLinearColor::White, 1.0f);
	SoftBrush = FSlateRoundedBoxBrush(FLinearColor::White, 10.0f);
	SlotBrush = FSlateRoundedBoxBrush(FLinearColor::White, 12.0f, FLinearColor::White, 1.0f);
	DotBrush = FSlateRoundedBoxBrush(FLinearColor::White, 999.0f);
	GlowBrush = FSlateRoundedBoxBrush(FLinearColor::White, 999.0f);
	RearViewBrush.DrawAs = ESlateBrushDrawType::Image;
	RearViewBrush.Tiling = ESlateBrushTileType::NoTile;
	RearViewBrush.Mirroring = ESlateBrushMirrorType::NoMirror;
	RearViewBrush.ImageSize = FVector2D(768.0f, 192.0f);
}

void UUBCombatHudWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

int32 UUBCombatHudWidget::NativePaint(
	const FPaintArgs& Args,
	const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements,
	int32 LayerId,
	const FWidgetStyle& InWidgetStyle,
	bool bParentEnabled) const
{
	int32 CurrentLayer = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);

	const FVector2D ViewSize = AllottedGeometry.GetLocalSize();
	if (ViewSize.X <= 32.0f || ViewSize.Y <= 32.0f)
	{
		return CurrentLayer;
	}

	APlayerController* OwningPlayer = GetOwningPlayer();
	AUBCombatHud* Hud = OwningPlayer ? Cast<AUBCombatHud>(OwningPlayer->GetHUD()) : nullptr;
	APawn* PlayerPawn = OwningPlayer ? OwningPlayer->GetPawn() : nullptr;
	if (!Hud || !PlayerPawn)
	{
		return CurrentLayer;
	}

	TArray<AUBPrototypeTargetCar*> TargetCars;
	Hud->GatherTargetCars(TargetCars);

	TArray<FVector> RaceWaypoints;
	if (const UWorld* World = GetWorld())
	{
		if (const UUBPowerPrototypeSubsystem* PrototypeSubsystem = World->GetSubsystem<UUBPowerPrototypeSubsystem>())
		{
			RaceWaypoints = PrototypeSubsystem->GetPrototypeRaceWaypoints();
		}
	}

	CurrentLayer += 1;
	CurrentLayer = DrawRearViewMirror(AllottedGeometry, OutDrawElements, CurrentLayer, Hud, PlayerPawn, TargetCars, ViewSize);
	CurrentLayer = DrawPositionPanel(AllottedGeometry, OutDrawElements, CurrentLayer + 1, Hud, PlayerPawn, TargetCars, RaceWaypoints, ViewSize);
	CurrentLayer = DrawMiniMap(AllottedGeometry, OutDrawElements, CurrentLayer + 1, Hud, PlayerPawn, TargetCars, RaceWaypoints, ViewSize);
	CurrentLayer = DrawSpeedCluster(AllottedGeometry, OutDrawElements, CurrentLayer + 1, Hud, PlayerPawn, ViewSize);
	CurrentLayer = DrawPowerSlots(AllottedGeometry, OutDrawElements, CurrentLayer + 1, Hud, PlayerPawn, ViewSize);

	return CurrentLayer;
}

FPaintGeometry UUBCombatHudWidget::MakePaintGeometry(const FGeometry& AllottedGeometry, const FVector2D& Position, const FVector2D& Size) const
{
	return AllottedGeometry.ToPaintGeometry(ToVector2f(Size), FSlateLayoutTransform(ToVector2f(Position)));
}

void UUBCombatHudWidget::DrawBox(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FVector2D& Position, const FVector2D& Size, const FSlateBrush& Brush, const FLinearColor& Tint) const
{
	FSlateDrawElement::MakeBox(OutDrawElements, LayerId, MakePaintGeometry(AllottedGeometry, Position, Size), &Brush, ESlateDrawEffect::None, Tint);
}

void UUBCombatHudWidget::DrawPanel(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FVector2D& Position, const FVector2D& Size, const FLinearColor& FillColor, const FLinearColor& BorderColor) const
{
	DrawBox(AllottedGeometry, OutDrawElements, LayerId, Position + FVector2D(3.0f, 4.0f), Size, PanelBrush, FLinearColor(0.0f, 0.0f, 0.0f, 0.42f));
	DrawBox(AllottedGeometry, OutDrawElements, LayerId + 1, Position, Size, PanelBrush, FillColor);
	DrawBox(AllottedGeometry, OutDrawElements, LayerId + 2, Position + FVector2D(4.0f, 3.0f), FVector2D(Size.X - 8.0f, Size.Y * 0.30f), SoftBrush, FLinearColor(1.0f, 1.0f, 1.0f, 0.065f));

	DrawLine(AllottedGeometry, OutDrawElements, LayerId + 3, Position + FVector2D(12.0f, 0.0f), Position + FVector2D(Size.X - 12.0f, 0.0f), BorderColor, 1.8f);
	DrawLine(AllottedGeometry, OutDrawElements, LayerId + 3, Position + FVector2D(Size.X, 14.0f), Position + FVector2D(Size.X, Size.Y - 14.0f), WithAlpha(BorderColor, BorderColor.A * 0.7f), 1.2f);
	DrawLine(AllottedGeometry, OutDrawElements, LayerId + 3, Position + FVector2D(Size.X - 12.0f, Size.Y), Position + FVector2D(12.0f, Size.Y), WithAlpha(BorderColor, BorderColor.A * 0.55f), 1.4f);
	DrawLine(AllottedGeometry, OutDrawElements, LayerId + 3, Position + FVector2D(0.0f, Size.Y - 14.0f), Position + FVector2D(0.0f, 14.0f), WithAlpha(BorderColor, BorderColor.A * 0.55f), 1.2f);
	DrawLine(AllottedGeometry, OutDrawElements, LayerId + 3, Position + FVector2D(0.0f, 14.0f), Position + FVector2D(14.0f, 0.0f), WithAlpha(BorderColor, BorderColor.A * 0.95f), 1.5f);
	DrawLine(AllottedGeometry, OutDrawElements, LayerId + 3, Position + FVector2D(Size.X - 14.0f, 0.0f), Position + FVector2D(Size.X, 14.0f), WithAlpha(BorderColor, BorderColor.A * 0.95f), 1.5f);
}

void UUBCombatHudWidget::DrawLine(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FVector2D& Start, const FVector2D& End, const FLinearColor& Tint, float Thickness) const
{
	TArray<FVector2f> Points;
	Points.Add(ToVector2f(Start));
	Points.Add(ToVector2f(End));
	FSlateDrawElement::MakeLines(
		OutDrawElements,
		LayerId,
		AllottedGeometry.ToPaintGeometry(ToVector2f(AllottedGeometry.GetLocalSize()), FSlateLayoutTransform()),
		Points,
		ESlateDrawEffect::None,
		Tint,
		true,
		Thickness);
}

void UUBCombatHudWidget::DrawPolyline(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId, const TArray<FVector2D>& Points, const FLinearColor& Tint, float Thickness) const
{
	TArray<FVector2f> SlatePoints;
	SlatePoints.Reserve(Points.Num());
	for (const FVector2D& Point : Points)
	{
		SlatePoints.Add(ToVector2f(Point));
	}

	FSlateDrawElement::MakeLines(
		OutDrawElements,
		LayerId,
		AllottedGeometry.ToPaintGeometry(ToVector2f(AllottedGeometry.GetLocalSize()), FSlateLayoutTransform()),
		SlatePoints,
		ESlateDrawEffect::None,
		Tint,
		true,
		Thickness);
}

void UUBCombatHudWidget::DrawTextGlow(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FString& Text, const FSlateFontInfo& Font, const FVector2D& Position, const FLinearColor& TextColor, const FLinearColor& GlowColor, FVector2D TextBoxSize) const
{
	FSlateDrawElement::MakeText(OutDrawElements, LayerId, MakePaintGeometry(AllottedGeometry, Position + FVector2D(1.5f, 1.5f), TextBoxSize), Text, Font, ESlateDrawEffect::None, GlowColor);
	FSlateDrawElement::MakeText(OutDrawElements, LayerId + 1, MakePaintGeometry(AllottedGeometry, Position, TextBoxSize), Text, Font, ESlateDrawEffect::None, TextColor);
}

void UUBCombatHudWidget::DrawMiniTriangle(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FVector2D& Center, float Size, float YawDegrees, const FLinearColor& Tint) const
{
	const float YawRadians = FMath::DegreesToRadians(YawDegrees);
	const FVector2D Forward(FMath::Cos(YawRadians), FMath::Sin(YawRadians));
	const FVector2D Right(-Forward.Y, Forward.X);
	const FVector2D Nose = Center + Forward * Size;

	TArray<FVector2D> Points;
	Points.Add(Nose);
	Points.Add(Center - Forward * Size * 0.72f + Right * Size * 0.58f);
	Points.Add(Center - Forward * Size * 0.72f - Right * Size * 0.58f);
	Points.Add(Nose);
	DrawPolyline(AllottedGeometry, OutDrawElements, LayerId, Points, Tint, 2.6f);
}

int32 UUBCombatHudWidget::DrawRearViewMirror(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId, AUBCombatHud* Hud, APawn* PlayerPawn, const TArray<AUBPrototypeTargetCar*>& TargetCars, const FVector2D& ViewSize) const
{
	const float Width = FMath::Clamp(ViewSize.X * 0.50f, 430.0f, 760.0f);
	const float Height = FMath::Clamp(ViewSize.Y * 0.112f, 68.0f, 104.0f);
	const FVector2D Position((ViewSize.X - Width) * 0.5f, ViewSize.Y * 0.032f);
	const FVector2D Size(Width, Height);

	DrawPanel(AllottedGeometry, OutDrawElements, LayerId, Position, Size, FLinearColor(0.006f, 0.028f, 0.038f, 0.72f), FLinearColor(0.11f, 0.82f, 1.0f, 0.76f));

	const FVector2D InnerPosition = Position + FVector2D(14.0f, 9.0f);
	const FVector2D InnerSize = Size - FVector2D(28.0f, 18.0f);
	Hud->UpdateRearViewCapture(PlayerPawn);
	if (UTextureRenderTarget2D* RenderTarget = Hud->GetRearViewRenderTarget())
	{
		RearViewBrush.SetResourceObject(RenderTarget);
		DrawBox(AllottedGeometry, OutDrawElements, LayerId + 4, InnerPosition, InnerSize, RearViewBrush, FLinearColor(0.92f, 1.0f, 1.0f, 0.94f));
		DrawBox(AllottedGeometry, OutDrawElements, LayerId + 5, InnerPosition, InnerSize, SoftBrush, FLinearColor(0.0f, 0.09f, 0.13f, 0.18f));
	}
	else
	{
		DrawBox(AllottedGeometry, OutDrawElements, LayerId + 4, InnerPosition, InnerSize, SoftBrush, FLinearColor(0.0f, 0.0f, 0.0f, 0.46f));
	}

	for (int32 Index = 0; Index < 4; ++Index)
	{
		const float Alpha = static_cast<float>(Index + 1) / 5.0f;
		DrawLine(AllottedGeometry, OutDrawElements, LayerId + 6, InnerPosition + FVector2D(10.0f, InnerSize.Y * Alpha), InnerPosition + FVector2D(InnerSize.X - 10.0f, InnerSize.Y * Alpha - 3.0f), FLinearColor(0.32f, 0.86f, 0.94f, 0.075f), 1.0f);
	}

	const FVector PlayerLocation = PlayerPawn->GetActorLocation();
	const FVector Forward = PlayerPawn->GetActorForwardVector().GetSafeNormal2D();
	const FVector Right = PlayerPawn->GetActorRightVector().GetSafeNormal2D();
	int32 BehindCount = 0;
	for (const AUBPrototypeTargetCar* TargetCar : TargetCars)
	{
		if (!TargetCar)
		{
			continue;
		}

		const FVector ToTarget = TargetCar->GetActorLocation() - PlayerLocation;
		const float BehindDistance = -FVector::DotProduct(ToTarget, Forward);
		if (BehindDistance < 120.0f)
		{
			continue;
		}

		const float Side = FMath::Clamp(FVector::DotProduct(ToTarget, Right) / 3200.0f, -1.0f, 1.0f);
		const float Depth = FMath::Clamp(BehindDistance / 7200.0f, 0.0f, 1.0f);
		const FVector2D DotPosition(
			InnerPosition.X + InnerSize.X * (0.5f - Side * 0.42f),
			InnerPosition.Y + InnerSize.Y * (0.75f - Depth * 0.48f));
		const float DotSize = FMath::Lerp(9.0f, 4.5f, Depth);
		DrawBox(AllottedGeometry, OutDrawElements, LayerId + 8, DotPosition - FVector2D(DotSize * 0.5f), FVector2D(DotSize), DotBrush, FLinearColor(1.0f, 0.18f, 0.04f, 0.92f));
		++BehindCount;
	}

	if (BehindCount > 0)
	{
		DrawTextGlow(AllottedGeometry, OutDrawElements, LayerId + 9, TEXT("THREAT"), LabelFont, Position + FVector2D(Width - 78.0f, Height - 24.0f), FLinearColor(1.0f, 0.26f, 0.1f, 0.95f), FLinearColor(1.0f, 0.0f, 0.0f, 0.28f), FVector2D(100.0f, 24.0f));
	}
	else
	{
		DrawTextGlow(AllottedGeometry, OutDrawElements, LayerId + 9, TEXT("CLEAR"), LabelFont, Position + FVector2D(Width - 70.0f, Height - 24.0f), FLinearColor(0.48f, 1.0f, 0.58f, 0.82f), FLinearColor(0.0f, 1.0f, 0.25f, 0.18f), FVector2D(80.0f, 24.0f));
	}

	const float HealthWidth = 78.0f;
	const FVector2D HealthPosition(Position.X + Width * 0.5f - HealthWidth * 0.5f, Position.Y + Height + 7.0f);
	DrawBox(AllottedGeometry, OutDrawElements, LayerId + 8, HealthPosition, FVector2D(HealthWidth, 8.0f), GlowBrush, FLinearColor(0.02f, 0.08f, 0.03f, 0.78f));
	DrawBox(AllottedGeometry, OutDrawElements, LayerId + 9, HealthPosition + FVector2D(2.0f), FVector2D(HealthWidth - 4.0f, 4.0f), GlowBrush, FLinearColor(0.24f, 1.0f, 0.28f, 0.92f));

	return LayerId + 10;
}

int32 UUBCombatHudWidget::DrawPositionPanel(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId, AUBCombatHud* Hud, APawn* PlayerPawn, const TArray<AUBPrototypeTargetCar*>& TargetCars, const TArray<FVector>& RaceWaypoints, const FVector2D& ViewSize) const
{
	TArray<FUBHudRaceEntry> Entries;
	Entries.Add({ PlayerPawn, Hud->ComputeRouteProgress(PlayerPawn, RaceWaypoints), true });
	for (AUBPrototypeTargetCar* TargetCar : TargetCars)
	{
		if (TargetCar)
		{
			Entries.Add({ TargetCar, Hud->ComputeRouteProgress(TargetCar, RaceWaypoints), false });
		}
	}

	const float PlayerProgress = Entries[0].Progress;
	const FVector PlayerLocation = PlayerPawn->GetActorLocation();
	const FVector PlayerForward = PlayerPawn->GetActorForwardVector().GetSafeNormal2D();
	int32 CarsAhead = 0;
	for (const AUBPrototypeTargetCar* TargetCar : TargetCars)
	{
		if (!TargetCar)
		{
			continue;
		}

		const float TargetProgress = Hud->ComputeRouteProgress(TargetCar, RaceWaypoints);
		const FVector ToTarget = TargetCar->GetActorLocation() - PlayerLocation;
		const float ForwardDistance = FVector::DotProduct(ToTarget, PlayerForward);
		const bool bAheadOnRoute = TargetProgress > PlayerProgress + 240.0f;
		const bool bAheadInSightLine = ForwardDistance > 220.0f && ToTarget.Size2D() < 18000.0f;
		if (bAheadOnRoute || bAheadInSightLine)
		{
			++CarsAhead;
		}
	}

	const int32 PlayerPosition = FMath::Clamp(CarsAhead + 1, 1, Entries.Num());
	const FVector2D Position(ViewSize.X * 0.032f, ViewSize.Y * 0.075f);
	const FVector2D Size(FMath::Clamp(ViewSize.X * 0.175f, 188.0f, 250.0f), 132.0f);
	DrawPanel(AllottedGeometry, OutDrawElements, LayerId, Position, Size, FLinearColor(0.01f, 0.045f, 0.064f, 0.66f), FLinearColor(0.08f, 0.72f, 1.0f, 0.72f));

	DrawTextGlow(AllottedGeometry, OutDrawElements, LayerId + 4, TEXT("POSITION"), LabelFont, Position + FVector2D(14.0f, 10.0f), FLinearColor(0.36f, 0.86f, 1.0f, 0.92f), FLinearColor(0.0f, 0.7f, 1.0f, 0.16f), FVector2D(160.0f, 24.0f));
	DrawTextGlow(AllottedGeometry, OutDrawElements, LayerId + 5, FString::Printf(TEXT("%d"), PlayerPosition), LargeFont, Position + FVector2D(14.0f, 30.0f), FLinearColor::White, FLinearColor(0.0f, 0.6f, 1.0f, 0.26f), FVector2D(90.0f, 58.0f));
	DrawTextGlow(AllottedGeometry, OutDrawElements, LayerId + 5, FString::Printf(TEXT("/%d"), Entries.Num()), MediumFont, Position + FVector2D(82.0f, 58.0f), FLinearColor(0.78f, 0.92f, 1.0f, 0.9f), FLinearColor(0.0f, 0.6f, 1.0f, 0.18f), FVector2D(70.0f, 28.0f));

	const int32 NextPosition = FMath::Max(1, PlayerPosition - 1);
	const FString NextText = PlayerPosition > 1 ? FString::Printf(TEXT("NEXT  %s"), *Hud->FormatOrdinal(NextPosition)) : TEXT("LEADING");
	DrawTextGlow(AllottedGeometry, OutDrawElements, LayerId + 5, NextText, LabelFont, Position + FVector2D(14.0f, 100.0f), FLinearColor(0.84f, 0.94f, 1.0f, 0.92f), FLinearColor(0.0f, 0.8f, 1.0f, 0.12f), FVector2D(150.0f, 24.0f));

	const int32 DotCount = FMath::Min(Entries.Num(), 8);
	for (int32 Index = 0; Index < DotCount; ++Index)
	{
		const float DotX = Position.X + 118.0f + static_cast<float>(Index % 4) * 18.0f;
		const float DotY = Position.Y + 26.0f + static_cast<float>(Index / 4) * 18.0f;
		const FLinearColor DotColor = Index == PlayerPosition - 1
			? FLinearColor(0.18f, 0.84f, 1.0f, 0.95f)
			: FLinearColor(1.0f, 0.24f, 0.06f, 0.82f);
		DrawBox(AllottedGeometry, OutDrawElements, LayerId + 6, FVector2D(DotX, DotY), FVector2D(10.0f), DotBrush, DotColor);
	}

	return LayerId + 8;
}

int32 UUBCombatHudWidget::DrawPowerSlots(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId, AUBCombatHud* Hud, APawn* PlayerPawn, const FVector2D& ViewSize) const
{
	const UUBPowerInventoryComponent* PowerComponent = PlayerPawn->FindComponentByClass<UUBPowerInventoryComponent>();
	const TArray<FUBPowerSlot>* Slots = PowerComponent ? &PowerComponent->GetPowerSlots() : nullptr;
	const int32 SelectedIndex = PowerComponent ? PowerComponent->GetSelectedSlotIndex() : 0;

	const float SlotSize = FMath::Clamp(ViewSize.Y * 0.086f, 56.0f, 78.0f);
	const float Gap = SlotSize * 0.22f;
	const float TotalWidth = SlotSize * 3.0f + Gap * 2.0f;
	const FVector2D TrackPosition(ViewSize.X * 0.5f - TotalWidth * 0.5f - 26.0f, ViewSize.Y - SlotSize - 44.0f);
	const FVector2D TrackSize(TotalWidth + 52.0f, SlotSize + 22.0f);
	DrawPanel(AllottedGeometry, OutDrawElements, LayerId, TrackPosition, TrackSize, FLinearColor(0.006f, 0.02f, 0.026f, 0.58f), FLinearColor(0.14f, 0.82f, 1.0f, 0.36f));
	DrawLine(AllottedGeometry, OutDrawElements, LayerId + 4, TrackPosition + FVector2D(22.0f, TrackSize.Y * 0.5f), TrackPosition + FVector2D(TrackSize.X - 22.0f, TrackSize.Y * 0.5f), FLinearColor(0.15f, 0.9f, 1.0f, 0.30f), 2.0f);

	const FVector2D BasePosition(TrackPosition.X + 26.0f, TrackPosition.Y + 11.0f);
	for (int32 Index = 0; Index < 3; ++Index)
	{
		const bool bHasSlot = Slots && Slots->IsValidIndex(Index) && (*Slots)[Index].PowerType != EUBPowerType::None;
		const bool bSelected = Index == SelectedIndex;
		const float VisualScale = bSelected ? 1.08f : 1.0f;
		const float ScaledSlotSize = SlotSize * VisualScale;
		const FVector2D SlotPosition = BasePosition + FVector2D(static_cast<float>(Index) * (SlotSize + Gap), 0.0f) - FVector2D((ScaledSlotSize - SlotSize) * 0.5f);
		const FUBPowerSlot PowerSlot = bHasSlot ? (*Slots)[Index] : FUBPowerSlot();
		FLinearColor PowerColor = bHasSlot ? UBPowerTypeLinearColor(PowerSlot.PowerType) : FLinearColor(0.36f, 0.62f, 0.78f, 1.0f);
		PowerColor.A = bSelected ? 0.96f : 0.58f;

		DrawBox(AllottedGeometry, OutDrawElements, LayerId + 5, SlotPosition + FVector2D(2.5f, 4.0f), FVector2D(ScaledSlotSize), SlotBrush, FLinearColor(0.0f, 0.0f, 0.0f, 0.38f));
		DrawBox(AllottedGeometry, OutDrawElements, LayerId + 6, SlotPosition, FVector2D(ScaledSlotSize), SlotBrush, FLinearColor(0.014f, 0.032f, 0.04f, bSelected ? 0.86f : 0.62f));
		DrawLine(AllottedGeometry, OutDrawElements, LayerId + 7, SlotPosition + FVector2D(10.0f, 0.0f), SlotPosition + FVector2D(ScaledSlotSize - 10.0f, 0.0f), PowerColor, bSelected ? 3.0f : 1.6f);
		DrawLine(AllottedGeometry, OutDrawElements, LayerId + 7, SlotPosition + FVector2D(ScaledSlotSize - 1.0f, 10.0f), SlotPosition + FVector2D(ScaledSlotSize - 1.0f, ScaledSlotSize - 10.0f), WithAlpha(PowerColor, PowerColor.A * 0.58f), 1.4f);

		if (bHasSlot)
		{
			if (FSlateBrush* IconBrush = GetPowerIconBrush(Hud, PowerSlot.PowerType, PowerSlot.bIsSuper))
			{
				DrawBox(AllottedGeometry, OutDrawElements, LayerId + 8, SlotPosition + FVector2D(9.0f), FVector2D(ScaledSlotSize - 18.0f), *IconBrush, FLinearColor::White);
			}

			if (PowerSlot.bIsSuper)
			{
				DrawLine(AllottedGeometry, OutDrawElements, LayerId + 9, SlotPosition + FVector2D(8.0f, 8.0f), SlotPosition + FVector2D(ScaledSlotSize - 8.0f, 8.0f), FLinearColor(1.0f, 0.82f, 0.1f, 1.0f), 3.0f);
				DrawLine(AllottedGeometry, OutDrawElements, LayerId + 9, SlotPosition + FVector2D(ScaledSlotSize - 8.0f, 8.0f), SlotPosition + FVector2D(ScaledSlotSize - 8.0f, ScaledSlotSize - 8.0f), FLinearColor(1.0f, 0.82f, 0.1f, 0.92f), 3.0f);
			}
		}
		else
		{
			const FVector2D Mid = SlotPosition + FVector2D(ScaledSlotSize * 0.5f);
			DrawLine(AllottedGeometry, OutDrawElements, LayerId + 8, Mid - FVector2D(ScaledSlotSize * 0.22f, 0.0f), Mid + FVector2D(ScaledSlotSize * 0.22f, 0.0f), FLinearColor(0.62f, 0.9f, 1.0f, 0.30f), 2.0f);
			DrawLine(AllottedGeometry, OutDrawElements, LayerId + 8, Mid - FVector2D(0.0f, ScaledSlotSize * 0.22f), Mid + FVector2D(0.0f, ScaledSlotSize * 0.22f), FLinearColor(0.62f, 0.9f, 1.0f, 0.18f), 1.5f);
		}
	}

	return LayerId + 10;
}

int32 UUBCombatHudWidget::DrawMiniMap(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId, AUBCombatHud* Hud, APawn* PlayerPawn, const TArray<AUBPrototypeTargetCar*>& TargetCars, const TArray<FVector>& RaceWaypoints, const FVector2D& ViewSize) const
{
	const float Radius = FMath::Clamp(ViewSize.Y * 0.128f, 74.0f, 110.0f);
	const FVector2D Center(ViewSize.X - Radius - ViewSize.X * 0.052f, ViewSize.Y * 0.165f);
	const FVector2D PanelPosition(Center.X - Radius * 1.12f, Center.Y - Radius * 1.12f);
	const FVector2D PanelSize(Radius * 2.24f);
	DrawPanel(AllottedGeometry, OutDrawElements, LayerId, PanelPosition, PanelSize, FLinearColor(0.006f, 0.034f, 0.062f, 0.62f), FLinearColor(0.1f, 0.68f, 1.0f, 0.78f));

	DrawBox(AllottedGeometry, OutDrawElements, LayerId + 4, Center - FVector2D(Radius), FVector2D(Radius * 2.0f), DotBrush, FLinearColor(0.0f, 0.04f, 0.075f, 0.36f));
	DrawLine(AllottedGeometry, OutDrawElements, LayerId + 5, Center - FVector2D(Radius, 0.0f), Center + FVector2D(Radius, 0.0f), FLinearColor(0.2f, 0.75f, 1.0f, 0.16f), 1.0f);
	DrawLine(AllottedGeometry, OutDrawElements, LayerId + 5, Center - FVector2D(0.0f, Radius), Center + FVector2D(0.0f, Radius), FLinearColor(0.2f, 0.75f, 1.0f, 0.16f), 1.0f);

	if (RaceWaypoints.Num() > 1)
	{
		for (int32 Index = 0; Index < RaceWaypoints.Num(); ++Index)
		{
			const FVector2D A = Hud->ProjectToMiniMap(RaceWaypoints[Index], RaceWaypoints, Center, Radius * 0.74f);
			const FVector2D B = Hud->ProjectToMiniMap(RaceWaypoints[(Index + 1) % RaceWaypoints.Num()], RaceWaypoints, Center, Radius * 0.74f);
			DrawLine(AllottedGeometry, OutDrawElements, LayerId + 6, A, B, FLinearColor(0.0f, 0.0f, 0.0f, 0.55f), 5.0f);
			DrawLine(AllottedGeometry, OutDrawElements, LayerId + 7, A, B, FLinearColor(0.27f, 0.94f, 1.0f, 0.74f), 2.2f);
		}
	}

	for (const AUBPrototypeTargetCar* TargetCar : TargetCars)
	{
		if (!TargetCar)
		{
			continue;
		}

		const FVector2D TargetDot = Hud->ProjectToMiniMap(TargetCar->GetActorLocation(), RaceWaypoints, Center, Radius * 0.74f);
		DrawBox(AllottedGeometry, OutDrawElements, LayerId + 8, TargetDot - FVector2D(4.0f), FVector2D(8.0f), DotBrush, FLinearColor(1.0f, 0.24f, 0.04f, 0.95f));
	}

	const FVector2D PlayerDot = Hud->ProjectToMiniMap(PlayerPawn->GetActorLocation(), RaceWaypoints, Center, Radius * 0.74f);
	DrawMiniTriangle(AllottedGeometry, OutDrawElements, LayerId + 9, PlayerDot, 9.0f, PlayerPawn->GetActorRotation().Yaw, FLinearColor(0.14f, 0.86f, 1.0f, 1.0f));

	const FVector2D LapPosition(PanelPosition.X + PanelSize.X - 58.0f, PanelPosition.Y + 14.0f);
	DrawBox(AllottedGeometry, OutDrawElements, LayerId + 8, LapPosition, FVector2D(44.0f, 34.0f), SoftBrush, FLinearColor(0.0f, 0.02f, 0.03f, 0.50f));
	DrawTextGlow(AllottedGeometry, OutDrawElements, LayerId + 9, TEXT("LAP"), TinyFont, LapPosition + FVector2D(8.0f, 2.0f), FLinearColor(0.62f, 0.92f, 1.0f, 0.82f), FLinearColor(0.0f, 0.8f, 1.0f, 0.12f), FVector2D(42.0f, 16.0f));
	DrawTextGlow(AllottedGeometry, OutDrawElements, LayerId + 9, TEXT("1/3"), LabelFont, LapPosition + FVector2D(8.0f, 15.0f), FLinearColor::White, FLinearColor(0.0f, 0.8f, 1.0f, 0.18f), FVector2D(42.0f, 20.0f));

	return LayerId + 10;
}

int32 UUBCombatHudWidget::DrawSpeedCluster(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId, AUBCombatHud* Hud, APawn* PlayerPawn, const FVector2D& ViewSize) const
{
	const float SpeedKmh = Hud->GetActorSpeedKmh(PlayerPawn);
	const FVector2D Size(206.0f, 78.0f);
	const FVector2D Position(ViewSize.X - Size.X - ViewSize.X * 0.045f, ViewSize.Y - Size.Y - 42.0f);
	DrawPanel(AllottedGeometry, OutDrawElements, LayerId, Position, Size, FLinearColor(0.006f, 0.032f, 0.052f, 0.64f), FLinearColor(0.05f, 0.7f, 1.0f, 0.56f));

	const float SpeedFraction = FMath::Clamp(SpeedKmh / 260.0f, 0.0f, 1.0f);
	const FVector2D BarPosition = Position + FVector2D(18.0f, Size.Y - 18.0f);
	const float BarWidth = Size.X - 36.0f;
	DrawLine(AllottedGeometry, OutDrawElements, LayerId + 4, BarPosition, BarPosition + FVector2D(BarWidth, 0.0f), FLinearColor(0.18f, 0.35f, 0.45f, 0.58f), 5.0f);
	DrawLine(AllottedGeometry, OutDrawElements, LayerId + 5, BarPosition, BarPosition + FVector2D(BarWidth * SpeedFraction, 0.0f), FLinearColor(1.0f, 0.48f, 0.08f, 0.95f), 5.0f);
	for (int32 Tick = 0; Tick <= 5; ++Tick)
	{
		const float X = BarPosition.X + BarWidth * (static_cast<float>(Tick) / 5.0f);
		DrawLine(AllottedGeometry, OutDrawElements, LayerId + 6, FVector2D(X, BarPosition.Y - 9.0f), FVector2D(X, BarPosition.Y - 2.0f), FLinearColor(0.78f, 0.95f, 1.0f, 0.44f), 1.0f);
	}

	DrawTextGlow(AllottedGeometry, OutDrawElements, LayerId + 6, FString::Printf(TEXT("%03.0f"), SpeedKmh), SpeedFont, Position + FVector2D(18.0f, 8.0f), FLinearColor::White, FLinearColor(0.0f, 0.58f, 1.0f, 0.28f), FVector2D(110.0f, 52.0f));
	DrawTextGlow(AllottedGeometry, OutDrawElements, LayerId + 6, TEXT("KM/H"), LabelFont, Position + FVector2D(128.0f, 36.0f), FLinearColor(0.58f, 0.9f, 1.0f, 0.84f), FLinearColor(0.0f, 0.6f, 1.0f, 0.12f), FVector2D(70.0f, 22.0f));
	const FString Gear = SpeedKmh < 2.0f ? TEXT("N") : TEXT("D");
	DrawBox(AllottedGeometry, OutDrawElements, LayerId + 5, Position + FVector2D(146.0f, 10.0f), FVector2D(32.0f, 24.0f), SoftBrush, FLinearColor(0.0f, 0.09f, 0.12f, 0.72f));
	DrawTextGlow(AllottedGeometry, OutDrawElements, LayerId + 6, Gear, MediumFont, Position + FVector2D(154.0f, 8.0f), FLinearColor(0.22f, 0.92f, 1.0f, 0.96f), FLinearColor(0.0f, 0.7f, 1.0f, 0.24f), FVector2D(30.0f, 28.0f));

	return LayerId + 8;
}

FSlateBrush* UUBCombatHudWidget::GetPowerIconBrush(AUBCombatHud* Hud, EUBPowerType PowerType, bool bIsSuper) const
{
	if (!Hud || PowerType == EUBPowerType::None)
	{
		return nullptr;
	}

	const int32 CacheKey = static_cast<int32>(PowerType) + (bIsSuper ? 1000 : 0);
	if (FSlateBrush* CachedBrush = PowerIconBrushCache.Find(CacheKey))
	{
		return CachedBrush;
	}

	UTexture2D* IconTexture = Hud->GetCachedPowerIcon(PowerType, bIsSuper);
	if (!IconTexture)
	{
		return nullptr;
	}

	FSlateBrush Brush;
	Brush.DrawAs = ESlateBrushDrawType::Image;
	Brush.Tiling = ESlateBrushTileType::NoTile;
	Brush.Mirroring = ESlateBrushMirrorType::NoMirror;
	Brush.ImageSize = FVector2D(1024.0f, 1024.0f);
	Brush.SetResourceObject(IconTexture);
	PowerIconBrushCache.Add(CacheKey, Brush);
	return PowerIconBrushCache.Find(CacheKey);
}
