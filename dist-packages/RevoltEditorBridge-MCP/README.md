# Revolt Editor Bridge MCP Server

Offline-first TypeScript MCP server for the local Unreal Editor bridge.

## Offline Model

- No cloud APIs are called.
- No model inference is implemented in Phase 6.
- The server communicates only with the local Unreal bridge by default.
- `REVOLT_UE_BRIDGE_HOST` is restricted to `127.0.0.1` or `localhost`.

## Install

```powershell
cd mcp-server
npm install
npm run build
```

## Configuration

Environment variables:

| Variable | Default | Description |
| --- | --- | --- |
| `REVOLT_UE_BRIDGE_HOST` | `127.0.0.1` | Local Unreal bridge host. Must be `127.0.0.1` or `localhost` in Phase 6. |
| `REVOLT_UE_BRIDGE_PORT` | `8765` | Local Unreal bridge port. |
| `REVOLT_UE_BRIDGE_TIMEOUT_MS` | `5000` | Request timeout in milliseconds. |
| `REVOLT_LOG_LEVEL` | `info` | `debug`, `info`, `warn`, or `error`. |

## Test Unreal Bridge

Start Unreal Editor, open `Tools > Revolt Bridge`, and click `Start Bridge`.

Then run:

```powershell
npm run test:bridge
```

Expected result:

```json
{
  "ok": true
}
```

The response includes the bridge request id and result payload.

## MCP Client Configuration

Build first:

```powershell
cd mcp-server
npm install
npm run build
```

Configure your MCP client to run:

```text
node E:\Ue Plugin\mcp-server\dist\index.js
```

Example MCP client entry:

```json
{
  "mcpServers": {
    "revolt-editor-bridge": {
      "command": "node",
      "args": ["E:\\Ue Plugin\\mcp-server\\dist\\index.js"],
      "env": {
        "REVOLT_UE_BRIDGE_HOST": "127.0.0.1",
        "REVOLT_UE_BRIDGE_PORT": "8765",
        "REVOLT_UE_BRIDGE_TIMEOUT_MS": "5000",
        "REVOLT_LOG_LEVEL": "info"
      }
    }
  }
}
```

## Tools

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

Mutating tools include `dryRun` and `approved`. They default to `dryRun: true` unless `approved: true` is explicitly provided.
