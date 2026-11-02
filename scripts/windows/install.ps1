#Requires -RunAsAdministrator
<#
.SYNOPSIS
    Installs the Internet Access Control plugin into an existing Veyon installation.
.DESCRIPTION
    Stops the Veyon service, copies internetaccesscontrol.dll into the Veyon plugins
    directory and starts the service again. Install the plugin on the supervisor computer
    running Veyon Master as well as on all managed computers.
.PARAMETER DllPath
    Path to internetaccesscontrol.dll (defaults to the directory of this script).
.EXAMPLE
    powershell -ExecutionPolicy Bypass -File install.ps1
#>

param(
    [string]$DllPath = (Join-Path $PSScriptRoot "internetaccesscontrol.dll")
)

$ErrorActionPreference = "Stop"

$veyonPath = @("$env:ProgramFiles\Veyon", "${env:ProgramFiles(x86)}\Veyon") |
    Where-Object { Test-Path (Join-Path $_ "veyon-core.dll") } |
    Select-Object -First 1
if (-not $veyonPath) {
    Write-Error "Veyon installation not found in $env:ProgramFiles\Veyon."
    exit 1
}

if (-not (Test-Path $DllPath)) {
    Write-Error "Plugin file '$DllPath' not found."
    exit 1
}

$veyonVersion = (Get-Item (Join-Path $veyonPath "veyon-core.dll")).VersionInfo.ProductVersion
Write-Host "[+] Veyon $veyonVersion found in $veyonPath"
if ($veyonVersion -notlike "4.11.*") {
    Write-Warning "This plugin was built for Veyon 4.11 and may not load with Veyon $veyonVersion."
}

$pluginsDir = Join-Path $veyonPath "plugins"
New-Item -ItemType Directory -Path $pluginsDir -Force | Out-Null

Write-Host "[*] Stopping Veyon service..."
Stop-Service -Name "VeyonService" -Force -ErrorAction SilentlyContinue
Start-Sleep -Seconds 1

Write-Host "[*] Copying plugin to $pluginsDir..."
Copy-Item -Path $DllPath -Destination (Join-Path $pluginsDir "internetaccesscontrol.dll") -Force

Write-Host "[*] Starting Veyon service..."
Start-Service -Name "VeyonService" -ErrorAction SilentlyContinue

Write-Host "[+] Internet Access Control installed successfully." -ForegroundColor Green
