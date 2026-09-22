// Copyright 2026 Silvan Teufel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PoiseBreakTypes.generated.h"

/**
 * How hard the break was.
 *
 * The tier is NOT a property of the hit. It comes from the overshoot - how far past zero the poise
 * pool was pushed - which is the whole point of the plugin. See UPoiseBreakStatics::TierIndexFor.
 */
UENUM(BlueprintType)
enum class EPoiseBreakOutcome : uint8
{
	/** The pool took the damage and held. */
	Held,
	/** The pool was pushed past zero and the actor broke. */
	Broke,
	/** Ignored: the actor is inside the immunity window of an earlier break. */
	Immune,
	/** Nothing arrived - zero or negative poise damage, or hyper armor scaled it to nothing. */
	Nothing,
};

/**
 * One break tier.
 *
 * Tiers are chosen by overshoot, ascending. The tier with the largest MinOvershoot that the
 * overshoot still reaches wins, so the first tier should sit at zero - it is the one that catches
 * every break that is not big enough to be anything else.
 */
USTRUCT(BlueprintType)
struct POISEBREAK_API FPoiseTier
{
	GENERATED_BODY()

	/** Shown in the log and handed to your project in OnPoiseBroken. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PoiseBreak")
	FName Name = NAME_None;

	/** Smallest overshoot that reaches this tier. Inclusive. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PoiseBreak", meta = (ClampMin = "0.0"))
	float MinOvershoot = 0.0f;

	/** How long the actor stays staggered. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PoiseBreak", meta = (ClampMin = "0.0"))
	float StaggerSeconds = 0.35f;

	/**
	 * How long AFTER the stagger ends poise damage is still ignored.
	 *
	 * The immunity always covers the stagger itself as well - this is the extra on top. Without it,
	 * the next hit lands on a full pool a frame after the actor stands up, and four attackers each
	 * landing the smallest possible break hold it still forever.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PoiseBreak", meta = (ClampMin = "0.0"))
	float ExtraImmunitySeconds = 0.2f;
};

/**
 * Everything the rules need to know, in one struct.
 *
 * Passed to the pure functions explicitly rather than read from the settings inside them: the tests
 * hand in their own, and a project that wants per-enemy rules can too.
 */
USTRUCT(BlueprintType)
struct POISEBREAK_API FPoiseBreakRules
{
	GENERATED_BODY()

	/** Tiers, ascending by MinOvershoot. NormaliseTiers puts them in order and keeps them there. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PoiseBreak")
	TArray<FPoiseTier> Tiers;

	/** Seconds without an effective poise hit before the pool starts filling again. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PoiseBreak", meta = (ClampMin = "0.0"))
	float RegenDelaySeconds = 2.0f;

	/** Points per second once the delay has passed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PoiseBreak", meta = (ClampMin = "0.0"))
	float RegenPerSecond = 25.0f;

	/** Incoming poise damage is multiplied by this while a hyper-armor window is open. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PoiseBreak", meta = (ClampMin = "0.0"))
	float HyperArmorScale = 0.25f;

	/**
	 * Refill the pool to full when it breaks.
	 *
	 * On (the default) is the Souls-like reading: the break IS the punishment and the actor gets a
	 * clean pool afterwards. Off is the posture reading: the pool stays empty and has to regenerate,
	 * so a second break follows quickly if the pressure keeps up.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PoiseBreak")
	bool bRefillOnBreak = true;

	/** Let the pool regenerate while the actor is staggered. Off by default, and see the docs for why. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PoiseBreak")
	bool bRegenWhileStaggered = false;
};

/** The pool of one actor. Plain data: no actor, no world, no clock. */
USTRUCT(BlueprintType)
struct POISEBREAK_API FPoiseState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PoiseBreak", meta = (ClampMin = "0.0"))
	float Max = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PoiseBreak")
	float Current = 100.0f;

	/** Since the last hit that actually took poise off. Drives the regeneration delay. */
	UPROPERTY(BlueprintReadOnly, Category = "PoiseBreak")
	float SecondsSinceHit = 1000.0f;

	/** Greater than zero means staggered. */
	UPROPERTY(BlueprintReadOnly, Category = "PoiseBreak")
	float StaggerRemaining = 0.0f;

	/** Greater than zero means poise damage is ignored. Covers the stagger plus the tier's extra. */
	UPROPERTY(BlueprintReadOnly, Category = "PoiseBreak")
	float ImmuneRemaining = 0.0f;

	/** Name of the tier of the last break, for the log and the HUD. */
	UPROPERTY(BlueprintReadOnly, Category = "PoiseBreak")
	FName LastTier = NAME_None;

	/** How many times this actor has broken. Handy for "stagger resistance grows" rules. */
	UPROPERTY(BlueprintReadOnly, Category = "PoiseBreak")
	int32 BreakCount = 0;
};

/** What one call to ApplyPoiseDamage did. */
USTRUCT(BlueprintType)
struct POISEBREAK_API FPoiseBreakResult
{
	GENERATED_BODY()

	/** The state after the hit. Assign it back; the functions never mutate what they are given. */
	UPROPERTY(BlueprintReadOnly, Category = "PoiseBreak")
	FPoiseState State;

	UPROPERTY(BlueprintReadOnly, Category = "PoiseBreak")
	EPoiseBreakOutcome Outcome = EPoiseBreakOutcome::Nothing;

	/** Index into the normalised tier list, or -1 when nothing broke. */
	UPROPERTY(BlueprintReadOnly, Category = "PoiseBreak")
	int32 TierIndex = -1;

	UPROPERTY(BlueprintReadOnly, Category = "PoiseBreak")
	FName TierName = NAME_None;

	/** How far past zero the pool was pushed. Zero unless the actor broke. */
	UPROPERTY(BlueprintReadOnly, Category = "PoiseBreak")
	float Overshoot = 0.0f;

	/** The poise damage that actually arrived, after hyper armor. */
	UPROPERTY(BlueprintReadOnly, Category = "PoiseBreak")
	float AppliedPoiseDamage = 0.0f;
};
