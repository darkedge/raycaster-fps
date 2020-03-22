[System.IO.File]::WriteAllLines("wd.diff", (git diff))

[System.IO.File]::WriteAllLines("strings.txt", (git rev-parse HEAD))
Add-Content -Path strings.txt -Value (Get-Date -Format "yyyy-MM-dd HH:mm")
