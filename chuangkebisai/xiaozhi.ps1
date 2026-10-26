param(
    [ValidateSet("build", "flash", "monitor", "flash-monitor")]
    [string]$Action = "build",
    [string]$Port = "COM5",
    [int]$Jobs = 8
)

$ErrorActionPreference = "Stop"

$ProjectDir = Join-Path $PSScriptRoot "main\xiaozhi-esp32-2.2.4"
$BuildDir = Join-Path $ProjectDir "build"
$CMake = "C:\Espressif\tools\cmake\3.30.2\bin\cmake.exe"
$PythonEnv = "C:\Espressif\tools\python\v5.5.4\venv"
$Python = Join-Path $PythonEnv "Scripts\python.exe"
$NinjaDir = "C:\Espressif\tools\ninja\1.12.1"
$ToolchainDir = "C:\Espressif\tools\xtensa-esp-elf\esp-14.2.0_20260121\xtensa-esp-elf\bin"
$CMakeDir = Split-Path $CMake -Parent

$env:IDF_PATH = "D:\esp32\v5.5.4\esp-idf"
$env:IDF_PYTHON_ENV_PATH = $PythonEnv
$env:IDF_PYTHON_CHECK_CONSTRAINTS = "0"
$env:ESP_ROM_ELF_DIR = "C:\Espressif\tools\esp-rom-elfs\20241011"
$env:PATH = "$PythonEnv\Scripts;$CMakeDir;$NinjaDir;$ToolchainDir;$env:PATH"

function Assert-PathExists {
    param([string]$Path, [string]$Name)

    if (-not (Test-Path -LiteralPath $Path)) {
        throw "$Name not found: $Path"
    }
}

function Configure-XiaoZhi {
    Assert-PathExists $ProjectDir "XiaoZhi project"
    Assert-PathExists $CMake "CMake"
    Assert-PathExists $Python "ESP-IDF Python"

    if (-not (Test-Path -LiteralPath (Join-Path $BuildDir "build.ninja"))) {
        & $CMake `
            -S $ProjectDir `
            -B $BuildDir `
            -G Ninja `
            -DIDF_TARGET=esp32s3 `
            '-DSDKCONFIG_DEFAULTS=sdkconfig.defaults;sdkconfig.defaults.esp32s3;sdkconfig.defaults.chuangkebisai'
    }
}

function Build-XiaoZhi {
    Configure-XiaoZhi
    & $CMake --build $BuildDir -- -j $Jobs
}

function Invoke-Idf {
    param([string[]]$IdfArgs)

    Assert-PathExists $Python "ESP-IDF Python"
    & $Python (Join-Path $env:IDF_PATH "tools\idf.py") -C $ProjectDir @IdfArgs
}

switch ($Action) {
    "build" {
        Build-XiaoZhi
    }
    "flash" {
        Build-XiaoZhi
        Invoke-Idf @("-p", $Port, "flash")
    }
    "monitor" {
        Invoke-Idf @("-p", $Port, "monitor")
    }
    "flash-monitor" {
        Build-XiaoZhi
        Invoke-Idf @("-p", $Port, "flash", "monitor")
    }
}
