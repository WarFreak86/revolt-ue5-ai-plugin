# llama.cpp Setup

llama.cpp server is optional. Manual Command Mode works without it.

1. Build or install llama.cpp locally.
2. Start a local HTTP server, commonly on port `8080`.
3. In the desktop runtime, create a model profile:
   - Provider type: `llama.cpp server`
   - Base URL: `http://127.0.0.1:8080`
   - Classification: `offline`
4. Click `Test Connection`.

The desktop app validates local hosts by default.
