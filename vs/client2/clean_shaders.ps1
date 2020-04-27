# Path to shader source files
$ShaderPath = "..\..\src\client2\shaders"

foreach ($file in Get-ChildItem -Path $ShaderPath -Recurse -Include '*.hlsl', '*.h')
{
    Remove-Item $file
}
