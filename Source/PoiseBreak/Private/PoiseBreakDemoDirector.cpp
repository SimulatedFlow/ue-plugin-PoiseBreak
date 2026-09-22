// Copyright 2026 Silvan Teufel. All Rights Reserved.

#include "PoiseBreakDemoDirector.h"

#include "Components/TextRenderComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "PoiseBreakComponent.h"
#include "PoiseBreakStatics.h"

namespace PoiseBreakDemoLocal
{
	/** Where the three dummies stand. Horizontal is X - Y is depth, and the camera looks along +Y. */
	constexpr float SlotX[3] = {-760.0f, 0.0f, 760.0f};
	const TCHAR* SlotName[3] = {TEXT("FRESH"), TEXT("WORN"), TEXT("ARMORED")};

	const FColor Held(110, 210, 130);
	const FColor Low(235, 190, 80);
	const FColor Broken(230, 95, 95);
	const FColor Immune(120, 150, 230);
	const FColor Frame(60, 62, 72);
}

APoiseBreakDemoDirector::APoiseBreakDemoDirector()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Fresh = CreateDefaultSubobject<UPoiseBreakComponent>(TEXT("Fresh"));
	Worn = CreateDefaultSubobject<UPoiseBreakComponent>(TEXT("Worn"));
	Armored = CreateDefaultSubobject<UPoiseBreakComponent>(TEXT("Armored"));

	BoardText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("BoardText"));
	BoardText->SetupAttachment(Root);
	BoardText->SetHorizontalAlignment(EHTA_Center);
	BoardText->SetVerticalAlignment(EVRTA_TextBottom);
	BoardText->SetWorldSize(54.0f);
	BoardText->SetTextRenderColor(FColor::White);
	// Yaw 270, not 90: at 90 a TextRender renders mirrored.
	BoardText->SetRelativeRotation(FRotator(0.0f, 270.0f, 0.0f));
	BoardText->SetRelativeLocation(FVector(0.0f, 0.0f, 770.0f));

	StateText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("StateText"));
	StateText->SetupAttachment(Root);
	StateText->SetHorizontalAlignment(EHTA_Center);
	StateText->SetVerticalAlignment(EVRTA_TextTop);
	StateText->SetWorldSize(33.0f);
	StateText->SetTextRenderColor(FColor(205, 212, 226));
	StateText->SetRelativeRotation(FRotator(0.0f, 270.0f, 0.0f));
	StateText->SetRelativeLocation(FVector(-700.0f, 0.0f, 672.0f));

	RuleText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("RuleText"));
	RuleText->SetupAttachment(Root);
	RuleText->SetHorizontalAlignment(EHTA_Center);
	RuleText->SetVerticalAlignment(EVRTA_TextTop);
	RuleText->SetWorldSize(33.0f);
	RuleText->SetTextRenderColor(FColor(250, 205, 120));
	RuleText->SetRelativeRotation(FRotator(0.0f, 270.0f, 0.0f));
	RuleText->SetRelativeLocation(FVector(700.0f, 0.0f, 672.0f));
}

void APoiseBreakDemoDirector::BeginPlay()
{
	Super::BeginPlay();
	StartCycle();
}

void APoiseBreakDemoDirector::StartCycle()
{
	CycleTime = 0.0f;
	Naechster = 0;

	for (int32 i = 0; i < 3; ++i)
	{
		LastTier[i] = NAME_None;
		LastOvershoot[i] = 0.0f;
		Breaks[i] = 0;
	}

	UPoiseBreakComponent* All[3] = {Fresh, Worn, Armored};
	for (UPoiseBreakComponent* C : All)
	{
		if (C)
		{
			// The demo drives the clock itself: a component's own tick does not run in an editor
			// viewport, and this director is built to be filmed there.
			C->SetAutoTick(false);
			C->SetMaxPoise(100.0f, /*bRefill=*/true);
			C->ResetPoise();
			C->SetHyperArmor(false);
		}
	}
	if (Armored)
	{
		Armored->SetHyperArmor(true);
	}

	// A fixed script, not a simulation: a demo that plays out differently on every take cannot be
	// described by the sentence printed under it.
	//
	// Arranged around one moment. At seven seconds the SAME twenty-two points of poise damage reach
	// all three, and all three answer differently - the fresh one shrugs, the worn one breaks, and
	// the one with hyper armor barely notices. That is the whole product in one frame.
	Skript.Reset();
	Skript.Add({1.2f, 12.0f, false});    // wearing the middle one down
	Skript.Add({2.4f, 12.0f, false});
	Skript.Add({3.6f, 12.0f, false});
	Skript.Add({4.8f, 12.0f, false});
	Skript.Add({6.0f, 12.0f, false});
	Skript.Add({6.9f, 12.0f, false});
	Skript.Add({BigHitAt, BigHitAmount, true});   // the moment

	// Then a burst on the one that just broke: four hits inside its immunity window, none of which
	// may do anything. Without the window, four attackers hold an actor still for ever.
	Skript.Add({7.4f, 30.0f, true});
	Skript.Add({7.8f, 30.0f, true});
	Skript.Add({8.2f, 30.0f, true});
	Skript.Add({8.6f, 30.0f, true});

	// Steady pressure, so the pools drain and the tiers climb.
	for (int32 i = 0; i < 6; ++i)
	{
		Skript.Add({10.0f + i * 0.9f, 24.0f, true});
	}

	// Then silence from 16 s to 22 s: the pools come back, and the one that is staggered comes back
	// later, because regeneration does not run during a stagger.

	// And one enormous hit at the end, to show a knockdown against a pool that is nearly full again.
	Skript.Add({22.5f, 130.0f, true});

	bBereit = true;
}

void APoiseBreakDemoDirector::Apply(UPoiseBreakComponent* Comp, const float Amount,
	const bool bHyperArmor, const int32 Slot)
{
	if (!Comp)
	{
		return;
	}
	const FPoiseBreakResult R = Comp->AddPoiseDamage(Amount, bHyperArmor);
	if (R.Outcome == EPoiseBreakOutcome::Broke)
	{
		LastTier[Slot] = R.TierName;
		LastOvershoot[Slot] = R.Overshoot;
		Breaks[Slot] += 1;
	}
}

void APoiseBreakDemoDirector::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bAutoRun)
	{
		// The editor hands out one enormous delta after a recompile; unclamped it would play the
		// whole script inside a single frame.
		StepDemo(FMath::Clamp(DeltaSeconds, 0.0f, 0.1f));
	}
}

void APoiseBreakDemoDirector::StepDemo(const float DeltaSeconds)
{
	// BeginPlay does not run in an editor viewport.
	if (!bBereit)
	{
		StartCycle();
	}

	CycleTime += DeltaSeconds;
	if (CycleTime > CycleSeconds)
	{
		StartCycle();
		return;
	}

	UPoiseBreakComponent* All[3] = {Fresh, Worn, Armored};

	for (int32 i = 0; i < 3; ++i)
	{
		if (All[i])
		{
			All[i]->AdvanceTime(DeltaSeconds);
		}
	}

	while (Naechster < Skript.Num() && CycleTime >= Skript[Naechster].Wann)
	{
		const FGeplant G = Skript[Naechster++];
		if (G.bAlle)
		{
			Apply(Fresh, G.Menge, false, 0);
			Apply(Worn, G.Menge, false, 1);
			Apply(Armored, G.Menge, true, 2);
		}
		else
		{
			Apply(Worn, G.Menge, false, 1);
		}
	}

	if (BoardText)
	{
		BoardText->SetText(FText::FromString(HeadlineFor()));
	}
	if (StateText)
	{
		StateText->SetText(FText::FromString(BuildStateText()));
	}
	if (RuleText)
	{
		RuleText->SetText(FText::FromString(RuleFor()));
	}

	if (bDrawDemo)
	{
		DrawBars();
	}
}

FString APoiseBreakDemoDirector::HeadlineFor() const
{
	// Every headline has to be true for the WHOLE phase it covers. A caption the viewer can
	// contradict with the frame in front of them is worse than no caption at all.
	if (CycleTime < BigHitAt)
	{
		return TEXT("three dummies, one hundred poise each - only the middle one is being worn down");
	}
	if (CycleTime < 9.5f)
	{
		return TEXT("the same 22 points reach all three, and all three answer differently");
	}
	if (CycleTime < 16.0f)
	{
		return TEXT("steady pressure: the deeper past zero a hit pushes, the bigger the break");
	}
	if (CycleTime < 22.0f)
	{
		return TEXT("nobody is hitting - the pools come back, but not while an actor is staggered");
	}
	return TEXT("one enormous hit: a knockdown is an overshoot, not a big number");
}

FString APoiseBreakDemoDirector::RuleFor() const
{
	if (CycleTime < BigHitAt)
	{
		return TEXT("THE RULE\n\nthe tier comes from the\nOVERSHOOT - how far past\nzero the pool was pushed\n\nnot from the size\nof the hit");
	}
	if (CycleTime < 9.5f)
	{
		return TEXT("THE MOMENT\n\nsame damage, three answers:\n\nfull pool   -> holds\nworn pool   -> breaks\nhyper armor -> a quarter\n              arrives");
	}
	if (CycleTime < 16.0f)
	{
		return TEXT("NO CHAIN-LOCK\n\nthe immunity window covers\nthe stagger itself\n\nfour attackers cannot hold\nan actor on the floor");
	}
	if (CycleTime < 22.0f)
	{
		return TEXT("RECOVERY\n\nevery hit resets the delay\n\nand nothing regenerates\nwhile an actor is\nstaggered");
	}
	return TEXT("KNOCKDOWN\n\n130 against a pool that is\nnearly full still goes\ndeep past zero\n\nso the tier is the\ntop one");
}

FString APoiseBreakDemoDirector::BuildStateText() const
{
	const UPoiseBreakComponent* All[3] = {Fresh, Worn, Armored};

	FString S = TEXT("THE POOLS\n");
	for (int32 i = 0; i < 3; ++i)
	{
		if (!All[i])
		{
			continue;
		}
		const FPoiseState& St = All[i]->GetState();
		const TCHAR* Zustand =
			All[i]->IsStaggered() ? TEXT("STAGGERED") :
			(All[i]->IsImmune() ? TEXT("immune   ") : TEXT("         "));

		S += FString::Printf(TEXT("%-8s %5.0f/%3.0f %s\n"),
			PoiseBreakDemoLocal::SlotName[i], St.Current, St.Max, Zustand);
	}

	S += TEXT("\nLAST BREAK\n");
	for (int32 i = 0; i < 3; ++i)
	{
		S += FString::Printf(TEXT("%-8s %-10s %s\n"),
			PoiseBreakDemoLocal::SlotName[i],
			LastTier[i].IsNone() ? TEXT("-") : *LastTier[i].ToString(),
			LastTier[i].IsNone() ? TEXT("")
				: *FString::Printf(TEXT("over %3.0f  x%d"), LastOvershoot[i], Breaks[i]));
	}
	return S;
}

void APoiseBreakDemoDirector::DrawBars() const
{
#if ENABLE_DRAW_DEBUG
	const UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	UWorld* Mutable = const_cast<UWorld*>(World);

	// Bleibende Linien plus ein Loeschen zu Beginn jedes Schritts - NICHT Linien mit Standzeit.
	//
	// WARUM: eine Standzeit laeuft ueber den Welt-Takt ab, und in einem Editor-Viewport ohne
	// Echtzeit tickt die Welt gar nicht. Die Formen blieben dann entweder ewig stehen (und jedes
	// Bild waere die Summe aller vorherigen) oder waeren beim Auslesen laengst weg. So haengt das
	// Bild nur noch davon ab, dass StepDemo gerufen wurde.
	FlushPersistentDebugLines(Mutable);


	constexpr float Hoehe = 320.0f;
	constexpr float Fuss = 60.0f;

	const UPoiseBreakComponent* All[3] = {Fresh, Worn, Armored};
	const FVector Basis = GetActorLocation() - FVector(0.0f, 420.0f, 0.0f);

	for (int32 i = 0; i < 3; ++i)
	{
		if (!All[i])
		{
			continue;
		}
		const float X = PoiseBreakDemoLocal::SlotX[i];
		const float Anteil = All[i]->GetPoiseFraction();

		const FColor Farbe =
			All[i]->IsStaggered() ? PoiseBreakDemoLocal::Broken :
			(All[i]->IsImmune() ? PoiseBreakDemoLocal::Immune :
			(Anteil < 0.25f ? PoiseBreakDemoLocal::Low : PoiseBreakDemoLocal::Held));

		// A bar of five plies, so it reads as a bar rather than a wire from any distance.
		for (int32 Ply = 0; Ply < 5; ++Ply)
		{
			const FVector Seite(X + (Ply - 2) * 7.0f, 0.0f, 0.0f);
			const FVector Unten = Basis + Seite + FVector(0.0f, 0.0f, Fuss);
			DrawDebugLine(Mutable, Unten, Unten + FVector(0.0f, 0.0f, Hoehe),
				PoiseBreakDemoLocal::Frame, true, -1.0f, 0, 4.0f);
			if (Anteil > 0.0f)
			{
				DrawDebugLine(Mutable, Unten, Unten + FVector(0.0f, 0.0f, Hoehe * Anteil),
					Farbe, true, -1.0f, 0, 13.0f);
			}
		}

		// The tier thresholds, drawn BELOW the floor of the bar: an overshoot is what happens past
		// zero, so it has to be shown past zero or the picture argues with the rule.
		const FPoiseBreakRules R = All[i]->GetRules();
		const TArray<FPoiseTier> Tiers = UPoiseBreakStatics::NormaliseTiers(R.Tiers);
		for (int32 T = 1; T < Tiers.Num(); ++T)
		{
			const float Z = Fuss - Tiers[T].MinOvershoot * (Hoehe / 100.0f);
			const FVector L = Basis + FVector(X - 40.0f, 0.0f, Z);
			DrawDebugLine(Mutable, L, L + FVector(80.0f, 0.0f, 0.0f),
				FColor(150, 130, 90), true, -1.0f, 0, 3.0f);
		}

		// And the overshoot of the last break as a stub going down from zero.
		if (LastOvershoot[i] > 0.0f)
		{
			const FVector Null = Basis + FVector(X, 0.0f, Fuss);
			DrawDebugLine(Mutable, Null,
				Null - FVector(0.0f, 0.0f, LastOvershoot[i] * (Hoehe / 100.0f)),
				PoiseBreakDemoLocal::Broken, true, -1.0f, 0, 7.0f);
		}
	}
#endif
}
