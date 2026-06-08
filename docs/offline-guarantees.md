# Offline Guarantees

Default RevoltEditorBridge use is offline-first.

- No telemetry is implemented.
- No cloud provider is configured by default.
- Unreal bridge binds to localhost / `127.0.0.1` by default.
- Desktop runtime uses a local SQLite database.
- MCP server communicates with the local Unreal bridge by default.
- Local model support is available through Ollama, LM Studio, llama.cpp, or local OpenAI-compatible endpoints.
- Manual Command Mode works with no model installed.
- Optional online providers, if added later, must be clearly labeled and disabled by default.
