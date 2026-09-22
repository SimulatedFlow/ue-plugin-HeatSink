// Copyright 2026 Silvan Teufel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HeatSinkTypes.h"
#include "HeatSinkDemoDirector.generated.h"

class UTextRenderComponent;

/**
 * Runs the shipped demo: two weapons under the SAME heat rules, fired differently.
 *
 * The left one holds the trigger down, the right one fires bursts and vents. Both are handed the
 * same pseudo-random sequence from a fixed seed, so the jam on the left is not luck and the picture
 * shows trigger discipline rather than two different runs.
 *
 * Every decision comes from UHeatSinkStatics, which is the plugin; the component is a clock around
 * exactly those calls, and the tests call them too.
 */
UCLASS(meta = (DisplayName = "HeatSink Demo Director"))
class HEATSINK_API AHeatSinkDemoDirector : public AActor
{
	GENERATED_BODY()

public:
	AHeatSinkDemoDirector();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual bool ShouldTickIfViewportsOnly() const override { return true; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HeatSink Demo",
		meta = (ClampMin = "16.0", ClampMax = "180.0"))
	float CycleSeconds = 28.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HeatSink Demo")
	bool bDrawDemo = true;

	/** Let Tick drive the demo. Off when something else steps it - see StepDemo. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HeatSink Demo")
	bool bAutoRun = true;

	/**
	 * Advance the demo by exactly this many seconds and redraw.
	 *
	 * An actor only ticks in an editor viewport while that viewport is set to realtime, which is the
	 * user's setting and not the plugin's. The screenshot run steps the demo by hand instead.
	 */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "HeatSink Demo")
	void StepDemo(float Seconds);

	/** The headline. TextRender, because HighResShot does not capture DrawDebugString. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HeatSink Demo")
	TObjectPtr<UTextRenderComponent> BoardText;

	/** Both weapons, in words. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HeatSink Demo")
	TObjectPtr<UTextRenderComponent> StateText;

	/** The rule the current phase is showing. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HeatSink Demo")
	TObjectPtr<UTextRenderComponent> RuleText;

private:
	/** One weapon: the plugin's state, plus what the run has looked like. */
	struct FWaffe
	{
		FHeatState Stand;
		FString Name;
		/** true: holds the trigger down. false: fires bursts and vents in the gaps. */
		bool bHaeltDrauf = false;
		float SeitSchuss = 0.0f;
		/**
		 * How long the weapon has been jammed and not yet being cleared.
		 *
		 * Its own counter, NOT SeitSchuss: the panicking shooter pulls the trigger every hundred
		 * milliseconds, which resets SeitSchuss forever, so a jam gated on that timer is never
		 * cleared at all. The first take spent half its cycle on a dead weapon reporting a hundred
		 * blocked shots.
		 */
		float SeitKlemmt = 0.0f;
		int32 ImStoss = 0;
		int32 Blockiert = 0;
		/** The last outcomes, for the strip under the bar. */
		TArray<EHeatShotOutcome> Verlauf;
		bool bWollteSchiessen = false;
	};

	void StartCycle();
	/** One fixed sub-step. Never called with anything but Takt - see StepDemo. */
	void EinTakt();
	void Ausloesen(FWaffe& W, float Seconds);
	float NaechsteZufallszahl();
	FString BuildStateText() const;
	FString HeadlineFor() const;
	FString RuleFor() const;
	void DrawBars() const;

	FVector Basis() const;

	FWaffe Dauerfeuer;
	FWaffe Diszipliniert;

	/** A tiny fixed-seed generator, so the demo plays out the same way on every take. */
	uint32 Zustand = 0;

	float CycleTime = 0.0f;
	/** Time handed in but not yet consumed by a whole sub-step. */
	float Rest = 0.0f;
	bool bBereit = false;

	FHeatRules Regeln;

	/**
	 * The demo runs on its OWN fixed clock, not on the caller's step size.
	 *
	 * The rules are step-size independent, but the trigger is not: a shooter who pulls every
	 * 110 ms lands on different milliseconds under a 16 ms step than under a 117 ms one, and a
	 * different millisecond means a different random draw for the jam. The video and the store
	 * images are taken with different step sizes, and without this they showed two different
	 * runs of the same demo - one with jams and one without.
	 */
	static constexpr float Takt = 1.0f / 120.0f;
	static constexpr float SekundenJeSchuss = 0.11f;
	static constexpr int32 StossLaenge = 5;
};
