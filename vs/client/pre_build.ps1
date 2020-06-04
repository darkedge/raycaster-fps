# Parameters are passed without quotes
Param(
    [String]$Platform,
    [String]$Configuration,
    [String]$TargetDir
)

# Code generation
# TODO: Disabled until we load this at run-time instead of compiling it in
<#

$TxtTemplate = @'
#pragma once
namespace mj
{{
  namespace txt
  {{
    {0}
  }}
}}
'@

$StringTemplate = 'const char* p{0} = "{1}";'

function FormatDiff ([String[]]$Diff) {
    $Diff = ($Diff -replace '\\', '\\') # Escape backslashes (input is escaped, output is not)
    $Diff = ($Diff -replace '"', '\"') # Escape double quotes
    $Diff = ($Diff -join '\r\n"' + [Environment]::NewLine + '      "') # Concatenate array
    return $Diff
}

# Add strings
$Strings = New-Object System.Collections.ArrayList
$Strings += $StringTemplate -f 'GitCommitId', (git rev-parse --short HEAD)
$Strings += $StringTemplate -f 'GitRevision', (git rev-list --count HEAD)
$Strings += $StringTemplate -f 'GitBranch', (git rev-parse --abbrev-ref HEAD)
$Strings += $StringTemplate -f 'DateTime', (Get-Date -Format "yyyy-MM-dd HH:mm:ss")
$Strings += $StringTemplate -f 'BuildConfiguration', $Configuration
$Strings += $StringTemplate -f 'GitDiff', (FormatDiff (git diff --shortstat))
$Strings += $StringTemplate -f 'GitDiffStaged', (FormatDiff (git diff --staged --shortstat))

$GeneratedPath = '..\..\src\client\generated'
If (!(Test-Path -Path $GeneratedPath)) {
    New-Item -ItemType Directory -Force -Path $GeneratedPath
}

$TxtPath = Join-Path -Path $GeneratedPath -ChildPath 'text.h'
$TxtTemplate = $TxtTemplate -f $($Strings -join [Environment]::NewLine + '    ')
[System.IO.File]::WriteAllLines($TxtPath, $TxtTemplate)

#>
