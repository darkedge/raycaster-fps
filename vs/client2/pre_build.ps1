# Parameters are passed without quotes
Param(
    [String]$Platform,
    [String]$Configuration,
    [String]$TargetDir
)

# Path to bgfx shaderc executable (after Push-Location of shader folder)
$Exe = '../../../tools/bgfx/shaderc.exe'

# Path to shader source files
$ShaderPath = "..\..\src\client2\shaders"

if ($Configuration -eq 'Debug') {
    $Debug = '--debug'
}

$ShaderPathExists = Test-Path -Path $ShaderPath
if (-Not $ShaderPathExists) {
    Write-Output "Could not find shader path $($ShaderPath)!"
}
else {
    Push-Location -Path $ShaderPath

    function BuildShaders ([String]$Filter, [String]$Type, [String]$Profile) {
        Get-ChildItem -Recurse -Filter $Filter |
        Foreach-Object {
            $InFile = (Resolve-Path -Path $_.FullName -Relative)
            $InTime = (Get-Item $_.FullName).LastWriteTime
            $OutHeader = (Join-Path -Path $_.Directory -ChildPath ($_.Basename + '.h'))

            # Test if input is newer than output
            $HeaderExists = Test-Path -Path $OutHeader
            if ($HeaderExists) {
                $OutTime = (Get-Item $OutHeader).LastWriteTime
            }

            if (-Not $HeaderExists -Or ($InTime -gt $OutTime)) {
                # Write source file name to stdout
                Write-Output $_.Name

                # Generate compiled header file
                & $Exe -f $InFile -o $OutHeader --platform windows --Type $Type --Profile $Profile --bin2c $Debug
            } 
        }
    }

    # Compute shaders
    Write-Output 'Building compute shaders...'
    BuildShaders 'cs_*.sc' 'c' 'cs_5_0'

    # Vertex shaders
    Write-Output 'Building vertex shaders...'
    BuildShaders 'vs_*.sc' 'v' 'vs_5_0'

    # Fragment shaders
    Write-Output 'Building fragment shaders...'
    BuildShaders 'fs_*.sc' 'f' 'ps_5_0'
    
    Pop-Location
}

# Code generation

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

# Add strings
$Strings = New-Object System.Collections.ArrayList
$Strings += $StringTemplate -f 'GitCommitId', (git rev-parse --short HEAD)
$Strings += $StringTemplate -f 'GitRevision', (git rev-list --count HEAD)
$Strings += $StringTemplate -f 'GitBranch', (git rev-parse --abbrev-ref HEAD)
$Strings += $StringTemplate -f 'DateTime', (Get-Date -Format "yyyy-MM-dd HH:mm:ss")
$Strings += $StringTemplate -f 'BuildConfiguration', $Configuration

# Diff
$GitDiff = ((git diff) -ne $null, "")
$GitDiff = ($GitDiff -replace '\\', '\\') # Escape backslashes (input is escaped, output is not)
$GitDiff = ($GitDiff -replace '"', '\"') # Escape double quotes
$GitDiff = ($GitDiff -join '\r\n"' + [Environment]::NewLine + '      "') # Concatenate array
$Strings += $StringTemplate -f 'GitDiff', $GitDiff

$GeneratedPath = '..\..\src\client2\generated'
If (!(Test-Path -Path $GeneratedPath)) {
    New-Item -ItemType Directory -Force -Path $GeneratedPath
}

$TxtPath = Join-Path -Path $GeneratedPath -ChildPath 'text.h'
$TxtTemplate = $TxtTemplate -f $($Strings -join [Environment]::NewLine + '    ')
[System.IO.File]::WriteAllLines($TxtPath, $TxtTemplate)
