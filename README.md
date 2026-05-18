# Fat Man Simulator

Johnny's car broke down on the way to a meeting in Veles. He has to walk. He hates every step of it.

A third-person, dark-comedy action game built in Unreal Engine 5.7.

---

## What's in this repo

This repository contains the **project scaffold and gameplay code** for Fat Man Simulator. It does **not** include binary assets (meshes, textures, materials, animations, levels, Blueprint assets) — those are authored in the Unreal Editor.

```
FatManSimulator.uproject       Unreal project descriptor
Config/                        Project config (Engine, Game, Input)
Source/
  FatManSimulator.Target.cs       Game build target
  FatManSimulatorEditor.Target.cs Editor build target
  FatManSimulator/
    FatManSimulator.Build.cs        Module deps
    FatManSimulator.{h,cpp}         Module entry point
    JohnnyCar.{h,cpp}               The opening drivable car (arcade movement, scripted breakdown, possess-handoff)
    JohnnyCharacter.{h,cpp}         The fat man himself (meters, sprint, full combat suite)
    JohnnyVoiceComponent.{h,cpp}    Self-hating commentary + food daydreams
    FoodPickup.{h,cpp}              Kebapi, burgers, gyros, burek, baklava, candy
    ThrownFood.{h,cpp}              Projectile food that slips greasy enemies or chips sweet damage
    EnemyBase.{h,cpp}               Patrolling enemy with chase, melee, knockdown & finisher hooks
    GangsterEnemy.{h,cpp}           The loan-shark goons who taunt while chasing
    GamblingMachine.{h,cpp}         The rigged slot machine
    StoryFlowSubsystem.{h,cpp}      Tracks story beats + wallet + debt
    FatManSimulatorGameMode.{h,cpp}
    FatManSimulatorPlayerController.{h,cpp}
Content/                       Placeholder folders for assets (created in editor)
Docs/Levels.md                 Level-by-level design notes
```

---

## Quick start

1. **Install Unreal Engine 5.7** (or the closest version you have — see "Engine version" below).
2. Right-click `FatManSimulator.uproject` → **Generate Visual Studio project files** (Windows) or use `GenerateProjectFiles` on Mac/Linux.
3. Open `FatManSimulator.sln` (or the equivalent) and build the **Development Editor** configuration.
4. Open `FatManSimulator.uproject` — Unreal Editor will launch.
5. Follow **First-time editor setup** below to wire up Blueprints, Input, and the starter level.

### Engine version

The `.uproject` points at `5.7`. If your installed engine is a different version, either:
- Right-click the `.uproject` in Explorer → **Switch Unreal Engine version**, **or**
- Edit `FatManSimulator.uproject` and change `"EngineAssociation"` to your version string.

---

## First-time editor setup

The C++ defines all systems, but a few editor-side assets need to be created once. The character will boot up with hardcoded fallback voice lines even if you skip step 3, but it won't move without input bindings.

### 1. Create the Johnny Blueprint
- In `Content/Blueprints/`, right-click → **Blueprint Class** → parent `JohnnyCharacter` → name it `BP_Johnny`.
- Open it. In the **Mesh** component, assign a skeletal mesh (the UE5 Mannequin works as a placeholder — you can scale it up on X/Y for the fat-man silhouette).
- Set **Mesh** location to `(0, 0, -96)` and rotation to `(0, 0, -90)` so it stands inside the capsule.

### 2. Set the default Pawn in the Game Mode
- `Config/DefaultEngine.ini` already points `GlobalDefaultGameMode` at `FatManSimulatorGameMode`.
- Create `Content/Blueprints/BP_FatManSimulatorGameMode` (parent: `FatManSimulatorGameMode`), set **Default Pawn Class** = `BP_Johnny`.
- In **Project Settings → Maps & Modes**, set **Default GameMode** = `BP_FatManSimulatorGameMode`.

### 3. Create Enhanced Input assets
In `Content/Input/`:
- **Input Mapping Context** → `IMC_Default`
- **Input Actions** (Digital bool unless noted):
  `IA_Move` (Axis2D), `IA_Look` (Axis2D), `IA_Jump`, `IA_Sprint`,
  `IA_Attack`, `IA_Parry`, `IA_BellySlam`, `IA_Haymaker`, `IA_ThrowFood`, `IA_Interact`,
  `IA_HotbarSlot1`…`IA_HotbarSlot5`.

Wire bindings in `IMC_Default`:

| Action          | Key                                   | What it does                                                  |
| --------------- | ------------------------------------- | ------------------------------------------------------------- |
| IA_Move         | WASD (with Swizzle/Negate as usual)   | Movement                                                      |
| IA_Look         | Mouse XY                              | Camera                                                        |
| IA_Jump         | Space Bar                             | Jump                                                          |
| IA_Sprint       | Left Shift                            | Sprint (drains stamina)                                       |
| IA_Attack       | Left Mouse Button                     | Light jab; chains into the 3-hit combo                        |
| IA_Parry        | Right Mouse Button                    | Tight-window parry; success → free Riposte (Haymaker)         |
| IA_BellySlam    | Q                                     | Heavy AOE knockback + knockdown                               |
| IA_Haymaker     | F                                     | Heavy single-target staggering blow                           |
| IA_ThrowFood    | G                                     | Lob the active hotbar food                                    |
| IA_Interact     | E                                     | Interact / Finisher when prompt visible                       |
| IA_HotbarSlot1…5| 1 / 2 / 3 / 4 / 5                     | Pick the active throwable slot                                |

**Combo cancels:** after two light hits, press **Q** for a free Belly Slam or **F** for a free Haymaker — the heavy's wind-up is skipped.

Open `BP_Johnny` → assign every Input Action and `IMC_Default` to the matching `UPROPERTY` slots under category **Johnny|Input**.

### 4. Create the HUD
- `Content/UI/WBP_HUD` — Widget Blueprint with three progress bars labeled **HEALTH**, **STAMINA**, **HUNGER**.
- In `BP_Johnny` Event Graph, on `BeginPlay`: Create Widget → `WBP_HUD` → Add to Viewport.
- Bind the three bars to the character's `OnHealthChanged`, `OnStaminaChanged`, `OnHungerChanged` delegates.
- Add a centered subtitle TextBlock, bind it to `Voice → OnLineSpoken`.

### 5. Build the first level
- `Content/Maps/L_Skopje_Drive.umap` — opens with the playable driving scene, transitions to on-foot when the car breaks down. See `Docs/Levels.md` for the full level breakdown including the car setup checklist.

---

## Gameplay tuning at a glance

All numbers are exposed as `EditDefaultsOnly` on `JohnnyCharacter`. Adjust them in `BP_Johnny`:

- **Meters**: `MaxHealth/Stamina/Hunger`, decay/regen rates, starvation damage
- **Speeds**: walk / sprint / exhausted (when stamina hits zero, Johnny waddles)
- **Light**: jab damage, hook damage (combo finisher), range, radius, cooldown, stamina cost, combo-reset window, hook knockback
- **Belly Slam**: wind-up, recovery, AOE radius, damage, knockback, knockdown duration, stamina cost
- **Haymaker**: wind-up, recovery, cone range/radius, damage, enemy stagger duration, stamina cost
- **Parry**: window (~0.35s), whiff cooldown, parry-stagger duration on the attacker, Riposte damage multiplier
- **Throwables**: thrown-food projectile class, throw cooldown, throw speed, hotbar size
- **Finisher**: cone range/angle, low-HP threshold, Stomp damage, Eat hunger restore, "Eat instead of Stomp" hunger threshold, finisher duration
- **Hunger-as-armor**: fed/hungry thresholds (fractions of MaxHunger) and the incoming-damage multipliers each band applies
  - Fed (Hunger ≥ 75%): −25% incoming damage and super-armor frames on heavy wind-ups
  - Hungry (Hunger ≤ 25%): +50% incoming damage and every hit flinches Johnny

Voice lines are seeded with defaults in C++. To replace them, fill in the `LineSets` map on Johnny's `Voice` component in `BP_Johnny` — any entry you set there overrides the default.

---

## Content / asset checklist

Things you'll need to add (placeholders are fine):
- [ ] Johnny skeletal mesh + idle/walk/run/dodge/attack/hurt/death animation montages
- [ ] Enemy skeletal mesh + animations
- [ ] Food static meshes (one per `EFoodType`)
- [ ] Slot machine static mesh
- [ ] Skopje road + Veles city environment kit
- [ ] Subtitle / HUD widget styling
- [ ] Voice-line sound cues (assign per line in the `LineSets` map)
- [ ] Footstep + grunt audio
- [ ] Music: travel theme, chase theme, casino sting, end credits

See `Docs/Levels.md` for environment scope.
