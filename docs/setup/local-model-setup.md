# Local Model Setup

Model use is optional. Manual Command Mode works with no model installed.

## Desktop Wizard

Open `Local Model Setup` in the desktop runtime.

The wizard supports:

- Ollama
- LM Studio
- llama.cpp server
- Local OpenAI-compatible endpoint

It can detect local endpoints, fetch model lists where available, run a safe test prompt, save a profile to SQLite, and set the profile active.

The safe test prompt is local-only and does not include Unreal project data.

## Ollama

1. Install Ollama locally.
2. Start Ollama.
3. In the desktop app, create a model profile:
   - Provider: `Ollama`
   - Base URL: `http://127.0.0.1:11434`
   - Classification: `offline`

## LM Studio

1. Start LM Studio's local server.
2. Create a model profile:
   - Provider: `LM Studio`
   - Base URL: `http://127.0.0.1:1234`
   - Classification: `offline` or `local-network`

## llama.cpp Server

1. Start your local llama.cpp HTTP server.
2. Create a model profile:
   - Provider: `llama.cpp server`
   - Base URL: `http://127.0.0.1:8080`
   - Classification: `offline`

## Local OpenAI-Compatible Endpoint

Use only local endpoints by default, such as `http://127.0.0.1:8000`.

Online providers are not required and must remain disabled by default.

External URLs are blocked unless `Enable Online Providers` is explicitly enabled.
