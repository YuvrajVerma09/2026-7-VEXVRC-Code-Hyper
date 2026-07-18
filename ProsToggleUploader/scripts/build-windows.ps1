param(
    [Parameter(Mandatory = $true)]
    [string]$QtPrefix
)

$ErrorActionPreference = "Stop"
cmake -S . -B build -G Ninja -DCMAKE_PREFIX_PATH="$QtPrefix"
cmake --build build
Write-Host "Built: build\ProsToggleUploader.exe"
