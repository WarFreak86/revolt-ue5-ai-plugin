Phase 0  - Plugin skeleton
Phase 1  - Read-only localhost editor bridge
Phase 2  - Safe editor mutations
Phase 3  - Asset manipulation tools
Phase 4  - Blueprint creation tools
Phase 5  - Local MCP server
Phase 6  - Offline desktop runtime
Phase 7  - Local model routing
Phase 8  - Local project indexing
Phase 9  - Biome Data Assets
Phase 10 - Land generation actor
Phase 11 - Biome spawning
Phase 12 - Project auditing
Phase 13 - Offline installer/package
Phase 14 - Gameplay template starter kit

# Offline and Privacy Model

## Default Mode

The default mode is Offline Local Mode.

In this mode:

- Unreal communicates only with the local desktop runtime.
- The desktop runtime communicates only with local model servers.
- Project files are indexed locally.
- Context summaries are stored locally.
- Command history is stored locally in SQLite.
- No telemetry is collected.
- No cloud APIs are called.
- No project data leaves the machine.

## Network Policy

Default allowed addresses:

```text
127.0.0.1
localhost