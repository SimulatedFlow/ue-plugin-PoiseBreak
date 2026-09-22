// Copyright 2026 Silvan Teufel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PoiseBreakTypes.h"
#include "PoiseBreakComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FPoiseBrokenSignature,
	FName, TierName, int32, TierIndex, float, Overshoot);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPoiseStaggerEndedSignature);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPoiseChangedSignature,
	float, Current, float, Max);

/**
 * UPoiseBreakComponent
 *
 * Put it on anything that can be staggered. Call AddPoiseDamage from wherever your project already
 * applies damage; bind OnPoiseBroken and play the reaction.
 *
 * The component owns a clock and a delegate list. Every decision it makes comes from
 * UPoiseBreakStatics, which is what the tests call too.
 */
UCLASS(ClassGroup = (PoiseBreak), meta = (BlueprintSpawnableComponent))
class POISEBREAK_API UPoiseBreakComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPoiseBreakComponent();

	/**
	 * One hit against the pool. Returns the full result so the caller can react in the same frame.
	 *
	 * bHyperArmor is the ATTACKER'S question answered by the DEFENDER'S state in most projects: it
	 * is true while this actor is inside one of its own hyper-armor windows. Drive it from an anim
	 * notify with SetHyperArmor, or pass it explicitly here for a one-off.
	 */
	UFUNCTION(BlueprintCallable, Category = "PoiseBreak")
	FPoiseBreakResult AddPoiseDamage(float Amount, bool bHyperArmor = false);

	/** Same, but uses the hyper-armor flag the component is currently holding. */
	UFUNCTION(BlueprintCallable, Category = "PoiseBreak")
	FPoiseBreakResult AddPoiseDamageAuto(float Amount);

	/** Open or close a hyper-armor window. An anim notify state is the natural driver. */
	UFUNCTION(BlueprintCallable, Category = "PoiseBreak")
	void SetHyperArmor(bool bEnabled);

	UFUNCTION(BlueprintPure, Category = "PoiseBreak")
	bool HasHyperArmor() const { return bHyperArmorActive; }

	/** Fill the pool, clear the stagger and the immunity. For respawns and phase changes. */
	UFUNCTION(BlueprintCallable, Category = "PoiseBreak")
	void ResetPoise();

	/**
	 * Change the pool size at runtime, keeping the current value sensible.
	 *
	 * For a boss that gets sturdier between phases, and for anything that has to be set up before
	 * BeginPlay has run - the demo director in this plugin is one such caller.
	 */
	UFUNCTION(BlueprintCallable, Category = "PoiseBreak")
	void SetMaxPoise(float NewMax, bool bRefill = true);

	/** Turn the component's own clock on or off. See AdvanceTime. */
	UFUNCTION(BlueprintCallable, Category = "PoiseBreak")
	void SetAutoTick(bool bEnabled) { bAutoTick = bEnabled; }

	/** Force a break of the given tier without going through the pool - for scripted moments. */
	UFUNCTION(BlueprintCallable, Category = "PoiseBreak")
	void ForceBreak(int32 TierIndex);

	UFUNCTION(BlueprintPure, Category = "PoiseBreak")
	const FPoiseState& GetState() const { return State; }

	UFUNCTION(BlueprintPure, Category = "PoiseBreak")
	bool IsStaggered() const;

	UFUNCTION(BlueprintPure, Category = "PoiseBreak")
	bool IsImmune() const;

	UFUNCTION(BlueprintPure, Category = "PoiseBreak")
	float GetPoiseFraction() const;

	/** The rules this component is using - the project defaults unless bOverrideRules is set. */
	UFUNCTION(BlueprintPure, Category = "PoiseBreak")
	FPoiseBreakRules GetRules() const;

	/**
	 * Step the clock by hand.
	 *
	 * Public, and bAutoTick can be switched off, for a server on a fixed step, a replay being
	 * scrubbed, or a demo running without a game.
	 */
	UFUNCTION(BlueprintCallable, Category = "PoiseBreak")
	void AdvanceTime(float DeltaSeconds);

	UPROPERTY(BlueprintAssignable, Category = "PoiseBreak")
	FPoiseBrokenSignature OnPoiseBroken;

	UPROPERTY(BlueprintAssignable, Category = "PoiseBreak")
	FPoiseStaggerEndedSignature OnStaggerEnded;

	UPROPERTY(BlueprintAssignable, Category = "PoiseBreak")
	FPoiseChangedSignature OnPoiseChanged;

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	/** Poise pool of this actor. Zero or less takes the project default. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PoiseBreak", meta = (ClampMin = "0.0"))
	float MaxPoise = 0.0f;

	/** Use the rules below instead of the project settings. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PoiseBreak")
	bool bOverrideRules = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PoiseBreak", meta = (EditCondition = "bOverrideRules"))
	FPoiseBreakRules RuleOverride;

	/** Let the component run its own clock. Off when something else drives AdvanceTime. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PoiseBreak")
	bool bAutoTick = true;

private:
	UPROPERTY()
	FPoiseState State;

	bool bHyperArmorActive = false;

	/** So OnStaggerEnded fires once, on the edge. */
	bool bWasStaggered = false;

	void BroadcastBreak(const FPoiseBreakResult& Result);
};
