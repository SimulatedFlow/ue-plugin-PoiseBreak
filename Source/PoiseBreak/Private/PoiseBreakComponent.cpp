// Copyright 2026 Silvan Teufel. All Rights Reserved.

#include "PoiseBreakComponent.h"

#include "PoiseBreakLog.h"
#include "PoiseBreakSettings.h"
#include "PoiseBreakStatics.h"

#include "GameFramework/Actor.h"

UPoiseBreakComponent::UPoiseBreakComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

void UPoiseBreakComponent::BeginPlay()
{
	Super::BeginPlay();

	const UPoiseBreakSettings* Settings = UPoiseBreakSettings::Get();
	State.Max = MaxPoise > 0.0f ? MaxPoise : Settings->DefaultMaxPoise;
	State.Current = State.Max;
	State.SecondsSinceHit = 1000.0f;

	OnPoiseChanged.Broadcast(State.Current, State.Max);
}

FPoiseBreakRules UPoiseBreakComponent::GetRules() const
{
	return bOverrideRules ? RuleOverride : UPoiseBreakSettings::Get()->Rules;
}

void UPoiseBreakComponent::TickComponent(const float DeltaTime, const ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bAutoTick)
	{
		AdvanceTime(DeltaTime);
	}
}

void UPoiseBreakComponent::AdvanceTime(const float DeltaSeconds)
{
	const float Before = State.Current;
	State = UPoiseBreakStatics::Advance(State, DeltaSeconds, GetRules());

	if (bWasStaggered && !UPoiseBreakStatics::IsStaggered(State))
	{
		bWasStaggered = false;
		OnStaggerEnded.Broadcast();
	}

	if (!FMath::IsNearlyEqual(Before, State.Current))
	{
		OnPoiseChanged.Broadcast(State.Current, State.Max);
	}
}

FPoiseBreakResult UPoiseBreakComponent::AddPoiseDamage(const float Amount, const bool bHyperArmor)
{
	const FPoiseBreakResult Result =
		UPoiseBreakStatics::ApplyPoiseDamage(State, Amount, bHyperArmor, GetRules());

	const float Before = State.Current;
	State = Result.State;

	if (Result.Outcome == EPoiseBreakOutcome::Broke)
	{
		BroadcastBreak(Result);
	}

	if (!FMath::IsNearlyEqual(Before, State.Current))
	{
		OnPoiseChanged.Broadcast(State.Current, State.Max);
	}

	return Result;
}

FPoiseBreakResult UPoiseBreakComponent::AddPoiseDamageAuto(const float Amount)
{
	return AddPoiseDamage(Amount, bHyperArmorActive);
}

void UPoiseBreakComponent::SetHyperArmor(const bool bEnabled)
{
	bHyperArmorActive = bEnabled;
}

void UPoiseBreakComponent::ResetPoise()
{
	State.Current = State.Max;
	State.SecondsSinceHit = 1000.0f;
	State.StaggerRemaining = 0.0f;
	State.ImmuneRemaining = 0.0f;
	bWasStaggered = false;
	OnPoiseChanged.Broadcast(State.Current, State.Max);
}

void UPoiseBreakComponent::SetMaxPoise(const float NewMax, const bool bRefill)
{
	State.Max = FMath::Max(1.0f, NewMax);
	State.Current = bRefill ? State.Max : FMath::Min(State.Current, State.Max);
	OnPoiseChanged.Broadcast(State.Current, State.Max);
}

void UPoiseBreakComponent::ForceBreak(const int32 TierIndex)
{
	const FPoiseBreakRules Rules = GetRules();
	const TArray<FPoiseTier> Sorted = UPoiseBreakStatics::NormaliseTiers(Rules.Tiers);
	if (!Sorted.IsValidIndex(TierIndex))
	{
		UE_LOG(LogPoiseBreak, Warning, TEXT("[%s] ForceBreak: no tier %d (there are %d)."),
			*GetNameSafe(GetOwner()), TierIndex, Sorted.Num());
		return;
	}

	const FPoiseTier& Tier = Sorted[TierIndex];

	// A scripted break ignores the immunity window on purpose: it is a moment the designer wrote,
	// not a hit that happened to land.
	State.LastTier = Tier.Name;
	State.BreakCount += 1;
	State.StaggerRemaining = Tier.StaggerSeconds;
	State.ImmuneRemaining = Tier.StaggerSeconds + Tier.ExtraImmunitySeconds;
	State.Current = Rules.bRefillOnBreak ? State.Max : 0.0f;
	State.SecondsSinceHit = 0.0f;

	FPoiseBreakResult Result;
	Result.State = State;
	Result.Outcome = EPoiseBreakOutcome::Broke;
	Result.TierIndex = TierIndex;
	Result.TierName = Tier.Name;
	Result.Overshoot = Tier.MinOvershoot;
	BroadcastBreak(Result);

	OnPoiseChanged.Broadcast(State.Current, State.Max);
}

void UPoiseBreakComponent::BroadcastBreak(const FPoiseBreakResult& Result)
{
	bWasStaggered = true;

	if (UPoiseBreakSettings::Get()->bLogBreaks)
	{
		UE_LOG(LogPoiseBreak, Display, TEXT("[%s] broke: %s (tier %d), overshoot %.1f, staggered %.2fs"),
			*GetNameSafe(GetOwner()), *Result.TierName.ToString(), Result.TierIndex,
			Result.Overshoot, Result.State.StaggerRemaining);
	}

	OnPoiseBroken.Broadcast(Result.TierName, Result.TierIndex, Result.Overshoot);
}

bool UPoiseBreakComponent::IsStaggered() const
{
	return UPoiseBreakStatics::IsStaggered(State);
}

bool UPoiseBreakComponent::IsImmune() const
{
	return UPoiseBreakStatics::IsImmune(State);
}

float UPoiseBreakComponent::GetPoiseFraction() const
{
	return UPoiseBreakStatics::PoiseFraction(State);
}
