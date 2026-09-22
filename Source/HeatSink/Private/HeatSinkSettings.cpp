// Copyright 2026 Silvan Teufel. All Rights Reserved.

#include "HeatSinkSettings.h"

UHeatSinkSettings::UHeatSinkSettings()
	: bLogEvents(false)
{
	// Eight shots to the top, two and a half seconds of lockout: long enough that overheating is a
	// mistake, short enough that it is not a punishment the player stops playing over.
	Rules.MaxHeat = 100.0f;
	Rules.HeatPerShot = 12.5f;
	Rules.CoolPerSecond = 25.0f;
	Rules.CoolDelaySeconds = 0.6f;
	Rules.OverheatLockoutSeconds = 2.5f;
	Rules.HeatAfterLockout = 0.0f;
	Rules.bLastShotFires = true;

	// Venting is three times faster than waiting out the same heat - and costs a window.
	Rules.VentSeconds = 1.2f;
	Rules.HeatAfterVent = 0.0f;

	// Jams off by default. They are a strong flavour, and a project that did not ask for them
	// should not discover them in a playtest.
	Rules.JamFromFraction = 1.0f;
	Rules.JamChanceAtMax = 0.0f;
	Rules.ClearJamSeconds = 1.8f;
}

const UHeatSinkSettings* UHeatSinkSettings::Get()
{
	const UHeatSinkSettings* Settings = GetDefault<UHeatSinkSettings>();
	check(Settings);
	return Settings;
}
