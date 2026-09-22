// Copyright 2026 Silvan Teufel. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "HeatSinkStatics.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace HeatSinkTests
{
	constexpr EAutomationTestFlags TestFlags = EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::CommandletContext
		| EAutomationTestFlags::EngineFilter;

	/** Eight shots to overheat, two and a half seconds locked out. */
	static FHeatRules Rules()
	{
		FHeatRules R;
		R.MaxHeat = 100.0f;
		R.HeatPerShot = 12.5f;
		R.CoolPerSecond = 25.0f;
		R.CoolDelaySeconds = 0.6f;
		R.OverheatLockoutSeconds = 2.5f;
		R.HeatAfterLockout = 0.0f;
		R.bLastShotFires = true;
		R.VentSeconds = 1.2f;
		R.HeatAfterVent = 0.0f;
		R.JamFromFraction = 1.0f;     // jams off unless a test asks for them
		R.JamChanceAtMax = 0.0f;
		R.ClearJamSeconds = 1.8f;
		return R;
	}

	static FHeatState Bei(const float Heat)
	{
		FHeatState S;
		S.Heat = Heat;
		return S;
	}
}

// -------------------------------------------------------------------------------------------------
// The rule that makes an overheat mean something.
// -------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHeatSinkOverheatIsALockoutNotAThreshold,
	"HeatSink.Overheat.IsALockoutNotAThreshold", HeatSinkTests::TestFlags)

bool FHeatSinkOverheatIsALockoutNotAThreshold::RunTest(const FString&)
{
	using namespace HeatSinkTests;
	const FHeatRules R = Rules();

	// Eight shots take it to the top.
	FHeatState S;
	for (int32 i = 0; i < 7; ++i)
	{
		const FHeatShotResult Schuss = UHeatSinkStatics::Fire(S, R, 0.5f);
		TestTrue(TEXT("the first seven shots fire cleanly"), Schuss.Outcome == EHeatShotOutcome::Fired);
		S = Schuss.State;
	}

	// THE EIGHTH FIRES, and then overheats. Refusing a trigger pull at eighty-seven percent heat
	// reads as a broken gun - the player cannot see the number the way the code can.
	const FHeatShotResult Achter = UHeatSinkStatics::Fire(S, R, 0.5f);
	TestTrue(TEXT("the shot that crosses the line still fires"), Achter.bFired);
	TestTrue(TEXT("and reports the overheat"), Achter.Outcome == EHeatShotOutcome::FiredAndOverheated);
	S = Achter.State;
	TestNearlyEqual(TEXT("the lockout is running"), S.LockoutRemaining, 2.5f, 0.0001f);

	// THE POINT: heat falls during the lockout, but the weapon does NOT come back when it drops
	// below the maximum. Without the lockout the player learns to tap the trigger on the boundary,
	// which is the exact behaviour the mechanic exists to discourage.
	FHeatState Waehrend = UHeatSinkStatics::Advance(S, 1.0f, R);
	TestFalse(TEXT("one second in, still locked out"), UHeatSinkStatics::CanFire(Waehrend));
	TestTrue(TEXT("and the trigger says why"),
		UHeatSinkStatics::Fire(Waehrend, R, 0.5f).Outcome == EHeatShotOutcome::BlockedOverheated);

	Waehrend = UHeatSinkStatics::Advance(Waehrend, 2.0f, R);   // t = 3 s > 2.5 s
	TestTrue(TEXT("past the lockout it fires again"), UHeatSinkStatics::CanFire(Waehrend));
	TestNearlyEqual(TEXT("and comes back at the heat the rule promises"), Waehrend.Heat, 0.0f, 0.0001f);

	// The stricter reading: the weapon refuses the shot it cannot afford, and still overheats -
	// so the player is not left tapping a trigger that silently does nothing.
	FHeatRules Streng = R;
	Streng.bLastShotFires = false;
	const FHeatShotResult Verweigert = UHeatSinkStatics::Fire(Bei(95.0f), Streng, 0.5f);
	TestFalse(TEXT("with the strict rule the last shot does not fire"), Verweigert.bFired);
	TestTrue(TEXT("but the weapon is locked out"), Verweigert.State.LockoutRemaining > 0.0f);

	return true;
}

// -------------------------------------------------------------------------------------------------
// Cooling.
// -------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHeatSinkCoolingWaitsForTheDelay,
	"HeatSink.Cooling.WaitsForTheDelayThatEveryShotResets", HeatSinkTests::TestFlags)

bool FHeatSinkCoolingWaitsForTheDelay::RunTest(const FString&)
{
	using namespace HeatSinkTests;
	const FHeatRules R = Rules();   // delay 0.6 s, 25 per second

	// A shot, then a fifth of a second. Nothing cools yet.
	FHeatState S = UHeatSinkStatics::Fire(Bei(50.0f), R, 0.5f).State;
	const float NachSchuss = S.Heat;
	S = UHeatSinkStatics::Advance(S, 0.2f, R);
	TestNearlyEqual(TEXT("inside the delay nothing cools"), S.Heat, NachSchuss, 0.0001f);

	// THE REASON THE DELAY EXISTS: without it, heat drains between the rounds of a burst and the
	// weapon never overheats at all - which looks, from outside, like the system does nothing.
	// Here a steady burst climbs, because each shot puts the delay back to zero.
	FHeatState Salve;
	for (int32 i = 0; i < 6; ++i)
	{
		Salve = UHeatSinkStatics::Fire(Salve, R, 0.5f).State;
		Salve = UHeatSinkStatics::Advance(Salve, 0.15f, R);   // faster than the delay
	}
	TestTrue(TEXT("a burst inside the delay really does build heat"), Salve.Heat > 70.0f);

	// Past the delay it cools at the stated rate.
	FHeatState Ruhe = Bei(80.0f);
	Ruhe.SecondsSinceShot = 0.0f;
	Ruhe = UHeatSinkStatics::Advance(Ruhe, 1.6f, R);   // 0.6 s of delay, then 1.0 s of cooling
	TestNearlyEqual(TEXT("one second of cooling is twenty-five"), Ruhe.Heat, 55.0f, 0.001f);

	// And it never goes below nothing.
	for (int32 i = 0; i < 20; ++i)
	{
		Ruhe = UHeatSinkStatics::Advance(Ruhe, 1.0f, R);
	}
	TestNearlyEqual(TEXT("heat stops at zero"), Ruhe.Heat, 0.0f, 0.0001f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHeatSinkCoolingIsStepSizeIndependent,
	"HeatSink.Cooling.TwoHalfStepsEqualOneWholeStep", HeatSinkTests::TestFlags)

bool FHeatSinkCoolingIsStepSizeIndependent::RunTest(const FString&)
{
	using namespace HeatSinkTests;
	const FHeatRules R = Rules();

	// A frame rate that changes must not change the weapon.
	FHeatState A = Bei(90.0f);
	A.SecondsSinceShot = 10.0f;
	FHeatState B = A;

	A = UHeatSinkStatics::Advance(A, 1.0f, R);

	B = UHeatSinkStatics::Advance(B, 0.5f, R);
	B = UHeatSinkStatics::Advance(B, 0.5f, R);

	TestNearlyEqual(TEXT("one whole step and two half steps agree"), A.Heat, B.Heat, 0.0001f);

	// ACROSS THE DELAY BOUNDARY, which is where this used to be wrong: a single long step cooled
	// for its whole length as soon as its END was past the delay, so one step of 1.6 s cooled 1.6 s
	// while four of 0.4 s cooled 1.0 s. The frame rate decided how fast the weapon recovered.
	FHeatState Gross = Bei(90.0f);
	Gross.SecondsSinceShot = 0.0f;
	FHeatState Klein = Gross;

	Gross = UHeatSinkStatics::Advance(Gross, 1.6f, R);
	for (int32 i = 0; i < 4; ++i)
	{
		Klein = UHeatSinkStatics::Advance(Klein, 0.4f, R);
	}

	TestNearlyEqual(TEXT("one long step and four short ones agree across the delay"),
		Gross.Heat, Klein.Heat, 0.0001f);
	TestNearlyEqual(TEXT("and both cooled for exactly the second that lay past it"),
		Gross.Heat, 65.0f, 0.001f);

	return true;
}

// -------------------------------------------------------------------------------------------------
// Venting is a trade.
// -------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHeatSinkVentingCostsAWindow,
	"HeatSink.Vent.IsFasterButCostsAWindow", HeatSinkTests::TestFlags)

bool FHeatSinkVentingCostsAWindow::RunTest(const FString&)
{
	using namespace HeatSinkTests;
	const FHeatRules R = Rules();   // vent 1.2 s, cooling 25/s

	// Venting from ninety takes 1.2 s. Waiting for the same heat to cool takes 3.6 s plus the
	// delay. That difference is the whole decision the mechanic offers the player.
	FHeatState Lueften = UHeatSinkStatics::BeginVent(Bei(90.0f), R);
	TestTrue(TEXT("the vent is running"), Lueften.VentRemaining > 0.0f);
	TestFalse(TEXT("and the weapon cannot fire during it"), UHeatSinkStatics::CanFire(Lueften));
	TestTrue(TEXT("the trigger says why"),
		UHeatSinkStatics::Fire(Lueften, R, 0.5f).Outcome == EHeatShotOutcome::BlockedVenting);

	Lueften = UHeatSinkStatics::Advance(Lueften, 1.3f, R);
	TestTrue(TEXT("after the vent it fires again"), UHeatSinkStatics::CanFire(Lueften));
	TestNearlyEqual(TEXT("and the heat is what the rule promises"), Lueften.Heat, 0.0f, 0.0001f);

	// Venting cannot be used to escape a lockout - that would make overheating free.
	FHeatState Gesperrt = Bei(100.0f);
	Gesperrt.LockoutRemaining = 2.0f;
	const FHeatState Versuch = UHeatSinkStatics::BeginVent(Gesperrt, R);
	TestNearlyEqual(TEXT("venting during a lockout does nothing"), Versuch.VentRemaining, 0.0f, 0.0001f);
	TestNearlyEqual(TEXT("and the lockout keeps running"), Versuch.LockoutRemaining, 2.0f, 0.0001f);

	// A heat-after-vent at or above the maximum would overheat the weapon again immediately and
	// forever; the rules refuse to mean that.
	FHeatRules Unsinn = R;
	Unsinn.HeatAfterVent = 500.0f;
	Unsinn.HeatAfterLockout = 500.0f;
	const FHeatRules Fest = UHeatSinkStatics::NormaliseRules(Unsinn);
	TestTrue(TEXT("the heat a vent leaves is inside the bar"), Fest.HeatAfterVent < Fest.MaxHeat);
	TestTrue(TEXT("and so is the heat a lockout leaves"), Fest.HeatAfterLockout < Fest.MaxHeat);

	return true;
}

// -------------------------------------------------------------------------------------------------
// Jams.
// -------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHeatSinkJamChanceRisesWithHeat,
	"HeatSink.Jam.ChanceRisesWithHeatAndIsNotAnOverheat", HeatSinkTests::TestFlags)

bool FHeatSinkJamChanceRisesWithHeat::RunTest(const FString&)
{
	using namespace HeatSinkTests;
	FHeatRules R = Rules();
	R.JamFromFraction = 0.6f;
	R.JamChanceAtMax = 0.4f;

	// A cold weapon never jams. A flat chance would make the mechanic read as random punishment
	// instead of as a consequence of how the player was shooting.
	TestNearlyEqual(TEXT("cold, no chance at all"),
		UHeatSinkStatics::JamChance(Bei(10.0f), R), 0.0f, 0.0001f);
	TestNearlyEqual(TEXT("at the threshold, still nothing"),
		UHeatSinkStatics::JamChance(Bei(60.0f), R), 0.0f, 0.0001f);
	TestNearlyEqual(TEXT("halfway past it, half the chance"),
		UHeatSinkStatics::JamChance(Bei(80.0f), R), 0.2f, 0.0001f);
	TestNearlyEqual(TEXT("at the top, the full chance"),
		UHeatSinkStatics::JamChance(Bei(100.0f), R), 0.4f, 0.0001f);

	// A jam is a SEPARATE state from an overheat, with its own clearing time. Folding the two
	// together would mean a jam could be waited out, and waiting is what an overheat is for.
	const FHeatShotResult Klemmt = UHeatSinkStatics::Fire(Bei(80.0f), R, 0.01f);
	TestTrue(TEXT("the shot fired"), Klemmt.bFired);
	TestTrue(TEXT("and jammed"), Klemmt.Outcome == EHeatShotOutcome::FiredAndJammed);
	TestTrue(TEXT("the weapon is jammed"), Klemmt.State.bJammed);
	TestFalse(TEXT("and cannot fire"), UHeatSinkStatics::CanFire(Klemmt.State));

	// Waiting does not clear it, however long.
	FHeatState Warten = Klemmt.State;
	for (int32 i = 0; i < 30; ++i)
	{
		Warten = UHeatSinkStatics::Advance(Warten, 1.0f, R);
	}
	TestTrue(TEXT("thirty seconds of waiting do not clear a jam"), Warten.bJammed);
	TestNearlyEqual(TEXT("even though the weapon is stone cold"), Warten.Heat, 0.0f, 0.0001f);

	// Clearing does, and takes its own time.
	FHeatState Frei = UHeatSinkStatics::BeginClearJam(Warten, R);
	TestTrue(TEXT("clearing is under way"), Frei.ClearRemaining > 0.0f);
	Frei = UHeatSinkStatics::Advance(Frei, 1.0f, R);
	TestTrue(TEXT("still jammed part way through"), Frei.bJammed);
	Frei = UHeatSinkStatics::Advance(Frei, 1.0f, R);
	TestFalse(TEXT("and clear afterwards"), Frei.bJammed);
	TestTrue(TEXT("and firing again"), UHeatSinkStatics::CanFire(Frei));

	// A lucky roll at the same heat does not jam.
	TestTrue(TEXT("a lucky roll fires cleanly"),
		UHeatSinkStatics::Fire(Bei(80.0f), R, 0.99f).Outcome == EHeatShotOutcome::Fired);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
