#!/usr/bin/env pwsh

<#
.SYNOPSIS
  Supernaut Python API Server Launcher
  
  Phase 7.7 Task 3 Alternative: Use proven Python wrapper instead of C++ native
  
  This launcher:
  1. Checks Python environment
  2. Validates S7 model files
  3. Starts Flask HTTP API server (port 5775)
  4. Provides graceful shutdown
#>

param(
  [int]$Port = 5775,
  [switch]$Verbose,
  [switch]$Help
)

if ($Help) {
  Write-Host "
Supernaut Python API Server Launcher

Usage:
  .\launch-supernaut-python.ps1 [options]

Options:
  -Port <n>      HTTP server port (default: 5775)
  -Verbose       Show detailed output
  -Help          Show this help message

Details:
  This launcher uses the proven Python wrapper (supernaut_api.py) instead of
  the C++ native build, avoiding MSVC math library compatibility issues.
  
  Python wrapper includes:
  - Full S7 model inference
  - HTTP REST API (5 endpoints)
  - Deterministic inference (fixed timestamp)
  - Manifest prefetch integration
  
  Endpoints:
  - GET  /health          - Service health
  - GET  /model-info      - Model metadata  
  - POST /tokenize        - Text to tokens
  - POST /forward         - Tokens to logits
  - POST /generate        - Autoregressive generation
"
  exit 0
}

$ErrorActionPreference = "Stop"

Write-Host ""
Write-Host "╔════════════════════════════════════════════════════════════════╗" -ForegroundColor Cyan
Write-Host "║    Supernaut Python API Server (Phase 7.7 Alternative)         ║" -ForegroundColor Cyan
Write-Host "║    Manifest Prefetch Ready - Port $Port                              ║" -ForegroundColor Cyan
Write-Host "╚════════════════════════════════════════════════════════════════╝" -ForegroundColor Cyan
Write-Host ""

# Check Python installation
Write-Host "Checking Python environment..." -ForegroundColor Yellow
try {
  $python = (python --version 2>&1)
  Write-Host "  ✅ $python" -ForegroundColor Green
} catch {
  Write-Host "  ❌ Python not found in PATH" -ForegroundColor Red
  exit 1
}

# Check required Python packages
Write-Host ""
Write-Host "Checking Python dependencies..." -ForegroundColor Yellow

$required_packages = @("flask", "torch")
foreach ($pkg in $required_packages) {
  try {
    python -c "import $pkg" 2>&1 | Out-Null
    Write-Host "  ✅ $pkg installed" -ForegroundColor Green
  } catch {
    Write-Host "  ⚠️  $pkg not installed - installing..." -ForegroundColor Yellow
    python -m pip install $pkg -q
  }
}

# Verify S7 model files
Write-Host ""
Write-Host "Verifying model files..." -ForegroundColor Yellow

$modelRoot = if (-not [string]::IsNullOrWhiteSpace($env:SUPERNAUT_MODEL_ROOT)) {
  $env:SUPERNAUT_MODEL_ROOT
} else {
  "E:\models\s7-llm-mini"
}

function Resolve-ModelFile {
  param(
    [Parameter(Mandatory = $true)][string]$EnvName,
    [Parameter(Mandatory = $true)][string[]]$Candidates
  )
  $envValue = (Get-Item -Path "Env:$EnvName" -ErrorAction SilentlyContinue).Value
  if (-not [string]::IsNullOrWhiteSpace($envValue)) {
    return $envValue
  }
  foreach ($candidate in $Candidates) {
    if (Test-Path -LiteralPath $candidate) {
      return $candidate
    }
  }
  return $Candidates[0]
}

$s7Path = Resolve-ModelFile -EnvName "SUPERNAUT_S7_PATH" -Candidates @(
  (Join-Path $modelRoot "micronaut-weights-v2.s7"),
  (Join-Path $modelRoot "model\micronaut-weights-v2.s7"),
  (Join-Path $modelRoot "supernaut\model\micronaut-weights-v2.s7")
)
$manifestPath = Resolve-ModelFile -EnvName "SUPERNAUT_MANIFEST_PATH" -Candidates @(
  (Join-Path $modelRoot "manifest-weights-v2.json"),
  (Join-Path $modelRoot "model\manifest-weights-v2.json"),
  (Join-Path $modelRoot "supernaut\model\manifest-weights-v2.json")
)
$trainingPath = Resolve-ModelFile -EnvName "SUPERNAUT_TRAINING_DATA_PATH" -Candidates @(
  (Join-Path $modelRoot "training-data.sorted.jsonl"),
  (Join-Path $modelRoot "model\training-data.sorted.jsonl"),
  (Join-Path $modelRoot "supernaut\model\training-data.sorted.jsonl")
)

$model_files = @($s7Path, $manifestPath, $trainingPath)
Write-Host "  Model root: $modelRoot" -ForegroundColor Gray

foreach ($file in $model_files) {
  if (Test-Path -LiteralPath $file) {
    $size = (Get-Item -LiteralPath $file).Length
    $size_mb = [math]::Round($size / 1MB, 2)
    Write-Host "  ✅ $(Split-Path $file -Leaf) ($size_mb MB)" -ForegroundColor Green
  } else {
    Write-Host "  ❌ Missing: $(Split-Path $file -Leaf)" -ForegroundColor Red
    Write-Host "     expected path: $file" -ForegroundColor DarkGray
    exit 1
  }
}

# Check port availability
Write-Host ""
Write-Host "Checking port $Port availability..." -ForegroundColor Yellow

$port_check = netstat -ano 2>$null | Select-String ":$Port"
if ($port_check) {
  Write-Host "  ⚠️  Port $Port already in use" -ForegroundColor Yellow
  $port = $Port + 1
  Write-Host "  Using alternate port: $port" -ForegroundColor Cyan
} else {
  Write-Host "  ✅ Port $Port available" -ForegroundColor Green
  $port = $Port
}

# Start the server
Write-Host ""
Write-Host "Starting Supernaut Python API Server..." -ForegroundColor Yellow
Write-Host "  Location: $PSScriptRoot\supernaut_api.py" -ForegroundColor Gray
Write-Host "  Port: $port" -ForegroundColor Gray
Write-Host ""

Write-Host "Server Configuration:" -ForegroundColor Cyan
Write-Host "  Model: Supernaut-Copilot v7.3.0" -ForegroundColor Gray
Write-Host "  Layers: 146" -ForegroundColor Gray
Write-Host "  Parameters: 100.3M" -ForegroundColor Gray
Write-Host "  Vocab: 256" -ForegroundColor Gray
Write-Host "  Deterministic: Yes (fixed timestamp)" -ForegroundColor Gray
Write-Host ""

Write-Host "Endpoints:" -ForegroundColor Cyan
Write-Host "  GET  http://localhost:$port/health" -ForegroundColor Gray
Write-Host "  GET  http://localhost:$port/model-info" -ForegroundColor Gray
Write-Host "  POST http://localhost:$port/tokenize" -ForegroundColor Gray
Write-Host "  POST http://localhost:$port/forward" -ForegroundColor Gray
Write-Host "  POST http://localhost:$port/generate" -ForegroundColor Gray
Write-Host ""

Write-Host "Press Ctrl+C to stop server..." -ForegroundColor Yellow
Write-Host ""

# Set environment variable for port
$env:FLASK_PORT = $port
$env:SUPERNAUT_MODEL_ROOT = $modelRoot
$env:SUPERNAUT_S7_PATH = $s7Path
$env:SUPERNAUT_MANIFEST_PATH = $manifestPath
$env:SUPERNAUT_TRAINING_DATA_PATH = $trainingPath

# Launch Python API server
try {
  & python "$PSScriptRoot\supernaut_api.py"
} catch {
  Write-Host "❌ Failed to start server: $_" -ForegroundColor Red
  exit 1
}
