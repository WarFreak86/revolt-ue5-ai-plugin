# LM Studio Setup

LM Studio is optional. Manual Command Mode works without it.

1. Install LM Studio locally.
2. Load a local model.
3. Start the local server.
4. In the desktop runtime, create a model profile:
   - Provider type: `LM Studio`
   - Base URL: `http://127.0.0.1:1234`
   - Classification: `offline` or `local-network`
5. Click `Test Connection`.

Do not use a remote endpoint for default offline testing.
