<#
.SYNOPSIS
    Automatically shares and attaches a USB device (e.g. your MCU/dev board) to WSL,
    and keeps re-attaching it every time it's unplugged/replugged or reset.

.HOW IT WORKS
    - Uses usbipd-win's --hardware-id (VID:PID) instead of --busid, so it keeps working
      even if Windows assigns the device a different bus ID on reconnect.
    - `usbipd policy add` runs once (as Admin) to permanently allow that VID:PID to be
      auto-bound, so you never have to run `usbipd bind` by hand again.
    - `usbipd attach --auto-attach` then runs forever in the background, watching for
      that device and attaching it to WSL the instant it appears.

.SETUP
    1. Find your device's VID:PID:
         usbipd list
       Look for your board, e.g. "0483:374b STMicroelectronics ST-LINK".

    2. Edit $HardwareId below (or pass -HardwareId when running the script).

    3. Run once as Administrator to register the auto-bind policy (only needed once,
       persists across reboots):
         powershell -File usbipd-auto-attach.ps1 -SetupPolicy

    4. Run normally (no admin needed) to start watching/attaching. Leave it running
       in a terminal, or set it up via Task Scheduler to start at login (see bottom
       of this file for a one-liner).
         powershell -File usbipd-auto-attach.ps1

.NOTES
    Requires usbipd-win 4.2.0+ (for --hardware-id / --auto-attach / policy). Update with:
      winget upgrade dorssel.usbipd-win
#>

param(
    [string]$HardwareId,  # <-- replace with your MCU's VID:PID from `usbipd list`
    [switch]$SetupPolicy
)

function Test-Admin {
    $id = [Security.Principal.WindowsIdentity]::GetCurrent()
    (New-Object Security.Principal.WindowsPrincipal $id).IsInRole([Security.Principal.WindowsBuiltinRole]::Administrator)
}

if ($SetupPolicy) {
    if (-not (Test-Admin)) {
        Write-Host "Re-launching elevated to register the auto-bind policy..."
        Start-Process powershell -Verb RunAs -ArgumentList "-File `"$PSCommandPath`" -HardwareId $HardwareId -SetupPolicy"
        exit
    }
    Write-Host "Registering auto-bind policy for $HardwareId ..."
    usbipd policy add --effect Allow --operation AutoBind --hardware-id $HardwareId
    Write-Host "Done. You can close this window and run the script normally (no admin) from now on."
    exit
}

Write-Host "Watching for device $HardwareId and auto-attaching to WSL. Ctrl+C to stop."
while ($true) {
    usbipd attach --wsl --hardware-id $HardwareId --auto-attach
    # --auto-attach blocks and keeps retrying internally, but if the usbipd service
    # restarts or the command exits for any reason, loop and restart it.
    Start-Sleep -Seconds 2
}

<#
OPTIONAL: run this automatically at every Windows login (hidden window), so you
never have to remember to start it. Run once from an elevated PowerShell:
#> 

$action  = New-ScheduledTaskAction -Execute "powershell.exe" -Argument "-WindowStyle Hidden -File `"C:\path\to\usbipd-auto-attach.ps1`""
$trigger = New-ScheduledTaskTrigger -AtLogOn
Register-ScheduledTask -TaskName "usbipd-auto-attach" -Action $action -Trigger $trigger -RunLevel Limited

