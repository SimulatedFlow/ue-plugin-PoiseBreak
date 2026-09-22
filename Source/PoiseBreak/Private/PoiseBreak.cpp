// Copyright 2026 Silvan Teufel. All Rights Reserved.

#include "PoiseBreak.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "HAL/IConsoleManager.h"
#include "PoiseBreakComponent.h"
#include "PoiseBreakLog.h"
#include "PoiseBreakStatics.h"

DEFINE_LOG_CATEGORY(LogPoiseBreak);

#define LOCTEXT_NAMESPACE "FPoiseBreakModule"

namespace
{
	UWorld* PoiseBreakConsoleWorld()
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

	void PoiseBreakDumpCommand()
	{
		UWorld* World = PoiseBreakConsoleWorld();
		if (!World)
		{
			UE_LOG(LogPoiseBreak, Warning, TEXT("PoiseBreak.Dump: no running world."));
			return;
		}

		int32 Found = 0;
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			const UPoiseBreakComponent* Poise = It->FindComponentByClass<UPoiseBreakComponent>();
			if (!Poise)
			{
				continue;
			}
			++Found;
			const FPoiseState& S = Poise->GetState();
			UE_LOG(LogPoiseBreak, Display,
				TEXT("%-24s poise %6.1f / %6.1f  %s%s  breaks %d  last %s"),
				*It->GetName(), S.Current, S.Max,
				Poise->IsStaggered() ? TEXT("STAGGERED ") : TEXT(""),
				Poise->IsImmune() ? TEXT("immune") : TEXT(""),
				S.BreakCount,
				S.LastTier.IsNone() ? TEXT("-") : *S.LastTier.ToString());
		}

		if (Found == 0)
		{
			UE_LOG(LogPoiseBreak, Display, TEXT("PoiseBreak.Dump: nothing in this level has a poise component."));
		}
	}

	void PoiseBreakTiersCommand()
	{
		for (const FPoiseTier& T : UPoiseBreakStatics::DefaultTiers())
		{
			UE_LOG(LogPoiseBreak, Display,
				TEXT("%-12s from overshoot %5.1f  stagger %.2fs  immune for a further %.2fs"),
				*T.Name.ToString(), T.MinOvershoot, T.StaggerSeconds, T.ExtraImmunitySeconds);
		}
	}

	FAutoConsoleCommand GPoiseBreakDump(
		TEXT("PoiseBreak.Dump"),
		TEXT("Every actor in the level with a poise component: pool, stagger, immunity and break count."),
		FConsoleCommandDelegate::CreateStatic(&PoiseBreakDumpCommand));

	FAutoConsoleCommand GPoiseBreakTiers(
		TEXT("PoiseBreak.Tiers"),
		TEXT("Print the default tier list."),
		FConsoleCommandDelegate::CreateStatic(&PoiseBreakTiersCommand));
}

void FPoiseBreakModule::StartupModule()
{
}

void FPoiseBreakModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FPoiseBreakModule, PoiseBreak)
