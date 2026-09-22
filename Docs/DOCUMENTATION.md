# HeatSink — Weapon Heat, Overheat Lockout & Venting

**The lockout is a wait, not a heat check.**

Bring the weapon back the instant heat drops one point below the maximum and the player learns to tap
the trigger on the boundary — which is the exact habit an overheat mechanic exists to discourage.
HeatSink locks the weapon for a fixed time instead, and tapping buys nothing.

---

## 0. Supported engine and platforms

* Unreal Engine **5.8**
* **Win64.** The plugin's `PlatformAllowList` is Win64 only; Mac and Linux are not supported.
* One runtime C++ module, no third-party code, full source included.
* No dependency on GameplayAbilities or any weapon framework. It does not spawn anything, does not
  know what a bullet is, and has no opinion about ammunition.

---

## 1. The five-minute install

1. Add a **Heat Sink** component to the weapon, or to whatever owns it.
2. Where your project pulls the trigger:

```
Result = TryFire()
if Result.bFired:  spawn the projectile
```

3. Bind `OnHeatChanged(Fraction)` to the gauge, `OnOverheated(LockoutSeconds)` to the steam, and
   `OnReady()` to the click that says you can shoot again.

That is all of it. The component runs its own clock.

---

## 2. Cooling starts after a delay, and every shot resets it

```
HeatPerShot         12.0
CoolPerSecond       25.0
CoolDelaySeconds     0.6    <- the one that matters
```

Without the delay, heat drains between the rounds of a burst and the weapon never overheats at all —
which, from the outside, looks like the whole system does nothing. The delay is what makes sustained
fire different from the same number of rounds spread out.

Every shot pushes the clock back to zero. Cooling is linear once it starts, and a step that straddles
the delay boundary only cools for the part of itself that lies past it. A frame rate that changes does
not change how fast the weapon cools.

---

## 3. The overheat lockout

```
OverheatLockoutSeconds   2.5
HeatAfterLockout         0.0
bLastShotFires           true
```

The lockout is **a duration**, not a condition. It does not end early because the weapon cooled
quickly, and it cannot be shortened by releasing the trigger. `GetSecondsUntilReady()` is a countdown
a HUD can draw.

`bLastShotFires` is on by default: the shot that crosses the maximum still fires, and the overheat
follows it. Refusing a trigger pull at ninety-nine percent heat reads as a broken gun, and the player
cannot see the number the way the code can. Switch it off for the stricter reading, where the weapon
simply will not take a shot it cannot afford.

Pulling the trigger during a lockout returns `BlockedOverheated`. Nothing happens, nothing is queued,
and no heat is added.

---

## 4. Venting is a cost you choose

```
VentSeconds     1.2
HeatAfterVent   0.0
```

`BeginVent()` clears the heat completely in less time than a lockout takes — and during it the weapon
cannot fire. That is the entire mechanic: spend a second on purpose, or lose two and a half by
accident. A vent is refused while the weapon is locked out, jammed or already venting, because none of
those are moments the player gets to choose.

The shipped demo is built around exactly this comparison. Over one cycle the disciplined shooter
fires substantially more rounds than the one holding the trigger down, with no blocked pulls at all.

---

## 5. Jams, if you want them

```
JamFromFraction   1.0    fraction of MaxHeat above which jams are possible (1 = off)
JamChanceAtMax    0.0    chance at full heat, scaled down linearly to nothing at JamFromFraction
ClearJamSeconds   1.8
```

Off by default. A flat chance would make a cold weapon jam as often as a glowing one, and the player
would read the whole mechanic as random punishment rather than as a consequence of how they were
shooting — so the chance is zero below the fraction and rises linearly above it.

The roll is measured against the heat the shot **leaves**, not the heat it started from. A shot taken
at ninety percent that lands on a hundred is a hot shot, and treating it as a ninety percent shot
understates exactly the case the mechanic is about.

A jam does not clear itself. Call `BeginClearJam()` — usually from a reload input — and the weapon is
back after `ClearJamSeconds`. `OnJammed` fires when it happens.

Set `JamFromFraction` low and `JamChanceAtMax` high enough that a player who holds the trigger
actually meets a jam. Rules that reach a state once an hour teach nobody anything.

---

## 6. The outcomes

| `EHeatShotOutcome` | Meaning |
|---|---|
| `Fired` | It fired and the weapon is still usable. |
| `FiredAndOverheated` | It fired, and that shot pushed it over the line. |
| `FiredAndJammed` | It fired and jammed. |
| `BlockedOverheated` | The lockout is running. |
| `BlockedJammed` | It is jammed and has to be cleared. |
| `BlockedVenting` | A vent is in progress. |

`FHeatShotResult::JamChance` carries the chance the shot was measured against, so a debug readout can
show it without recomputing it.

---

## 7. The random value can be handed in

`TryFire()` draws the jam roll from the engine's stream. `TryFireSeeded(RandomValue)` takes it.

Use the seeded form wherever a server, a client prediction and a replay have to reach the same answer
from the same inputs.

---

## 8. What HeatSink is not

* **It is not a weapon.** It does not spawn projectiles, play sounds, or know your fire rate.
* **It is not ammunition.** Heat and magazines are two systems, and a weapon can have both.
* **It does not animate anything.** It tells you the weapon overheated and for how long.
* **It is not replicated.** Fire on the server and replicate the result. Heat on a client is heat the
  client can edit.

---

## 9. Console commands

| Command | What it does |
|---|---|
| `HeatSink.Dump` | Every weapon in the level with a heat component: heat, state, shots fired, overheats, jams and seconds until ready. |

---

## 10. API reference

### `UHeatSinkComponent`

`TryFire()`, `TryFireSeeded(RandomValue)`, `BeginVent()`, `BeginClearJam()`, `ResetHeat()`,
`AddHeat(Amount)`, `CanFire()`, `GetHeatFraction()`, `GetJamChance()`, `GetSecondsUntilReady()`,
`GetState()`, `GetRules()`, `AdvanceTime(float)`, `SetAutoTick(bool)`.

Delegates: `OnHeatChanged(Fraction)`, `OnOverheated(LockoutSeconds)`, `OnReady()`, `OnJammed()`.

`OnReady` fires **once, on the edge**. A delegate that fires every frame the weapon is usable is a
delegate nobody can bind a sound to.

`AddHeat` exists for heat that is not a shot — standing in lava, an enemy's beam, a hostile
environment.

`AdvanceTime` is public and `bAutoTick` can be switched off, for a server on a fixed step, a replay
being scrubbed, or a demo running in an editor viewport.

### `UHeatSinkStatics` — the rules, on their own

`NormaliseRules`, `CanFire`, `HeatFraction`, `JamChance`, `Fire`, `Advance`, `BeginVent`,
`BeginClearJam`, `SecondsUntilReady`.

No world, no actor, no clock, no random stream. The component calls exactly these and so do the tests,
which is the only way the gauge on screen and the weapon's behaviour cannot drift apart.

### Project Settings > Plugins > HeatSink

`Rules` is the project default; a component can override it with `bOverrideRules` and `RuleOverride`.
`bLogEvents` writes a line for every overheat and every jam.

---

## 11. The demo level

`Content/HeatSink/Maps/L_HeatSinkDemo` — two weapons under the **same** heat curve, fed the same
sequence of random numbers, fired differently. The left one holds the trigger down. The right one
fires bursts of five and vents in the gaps.

The tall bar in each column is the heat; the orange line at the top is the maximum and the shorter
line below it is where jams begin. The strip of ticks along the floor is every trigger pull — a long
tick fired, a short one was refused.

The left column runs to the line, turns red, fills its strip with refusals, and jams. The right column
never reaches the line: it stops on its own, turns green while it vents, and carries on. By the end of
the cycle it has fired considerably more rounds with nothing blocked at all.

The demo runs on its own fixed sub-step rather than the caller's, so the same cycle plays out
identically whether it is stepped at eight frames a second or at a hundred and twenty.

**If the level looks frozen**, the viewport is not set to realtime. Either switch realtime on, or
call `StepDemo(Seconds)` on the director yourself — that is what the screenshot run does.

---

## 12. Troubleshooting

**The weapon never overheats.** `CoolDelaySeconds` is zero or very small, so heat drains between the
rounds of a burst. That one value is the difference between a working heat system and a decorative
one.

**The weapon overheats after two shots.** `HeatPerShot` is too large for `MaxHeat`. Divide the maximum
by the burst length you want.

**Players tap the trigger on the boundary and never lose anything.** Something is ending the lockout
early. `OverheatLockoutSeconds` is a duration; nothing in the plugin shortens it.

**The trigger does nothing and there is no lockout.** It is jammed or venting. `HeatSink.Dump` prints
the state, and `FHeatShotResult::Outcome` says which.

**Jams never happen.** `JamFromFraction` is 1.0, which switches them off, or `JamChanceAtMax` is zero.

**Jams happen constantly.** `JamFromFraction` is low enough that ordinary firing sits above it. It is
a fraction of `MaxHeat`, not an absolute heat value.

**`OnReady` never fires.** It fires on the edge only. If the weapon was never blocked, there is no
edge to fire on.
