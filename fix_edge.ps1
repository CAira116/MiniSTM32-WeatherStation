$edgePath = "C:\Program Files (x86)\Microsoft\Edge\Application\msedge.exe"
if (-not (Test-Path $edgePath)) { $edgePath = "C:\Program Files\Microsoft\Edge\Application\msedge.exe" }
if (Test-Path $edgePath) {
    $WshShell = New-Object -ComObject WScript.Shell
    $Shortcut = $WshShell.CreateShortcut("$env:USERPROFILE\Desktop\Microsoft Edge.lnk")
    $Shortcut.TargetPath = $edgePath
    $Shortcut.Arguments = ""
    $Shortcut.Save()
    Write-Host "Done - clean shortcut created"
} else { Write-Host "Edge.exe not found" }
