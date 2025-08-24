@echo off
:: Requires shadercross CLI installed from SDL_shadercross

echo Running rebuild shaders...

:: Process .vert.hlsl files
for %%f in (*.vert.hlsl) do (
    @REM echo "processing: %%~nf" 
    if exist "%%f" (
        shadercross "%%f" -o "../compiled/SPIRV/%%~nf.spv"
        shadercross "%%f" -o "../compiled/MSL/%%~nf.msl"
        shadercross "%%f" -o "../compiled/DXIL/%%~nf.dxil"
    )
)

:: Process .frag.hlsl files
for %%f in (*.frag.hlsl) do (
    if exist "%%f" (
        shadercross "%%f" -o "../compiled/SPIRV/%%~nf.spv"
        shadercross "%%f" -o "../compiled/MSL/%%~nf.msl"
        shadercross "%%f" -o "../compiled/DXIL/%%~nf.dxil"
    )
)

:: Process .comp.hlsl files
for %%f in (*.comp.hlsl) do (
    if exist "%%f" (
        shadercross "%%f" -o "../compiled/SPIRV/%%~nf.spv"
        shadercross "%%f" -o "../compiled/MSL/%%~nf.msl"
        shadercross "%%f" -o "../compiled/DXIL/%%~nf.dxil"
    )
)

:: success exit code
exit /b 0