// Copyright 2026 Silvan Teufel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HeatSinkTypes.h"
#include "HeatSinkComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FHeatChangedSignature, float, Fraction);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FHeatOverheatedSignature, float, LockoutSeconds);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FHeatReadySignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FHeatJammedSignature);

/**
 * UHeatSinkComponent
 *
 * Goes on the weapon, or on whatever owns it. Call TryFire where your project pulls the trigger;
 * if it says the shot fired, spawn the projectile.
 *
 * It does not spawn anything, does not know what a bullet is, and has no opinion about ammunition.
 */
UCLASS(ClassGroup = (HeatSink), meta = (BlueprintSpawnableComponent))
class HEATSINK_API UHeatSinkComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHeatSinkComponent();

	/**
	 * One trigger pull. Returns the whole result so the caller can react in the same frame.
	 *
	 * The random value for the jam roll is drawn here. Use TryFireSeeded when a server, a client
	 * prediction and a replay all have to reach the same answer.
	 */
	UFUNCTION(BlueprintCallable, Category = "HeatSink")
	FHeatShotResult TryFire();

	/** Same, with the jam roll handed in. */
	UFUNCTION(BlueprintCallable, Category = "HeatSink")
	FHeatShotResult TryFireSeeded(float RandomValue);

	/** Start a manual vent. Faster than waiting, and it costs a window in which you cannot fire. */
	UFUNCTION(BlueprintCallable, Category = "HeatSink")
	void BeginVent();

	/** Start clearing a jam. */
	UFUNCTION(BlueprintCallable, Category = "HeatSink")
	void BeginClearJam();

	/** Cool the weapon right down. For a reload that swaps the barrel, or a pickup. */
	UFUNCTION(BlueprintCallable, Category = "HeatSink")
	void ResetHeat();

	/** Add heat from something that is not a shot - standing in lava, an enemy's beam. */
	UFUNCTION(BlueprintCallable, Category = "HeatSink")
	void AddHeat(float Amount);

	UFUNCTION(BlueprintPure, Category = "HeatSink")
	bool CanFire() const;

	UFUNCTION(BlueprintPure, Category = "HeatSink")
	float GetHeatFraction() const;

	UFUNCTION(BlueprintPure, Category = "HeatSink")
	float GetJamChance() const;

	/** Seconds until the trigger works again. Zero when it already does. */
	UFUNCTION(BlueprintPure, Category = "HeatSink")
	float GetSecondsUntilReady() const;

	UFUNCTION(BlueprintPure, Category = "HeatSink")
	const FHeatState& GetState() const { return State; }

	UFUNCTION(BlueprintPure, Category = "HeatSink")
	FHeatRules GetRules() const;

	/** Step the clock by hand. Public so a fixed-step server or a demo can drive it. */
	UFUNCTION(BlueprintCallable, Category = "HeatSink")
	void AdvanceTime(float DeltaSeconds);

	UFUNCTION(BlueprintCallable, Category = "HeatSink")
	void SetAutoTick(bool bEnabled) { bAutoTick = bEnabled; }

	UPROPERTY(BlueprintAssignable, Category = "HeatSink")
	FHeatChangedSignature OnHeatChanged;

	UPROPERTY(BlueprintAssignable, Category = "HeatSink")
	FHeatOverheatedSignature OnOverheated;

	/** Fires once, on the edge, when the weapon becomes usable again. */
	UPROPERTY(BlueprintAssignable, Category = "HeatSink")
	FHeatReadySignature OnReady;

	UPROPERTY(BlueprintAssignable, Category = "HeatSink")
	FHeatJammedSignature OnJammed;

protected:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HeatSink")
	bool bOverrideRules = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HeatSink", meta = (EditCondition = "bOverrideRules"))
	FHeatRules RuleOverride;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HeatSink")
	bool bAutoTick = true;

private:
	UPROPERTY()
	FHeatState State;

	/** So OnReady fires once, on the edge. */
	bool bWarBereit = true;
};
