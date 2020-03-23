Param(
  [String]$Platform,
  [String]$Configuration,
  [String]$TargetDir
)

[System.IO.File]::WriteAllLines("wd.diff", ((git diff), "" -ne $null)[0])

[System.IO.File]::WriteAllLines("strings.txt", (git rev-parse --short HEAD))
Add-Content -Path strings.txt -Value (git rev-list --count HEAD)
Add-Content -Path strings.txt -Value (git rev-parse --abbrev-ref HEAD)
Add-Content -Path strings.txt -Value (Get-Date -Format "yyyy-MM-dd HH:mm")
Add-Content -Path strings.txt -Value ($Configuration)
