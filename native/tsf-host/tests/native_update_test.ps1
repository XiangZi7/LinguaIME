$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot '..\..\..\scripts\native-update.ps1')
$fixture = Join-Path (Get-Location) ('installer-fixture-' + [Guid]::NewGuid().ToString('N'))
$source = Join-Path $fixture 'source'
$state = Join-Path $fixture 'installed'
New-Item -ItemType Directory -Path $source -Force | Out-Null
foreach ($name in @('lingua_tsf.dll', 'lingua_runtime.dll', 'ime_ffi.dll')) {
  [IO.File]::WriteAllText((Join-Path $source $name), $name)
}
$first = Stage-LinguaRuntime $source $state
Publish-LinguaRuntime $first
$active = Join-Path $state 'active-runtime.txt'
if ((Read-LinguaRuntimeRecord $active) -ne $first.Record) { throw 'Initial publish failed' }
$share = [IO.FileShare]::ReadWrite -bor [IO.FileShare]::Delete
$reader = [IO.File]::Open($active, [IO.FileMode]::Open, [IO.FileAccess]::Read, $share)
try {
  [IO.File]::WriteAllText((Join-Path $source 'lingua_runtime.dll'), 'runtime-v2')
  $second = Stage-LinguaRuntime $source $state
  Publish-LinguaRuntime $second
  if ((Read-LinguaRuntimeRecord $active) -ne $second.Record) { throw 'Atomic replacement failed' }
  $buffer = New-Object byte[] $reader.Length
  [void]$reader.Read($buffer, 0, $buffer.Length)
  if ([Text.Encoding]::ASCII.GetString($buffer) -ne $first.Record) { throw 'Existing reader saw partial replacement' }
} finally { $reader.Dispose() }
if ((Read-LinguaRuntimeRecord (Join-Path $state 'previous-runtime.txt')) -ne $first.Record) {
  throw 'Previous runtime was not preserved'
}
Publish-LinguaRuntime $second
Undo-LinguaRuntimeUpdate $state
if ((Read-LinguaRuntimeRecord $active) -ne $first.Record) { throw 'Rollback failed' }
$version = ($second.Record -split '\n')[1]
$installedRuntime = Join-Path $state "runtime\$version\lingua_runtime.dll"
[IO.File]::WriteAllText($installedRuntime, 'corrupt')
$rejected = $false
try { Stage-LinguaRuntime $source $state | Out-Null }
catch { $rejected = $true }
if (!$rejected) { throw 'Corrupt immutable file was accepted' }
if ((Read-LinguaRuntimeRecord $active) -ne $first.Record) { throw 'Failed stage changed active runtime' }
Write-Output 'Native installer: immutable staging, atomic readers, previous version and rollback passed.'
