// Copyright 2026 Silvan Teufel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PoiseBreakTypes.h"
#include "PoiseBreakDemoDirector.generated.h"

class UPoiseBreakComponent;
class UTextRenderComponent;

/**
 * Runs the shipped demo: three dummies, the same hits, three different answers.
 *
 * It owns three real UPoiseBreakComponents and drives them with AdvanceTime and AddPoiseDamage, so
 * every bar and every word on the board is the plugin's own output rather than a retelling of it.
 *
 * It ticks in the editor viewport, because that is where the store images are taken and a Blueprint
 * does not tick there.
 */
UCLASS(meta = (DisplayName = "PoiseBreak Demo Director"))
class POISEBREAK_API APoiseBreakDemoDirector : public AActor
{
	GENERATED_BODY()

public:
	APoiseBreakDemoDirector();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual bool ShouldTickIfViewportsOnly() const override { return true; }

	/** One run of the script. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PoiseBreak Demo",
		meta = (ClampMin = "16.0", ClampMax = "180.0"))
	float CycleSeconds = 26.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PoiseBreak Demo")
	bool bDrawDemo = true;

	/**
	 * Let Tick drive the demo. Off when something else steps it.
	 *
	 * WARUM ES DEN SCHALTER GIBT: ein Actor tickt im Editor-Viewport nur, wenn der Viewport auf
	 * Echtzeit steht - und das ist eine Einstellung des Anwenders, keine des Plugins. Eine
	 * Bildfolge, die davon abhaengt, kommt irgendwann als Standbild heraus.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PoiseBreak Demo")
	bool bAutoRun = true;

	/**
	 * Advance the demo by exactly this many seconds and redraw.
	 *
	 * Public and callable from anywhere: the screenshot run steps it by a fixed amount per frame,
	 * so frame N always shows the same moment of the script.
	 */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "PoiseBreak Demo")
	void StepDemo(float Seconds);

	/** The headline. TextRender, because HighResShot does not capture DrawDebugString. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PoiseBreak Demo")
	TObjectPtr<UTextRenderComponent> BoardText;

	/** The three pools, in words. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PoiseBreak Demo")
	TObjectPtr<UTextRenderComponent> StateText;

	/** The rule the current phase is showing. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PoiseBreak Demo")
	TObjectPtr<UTextRenderComponent> RuleText;

	/** Full pool, chipped, and the one with hyper armor. All three are real components. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PoiseBreak Demo")
	TObjectPtr<UPoiseBreakComponent> Fresh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PoiseBreak Demo")
	TObjectPtr<UPoiseBreakComponent> Worn;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PoiseBreak Demo")
	TObjectPtr<UPoiseBreakComponent> Armored;

private:
	/** One scripted hit: when, how much, and whether it goes to everybody. */
	struct FGeplant
	{
		float Wann = 0.0f;
		float Menge = 0.0f;
		bool bAlle = true;      // all three, or only the one being worn down
	};

	void StartCycle();
	void Apply(UPoiseBreakComponent* Comp, float Amount, bool bHyperArmor, int32 Slot);
	FString BuildStateText() const;
	FString HeadlineFor() const;
	FString RuleFor() const;
	void DrawBars() const;

	TArray<FGeplant> Skript;
	int32 Naechster = 0;
	float CycleTime = 0.0f;
	bool bBereit = false;

	/** The last break each dummy took, for the board. */
	FName LastTier[3];
	float LastOvershoot[3] = {0.0f, 0.0f, 0.0f};
	int32 Breaks[3] = {0, 0, 0};

	/** The synchronised hit that makes the point, so the headline can name it. */
	static constexpr float BigHitAt = 7.0f;
	static constexpr float BigHitAmount = 22.0f;
};
