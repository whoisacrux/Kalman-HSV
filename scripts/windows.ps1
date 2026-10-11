param(
    [string]$OpenCVRoot = "C:\opencv",
    [ValidateRange(0,99)][int]$Camera = 0,
    [switch]$Demo,
    [switch]$Simulation,
    [switch]$Motion
)
$ErrorActionPreference = "Stop"
$projectRoot = Split-Path -Parent $PSScriptRoot
Set-Location $projectRoot
try {
    if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
        throw "CMake bulunamadi. CMake kurulumunda 'Add CMake to PATH' secin; terminali yeniden acin."
    }
    $candidates = @($env:OpenCV_DIR, (Join-Path $OpenCVRoot 'build'), $OpenCVRoot)
    $configDir = $null
    foreach ($candidate in $candidates) {
        if ($candidate -and (Test-Path (Join-Path $candidate 'OpenCVConfig.cmake'))) {
            $configDir = $candidate
            break
        }
    }
    if (-not $configDir) {
        $entered = Read-Host 'OpenCVConfig.cmake dosyasinin bulundugu klasor (ornek C:\opencv\build)'
        if (Test-Path (Join-Path $entered 'OpenCVConfig.cmake')) { $configDir = $entered }
        else { throw 'OpenCV bulunamadi. README.md icindeki Windows kurulumunu tamamlayin.' }
    }
    $buildDir = Join-Path $projectRoot 'build-windows'
    & cmake -S $projectRoot -B $buildDir -G 'Visual Studio 17 2022' -A x64 "-DOpenCV_DIR=$configDir"
    if ($LASTEXITCODE -ne 0) { throw 'CMake yapilandirmasi basarisiz. Visual Studio 2022 C++ is yukunu kontrol edin.' }
    & cmake --build $buildDir --config Release --parallel
    if ($LASTEXITCODE -ne 0) { throw 'Derleme basarisiz.' }
    & ctest --test-dir $buildDir -C Release --output-on-failure
    if ($LASTEXITCODE -ne 0) { throw 'Kalman veya hareket simulasyonu testleri basarisiz.' }
    $outputDir = Join-Path $buildDir 'Release'
    # The official Windows package uses build/x64/vc16/bin (or a newer vc folder).
    $searchRoot = $configDir
    if ((Split-Path $configDir -Leaf) -eq 'lib') { $searchRoot = Split-Path $configDir -Parent }
    $runtimeDlls = @(Get-ChildItem -Path $searchRoot -Recurse -Filter 'opencv_world*.dll' |
        Where-Object { $_.Name -notmatch 'd\.dll$' -and $_.FullName -match '[\\/]x64[\\/]' })
    if ($runtimeDlls.Count -eq 0) {
        throw 'OpenCV x64 runtime DLL bulunamadi. Resmi Windows paketini kullanin ve OpenCVRoot yolunu kontrol edin.'
    }
    $runtimeDir = $runtimeDlls[0].DirectoryName
    Get-ChildItem -Path $runtimeDir -Filter '*.dll' | Copy-Item -Destination $outputDir -Force
    $appArgs = @('--camera', "$Camera")
    if ($Demo) { $appArgs += '--demo' }
    if ($Simulation) { $appArgs += '--simulation' }
    if ($Motion) { $appArgs += '--motion' }
    Write-Host 'Uygulama aciliyor. 1: HSV / 2: Kalman / 3: 2D hareket / ESC: cikis' -ForegroundColor Cyan
    & (Join-Path $outputDir 'hsv_kalman_lab.exe') @appArgs
    if ($LASTEXITCODE -ne 0) { throw 'Uygulama hata ile kapandi.' }
} catch {
    Write-Host $_.Exception.Message -ForegroundColor Red
    exit 1
}
