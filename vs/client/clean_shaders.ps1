# Path to shader source files
$ShaderPath = "..\..\src\client\shaders"

foreach ($file in Get-ChildItem -Path $ShaderPath -Recurse -Include '*.hlsl', '*.h')
{
    Remove-Item $file
}
