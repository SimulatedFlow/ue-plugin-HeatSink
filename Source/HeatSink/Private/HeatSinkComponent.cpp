// Copyright 2026 Silvan Teufel. All Rights Reserved.

#include "HeatSinkComponent.h"

#include "GameFramework/Actor.h"
#include "HeatSinkLog.h"
#include "HeatSinkSettings.h"
#include "HeatSinkStatics.h"

UHeatSinkComponent::UHeatSinkComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

FHeatRules UHeatSinkComponent::GetRules() const
{
	return UHeatSinkStatics::NormaliseRules(
		bOverrideRules ? RuleOverride : UHeatSinkSettings::Get()->Rules);
}

bool UHeatSinkComponent::CanFire() const
{
	return UHeatSinkStatics::CanFire(State);
}

float UHeatSinkComponent::GetHeatFraction() const
{
	return UHeatSinkStatics::HeatFraction(State, GetRules());
}

float UHeatSinkComponent::GetJamChance() const
{
	return UHeatSinkStatics::JamChance(State, GetRules());
}

float UHeatSinkComponent::GetSecondsUntilReady() const
{
	return UHeatSinkStatics::SecondsUntilReady(State);
}

FHeatShotResult UHeatSinkComponent::TryFire()
{
	return TryFireSeeded(FMath::FRand());
}

FHeatShotResult UHeatSinkComponent::TryFireSeeded(const float RandomValue)
{
	const FHeatRules R = GetRules();
	const FHeatShotResult Ergebnis = UHeatSinkStatics::Fire(State, R, RandomValue);

	const float Vorher = GetHeatFraction();
	State = Ergebnis.State;
	bWarBereit = UHeatSinkStatics::CanFire(State);

	if (!FMath::IsNearlyEqual(Vorher, GetHeatFraction()))
	{
		OnHeatChanged.Broadcast(GetHeatFraction());
	}

	if (Ergebnis.Outcome == EHeatShotOutcome::FiredAndOverheated)
	{
		if (UHeatSinkSettings::Get()->bLogEvents)
		{
			UE_LOG(LogHeatSink, Display, TEXT("[%s] overheated after %d shots, locked out for %.2fs"),
				*GetNameSafe(GetOwner()), State.ShotsFired, State.LockoutRemaining);
		}
		OnOverheated.Broadcast(State.LockoutRemaining);
	}
	else if (Ergebnis.Outcome == EHeatShotOutcome::FiredAndJammed)
	{
		if (UHeatSinkSettings::Get()->bLogEvents)
		{
			UE_LOG(LogHeatSink, Display, TEXT("[%s] jammed at %.0f%% heat (chance was %.1f%%)"),
				*GetNameSafe(GetOwner()), GetHeatFraction() * 100.0f, Ergebnis.JamChance * 100.0f);
		}
		OnJammed.Broadcast();
	}

	return Ergebnis;
}

void UHeatSinkComponent::BeginVent()
{
	State = UHeatSinkStatics::BeginVent(State, GetRules());
	bWarBereit = UHeatSinkStatics::CanFire(State);
}

void UHeatSinkComponent::BeginClearJam()
{
	State = UHeatSinkStatics::BeginClearJam(State, GetRules());
	bWarBereit = UHeatSinkStatics::CanFire(State);
}

void UHeatSinkComponent::ResetHeat()
{
	State = FHeatState();
	bWarBereit = true;
	OnHeatChanged.Broadcast(0.0f);
}

void UHeatSinkComponent::AddHeat(const float Amount)
{
	if (Amount <= 0.0f)
	{
		return;
	}
	const FHeatRules R = GetRules();
	State.Heat = FMath::Min(State.Heat + Amount, R.MaxHeat);
	OnHeatChanged.Broadcast(GetHeatFraction());
}

void UHeatSinkComponent::TickComponent(const float DeltaTime, const ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bAutoTick)
	{
		AdvanceTime(DeltaTime);
	}
}

void UHeatSinkComponent::AdvanceTime(const float DeltaSeconds)
{
	const float Vorher = GetHeatFraction();
	State = UHeatSinkStatics::Advance(State, DeltaSeconds, GetRules());

	if (!FMath::IsNearlyEqual(Vorher, GetHeatFraction()))
	{
		OnHeatChanged.Broadcast(GetHeatFraction());
	}

	// On the edge only: a "ready" that fires every frame is a sound effect nobody can use.
	const bool bBereit = UHeatSinkStatics::CanFire(State);
	if (bBereit && !bWarBereit)
	{
		OnReady.Broadcast();
	}
	bWarBereit = bBereit;
}
