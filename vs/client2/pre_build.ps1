# Parameters are passed without quotes
Param(
    [String]$Platform,
    [String]$Configuration,
    [String]$TargetDir
)

$cmd = '../../../tools/bgfx/shaderc.exe'
$shaderPath = "..\..\src\game_client\shaders"
$exists = Test-Path -Path $shaderPath
if ($exists) {
    Push-Location -Path $shaderPath
    Get-ChildItem -Recurse -Filter vs_*.sc |
    Foreach-Object {    
        $rel = (Resolve-Path -Path $_.FullName -Relative)
        $scWrite = (Get-Item $_.FullName).LastWriteTime
        $out = (Join-Path -Path $_.Directory -ChildPath ($_.Basename + '.h'))
        $exists = Test-Path -Path $out

        if ($exists) {
            $hWrite = (Get-Item $out).LastWriteTime
        }

        if (-Not $exists -Or ($scWrite -gt $hWrite)) {
            Write-Output $_.Name
            $prm = '-f', $rel, '-o', $out, '--platform', 'windows', '--type', 'v', '--profile', 'vs_5_0', '--bin2c'
            & $cmd $prm
        } 
    }
    Pop-Location

    Push-Location -Path $shaderPath
    Get-ChildItem -Recurse -Filter fs_*.sc |
    Foreach-Object {
        $rel = (Resolve-Path -Path $_.FullName -Relative)
        $scWrite = (Get-Item $_.FullName).LastWriteTime
        $out = (Join-Path -Path $_.Directory -ChildPath ($_.Basename + '.h'))
        $exists = Test-Path -Path $out

        if ($exists) {
            $hWrite = (Get-Item $out).LastWriteTime
        }
    
        if (-Not $exists -Or ($scWrite -gt $hWrite)) {
            Write-Output $_.Name
            $prm = '-f', $rel, '-o', $out, '--platform', 'windows', '--type', 'f', '--profile', 'ps_5_0', '--bin2c'
            & $cmd $prm
        }
    }
    Pop-Location
}
