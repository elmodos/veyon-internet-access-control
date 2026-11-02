#Requires -RunAsAdministrator
<#
.SYNOPSIS
    Shows the state of the Internet Access Control plugin on this computer.
.DESCRIPTION
    Reports whether the plugin is installed, whether the Veyon service is running,
    which firewall rules of the plugin are active and whether the internet and the
    default gateway are reachable.
.PARAMETER RemoveRules
    Removes all firewall rules of the plugin, e.g. to recover from a stuck block.
#>

param(
    [switch]$RemoveRules
)

$veyonPath = @("$env:ProgramFiles\Veyon", "${env:ProgramFiles(x86)}\Veyon") |
    Where-Object { Test-Path $_ } |
    Select-Object -First 1

Write-Host "== Plugin"
$pluginFile = if ($veyonPath) { Join-Path $veyonPath "plugins\internetaccesscontrol.dll" }
if ($pluginFile -and (Test-Path $pluginFile)) {
    Write-Host "  installed: $pluginFile ($((Get-Item $pluginFile).LastWriteTime))" -ForegroundColor Green
} else {
    Write-Host "  not installed" -ForegroundColor Yellow
}

Write-Host "== Veyon service"
$service = Get-Service -Name "VeyonService" -ErrorAction SilentlyContinue
Write-Host "  $(if ($service) { $service.Status } else { 'not found' })"

Write-Host "== Firewall rules"
$rules = Get-NetFirewallRule -DisplayName "Veyon_IAC_*" -ErrorAction SilentlyContinue
if ($rules) {
    foreach ($rule in $rules) {
        $port = $rule | Get-NetFirewallPortFilter
        Write-Host ("  {0}  enabled={1}  protocol={2}  ports={3}" -f $rule.DisplayName, $rule.Enabled, $port.Protocol, ($port.RemotePort -join ","))
    }
    Write-Host "  -> internet is BLOCKED" -ForegroundColor Yellow
} else {
    Write-Host "  none -> internet is not blocked by the plugin" -ForegroundColor Green
}

Write-Host "== Connectivity"
$gateway = (Get-NetRoute -DestinationPrefix "0.0.0.0/0" -ErrorAction SilentlyContinue | Sort-Object RouteMetric | Select-Object -First 1).NextHop
if ($gateway) {
    $ok = Test-Connection -ComputerName $gateway -Count 1 -Quiet
    Write-Host "  gateway $gateway reachable: $ok"
}
$internet = Test-NetConnection -ComputerName "1.1.1.1" -Port 443 -WarningAction SilentlyContinue
Write-Host "  1.1.1.1:443 reachable: $($internet.TcpTestSucceeded)"

if ($RemoveRules -and $rules) {
    $rules | Remove-NetFirewallRule
    Write-Host "[+] Removed $($rules.Count) firewall rule(s)." -ForegroundColor Green
}
