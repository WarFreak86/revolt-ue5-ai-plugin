# Ollama Setup

Ollama is optional. Manual Command Mode works without it.

1. Install Ollama locally.
2. Start the Ollama service.
3. Pull or create a local model while internet is available, or use a model already installed.
4. In the desktop runtime, create a model profile:
   - Provider type: `Ollama`
   - Base URL: `http://127.0.0.1:11434`
   - Classification: `offline`
5. Click `Test Connection`.

Project data is sent only to this local endpoint when the profile is active.
