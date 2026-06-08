# Unreal Plugin Setup

## Build

```powershell
.\scripts\build-plugin\Build-HostProject.ps1
```

## Open Plugin

1. Open `unreal/HostProject/HostProject.uproject`.
2. Confirm `Revolt Editor Bridge` appears in the plugin list.
3. Open `Tools > Revolt Bridge`.
4. Click `Start Bridge`.

## Bridge Endpoint

Default endpoint:

```text
http://127.0.0.1:8765/
```

The plugin must not bind to `0.0.0.0` by default.
