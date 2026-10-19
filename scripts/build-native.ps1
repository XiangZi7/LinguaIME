param([switch]$SkipRust)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
Push-Location $projectRoot
try {
  if (!$SkipRust) {
    cargo build -p ime-ffi --release --locked --offline
    if ($LASTEXITCODE -ne 0) { throw 'Rust bridge build failed' }
  }
  $installation = & "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
  if (!$installation) { throw 'Install Visual Studio C++ desktop build tools and Windows SDK' }
  $output = Join-Path $projectRoot 'build\native'
  New-Item -ItemType Directory -Path $output -Force | Out-Null
  $native = Join-Path $projectRoot 'native\tsf-host'
  $rustOutput = Join-Path $projectRoot 'target\release'
  Copy-Item -LiteralPath (Join-Path $rustOutput 'ime_ffi.dll') -Destination $output -Force
  $sources = @('dll.cpp','registration.cpp','text_service.cpp','candidate_window.cpp') | ForEach-Object { '"' + (Join-Path $native "src\$_") + '"' }
  $response = @('/nologo /std:c++20 /EHsc /MD /utf-8 /W4 /DUNICODE /D_UNICODE /LD', "/I`"$native\include`"", ($sources -join ' '), "/link /OUT:`"$output\lingua_tsf.dll`" `"$rustOutput\ime_ffi.dll.lib`" msctf.lib ole32.lib oleaut32.lib uuid.lib advapi32.lib user32.lib gdi32.lib dwmapi.lib")
  $response = $response -replace 'msctf.lib ', ''
  $response += "/DEF:`"$native\lingua_tsf.def`""
  $response | Set-Content -LiteralPath (Join-Path $output 'host.rsp') -Encoding utf8
  $testResponse = @('/nologo /std:c++20 /EHsc /MD /utf-8 /W4', "/I`"$native\include`" `"$native\tests\key_policy_test.cpp`"", "/link /OUT:`"$output\native_contract_test.exe`" `"$rustOutput\ime_ffi.dll.lib`"")
  $testResponse | Set-Content -LiteralPath (Join-Path $output 'test.rsp') -Encoding utf8
  $devcmd = Join-Path $installation 'Common7\Tools\VsDevCmd.bat'
  $batch = Join-Path $output 'compile.cmd'
  @("@echo off", "call `"$devcmd`" -arch=x64 -host_arch=x64 >nul", 'if errorlevel 1 exit /b 1', 'cl @host.rsp', 'if errorlevel 1 exit /b 1', 'cl @test.rsp', 'exit /b %errorlevel%') | Set-Content -LiteralPath $batch -Encoding ascii
  Push-Location $output
  try {
    & $env:ComSpec /d /c $batch
    if ($LASTEXITCODE -ne 0) { throw 'Native compilation failed' }
    & (Join-Path $output 'native_contract_test.exe')
    if ($LASTEXITCODE -ne 0) { throw 'Native contract test failed' }
  } finally { Pop-Location }
} finally { Pop-Location }
