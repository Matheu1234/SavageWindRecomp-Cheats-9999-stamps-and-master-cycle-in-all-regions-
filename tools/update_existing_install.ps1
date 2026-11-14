# Update an existing portable Savage Wind Windows installation from a newly
# compiled release folder, without overwriting the user's game files or saves.
# Close Savage Wind first! Run from PowerShell, outside the installed folder.
param(
  [Parameter(Mandatory=$true)][string]$ReleaseDir,
  [Parameter(Mandatory=$true)][string]$InstallDir
)
$ErrorActionPreference = 'Stop'
$source = (Resolve-Path -LiteralPath $ReleaseDir).Path
$target = (Resolve-Path -LiteralPath $InstallDir).Path
if ($source -eq $target) { throw 'Source and installation must be different folders.' }
if ($target.StartsWith($source + [IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase) -or
    $source.StartsWith($target + [IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)) {
    throw 'Do not put the source inside the installation or vice versa.'
}
if (-not (Test-Path -LiteralPath (Join-Path $source 'savagewind.exe'))) {
    throw 'The source must be the compiled Windows distribution containing savagewind.exe.'
}
if (-not (Test-Path -LiteralPath (Join-Path $target 'savagewind.exe'))) {
    throw 'The destination must be an existing portable Savage Wind installation.'
}
$running = Get-Process -Name 'savagewind' -ErrorAction SilentlyContinue
if ($running) { throw 'Close Savage Wind completely before updating its executables.' }
$stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$backup = Join-Path $target ('.savagewind-update-backup-' + $stamp)
New-Item -ItemType Directory -Path $backup -Force | Out-Null
# Never touch these data directories/settings. A changed generator will cause
# Savage Wind's own launcher to rebuild swgame.dll from extracted/exefs.
$exclude = @('extracted','save','cache','fonts','cheats','savagewind.ini',
             'savagewind-cheats.ini','swgame.dll','swgame.dll.id','savagewind.log',
             'splash.jpg','splash.png','splash.jpeg','screenshots','colour-luts','logs','mods')
try {
  foreach($item in (Get-ChildItem -LiteralPath $source -Force)) {
    if ($exclude -contains $item.Name) { continue }
    $dst = Join-Path $target $item.Name
    if ($item.PSIsContainer) {
      if (Test-Path -LiteralPath $dst) {
        $save = Join-Path $backup $item.Name
        Move-Item -LiteralPath $dst -Destination $save -ErrorAction Stop
      }
      Copy-Item -LiteralPath $item.FullName -Destination $dst -Recurse -Force -ErrorAction Stop
    } else {
      if (Test-Path -LiteralPath $dst) {
        Copy-Item -LiteralPath $dst -Destination (Join-Path $backup $item.Name) -Force
      }
      Copy-Item -LiteralPath $item.FullName -Destination $dst -Force -ErrorAction Stop
    }
  }
  # Install new manifests and refresh the two built-in cheats (back up old
  # manifests); preserve third-party cheats and user toggle state.
  $srcCheats = Join-Path $source 'cheats'
  $dstCheats = Join-Path $target 'cheats'
  if (Test-Path -LiteralPath $srcCheats) {
    New-Item -ItemType Directory -Path $dstCheats -Force | Out-Null
    foreach($cheat in (Get-ChildItem -LiteralPath $srcCheats -Directory)) {
      $cheatDest = Join-Path $dstCheats $cheat.Name
      New-Item -ItemType Directory -Path $cheatDest -Force | Out-Null
      $srcManifest = Join-Path $cheat.FullName 'cheat.ini'
      $dstManifest = Join-Path $cheatDest 'cheat.ini'
      if (Test-Path -LiteralPath $srcManifest) {
        # Built-in manifests must be updated: v3 intentionally shipped a
        # 'planned' 9999-stamps entry. Preserve any previous version in backup.
        $known = @('9999 Map Stamps Limit', 'Motorcycle in All Regions')
        if ((Test-Path -LiteralPath $dstManifest) -and ($known -contains $cheat.Name)) {
          $manifestBackupDir = Join-Path (Join-Path $backup 'cheats') $cheat.Name
          New-Item -ItemType Directory -Path $manifestBackupDir -Force | Out-Null
          Copy-Item -LiteralPath $dstManifest -Destination (Join-Path $manifestBackupDir 'cheat.ini') -Force
          Copy-Item -LiteralPath $srcManifest -Destination $dstManifest -Force
        } elseif (-not (Test-Path -LiteralPath $dstManifest)) {
          Copy-Item -LiteralPath $srcManifest -Destination $dstManifest -Force
        }
      }
    }
  }
} catch {
  Write-Warning 'Update interrupted. Check the backup folder before retrying.'
  Write-Warning $backup
  throw
}
Write-Host "Update files installed. Previous files backed up in: $backup"
Write-Host 'If the game DLL version is incompatible, use Configuration > Settings > Prelaunch > Rebuild compiled game code.'
Write-Host 'For 9999 stamps, rebuild compiled game code (swgame.dll) after updating to embed the ARM64 patch.'
Write-Host 'Your saves, extracted game files and cheat settings have not been overwritten.'
