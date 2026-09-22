// Copyright 2026 Silvan Teufel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "HeatSinkTypes.h"
#include "HeatSinkSettings.generated.h"

/** Project Settings > Plugins > HeatSink. */
UCLASS(config = Game, defaultconfig, meta = (DisplayName = "HeatSink"))
class HEATSINK_API UHeatSinkSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UHeatSinkSettings();

	virtual FName GetContainerName() const override { return TEXT("Project"); }
	virtual FName GetCategoryName() const override { return TEXT("Plugins"); }

	static const UHeatSinkSettings* Get();

	UPROPERTY(config, EditAnywhere, Category = "Rules")
	FHeatRules Rules;

	/** Write a line for every overheat and every jam. */
	UPROPERTY(config, EditAnywhere, Category = "Diagnostics")
	bool bLogEvents;
};
