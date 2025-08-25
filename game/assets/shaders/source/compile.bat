@echo off
:: Requires shadercross CLI installed from SDL_shadercross

setlocal enabledelayedexpansion
set FAILED=0

echo Running rebuild shaders...

:: Process .vert.hlsl files
for %%f in (*.vert.hlsl) do (
    if exist "%%f" (
        shadercross "%%f" -o "../compiled/SPIRV/%%~nf.spv"
        if errorlevel 1 set FAILED=1
        shadercross "%%f" -o "../compiled/MSL/%%~nf.msl"
        if errorlevel 1 set FAILED=1
        shadercross "%%f" -o "../compiled/DXIL/%%~nf.dxil"
        if errorlevel 1 set FAILED=1
    )
)

:: Process .frag.hlsl files
for %%f in (*.frag.hlsl) do (
    if exist "%%f" (
        shadercross "%%f" -o "../compiled/SPIRV/%%~nf.spv"
        if errorlevel 1 set FAILED=1
        shadercross "%%f" -o "../compiled/MSL/%%~nf.msl"
        if errorlevel 1 set FAILED=1
        shadercross "%%f" -o "../compiled/DXIL/%%~nf.dxil"
        if errorlevel 1 set FAILED=1
    )
)

:: Process .comp.hlsl files
for %%f in (*.comp.hlsl) do (
    if exist "%%f" (
        shadercross "%%f" -o "../compiled/SPIRV/%%~nf.spv"
        if errorlevel 1 set FAILED=1
        shadercross "%%f" -o "../compiled/MSL/%%~nf.msl"
        if errorlevel 1 set FAILED=1
        shadercross "%%f" -o "../compiled/DXIL/%%~nf.dxil"
        if errorlevel 1 set FAILED=1
    )
)

if "!FAILED!"=="1" (
    exit /b 1
) else (
    exit /b 0
)