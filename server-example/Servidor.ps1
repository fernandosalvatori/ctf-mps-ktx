param([ValidateSet('iniciar','parar','status')][string]$Acao='status')
$ErrorActionPreference='Stop'
$serverRoot=[IO.Path]::GetFullPath($PSScriptRoot)
$serverExe=Join-Path $serverRoot 'mvdsv.exe'
$statePath=Join-Path $serverRoot 'logs\processo.json'
$port=27561
function Get-OwnedServer {
    if (-not (Test-Path -LiteralPath $statePath)) { return $null }
    $saved=Get-Content -LiteralPath $statePath -Raw | ConvertFrom-Json
    $proc=Get-Process -Id $saved.pid -ErrorAction SilentlyContinue
    if ($proc -and $proc.Path -eq $serverExe -and $proc.StartTime.ToUniversalTime().Ticks.ToString() -eq $saved.startTicks) { return $proc }
    return $null
}
function Read-ServerStatus {
    $udp=[Net.Sockets.UdpClient]::new()
    try {
        $udp.Client.ReceiveTimeout=1800
        $packet=[byte[]](255,255,255,255)+[Text.Encoding]::ASCII.GetBytes("status`n")+[byte[]](0)
        [void]$udp.Send($packet,$packet.Length,'127.0.0.1',$port)
        $endpoint=[Net.IPEndPoint]::new([Net.IPAddress]::Any,0)
        $response=$udp.Receive([ref]$endpoint)
        return [Text.Encoding]::GetEncoding(28591).GetString($response,5,$response.Length-5).Trim([char]0)
    } finally { $udp.Dispose() }
}
$owned=Get-OwnedServer
if ($Acao -eq 'iniciar') {
    if ($owned) { Write-Host "CTFNormal KTX ja iniciado (PID $($owned.Id))."; exit 0 }
    if (-not (Test-Path -LiteralPath $serverExe)) { throw 'mvdsv.exe ausente.' }
    $occupied=Get-NetUDPEndpoint -LocalPort $port -ErrorAction SilentlyContinue
    if ($occupied) { throw "Porta UDP $port ocupada; nenhum processo foi alterado." }
    # KTX reads these presets inline; keep one editable source of game rules.
    $rulesPath=Join-Path $serverRoot 'ktx\rules.cfg'
    Copy-Item -LiteralPath $rulesPath -Destination (Join-Path $serverRoot 'ktx\configs\usermodes\default.cfg') -Force
    Copy-Item -LiteralPath $rulesPath -Destination (Join-Path $serverRoot 'ktx\configs\usermodes\matchless\ctf.cfg') -Force
    $argsList=@('-basedir',('"'+$serverRoot+'"'),'-game','ktx','-port',"$port",'+exec','server.cfg','+map','e1m1','+logfile')
    $owned=Start-Process -FilePath $serverExe -ArgumentList $argsList -WorkingDirectory $serverRoot -WindowStyle Hidden -PassThru
    Start-Sleep -Seconds 2
    $owned.Refresh()
    if ($owned.HasExited) { throw "MVDSV encerrou com codigo $($owned.ExitCode). Consulte qconsole*.log." }
    @{pid=$owned.Id;startTicks=$owned.StartTime.ToUniversalTime().Ticks.ToString();exe=$serverExe;port=$port} | ConvertTo-Json | Set-Content -LiteralPath $statePath -Encoding UTF8
    Write-Host "CTFNormal KTX iniciado. No ezQuake: connect 127.0.0.1:$port"
    Write-Output (Read-ServerStatus)
} elseif ($Acao -eq 'parar') {
    if (-not $owned) { Write-Host 'CTFNormal KTX ja esta parado ou nao foi iniciado por este launcher.'; exit 0 }
    Stop-Process -Id $owned.Id
    $owned.WaitForExit(5000) | Out-Null
    Remove-Item -LiteralPath $statePath -ErrorAction SilentlyContinue
    Write-Host 'CTFNormal KTX parado. Nenhum outro servidor foi alterado.'
} else {
    if (-not $owned) { Write-Host 'CTFNormal KTX parado (sem processo deste launcher).'; exit 0 }
    Write-Host "CTFNormal KTX ativo, PID $($owned.Id), UDP $port."
    Write-Output (Read-ServerStatus)
}
