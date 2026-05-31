#include "UI/UBCombatHud.h"

#include "Components/PrimitiveComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Powers/UBPowerInventoryComponent.h"
#include "Powers/UBPowerTypes.h"
#include "Powers/UBPowerVisuals.h"
#include "Prototype/UBPowerPrototypeSubsystem.h"
#include "Prototype/UBPrototypeTargetCar.h"
#include "Vehicle/UBVehicleHealthComponent.h"

AUBCombatHud::AUBCombatHud()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AUBCombatHud::BeginPlay()
{
	Super::BeginPlay();

	RearViewRenderTarget = NewObject<UTextureRenderTarget2D>(this, TEXT("UBRearViewRenderTarget"));
	if (RearViewRenderTarget)
	{
		RearViewRenderTarget->RenderTargetFormat = RTF_RGBA8;
		RearViewRenderTarget->ClearColor = FLinearColor::Black;
		RearViewRenderTarget->InitAutoFormat(768, 192);
		RearViewRenderTarget->UpdateResourceImmediate(true);
	}

	RearViewCapture = NewObject<USceneCaptureComponent2D>(this, TEXT("UBRearViewCapture"));
	if (RearViewCapture)
	{
		RearViewCapture->RegisterComponent();
		RearViewCapture->TextureTarget = RearViewRenderTarget;
		RearViewCapture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
		RearViewCapture->FOVAngle = 74.0f;
		RearViewCapture->bCaptureEveryFrame = false;
		RearViewCapture->bCaptureOnMovement = false;
		RearViewCapture->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_RenderScenePrimitives;
	}
}

void AUBCombatHud::DrawHUD()
{
	Super::DrawHUD();

	if (!Canvas || !PlayerOwner)
	{
		return;
	}

	APawn* PlayerPawn = PlayerOwner->GetPawn();
	if (!PlayerPawn)
	{
		return;
	}

	TArray<AUBPrototypeTargetCar*> TargetCars;
	GatherTargetCars(TargetCars);

	TArray<FVector> RaceWaypoints;
	if (const UWorld* World = GetWorld())
	{
		if (const UUBPowerPrototypeSubsystem* PrototypeSubsystem = World->GetSubsystem<UUBPowerPrototypeSubsystem>())
		{
			RaceWaypoints = PrototypeSubsystem->GetPrototypeRaceWaypoints();
		}
	}

	DrawRearViewMirror(PlayerPawn, TargetCars);
	DrawPositionPanel(PlayerPawn, TargetCars, RaceWaypoints);
	DrawPowerSlots(PlayerPawn);
	DrawMiniMap(PlayerPawn, TargetCars, RaceWaypoints);
	DrawSpeedCluster(PlayerPawn);
}

void AUBCombatHud::DrawRearViewMirror(APawn* PlayerPawn, const TArray<AUBPrototypeTargetCar*>& TargetCars)
{
	if (!Canvas || !PlayerPawn)
	{
		return;
	}

	const float Width = FMath::Clamp(Canvas->SizeX * 0.43f, 360.0f, 620.0f);
	const float Height = FMath::Clamp(Canvas->SizeY * 0.105f, 58.0f, 92.0f);
	const float X = (Canvas->SizeX - Width) * 0.5f;
	const float Y = Canvas->SizeY * 0.045f;

	DrawGlassPanel(X, Y, Width, Height, FLinearColor(0.018f, 0.06f, 0.08f, 0.72f), FLinearColor(0.12f, 0.85f, 1.0f, 0.74f), 2.0f);
	UpdateRearViewCapture(PlayerPawn);
	if (RearViewRenderTarget)
	{
		DrawTextureFit(RearViewRenderTarget, X + 12.0f, Y + 8.0f, Width - 24.0f, Height - 16.0f, FLinearColor(0.88f, 1.0f, 1.0f, 0.92f));
		DrawRect(FLinearColor(0.0f, 0.08f, 0.12f, 0.18f), X + 12.0f, Y + 8.0f, Width - 24.0f, Height - 16.0f);
	}
	else
	{
		DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, 0.36f), X + 12.0f, Y + 8.0f, Width - 24.0f, Height - 16.0f);
	}

	DrawLine(X + 22.0f, Y + Height * 0.58f, X + Width - 22.0f, Y + Height * 0.49f, FLinearColor(0.36f, 0.9f, 0.95f, 0.24f), 1.0f);
	DrawLine(X + 36.0f, Y + Height * 0.74f, X + Width - 42.0f, Y + Height * 0.67f, FLinearColor(0.0f, 0.9f, 1.0f, 0.18f), 1.0f);

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
		const float DotX = X + Width * (0.5f - Side * 0.42f);
		const float DotY = Y + Height * (0.72f - Depth * 0.42f);
		const float DotSize = FMath::Lerp(9.0f, 4.0f, Depth);
		DrawRect(FLinearColor(1.0f, 0.18f, 0.05f, 0.85f), DotX - DotSize * 0.5f, DotY - DotSize * 0.5f, DotSize, DotSize);
		++BehindCount;
	}

	if (BehindCount == 0)
	{
		DrawText(TEXT("CLEAR"), FLinearColor(0.48f, 1.0f, 0.58f, 0.75f), X + Width * 0.48f, Y + Height - 20.0f, GEngine ? GEngine->GetSmallFont() : nullptr, 0.78f, false);
	}

	const float HealthWidth = 72.0f;
	DrawSoftBar(X + Width * 0.5f - HealthWidth * 0.5f, Y + Height + 7.0f, HealthWidth, 8.0f, 1.0f, FLinearColor(0.24f, 1.0f, 0.28f, 0.9f), FLinearColor(0.02f, 0.08f, 0.03f, 0.78f));
}

void AUBCombatHud::DrawPositionPanel(APawn* PlayerPawn, const TArray<AUBPrototypeTargetCar*>& TargetCars, const TArray<FVector>& RaceWaypoints)
{
	if (!Canvas || !PlayerPawn)
	{
		return;
	}

	TArray<FUBRaceEntry> Entries;
	Entries.Add({ PlayerPawn, ComputeRouteProgress(PlayerPawn, RaceWaypoints), true });
	for (AUBPrototypeTargetCar* TargetCar : TargetCars)
	{
		if (TargetCar)
		{
			Entries.Add({ TargetCar, ComputeRouteProgress(TargetCar, RaceWaypoints), false });
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

		const float TargetProgress = ComputeRouteProgress(TargetCar, RaceWaypoints);
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

	const float X = Canvas->SizeX * 0.035f;
	const float Y = Canvas->SizeY * 0.07f;
	const float Width = FMath::Clamp(Canvas->SizeX * 0.17f, 172.0f, 230.0f);
	const float Height = 132.0f;
	DrawGlassPanel(X, Y, Width, Height, FLinearColor(0.02f, 0.09f, 0.14f, 0.68f), FLinearColor(0.0f, 0.6f, 1.0f, 0.68f), 2.0f);

	UFont* MediumFont = GEngine ? GEngine->GetMediumFont() : nullptr;
	UFont* SmallFont = GEngine ? GEngine->GetSmallFont() : nullptr;
	DrawText(TEXT("POSITION"), FLinearColor(0.34f, 0.84f, 1.0f, 0.92f), X + 14.0f, Y + 10.0f, SmallFont, 0.78f, false);
	DrawText(FString::Printf(TEXT("%d"), PlayerPosition), FLinearColor::White, X + 14.0f, Y + 30.0f, MediumFont, 2.0f, false);
	DrawText(FString::Printf(TEXT("/ %d"), Entries.Num()), FLinearColor(0.78f, 0.92f, 1.0f, 0.85f), X + 76.0f, Y + 55.0f, SmallFont, 0.92f, false);

	const int32 NextPosition = FMath::Max(1, PlayerPosition - 1);
	const FString NextText = PlayerPosition > 1 ? FString::Printf(TEXT("NEXT AT  %s"), *FormatOrdinal(NextPosition)) : TEXT("LEADING");
	DrawText(NextText, FLinearColor(0.84f, 0.94f, 1.0f, 0.92f), X + 14.0f, Y + 92.0f, SmallFont, 0.84f, false);

	for (int32 Index = 0; Index < FMath::Min(Entries.Num(), 7); ++Index)
	{
		const float DotX = X + 16.0f + static_cast<float>(Index) * 18.0f;
		const FLinearColor DotColor = Index == PlayerPosition - 1
			? FLinearColor(0.2f, 0.85f, 1.0f, 0.95f)
			: FLinearColor(1.0f, 0.28f, 0.08f, 0.8f);
		DrawRect(DotColor, DotX, Y + 78.0f, 10.0f, 10.0f);
	}
}

void AUBCombatHud::DrawPowerSlots(APawn* PlayerPawn)
{
	if (!Canvas || !PlayerPawn)
	{
		return;
	}

	const UUBPowerInventoryComponent* PowerComponent = PlayerPawn->FindComponentByClass<UUBPowerInventoryComponent>();
	const TArray<FUBPowerSlot>* Slots = PowerComponent ? &PowerComponent->GetPowerSlots() : nullptr;
	const int32 SelectedIndex = PowerComponent ? PowerComponent->GetSelectedSlotIndex() : 0;

	const float SlotSize = FMath::Clamp(Canvas->SizeY * 0.078f, 48.0f, 70.0f);
	const float Gap = 12.0f;
	const float TotalWidth = SlotSize * 3.0f + Gap * 2.0f;
	const float X = Canvas->SizeX * 0.5f - TotalWidth * 0.5f;
	const float Y = Canvas->SizeY - SlotSize - 34.0f;

	for (int32 Index = 0; Index < 3; ++Index)
	{
		const bool bHasSlot = Slots && Slots->IsValidIndex(Index);
		const bool bSelected = bHasSlot && Index == SelectedIndex;
		const float SlotX = X + static_cast<float>(Index) * (SlotSize + Gap);
		FLinearColor BorderColor = bSelected ? FLinearColor(0.0f, 0.88f, 1.0f, 0.95f) : FLinearColor(0.32f, 0.55f, 0.68f, 0.48f);
		if (bHasSlot)
		{
			BorderColor = bSelected
				? UBPowerTypeLinearColor((*Slots)[Index].PowerType)
				: UBPowerTypeLinearColor((*Slots)[Index].PowerType) * 0.48f;
			BorderColor.A = bSelected ? 0.96f : 0.58f;
		}
		DrawGlassPanel(SlotX, Y, SlotSize, SlotSize, FLinearColor(0.02f, 0.045f, 0.06f, bSelected ? 0.78f : 0.52f), BorderColor, bSelected ? 3.0f : 1.5f);

		if (bHasSlot)
		{
			const FUBPowerSlot& Slot = (*Slots)[Index];
			if (Slot.PowerType != EUBPowerType::None)
			{
				UTexture2D* Icon = GetCachedPowerIcon(Slot.PowerType, Slot.bIsSuper);
				DrawTextureFit(Icon, SlotX + 8.0f, Y + 8.0f, SlotSize - 16.0f, SlotSize - 16.0f, FLinearColor::White);
				if (Slot.bIsSuper)
				{
					DrawLine(SlotX + 8.0f, Y + 8.0f, SlotX + SlotSize - 8.0f, Y + 8.0f, FLinearColor(1.0f, 0.86f, 0.12f, 0.95f), 3.0f);
					DrawLine(SlotX + SlotSize - 8.0f, Y + 8.0f, SlotX + SlotSize - 8.0f, Y + SlotSize - 8.0f, FLinearColor(1.0f, 0.86f, 0.12f, 0.95f), 3.0f);
				}
			}
		}
		else
		{
			DrawLine(SlotX + 16.0f, Y + SlotSize * 0.5f, SlotX + SlotSize - 16.0f, Y + SlotSize * 0.5f, FLinearColor(0.58f, 0.82f, 0.95f, 0.28f), 2.0f);
		}
	}
}

void AUBCombatHud::DrawMiniMap(APawn* PlayerPawn, const TArray<AUBPrototypeTargetCar*>& TargetCars, const TArray<FVector>& RaceWaypoints)
{
	if (!Canvas || !PlayerPawn)
	{
		return;
	}

	const float Radius = FMath::Clamp(Canvas->SizeY * 0.115f, 68.0f, 98.0f);
	const FVector2D Center(Canvas->SizeX - Radius - Canvas->SizeX * 0.05f, Canvas->SizeY * 0.15f);
	const float PanelSize = Radius * 2.2f;
	DrawGlassPanel(Center.X - PanelSize * 0.5f, Center.Y - PanelSize * 0.5f, PanelSize, PanelSize, FLinearColor(0.01f, 0.055f, 0.11f, 0.58f), FLinearColor(0.0f, 0.5f, 1.0f, 0.74f), 2.0f);

	DrawLine(Center.X - Radius, Center.Y, Center.X + Radius, Center.Y, FLinearColor(0.18f, 0.72f, 1.0f, 0.18f), 1.0f);
	DrawLine(Center.X, Center.Y - Radius, Center.X, Center.Y + Radius, FLinearColor(0.18f, 0.72f, 1.0f, 0.18f), 1.0f);

	if (RaceWaypoints.Num() > 1)
	{
		for (int32 Index = 0; Index < RaceWaypoints.Num(); ++Index)
		{
			const FVector2D A = ProjectToMiniMap(RaceWaypoints[Index], RaceWaypoints, Center, Radius * 0.78f);
			const FVector2D B = ProjectToMiniMap(RaceWaypoints[(Index + 1) % RaceWaypoints.Num()], RaceWaypoints, Center, Radius * 0.78f);
			DrawLine(A.X, A.Y, B.X, B.Y, FLinearColor(0.28f, 0.92f, 1.0f, 0.62f), 2.5f);
		}
	}

	const FVector2D PlayerDot = ProjectToMiniMap(PlayerPawn->GetActorLocation(), RaceWaypoints, Center, Radius * 0.78f);
	DrawRect(FLinearColor(0.06f, 0.72f, 1.0f, 1.0f), PlayerDot.X - 5.0f, PlayerDot.Y - 5.0f, 10.0f, 10.0f);

	for (const AUBPrototypeTargetCar* TargetCar : TargetCars)
	{
		if (!TargetCar)
		{
			continue;
		}

		const FVector2D TargetDot = ProjectToMiniMap(TargetCar->GetActorLocation(), RaceWaypoints, Center, Radius * 0.78f);
		DrawRect(FLinearColor(1.0f, 0.24f, 0.04f, 0.9f), TargetDot.X - 3.0f, TargetDot.Y - 3.0f, 6.0f, 6.0f);
	}

	DrawText(TEXT("LAP"), FLinearColor(0.64f, 0.92f, 1.0f, 0.82f), Center.X + Radius * 0.55f, Center.Y - Radius * 0.72f, GEngine ? GEngine->GetSmallFont() : nullptr, 0.72f, false);
	DrawText(TEXT("1/3"), FLinearColor::White, Center.X + Radius * 0.54f, Center.Y - Radius * 0.5f, GEngine ? GEngine->GetSmallFont() : nullptr, 1.05f, false);
}

void AUBCombatHud::DrawSpeedCluster(APawn* PlayerPawn)
{
	if (!Canvas || !PlayerPawn)
	{
		return;
	}

	const float SpeedKmh = GetActorSpeedKmh(PlayerPawn);
	const float Width = 156.0f;
	const float Height = 48.0f;
	const float X = Canvas->SizeX - Width - Canvas->SizeX * 0.045f;
	const float Y = Canvas->SizeY * 0.315f;
	DrawGlassPanel(X, Y, Width, Height, FLinearColor(0.02f, 0.08f, 0.13f, 0.62f), FLinearColor(0.0f, 0.55f, 1.0f, 0.5f), 1.5f);
	DrawText(FString::Printf(TEXT("%03.0f"), SpeedKmh), FLinearColor::White, X + 22.0f, Y + 3.0f, GEngine ? GEngine->GetMediumFont() : nullptr, 1.15f, false);
	DrawText(TEXT("KM/H"), FLinearColor(0.58f, 0.88f, 1.0f, 0.78f), X + 96.0f, Y + 21.0f, GEngine ? GEngine->GetSmallFont() : nullptr, 0.62f, false);
	const FString Gear = SpeedKmh < 2.0f ? TEXT("N") : TEXT("D");
	DrawText(Gear, FLinearColor(0.22f, 0.92f, 1.0f, 0.92f), X + 96.0f, Y + 4.0f, GEngine ? GEngine->GetMediumFont() : nullptr, 0.9f, false);
}

void AUBCombatHud::DrawGlassPanel(float X, float Y, float Width, float Height, const FLinearColor& FillColor, const FLinearColor& BorderColor, float BorderThickness)
{
	DrawRect(FillColor, X, Y, Width, Height);
	DrawRect(FLinearColor(1.0f, 1.0f, 1.0f, 0.08f), X + 2.0f, Y + 2.0f, Width - 4.0f, Height * 0.28f);
	DrawLine(X, Y, X + Width, Y, BorderColor, BorderThickness);
	DrawLine(X + Width, Y, X + Width, Y + Height, BorderColor, BorderThickness);
	DrawLine(X + Width, Y + Height, X, Y + Height, BorderColor * 0.65f, BorderThickness);
	DrawLine(X, Y + Height, X, Y, BorderColor * 0.65f, BorderThickness);
}

void AUBCombatHud::DrawSoftBar(float X, float Y, float Width, float Height, float FillAlpha, const FLinearColor& FillColor, const FLinearColor& BackColor)
{
	DrawRect(BackColor, X, Y, Width, Height);
	DrawRect(FillColor, X + 2.0f, Y + 2.0f, (Width - 4.0f) * FMath::Clamp(FillAlpha, 0.0f, 1.0f), Height - 4.0f);
}

void AUBCombatHud::DrawTextureFit(UTexture2D* Texture, float X, float Y, float Width, float Height, const FLinearColor& TintColor)
{
	DrawTextureFit(static_cast<UTexture*>(Texture), X, Y, Width, Height, TintColor);
}

void AUBCombatHud::DrawTextureFit(UTexture* Texture, float X, float Y, float Width, float Height, const FLinearColor& TintColor)
{
	if (!Texture)
	{
		DrawRect(FLinearColor(0.16f, 0.4f, 0.55f, 0.35f), X, Y, Width, Height);
		return;
	}

	DrawTexture(Texture, X, Y, Width, Height, 0.0f, 0.0f, 1.0f, 1.0f, TintColor, BLEND_Translucent);
}

void AUBCombatHud::GatherTargetCars(TArray<AUBPrototypeTargetCar*>& OutTargetCars) const
{
	OutTargetCars.Reset();
	if (!GetWorld())
	{
		return;
	}

	for (TActorIterator<AUBPrototypeTargetCar> It(GetWorld()); It; ++It)
	{
		AUBPrototypeTargetCar* TargetCar = *It;
		if (!TargetCar)
		{
			continue;
		}

		const UUBVehicleHealthComponent* HealthComponent = TargetCar->FindComponentByClass<UUBVehicleHealthComponent>();
		if (HealthComponent && HealthComponent->IsDestroyed())
		{
			continue;
		}

		OutTargetCars.Add(TargetCar);
	}
}

float AUBCombatHud::GetActorSpeedKmh(const AActor* Actor) const
{
	UPrimitiveComponent* Primitive = Actor ? Cast<UPrimitiveComponent>(Actor->GetRootComponent()) : nullptr;
	const FVector Velocity = Primitive ? Primitive->GetPhysicsLinearVelocity() : (Actor ? Actor->GetVelocity() : FVector::ZeroVector);
	return Velocity.Size2D() * 0.036f;
}

float AUBCombatHud::ComputeRouteProgress(const AActor* Actor, const TArray<FVector>& RaceWaypoints) const
{
	if (!Actor || RaceWaypoints.Num() < 2)
	{
		return 0.0f;
	}

	float BestDistanceSquared = TNumericLimits<float>::Max();
	float BestProgress = 0.0f;
	float AccumulatedDistance = 0.0f;
	const FVector Location = Actor->GetActorLocation();

	for (int32 Index = 0; Index < RaceWaypoints.Num(); ++Index)
	{
		const FVector SegmentStart = RaceWaypoints[Index];
		const FVector SegmentEnd = RaceWaypoints[(Index + 1) % RaceWaypoints.Num()];
		const FVector Segment = SegmentEnd - SegmentStart;
		const float SegmentLengthSquared = Segment.SizeSquared2D();
		const float SegmentLength = FMath::Sqrt(SegmentLengthSquared);
		if (SegmentLength <= KINDA_SMALL_NUMBER)
		{
			continue;
		}

		const float Alpha = FMath::Clamp(FVector::DotProduct(Location - SegmentStart, Segment) / SegmentLengthSquared, 0.0f, 1.0f);
		const FVector Projected = SegmentStart + Segment * Alpha;
		const float DistanceSquared = FVector::DistSquared2D(Location, Projected);
		if (DistanceSquared < BestDistanceSquared)
		{
			BestDistanceSquared = DistanceSquared;
			BestProgress = AccumulatedDistance + SegmentLength * Alpha;
		}

		AccumulatedDistance += SegmentLength;
	}

	return BestProgress;
}

FVector2D AUBCombatHud::ProjectToMiniMap(const FVector& WorldLocation, const TArray<FVector>& RaceWaypoints, const FVector2D& Center, float Radius) const
{
	if (RaceWaypoints.Num() < 2)
	{
		return Center;
	}

	FBox Bounds(EForceInit::ForceInit);
	for (const FVector& Waypoint : RaceWaypoints)
	{
		Bounds += Waypoint;
	}

	const FVector BoundsCenter = Bounds.GetCenter();
	const FVector BoundsExtent = Bounds.GetExtent();
	const float MaxExtent = FMath::Max(FMath::Max(static_cast<float>(BoundsExtent.X), static_cast<float>(BoundsExtent.Y)), 1.0f);
	const FVector Relative = WorldLocation - BoundsCenter;
	return FVector2D(
		Center.X + FMath::Clamp(Relative.X / MaxExtent, -1.0f, 1.0f) * Radius,
		Center.Y + FMath::Clamp(Relative.Y / MaxExtent, -1.0f, 1.0f) * Radius);
}

FString AUBCombatHud::FormatOrdinal(int32 Position) const
{
	switch (Position)
	{
	case 1:
		return TEXT("1st");
	case 2:
		return TEXT("2nd");
	case 3:
		return TEXT("3rd");
	default:
		return FString::Printf(TEXT("%dth"), Position);
	}
}

UTexture2D* AUBCombatHud::GetCachedPowerIcon(EUBPowerType PowerType, bool bIsSuper)
{
	const int32 CacheKey = static_cast<int32>(PowerType) + (bIsSuper ? 1000 : 0);
	if (TObjectPtr<UTexture2D>* CachedTexture = PowerIconCache.Find(CacheKey))
	{
		return CachedTexture->Get();
	}

	UTexture2D* LoadedTexture = UBLoadPowerIcon(PowerType, bIsSuper);
	PowerIconCache.Add(CacheKey, LoadedTexture);
	return LoadedTexture;
}

void AUBCombatHud::UpdateRearViewCapture(APawn* PlayerPawn)
{
	if (!PlayerPawn || !RearViewCapture || !RearViewRenderTarget)
	{
		return;
	}

	const FVector Forward = PlayerPawn->GetActorForwardVector().GetSafeNormal();
	const FVector CaptureLocation = PlayerPawn->GetActorLocation() + FVector::UpVector * 190.0f + Forward * 120.0f;
	const FRotator CaptureRotation = (-Forward).Rotation();
	RearViewCapture->SetWorldLocation(CaptureLocation);
	RearViewCapture->SetWorldRotation(FRotator(-4.0f, CaptureRotation.Yaw, 0.0f));
	RearViewCapture->CaptureScene();
}
