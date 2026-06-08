# Project: Offline-First UE5 AI Editor Automation Plugin

You are helping build an Unreal Engine 5 plugin and companion desktop/MCP runtime.

## Project Name

RevoltEditorBridge

## Core Goal

Build an offline-first Unreal Engine 5 editor automation plugin that allows AI coding tools and local AI models to inspect, modify, generate, audit, and procedurally build Unreal projects through a safe local bridge.

The system must work without internet after installation.

## Major Components

1. Unreal Engine 5 Editor Plugin
2. Local MCP Server
3. Desktop Runtime
4. Offline Local Model Routing
5. Contextual Project/Scene Introspection
6. Safe Editor Automation
7. Asset and Blueprint Manipulation
8. PCG / Biome Data Asset System
9. Land Generation Actor
10. Editable Biome-Based Actor Spawning
11. Project Auditing and Fix Plans
12. Optional Gameplay Templates
13. Packaging and Documentation

## Offline-First Rule

The system must function without internet.

Any feature that requires the internet must be optional, disabled by default, and clearly labeled as online.

Default behavior:

- No cloud API calls.
- No telemetry.
- No remote project indexing.
- No remote asset upload.
- No online license check required for local use.
- No project source, asset names, scene hierarchy, prompts, logs, or generated context leaves the machine.
- Unreal plugin communicates only over localhost.
- Desktop runtime communicates with local model servers by default.
- Cloud providers are optional later adapters only.

## Local Communication

Allowed by default:

- Unreal Editor Plugin ↔ Desktop Runtime over localhost.
- Desktop Runtime ↔ MCP Server over stdio or localhost.
- Desktop Runtime ↔ Ollama / LM Studio / llama.cpp / local OpenAI-compatible endpoint.
- AI IDE ↔ MCP Server.

Blocked by default:

- Public network binding.
- External domains.
- Cloud model calls.
- Telemetry.
- Shell execution.
- Arbitrary file deletion.
- Automatic source-code rewrites without approval.

## Recommended Repository Structure

Create or maintain this structure:

revolt-ue5-ai/
  unreal/
    HostProject/
      HostProject.uproject
      Plugins/
        RevoltEditorBridge/
          RevoltEditorBridge.uplugin
          Source/
            RevoltEditorBridge/
            RevoltEditorBridgeEditor/
          Content/
          Resources/

  mcp-server/
    package.json
    src/

  desktop/
    package.json
    src/

  docs/
    architecture.md
    safety-model.md
    offline-model.md
    mcp-tools.md
    biome-format.md
    codex-prompts.md
    phase-status.md
    troubleshooting.md

  scripts/
    test-bridge/
    build-plugin/

## Required Documents

Create these documentation files if missing:

1. AGENTS.md
2. docs/architecture.md
3. docs/safety-model.md
4. docs/offline-model.md
5. docs/mcp-tools.md
6. docs/biome-format.md
7. docs/phase-status.md
8. docs/codex-prompts.md

## AGENTS.md Rules

Add these rules:

- Implement one phase at a time.
- Do not skip phases.
- Do not rewrite working systems unless required.
- Do not add unrelated features.
- Do not add telemetry.
- Do not add cloud providers unless a later phase explicitly requests it.
- Do not send any project data to external services.
- Keep Unreal editor-only code inside editor modules.
- Keep Unreal plugin communication localhost-only by default.
- Mutating editor actions must support dry-run, approval, logging, and undo where possible.
- Generated assets must go under /Game/RevoltGenerated.
- Generated actors must be tagged Revolt.Generated.
- Do not delete user assets.
- Do not add arbitrary shell execution.
- Do not add arbitrary Python execution unless a later approved phase explicitly requests it.
- Update docs/phase-status.md after each phase.

## Phase List

Add this to docs/phase-status.md:

Phase 0 - Project documentation and repo setup
Phase 1 - UE5 plugin skeleton
Phase 2 - Read-only localhost Unreal bridge
Phase 3 - Safe editor mutations
Phase 4 - Asset manipulation tools
Phase 5 - Blueprint creation tools
Phase 6 - TypeScript MCP server
Phase 7 - Offline desktop runtime shell
Phase 8 - Offline local model routing
Phase 9 - Local project indexing
Phase 10 - Biome Data Asset system
Phase 11 - Land Generation Actor
Phase 12 - Biome-based editable actor spawning
Phase 13 - Project auditing and fix plans
Phase 14 - Manual command mode
Phase 15 - Arena shooter starter template
Phase 16 - Advanced zombie shooter template
Phase 17 - Packaging, docs, and offline installer preparation

## Instructions

Only create/update the planning and documentation files in this task.

Do not implement plugin code yet.

## Acceptance Tests

1. AGENTS.md exists and contains offline-first and safety rules.
2. docs/architecture.md explains the full system architecture.
3. docs/safety-model.md explains permissions, approval, dry-run, local-only communication, and cloud opt-in.
4. docs/offline-model.md explains Ollama, LM Studio, llama.cpp, and local OpenAI-compatible endpoints.
5. docs/mcp-tools.md lists planned MCP tools.
6. docs/biome-format.md defines the planned biome data structure.
7. docs/phase-status.md lists all phases.
8. No production code is created in this phase.