#Requires -RunAsAdministrator
<#
.SYNOPSIS
    Removes the Internet Access Control plugin and all of its firewall rules.
.EXAMPLE
    powershell -ExecutionPolicy Bypass -File uninstall.ps1
#>

$ErrorActionPreference = "Stop"

$veyonPath = @("$env:ProgramFiles\Veyon", "${env:ProgramFiles(x86)}\Veyon") |
    Where-Object { Test-Path $_ } |
    Select-Object -First 1

Write-Host "[*] Stopping Veyon service..."
Stop-Service -Name "VeyonService" -Force -ErrorAction SilentlyContinue

Write-Host "[*] Removing firewall rules..."
Get-NetFirewallRule -DisplayName "Veyon_IAC_*" -ErrorAction SilentlyContinue | Remove-NetFirewallRule

if ($veyonPath) {
    $pluginFile = Join-Path $veyonPath "plugins\internetaccesscontrol.dll"
    if (Test-Path $pluginFile) {
        Remove-Item -Path $pluginFile -Force
        Write-Host "[+] Removed $pluginFile"
    } else {
        Write-Host "[-] Plugin was not installed in $pluginFile"
    }

    Write-Host "[*] Starting Veyon service..."
    Start-Service -Name "VeyonService" -ErrorAction SilentlyContinue
}

Write-Host "[+] Internet Access Control uninstalled." -ForegroundColor Green
