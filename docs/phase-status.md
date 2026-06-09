# RevoltEditorBridge Phase Status

## Current Phase

Phase 17 - Packaging, docs, and offline installer preparation: Complete.

Post-Phase 17 safety enhancement - Visual dry-run diff viewer: Complete.

Post-Phase 17 utility enhancement - Project audit and fix plan system: Complete.

Post-Phase 17 desktop enhancement - Local model setup wizard: Complete.

Post-Phase 17 usability enhancement - Beginner and Advanced experience modes: Complete.

Post-Phase 17 usability enhancement - Beginner Dashboard: Complete.

Post-Phase 17 usability enhancement - New Game Prototype Wizard: Complete.

Post-Phase 17 usability enhancement - Guided Recipe System Foundation: Complete.

Post-Phase 17 recipe implementation - Create Health Pickup Guided Recipe: Complete.

Post-Phase 17 reliability pass - Desktop runtime setup, connection, model, and UI button stabilization: Complete.

Post-Phase 17 usability enhancement - Beginner Unreal connection wizard: Complete.

Post-Phase 17 usability enhancement - Beginner plan approval execution workflow: Complete.

## Completed Work

- Created `unreal/HostProject/HostProject.uproject`.
- Created `unreal/HostProject/Plugins/RevoltEditorBridge/RevoltEditorBridge.uplugin`.
- Added runtime-safe `RevoltEditorBridge` module.
- Added editor-only `RevoltEditorBridgeEditor` module.
- Added `LogRevoltEditorBridge` logging category.
- Added editor startup and shutdown log messages.
- Added a Tools menu entry named `Revolt Bridge`.
- Added a basic `Revolt Bridge` editor tab with Phase 1 placeholder sections:
  - Plugin title
  - Bridge status
  - Server controls placeholder
  - Command history placeholder
  - Approval queue placeholder
  - Offline mode indicator
- Added Phase 2 localhost-only read-only bridge controls:
  - `Start Bridge`
  - `Stop Bridge`
  - Bridge status
  - Local endpoint
  - Last command
  - Last response summary
  - Offline mode status
- Added a lightweight HTTP endpoint bound to `127.0.0.1:8765`.
- Added JSON request and response routing.
- Added read-only commands:
  - `ping`
  - `get_project_summary`
  - `get_open_level_summary`
  - `get_selected_actors`
  - `find_assets`
- Added structured errors for invalid JSON, invalid parameters, unknown commands, no open level, no selected actors, bridge already running, and bridge not running.
- Added Phase 3 approval-based editor mutation support:
  - Approval queue UI
  - Command history UI
  - `Approve Selected Command`
  - `Reject Selected Command`
  - `Clear Command History`
- Added command history fields:
  - Timestamp
  - Command name
  - Target
  - Dry-run/applied/rejected/pending status
  - Result summary
- Added safe mutating commands:
  - `set_actor_transform`
  - `set_actor_property`
  - `spawn_actor`
  - `duplicate_selected_actors`
  - `delete_generated_actors_only`
- Added dry-run previews for mutating commands.
- Added approval checks using `approved: true` or UI approval.
- Added `FScopedTransaction` wrapping for applied editor mutations.
- Added validation for actor lookup, editable properties, finite transforms, allowed spawn classes, generated actor delete safety, selected actors, open levels, and mutation permission.
- Added Phase 4 safe generated asset manipulation commands:
  - `create_folder`
  - `create_data_asset`
  - `create_material_instance`
  - `set_material_instance_parameter`
  - `bulk_edit_data_assets`
  - `save_asset`
  - `save_generated_assets`
- Added generated asset path validation for `/Game/RevoltGenerated`.
- Added helper logic to create missing generated folders.
- Added approved data asset class validation.
- Added parent material validation for material instance creation.
- Added material instance validation before parameter edits.
- Added bulk data asset edit previews before application.
- Added explicit save commands without automatic package saves.
- Added Phase 5 simple Blueprint creation commands:
  - `create_blueprint_class`
  - `add_blueprint_variable`
  - `set_blueprint_default_value`
  - `add_component_to_blueprint`
  - `compile_blueprint`
  - `get_blueprint_compile_status`
- Added generated Blueprint path validation for `/Game/RevoltGenerated/Blueprints`.
- Added safe Blueprint parent class validation for `Actor`, `Pawn`, `Character`, `ActorComponent`, `SceneComponent`, and `UserWidget`.
- Added supported Blueprint variable type validation.
- Added allowed component class validation for Phase 5 components.
- Added compile-after-modification validation.
- Added Phase 6 TypeScript MCP server under `mcp-server/`.
- Added local-only Unreal bridge client configuration:
  - `REVOLT_UE_BRIDGE_HOST`
  - `REVOLT_UE_BRIDGE_PORT`
  - `REVOLT_UE_BRIDGE_TIMEOUT_MS`
  - `REVOLT_LOG_LEVEL`
- Added MCP tools:
  - `unreal_ping`
  - `unreal_get_project_summary`
  - `unreal_get_open_level_summary`
  - `unreal_get_selected_actors`
  - `unreal_find_assets`
  - `unreal_set_actor_transform`
  - `unreal_set_actor_property`
  - `unreal_spawn_actor`
  - `unreal_create_blueprint_class`
  - `unreal_compile_blueprint`
- Added strict TypeScript/Zod input schemas for all MCP tools.
- Added `npm run test:bridge`.
- Added MCP client setup instructions in `mcp-server/README.md`.
- Added Phase 7 Electron desktop runtime shell under `desktop/`.
- Added TypeScript desktop app structure:
  - Main process
  - Preload bridge
  - Renderer shell
  - Runtime config
  - Local SQLite persistence
- Added desktop pages:
  - Dashboard
  - Unreal Connection
  - MCP Server
  - Command History
  - Approval Queue
  - Settings
  - Offline Mode
- Added SQLite-backed persistence for:
  - Projects
  - Sessions
  - Command history
  - Approval events
  - Settings
  - Model profiles placeholder
- Added dashboard status cards for:
  - Unreal bridge status
  - MCP server status
  - Offline mode status
  - Active project placeholder
- Added settings for:
  - UE bridge host
  - UE bridge port
  - MCP server command/path
  - Default permission mode
  - Enable Online Providers toggle
  - Offline Mode toggle
- Added permission modes:
  - `ReadOnly`
  - `SafeEdit`
  - `ProjectEdit`
  - `Dangerous`
- Added `config/runtime.json` to keep `Dangerous` mode unavailable by default.
- Added local Unreal bridge ping check from the desktop shell.
- Added command history recording for desktop bridge status checks.
- Added `desktop/README.md` with offline defaults and run instructions.
- Added Phase 8 local model provider routing to the desktop runtime.
- Added `ModelProvider` abstraction with:
  - `id`
  - `name`
  - `classification`
  - `supportsStreaming`
  - `supportsTools`
  - `complete`
- Added supported local model providers:
  - Ollama
  - LM Studio
  - llama.cpp server
  - Local OpenAI-compatible endpoint
- Added model profile fields:
  - Provider type
  - Display name
  - Base URL
  - Model name
  - Context length
  - Temperature
  - Max output tokens
  - Classification
- Added Model Profiles desktop UI for:
  - Creating profiles
  - Editing profiles
  - Deleting profiles
  - Selecting active model
  - Testing local connection
  - Showing provider classification
- Added Manual Command Mode when no model profile is selected.
- Added local project summary test task using Unreal `get_project_summary` and the selected local model.
- Added local-only model endpoint validation for `localhost`, `127.0.0.1`, `::1`, and `.local` hosts.
- Added Phase 9 local project indexing to the desktop runtime.
- Added local index database tables:
  - `indexed_files`
  - `file_chunks`
  - `asset_summaries`
  - `scene_summaries`
  - `index_runs`
- Added local file scanner for selected project folders.
- Added default ignored folders:
  - `Binaries`
  - `DerivedDataCache`
  - `Intermediate`
  - `Saved`
  - `.git`
  - `.vs`
  - `node_modules`
- Added indexed file extensions:
  - `.cpp`
  - `.h`
  - `.cs`
  - `.uproject`
  - `.uplugin`
  - `.ini`
  - `.json`
  - `.md`
  - `.py`
  - `.ts`
  - `.tsx`
- Added local text chunking for indexed file content.
- Added metadata-only indexing when no local model profile is active.
- Added optional local summaries through the selected local model profile.
- Added local Unreal context indexing for asset metadata and open scene summaries when the Unreal bridge is available.
- Added Project Index desktop UI with:
  - Start indexing
  - Stop indexing
  - Last index time
  - Indexed file count
  - Indexed asset count
  - Clear local index
- Added Phase 10 biome data asset system.
- Added `URevoltBiomeDataAsset` derived from `UPrimaryDataAsset`.
- Added biome configuration structs:
  - `FRevoltTerrainSettings`
  - `FRevoltTerrainStamp`
  - `FRevoltFoliageSpawnRule`
  - `FRevoltStructureSpawnRule`
  - `FRevoltObjectiveSpawnRule`
  - `FRevoltNavigationRules`
  - `FRevoltLightingRules`
- Exposed biome asset fields to the editor and Blueprints.
- Added JSON import/export helpers on `URevoltBiomeDataAsset`.
- Added bridge commands:
  - `create_biome_asset`
  - `read_biome_asset`
  - `update_biome_asset`
  - `validate_biome_asset`
  - `export_biome_asset_json`
- Added generated biome path enforcement for `/Game/RevoltGenerated/Biomes`.
- Added biome validation for empty biome names, invalid seeds, density, scale ranges, slope ranges, height ranges, missing mesh references, missing actor class references, and terrain resolution.
- Added Phase 11 `ARevoltLandGenActor` for deterministic terrain preview and generation planning.
- Added land generation actor properties:
  - `Biome`
  - `SeedOverride`
  - `GenerationBounds`
  - `bPreviewMode`
  - `GeneratedContentFolderName`
- Added CallInEditor functions:
  - `ValidateBiome`
  - `GenerateTerrainPreview`
  - `ClearTerrainPreview`
  - `RandomizeSeed`
- Added deterministic preview planning with `FRandomStream`.
- Added preview zone data for:
  - Terrain regions
  - Candidate spawn zones
  - Blocked zones
- Added debug visualization for generation bounds and preview zones.
- Added generated preview components tagged with `Revolt.Generated` and `Revolt.LandPreview`.
- Added generated-only preview clearing.
- Added bridge commands:
  - `landgen_validate`
  - `landgen_generate_preview`
  - `landgen_clear_preview`
  - `landgen_randomize_seed`
- Added Phase 12 biome-based editable actor spawning.
- Extended `ARevoltLandGenActor` with CallInEditor functions:
  - `SpawnBiomeContent`
  - `ClearGeneratedContent`
  - `BakeGeneratedContent`
  - `GetGeneratedSummary`
- Added deterministic placement preview data using biome seed or `SeedOverride`.
- Added biome content spawning from:
  - Foliage rules
  - Structure rules
  - Objective rules
- Added placement filters for:
  - Slope range
  - Height range
  - Minimum distance
  - Collision overlap
  - Generation bounds
  - Random seed
- Added generated actor tags:
  - `Revolt.Generated`
  - `Revolt.Biome.<BiomeName>`
  - `Revolt.Rule.<RuleType>`
- Added generated-only actor clearing for the active land generation actor's biome.
- Added bake support that marks generated biome actors as baked and removes `Revolt.Generated` so later clear operations do not delete baked content.
- Added bridge commands:
  - `landgen_spawn_biome_content`
  - `landgen_clear_generated_content`
  - `landgen_get_generated_summary`
  - `landgen_bake_generated_content`
- Added Phase 13 read-only project auditing commands:
  - `run_project_audit`
  - `audit_current_level`
  - `audit_blueprints`
  - `audit_assets`
  - `generate_fix_plan`
- Added Phase 14 Manual Command Mode to the desktop runtime.
- Added manual UI forms for common bridge tools and generated content workflows.
- Added raw JSON request/response display for manual commands.
- Added local JSON command history export.
- Added Phase 15 arena shooter starter template base classes:
  - `ARevoltArenaPlayerCharacter`
  - `URevoltArenaWeaponData`
  - `URevoltArenaEnemyData`
  - `URevoltArenaWaveData`
  - `URevoltArenaHealthComponent`
  - `URevoltArenaDamageComponent`
  - `ARevoltArenaWaveSpawner`
  - `ARevoltArenaPickupActor`
  - `ARevoltArenaObjectiveActor`
- Added arena shooter generated template bridge commands:
  - `create_arena_shooter_template`
  - `configure_weapon_data`
  - `configure_enemy_data`
  - `spawn_test_arena`
- Added Phase 16 zombie shooter template classes and data systems:
  - `ARevoltZombieEnemyCharacter`
  - `URevoltZombieEnemyData`
  - `URevoltZombieWaveData`
  - `ARevoltZombieWaveSpawner`
  - `ARevoltZombieExtractionZone`
  - `ARevoltZombieObjectiveActor`
  - `URevoltZombieObjectiveData`
  - `ARevoltZombieDirectorActor`
  - `URevoltInventoryComponent`
- Extended weapon data with recoil and ammo/reload structs.
- Added replication-ready health and damage component markers.
- Added AI perception component integration on the zombie base class.
- Added simple cover helper, day/night hooks, weather hooks, objective hooks, extraction zone, and save/load placeholders.
- Added zombie shooter generated template bridge commands:
  - `create_zombie_shooter_template`
  - `configure_zombie_data`
  - `configure_weapon_recoil`
  - `spawn_zombie_test_arena`
  - `configure_wave_data`
- Added `docs/templates/zombie-shooter-template.md`.
- Added Phase 17 external testing documentation:
  - Installation guide
  - Offline quickstart
  - Unreal plugin setup
  - Desktop runtime setup
  - MCP client setup
  - Ollama setup
  - LM Studio setup
  - llama.cpp setup
  - Manual Command Mode guide
  - Safety model guide
  - Biome creation guide
  - Project auditing guide
  - Troubleshooting guide
  - Packaging guide
  - Known limitations
  - Offline guarantees
- Added packaging scripts:
  - `scripts/package/Package-Plugin.ps1`
  - `scripts/package/Package-Desktop.ps1`
  - `scripts/package/Package-McpServer.ps1`
  - `scripts/package/Package-All.ps1`
- Added example prompt library under `examples/prompts`.
- Added example biome JSON files under `examples/biomes`.
- Added `VERSION.json`.
- Added `CHANGELOG.md`.

## Phase 1 Safety Notes

- No networking is implemented.
- No AI or model logic is implemented.
- No asset mutation is implemented.
- No actor mutation is implemented.
- The editor UI explicitly states that no network server is started in Phase 1.

## Phase 2 Safety Notes

- The bridge binds only to `127.0.0.1` by default.
- The bridge does not bind to `0.0.0.0`.
- No external network requests are made.
- No AI or model logic is implemented.
- No mutating commands are implemented.
- Asset and actor commands are read-only inspection commands.
- The bridge is stopped cleanly during editor module shutdown.

## Phase 3 Safety Notes

- Mutating commands require `permission_level: "editor_mutation"`.
- Mutating commands do not apply unless `dry_run: true`, `approved: true`, or approved from the UI queue.
- Dry-run requests return previews and do not modify actors.
- UI approval applies the queued command through the same validation path.
- UI rejection records a rejected command history entry and applies nothing.
- Applied actor mutations use `FScopedTransaction` where possible for Undo support.
- `spawn_actor` tags spawned actors with `Revolt.Generated`.
- `duplicate_selected_actors` tags duplicated actors with `Revolt.Generated`.
- `delete_generated_actors_only` refuses to delete any actor without `Revolt.Generated`.
- No user assets are deleted.
- No packages are automatically saved.
- No arbitrary Python or shell execution is added.

## Phase 4 Safety Notes

- Generated asset creation is restricted to `/Game/RevoltGenerated`.
- `create_folder` only creates folders under `/Game/RevoltGenerated`.
- Asset mutation commands use the existing dry-run, approval queue, and command history flow.
- `create_data_asset` only supports approved `UDataAsset` classes for Phase 4.
- `create_material_instance` requires an existing parent material.
- `set_material_instance_parameter` only edits generated material instances.
- `bulk_edit_data_assets` only edits generated data assets and previews property changes first.
- `save_asset` refuses assets outside `/Game/RevoltGenerated` unless `allow_user_asset_save: true` is explicitly provided and the command is approved.
- `save_generated_assets` only saves assets under `/Game/RevoltGenerated`.
- No asset deletion or asset rename commands are added.
- No C++ source file mutation command is added.
- No packages are saved automatically; saves happen only through explicit save commands.

## Phase 5 Safety Notes

- Generated Blueprints are created only under `/Game/RevoltGenerated/Blueprints`.
- Blueprint mutations use the existing dry-run, approval queue, and command history flow.
- Blueprint mutations may use `permission_level: "blueprint_mutation"`.
- User Blueprints outside `/Game/RevoltGenerated` are refused unless `allow_user_blueprint_edit: true` is explicitly provided and the command is approved.
- `create_blueprint_class` only supports safe parent classes: `Actor`, `Pawn`, `Character`, `ActorComponent`, `SceneComponent`, and `UserWidget`.
- `add_blueprint_variable` supports simple scalar variable types only in Phase 5.
- `add_component_to_blueprint` supports allowed simple component classes only.
- No complex Blueprint graphs are created or edited in this phase.
- Blueprint compile validation runs after applied modifications.
- No asset deletion, rename, C++ source edits, Python execution, shell execution, or external network calls are added.

## Phase 6 Safety Notes

- MCP server is offline-first.
- MCP server does not call cloud APIs.
- MCP server does not implement model inference.
- MCP server only communicates with the local Unreal bridge by default.
- `REVOLT_UE_BRIDGE_HOST` is restricted to `127.0.0.1` or `localhost` in Phase 6.
- Mutating MCP tools include `dryRun` and `approved`.
- Mutating MCP tools default to `dryRun: true` unless `approved: true` is explicitly provided.

## Phase 7 Safety Notes

- Desktop runtime is offline-first.
- Desktop runtime does not require login.
- Desktop runtime does not implement telemetry.
- Desktop runtime does not implement model inference.
- Desktop runtime does not configure cloud providers by default.
- Unreal bridge host is validated to `127.0.0.1` or `localhost`.
- `Enable Online Providers` defaults to `false`.
- `Offline Mode` defaults to `true`.
- `Dangerous` permission mode is disabled unless `desktop/config/runtime.json` explicitly enables it.
- Desktop persistence uses local SQLite data only.

## Phase 8 Safety Notes

- Local model routing is offline-first.
- Online providers do not exist in Phase 8.
- Model profile classification is limited to `offline` or `local-network`.
- `Enable Online Providers` remains present but defaults to `false`.
- Model base URLs are restricted to local hosts only.
- No telemetry, login, cloud provider, or online API calls are added.
- Project summary data is only sent to the selected local model endpoint.
- If no model is configured, the app stays usable in Manual Command Mode.

## Phase 9 Safety Notes

- Indexing is local-only.
- Indexed content is stored only in the local SQLite runtime database.
- Project data is not uploaded.
- Cloud APIs are not called.
- Cloud embeddings are not required or implemented.
- No remote vector database is added.
- Ignored folders are skipped before file indexing.
- Local model summaries are optional and use only the selected local/offline model endpoint.
- If no model is selected, indexing stores metadata and chunks only.

## Phase 10 Safety Notes

- Biome assets are data-only configuration assets.
- No terrain generation is implemented in Phase 10.
- Generated biome assets are restricted to `/Game/RevoltGenerated/Biomes`.
- `create_biome_asset` and `update_biome_asset` are approval-gated mutating commands.
- Biome mutations support dry-run previews.
- Biome updates use `FScopedTransaction` for undo support.
- Packages are marked dirty but not automatically saved.
- No actor spawning, terrain mutation, asset deletion, or source file mutation is added.

## Phase 11 Safety Notes

- Land generation is preview/planning-only in Phase 11.
- No final Landscape editing is implemented.
- Generated preview content is component-based debug data.
- Generated preview components are tagged `Revolt.Generated` and `Revolt.LandPreview`.
- `ClearTerrainPreview` only removes generated preview components with both preview tags.
- Manually placed actors and untagged components are not removed.
- Bridge preview, clear, and seed randomization commands are approval-gated mutations.
- Preview generation uses deterministic `FRandomStream` seeded by `SeedOverride` or the assigned biome seed.

## Phase 12 Safety Notes

- Biome content spawning creates editable level actors.
- Generated actors are tagged `Revolt.Generated` and `Revolt.Biome.<BiomeName>` at spawn time.
- `ClearGeneratedContent` only removes actors with both `Revolt.Generated` and the active biome tag.
- Manually placed actors are not deleted.
- Dry-run spawning returns placement previews without spawning actors.
- Spawn, clear, and bake bridge commands require the existing dry-run/approval mutation flow.
- Baked generated actors are marked `Revolt.Baked` and no longer have `Revolt.Generated`, so clear operations do not remove them.
- Placement is deterministic from the effective biome seed.

## Phase 13 Safety Notes

- Project auditing commands are read-only.
- No assets, actors, Blueprints, packages, or project files are modified by audit commands.
- `generate_fix_plan` returns recommended next actions only and does not execute fixes.
- Audits inspect the current level, Blueprint compile status, and asset registry metadata where practical.
- Fix plan entries include permission level and suggested bridge command metadata for future manual approval flows.
- No auto-fix command is implemented in Phase 13.
- No external network request is made.

## Phase 14 Safety Notes

- Manual Command Mode is available in the desktop runtime when no model profile is configured.
- Manual commands communicate only with the configured local Unreal bridge endpoint.
- Manual Mode does not require internet, cloud providers, or a local model.
- Mutating manual commands default to `dry_run: true`.
- Mutating manual commands include explicit approval and permission fields before being sent.
- Raw JSON request and response bodies are shown for advanced users.
- Command history export writes a local JSON file only.
- No cloud features, telemetry, or model inference are added in Phase 14.

## Phase 15 Safety Notes

- Arena shooter template assets are restricted to `/Game/RevoltGenerated/Templates/ArenaShooter`.
- Template creation uses the existing mutation approval, dry-run, and command history flow.
- Existing generated template assets are detected and not overwritten by template creation.
- Weapon and enemy tuning commands only edit generated arena template data assets.
- Test arena spawning only creates generated actors tagged `Revolt.Generated` and `Revolt.Template.ArenaShooter`.
- Packages are marked dirty but are not automatically saved.
- No user assets are overwritten.
- The full zombie shooter kit is not implemented in this phase.

## Phase 16 Safety Notes

- Zombie shooter template assets are restricted to `/Game/RevoltGenerated/Templates/ZombieShooter`.
- Zombie template creation uses the existing mutation approval, dry-run, and command history flow.
- Existing generated zombie template assets are detected and not overwritten.
- Zombie, weapon recoil, and wave tuning commands only edit generated zombie template data assets.
- Test arena spawning only creates generated actors tagged `Revolt.Generated` and `Revolt.Template.ZombieShooter`.
- Health replication support is marked as a template assumption; full multiplayer authority/prediction is not implemented.
- Save/load functions are placeholders and do not write files.
- No user content is overwritten or deleted.

## Phase 17 Safety Notes

- Packaging scripts build local artifacts only.
- Documentation states default use does not require internet after local dependencies are installed.
- Offline guarantees document no telemetry, no cloud provider by default, localhost-only Unreal bridge, local SQLite database, and local model support.
- Online providers are described only as optional future/default-disabled features.
- Example prompts instruct dry-run and approval for mutating workflows.
- Example biome JSON files are local examples and do not create assets until an approved bridge command is run.
- No user content is overwritten or deleted.

## Visual Dry-Run Diff Viewer Safety Notes

- Added a `Dry-Run Diff Viewer` section to the Unreal `Revolt Bridge` tab.
- Mutating commands now run a dry-run preview before approval and attach structured `diff` JSON with `dryRun`, `requiresApproval`, and `riskLevel` fields.
- Diff entries include command id, command name, target type/path, changed property or field, old value, new value, risk level, required permission level, and approval status.
- Pending approval commands store their diff entries in the approval queue, and the UI can approve/reject selected changes, approve/reject the entire command, clear rejected entries, or copy the diff as JSON.
- Approved and rejected commands update command history with user approval/rejection context, timestamp, change count, risk level, and result summary.
- Dangerous-risk operations are still constrained by the existing command safety model and generated-content restrictions.

## Project Audit and Fix Plan Safety Notes

- Added a read-only `Project Audit` section to the Unreal `Revolt Bridge` tab.
- Added UI buttons for current level, selected actors, assets, Blueprints, generated content, full project audit, fix plan generation, and local JSON export.
- Added bridge commands `audit_selected_actors`, `audit_generated_content`, `run_full_project_audit`, and `export_audit_report` alongside existing audit commands.
- Audit issue JSON now includes structured fields such as `issueId`, `severity`, `category`, `targetType`, `targetPath`, `confidence`, `autoFixAvailable`, `requiredPermission`, `suggestedCommand`, and `suggestedParams`.
- Fix plans remain suggestions only and include `executed: false`.
- Audit and export commands do not modify actors, assets, Blueprints, source files, maps, project settings, or generated content.

## Local Model Setup Wizard Safety Notes

- Added a desktop `Local Model Setup` page for Ollama, LM Studio, llama.cpp server, and local OpenAI-compatible endpoints.
- The wizard detects local endpoints, fetches model lists where available, allows manual model entry, runs a safe local test prompt, saves profiles to SQLite, and can set a profile active.
- Model profiles now track provider classification, streaming/tools support, enabled state, created timestamp, last tested timestamp, and last test status.
- Dashboard model status shows the active profile, provider type, offline/local classification, and last test result.
- The no-model fallback remains: `No local model is configured. Manual Command Mode is still available.`
- Supported providers are offline or local-network only; no cloud providers were added.
- External URLs remain blocked unless `Enable Online Providers` is explicitly enabled.
- The safe test prompt does not include Unreal project context.

## Beginner and Advanced Mode Safety Notes

- Added persisted `experienceMode` setting with allowed values `beginner` and `advanced`.
- Default experience mode is `beginner`.
- Beginner Mode adds a guided home screen with simple actions for playable levels, prototypes, enemies, weapons, pickups, biomes/worlds, audits, and test packaging.
- Beginner Mode hides advanced navigation for raw MCP status, raw JSON command editing, low-level history/approval debugging, direct model profile management, and project indexing.
- Advanced Mode preserves the existing full desktop runtime toolset.
- The mode switch is available in the header and Settings page and persists locally through the existing SQLite settings layer.
- Offline-first behavior, cloud-disabled defaults, and no-telemetry rules remain unchanged.

## Beginner Dashboard Safety Notes

- Expanded Beginner Mode into a `Beginner Dashboard` with project connection status, Unreal bridge status, active model status, Manual Command Mode availability, Project Health placeholder, and Next Recommended Action placeholder.
- Added beginner action cards for making a level playable, creating a prototype, adding enemies, adding a weapon, adding health pickups, creating a main menu, creating a biome/world, checking the game for problems, and opening tutorials.
- Each card includes a title, one-sentence description, difficulty label, estimated result, and button.
- Dashboard buttons only route to safe placeholders or existing workflows; no commands execute automatically.
- Advanced screens are preserved and remain available in Advanced Mode.
- Offline messaging states that the tool works offline, local models are optional, and manual workflows are always available.

## New Game Prototype Wizard Safety Notes

- Added a beginner `What Do You Want to Build?` wizard from the Beginner Dashboard.
- The wizard captures game type, scope, gameplay style, and setup style, then creates a local dry-run plan.
- Plans include required systems, suggested generated assets, suggested map setup, a first playable milestone, and the next recommended action.
- The latest wizard plan is stored locally in SQLite and survives app restart.
- `Create Dry-Run Plan` does not call Unreal, execute bridge commands, create assets, mutate files, or require a local model.
- `Send to Guided Recipe` is a placeholder that records a local history entry only.
- Offline-first behavior remains unchanged; no cloud providers, telemetry, or external network calls were added.

## Guided Recipe System Foundation Safety Notes

- Added a desktop `Recipes` page for Beginner Mode.
- Added a recipe schema with recipe id, title, descriptions, category, difficulty, permission level, output summary, form fields, generated command plan, dry-run support, and approval requirement.
- Added recipe categories for Player, Enemy, Weapon, Pickup, UI, Level, Biome, Objective, and Utility, with Easy, Medium, and Advanced difficulty labels.
- Added placeholder recipes:
  - Create Health Pickup
  - Create Basic Enemy
  - Create Simple Weapon
  - Add PlayerStart
  - Create Main Menu
  - Create Zombie Arena
- Added schema-driven form rendering for text, number, select, checkbox, asset path, actor class, and vector/location fields.
- Recipe execution defaults to `dry_run: true`, `approved: false`, and the recipe's required permission level.
- Supported recipe steps route through the existing local Unreal bridge dry-run/approval path.
- Unsupported placeholder commands are recorded as preview-only and are not executed silently.
- Recent recipe runs are persisted locally in SQLite.
- No cloud calls, telemetry, model requirement, or external network calls were added.

## Create Health Pickup Guided Recipe Safety Notes

- Implemented the `Create Health Pickup` recipe as the first complete guided recipe.
- The recipe is categorized as `Pickup`, marked `Easy`, and uses the beginner description: `Creates an item the player can collect to restore health.`
- Added form fields for Blueprint name, heal amount, respawn time, optional pickup mesh path, optional pickup sound path, optional level placement, location, and generated folder path.
- Defaults are `BP_HealthPickup`, heal amount `25`, respawn time `10`, place in current level `false`, and generated folder `/Game/RevoltGenerated/Pickups`.
- Generated Blueprint assets are restricted to `/Game/RevoltGenerated`, with the recipe defaulting to `/Game/RevoltGenerated/Pickups`.
- The recipe generates a command plan using existing safe commands:
  - `create_blueprint_class`
  - `add_component_to_blueprint`
  - `add_blueprint_variable`
  - `set_blueprint_default_value`
  - `compile_blueprint`
  - `spawn_actor` only when `Place in current level` is selected
- Recipe dry-run creates a local command-plan preview first; no asset is created during dry-run.
- Applying the recipe requires the explicit desktop approval checkbox and sends approved commands through the local Unreal bridge.
- Added beginner approval summary: `This will create a generated Blueprint for a health pickup. It will not edit your existing assets.`
- Added a post-run result panel for Blueprint creation, variables, compile result, placement, next recommended step, and missing behavior notes.
- Complex Blueprint graph gameplay behavior is not generated yet; the recipe clearly labels overlap/health-restore logic as a next step.
- No existing user asset is modified, no asset is deleted, no package is automatically saved, and no cloud/network behavior was added.

## Make This Level Playable Workflow Safety Notes

- Added a Beginner Mode `Make This Level Playable` workflow from the Beginner Dashboard.
- The workflow runs read-only checks first by calling local Unreal bridge summary/audit commands; it does not modify actors, assets, maps, project settings, or generated content during checking.
- The check reports beginner-friendly readiness results for player spawn, player movement, lighting, enemy movement, and objective presence.
- Current-level audit metadata now includes read-only signals for PlayerStart, GameMode, playable Pawn/Character, lighting, camera setup, AI-like actors, NavMesh, and simple objective markers.
- The workflow generates fix-plan items with beginner explanations, technical commands, risk level, generated-content scope, and dry-run availability.
- Suggested fixes include adding a player spawn point, generated player character, default GameMode review, basic lighting, enemy walking area, and simple objective marker.
- Fixes never execute automatically; `Dry-run selected fix` and `Dry-run all safe fixes` send commands with `dry_run: true` and `approved: false`.
- Dry-run previews route through the existing local Unreal bridge diff/approval system; approval is still required before any mutating command can apply.
- Beginner Mode hides technical JSON by default and exposes it only through `Show Technical Details`.
- No user assets are overwritten, no cloud calls are added, and offline-first behavior remains unchanged.

## Desktop Runtime Stabilization Safety Notes

- Added centralized desktop app defaults so renderer state has safe values before IPC, Unreal, MCP, model providers, indexing, recipes, or approvals respond.
- Electron startup now supports an optional renderer dev-server URL through `REVOLT_RENDERER_DEV_URL` or `VITE_DEV_SERVER_URL`, and otherwise loads the built renderer from the correct production path.
- Renderer load failures now log clear errors and display a local fallback page instead of a blank window.
- The desktop renderer initializes from safe defaults first, then merges IPC state when available.
- Startup no longer requires Unreal, MCP, Ollama, LM Studio, llama.cpp, or any local model endpoint to be running.
- Unreal connection checks now update offline status cards instead of throwing uncaught UI errors.
- Model profile row actions, active model selection, folder picking, and indexing stop actions now report friendly messages on failure.
- Index-status refresh failures are caught so background polling does not create uncaught promise errors.
- Date formatting now handles empty or invalid timestamps safely.
- No cloud providers, telemetry, online calls, destructive commands, or feature rewrites were added.

## Emergency Desktop Button Stabilization Notes

- Persisted settings now merge with defaults and ignore corrupted individual setting values instead of breaking startup.
- Renderer array reads now use safe-array fallbacks for histories, approvals, model profiles, recipes, recipe runs, prototype plan lists, playable fix plans, and model detection results.
- Added a lightweight notification system for controlled warnings and errors.
- Added a vanilla TypeScript renderer error boundary for uncaught render errors and rejected button promises; the app is not React-based, so no React dependency was added.
- Added a safe button action wrapper and applied it to key repeated actions such as history clearing/export and manual JSON refresh.
- Added a setup checklist dashboard card that summarizes optional Unreal, MCP, and local-model readiness.
- Local model form and wizard endpoints are validated as localhost/private LAN by default.
- Missing DOM nodes are safely skipped with a warning instead of blanking the whole app.
- Dashboard, Unreal Connection, Local Model Setup, Manual Command Mode, Settings, and Beginner workflows remain usable with Unreal, MCP, and local model services disconnected.

## Beginner Unreal Connection Wizard Notes

- Added a `Connect to Unreal` wizard reachable from the Dashboard, Setup Checklist, Unreal Connection page, and Beginner Mode home.
- The wizard guides users through opening Unreal, enabling the plugin, opening `Tools > Revolt Bridge`, clicking `Start Bridge`, and testing the local connection.
- The default local endpoint is now `http://127.0.0.1:8765`.
- Added `testDefaultUnrealConnection()` with structured statuses: `connected`, `unreachable`, `invalid_response`, `timeout`, `invalid_endpoint`, and `unknown_error`.
- Unreal connection testing never requires MCP, a local model, internet access, or cloud services.
- Beginner success and failure messages explain the next step without exposing raw technical details by default.
- Advanced host, port, endpoint, timeout, and reset controls are collapsed behind advanced settings.
- The MCP server default bridge port and setup docs now match the Unreal plugin default port.

## Beginner Plan Approval Execution Notes

- Added a Beginner Dashboard typed request box for simple local intents such as `make a map`.
- Typed input creates a beginner-readable dry-run plan only; it does not apply Unreal changes immediately.
- The `make a map` intent maps to the existing local `spawn_test_arena` Unreal bridge command.
- Dry-run execution uses `dry_run: true`, `approved: false`, and `permission_level: editor_mutation`.
- Approved execution requires the user to click `Approve and Create Map`, then sends `dry_run: false` and `approved: true`.
- The workflow shows planned changes, risk level, permission level, dry-run result, approved execution result, technical JSON, and next steps.
- `spawn_test_arena` is now included in the desktop mutating-command safety defaults so dry-run remains default unless approval is explicit.
- Generated pickup and objective actors spawned by the arena workflow are tagged with `Revolt.Generated`.
- The workflow uses the existing desktop-to-Unreal localhost bridge at `http://127.0.0.1:8765`; no MCP, local model, cloud call, or telemetry is required.

## Acceptance Tests

1. Beginner Dashboard appears in Beginner Mode.
2. Advanced Mode bypasses the Beginner Dashboard.
3. Action cards render correctly.
4. Buttons route to existing or placeholder workflows.
5. Unreal connection status is visible.
6. No commands execute automatically from the dashboard.
7. App still works offline.
8. Prototype Wizard opens from Beginner Dashboard.
9. User can select game type, scope, gameplay style, and setup style.
10. Wizard produces a readable recommended plan.
11. No Unreal project changes happen during planning.
12. Latest wizard plan persists locally.
13. App works with no model configured.
14. Recipes page appears in Beginner Mode.
15. Recipe cards render and open detail views.
16. Recipe forms render from schema fields.
17. Generated command plan preview appears.
18. Recipe execution defaults to dry-run and approval-required.
19. Unsupported placeholder recipe commands do not execute silently.
20. Recent recipe runs persist locally.
21. Create Health Pickup appears in the Recipes page.
22. Create Health Pickup form defaults are correct.
23. Dry-run shows planned Blueprint creation, component, variables, defaults, and compile command.
24. Approval is required before asset creation.
25. Generated pickup Blueprint path stays under `/Game/RevoltGenerated`.
26. Approved recipe creates the Blueprint when the Unreal bridge supports the underlying Blueprint commands.
27. Result panel explains created structure, compile status, placement, and missing gameplay behavior.
28. No existing user asset is modified.
29. `Make This Level Playable` opens from the Beginner Dashboard.
30. The workflow checks the current level without modifying it.
31. A missing PlayerStart is shown as `Player cannot spawn`.
32. The fix plan suggests adding a player spawn point.
33. Dry-run selected fix produces a bridge dry-run preview.
34. Approval remains required before any fix can apply.
35. Beginner summaries are readable without Unreal jargon.
36. Technical details are hidden by default but available on request.
37. The workflow works offline against the localhost Unreal bridge.
38. `npm run dev` opens a visible desktop app.
39. The app does not show a blank window when Unreal, MCP, and local models are disconnected.
40. The renderer has safe default state for Unreal, MCP, model, settings, history, approvals, recipes, tutorials, and errors.
41. Unreal connection checks show offline status instead of throwing uncaught render errors.
42. Model setup/profile buttons show clear messages when endpoints or profiles are missing.
43. Background index refresh does not throw uncaught errors when local services are unavailable.
44. Visible button failures produce a friendly notification or inline message instead of `Cannot read properties of undefined`.
45. Setup checklist card appears on the Dashboard.
46. Model endpoint forms reject non-local URLs unless online providers are explicitly enabled.
47. Corrupt or missing persisted settings fall back to defaults.
48. `Connect to Unreal` appears from beginner and dashboard entry points.
49. The Unreal connection wizard shows the five beginner setup steps.
50. `Test Connection` reports a friendly offline message when Unreal is disconnected.
51. Default Unreal endpoint is `http://127.0.0.1:8765`.
52. Advanced connection settings are collapsed by default.
53. Typing `make a map` on the Beginner Dashboard creates a dry-run plan without applying changes.
54. `Approve and Create Map` is disabled until the dry-run succeeds.
55. Clicking `Approve and Create Map` sends the approved Unreal bridge command.
56. Generated arena actors are tagged with `Revolt.Generated`.

## Manual Test Procedure

1. Build the desktop runtime with `cd desktop; npm run build`.
2. Launch the app with `npm run dev` with internet disconnected.
3. Confirm the app opens to `Beginner Dashboard` when `Beginner Mode` is selected.
4. Confirm project connection, Unreal bridge, active model, Manual workflows, Project Health, and Next Recommended Action cards are visible.
5. Confirm all beginner action cards render with difficulty, estimated result, and buttons.
6. Click each beginner action and confirm only the guided placeholder/result text changes.
7. Confirm no bridge command is sent automatically from the dashboard.
8. Click `Create a new game prototype` and confirm the `What Do You Want to Build?` wizard opens.
9. Choose a game type, scope, gameplay style, and setup style, then click `Create Dry-Run Plan`.
10. Confirm the plan shows systems, assets, map setup, first milestone, next action, safety notes, and raw local JSON.
11. Restart the app and confirm the latest plan is still visible.
12. Click `Send to Guided Recipe` and confirm it reports a placeholder without running Unreal commands.
13. Open `Recipes` in Beginner Mode and confirm recipe cards render.
14. Open each recipe and confirm the schema form and generated command plan preview render.
15. Click `Run Dry-Run` for `Add PlayerStart` with Unreal bridge running and confirm it routes through the bridge dry-run approval flow.
16. Click `Run Dry-Run` for an unsupported placeholder recipe and confirm it records preview-only without executing a bridge command.
17. Restart the app and confirm recent recipe runs are still listed.
18. Open `Create Health Pickup` and confirm defaults are `BP_HealthPickup`, `25`, `10`, place in current level unchecked, and generated folder `/Game/RevoltGenerated/Pickups`.
19. Click `Preview Command Plan` and confirm planned commands include Blueprint creation, StaticMeshComponent, HealAmount, RespawnTime, defaults, and compile.
20. Click `Run Dry-Run` and confirm the result panel states no Blueprint was created during dry-run.
21. Check the approval box, click `Apply Approved Recipe`, and confirm the generated Blueprint is created under `/Game/RevoltGenerated/Pickups` if Unreal bridge is running.
22. Confirm the result panel reports variables, compile status, optional placement, and the missing overlap/health-restore behavior as a next step.
23. From the Beginner Dashboard, click `Make this level playable` and confirm the workflow page opens.
24. Click `Run Level Check` and confirm no Unreal changes occur.
25. In a level without a PlayerStart, confirm the summary says `Player can spawn: No` and explains that the player cannot spawn.
26. Confirm the fix plan includes `Add player spawn point`.
27. Select `Add player spawn point`, click `Dry-run selected fix`, and confirm a dry-run response appears with approval still required.
28. Click `Show Technical Details` and confirm raw workflow/dry-run JSON becomes visible.
29. Run `cd desktop; npm run build` and confirm the TypeScript build succeeds.
30. Run `cd desktop; npm run dev` with Unreal, MCP, and local model endpoints stopped.
31. Confirm a visible desktop window opens and the dashboard renders instead of a blank page.
32. Click `Check Unreal Bridge` and confirm the UI reports the bridge as offline without a fatal render error.
33. Open `Local Model Setup`, test an endpoint that is not running, and confirm a clear failure message appears.
34. Confirm the Dashboard shows the setup checklist card.
35. Click visible Dashboard, Settings, Unreal Connection, Local Model Setup, and Manual Command Mode buttons with services stopped and confirm controlled messages appear.
36. Switch to `Advanced Mode` and confirm the app bypasses the Beginner Dashboard and preserves advanced tools.
37. Open `Connect to Unreal` and confirm the wizard explains each setup step.
38. With Unreal closed, click `Test Connection` and confirm the app reports a controlled unreachable/offline message.
39. Confirm advanced connection settings show host `127.0.0.1`, port `8765`, and endpoint `http://127.0.0.1:8765`.
40. Type `make a map` in the Beginner Dashboard request box and click `Create Dry-Run Plan`.
41. Confirm the plan shows the generated arena actors and does not change the level during dry-run.
42. Click `Approve and Create Map` and confirm Unreal receives the approved `spawn_test_arena` command.
43. Confirm the result panel shows success or the specific Unreal bridge failure.

## Known Documentation Gap

- `docs/architecture.md` and `docs/safety-model.md` were requested for reading but are not present in the repository yet.

## Build Configuration

- Added explicit Unreal Engine path configuration at `config/unreal-engine.json`.
- Configured the current local engine root as `E:\UE5\UE_5.7`.
- Added build helper scripts under `scripts/build-plugin`.
- Added custom path detection through `-EngineRoot`, `UE_ENGINE_DIR`, `config/unreal-engine.json`, `E:\UE5\UE_5.7`, and default Epic Games folders.
- Added build instructions in `docs/building.md`.
- Build helper validates UnrealBuildTool write access to `%LOCALAPPDATA%\UnrealEngine\Intermediate\Build`, `%LOCALAPPDATA%\Unreal Engine`, `%APPDATA%\Unreal Engine\UnrealBuildTool`, and `%LOCALAPPDATA%\UnrealBuildTool` before compiling, because installed Unreal builds use those paths even when the engine is installed in a custom location.
- Build helper passes `-NoUBA` to avoid requiring `C:\ProgramData\Epic\UnrealBuildAccelerator` access in restricted environments.

## Phase List

- Phase 0 - Project documentation and repo setup
- Phase 1 - UE5 plugin skeleton
- Phase 2 - Read-only localhost Unreal bridge
- Phase 3 - Safe editor mutations
- Phase 4 - Asset manipulation tools
- Phase 5 - Blueprint creation tools
- Phase 6 - TypeScript MCP server
- Phase 7 - Offline desktop runtime shell
- Phase 8 - Offline local model routing
- Phase 9 - Local project indexing
- Phase 10 - Biome Data Asset system
- Phase 11 - Land Generation Actor
- Phase 12 - Biome-based editable actor spawning
- Phase 13 - Project auditing and fix plans
- Phase 14 - Manual command mode
- Phase 15 - Arena shooter starter template
- Phase 16 - Advanced zombie shooter template
- Phase 17 - Packaging, docs, and offline installer preparation
