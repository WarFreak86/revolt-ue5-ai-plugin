# Revolt UE5 AI Plugin - Agent Instructions

You are working on a UE5 C++ editor plugin and companion TypeScript MCP/Desktop runtime.

## Primary goals

Build the project in small, safe phases. Do not skip phases. Do not introduce destructive editor actions without approval gates.

## Rules

1. Keep Unreal editor-only code inside editor modules.
2. Do not block the Unreal game/editor thread with model calls, large JSON parsing, workspace indexing, or expensive generation.
3. Mutating editor actions must support dry-run mode, approval, logging, and undo where possible.
4. Do not add arbitrary shell execution.
5. Do not add arbitrary Python execution until explicitly requested in a later phase.
6. Do not delete user assets.
7. Any generated asset must go under `/Game/RevoltGenerated`.
8. Any generated actor must be tagged with `Revolt.Generated`.
9. Keep commands schema-driven with structured JSON responses.
10. After each phase, update `docs/phase-status.md`.

## Coding style

- Prefer simple, readable C++ over clever abstractions.
- Use Unreal naming conventions.
- Add useful comments around editor-only APIs and async boundaries.
- Avoid large rewrites unless asked.
- Preserve existing behavior unless the current phase requires changing it.

## Testing expectations

Each phase must include a clear acceptance test and a short manual test procedure.

## Offline-First Rules

This project must be usable offline.

1. Do not require a cloud account.
2. Do not require an online model provider.
3. Do not add telemetry.
4. Do not send project files, asset names, scene data, source code, prompts, logs, or generated context to external services.
5. Use localhost communication only by default.
6. The Unreal plugin must bind only to `127.0.0.1`.
7. Remote network access must be disabled by default.
8. Cloud model providers may exist only as optional adapters.
9. Online features must be clearly labeled in the UI.
10. Every model provider must declare whether it is:
   - Offline
   - Local network
   - Online
11. The default model provider must be local/offline.
12. The app must still launch if no internet connection exists.
13. The app must still work if no model is configured, using manual command mode.