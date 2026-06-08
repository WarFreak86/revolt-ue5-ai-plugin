# Safety Model Guide

RevoltEditorBridge is offline-first and approval-gated.

## Permission Rules

- Read-only commands inspect project state.
- Mutating commands require dry-run or approval.
- Approval can be provided by request payload or UI queue.
- Generated assets must be under `/Game/RevoltGenerated`.
- Generated actors must be tagged `Revolt.Generated`.

## Blocked by Default

- Telemetry.
- Cloud providers.
- External network calls.
- Public bridge binding.
- Arbitrary shell execution.
- Arbitrary Python execution.
- User asset deletion.

## Offline Guarantees

- Unreal bridge defaults to `127.0.0.1`.
- Desktop runtime stores local SQLite data.
- Local models are optional.
- Online providers, if mentioned, are optional and disabled by default.
