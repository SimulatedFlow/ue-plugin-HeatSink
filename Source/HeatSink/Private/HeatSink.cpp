// Copyright 2026 Silvan Teufel. All Rights Reserved.

#include "HeatSink.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "HAL/IConsoleManager.h"
#include "HeatSinkComponent.h"
#include "HeatSinkLog.h"

DEFINE_LOG_CATEGORY(LogHeatSink);

#define LOCTEXT_NAMESPACE "FHeatSinkModule"

namespace
{
	UWorld* HeatSinkConsoleWorld()
	{
		if (!GEngine)
		{
			return nullptr;
		}
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			if (Context.World() && (Context.WorldType == EWorldType::PIE || Context.WorldType == EWorldType::Game))
			{
				return Context.World();
			}
		}
		return nullptr;
	}

	void HeatSinkDumpCommand()
	{
		UWorld* World = HeatSinkConsoleWorld();
		if (!World)
		{
			UE_LOG(LogHeatSink, Warning, TEXT("HeatSink.Dump: no running world."));
			return;
		}

		int32 Gefunden = 0;
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			const UHeatSinkComponent* H = It->FindComponentByClass<UHeatSinkComponent>();
			if (!H)
			{
				continue;
			}
			++Gefunden;
			const FHeatState& S = H->GetState();
			UE_LOG(LogHeatSink, Display,
				TEXT("%-24s heat %3.0f%%  %s  shots %5d  overheats %3d  jams %3d  jam chance %.1f%%"),
				*It->GetName(), H->GetHeatFraction() * 100.0f,
				H->CanFire() ? TEXT("ready       ")
					: *FString::Printf(TEXT("%s %.2fs"),
						S.bJammed ? TEXT("JAMMED ") : (S.LockoutRemaining > 0.0f ? TEXT("LOCKED ") : TEXT("VENTING")),
						H->GetSecondsUntilReady()),
				S.ShotsFired, S.Overheats, S.Jams, H->GetJamChance() * 100.0f);
		}

		if (Gefunden == 0)
		{
			UE_LOG(LogHeatSink, Display, TEXT("HeatSink.Dump: nothing in this level has a heat component."));
		}
	}

	FAutoConsoleCommand GHeatSinkDump(
		TEXT("HeatSink.Dump"),
		TEXT("Every weapon with a heat component: heat, state, shots, overheats, jams."),
		FConsoleCommandDelegate::CreateStatic(&HeatSinkDumpCommand));
}

void FHeatSinkModule::StartupModule()
{
}

void FHeatSinkModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FHeatSinkModule, HeatSink)
