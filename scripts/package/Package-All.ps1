param(
	[string]$EngineRoot,
	[string]$OutputRoot
)

$ErrorActionPreference = "Stop"

$scriptPath = Split-Path -Parent $PSCommandPath
$repoRoot = (Resolve-Path (Join-Path $scriptPath "..\..")).Path
if ([string]::IsNullOrWhiteSpace($OutputRoot)) {
	$OutputRoot = Join-Path $repoRoot "dist-packages"
}

& (Join-Path $scriptPath "Package-Plugin.ps1") -EngineRoot $EngineRoot -OutputRoot $OutputRoot
& (Join-Path $scriptPath "Package-Desktop.ps1") -OutputRoot $OutputRoot
& (Join-Path $scriptPath "Package-McpServer.ps1") -OutputRoot $OutputRoot

Write-Host "All packages written to: $OutputRoot"
