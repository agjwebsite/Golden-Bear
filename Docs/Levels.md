# Level Design — Fat Man Simulator

Story beats are tracked by `UStoryFlowSubsystem::EStoryBeat`. Each beat below is its own level (`Content/Maps/L_*.umap`) or a sub-section of a larger streamed level.

---

## L_Skopje_Road  *(CarBreakdown → OnTheRoad)*

**Opening cinematic** (Level Sequence): Johnny's old hatchback steams to a halt on the side of the road outside Skopje. He kicks the tire. The tire wins. Cut to gameplay.

**Layout**: a 1–2 km linear stretch of country road and farmland between Skopje and Veles. Cliffs/forest on one side, fields on the other, so the player stays on the path.

**Population**:
- 3–4 patches of stray dogs (`AEnemyBase`, melee, low HP — knocks Johnny's stamina around)
- 2 roadside food vendors (`AFoodPickup` cluster — burek, kebapi)
- 1 mid-route gas station with a vending machine (candy pickups)
- Periodic voice-line zones (trigger boxes that call `Voice->Say(EJohnnyLine::FoodCraving)`)

**Exit**: a trigger box at the city limits that sets the story beat to `GangsterChase` and seamlessly opens `L_Veles_Outskirts`.

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
