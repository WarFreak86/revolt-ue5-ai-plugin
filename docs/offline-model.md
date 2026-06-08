# Offline Model Setup

Revolt Desktop Runtime supports local/offline model endpoints only by default.

## Local Model Setup Wizard

Open `Local Model Setup` in the desktop runtime.

The wizard walks through:

1. Choose provider.
2. Detect local endpoint.
3. Confirm base URL.
4. Fetch or enter model name.
5. Test connection.
6. Run the safe local test prompt.
7. Save the model profile.
8. Set it as active.

The safe test prompt is:

```text
Reply with the word READY and a one-sentence description of your coding capability.
```

No Unreal project data is included in this test.

## Supported Providers

| Provider | Default URL | Classification |
| --- | --- | --- |
| Ollama | `http://127.0.0.1:11434` | Offline |
| LM Studio | `http://127.0.0.1:1234` | Offline or Local Network |
| llama.cpp server | `http://127.0.0.1:8080` | Offline |
| Local OpenAI-compatible endpoint | User-entered local URL | Offline or Local Network |

## URL Safety

Offline/local providers must use:

- `localhost`
- `127.0.0.1`
- `::1`
- `.local` hosts
- Private LAN addresses such as `10.x.x.x`, `172.16.x.x` through `172.31.x.x`, or `192.168.x.x`

External URLs are blocked unless `Enable Online Providers` is explicitly enabled. No cloud providers are added in this phase.

## No Model Fallback

If no local model is configured, the app still works in Manual Command Mode.
