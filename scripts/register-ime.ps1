param([switch]$Unregister, [string]$NativeDirectory = '')
$ErrorActionPreference = 'Stop'
$classId = '{49A6E853-66A7-45AC-981F-ECB7F74F50CF}'
$profileId = '{2079BC15-856C-4CBB-9B2E-5472C9903242}'
$tip = "0804:$classId$profileId"
$comKey = "Registry::HKEY_CURRENT_USER\Software\Classes\CLSID\$classId\InprocServer32"
if ($Unregister) {
  if (!(Test-Path -LiteralPath $comKey)) { Write-Output 'LinguaIME is not registered.'; exit 0 }
  $dll = (Get-Item -LiteralPath $comKey).GetValue('')
} else {
  if (!$NativeDirectory) { $NativeDirectory = Join-Path (Split-Path -Parent $PSScriptRoot) 'build\native' }
  $sourceHost = Join-Path $NativeDirectory 'lingua_tsf.dll'
  $sourceCore = Join-Path $NativeDirectory 'ime_ffi.dll'
  if (!(Test-Path -LiteralPath $sourceHost) -or !(Test-Path -LiteralPath $sourceCore)) { throw 'Native binaries are missing. Run pnpm native:build first.' }
  $version = (Get-FileHash -LiteralPath $sourceHost).Hash.Substring(0, 8) + '-' + (Get-FileHash -LiteralPath $sourceCore).Hash.Substring(0, 8)
  $installDirectory = Join-Path $env:LOCALAPPDATA "LinguaIME\native\$version"
  New-Item -ItemType Directory -Path $installDirectory -Force | Out-Null
  $dll = Join-Path $installDirectory 'lingua_tsf.dll'
  if (!(Test-Path -LiteralPath $dll)) { Copy-Item -LiteralPath $sourceHost -Destination $dll }
  $core = Join-Path $installDirectory 'ime_ffi.dll'
  if (!(Test-Path -LiteralPath $core)) { Copy-Item -LiteralPath $sourceCore -Destination $core }
}
if (!(Test-Path -LiteralPath $dll)) { throw 'The registered native DLL is missing. Rebuild and reinstall LinguaIME.' }
$arguments = @('/s')
if ($Unregister) { $arguments += '/u' }
$arguments += "`"$dll`""
$identity = [Security.Principal.WindowsIdentity]::GetCurrent()
$principal = New-Object Security.Principal.WindowsPrincipal($identity)
$isAdministrator = $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
if ($isAdministrator) {
  $process = Start-Process -FilePath "$env:WINDIR\System32\regsvr32.exe" -ArgumentList $arguments -WindowStyle Hidden -Wait -PassThru
} else {
  # COM/TSF machine registration requires Windows elevation; only this step is elevated.
  $process = Start-Process -FilePath "$env:WINDIR\System32\regsvr32.exe" -ArgumentList $arguments -Verb RunAs -WindowStyle Hidden -Wait -PassThru
}
if ($process.ExitCode -ne 0) { throw "TSF registration failed (code $($process.ExitCode)). See docs/native.md." }

# Target only this profile; do not replace the user's existing keyboard/language list.
Add-Type -TypeDefinition @'
using System.Runtime.InteropServices;
public static class LinguaInputProfile {
  [DllImport("input.dll", CharSet=CharSet.Unicode, SetLastError=true)]
  [DefaultDllImportSearchPaths(DllImportSearchPath.System32)]
  [return: MarshalAs(UnmanagedType.Bool)]
  public static extern bool InstallLayoutOrTip(string profile, uint flags);
}
'@
$flags = [uint32]0
if ($Unregister) { $flags = [uint32]1 }
if (![LinguaInputProfile]::InstallLayoutOrTip("0x$tip", $flags)) { throw 'Windows could not update the language switcher. Open Windows language settings and retry.' }
Write-Output $(if ($Unregister) { 'LinguaIME unregistered. Personal data was retained.' } else { 'LinguaIME installed and enabled for this user. Select it with Win+Space.' })
