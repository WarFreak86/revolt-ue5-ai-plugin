# Building RevoltEditorBridge

## Unreal Engine Path

The build scripts do not assume Unreal Engine is installed in the default Epic Games folder.

The current configured engine path is:

```text
E:\UE5\UE_5.7
```

To change it, edit:

```text
config/unreal-engine.json
```

Set `EngineRoot` to the root folder that contains:

```text
Engine\Build\BatchFiles\Build.bat
Engine\Build\Build.version
```

You can also override the configured path without editing files:

```powershell
$env:UE_ENGINE_DIR = "E:\UE5\UE_5.7"
```

Or pass it directly:

```powershell
.\scripts\build-plugin\Build-HostProject.ps1 -EngineRoot "E:\UE5\UE_5.7"
```

## Detection Order

The resolver checks Unreal Engine paths in this order:

1. `-EngineRoot` argument
2. `UE_ENGINE_DIR` environment variable
3. `config/unreal-engine.json`
4. Known custom path `E:\UE5\UE_5.7`
5. Default Epic Games folders such as `C:\Program Files\Epic Games\UE_5.7`

## Build Commands

Validate which engine path will be used:

```powershell
.\scripts\build-plugin\Build-HostProject.ps1 -ResolveOnly
```

Generate project files, then build:

```powershell
.\scripts\build-plugin\Build-HostProject.ps1 -GenerateProjectFiles
```

Build only:

```powershell
.\scripts\build-plugin\Build-HostProject.ps1
```

Installed Unreal Engine builds write intermediate build configuration to the user settings cache:

```text
%LOCALAPPDATA%\UnrealEngine\Intermediate\Build
```

UnrealBuildTool also reads user build configuration from:

```text
%APPDATA%\Unreal Engine\UnrealBuildTool
```

UnrealBuildTool writes build logs to:

```text
%LOCALAPPDATA%\UnrealBuildTool
```

UnrealBuildTool may also read local user config from:

```text
%LOCALAPPDATA%\Unreal Engine
```

The build helper checks this path before invoking UnrealBuildTool so permission issues fail with a clear message.

If you are running inside a restricted sandbox, grant write access to:

```text
C:\Users\<you>\AppData\Local\UnrealEngine
C:\Users\<you>\AppData\Local\Unreal Engine
C:\Users\<you>\AppData\Local\UnrealBuildTool
C:\Users\<you>\AppData\Roaming\Unreal Engine
```

If you already know UnrealBuildTool can write there and only want to skip the preflight check, pass `-SkipWritableCacheCheck`.

## Safety Notes

- These scripts only call Unreal's own project generation and build tools.
- The build command passes `-NoUBA` to avoid Unreal Build Accelerator writing to `C:\ProgramData`.
- They do not run arbitrary shell commands.
- They do not call external network services.
- They do not modify project assets.
