# Packaging Guide

Packaging writes local artifacts under `dist-packages`.

## Unreal Plugin

```powershell
.\scripts\package\Package-Plugin.ps1
```

Creates `RevoltEditorBridge-UnrealPlugin.zip`.

## Desktop Runtime

```powershell
.\scripts\package\Package-Desktop.ps1
```

Creates `RevoltDesktopRuntime.zip` after `npm run build`.

## MCP Server

```powershell
.\scripts\package\Package-McpServer.ps1
```

Creates `RevoltEditorBridge-MCP.zip` after `npm run build`.

## All Packages

```powershell
.\scripts\package\Package-All.ps1
```

Packaging does not add telemetry, cloud providers, or external service calls.
