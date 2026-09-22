// Copyright 2026 Silvan Teufel. All Rights Reserved.

#include "PoiseBreakStatics.h"

float UPoiseBreakStatics::EffectivePoiseDamage(const float Amount, const bool bHyperArmor,
	const float HyperArmorScale)
{
	if (Amount <= 0.0f)
	{
		return 0.0f;
	}
	if (!bHyperArmor)
	{
		return Amount;
	}
	// Clamped at zero, not at one: a scale above one is a legitimate thing to write ("this window
	// makes you MORE fragile"), and refusing it would be the plugin having an opinion it has not
	// earned.
	return Amount * FMath::Max(0.0f, HyperArmorScale);
}

TArray<FPoiseTier> UPoiseBreakStatics::NormaliseTiers(const TArray<FPoiseTier>& Tiers)
{
	TArray<FPoiseTier> Out = Tiers;
	for (FPoiseTier& T : Out)
	{
		T.MinOvershoot = FMath::Max(0.0f, T.MinOvershoot);
		T.StaggerSeconds = FMath::Max(0.0f, T.StaggerSeconds);
		T.ExtraImmunitySeconds = FMath::Max(0.0f, T.ExtraImmunitySeconds);
	}
	Out.StableSort([](const FPoiseTier& A, const FPoiseTier& B)
	{
		return A.MinOvershoot < B.MinOvershoot;
	});
	return Out;
}

int32 UPoiseBreakStatics::TierIndexFor(const float Overshoot, const TArray<FPoiseTier>& SortedTiers)
{
	if (SortedTiers.Num() == 0)
	{
		return INDEX_NONE;
	}

	// Walk forward and keep the last one that still qualifies. Tier 0 is the floor: an overshoot
	// smaller than every threshold is still a break, and it gets the smallest tier rather than
	// silently becoming a non-event.
	int32 Best = 0;
	for (int32 Index = 0; Index < SortedTiers.Num(); ++Index)
	{
		if (Overshoot >= SortedTiers[Index].MinOvershoot)
		{
			Best = Index;
		}
		else
		{
			break;
		}
	}
	return Best;
}

FPoiseBreakResult UPoiseBreakStatics::ApplyPoiseDamage(const FPoiseState& State, const float Amount,
	const bool bHyperArmor, const FPoiseBreakRules& Rules)
{
	FPoiseBreakResult Result;
	Result.State = State;

	if (State.ImmuneRemaining > 0.0f)
	{
		Result.Outcome = EPoiseBreakOutcome::Immune;
		return Result;
	}

	const float Applied = EffectivePoiseDamage(Amount, bHyperArmor, Rules.HyperArmorScale);
	Result.AppliedPoiseDamage = Applied;

	if (Applied <= 0.0f)
	{
		// The regeneration delay is NOT reset here. A hit that took no poise off is not pressure,
		// and letting it hold the pool down would make a hyper-armor scale of zero into a way of
		// keeping an enemy permanently unable to recover.
		Result.Outcome = EPoiseBreakOutcome::Nothing;
		return Result;
	}

	Result.State.Current = State.Current - Applied;
	Result.State.SecondsSinceHit = 0.0f;

	if (Result.State.Current > 0.0f)
	{
		Result.Outcome = EPoiseBreakOutcome::Held;
		return Result;
	}

	// --- the break -----------------------------------------------------------------------------
	const float Overshoot = -Result.State.Current;
	const TArray<FPoiseTier> Sorted = NormaliseTiers(Rules.Tiers);
	const int32 Index = TierIndexFor(Overshoot, Sorted);

	Result.Outcome = EPoiseBreakOutcome::Broke;
	Result.Overshoot = Overshoot;
	Result.TierIndex = Index;

	const FPoiseTier Tier = Sorted.IsValidIndex(Index) ? Sorted[Index] : FPoiseTier();
	Result.TierName = Tier.Name;

	Result.State.LastTier = Tier.Name;
	Result.State.BreakCount = State.BreakCount + 1;
	Result.State.StaggerRemaining = Tier.StaggerSeconds;

	// The immunity covers the stagger AND the extra afterwards. Anything less and a hit landing
	// while the actor is on the floor breaks it again before it can stand up.
	Result.State.ImmuneRemaining = Tier.StaggerSeconds + Tier.ExtraImmunitySeconds;

	Result.State.Current = Rules.bRefillOnBreak ? State.Max : 0.0f;

	return Result;
}

FPoiseState UPoiseBreakStatics::Advance(const FPoiseState& State, const float DeltaSeconds,
	const FPoiseBreakRules& Rules)
{
	FPoiseState Out = State;
	if (DeltaSeconds <= 0.0f)
	{
		return Out;
	}

	Out.SecondsSinceHit = State.SecondsSinceHit + DeltaSeconds;
	Out.StaggerRemaining = FMath::Max(0.0f, State.StaggerRemaining - DeltaSeconds);
	Out.ImmuneRemaining = FMath::Max(0.0f, State.ImmuneRemaining - DeltaSeconds);

	const bool bBlocked = IsStaggered(State) && !Rules.bRegenWhileStaggered;
	if (!bBlocked && Out.SecondsSinceHit >= Rules.RegenDelaySeconds && Rules.RegenPerSecond > 0.0f)
	{
		Out.Current = FMath::Min(Out.Max, Out.Current + Rules.RegenPerSecond * DeltaSeconds);
	}

	return Out;
}

bool UPoiseBreakStatics::IsStaggered(const FPoiseState& State)
{
	return State.StaggerRemaining > 0.0f;
}

bool UPoiseBreakStatics::IsImmune(const FPoiseState& State)
{
	return State.ImmuneRemaining > 0.0f;
}

float UPoiseBreakStatics::PoiseFraction(const FPoiseState& State)
{
	if (State.Max <= 0.0f)
	{
		return 0.0f;
	}
	return FMath::Clamp(State.Current / State.Max, 0.0f, 1.0f);
}

TArray<FPoiseTier> UPoiseBreakStatics::DefaultTiers()
{
	TArray<FPoiseTier> Tiers;

	FPoiseTier Flinch;
	Flinch.Name = TEXT("Flinch");
	Flinch.MinOvershoot = 0.0f;
	Flinch.StaggerSeconds = 0.35f;
	Flinch.ExtraImmunitySeconds = 0.20f;
	Tiers.Add(Flinch);

	FPoiseTier Stagger;
	Stagger.Name = TEXT("Stagger");
	Stagger.MinOvershoot = 15.0f;
	Stagger.StaggerSeconds = 0.90f;
	Stagger.ExtraImmunitySeconds = 0.60f;
	Tiers.Add(Stagger);

	FPoiseTier Knockdown;
	Knockdown.Name = TEXT("Knockdown");
	Knockdown.MinOvershoot = 40.0f;
	Knockdown.StaggerSeconds = 2.00f;
	Knockdown.ExtraImmunitySeconds = 1.20f;
	Tiers.Add(Knockdown);

	return Tiers;
}
