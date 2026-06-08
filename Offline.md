## Offline-First Requirement

This plugin must work without internet access after installation.

Default behavior:
- No cloud API calls.
- No external telemetry.
- No remote asset uploads.
- No online license check required for local use.
- No cloud model required.
- No project source, asset names, scene hierarchy, or generated context leaves the local machine.

Allowed local-only communication:
- Unreal Editor Plugin ↔ Desktop Runtime over localhost.
- Desktop Runtime ↔ local model server such as Ollama, LM Studio, or llama.cpp.
- MCP server ↔ local AI IDE/client over stdio or localhost.

Optional online features must be disabled by default and clearly labeled.