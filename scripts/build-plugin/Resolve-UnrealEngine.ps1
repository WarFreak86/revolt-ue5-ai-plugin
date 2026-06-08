param(
	[string]$EngineRoot
)

$ErrorActionPreference = "Stop"

function Get-RepoRoot {
	$scriptPath = Split-Path -Parent $PSCommandPath
	return (Resolve-Path (Join-Path $scriptPath "..\..")).Path
}

function Test-UnrealEngineRoot {
	param([Parameter(Mandatory = $true)][string]$Path)

	if ([string]::IsNullOrWhiteSpace($Path)) {
		return $false
	}

	$resolvedPath = $Path.Trim().Trim('"')
	$buildBat = Join-Path $resolvedPath "Engine\Build\BatchFiles\Build.bat"
	$versionFile = Join-Path $resolvedPath "Engine\Build\Build.version"
	return (Test-Path -LiteralPath $buildBat) -and (Test-Path -LiteralPath $versionFile)
}

function Get-ConfiguredEngineRoot {
	param([Parameter(Mandatory = $true)][string]$RepoRoot)

	$configPath = Join-Path $RepoRoot "config\unreal-engine.json"
	if (-not (Test-Path -LiteralPath $configPath)) {
		return $null
	}

	$config = Get-Content -Raw -LiteralPath $configPath | ConvertFrom-Json
	return $config.EngineRoot
}

$repoRoot = Get-RepoRoot
$candidateRoots = New-Object System.Collections.Generic.List[string]

if (-not [string]::IsNullOrWhiteSpace($EngineRoot)) {
	$candidateRoots.Add($EngineRoot)
}

if (-not [string]::IsNullOrWhiteSpace($env:UE_ENGINE_DIR)) {
	$candidateRoots.Add($env:UE_ENGINE_DIR)
}

$configuredRoot = Get-ConfiguredEngineRoot -RepoRoot $repoRoot
if (-not [string]::IsNullOrWhiteSpace($configuredRoot)) {
	$candidateRoots.Add($configuredRoot)
}

$candidateRoots.Add("E:\UE5\UE_5.7")
$candidateRoots.Add("C:\Program Files\Epic Games\UE_5.7")
$candidateRoots.Add("C:\Program Files\Epic Games\UE_5.6")
$candidateRoots.Add("C:\Program Files\Epic Games\UE_5.5")
$candidateRoots.Add("C:\Program Files\Epic Games\UE_5.4")
$candidateRoots.Add("C:\Program Files\Epic Games\UE_5.3")
$candidateRoots.Add("C:\Program Files\Epic Games\UE_5.2")
$candidateRoots.Add("C:\Program Files\Epic Games\UE_5.1")
$candidateRoots.Add("C:\Program Files\Epic Games\UE_5.0")

$uniqueCandidates = $candidateRoots | Where-Object { -not [string]::IsNullOrWhiteSpace($_) } | Select-Object -Unique
foreach ($candidateRoot in $uniqueCandidates) {
	if (Test-UnrealEngineRoot -Path $candidateRoot) {
		$resolvedRoot = (Resolve-Path -LiteralPath $candidateRoot).Path
		[PSCustomObject]@{
			RepoRoot = $repoRoot
			EngineRoot = $resolvedRoot
			BuildBat = Join-Path $resolvedRoot "Engine\Build\BatchFiles\Build.bat"
			GenerateProjectFilesBat = Join-Path $resolvedRoot "Engine\Build\BatchFiles\GenerateProjectFiles.bat"
		}
		return
	}
}

$candidateText = ($uniqueCandidates | ForEach-Object { "  - $_" }) -join [Environment]::NewLine
throw "Unable to find a valid Unreal Engine install. Checked:$([Environment]::NewLine)$candidateText$([Environment]::NewLine)Set UE_ENGINE_DIR or edit config\unreal-engine.json."
