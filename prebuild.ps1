[System.IO.File]::WriteAllLines("wd.diff", (git diff))

[System.IO.File]::WriteAllLines("strings.txt", (git rev-parse --short HEAD))
Add-Content -Path strings.txt -Value (Get-Date -Format "yyyy-MM-dd HH:mm")
Add-Content -Path strings.txt -Value (git rev-list --count HEAD)
