param(
	[string]$EngineRoot,
	[switch]$GenerateProjectFiles,
	[switch]$ResolveOnly,
	[switch]$SkipWritableCacheCheck
)

$ErrorActionPreference = "Stop"

$scriptPath = Split-Path -Parent $PSCommandPath
$resolverPath = Join-Path $scriptPath "Resolve-UnrealEngine.ps1"
$engine = & $resolverPath -EngineRoot $EngineRoot
$projectPath = Join-Path $engine.RepoRoot "unreal\HostProject\HostProject.uproject"

Write-Host "Using Unreal Engine: $($engine.EngineRoot)"
Write-Host "Using project: $projectPath"

if ($ResolveOnly) {
	return
}

if (-not (Test-Path -LiteralPath $projectPath)) {
	throw "Project file not found: $projectPath"
}

if (-not $SkipWritableCacheCheck) {
	$unrealWritableCache = Join-Path $env:LOCALAPPDATA "UnrealEngine\Intermediate\Build"
	$unrealLocalConfig = Join-Path $env:LOCALAPPDATA "Unreal Engine"
	$unrealRoamingConfig = Join-Path $env:APPDATA "Unreal Engine\UnrealBuildTool"
	$unrealBuildToolLog = Join-Path $env:LOCALAPPDATA "UnrealBuildTool"
	try {
		New-Item -ItemType Directory -Force -Path $unrealWritableCache | Out-Null
		$probePath = Join-Path $unrealWritableCache "RevoltEditorBridge.WriteProbe.tmp"
		Set-Content -LiteralPath $probePath -Value "write-check"
		Remove-Item -LiteralPath $probePath -Force
		Write-Host "Verified UnrealBuildTool writable cache: $unrealWritableCache"

		New-Item -ItemType Directory -Force -Path $unrealLocalConfig | Out-Null
		$localConfigProbePath = Join-Path $unrealLocalConfig "RevoltEditorBridge.WriteProbe.tmp"
		Set-Content -LiteralPath $localConfigProbePath -Value "write-check"
		Remove-Item -LiteralPath $localConfigProbePath -Force
		Write-Host "Verified Unreal local config path: $unrealLocalConfig"

		New-Item -ItemType Directory -Force -Path $unrealRoamingConfig | Out-Null
		$roamingProbePath = Join-Path $unrealRoamingConfig "RevoltEditorBridge.WriteProbe.tmp"
		Set-Content -LiteralPath $roamingProbePath -Value "write-check"
		Remove-Item -LiteralPath $roamingProbePath -Force
		Write-Host "Verified UnrealBuildTool roaming config path: $unrealRoamingConfig"

		New-Item -ItemType Directory -Force -Path $unrealBuildToolLog | Out-Null
		$logProbePath = Join-Path $unrealBuildToolLog "RevoltEditorBridge.WriteProbe.tmp"
		Set-Content -LiteralPath $logProbePath -Value "write-check"
		Remove-Item -LiteralPath $logProbePath -Force
		Write-Host "Verified UnrealBuildTool log path: $unrealBuildToolLog"
	}
	catch {
		throw "UnrealBuildTool needs write access to '$unrealWritableCache', '$unrealLocalConfig', '$unrealRoamingConfig', and '$unrealBuildToolLog'. Grant write permission to those folders or run this script outside a restricted sandbox. Use -SkipWritableCacheCheck only if you know UBT can write there."
	}
}

if ($GenerateProjectFiles) {
	if (-not (Test-Path -LiteralPath $engine.GenerateProjectFilesBat)) {
		throw "GenerateProjectFiles.bat not found: $($engine.GenerateProjectFilesBat)"
	}
	& $engine.GenerateProjectFilesBat -project="$projectPath" -game -engine
	if ($LASTEXITCODE -ne 0) {
		throw "GenerateProjectFiles.bat failed with exit code $LASTEXITCODE."
	}
}

& $engine.BuildBat HostProjectEditor Win64 Development -Project="$projectPath" -WaitMutex -NoHotReload -NoUBA
if ($LASTEXITCODE -ne 0) {
	throw "Unreal build failed with exit code $LASTEXITCODE."
}
