@echo off

set CONFIG_COMMAND=cmake --no-warn-unused-cli -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=TRUE -DCMAKE_BUILD_TYPE:STRING=Release -S . -B build -G Ninja
set BUILD_COMMAND=cmake --build build --config Release --target GameDLL
set PROJECT=C:\Programming\c++_game_engine_2
set DLL_SRC=%PROJECT%\build\game\src\dll_boundary\libGameDLL.dll
set DLL_DST=%PROJECT%\build\game\src\engine\libGameDLL.dll
set MSYS2_PATH=C:\Software\msys64

:: Run configuration and build in single shell session
echo Running configuration command...
call "%MSYS2_PATH%\msys2_shell.cmd" -defterm -where %PROJECT% -no-start -ucrt64 -shell bash -c '%BUILD_COMMAND% || exit 1'

echo Done building...

:: Verify build succeeded before copying
if errorlevel 1 (
    echo Build failed, not copying DLL
    exit /b 1
)

:: Copy DLL - adjust paths as needed
set COPY_CMD=xcopy /Y "%DLL_SRC%" "%DLL_DST%"

echo DLL_SRC %DLL_SRC%
echo DLL_DST %DLL_DST%
echo %COPY_CMD%

if exist %DLL_SRC% (
  echo F | %COPY_CMD% > nul
  echo Done copying dll...
) else (
  echo DLL not found at %DLL_SRC%
  exit /b 1
)

@REM success
exit /b 0