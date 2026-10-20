param([switch]$Download)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$bundle = Join-Path $projectRoot 'build\native\rime'
$cache = Join-Path $projectRoot 'build\rime-download'
$archive = Join-Path $cache 'rime.7z'
$archiveHash = '7478C7CAA4FF6B37DE86DABA1F7CE4A994A4F5BA24872A820FB2B3A9B01FED15'
function Get-RimeHash([string]$Path) {
  $algorithm = [Security.Cryptography.SHA256]::Create()
  $stream = [IO.File]::OpenRead($Path)
  try { [BitConverter]::ToString($algorithm.ComputeHash($stream)).Replace('-', '') }
  finally { $stream.Dispose(); $algorithm.Dispose() }
}
if (!(Test-Path -LiteralPath (Join-Path $bundle 'rime.dll')) -or
    !(Test-Path -LiteralPath (Join-Path $bundle 'tools\rime_deployer.exe'))) {
  New-Item -ItemType Directory -Path $cache, (Join-Path $bundle 'tools') -Force | Out-Null
  if (!(Test-Path -LiteralPath $archive)) {
    if (!$Download) { throw 'Rime runtime missing. Run powershell -File scripts/prepare-rime.ps1 -Download once.' }
    Invoke-WebRequest -Uri 'https://github.com/rime/librime/releases/download/1.17.0/rime-33e7814-Windows-msvc-x64.7z' -OutFile $archive
  }
  if ((Get-RimeHash $archive) -ne $archiveHash) { throw 'Rime release archive failed SHA-256 verification.' }
  $sevenZip = Join-Path $env:ProgramFiles '7-Zip\7z.exe'
  if (!(Test-Path -LiteralPath $sevenZip)) { throw 'Install 7-Zip to extract the official Rime .7z release.' }
  $unpacked = Join-Path $cache 'unpacked'
  & $sevenZip x $archive "-o$unpacked" -y | Out-Null
  if ($LASTEXITCODE -ne 0) { throw 'Rime archive extraction failed.' }
  Copy-Item -LiteralPath (Join-Path $unpacked 'dist\lib\rime.dll') -Destination (Join-Path $bundle 'rime.dll') -Force
  Copy-Item -LiteralPath (Join-Path $unpacked 'dist\bin\rime_deployer.exe') -Destination (Join-Path $bundle 'tools\rime_deployer.exe') -Force
}
if ((Get-RimeHash (Join-Path $bundle 'rime.dll')) -ne '86B4C7357D4C6D293CE5589B234D8859CA2AC30923A03BEDFA3926EEAF97FB0B') {
  throw 'Rime DLL failed SHA-256 verification.'
}
$data = Join-Path $bundle 'data'
New-Item -ItemType Directory -Path $data -Force | Out-Null
$changed = $false
foreach ($file in Get-ChildItem -LiteralPath (Join-Path $projectRoot 'data\rime') -File) {
  $target = Join-Path $data $file.Name
  if (!(Test-Path -LiteralPath $target) -or (Get-RimeHash $file.FullName) -ne (Get-RimeHash $target)) {
    Copy-Item -LiteralPath $file.FullName -Destination $target -Force
    $changed = $true
  }
}
if ($changed -or !(Test-Path -LiteralPath (Join-Path $data 'build\lingua_pinyin.schema.yaml'))) {
  $previousPath = $env:PATH
  try {
    $env:PATH = $bundle + ';' + $env:PATH
    & (Join-Path $bundle 'tools\rime_deployer.exe') --build $data $data (Join-Path $data 'build')
    if ($LASTEXITCODE -ne 0) { throw 'Rime schema deployment failed.' }
  } finally { $env:PATH = $previousPath }
}
# Only immutable runtime files are shipped. Deployment tools and machine-generated
# installation metadata remain in the build directory.
$files = [ordered]@{'rime.dll' = (Get-RimeHash (Join-Path $bundle 'rime.dll'))}
foreach ($file in Get-ChildItem -LiteralPath (Join-Path $data 'build') -File | Sort-Object Name) {
  $files['data/build/' + $file.Name] = Get-RimeHash $file.FullName
}
foreach ($name in @('LIBRIME-LICENSE.txt','LICENSE','AUTHORS')) {
  $files['data/' + $name] = Get-RimeHash (Join-Path $data $name)
}
$manifest = [ordered]@{version = '1.17.0'; files = $files} | ConvertTo-Json -Depth 4 -Compress
$manifestPath = Join-Path $bundle 'manifest.json'
[IO.File]::WriteAllText($manifestPath, $manifest, (New-Object Text.UTF8Encoding($false)))
[IO.File]::WriteAllText((Join-Path $bundle 'runtime-id.txt'), (Get-RimeHash $manifestPath), [Text.Encoding]::ASCII)
Write-Output 'Rime 1.17.0: verified runtime and offline pinyin schema ready.'
