// Copyright 2026 Silvan Teufel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "HeatSinkTypes.h"
#include "HeatSinkStatics.generated.h"

/**
 * The rules, on their own.
 *
 * No world, no actor, no clock, no random stream - the random value for a jam is handed in. The
 * component calls exactly these and so do the tests.
 */
UCLASS()
class HEATSINK_API UHeatSinkStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Rules with the impossible combinations corrected. */
	UFUNCTION(BlueprintPure, Category = "HeatSink|Rules")
	static FHeatRules NormaliseRules(const FHeatRules& Rules);

	/** Can the trigger do anything at all right now? */
	UFUNCTION(BlueprintPure, Category = "HeatSink|Rules")
	static bool CanFire(const FHeatState& State);

	/** 0..1 for a gauge. */
	UFUNCTION(BlueprintPure, Category = "HeatSink|Rules")
	static float HeatFraction(const FHeatState& State, const FHeatRules& Rules);

	/**
	 * The jam chance at this heat.
	 *
	 * Nothing below JamFromFraction, then rising linearly to JamChanceAtMax at full heat. A flat
	 * chance would make a cold weapon jam as often as a glowing one, and the player would read the
	 * whole mechanic as random punishment rather than as a consequence of how they were shooting.
	 */
	UFUNCTION(BlueprintPure, Category = "HeatSink|Rules")
	static float JamChance(const FHeatState& State, const FHeatRules& Rules);

	/**
	 * One trigger pull. RandomValue in [0,1) decides the jam and comes from the caller.
	 *
	 * The shot that crosses the maximum still fires when bLastShotFires is on, and the overheat
	 * follows it. Refusing a trigger pull at ninety-nine percent heat reads as a broken gun.
	 */
	UFUNCTION(BlueprintCallable, Category = "HeatSink|Rules")
	static FHeatShotResult Fire(const FHeatState& State, const FHeatRules& Rules, float RandomValue);

	/**
	 * Time passing: the lockout, the vent and the jam clearing run down, and heat falls once the
	 * cooling delay has passed. Linear, so two half steps and one whole step agree.
	 */
	UFUNCTION(BlueprintPure, Category = "HeatSink|Rules")
	static FHeatState Advance(const FHeatState& State, float DeltaSeconds, const FHeatRules& Rules);

	/** Start a manual vent. Refused while locked out, jammed or already venting. */
	UFUNCTION(BlueprintPure, Category = "HeatSink|Rules")
	static FHeatState BeginVent(const FHeatState& State, const FHeatRules& Rules);

	/** Start clearing a jam. Does nothing if there is no jam. */
	UFUNCTION(BlueprintPure, Category = "HeatSink|Rules")
	static FHeatState BeginClearJam(const FHeatState& State, const FHeatRules& Rules);

	/** Seconds until the weapon can fire again. Zero when it already can. */
	UFUNCTION(BlueprintPure, Category = "HeatSink|Rules")
	static float SecondsUntilReady(const FHeatState& State);
};
