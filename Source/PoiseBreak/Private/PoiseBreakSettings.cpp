// Copyright 2026 Silvan Teufel. All Rights Reserved.

#include "PoiseBreakSettings.h"

#include "PoiseBreakStatics.h"

UPoiseBreakSettings::UPoiseBreakSettings()
	: DefaultMaxPoise(100.0f)
	, bLogBreaks(false)
{
	Rules.Tiers = UPoiseBreakStatics::DefaultTiers();

	// Two seconds before the pool starts filling, then twenty-five a second: a fresh hundred-point
	// pool is back in four seconds of being left alone. Long enough that pressure means something,
	// short enough that a fight which moves on does not leave an enemy permanently brittle.
	Rules.RegenDelaySeconds = 2.0f;
	Rules.RegenPerSecond = 25.0f;

	// A quarter. Hyper armor should make trading favourable, not free.
	Rules.HyperArmorScale = 0.25f;

	Rules.bRefillOnBreak = true;
	Rules.bRegenWhileStaggered = false;
}

const UPoiseBreakSettings* UPoiseBreakSettings::Get()
{
	const UPoiseBreakSettings* Settings = GetDefault<UPoiseBreakSettings>();
	check(Settings);
	return Settings;
}
