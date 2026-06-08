param(
	[string]$OutputRoot,
	[switch]$SkipBuild,
	[switch]$IncludeNodeModules
)

$ErrorActionPreference = "Stop"

$scriptPath = Split-Path -Parent $PSCommandPath
$repoRoot = (Resolve-Path (Join-Path $scriptPath "..\..")).Path
$desktopRoot = Join-Path $repoRoot "desktop"

if ([string]::IsNullOrWhiteSpace($OutputRoot)) {
	$OutputRoot = Join-Path $repoRoot "dist-packages"
}

if (-not $SkipBuild) {
	Push-Location $desktopRoot
	try {
		npm run build
		if ($LASTEXITCODE -ne 0) {
			throw "Desktop build failed with exit code $LASTEXITCODE."
		}
	}
	finally {
		Pop-Location
	}
}

$stagingRoot = Join-Path $OutputRoot "RevoltDesktopRuntime"
$zipPath = Join-Path $OutputRoot "RevoltDesktopRuntime.zip"
Remove-Item -LiteralPath $stagingRoot -Recurse -Force -ErrorAction SilentlyContinue
Remove-Item -LiteralPath $zipPath -Force -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force -Path $stagingRoot | Out-Null

Copy-Item -LiteralPath (Join-Path $desktopRoot "dist") -Destination (Join-Path $stagingRoot "dist") -Recurse
Copy-Item -LiteralPath (Join-Path $desktopRoot "package.json") -Destination $stagingRoot
Copy-Item -LiteralPath (Join-Path $desktopRoot "README.md") -Destination $stagingRoot
if (Test-Path -LiteralPath (Join-Path $desktopRoot "package-lock.json")) {
	Copy-Item -LiteralPath (Join-Path $desktopRoot "package-lock.json") -Destination $stagingRoot
}
if (Test-Path -LiteralPath (Join-Path $desktopRoot "config")) {
	Copy-Item -LiteralPath (Join-Path $desktopRoot "config") -Destination (Join-Path $stagingRoot "config") -Recurse
}
if ($IncludeNodeModules -and (Test-Path -LiteralPath (Join-Path $desktopRoot "node_modules"))) {
	Copy-Item -LiteralPath (Join-Path $desktopRoot "node_modules") -Destination (Join-Path $stagingRoot "node_modules") -Recurse
}

Compress-Archive -Path (Join-Path $stagingRoot "*") -DestinationPath $zipPath -Force
Write-Host "Packaged desktop runtime: $zipPath"
