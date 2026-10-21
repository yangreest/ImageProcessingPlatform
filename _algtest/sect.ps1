$ErrorActionPreference = 'Stop'
$path = $args[0]
$pat  = $args[1]
$take = [int]$args[2]
$l = @(Get-Content $path | Where-Object { $_ -match '^\[T\]' -and $_ -notmatch '>>> RUN|<<< END' })
$s = -1
for ($i = 0; $i -lt $l.Count; $i++) { if ($l[$i] -match $pat) { $s = $i; break } }
if ($s -lt 0) { Write-Output "NOT FOUND: $pat"; exit 1 }
$e = [Math]::Min($l.Count - 1, $s + $take - 1)
for ($i = $s; $i -le $e; $i++) { Write-Output $l[$i] }
