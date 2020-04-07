# This script is called by the post-build event of dll project dependencies
# so the DLLs are copied to the superproject (currently game_client only)

# Parameters are passed without quotes
Param(
  [String]$Platform, # x64/Win32
  [String]$Configuration, # Debug/Release
  [String]$TargetPath     # Absolute path of the DLL file (including name and extension)
)

# Set working directory to location of this script
Push-Location -Path (Split-Path $MyInvocation.MyCommand.Path)

# Copy DLL file
$TargetDir = [IO.Path]::Combine('client2', $Platform, $Configuration)

# Create destination directory if it doesn't exist yet
If (!(Test-Path $TargetDir)) {
  New-Item -ItemType Directory -Force -Path $TargetDir
}

Copy-Item -Force -Path $TargetPath -Destination $TargetDir

Pop-Location
