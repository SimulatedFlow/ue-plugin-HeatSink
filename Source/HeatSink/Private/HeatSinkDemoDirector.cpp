// Copyright 2026 Silvan Teufel. All Rights Reserved.

#include "HeatSinkDemoDirector.h"

#include "Components/TextRenderComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "HeatSinkStatics.h"

namespace HeatSinkDemoLocal
{
	/**
	 * Horizontal is X - the camera looks along +Y, so Y is depth.
	 *
	 * The camera looks along +Y with +Z up, which puts world -X on the RIGHT of the picture, so the
	 * held-trigger column sits at POSITIVE X to land on the left of the frame where the text reads
	 * it first.
	 *
	 * Both columns stand outside the two text blocks (x = +-480, about 900 wide), or the bars draw
	 * straight through the numbers.
	 */
	constexpr float SpalteX[2] = {1050.0f, -1050.0f};
	constexpr float FloorY = 300.0f;

	const FColor Heiss(232, 104, 82);
	const FColor Kuehl(110, 200, 235);
	const FColor Rahmen(58, 60, 70);
	const FColor Grenze(240, 200, 110);
	const FColor Sperre(226, 78, 74);
	const FColor Klemmt(238, 150, 64);
	const FColor Ventil(120, 220, 170);
	const FColor Text(212, 218, 232);

	static FColor FarbeFuer(const EHeatShotOutcome Aus)
	{
		switch (Aus)
		{
		case EHeatShotOutcome::Fired:              return Kuehl;
		case EHeatShotOutcome::FiredAndOverheated: return Grenze;
		case EHeatShotOutcome::FiredAndJammed:     return Klemmt;
		case EHeatShotOutcome::BlockedOverheated:  return Sperre;
		case EHeatShotOutcome::BlockedJammed:      return Klemmt;
		case EHeatShotOutcome::BlockedVenting:     return Ventil;
		default:                                   return Rahmen;
		}
	}
}

AHeatSinkDemoDirector::AHeatSinkDemoDirector()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	BoardText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("BoardText"));
	BoardText->SetupAttachment(Root);
	BoardText->SetHorizontalAlignment(EHTA_Center);
	BoardText->SetVerticalAlignment(EVRTA_TextBottom);
	BoardText->SetWorldSize(54.0f);
	BoardText->SetTextRenderColor(FColor::White);
	// Yaw 270, not 90: at 90 a TextRender renders mirrored.
	BoardText->SetRelativeRotation(FRotator(0.0f, 270.0f, 0.0f));
	BoardText->SetRelativeLocation(FVector(0.0f, 0.0f, 790.0f));

	StateText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("StateText"));
	StateText->SetupAttachment(Root);
	StateText->SetHorizontalAlignment(EHTA_Center);
	StateText->SetVerticalAlignment(EVRTA_TextTop);
	StateText->SetWorldSize(33.0f);
	StateText->SetTextRenderColor(HeatSinkDemoLocal::Text);
	StateText->SetRelativeRotation(FRotator(0.0f, 270.0f, 0.0f));
	StateText->SetRelativeLocation(FVector(-480.0f, 0.0f, 680.0f));

	RuleText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("RuleText"));
	RuleText->SetupAttachment(Root);
	RuleText->SetHorizontalAlignment(EHTA_Center);
	RuleText->SetVerticalAlignment(EVRTA_TextTop);
	RuleText->SetWorldSize(33.0f);
	RuleText->SetTextRenderColor(FColor(250, 205, 120));
	RuleText->SetRelativeRotation(FRotator(0.0f, 270.0f, 0.0f));
	RuleText->SetRelativeLocation(FVector(480.0f, 0.0f, 680.0f));
}

FVector AHeatSinkDemoDirector::Basis() const
{
	return GetActorLocation() - FVector(0.0f, HeatSinkDemoLocal::FloorY, GetActorLocation().Z);
}

void AHeatSinkDemoDirector::BeginPlay()
{
	Super::BeginPlay();
	StartCycle();
}

void AHeatSinkDemoDirector::StartCycle()
{
	CycleTime = 0.0f;
	Rest = 0.0f;

	// A fixed seed, so the demo plays out the same way on every take and the sentence printed under
	// it stays true.
	Zustand = 0x5C1B93A7u;

	Regeln = FHeatRules();
	Regeln.MaxHeat = 100.0f;
	Regeln.HeatPerShot = 12.5f;          // eight shots to the line
	Regeln.CoolPerSecond = 25.0f;
	Regeln.CoolDelaySeconds = 0.6f;
	Regeln.OverheatLockoutSeconds = 2.5f;
	Regeln.HeatAfterLockout = 0.0f;
	Regeln.bLastShotFires = true;
	Regeln.VentSeconds = 1.2f;
	Regeln.HeatAfterVent = 0.0f;
	// Jams start in the upper third rather than at the very top. At 0.7/0.15 the only shot that
	// could ever jam was the one crossing the maximum - the weapon is locked out immediately
	// afterwards - and over a whole cycle the demo showed not a single jam. A demo that never
	// reaches a mechanic it advertises is worse than one that leaves it out.
	Regeln.JamFromFraction = 0.6f;
	Regeln.JamChanceAtMax = 0.35f;
	Regeln.ClearJamSeconds = 1.8f;
	Regeln = UHeatSinkStatics::NormaliseRules(Regeln);

	Dauerfeuer = FWaffe();
	Dauerfeuer.Name = TEXT("TRIGGER HELD");
	Dauerfeuer.bHaeltDrauf = true;
	Dauerfeuer.Stand.SecondsSinceShot = 1000.0f;

	Diszipliniert = FWaffe();
	Diszipliniert.Name = TEXT("BURST AND VENT");
	Diszipliniert.bHaeltDrauf = false;
	Diszipliniert.Stand.SecondsSinceShot = 1000.0f;

	bBereit = true;
}

float AHeatSinkDemoDirector::NaechsteZufallszahl()
{
	// A tiny xorshift. Deliberately NOT FMath::FRand: both weapons have to be fed the SAME sequence,
	// or the jam on the left is luck rather than the consequence of holding the trigger.
	Zustand ^= Zustand << 13;
	Zustand ^= Zustand >> 17;
	Zustand ^= Zustand << 5;
	return static_cast<float>(Zustand % 100000u) / 100000.0f;
}

void AHeatSinkDemoDirector::Ausloesen(FWaffe& W, const float Seconds)
{
	W.Stand = UHeatSinkStatics::Advance(W.Stand, Seconds, Regeln);
	W.SeitSchuss += Seconds;

	// A jammed weapon does not clear itself; somebody has to reach for it. Half a second of
	// realising, then the clearing time the rules name.
	if (W.Stand.bJammed && W.Stand.ClearRemaining <= 0.0f)
	{
		W.SeitKlemmt += Seconds;
		if (W.SeitKlemmt > 0.5f)
		{
			W.Stand = UHeatSinkStatics::BeginClearJam(W.Stand, Regeln);
			W.SeitKlemmt = 0.0f;
		}
	}
	else
	{
		W.SeitKlemmt = 0.0f;
	}

	// The disciplined shooter stops after a burst and vents in the gap, which is what the vent is
	// for: it is cheaper to spend a second on purpose than two and a half by accident.
	if (!W.bHaeltDrauf && W.ImStoss >= StossLaenge)
	{
		if (W.Stand.VentRemaining <= 0.0f && W.Stand.Heat > 0.0f && !W.Stand.bJammed
			&& W.Stand.LockoutRemaining <= 0.0f)
		{
			W.Stand = UHeatSinkStatics::BeginVent(W.Stand, Regeln);
		}
		if (W.Stand.VentRemaining <= 0.0f && W.Stand.Heat <= 0.0f)
		{
			W.ImStoss = 0;
		}
		W.bWollteSchiessen = false;
		return;
	}

	if (W.SeitSchuss < SekundenJeSchuss)
	{
		return;
	}
	W.SeitSchuss = 0.0f;
	W.bWollteSchiessen = true;

	const FHeatShotResult E = UHeatSinkStatics::Fire(W.Stand, Regeln, NaechsteZufallszahl());
	W.Stand = E.State;

	W.Verlauf.Add(E.Outcome);
	while (W.Verlauf.Num() > 34)
	{
		W.Verlauf.RemoveAt(0);
	}

	if (E.bFired)
	{
		++W.ImStoss;
	}
	else
	{
		++W.Blockiert;
	}
}

void AHeatSinkDemoDirector::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bAutoRun)
	{
		StepDemo(FMath::Clamp(DeltaSeconds, 0.0f, 0.1f));
	}
}

void AHeatSinkDemoDirector::EinTakt()
{
	CycleTime += Takt;
	if (CycleTime > CycleSeconds)
	{
		StartCycle();
		return;
	}
	Ausloesen(Dauerfeuer, Takt);
	Ausloesen(Diszipliniert, Takt);
}

void AHeatSinkDemoDirector::StepDemo(const float Seconds)
{
	// BeginPlay does not run in an editor viewport.
	if (!bBereit)
	{
		StartCycle();
	}

	// Consume the time in fixed sub-steps so the demo plays out identically whatever step size it
	// is handed. The leftover is carried, not dropped: dropping it would make a run at 60 steps a
	// second finish sooner than the same run at 8.
	Rest = FMath::Min(Rest + Seconds, CycleSeconds + Takt);
	while (Rest >= Takt)
	{
		Rest -= Takt;
		EinTakt();
	}

	if (BoardText)
	{
		BoardText->SetText(FText::FromString(HeadlineFor()));
	}
	if (StateText)
	{
		StateText->SetText(FText::FromString(BuildStateText()));
	}
	if (RuleText)
	{
		RuleText->SetText(FText::FromString(RuleFor()));
	}

	if (bDrawDemo)
	{
		DrawBars();
	}
}

FString AHeatSinkDemoDirector::HeadlineFor() const
{
	// Every headline is true for the WHOLE phase it covers.
	if (CycleTime < 9.0f)
	{
		return TEXT("both weapons, the same heat curve - one of them is holding the trigger");
	}
	if (CycleTime < 19.0f)
	{
		return TEXT("the lockout is a wait, not a heat check: tapping the boundary buys nothing");
	}
	return TEXT("a second spent venting on purpose beats two and a half spent locked out");
}

FString AHeatSinkDemoDirector::RuleFor() const
{
	if (CycleTime < 9.0f)
	{
		return TEXT("THE DELAY\n\ncooling starts 0.6 s after\nthe last shot, and every\nshot resets it\n\nwithout it heat drains\nbetween rounds and nothing\never overheats");
	}
	if (CycleTime < 19.0f)
	{
		return TEXT("THE LOCKOUT\n\n2.5 s, not \"until heat\ndrops\"\n\notherwise the player learns\nto tap the trigger on the\nline - the exact habit the\nmechanic exists to break");
	}
	return TEXT("THE VENT\n\ncosts 1.2 s and clears the\nheat completely\n\nthe choice is the mechanic:\nspend the second now, or\nlose the fight later");
}

FString AHeatSinkDemoDirector::BuildStateText() const
{
	FString S = FString::Printf(TEXT("%.1f s  -  %.1f heat per shot, max %.0f\n\n"),
		CycleTime, Regeln.HeatPerShot, Regeln.MaxHeat);

	auto Zustandswort = [](const FHeatState& H) -> FString
	{
		if (H.LockoutRemaining > 0.0f) { return FString::Printf(TEXT("LOCKED OUT %.1fs"), H.LockoutRemaining); }
		if (H.ClearRemaining > 0.0f)   { return FString::Printf(TEXT("CLEARING   %.1fs"), H.ClearRemaining); }
		if (H.bJammed)                 { return TEXT("JAMMED"); }
		if (H.VentRemaining > 0.0f)    { return FString::Printf(TEXT("VENTING    %.1fs"), H.VentRemaining); }
		return TEXT("READY");
	};

	auto Zeile = [&](const FWaffe& W) -> FString
	{
		return FString::Printf(TEXT("%s\n  heat        %5.0f  (%3.0f%%)\n  %s\n  shots fired %5d\n  blocked     %5d\n  overheats   %5d\n  jams        %5d\n  jam chance  %4.1f%%\n\n"),
			*W.Name, W.Stand.Heat, UHeatSinkStatics::HeatFraction(W.Stand, Regeln) * 100.0f,
			*Zustandswort(W.Stand), W.Stand.ShotsFired, W.Blockiert, W.Stand.Overheats, W.Stand.Jams,
			UHeatSinkStatics::JamChance(W.Stand, Regeln) * 100.0f);
	};

	S += Zeile(Dauerfeuer);
	S += Zeile(Diszipliniert);
	return S;
}

void AHeatSinkDemoDirector::DrawBars() const
{
#if ENABLE_DRAW_DEBUG
	const UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	UWorld* Mutable = const_cast<UWorld*>(World);

	// Persistent lines plus a flush at the start of every step - NOT lines with a lifetime. A
	// lifetime is counted down by the world tick, and an editor viewport that is not set to realtime
	// never ticks at all: the shapes would either pile up forever or never appear.
	FlushPersistentDebugLines(Mutable);

	using namespace HeatSinkDemoLocal;

	constexpr float Hoehe = 340.0f;
	constexpr float Fuss = 70.0f;

	const FVector B = Basis();
	const FWaffe* Waffen[2] = {&Dauerfeuer, &Diszipliniert};

	for (int32 i = 0; i < 2; ++i)
	{
		const FWaffe& W = *Waffen[i];
		const float X = SpalteX[i];
		const float Anteil = UHeatSinkStatics::HeatFraction(W.Stand, Regeln);

		// Colour is the state, not the number: locked out, jammed and venting each look different
		// from merely hot, because on screen they are different situations.
		FColor Farbe = FMath::Lerp(FLinearColor(Kuehl), FLinearColor(Heiss), Anteil).ToFColor(true);
		if (W.Stand.LockoutRemaining > 0.0f) { Farbe = Sperre; }
		else if (W.Stand.bJammed)            { Farbe = Klemmt; }
		else if (W.Stand.VentRemaining > 0.0f) { Farbe = Ventil; }

		for (int32 Ply = 0; Ply < 5; ++Ply)
		{
			const FVector Unten = B + FVector(X + (Ply - 2) * 7.0f, 0.0f, Fuss);
			DrawDebugLine(Mutable, Unten, Unten + FVector(0.0f, 0.0f, Hoehe),
				Rahmen, true, -1.0f, 0, 4.0f);

			const float Jetzt = Hoehe * FMath::Clamp(Anteil, 0.0f, 1.0f);
			if (Jetzt > 0.0f)
			{
				DrawDebugLine(Mutable, Unten, Unten + FVector(0.0f, 0.0f, Jetzt),
					Farbe, true, -1.0f, 0, 13.0f);
			}
		}

		// The line the weapon overheats on, and the one jams start at.
		const FVector Oben = B + FVector(X - 120.0f, 0.0f, Fuss + Hoehe);
		DrawDebugLine(Mutable, Oben, Oben + FVector(240.0f, 0.0f, 0.0f), Grenze, true, -1.0f, 0, 4.0f);

		const FVector Jam = B + FVector(X - 100.0f, 0.0f, Fuss + Hoehe * Regeln.JamFromFraction);
		DrawDebugLine(Mutable, Jam, Jam + FVector(200.0f, 0.0f, 0.0f), Klemmt, true, -1.0f, 0, 3.0f);

		// The last three seconds of trigger pulls, one tick each, coloured by what came of them.
		for (int32 k = 0; k < W.Verlauf.Num(); ++k)
		{
			const float Sx = X - 250.0f + k * 15.0f;
			const bool bGefeuert = W.Verlauf[k] == EHeatShotOutcome::Fired
				|| W.Verlauf[k] == EHeatShotOutcome::FiredAndOverheated
				|| W.Verlauf[k] == EHeatShotOutcome::FiredAndJammed;
			const FVector Unten = B + FVector(Sx, -260.0f, Fuss);
			DrawDebugLine(Mutable, Unten, Unten + FVector(0.0f, 0.0f, bGefeuert ? 70.0f : 26.0f),
				FarbeFuer(W.Verlauf[k]), true, -1.0f, 0, 7.0f);
		}

		// The muzzle: a flash only on a trigger pull that actually produced a shot.
		//
		// A star of four lines, not a debug sphere: a wireframe ball at this size reads as a golf
		// ball sitting on the floor, which is the last thing a weapon demo needs in the foreground.
		if (W.bWollteSchiessen && W.Verlauf.Num() > 0)
		{
			const EHeatShotOutcome Letzt = W.Verlauf.Last();
			const bool bGefeuert = Letzt == EHeatShotOutcome::Fired
				|| Letzt == EHeatShotOutcome::FiredAndOverheated
				|| Letzt == EHeatShotOutcome::FiredAndJammed;
			if (bGefeuert)
			{
				// At the foot of its own column, not out in the foreground: a mark 400 units nearer
				// the camera than everything else swings far out towards the frame edge and reads
				// as an unrelated symbol floating in the air.
				const FVector M = B + FVector(X, -60.0f, Fuss - 40.0f);
				const FVector Strahl[4] = {
					FVector(55.0f, 0.0f, 0.0f), FVector(-55.0f, 0.0f, 0.0f),
					FVector(0.0f, 0.0f, 55.0f), FVector(0.0f, 0.0f, -55.0f)};
				for (const FVector& S : Strahl)
				{
					DrawDebugLine(Mutable, M, M + S, Grenze, true, -1.0f, 0, 9.0f);
				}
			}
		}
	}
#endif
}
