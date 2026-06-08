# Revolt Desktop Runtime

Offline-first Electron desktop shell for Revolt Editor Bridge.

## Offline Defaults

- No login is required.
- No telemetry is implemented.
- Model inference is local-only in Phase 8 and requires a user-created local model profile.
- No cloud provider is configured by default.
- Unreal bridge communication defaults to `127.0.0.1:8765`.
- Experience mode defaults to `Beginner Mode`.
- Dangerous permission mode is disabled unless `config/runtime.json` explicitly enables it.

## Install and Run

```powershell
cd desktop
npm install
npm run dev
```

The app stores local runtime data in SQLite under Electron's user data directory.

## Pages

- Dashboard
- Beginner Home
- Prototype Wizard
- Recipes
- Unreal Connection
- MCP Server
- Command History
- Approval Queue
- Settings
- Offline Mode
- Model Profiles
- Local Model Setup
- Project Index

## Settings

Settings are stored locally and persist after restart:

- UE bridge host
- UE bridge port
- MCP server command/path
- Default permission mode
- Enable Online Providers toggle, default `false`
- Offline Mode toggle, default `true`
- Active model profile, default empty for Manual Command Mode
- Experience Mode, default `beginner`

## Beginner Dashboard and Advanced Mode

Beginner Mode shows guided workflows and hides risky technical tools. Advanced Mode exposes the full command and debugging interface.

Beginner Mode opens to a Beginner Dashboard with:

- Project connection status
- Unreal bridge status
- Active model status
- Manual workflow availability
- Project Health placeholder
- Next recommended action placeholder

Beginner Dashboard action cards:

- Make this level playable
- Create a new game prototype
- Add enemies
- Add a weapon
- Add health pickups
- Create a main menu
- Create a biome/world
- Check my game for problems
- Open tutorials

Dashboard buttons route to safe placeholders or existing workflows and do not execute commands automatically.

The `Create a new game prototype` action opens `What Do You Want to Build?`, a beginner wizard that stores the latest local dry-run plan in SQLite. It asks for game type, scope, gameplay style, and setup style, then suggests required systems, generated assets, map setup, a first playable milestone, and the next safe action. It does not execute Unreal commands or require a local model.

The `Recipes` page wraps technical bridge commands in beginner forms. Recipe cards open detail views, render schema-driven fields, preview generated command plans, and default execution to dry-run with approval required. Unsupported placeholder commands are recorded as preview-only and do not execute silently. Recent recipe runs are stored locally in SQLite.

`Create Health Pickup` is the first complete recipe. It creates a generated Blueprint asset structure under `/Game/RevoltGenerated/Pickups`, adds beginner-tunable variables such as `HealAmount` and `RespawnTime`, compiles the Blueprint when approved, and optionally places it in the current level. Gameplay overlap/health-restore graph logic is intentionally labeled as a next step.

Advanced Mode preserves the existing full toolset, including raw JSON command editing, MCP status, command history, approval queue, model profile management, and project indexing.

## Local Model Profiles

The desktop runtime supports local/offline profile routing for:

- Ollama
- LM Studio
- llama.cpp server
- Local OpenAI-compatible endpoint

Use the `Local Model Setup` page for a guided offline wizard:

1. Choose a provider.
2. Detect the local endpoint.
3. Confirm the base URL.
4. Fetch or enter the model name.
5. Test the connection.
6. Run the safe local test prompt.
7. Save the model profile.
8. Set it as active.

Default endpoints:

- Ollama: `http://127.0.0.1:11434`
- LM Studio: `http://127.0.0.1:1234`
- llama.cpp: `http://127.0.0.1:8080`
- Local OpenAI-compatible: user-entered localhost or private LAN URL

Each profile stores:

- Profile ID
- Provider type
- Display name
- Base URL
- Model name
- Context length
- Temperature
- Max output tokens
- Classification, either `offline` or `local-network`
- Streaming/tools support flags
- Created and last-tested timestamps
- Last test status

Online model providers are not added in this phase. Base URLs are validated as local/private hosts by default:

```text
localhost
127.0.0.1
::1
*.local
10.x.x.x
172.16.x.x - 172.31.x.x
192.168.x.x
```

If no active model profile is selected, the app remains usable in Manual Command Mode.

## Local Project Index

Phase 9 adds local-only indexing for selected project folders.

The index stores local SQLite records for:

- Indexed files
- File chunks
- Asset summaries
- Scene summaries
- Index runs

Ignored folders by default:

- `Binaries`
- `DerivedDataCache`
- `Intermediate`
- `Saved`
- `.git`
- `.vs`
- `node_modules`

Indexed file extensions:

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

If an active local model profile is selected, file and Unreal metadata summaries are generated through that local endpoint only. If no model is selected, indexing stores metadata and chunks only.

No remote vector database or cloud embeddings are used.

## Dangerous Mode

`Dangerous` is unavailable by default. To make the option selectable for explicit testing, edit:

```text
desktop/config/runtime.json
```

Set:

```json
{
  "dangerousModeAvailable": true
}
```

Do not enable this for normal offline-safe operation.
