// Copyright 2026 Silvan Teufel. All Rights Reserved.

#include "HeatSinkStatics.h"

FHeatRules UHeatSinkStatics::NormaliseRules(const FHeatRules& Rules)
{
	FHeatRules Out = Rules;

	Out.MaxHeat = FMath::Max(1.0f, Out.MaxHeat);
	Out.HeatPerShot = FMath::Max(0.0f, Out.HeatPerShot);
	Out.CoolPerSecond = FMath::Max(0.0f, Out.CoolPerSecond);
	Out.CoolDelaySeconds = FMath::Max(0.0f, Out.CoolDelaySeconds);
	Out.OverheatLockoutSeconds = FMath::Max(0.0f, Out.OverheatLockoutSeconds);
	Out.VentSeconds = FMath::Max(0.0f, Out.VentSeconds);
	Out.ClearJamSeconds = FMath::Max(0.0f, Out.ClearJamSeconds);

	// The heat a lockout or a vent leaves behind has to be inside the bar. Leaving it at or above
	// the maximum would overheat the weapon again the moment it came back, forever.
	Out.HeatAfterLockout = FMath::Clamp(Out.HeatAfterLockout, 0.0f, Out.MaxHeat * 0.95f);
	Out.HeatAfterVent = FMath::Clamp(Out.HeatAfterVent, 0.0f, Out.MaxHeat * 0.95f);

	Out.JamFromFraction = FMath::Clamp(Out.JamFromFraction, 0.0f, 1.0f);
	Out.JamChanceAtMax = FMath::Clamp(Out.JamChanceAtMax, 0.0f, 1.0f);

	return Out;
}

bool UHeatSinkStatics::CanFire(const FHeatState& State)
{
	return State.LockoutRemaining <= 0.0f
		&& State.VentRemaining <= 0.0f
		&& State.ClearRemaining <= 0.0f
		&& !State.bJammed;
}

float UHeatSinkStatics::HeatFraction(const FHeatState& State, const FHeatRules& Rules)
{
	const FHeatRules R = NormaliseRules(Rules);
	return FMath::Clamp(State.Heat / R.MaxHeat, 0.0f, 1.0f);
}

float UHeatSinkStatics::JamChance(const FHeatState& State, const FHeatRules& Rules)
{
	const FHeatRules R = NormaliseRules(Rules);
	if (R.JamChanceAtMax <= 0.0f || R.JamFromFraction >= 1.0f)
	{
		return 0.0f;
	}

	const float Anteil = HeatFraction(State, R);
	if (Anteil <= R.JamFromFraction)
	{
		return 0.0f;
	}

	const float Spanne = FMath::Max(KINDA_SMALL_NUMBER, 1.0f - R.JamFromFraction);
	const float Hinein = (Anteil - R.JamFromFraction) / Spanne;
	return FMath::Clamp(Hinein * R.JamChanceAtMax, 0.0f, 1.0f);
}

FHeatShotResult UHeatSinkStatics::Fire(const FHeatState& State, const FHeatRules& Rules,
	const float RandomValue)
{
	const FHeatRules R = NormaliseRules(Rules);

	FHeatShotResult Result;
	Result.State = State;

	// Three separate refusals with three separate reasons: a player who cannot shoot needs to know
	// whether to wait, to vent or to clear, and one blanket "cannot fire" tells them none of it.
	if (State.bJammed || State.ClearRemaining > 0.0f)
	{
		Result.Outcome = EHeatShotOutcome::BlockedJammed;
		return Result;
	}
	if (State.LockoutRemaining > 0.0f)
	{
		Result.Outcome = EHeatShotOutcome::BlockedOverheated;
		return Result;
	}
	if (State.VentRemaining > 0.0f)
	{
		Result.Outcome = EHeatShotOutcome::BlockedVenting;
		return Result;
	}

	const float Danach = State.Heat + R.HeatPerShot;
	const bool bWuerdeUeberhitzen = Danach >= R.MaxHeat;

	if (bWuerdeUeberhitzen && !R.bLastShotFires)
	{
		// The stricter reading: the weapon refuses a shot it cannot afford. It still overheats, so
		// the player is not left tapping a trigger that silently does nothing.
		Result.State.LockoutRemaining = R.OverheatLockoutSeconds;
		Result.State.Heat = R.MaxHeat;
		Result.State.Overheats = State.Overheats + 1;
		Result.Outcome = EHeatShotOutcome::BlockedOverheated;
		return Result;
	}

	Result.bFired = true;
	Result.State.ShotsFired = State.ShotsFired + 1;
	Result.State.SecondsSinceShot = 0.0f;
	Result.State.Heat = FMath::Min(Danach, R.MaxHeat);

	// The jam is measured against the heat the shot LEAVES, not the heat it started from: the shot
	// that takes a weapon to the top is exactly the one most likely to jam it.
	Result.JamChance = JamChance(Result.State, R);
	const bool bKlemmt = Result.JamChance > 0.0f && RandomValue < Result.JamChance;

	if (bWuerdeUeberhitzen)
	{
		Result.State.LockoutRemaining = R.OverheatLockoutSeconds;
		Result.State.Overheats = State.Overheats + 1;
		Result.Outcome = EHeatShotOutcome::FiredAndOverheated;
	}
	else if (bKlemmt)
	{
		Result.State.bJammed = true;
		Result.State.Jams = State.Jams + 1;
		Result.Outcome = EHeatShotOutcome::FiredAndJammed;
	}
	else
	{
		Result.Outcome = EHeatShotOutcome::Fired;
	}

	return Result;
}

FHeatState UHeatSinkStatics::Advance(const FHeatState& State, const float DeltaSeconds,
	const FHeatRules& Rules)
{
	const FHeatRules R = NormaliseRules(Rules);

	FHeatState Out = State;
	if (DeltaSeconds <= 0.0f)
	{
		return Out;
	}

	Out.SecondsSinceShot = State.SecondsSinceShot + DeltaSeconds;

	const bool bWarGesperrt = State.LockoutRemaining > 0.0f;
	Out.LockoutRemaining = FMath::Max(0.0f, State.LockoutRemaining - DeltaSeconds);
	if (bWarGesperrt && Out.LockoutRemaining <= 0.0f)
	{
		// The lockout ends with a defined heat, not with whatever cooling happened to leave. A
		// weapon that comes back at ninety-nine percent overheats on the next shot and the player
		// never gets to fire.
		Out.Heat = R.HeatAfterLockout;
	}

	const bool bWarLuften = State.VentRemaining > 0.0f;
	Out.VentRemaining = FMath::Max(0.0f, State.VentRemaining - DeltaSeconds);
	if (bWarLuften && Out.VentRemaining <= 0.0f)
	{
		Out.Heat = R.HeatAfterVent;
	}

	const bool bWarKlemmen = State.ClearRemaining > 0.0f;
	Out.ClearRemaining = FMath::Max(0.0f, State.ClearRemaining - DeltaSeconds);
	if (bWarKlemmen && Out.ClearRemaining <= 0.0f)
	{
		Out.bJammed = false;
	}

	// Passive cooling runs only when nothing else is going on. During a lockout or a vent the heat
	// is decided by the rule that ends it, not by drift.
	const bool bDarfKuehlen = Out.LockoutRemaining <= 0.0f && Out.VentRemaining <= 0.0f;

	if (bDarfKuehlen && R.CoolPerSecond > 0.0f)
	{
		// Only the part of the step that lies PAST the delay cools.
		//
		// WARUM (14.09.2026, von einem roten Test gefunden): vorher wurde auf den Stand am ENDE
		// des Schritts geprueft und dann die ganze Schrittlaenge gekuehlt. Ein Schritt von 1,6 s
		// ueber eine Verzoegerung von 0,6 s kuehlte damit 1,6 s statt 1,0 s - die Bildrate haette
		// also entschieden, wie schnell die Waffe abkuehlt, und die Zusicherung "zwei halbe
		// Schritte sind ein ganzer" galt genau an der Grenze nicht.
		const float Vorher = FMath::Max(0.0f, State.SecondsSinceShot);
		const float Wirksam = FMath::Clamp(Out.SecondsSinceShot - FMath::Max(Vorher, R.CoolDelaySeconds),
			0.0f, DeltaSeconds);
		if (Wirksam > 0.0f)
		{
			Out.Heat = FMath::Max(0.0f, Out.Heat - R.CoolPerSecond * Wirksam);
		}
	}

	return Out;
}

FHeatState UHeatSinkStatics::BeginVent(const FHeatState& State, const FHeatRules& Rules)
{
	const FHeatRules R = NormaliseRules(Rules);

	FHeatState Out = State;
	if (State.LockoutRemaining > 0.0f || State.VentRemaining > 0.0f
		|| State.ClearRemaining > 0.0f || State.bJammed)
	{
		return Out;
	}
	Out.VentRemaining = R.VentSeconds;
	return Out;
}

FHeatState UHeatSinkStatics::BeginClearJam(const FHeatState& State, const FHeatRules& Rules)
{
	const FHeatRules R = NormaliseRules(Rules);

	FHeatState Out = State;
	if (!State.bJammed || State.ClearRemaining > 0.0f)
	{
		return Out;
	}
	Out.ClearRemaining = R.ClearJamSeconds;
	return Out;
}

float UHeatSinkStatics::SecondsUntilReady(const FHeatState& State)
{
	return FMath::Max3(State.LockoutRemaining, State.VentRemaining, State.ClearRemaining);
}
