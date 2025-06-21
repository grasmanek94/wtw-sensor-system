# Load config from JSON
$configPath = "data/config.json"
$config = Get-Content $configPath | ConvertFrom-Json

# Extract values
$baseIp = $config.static_ip
$username = $config.auth_user
$password = $config.auth_pw

Write-Host "Base IP: $baseIp"
Write-Host "Username: $username"
Write-Host "Password: $password"

# Fix: wrap variables in ${} to avoid colon parsing issue
$authHeader = "Basic " + [Convert]::ToBase64String([Text.Encoding]::ASCII.GetBytes("${username}:${password}"))

# Parameters
$steps = 17280
$pi = [Math]::PI
$baseUri = "http://$baseIp/update"

Write-Host "BaseUri: $baseUri"

for ($i = 0; $i -lt $steps; $i++) {
    $angle = 2 * $pi * $i / $steps

    $seqnr = $i
    $loc = 0
    $rh = [Math]::Round(55 + 25 * [Math]::Sin($angle), 1)         # 30–80
    $temp = [Math]::Round(22.5 + 7.5 * [Math]::Sin($angle), 2)    # 15.00–30.00
    $co2 = [Math]::Round(800 + 400 * [Math]::Sin($angle))         # 400–1200
    $status = 0

    $uri = "${baseUri}?seqnr=$seqnr&deviceId=00000000&loc=$loc&rh[0]=$rh&temp[0]=$temp&co2[0]=$co2&status[0]=$status"

    Write-Host "Sending: $uri"
    Invoke-WebRequest -Uri $uri -Method POST -Headers @{ Authorization = $authHeader }
}
