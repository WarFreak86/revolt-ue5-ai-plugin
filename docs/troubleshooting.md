# Troubleshooting Guide

## Unreal Engine Not Found

Edit `config/unreal-engine.json` or set:

```powershell
$env:UE_ENGINE_DIR = "E:\UE5\UE_5.7"
```

## Bridge Offline

1. Open Unreal Editor.
2. Open `Tools > Revolt Bridge`.
3. Click `Start Bridge`.
4. Confirm endpoint is `127.0.0.1:8765`.

## Desktop App Cannot Connect

Check Settings:

- UE bridge host: `127.0.0.1`
- UE bridge port: `8765`

The desktop UI should still render when Unreal is closed or the bridge is stopped. If `Test Connection` reports offline, open Unreal, choose `Tools > Revolt Bridge`, click `Start Bridge`, then use `Connect to Unreal`.

## Connect to Unreal Wizard

Open the desktop runtime and choose `Connect to Unreal`.

The wizard uses the local default endpoint:

```text
http://127.0.0.1:8765
```

If connection testing fails, the app should show one of these controlled messages instead of crashing:

- `unreachable`: Unreal is closed or the bridge is not started.
- `invalid_response`: Something responded, but it was not the Revolt bridge.
- `timeout`: The local bridge did not respond quickly enough.
- `invalid_endpoint`: The endpoint is not a valid localhost URL.
- `unknown_error`: The app caught an unexpected local connection problem.

## Desktop Button Shows a Friendly Error

Buttons should not crash the renderer. If a button reports a controlled warning:

- Confirm Unreal is running only for Unreal commands.
- Confirm a local model server is running only for model tests.
- Confirm MCP is built only for MCP setup.
- Use Manual Command Mode when no model is configured.

If the same warning repeats, check the visible message and the terminal log; the app is expected to continue running.

## Cannot Read Properties of Undefined

This should be guarded by the desktop runtime's safe default state and renderer error boundary.

If it appears again:

1. Run `cd desktop; npm run build`.
2. Launch with `npm run dev`.
3. Note which page and button caused the message.
4. Keep Unreal, MCP, and model servers optional; missing services should show friendly offline messages, not blank screens.

## Local Model Endpoint Rejected

Local/offline model URLs must be localhost or private LAN addresses by default:

- `http://127.0.0.1:11434`
- `http://127.0.0.1:1234`
- `http://127.0.0.1:8080`
- `http://localhost:<port>`
- Private LAN addresses such as `192.168.x.x`

External URLs are blocked unless `Enable Online Providers` is explicitly enabled.

## MCP Client Cannot Start

Run:

```powershell
cd mcp-server
npm run build
```

Then configure your MCP client to run `node dist/index.js`.

## Packaging Fails

Run individual package scripts to isolate the issue:

```powershell
.\scripts\package\Package-Plugin.ps1
.\scripts\package\Package-Desktop.ps1
.\scripts\package\Package-McpServer.ps1
```

## Internet Disconnected

Default use should still work if dependencies are already installed locally. Manual Command Mode does not require a model or internet.

## Local Model Setup Wizard

Open the desktop runtime and choose `Local Model Setup`.

If no local model is configured, the app still works in Manual Command Mode.

## Ollama Not Detected

- Confirm Ollama is running locally.
- Use `http://127.0.0.1:11434`.
- If the model list is empty, enter the model name manually.
- The wizard uses only the local endpoint and does not send Unreal project data.

## LM Studio Not Detected

- Start LM Studio's local server.
- Use `http://127.0.0.1:1234` unless you changed the port.
- Load a model in LM Studio before testing.
- Keep classification `Offline` or `Local Network`.

## llama.cpp Server Not Detected

- Start the llama.cpp HTTP server.
- Use `http://127.0.0.1:8080` unless your server uses a different local port.
- If `/v1/models` is unavailable, enter the model name manually.
- External URLs are blocked unless online providers are explicitly enabled.
