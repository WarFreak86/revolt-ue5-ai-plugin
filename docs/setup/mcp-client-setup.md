# MCP Client Setup

The MCP server exposes Unreal bridge tools to MCP-capable local clients.

## Build

```powershell
cd mcp-server
npm install
npm run build
```

## Client Command

```text
node E:\Ue Plugin\mcp-server\dist\index.js
```

## Example MCP Config

```json
{
  "mcpServers": {
    "revolt-editor-bridge": {
      "command": "node",
      "args": ["E:\\Ue Plugin\\mcp-server\\dist\\index.js"],
      "env": {
        "REVOLT_UE_BRIDGE_HOST": "127.0.0.1",
        "REVOLT_UE_BRIDGE_PORT": "8765"
      }
    }
  }
}
```

The server talks only to the local Unreal bridge by default.
