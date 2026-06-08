param(
	[string]$EngineRoot,
	[string]$OutputRoot
)

$ErrorActionPreference = "Stop"

$scriptPath = Split-Path -Parent $PSCommandPath
$repoRoot = (Resolve-Path (Join-Path $scriptPath "..\..")).Path
$resolverPath = Join-Path $repoRoot "scripts\build-plugin\Resolve-UnrealEngine.ps1"
$engine = & $resolverPath -EngineRoot $EngineRoot

if ([string]::IsNullOrWhiteSpace($OutputRoot)) {
	$OutputRoot = Join-Path $repoRoot "dist-packages"
}

$pluginPath = Join-Path $repoRoot "unreal\HostProject\Plugins\RevoltEditorBridge\RevoltEditorBridge.uplugin"
$packageRoot = Join-Path $OutputRoot "RevoltEditorBridge-UnrealPlugin"
$zipPath = Join-Path $OutputRoot "RevoltEditorBridge-UnrealPlugin.zip"
$runUat = Join-Path $engine.EngineRoot "Engine\Build\BatchFiles\RunUAT.bat"
$ubtConfigDir = Join-Path $env:APPDATA "Unreal Engine\UnrealBuildTool"
$ubtConfigPath = Join-Path $ubtConfigDir "BuildConfiguration.xml"
$ubtConfigBackupPath = "$ubtConfigPath.RevoltBackup"

if (-not (Test-Path -LiteralPath $pluginPath)) {
	throw "Plugin descriptor not found: $pluginPath"
}
if (-not (Test-Path -LiteralPath $runUat)) {
	throw "RunUAT.bat not found: $runUat"
}

New-Item -ItemType Directory -Force -Path $OutputRoot | Out-Null
Remove-Item -LiteralPath $packageRoot -Recurse -Force -ErrorAction SilentlyContinue
Remove-Item -LiteralPath $zipPath -Force -ErrorAction SilentlyContinue

New-Item -ItemType Directory -Force -Path $ubtConfigDir | Out-Null
$hadExistingUbtConfig = Test-Path -LiteralPath $ubtConfigPath
if ($hadExistingUbtConfig) {
	Copy-Item -LiteralPath $ubtConfigPath -Destination $ubtConfigBackupPath -Force
}

try {
	@"
<?xml version="1.0" encoding="utf-8"?>
<Configuration xmlns="https://www.unrealengine.com/BuildConfiguration">
  <BuildConfiguration>
    <bAllowUBAExecutor>false</bAllowUBAExecutor>
  </BuildConfiguration>
</Configuration>
"@ | Set-Content -LiteralPath $ubtConfigPath -Encoding UTF8

	& $runUat BuildPlugin -Plugin="$pluginPath" -Package="$packageRoot" -TargetPlatforms=Win64 -Rocket
	if ($LASTEXITCODE -ne 0) {
		throw "Unreal plugin packaging failed with exit code $LASTEXITCODE."
	}
}
finally {
	if ($hadExistingUbtConfig) {
		Move-Item -LiteralPath $ubtConfigBackupPath -Destination $ubtConfigPath -Force
	}
	else {
		Remove-Item -LiteralPath $ubtConfigPath -Force -ErrorAction SilentlyContinue
	}
}

Compress-Archive -Path (Join-Path $packageRoot "*") -DestinationPath $zipPath -Force
Write-Host "Packaged Unreal plugin: $zipPath"
