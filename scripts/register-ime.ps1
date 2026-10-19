param([switch]$Unregister)
$ErrorActionPreference = 'Stop'
$dll = Join-Path (Split-Path -Parent $PSScriptRoot) 'build\native\lingua_tsf.dll'
if (!(Test-Path -LiteralPath $dll)) { throw 'Run scripts/build-native.ps1 first' }
$arguments = @('/s')
if ($Unregister) { $arguments += '/u' }
$arguments += "`"$dll`""
$process = Start-Process -FilePath "$env:WINDIR\System32\regsvr32.exe" -ArgumentList $arguments -WindowStyle Hidden -Wait -PassThru
if ($process.ExitCode -ne 0) { throw "TSF registration failed (code $($process.ExitCode)). See docs/native.md." }
Write-Output $(if ($Unregister) { 'LinguaIME unregistered. Personal data was retained.' } else { 'LinguaIME registered. Select it with Win+Space; restart target applications if needed.' })
