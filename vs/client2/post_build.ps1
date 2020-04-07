# Parameters are passed without quotes
Param(
  [String]$Platform,
  [String]$Configuration,
  [String]$TargetDir
)

<#
  All the requests related to creating, accessing or writing to PDB files are handled by
  mspdbsrv.exe. Sometimes it is possible that mspdbsrv.exe stays alive even after the build is over.
  In such scenarios, it is safe to add a post build event to kill the mspdbsrv.exe.
#>
$mspdbsrv = 'mspdbsrv'
$ProcessActive = Get-Process $mspdbsrv -ErrorAction SilentlyContinue
if ($null -ne $ProcessActive) {
  Stop-Process -Name $mspdbsrv
}
