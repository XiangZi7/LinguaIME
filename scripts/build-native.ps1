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
  $sources = @('dll.cpp','text_service.cpp','input_mode.cpp','language_bar.cpp','candidate_window.cpp','candidate_renderer.cpp') | ForEach-Object { '"' + (Join-Path $native "src\$_") + '"' }
  $response = @('/nologo /std:c++20 /EHsc /MD /O2 /utf-8 /W4 /WX /DUNICODE /D_UNICODE /LD', "/I`"$native\include`"", ($sources -join ' '), "/link /Brepro /OUT:`"$output\lingua_runtime.dll`" `"$rustOutput\ime_ffi.dll.lib`" ole32.lib oleaut32.lib uuid.lib advapi32.lib user32.lib gdi32.lib dwmapi.lib")
  $response[$response.Length - 1] += " /DEF:`"$native\lingua_runtime.def`" /DELAYLOAD:ime_ffi.dll /DELAY:UNLOAD delayimp.lib /IMPLIB:`"$output\lingua_runtime.lib`""
  $response[$response.Length - 1] += ' d2d1.lib dwrite.lib'
  $response | Set-Content -LiteralPath (Join-Path $output 'runtime.rsp') -Encoding utf8
  $hostSources = @('dll.cpp','registration.cpp','bootstrap_service.cpp','runtime_loader.cpp') | ForEach-Object { '"' + (Join-Path $native "src\$_") + '"' }
  @('/nologo /std:c++20 /EHsc /MD /utf-8 /W4 /WX /DUNICODE /D_UNICODE /DLINGUA_BOOTSTRAP /LD', "/I`"$native\include`"", ($hostSources -join ' '), "/link /Brepro /OUT:`"$output\lingua_tsf.dll`" /DEF:`"$native\lingua_tsf.def`" /IMPLIB:`"$output\lingua_tsf.lib`" ole32.lib oleaut32.lib uuid.lib advapi32.lib user32.lib") | Set-Content -LiteralPath (Join-Path $output 'host.rsp') -Encoding utf8
  $testResponse = @('/nologo /std:c++20 /EHsc /MD /utf-8 /W4', "/I`"$native\include`" `"$native\tests\key_policy_test.cpp`"", "/link /OUT:`"$output\native_contract_test.exe`" `"$rustOutput\ime_ffi.dll.lib`"")
  $testResponse | Set-Content -LiteralPath (Join-Path $output 'test.rsp') -Encoding utf8
  @('/nologo /std:c++20 /EHsc /MD /utf-8 /W4 /WX /DUNICODE /D_UNICODE /DLINGUA_TESTING', "/I`"$native\include`" `"$native\tests\tsf_context_test.cpp`"", "text_service.obj input_mode.obj language_bar.obj candidate_window.obj candidate_renderer.obj /link /OUT:`"$output\tsf_context_test.exe`" `"$rustOutput\ime_ffi.dll.lib`" ole32.lib oleaut32.lib uuid.lib advapi32.lib user32.lib gdi32.lib dwmapi.lib d2d1.lib dwrite.lib") | Set-Content -LiteralPath (Join-Path $output 'context-test.rsp') -Encoding utf8
  $devcmd = Join-Path $installation 'Common7\Tools\VsDevCmd.bat'
  @('/nologo /std:c++20 /EHsc /MD /utf-8 /W4 /WX /DUNICODE /D_UNICODE', "/I`"$native\include`" `"$native\tests\profile_tool.cpp`"", "/link /OUT:`"$output\profile_tool.exe`" ole32.lib oleaut32.lib uuid.lib user32.lib") | Set-Content -LiteralPath (Join-Path $output 'profile-tool.rsp') -Encoding utf8
  @('/nologo /std:c++20 /EHsc /MD /utf-8 /W4 /WX /DUNICODE /D_UNICODE /LD /DMOCK_FLAVOR=1', "/I`"$native\include`" `"$native\tests\mock_runtime.cpp`"", "/link /OUT:`"$output\mock-runtime-a.dll`" /DEF:`"$native\tests\mock_runtime.def`" ole32.lib oleaut32.lib uuid.lib") | Set-Content -LiteralPath (Join-Path $output 'mock-a.rsp') -Encoding utf8
  @('/nologo /std:c++20 /EHsc /MD /utf-8 /W4 /WX /DUNICODE /D_UNICODE /LD /DMOCK_FLAVOR=2', "/I`"$native\include`" `"$native\tests\mock_runtime.cpp`"", "/link /OUT:`"$output\mock-runtime-b.dll`" /DEF:`"$native\tests\mock_runtime.def`" ole32.lib oleaut32.lib uuid.lib") | Set-Content -LiteralPath (Join-Path $output 'mock-b.rsp') -Encoding utf8
  @('/nologo /std:c++20 /EHsc /MD /utf-8 /W4 /WX /DUNICODE /D_UNICODE /DLINGUA_TESTING', "/I`"$native\include`" `"$native\tests\hot_reload_test.cpp`" `"$native\src\bootstrap_service.cpp`" `"$native\src\runtime_loader.cpp`"", "/link /OUT:`"$output\hot_reload_test.exe`" ole32.lib oleaut32.lib uuid.lib user32.lib") | Set-Content -LiteralPath (Join-Path $output 'hot-reload.rsp') -Encoding utf8
  $batch = Join-Path $output 'compile.cmd'
  $compile = @("@echo off", "call `"$devcmd`" -arch=x64 -host_arch=x64 >nul", 'if errorlevel 1 exit /b 1')
  foreach ($responseName in @('runtime','host','test','context-test','profile-tool','mock-a','mock-b','hot-reload')) {
    $compile += "cl @$responseName.rsp"
    $compile += 'if errorlevel 1 exit /b 1'
  }
  $compile += 'exit /b 0'
  $compile | Set-Content -LiteralPath $batch -Encoding ascii
  Push-Location $output
  try {
    & $env:ComSpec /d /c $batch
    if ($LASTEXITCODE -ne 0) { throw 'Native compilation failed' }
    $previousDataDirectory = $env:LINGUAIME_DATA_DIR
    $env:LINGUAIME_DATA_DIR = Join-Path $output 'test-data'
    try {
      & (Join-Path $output 'native_contract_test.exe')
      if ($LASTEXITCODE -ne 0) { throw 'Native contract test failed' }
      & (Join-Path $output 'tsf_context_test.exe')
      if ($LASTEXITCODE -ne 0) { throw 'TSF context test failed' }
      & (Join-Path $output 'hot_reload_test.exe')
      if ($LASTEXITCODE -ne 0) { throw 'Runtime hot update test failed' }
      & powershell -NoProfile -ExecutionPolicy Bypass -File (Join-Path $native 'tests\native_update_test.ps1')
      if ($LASTEXITCODE -ne 0) { throw 'Native installation update test failed' }
    } finally { $env:LINGUAIME_DATA_DIR = $previousDataDirectory }
  } finally { Pop-Location }
} finally { Pop-Location }
