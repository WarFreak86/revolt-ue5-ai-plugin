# Installation Guide

This guide prepares RevoltEditorBridge for offline external testing.

## Requirements

- Windows development machine.
- Unreal Engine 5.7, or another compatible UE5 install configured in `config/unreal-engine.json`.
- Visual Studio Build Tools for Unreal C++ compilation.
- Node.js and npm for the desktop runtime and MCP server.

## Offline Guarantee

Default use does not require internet after dependencies and Unreal Engine are installed.

- No telemetry is implemented.
- No cloud provider is configured by default.
- Unreal bridge binds to `127.0.0.1` by default.
- Desktop runtime stores data in a local SQLite database.
- Local model support uses local endpoints such as Ollama, LM Studio, llama.cpp, or local OpenAI-compatible servers.
- Optional online providers, if added later, must be disabled by default and clearly labeled.

## Install Steps

1. Set the Unreal Engine path in `config/unreal-engine.json`.
2. Build the Unreal host project:

   ```powershell
   .\scripts\build-plugin\Build-HostProject.ps1
   ```

3. Build the MCP server:

   ```powershell
   cd mcp-server
   npm install
   npm run build
   ```

4. Build the desktop runtime:

   ```powershell
   cd desktop
   npm install
   npm run build
   ```

5. Open `unreal/HostProject/HostProject.uproject`, enable the plugin if needed, and open `Tools > Revolt Bridge`.
