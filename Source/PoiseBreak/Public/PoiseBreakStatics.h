// Copyright 2026 Silvan Teufel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PoiseBreakTypes.h"
#include "PoiseBreakStatics.generated.h"

/**
 * The rules, on their own.
 *
 * No world, no actor, no clock. The component calls exactly these functions and so do the tests,
 * which is the only way to be sure the meter on screen and the plugin agree.
 */
UCLASS()
class POISEBREAK_API UPoiseBreakStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Poise damage after hyper armor.
	 *
	 * Hyper armor scales what arrives at the POOL. It has nothing to say about health damage - a
	 * boss with hyper armor still bleeds, it just does not flinch. Folding the two together is the
	 * single most common way this feature is got wrong.
	 */
	UFUNCTION(BlueprintPure, Category = "PoiseBreak|Rules")
	static float EffectivePoiseDamage(float Amount, bool bHyperArmor, float HyperArmorScale);

	/**
	 * Tiers sorted ascending by MinOvershoot, with negatives clamped away.
	 *
	 * Duplicates are left alone on purpose. Two tiers at the same threshold both qualify and the
	 * later one wins, which is a sensible thing to write when a project generates its tiers.
	 */
	UFUNCTION(BlueprintPure, Category = "PoiseBreak|Rules")
	static TArray<FPoiseTier> NormaliseTiers(const TArray<FPoiseTier>& Tiers);

	/**
	 * Index of the tier this overshoot reaches, or -1 if the list is empty.
	 *
	 * The LAST tier whose MinOvershoot the overshoot still reaches - so the list must be sorted,
	 * which is what NormaliseTiers is for. An overshoot below every threshold still produces tier 0:
	 * the actor broke, and "broke, but only just" is a tier, not a non-event.
	 */
	UFUNCTION(BlueprintPure, Category = "PoiseBreak|Rules")
	static int32 TierIndexFor(float Overshoot, const TArray<FPoiseTier>& SortedTiers);

	/**
	 * One hit against the pool.
	 *
	 * THE RULE THAT MATTERS: the tier comes from the overshoot, not from Amount. Twenty points of
	 * poise damage is nothing to a full pool and a knockdown to one with five left, and that is what
	 * makes trading blows a decision instead of a coin toss.
	 *
	 * While the immunity window of an earlier break is open the hit is ignored ENTIRELY - the pool
	 * does not even take the damage. Letting it accumulate would only mean breaking again the
	 * instant the window closed, which is exactly the chain-lock the window exists to prevent.
	 */
	UFUNCTION(BlueprintCallable, Category = "PoiseBreak|Rules")
	static FPoiseBreakResult ApplyPoiseDamage(const FPoiseState& State, float Amount,
		bool bHyperArmor, const FPoiseBreakRules& Rules);

	/**
	 * Time passing: the stagger and the immunity run down, and the pool refills once the delay has
	 * passed. Linear, so two half-steps and one whole step give the same answer.
	 */
	UFUNCTION(BlueprintCallable, Category = "PoiseBreak|Rules")
	static FPoiseState Advance(const FPoiseState& State, float DeltaSeconds, const FPoiseBreakRules& Rules);

	UFUNCTION(BlueprintPure, Category = "PoiseBreak|Rules")
	static bool IsStaggered(const FPoiseState& State);

	UFUNCTION(BlueprintPure, Category = "PoiseBreak|Rules")
	static bool IsImmune(const FPoiseState& State);

	/** 0..1 for a meter. Zero Max reads as empty rather than dividing by nothing. */
	UFUNCTION(BlueprintPure, Category = "PoiseBreak|Rules")
	static float PoiseFraction(const FPoiseState& State);

	/** The default tier list: Flinch at 0, Stagger at 15, Knockdown at 40. */
	UFUNCTION(BlueprintPure, Category = "PoiseBreak|Rules")
	static TArray<FPoiseTier> DefaultTiers();
};
