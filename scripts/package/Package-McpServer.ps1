param(
	[string]$OutputRoot,
	[switch]$SkipBuild,
	[switch]$IncludeNodeModules
)

$ErrorActionPreference = "Stop"

$scriptPath = Split-Path -Parent $PSCommandPath
$repoRoot = (Resolve-Path (Join-Path $scriptPath "..\..")).Path
$mcpRoot = Join-Path $repoRoot "mcp-server"

if ([string]::IsNullOrWhiteSpace($OutputRoot)) {
	$OutputRoot = Join-Path $repoRoot "dist-packages"
}

if (-not $SkipBuild) {
	Push-Location $mcpRoot
	try {
		npm run build
		if ($LASTEXITCODE -ne 0) {
			throw "MCP server build failed with exit code $LASTEXITCODE."
		}
	}
	finally {
		Pop-Location
	}
}

$stagingRoot = Join-Path $OutputRoot "RevoltEditorBridge-MCP"
$zipPath = Join-Path $OutputRoot "RevoltEditorBridge-MCP.zip"
Remove-Item -LiteralPath $stagingRoot -Recurse -Force -ErrorAction SilentlyContinue
Remove-Item -LiteralPath $zipPath -Force -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force -Path $stagingRoot | Out-Null

Copy-Item -LiteralPath (Join-Path $mcpRoot "dist") -Destination (Join-Path $stagingRoot "dist") -Recurse
Copy-Item -LiteralPath (Join-Path $mcpRoot "src") -Destination (Join-Path $stagingRoot "src") -Recurse
Copy-Item -LiteralPath (Join-Path $mcpRoot "package.json") -Destination $stagingRoot
Copy-Item -LiteralPath (Join-Path $mcpRoot "tsconfig.json") -Destination $stagingRoot
Copy-Item -LiteralPath (Join-Path $mcpRoot "README.md") -Destination $stagingRoot
if (Test-Path -LiteralPath (Join-Path $mcpRoot "package-lock.json")) {
	Copy-Item -LiteralPath (Join-Path $mcpRoot "package-lock.json") -Destination $stagingRoot
}
if ($IncludeNodeModules -and (Test-Path -LiteralPath (Join-Path $mcpRoot "node_modules"))) {
	Copy-Item -LiteralPath (Join-Path $mcpRoot "node_modules") -Destination (Join-Path $stagingRoot "node_modules") -Recurse
}

Compress-Archive -Path (Join-Path $stagingRoot "*") -DestinationPath $zipPath -Force
Write-Host "Packaged MCP server: $zipPath"
