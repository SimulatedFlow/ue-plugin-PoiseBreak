# PoiseBreak — Poise, Stagger Tiers & Hyper Armor

**Whether a hit staggers should not depend on how big the hit was.**

It should depend on how close the target already was to breaking. That one sentence is the whole
plugin, and it is the difference between a fight that rewards pressure and a fight that feels like a
coin toss.

---

## 0. Supported engine and platforms

* Unreal Engine **5.8**
* **Win64.** The plugin's `PlatformAllowList` is Win64 only; Mac and Linux are not supported.
* One runtime C++ module, no third-party code, full source included.
* No dependency on GameplayAbilities or AIModule. Poise is a number your damage pipeline already
  knows how to produce, and what a stagger looks like is your project's business.

---

## 1. The five-minute install

1. Add a **Poise Break** component to anything that can be staggered.
2. Where your project applies damage, add one line:

```
AddPoiseDamage(PoiseDamage, bHyperArmor)
```

3. Bind `OnPoiseBroken(TierName, TierIndex, Overshoot)` and play the reaction.

That is all of it. The component runs its own clock, fills the pool back up and fires
`OnStaggerEnded` when the actor may act again.

---

## 2. The rule that matters: the tier comes from the overshoot

When a hit pushes the pool past zero, the plugin measures **how far past zero** it went, and picks
the tier from that.

| Situation | Poise damage | Pool before | Overshoot | Result |
|---|---|---|---|---|
| Fresh enemy | 22 | 100 | — | nothing, 78 left |
| Worn enemy | 22 | 5 | 17 | **Stagger** |
| Nearly broken | 60 | 5 | 55 | **Knockdown** |
| Fresh, huge hit | 60 | 100 | — | nothing, 40 left |

Look at the first and the last row. **The same hit, two different answers** — and that is what makes
trading blows a decision instead of a lottery. Key the tier off the hit's own size instead, as most
hand-rolled systems do, and a heavy weapon staggers everything equally whether the target was fresh
or one point from breaking. Pressure stops meaning anything.

A hit that lands exactly on zero is a break with no overshoot: the smallest tier. "It broke, but
only just" is a tier, not a non-event.

---

## 3. Tiers

A tier is a name, an entry threshold on the overshoot, a stagger length and an extra immunity.

```
Flinch     from overshoot   0   stagger 0.35s   +0.20s immune
Stagger    from overshoot  15   stagger 0.90s   +0.60s immune
Knockdown  from overshoot  40   stagger 2.00s   +1.20s immune
```

`NormaliseTiers` sorts them ascending and clamps negatives away, so a generated list does not have to
arrive in order. Duplicates are left alone — two tiers at the same threshold both qualify and the
later one wins, which is a reasonable thing to write.

The first tier is the floor. An overshoot below every threshold still produces tier 0.

---

## 4. The immunity window covers the stagger

This is the part that is usually missing, and its absence is the reason "getting stun-locked" is a
complaint in so many games.

When an actor breaks, it becomes immune for **the stagger plus the tier's extra**. During that
window, poise damage is **ignored entirely** — the pool does not even take it.

Why ignored rather than accumulated: a pool that drained while the actor was on the floor would
break again the instant the window closed. Four attackers each landing the smallest possible break
would hold an actor still forever, which is exactly what the window exists to prevent.

Health damage is untouched by this. A staggered actor still bleeds; it just cannot be staggered
again yet.

---

## 5. Hyper armor scales what arrives at the pool

`AddPoiseDamage(Amount, bHyperArmor = true)` multiplies the incoming poise damage by
`HyperArmorScale` — a quarter by default.

**It says nothing about health damage.** Folding the two together is the single most common way this
feature is got wrong: a boss with hyper armor should still take damage while it swings, it just
should not flinch. Drive the flag from an anim notify state with `SetHyperArmor`, then call
`AddPoiseDamageAuto`.

A scale above one is allowed. "This recovery window makes you *more* fragile" is a legitimate thing
for a designer to want, and the plugin has not earned an opinion about it.

---

## 6. Regeneration

`RegenDelaySeconds` after the last **effective** hit, then `RegenPerSecond` until full.

Three rules that all exist because of a specific failure:

* **Every effective hit resets the delay**, including one that breaks nothing. Reset only on breaks
  and a stream of chip damage lets the pool refill underneath it, so the enemy never staggers at all.
* **A hit that arrives as nothing does not reset the delay.** Otherwise a hyper-armor scale of zero
  becomes a way of holding an enemy's pool down forever.
* **Nothing regenerates while an actor is staggered** (unless you switch `bRegenWhileStaggered` on).
  A staggered actor recovering poise means the next hit behaves as though the stagger never happened.

Regeneration is linear, so two half-steps and one whole step give the same answer. A frame rate that
changes does not change the fight.

---

## 7. Refill on break, or not

`bRefillOnBreak` is on by default: the break **is** the punishment, and the actor gets a clean pool
afterwards. That is the Souls-like reading.

Off is the posture reading: the pool stays empty and has to regenerate, so a second break follows
quickly if the pressure keeps up. Both are one checkbox, because both are real designs.

---

## 8. What PoiseBreak is not

* **It does not animate anything.** It tells you an actor broke and how hard. What plays is yours.
* **It does not read or write health.** Poise damage and health damage are two numbers, and the
  plugin only knows about one of them.
* **It is not replicated.** Replicate the break event — a tier index and an overshoot — not the
  component. Poise itself is a server-side number.
* **It is not an ability system.** It pairs with one; it does not replace one.

---

## 9. Console commands

| Command | What it does |
|---|---|
| `PoiseBreak.Dump` | Every actor in the level with a poise component: pool, stagger, immunity, break count and last tier. |
| `PoiseBreak.Tiers` | Print the default tier list. |

---

## 10. API reference

### `UPoiseBreakComponent`

`AddPoiseDamage(Amount, bHyperArmor)`, `AddPoiseDamageAuto(Amount)`, `SetHyperArmor(bool)`,
`ResetPoise()`, `SetMaxPoise(NewMax, bRefill)`, `ForceBreak(TierIndex)`, `AdvanceTime(float)`,
`SetAutoTick(bool)`, `GetState()`, `IsStaggered()`, `IsImmune()`, `GetPoiseFraction()`, `GetRules()`.

Delegates: `OnPoiseBroken(TierName, TierIndex, Overshoot)`, `OnStaggerEnded()`,
`OnPoiseChanged(Current, Max)`.

`ForceBreak` ignores the immunity window on purpose: it is a moment the designer wrote, not a hit
that happened to land.

`AdvanceTime` is public and `bAutoTick` can be switched off, for a server on a fixed step, a replay
being scrubbed, or a demo running in an editor viewport.

### `UPoiseBreakStatics` — the rules, on their own

`EffectivePoiseDamage`, `NormaliseTiers`, `TierIndexFor`, `ApplyPoiseDamage`, `Advance`,
`IsStaggered`, `IsImmune`, `PoiseFraction`, `DefaultTiers`.

No world, no actor, no clock. The component calls exactly these and so do the tests, which is the
only way the meter on screen and the plugin cannot disagree.

---

## 11. The demo level

`Content/PoiseBreak/Maps/L_PoiseBreakDemo` — three dummies with a hundred poise each, and one moment
worth the whole plugin: at seven seconds the **same** twenty-two points of poise damage reach all
three and all three answer differently. The fresh one shrugs it off, the one that has been worn down
breaks, and the one inside a hyper-armor window takes a quarter.

After that: four hits inside the immunity window that do nothing, steady pressure that climbs the
tiers, six seconds of quiet in which the pools come back — and one enormous hit at the end that is a
knockdown because of how deep it went, not because it was large.

`ASODDemoDirector`-style note: the director drives three **real** components with `AddPoiseDamage`
and `AdvanceTime`, so every bar and every word on the board is the plugin's own output.

**If the level looks frozen**, the viewport is not set to realtime. Either switch realtime on, or
call `StepDemo(Seconds)` on the director yourself — that is what the screenshot run does.

---

## 12. Troubleshooting

**Nothing ever staggers.** The poise damage is too small for the pool, or something is holding
`bHyperArmor` on. `PoiseBreak.Dump` shows the pool; a pool that never drops is the first clue.

**Everything staggers constantly.** The pool is too small for the damage, or `RegenDelaySeconds` is
so long that it never comes back. Remember the delay resets on *every* effective hit.

**The tier is always the same.** Your thresholds are too far apart for the overshoots your damage
produces, or the tier list has one entry. `PoiseBreak.Tiers` prints the defaults for comparison.

**An actor is stun-locked anyway.** Something is calling `ForceBreak` in a loop — that call is
deliberately exempt from the immunity window.

**Poise regenerates during the stagger.** `bRegenWhileStaggered` is on.

**The pool is full immediately after every break.** That is `bRefillOnBreak`, and it is the default.
Switch it off for a posture-style bar.
