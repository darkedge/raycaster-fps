# Parameters are passed without quotes
Param(
    [String]$Platform,
    [String]$Configuration,
    [String]$TargetDir
)

# Path to bgfx shaderc executable
$Exe = '../../../tools/bgfx/shaderc.exe'

# Path to shader source files
$ShaderPath = "..\..\src\client2\shaders"

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
            $OutHlsl = (Join-Path -Path $_.Directory -ChildPath ($_.Basename + '.hlsl'))

            # Test if input is newer than output
            $HeaderExists = Test-Path -Path $OutHeader
            if ($HeaderExists) {
                $OutTime = (Get-Item $OutHeader).LastWriteTime
            }

            if (-Not $HeaderExists -Or ($InTime -gt $OutTime)) {
                # Write source file name to stdout
                Write-Output $_.Name

                # Generate compiled header file
                & $Exe -f $InFile -o $OutHeader --platform windows --Type $Type --Profile $Profile --bin2c
                
                # Generate preprocessed file for debugging
                & $Exe -f $InFile -o $OutHlsl --platform windows --Type $Type --Profile $Profile --preprocess
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
