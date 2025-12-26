@echo off
setlocal enabledelayedexpansion

:: SET YOUR PATHS HERE
set "CLI_PATH=C:\geode-sdk\geode.exe"
set "SDK_PATH=C:\GeodeSDK"

echo [1/5] Checking Prerequisites...
if not exist "!CLI_PATH!" (
    echo [ERROR] Geode CLI not found at !CLI_PATH!
    echo Please check your README and fix the CLI_PATH in this script.
    pause
    exit /b 1
)
echo [OK] Found CLI at !CLI_PATH!

:: ---------------------------------------------------------
:: 2. Reinstall SDK Logic
:: ---------------------------------------------------------
echo.
echo [ATTENTION] Mismatched SDKs cause LNK2001 errors.
set /p REINSTALL="Reinstall Geode SDK at !SDK_PATH!? (y/n): "
if /i "!REINSTALL!"=="y" (
    echo [2/5] Cleaning old SDK folder...
    if exist "!SDK_PATH!" rd /s /q "!SDK_PATH!"
    
    echo [2/5] Running: !CLI_PATH! sdk install "!SDK_PATH!"
    "!CLI_PATH!" sdk install "!SDK_PATH!"
    
    if !ERRORLEVEL! NEQ 0 (
        echo [ERROR] SDK Installation failed.
        pause
        exit /b !ERRORLEVEL!
    )
)

:: ---------------------------------------------------------
:: 3. Build Logic
:: ---------------------------------------------------------
echo [3/5] Cleaning Build Folder...
if exist build rd /s /q build
mkdir build
cd build

echo [4/5] Configuring CMake...
cmake .. -A x64 -T host=x64 ^
    -DGEODE_SDK="!SDK_PATH!" ^
    -DCMAKE_BUILD_TYPE=RelWithDebInfo

if !ERRORLEVEL! NEQ 0 (
    echo [ERROR] CMake configuration failed.
    pause
    exit /b !ERRORLEVEL!
)

echo [5/5] Building...
cmake --build . --config RelWithDebInfo

if !ERRORLEVEL! NEQ 0 (
    echo [ERROR] Build failed. Read the red text above.
    pause
    exit /b !ERRORLEVEL!
)

echo.
echo [SUCCESS] Mod built!

:: ---------------------------------------------------------
:: FIXED: Search for .geode file in multiple locations
:: ---------------------------------------------------------
set "MOD_DEST=%LocalAppData%\GeometryDash\geode\mods"
set "GEODE_FILE="

echo.
echo [Searching for .geode file...]

:: Check common build output locations
if exist "RelWithDebInfo\*.geode" (
    set "GEODE_FILE=RelWithDebInfo\*.geode"
    echo [FOUND] In RelWithDebInfo\
) else if exist "*.geode" (
    set "GEODE_FILE=*.geode"
    echo [FOUND] In build root
) else if exist "..\*.geode" (
    set "GEODE_FILE=..\*.geode"
    echo [FOUND] In project root
) else (
    echo [ERROR] Could not find .geode file!
    echo.
    echo Searching all files...
    dir /s /b *.geode
    echo.
    echo If you see your .geode file above, note its location.
    pause
    exit /b 1
)

:: Copy the file
if defined GEODE_FILE (
    echo [Copying to GD mods folder...]
    if not exist "!MOD_DEST!" mkdir "!MOD_DEST!"
    copy /Y "!GEODE_FILE!" "!MOD_DEST!\"
    
    if !ERRORLEVEL! EQU 0 (
        echo.
        echo ========================================
        echo [SUCCESS] Mod installed successfully!
        echo Location: !MOD_DEST!
        echo ========================================
    ) else (
        echo [ERROR] Copy failed!
    )
)

echo.
pause