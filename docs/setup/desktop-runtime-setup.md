# Desktop Runtime Setup

The desktop runtime is an offline-first Electron app.

## Build

```powershell
cd desktop
npm install
npm run build
```

## Run

```powershell
npm run dev
```

## Defaults

- No login.
- No telemetry.
- No cloud provider configured by default.
- `Enable Online Providers` defaults to `false`.
- If no model profile exists, the app runs in Manual Command Mode.
