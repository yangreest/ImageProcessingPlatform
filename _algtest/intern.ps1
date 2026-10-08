$ErrorActionPreference = 'Stop'
$path = $args[0]
$namePat = $args[1]
$lines = Get-Content $path
$cur = $null
$rows = @{}
$order = @()
foreach ($ln in $lines) {
    if ($ln -match '^\[T\] >>> RUN (\S+)') { $cur = $Matches[1]; if (-not $rows.ContainsKey($cur)) { $rows[$cur] = @{}; $order += $cur }; continue }
    if ($ln -match '^\[T\] <<< END') { $cur = $null; continue }
    if ($null -eq $cur) { continue }
    if ($ln -match 'noise0\s*:\s*([-+0-9.eE]+)') { $rows[$cur]['noise0'] = $Matches[1] }
    if ($ln -match 'min/max/layer0\s*:\s*([-+0-9.eE]+)\s*/\s*([-+0-9.eE]+)') { $rows[$cur]['l0min'] = $Matches[1]; $rows[$cur]['l0max'] = $Matches[2] }
    if ($ln -match 'min/max/mean_window\s*:\s*([-+0-9.eE]+)\s*[,/]\s*([-+0-9.eE]+)') { $rows[$cur]['mwmin'] = $Matches[1]; $rows[$cur]['mwmax'] = $Matches[2] }
    if ($ln -match 'min/max/sdev_image\s*:\s*([-+0-9.eE]+)\s*[,/]\s*([-+0-9.eE]+)') { $rows[$cur]['sdmin'] = $Matches[1]; $rows[$cur]['sdmax'] = $Matches[2] }
    if ($ln -match '原始像素范围\s*::\s*([-+0-9.eE]+)\s*/\s*([-+0-9.eE]+)') { $rows[$cur]['rawmin'] = $Matches[1]; $rows[$cur]['rawmax'] = $Matches[2] }
    if ($ln -match '增强后像素范围\s*::\s*([-+0-9.ena(ind)+]+)\s*/\s*(\S+)') { $rows[$cur]['enhmin'] = $Matches[1]; $rows[$cur]['enhmax'] = $Matches[2] }
}
"{0,-22} {1,10} {2,10} {3,10} {4,10} {5,10} {6,10} {7,10}" -f 'run','raw_min','raw_max','noise0','layer0_min','layer0_max','sdev_min','sdev_max'
foreach ($k in $order) {
    if ($namePat -and $k -notmatch $namePat) { continue }
    $r = $rows[$k]
    "{0,-22} {1,10} {2,10} {3,10} {4,10} {5,10} {6,10} {7,10}" -f $k, $r['rawmin'], $r['rawmax'], $r['noise0'], $r['l0min'], $r['l0max'], $r['sdmin'], $r['sdmax']
}
