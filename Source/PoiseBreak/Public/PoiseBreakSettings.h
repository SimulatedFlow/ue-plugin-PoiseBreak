// Copyright 2026 Silvan Teufel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "PoiseBreakTypes.h"
#include "PoiseBreakSettings.generated.h"

/** Project Settings > Plugins > PoiseBreak. The defaults every new component starts from. */
UCLASS(config = Game, defaultconfig, meta = (DisplayName = "PoiseBreak"))
class POISEBREAK_API UPoiseBreakSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UPoiseBreakSettings();

	virtual FName GetContainerName() const override { return TEXT("Project"); }
	virtual FName GetCategoryName() const override { return TEXT("Plugins"); }

	static const UPoiseBreakSettings* Get();

	/** The project-wide rules. A component may override them per actor. */
	UPROPERTY(config, EditAnywhere, Category = "Rules")
	FPoiseBreakRules Rules;

	/** Poise pool a component starts with when nothing else is set on it. */
	UPROPERTY(config, EditAnywhere, Category = "Rules", meta = (ClampMin = "1.0"))
	float DefaultMaxPoise;

	/** Write a line to the log for every break. Off in shipping builds is the usual choice. */
	UPROPERTY(config, EditAnywhere, Category = "Diagnostics")
	bool bLogBreaks;
};
