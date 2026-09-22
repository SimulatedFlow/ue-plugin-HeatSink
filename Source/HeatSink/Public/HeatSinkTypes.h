// Copyright 2026 Silvan Teufel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HeatSinkTypes.generated.h"

/** What happened when the trigger was pulled. */
UENUM(BlueprintType)
enum class EHeatShotOutcome : uint8
{
	/** It fired and the weapon is still usable. */
	Fired,
	/** It fired, and that shot pushed the weapon over the line. */
	FiredAndOverheated,
	/** It did not fire: the weapon is in its overheat lockout. */
	BlockedOverheated,
	/** It did not fire: the weapon is jammed and has to be cleared. */
	BlockedJammed,
	/** It did not fire: a vent is in progress. */
	BlockedVenting,
	/** It fired and jammed. */
	FiredAndJammed,
};

/** The heat curve. Passed to the pure functions explicitly. */
USTRUCT(BlueprintType)
struct HEATSINK_API FHeatRules
{
	GENERATED_BODY()

	/** Heat the weapon can hold before it overheats. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HeatSink", meta = (ClampMin = "1.0"))
	float MaxHeat = 100.0f;

	/** Heat one shot adds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HeatSink", meta = (ClampMin = "0.0"))
	float HeatPerShot = 12.0f;

	/** Heat lost per second once cooling has started. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HeatSink", meta = (ClampMin = "0.0"))
	float CoolPerSecond = 25.0f;

	/**
	 * Seconds after the last shot before cooling starts. Every shot resets it.
	 *
	 * Without a delay, heat quietly drains between the rounds of a burst and the weapon never
	 * overheats at all - which looks, from the outside, like the whole system does nothing.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HeatSink", meta = (ClampMin = "0.0"))
	float CoolDelaySeconds = 0.6f;

	/**
	 * How long the weapon is unusable after overheating.
	 *
	 * A LOCKOUT, not "wait until the heat drops". Without it the weapon comes back the instant heat
	 * falls one point below the maximum, and the player learns to tap the trigger on the boundary -
	 * which is the exact behaviour an overheat mechanic exists to discourage.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HeatSink", meta = (ClampMin = "0.0"))
	float OverheatLockoutSeconds = 2.5f;

	/** Heat the weapon is left with when the lockout ends. Zero is a clean slate. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HeatSink", meta = (ClampMin = "0.0"))
	float HeatAfterLockout = 0.0f;

	/**
	 * The shot that crosses the maximum still fires.
	 *
	 * On by default: refusing a trigger pull at ninety-nine percent heat reads as a broken gun, and
	 * the player cannot see the number the way the code can. Off gives the stricter reading, where
	 * the weapon simply will not take a shot it cannot afford.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HeatSink")
	bool bLastShotFires = true;

	/** How long a manual vent takes. During it the weapon cannot fire. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HeatSink", meta = (ClampMin = "0.0"))
	float VentSeconds = 1.2f;

	/** Heat left when a vent finishes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HeatSink", meta = (ClampMin = "0.0"))
	float HeatAfterVent = 0.0f;

	/** Fraction of MaxHeat above which jams become possible. One switches jams off. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HeatSink", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float JamFromFraction = 1.0f;

	/** Jam chance at maximum heat. Scaled down linearly to nothing at JamFromFraction. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HeatSink", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float JamChanceAtMax = 0.0f;

	/** How long clearing a jam takes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HeatSink", meta = (ClampMin = "0.0"))
	float ClearJamSeconds = 1.8f;
};

/** One weapon's heat. Plain data: no world, no actor, no clock. */
USTRUCT(BlueprintType)
struct HEATSINK_API FHeatState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "HeatSink")
	float Heat = 0.0f;

	/** Greater than zero: the overheat lockout is running. */
	UPROPERTY(BlueprintReadOnly, Category = "HeatSink")
	float LockoutRemaining = 0.0f;

	/** Greater than zero: a manual vent is running. */
	UPROPERTY(BlueprintReadOnly, Category = "HeatSink")
	float VentRemaining = 0.0f;

	/** Greater than zero: a jam is being cleared. */
	UPROPERTY(BlueprintReadOnly, Category = "HeatSink")
	float ClearRemaining = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "HeatSink")
	bool bJammed = false;

	/** Since the last shot. Drives the cooling delay. */
	UPROPERTY(BlueprintReadOnly, Category = "HeatSink")
	float SecondsSinceShot = 1000.0f;

	UPROPERTY(BlueprintReadOnly, Category = "HeatSink")
	int32 ShotsFired = 0;

	UPROPERTY(BlueprintReadOnly, Category = "HeatSink")
	int32 Overheats = 0;

	UPROPERTY(BlueprintReadOnly, Category = "HeatSink")
	int32 Jams = 0;
};

/** What one trigger pull did. */
USTRUCT(BlueprintType)
struct HEATSINK_API FHeatShotResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "HeatSink")
	FHeatState State;

	UPROPERTY(BlueprintReadOnly, Category = "HeatSink")
	EHeatShotOutcome Outcome = EHeatShotOutcome::BlockedOverheated;

	UPROPERTY(BlueprintReadOnly, Category = "HeatSink")
	bool bFired = false;

	/** The jam chance this shot was measured against, for a debug readout. */
	UPROPERTY(BlueprintReadOnly, Category = "HeatSink")
	float JamChance = 0.0f;
};
