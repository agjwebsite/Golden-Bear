# Level Design — Fat Man Simulator

Story beats are tracked by `UStoryFlowSubsystem::EStoryBeat`. Each beat below is its own level (`Content/Maps/L_*.umap`) or a sub-section of a larger streamed level.

---

## L_Skopje_Drive  *(Driving → CarBreakdown → OnTheRoad)*

**Opening playable scene (~30–45 seconds of driving)**

Johnny starts behind the wheel of his old hatchback (`AJohnnyCar`) on the road out of Skopje. Player drives forward with throttle/steer. After ~22 seconds (`TimeUntilSputter`), the engine starts misfiring — speed cuts in half on random coughs, exhaust smoke kicks on, Johnny pleads with the car. After another ~10 seconds (`SputterDuration`) the engine quits entirely. Short pause for a beat of comedy, then the player is automatically possessed into `BP_Johnny` standing next to the driver's door.

**Setup checklist**:
1. Create `BP_JohnnyCar` (parent: `JohnnyCar`). Assign a car static mesh and engine sound to its components.
2. Create `IMC_Driving` and Input Actions `IA_Throttle` (Axis1D — W=+1, S=-1), `IA_Steer` (Axis1D — D=+1, A=-1), `IA_Look` (Mouse XY), `IA_Horn` (H key).
3. Place one `BP_JohnnyCar` and one `BP_Johnny` in the level. On the car, set `DriverOnExit` to point at the Johnny actor.
4. In **World Settings**, set the **Default Pawn Class** to `BP_JohnnyCar` (override for this map only — the rest of the levels use `BP_Johnny`).
5. Place a hidden `BP_Johnny` somewhere reasonable; the car will teleport him to its driver-side door on breakdown.

**Layout**: ~600m of straight-ish road out of Skopje with one or two gentle curves. Surround with low fences / forest so the player can't drive off into the void. The breakdown timing is wall-clock based so the player ends up roughly where you place them; tune `TimeUntilSputter` to match your level length, or call `ForceBreakdown()` from a trigger box if you want it to fire at a specific spot instead.

**After the breakdown** — gameplay continues seamlessly in the same level:

**Layout (on-foot section)**: a 1–2 km linear stretch of country road and farmland between Skopje and Veles. Cliffs/forest on one side, fields on the other, so the player stays on the path.

**Population**:
- 3–4 patches of stray dogs (`AEnemyBase`, melee, low HP — knocks Johnny's stamina around)
- 2 roadside food vendors (`AFoodPickup` cluster — burek, kebapi)
- 1 mid-route gas station with a vending machine (candy pickups)
- Periodic voice-line zones (trigger boxes that call `Voice->Say(EJohnnyLine::FoodCraving)`)

**Exit**: a trigger box at the city limits that sets the story beat to `GangsterChase` and seamlessly opens `L_Veles_Outskirts`.

> Note: the on-foot section above lives in the **same** level as the driving section (`L_Skopje_Drive`). Keeping them in one map avoids the awkward load screen after the car dies.

---

## L_Veles_Outskirts  *(GangsterChase)*

**Trigger cutscene**: A black SUV screeches up. Three `AGangsterEnemy` instances pile out, yelling about the money Johnny owes.

**Layout**: narrow alleys, fences to scramble over, market stalls to knock down. Multiple branching escape paths. Stamina management is critical here — sprint too long and they catch up.

**Win condition**: outrun them (distance > `GiveUpDistance` for the lead gangster with `bCanGiveUp = true`), **or** fight all three to zero HP.

**Exit**: trigger box at the meeting building.

---

## L_Veles_Office  *(ArrivedTooLate → BossFires)*

**Cinematic-heavy beat** (Level Sequence + dialogue):
1. Johnny arrives, sweaty, panting.
2. Receptionist: "The meeting ended an hour ago."
3. Boss walks out. Fires him on the spot.
4. `Voice->Say(EJohnnyLine::BossFires)` — Johnny's one line of dignity.

Camera handoff back to gameplay on the street outside.

---

## L_Veles_Street  *(FoundADollar → GamblingHall)*

A short walkable street. A glinting dollar bill is highlighted on the sidewalk (`AFoodPickup` reskinned, or a dedicated `ADollarPickup` — designer's call). Picking it up:
- Adds 1 to `StoryFlowSubsystem.Wallet`
- Sets beat to `GamblingHall`
- A nearby `AGamblingMachine` glows.

---

## L_Veles_Casino  *(GamblingHall → Jackpot → DebtPaid → Luxury → BrokeAgain)*

**Single small interior**: dingy back-room casino, one slot machine.

Johnny walks up, presses Interact. `AGamblingMachine::Pull(Johnny)`:
- First pull pays out `RiggedFirstPullPayout = $1,000,000`
- `StoryFlowSubsystem::AdvanceToEnding` fires the chained timers:
  - +2.5s → `PayOffDebt()` (gangsters appear, Johnny throws cash at them, they leave)
  - +5.0s → `Luxury` beat (level sequence montage: mansion, sports car, banquet table groaning under kebapi)
  - +15.0s → `BrokeAgain` (the wallet drains to $0 buying food; Johnny is back where he started, in a stained shirt, scratching his belly)

**Final shot**: Johnny on a curb, holding an empty kebapi wrapper. Voice line: *"Where did it all go... oh. The kebapi. Right."* Cut to credits.

---

## Streaming / level loading

For a polished build, use **World Partition** or **Level Streaming Volumes** to seamlessly transition L_Skopje_Road → L_Veles_Outskirts → L_Veles_Office. For a first playable, hard cuts via `UGameplayStatics::OpenLevel` are fine — just have the `StoryFlowSubsystem` listen for `OnBeatChanged` and call `OpenLevel` accordingly from a `GameInstance` subclass or a level Blueprint.
