$ErrorActionPreference = 'Stop'
$path = $args[0]
$lines = Get-Content $path
$cur = $null
$rows = @()
foreach ($ln in $lines) {
    if ($ln -match '^\[T\] >>> RUN (.+)$') {
        $cur = [ordered]@{ label = $Matches[1]; layer0=''; meanwin=''; sdev=''; noise0=''; ms=''; rawrange=''; enh=''; extra='' }
        continue
    }
    if ($cur -ne $null) {
        if ($ln -match '^min/max/layer0:(.+)$')            { $cur.layer0  = $Matches[1].Trim(); continue }
        if ($ln -match '^min/max/mean_window:(.+)$')       { $cur.meanwin = $Matches[1].Trim(); continue }
        if ($ln -match '^min/max/sdev_image:(.+)$')        { $cur.sdev    = $Matches[1].Trim(); continue }
        if ($ln -match '^noise0:(.+)$')                    { $cur.noise0  = $Matches[1].Trim(); continue }
        if ($ln -match '^原始像素范围::(.+)$')              { $cur.rawrange= $Matches[1].Trim(); continue }
        if ($ln -match '^增强后像素范围::(.+)$')            { $cur.enh     = $Matches[1].Trim(); continue }
        if ($ln -match '^代码运行时间: (.+)$')              { $cur.ms      = $Matches[1].Trim(); continue }
        if ($ln -match '^\[T\] <<< END (.+)$') {
            $cur.extra = $Matches[1].Trim()
            $rows += [pscustomobject]$cur
            $cur = $null
            continue
        }
    }
    if ($ln -match '^\[T\] ' -and $ln -notmatch '^\[T\] >>>') { Write-Output $ln }
}
$rows | Format-Table -AutoSize -Wrap | Out-String -Width 400 | Write-Output
