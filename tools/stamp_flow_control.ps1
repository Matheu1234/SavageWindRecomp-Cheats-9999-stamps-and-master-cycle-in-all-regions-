# Use in a *second* PowerShell window while Savage Wind is running with
# SAVAGEWIND_STAMP_FLOW=1. Phases are opt-in and never modify saved games.
param(
    [Parameter(Mandatory=$true)][string]$InstallDir,
    [Parameter(Mandatory=$true)][ValidateSet('baseline','add','remove','idle')][string]$Phase
)
$ErrorActionPreference = 'Stop'
$folder = (Resolve-Path -LiteralPath $InstallDir).Path
if (-not (Test-Path -LiteralPath (Join-Path $folder 'savagewind.exe'))) {
    throw "savagewind.exe nao foi encontrado em $folder"
}
$target = Join-Path $folder 'stamp-flow-control.txt'
[System.IO.File]::WriteAllText($target, $Phase + "`n", [System.Text.Encoding]::ASCII)
Write-Host "Fase solicitada: $Phase ($target)"
Write-Host 'O jogo reconhecera a alteracao no proximo ciclo de consulta (ate 15 quadros).'
