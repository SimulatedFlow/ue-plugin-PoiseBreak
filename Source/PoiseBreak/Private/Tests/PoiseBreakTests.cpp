// Copyright 2026 Silvan Teufel. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "PoiseBreakStatics.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace PoiseBreakTests
{
	constexpr EAutomationTestFlags TestFlags = EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::CommandletContext
		| EAutomationTestFlags::EngineFilter;

	/** The rules the tests argue about, spelled out rather than read from the project settings. */
	static FPoiseBreakRules Rules(const bool bRefill = true, const bool bRegenWhileStaggered = false)
	{
		FPoiseBreakRules R;
		R.Tiers = UPoiseBreakStatics::DefaultTiers();   // Flinch 0, Stagger 15, Knockdown 40
		R.RegenDelaySeconds = 2.0f;
		R.RegenPerSecond = 25.0f;
		R.HyperArmorScale = 0.25f;
		R.bRefillOnBreak = bRefill;
		R.bRegenWhileStaggered = bRegenWhileStaggered;
		return R;
	}

	static FPoiseState Fresh(const float Max = 100.0f, const float Current = -1.0f)
	{
		FPoiseState S;
		S.Max = Max;
		S.Current = Current < 0.0f ? Max : Current;
		S.SecondsSinceHit = 1000.0f;
		return S;
	}
}

// -------------------------------------------------------------------------------------------------
// The rule the whole plugin is for.
// -------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPoiseBreakTierComesFromTheOvershoot,
	"PoiseBreak.Break.TierComesFromTheOvershootNotTheHit", PoiseBreakTests::TestFlags)

bool FPoiseBreakTierComesFromTheOvershoot::RunTest(const FString&)
{
	using namespace PoiseBreakTests;
	const FPoiseBreakRules R = Rules();

	// Twenty poise damage against a full pool: nothing happens.
	const FPoiseBreakResult Fresh20 = UPoiseBreakStatics::ApplyPoiseDamage(Fresh(), 20.0f, false, R);
	TestTrue(TEXT("twenty against a full pool holds"), Fresh20.Outcome == EPoiseBreakOutcome::Held);
	TestEqual(TEXT("and names no tier"), Fresh20.TierIndex, -1);

	// The SAME twenty against a pool with five left: a knockdown is not reached, but a Stagger is,
	// because the overshoot is fifteen.
	const FPoiseBreakResult Worn20 = UPoiseBreakStatics::ApplyPoiseDamage(Fresh(100.0f, 5.0f), 20.0f, false, R);
	TestTrue(TEXT("the same twenty against five left breaks"), Worn20.Outcome == EPoiseBreakOutcome::Broke);
	TestNearlyEqual(TEXT("overshoot is fifteen"), Worn20.Overshoot, 15.0f, 0.001f);
	TestEqual(TEXT("which is the Stagger tier"), Worn20.TierName, FName(TEXT("Stagger")));

	// A big hit against a nearly empty pool is a knockdown - not because it was big, but because it
	// went far past zero.
	const FPoiseBreakResult Worn60 = UPoiseBreakStatics::ApplyPoiseDamage(Fresh(100.0f, 5.0f), 60.0f, false, R);
	TestNearlyEqual(TEXT("overshoot is fifty-five"), Worn60.Overshoot, 55.0f, 0.001f);
	TestEqual(TEXT("Knockdown"), Worn60.TierName, FName(TEXT("Knockdown")));

	// And the same big hit against a full pool does not break at all. THIS is the pair that decides
	// whether trading blows is a decision or a coin toss: the hit is identical, the answer is not.
	const FPoiseBreakResult FullBig = UPoiseBreakStatics::ApplyPoiseDamage(Fresh(), 60.0f, false, R);
	TestTrue(TEXT("sixty against a full hundred still holds"), FullBig.Outcome == EPoiseBreakOutcome::Held);
	TestNearlyEqual(TEXT("and leaves forty"), FullBig.State.Current, 40.0f, 0.001f);

	// A hit that lands exactly on zero is a break with no overshoot - the smallest tier.
	const FPoiseBreakResult Exact = UPoiseBreakStatics::ApplyPoiseDamage(Fresh(100.0f, 10.0f), 10.0f, false, R);
	TestTrue(TEXT("landing exactly on zero is a break"), Exact.Outcome == EPoiseBreakOutcome::Broke);
	TestNearlyEqual(TEXT("with no overshoot"), Exact.Overshoot, 0.0f, 0.001f);
	TestEqual(TEXT("and the smallest tier"), Exact.TierName, FName(TEXT("Flinch")));

	return true;
}

// -------------------------------------------------------------------------------------------------
// Chain-lock.
// -------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPoiseBreakImmunityCoversTheStagger,
	"PoiseBreak.Break.ImmunityCoversTheStagger", PoiseBreakTests::TestFlags)

bool FPoiseBreakImmunityCoversTheStagger::RunTest(const FString&)
{
	using namespace PoiseBreakTests;
	const FPoiseBreakRules R = Rules();

	// Break it: Flinch, 0.35 s of stagger and 0.20 s on top.
	const FPoiseBreakResult First = UPoiseBreakStatics::ApplyPoiseDamage(Fresh(100.0f, 5.0f), 5.0f, false, R);
	TestEqual(TEXT("Flinch"), First.TierName, FName(TEXT("Flinch")));
	TestNearlyEqual(TEXT("staggered for the tier's length"), First.State.StaggerRemaining, 0.35f, 0.001f);
	TestNearlyEqual(TEXT("immune for the stagger plus the extra"), First.State.ImmuneRemaining, 0.55f, 0.001f);

	// A second attacker lands on the floored actor. It must do nothing at all - not even take poise
	// off, because a pool drained during the stagger would break again the frame the window closes.
	const FPoiseBreakResult During = UPoiseBreakStatics::ApplyPoiseDamage(First.State, 50.0f, false, R);
	TestTrue(TEXT("a hit during the immunity is refused"), During.Outcome == EPoiseBreakOutcome::Immune);
	TestNearlyEqual(TEXT("and the pool is untouched"), During.State.Current, First.State.Current, 0.001f);
	TestEqual(TEXT("and it is still one break, not two"), During.State.BreakCount, 1);

	// Once the window has fully run down, hits land again.
	FPoiseState Later = First.State;
	for (int32 Step = 0; Step < 6; ++Step)   // 6 x 0.1 s = 0.6 s > 0.55 s
	{
		Later = UPoiseBreakStatics::Advance(Later, 0.1f, R);
	}
	TestFalse(TEXT("no longer staggered"), UPoiseBreakStatics::IsStaggered(Later));
	TestFalse(TEXT("no longer immune"), UPoiseBreakStatics::IsImmune(Later));

	const FPoiseBreakResult After = UPoiseBreakStatics::ApplyPoiseDamage(Later, 10.0f, false, R);
	TestTrue(TEXT("and a hit lands again"), After.Outcome == EPoiseBreakOutcome::Held);

	return true;
}

// -------------------------------------------------------------------------------------------------
// Regeneration.
// -------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPoiseBreakNoRegenWhileStaggered,
	"PoiseBreak.Regen.NotWhileStaggered", PoiseBreakTests::TestFlags)

bool FPoiseBreakNoRegenWhileStaggered::RunTest(const FString&)
{
	using namespace PoiseBreakTests;

	// Refill off, so the pool is genuinely empty after the break and there is something to watch.
	const FPoiseBreakRules R = Rules(/*bRefill=*/false);

	const FPoiseBreakResult Broke = UPoiseBreakStatics::ApplyPoiseDamage(Fresh(100.0f, 5.0f), 25.0f, false, R);
	TestTrue(TEXT("it broke"), Broke.Outcome == EPoiseBreakOutcome::Broke);
	TestNearlyEqual(TEXT("and the pool is empty"), Broke.State.Current, 0.0f, 0.001f);
	TestNearlyEqual(TEXT("Stagger lasts 0.9 s"), Broke.State.StaggerRemaining, 0.9f, 0.001f);

	// Half a second into the stagger: the regeneration delay has not passed AND the actor is
	// staggered. Either reason alone is enough; the pool must not have moved.
	FPoiseState S = UPoiseBreakStatics::Advance(Broke.State, 0.5f, R);
	TestNearlyEqual(TEXT("nothing regenerates during the stagger"), S.Current, 0.0f, 0.001f);

	// Now run past the stagger but still inside the two-second delay.
	S = UPoiseBreakStatics::Advance(S, 1.0f, R);   // t = 1.5 s
	TestFalse(TEXT("the stagger is over"), UPoiseBreakStatics::IsStaggered(S));
	TestNearlyEqual(TEXT("but the delay still holds the pool down"), S.Current, 0.0f, 0.001f);

	// Past the delay: twenty-five a second.
	S = UPoiseBreakStatics::Advance(S, 1.0f, R);   // t = 2.5 s, one second of regen
	TestNearlyEqual(TEXT("a second of regeneration is twenty-five"), S.Current, 25.0f, 0.001f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPoiseBreakRegenIsStepSizeIndependent,
	"PoiseBreak.Regen.TwoHalfStepsEqualOneWholeStep", PoiseBreakTests::TestFlags)

bool FPoiseBreakRegenIsStepSizeIndependent::RunTest(const FString&)
{
	using namespace PoiseBreakTests;
	const FPoiseBreakRules R = Rules();

	// A frame rate that changes must not change the fight. Linear regeneration is the reason this
	// holds exactly rather than nearly.
	FPoiseState A = Fresh(100.0f, 10.0f);
	A.SecondsSinceHit = 10.0f;
	FPoiseState B = A;

	A = UPoiseBreakStatics::Advance(A, 1.0f, R);

	B = UPoiseBreakStatics::Advance(B, 0.5f, R);
	B = UPoiseBreakStatics::Advance(B, 0.5f, R);

	TestNearlyEqual(TEXT("one whole step and two half steps agree"), A.Current, B.Current, 0.0001f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPoiseBreakEveryEffectiveHitResetsTheDelay,
	"PoiseBreak.Regen.EveryEffectiveHitResetsTheDelay", PoiseBreakTests::TestFlags)

bool FPoiseBreakEveryEffectiveHitResetsTheDelay::RunTest(const FString&)
{
	using namespace PoiseBreakTests;
	const FPoiseBreakRules R = Rules();

	// Chip damage that never breaks anything still has to count as pressure. If only breaking hits
	// reset the delay, a stream of small hits lets the pool refill underneath them and the enemy
	// never staggers at all.
	FPoiseState S = Fresh(100.0f, 50.0f);
	S.SecondsSinceHit = 10.0f;

	const FPoiseBreakResult Chip = UPoiseBreakStatics::ApplyPoiseDamage(S, 5.0f, false, R);
	TestTrue(TEXT("the chip holds"), Chip.Outcome == EPoiseBreakOutcome::Held);
	TestNearlyEqual(TEXT("and the delay is back to zero"), Chip.State.SecondsSinceHit, 0.0f, 0.001f);

	// One second later - still inside the two-second delay - nothing has come back.
	const FPoiseState Soon = UPoiseBreakStatics::Advance(Chip.State, 1.0f, R);
	TestNearlyEqual(TEXT("no regeneration yet"), Soon.Current, 45.0f, 0.001f);

	// A hit that arrives as nothing must NOT reset the delay: otherwise a hyper-armor scale of zero
	// becomes a way of holding an enemy's pool down forever.
	const FPoiseBreakResult Nothing = UPoiseBreakStatics::ApplyPoiseDamage(Soon, 0.0f, false, R);
	TestTrue(TEXT("zero damage is a non-event"), Nothing.Outcome == EPoiseBreakOutcome::Nothing);
	TestNearlyEqual(TEXT("and leaves the delay running"), Nothing.State.SecondsSinceHit,
		Soon.SecondsSinceHit, 0.001f);

	return true;
}

// -------------------------------------------------------------------------------------------------
// Hyper armor.
// -------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPoiseBreakHyperArmorScalesPoiseOnly,
	"PoiseBreak.HyperArmor.ScalesWhatArrivesAtThePool", PoiseBreakTests::TestFlags)

bool FPoiseBreakHyperArmorScalesPoiseOnly::RunTest(const FString&)
{
	using namespace PoiseBreakTests;
	const FPoiseBreakRules R = Rules();   // scale 0.25

	TestNearlyEqual(TEXT("without hyper armor, all of it arrives"),
		UPoiseBreakStatics::EffectivePoiseDamage(40.0f, false, 0.25f), 40.0f, 0.001f);

	TestNearlyEqual(TEXT("with hyper armor, a quarter"),
		UPoiseBreakStatics::EffectivePoiseDamage(40.0f, true, 0.25f), 10.0f, 0.001f);

	// The pair that shows what the window is worth: forty against a pool with thirty left breaks
	// without hyper armor and holds with it.
	const FPoiseBreakResult Without = UPoiseBreakStatics::ApplyPoiseDamage(Fresh(100.0f, 30.0f), 40.0f, false, R);
	TestTrue(TEXT("without: break"), Without.Outcome == EPoiseBreakOutcome::Broke);

	const FPoiseBreakResult With = UPoiseBreakStatics::ApplyPoiseDamage(Fresh(100.0f, 30.0f), 40.0f, true, R);
	TestTrue(TEXT("with: holds"), With.Outcome == EPoiseBreakOutcome::Held);
	TestNearlyEqual(TEXT("and twenty is left"), With.State.Current, 20.0f, 0.001f);

	// A scale above one is allowed: "this window makes you more fragile" is a thing a designer may
	// legitimately want, and the plugin has not earned an opinion about it.
	TestNearlyEqual(TEXT("a scale above one is not clamped away"),
		UPoiseBreakStatics::EffectivePoiseDamage(10.0f, true, 2.0f), 20.0f, 0.001f);

	return true;
}

// -------------------------------------------------------------------------------------------------
// Tier list handling.
// -------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPoiseBreakTiersAreSortedNotTrusted,
	"PoiseBreak.Tiers.SortedNotTrusted", PoiseBreakTests::TestFlags)

bool FPoiseBreakTiersAreSortedNotTrusted::RunTest(const FString&)
{
	using namespace PoiseBreakTests;

	TArray<FPoiseTier> Muddled;
	FPoiseTier Big;   Big.Name = TEXT("Big");   Big.MinOvershoot = 40.0f;
	FPoiseTier Small; Small.Name = TEXT("Small"); Small.MinOvershoot = -5.0f;   // negative on purpose
	FPoiseTier Mid;   Mid.Name = TEXT("Mid");   Mid.MinOvershoot = 15.0f;
	Muddled.Add(Big);
	Muddled.Add(Small);
	Muddled.Add(Mid);

	const TArray<FPoiseTier> Sorted = UPoiseBreakStatics::NormaliseTiers(Muddled);
	TestEqual(TEXT("three tiers survive"), Sorted.Num(), 3);
	TestEqual(TEXT("smallest first"), Sorted[0].Name, FName(TEXT("Small")));
	TestEqual(TEXT("then the middle"), Sorted[1].Name, FName(TEXT("Mid")));
	TestEqual(TEXT("then the big one"), Sorted[2].Name, FName(TEXT("Big")));
	TestNearlyEqual(TEXT("the negative threshold is clamped to zero"), Sorted[0].MinOvershoot, 0.0f, 0.001f);

	TestEqual(TEXT("an overshoot of nothing still picks the smallest tier"),
		UPoiseBreakStatics::TierIndexFor(0.0f, Sorted), 0);
	TestEqual(TEXT("twenty reaches the middle"),
		UPoiseBreakStatics::TierIndexFor(20.0f, Sorted), 1);
	TestEqual(TEXT("a hundred reaches the top and stops there"),
		UPoiseBreakStatics::TierIndexFor(100.0f, Sorted), 2);
	TestEqual(TEXT("an empty list names no tier"),
		UPoiseBreakStatics::TierIndexFor(50.0f, TArray<FPoiseTier>()), -1);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
