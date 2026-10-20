# Shared by the development installer and the bundled desktop installer.
$ErrorActionPreference = 'Stop'
if (-not ('LinguaRuntimeFiles' -as [type])) {
  Add-Type -TypeDefinition @'
using System.Runtime.InteropServices;
public static class LinguaRuntimeFiles {
  [DllImport("kernel32.dll", CharSet=CharSet.Unicode, SetLastError=true)]
  [return: MarshalAs(UnmanagedType.Bool)]
  public static extern bool MoveFileEx(string source, string target, uint flags);
  [DllImport("kernel32.dll", CharSet=CharSet.Unicode, SetLastError=true)]
  [return: MarshalAs(UnmanagedType.Bool)]
  public static extern bool ReplaceFile(string target, string source, System.IntPtr backup,
      uint flags, System.IntPtr exclude, System.IntPtr reserved);
}
'@
}

function Use-LinguaUpdateLock([scriptblock]$Action) {
  $mutex = New-Object Threading.Mutex($false, 'Local\LinguaIME.NativeUpdate')
  $acquired = $false
  try {
    try { $acquired = $mutex.WaitOne(30000) }
    catch [Threading.AbandonedMutexException] { $acquired = $true }
    if (!$acquired) { throw 'Another LinguaIME update is in progress. Retry shortly.' }
    & $Action
  } finally {
    if ($acquired) { $mutex.ReleaseMutex() }
    $mutex.Dispose()
  }
}

function Read-LinguaRuntimeRecord([string]$Path) {
  if (!(Test-Path -LiteralPath $Path)) { return $null }
  $record = [IO.File]::ReadAllText($Path)
  if ($record -notmatch '\A1\r?\n[0-9A-F]{64}-[0-9A-F]{64}\r?\n?\z') {
    throw "Invalid LinguaIME runtime pointer: $Path"
  }
  return $record
}

function Write-LinguaRuntimeRecord([string]$Path, [string]$Record) {
  $temporary = $Path + '.tmp-' + [Guid]::NewGuid().ToString('N')
  try {
    $bytes = [Text.Encoding]::ASCII.GetBytes($Record)
    $stream = [IO.File]::Open($temporary, [IO.FileMode]::CreateNew, [IO.FileAccess]::Write, [IO.FileShare]::None)
    try { $stream.Write($bytes, 0, $bytes.Length); $stream.Flush($true) }
    finally { $stream.Dispose() }
    # Rename on the same volume. Readers share delete access and see the old or the complete new file.
    $published = if (Test-Path -LiteralPath $Path) {
      [LinguaRuntimeFiles]::ReplaceFile($Path, $temporary, [IntPtr]::Zero, 2, [IntPtr]::Zero, [IntPtr]::Zero)
    } else {
      [LinguaRuntimeFiles]::MoveFileEx($temporary, $Path, 8)
    }
    if (!$published) {
      throw "Could not publish the runtime pointer (Windows code $([Runtime.InteropServices.Marshal]::GetLastWin32Error()))."
    }
  } finally {
    if (Test-Path -LiteralPath $temporary) { Remove-Item -LiteralPath $temporary }
  }
}

function Get-LinguaFileHash([string]$Path) {
  # Windows PowerShell child processes may inherit incompatible PowerShell 7 module paths.
  # Use the shared .NET API so hashing does not depend on importing Get-FileHash.
  $algorithm = [Security.Cryptography.SHA256]::Create()
  $stream = $null
  try {
    $stream = [IO.File]::OpenRead($Path)
    return [BitConverter]::ToString($algorithm.ComputeHash($stream)).Replace('-', '')
  } finally {
    if ($stream) { $stream.Dispose() }
    $algorithm.Dispose()
  }
}

function Copy-LinguaImmutableFile([string]$Source, [string]$Target, [string]$ExpectedHash) {
  if (!(Test-Path -LiteralPath $Target)) { Copy-Item -LiteralPath $Source -Destination $Target }
  if ((Get-LinguaFileHash $Target) -ne $ExpectedHash) {
    throw "Installed native file failed verification: $Target"
  }
}

function Test-LinguaRimeBundle([string]$Directory) {
  $manifestPath = Join-Path $Directory 'manifest.json'
  $idPath = Join-Path $Directory 'runtime-id.txt'
  if (!(Test-Path -LiteralPath $manifestPath) -or !(Test-Path -LiteralPath $idPath)) {
    throw 'Rime bundle is missing. Run pnpm native:build first.'
  }
  if ([IO.File]::ReadAllText($idPath).Trim() -ne (Get-LinguaFileHash $manifestPath)) {
    throw 'Rime manifest failed verification.'
  }
  $manifest = [IO.File]::ReadAllText($manifestPath) | ConvertFrom-Json
  foreach ($required in @('rime.dll', 'data/build/default.yaml', 'data/build/lingua_pinyin.schema.yaml',
                          'data/build/pinyin_simp.prism.bin', 'data/build/pinyin_simp.table.bin')) {
    if (!$manifest.files.PSObject.Properties[$required]) { throw "Rime manifest is missing $required" }
  }
  foreach ($file in $manifest.files.PSObject.Properties) {
    if ($file.Name -notmatch '\A(?:rime\.dll|data/(?:build/)?[a-zA-Z0-9_.-]+)\z' -or
        $file.Value -notmatch '\A[0-9A-F]{64}\z') { throw 'Invalid Rime manifest entry.' }
    $path = Join-Path $Directory $file.Name
    if (!(Test-Path -LiteralPath $path) -or (Get-LinguaFileHash $path) -ne $file.Value) {
      throw "Rime file failed verification: $($file.Name)"
    }
  }
  return $manifest
}

function Stage-LinguaRuntime([string]$SourceDirectory, [string]$StateDirectory) {
  Use-LinguaUpdateLock {
    $sourceHost = Join-Path $SourceDirectory 'lingua_tsf.dll'
    $sourceRuntime = Join-Path $SourceDirectory 'lingua_runtime.dll'
    $sourceCore = Join-Path $SourceDirectory 'ime_ffi.dll'
    foreach ($source in @($sourceHost, $sourceRuntime, $sourceCore)) {
      if (!(Test-Path -LiteralPath $source)) { throw 'Native binaries are missing. Run pnpm native:build first.' }
    }
    $rimeSource = Join-Path $SourceDirectory 'rime'
    $rimeManifest = Test-LinguaRimeBundle $rimeSource
    $hostHash = Get-LinguaFileHash $sourceHost
    $runtimeHash = Get-LinguaFileHash $sourceRuntime
    $coreHash = Get-LinguaFileHash $sourceCore
    $version = $runtimeHash + '-' + $coreHash
    $hostDirectory = Join-Path $StateDirectory "host-v1-$hostHash"
    $runtimeDirectory = Join-Path $StateDirectory "runtime\$version"
    New-Item -ItemType Directory -Path $hostDirectory, $runtimeDirectory -Force | Out-Null
    $hostDll = Join-Path $hostDirectory 'lingua_tsf.dll'
    Copy-LinguaImmutableFile $sourceHost $hostDll $hostHash
    Copy-LinguaImmutableFile $sourceRuntime (Join-Path $runtimeDirectory 'lingua_runtime.dll') $runtimeHash
    Copy-LinguaImmutableFile $sourceCore (Join-Path $runtimeDirectory 'ime_ffi.dll') $coreHash
    $rimeTarget = Join-Path $runtimeDirectory 'rime'
    New-Item -ItemType Directory -Path $rimeTarget -Force | Out-Null
    foreach ($file in $rimeManifest.files.PSObject.Properties) {
      $target = Join-Path $rimeTarget $file.Name
      New-Item -ItemType Directory -Path (Split-Path -Parent $target) -Force | Out-Null
      Copy-LinguaImmutableFile (Join-Path $rimeSource $file.Name) $target $file.Value
    }
    foreach ($name in @('manifest.json','runtime-id.txt')) {
      $source = Join-Path $rimeSource $name
      Copy-LinguaImmutableFile $source (Join-Path $rimeTarget $name) (Get-LinguaFileHash $source)
    }
    Test-LinguaRimeBundle $rimeTarget | Out-Null
    [pscustomobject]@{ Dll = $hostDll; Root = $StateDirectory; Record = "1`n$version`n" }
  }
}

function Publish-LinguaRuntime($Installation) {
  Use-LinguaUpdateLock {
    $active = Join-Path $Installation.Root 'active-runtime.txt'
    $previous = Join-Path $Installation.Root 'previous-runtime.txt'
    $old = Read-LinguaRuntimeRecord $active
    if ($old -eq $Installation.Record) { return }
    if ($old) { Write-LinguaRuntimeRecord $previous $old }
    Write-LinguaRuntimeRecord $active $Installation.Record
  }
}

function Undo-LinguaRuntimeUpdate([string]$StateDirectory) {
  Use-LinguaUpdateLock {
    $activePath = Join-Path $StateDirectory 'active-runtime.txt'
    $previousPath = Join-Path $StateDirectory 'previous-runtime.txt'
    $current = Read-LinguaRuntimeRecord $activePath
    $previous = Read-LinguaRuntimeRecord $previousPath
    if (!$current -or !$previous) { throw 'No previous runtime is available.' }
    $version = ($previous -split '\r?\n')[1]
    $directory = Join-Path $StateDirectory "runtime\$version"
    if (!(Test-Path -LiteralPath (Join-Path $directory 'lingua_runtime.dll')) -or
        !(Test-Path -LiteralPath (Join-Path $directory 'ime_ffi.dll'))) {
      throw 'The previous runtime is incomplete. Reinstall a known good version.'
    }
    $hashes = $version -split '-'
    if ((Get-LinguaFileHash (Join-Path $directory 'lingua_runtime.dll')) -ne $hashes[0] -or
        (Get-LinguaFileHash (Join-Path $directory 'ime_ffi.dll')) -ne $hashes[1]) {
      throw 'The previous runtime failed verification. Reinstall a known good version.'
    }
    if (Test-Path -LiteralPath (Join-Path $directory 'rime')) {
      Test-LinguaRimeBundle (Join-Path $directory 'rime') | Out-Null
    }
    Write-LinguaRuntimeRecord $previousPath $current
    Write-LinguaRuntimeRecord $activePath $previous
    Write-Output 'LinguaIME rollback published. Running bootstrap hosts apply it when input is idle.'
  }
}
