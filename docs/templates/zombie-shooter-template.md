# Zombie Shooter Template

Phase 16 expands the Arena Shooter starter into a modular zombie shooter framework.

## Generated Location

All generated template assets must live under:

```text
/Game/RevoltGenerated/Templates/ZombieShooter
```

Bridge commands reject tuning requests outside that folder.

## Runtime Classes

- `ARevoltZombieEnemyCharacter` - Zombie enemy base class with health, damage, AI perception component, and simple cover helper.
- `URevoltZombieEnemyData` - Data asset for zombie health, speed, bite damage, perception radii, cover behavior, and spawn class.
- `URevoltZombieWaveData` - Data asset for zombie wave entries plus day/night and weather scaling flags.
- `ARevoltZombieWaveSpawner` - Call-in-editor spawner that creates generated zombie actors from wave data.
- `ARevoltZombieExtractionZone` - Generated extraction target actor.
- `ARevoltZombieObjectiveActor` and `URevoltZombieObjectiveData` - Small objective system for extraction-style goals.
- `ARevoltZombieDirectorActor` - Hook container for day/night, weather, and save/load placeholders.
- `URevoltInventoryComponent` - Lightweight inventory framework for weapon data assets.

The template also extends `URevoltArenaWeaponData` with recoil and ammo/reload data.

## Generated Data Assets

`create_zombie_shooter_template` creates these assets when missing:

- `Data/Weapons/DA_Weapon_Carbine`
- `Data/Weapons/DA_Weapon_ZombieShotgun`
- `Data/Zombies/DA_Zombie_Shambler`
- `Data/Zombies/DA_Zombie_Runner`
- `Data/Zombies/DA_Zombie_Brute`
- `Data/Waves/DA_Zombie_Wave_Test`
- `Data/Objectives/DA_Objective_Extract`
- `Maps/L_ZombieShooter_Test`

Existing generated assets are not overwritten.

## Bridge Commands

- `create_zombie_shooter_template` - Creates the generated folders, data assets, and test map if missing.
- `configure_zombie_data` - Tunes generated zombie enemy data.
- `configure_weapon_recoil` - Tunes generated zombie weapon recoil and ammo/reload fields.
- `configure_wave_data` - Tunes generated zombie wave composition.
- `spawn_zombie_test_arena` - Spawns generated test actors in the open level.

All commands are mutating commands and therefore use the existing dry-run, approval, permission, and command history flow.

## Multiplayer Assumptions

- Health and damage components are marked replication-ready.
- `CurrentHealth` is replicated by the health component.
- Full multiplayer gameplay authority, prediction, lag compensation, replicated inventory state, and networked UI are placeholders only in Phase 16.
- Use `bEnableReplicationSupport` fields as design markers for Blueprint or later C++ expansion.

## Save/Load Assumptions

`ARevoltZombieDirectorActor` exposes `SaveGamePlaceholder` and `LoadGamePlaceholder`.

These functions intentionally return `false` in Phase 16. They are hooks for later save-system work and do not write files.

## Safety

- No user content is overwritten.
- No assets are deleted.
- No generated packages are automatically saved.
- Spawned test actors are tagged `Revolt.Generated` and `Revolt.Template.ZombieShooter`.
