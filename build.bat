@echo off
setlocal enabledelayedexpansion

:: SET YOUR PATHS HERE
set "CLI_PATH=C:\geode-sdk\geode.exe"
set "SDK_PATH=C:\GeodeSDK"
<<<<<<< HEAD
set "MOD_DEST=%LocalAppData%\GeometryDash\geode\mods"

echo [1/5] Checking Prerequisites...

:: Check CLI
if not exist "!CLI_PATH!" (
    echo [ERROR] Geode CLI not found at !CLI_PATH!
    pause
    exit /b 1
)

:: Check Source Folder
if not exist "..\src" (
    if not exist "src" (
        echo [ERROR] 'src' folder not found. Are you running this from the project root?
        pause
        exit /b 1
    )
)

:: Check for stb_image.h (required for the code I gave you)
if not exist "src\stb_image.h" if not exist "..\src\stb_image.h" (
    echo [WARNING] stb_image.h not found in src! Build will likely fail.
)

echo [OK] Prerequisites met.
=======

echo [1/5] Checking Prerequisites...
if not exist "!CLI_PATH!" (
    echo [ERROR] Geode CLI not found at !CLI_PATH!
    echo Please check your README and fix the CLI_PATH in this script.
    pause
    exit /b 1
)
echo [OK] Found CLI at !CLI_PATH!
>>>>>>> f9748aae7b869712d450ac448ae08cd3401f5ab9

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
<<<<<<< HEAD
echo [SUCCESS] Build process finished.
=======
echo [SUCCESS] Mod built!
>>>>>>> f9748aae7b869712d450ac448ae08cd3401f5ab9

:: ---------------------------------------------------------
:: FIXED: Search for .geode file in multiple locations
:: ---------------------------------------------------------
<<<<<<< HEAD
=======
set "MOD_DEST=%LocalAppData%\GeometryDash\geode\mods"
>>>>>>> f9748aae7b869712d450ac448ae08cd3401f5ab9
set "GEODE_FILE="

echo.
echo [Searching for .geode file...]

<<<<<<< HEAD
:: Check common build output locations including src/build variants
=======
:: Check common build output locations
>>>>>>> f9748aae7b869712d450ac448ae08cd3401f5ab9
if exist "RelWithDebInfo\*.geode" (
    set "GEODE_FILE=RelWithDebInfo\*.geode"
    echo [FOUND] In RelWithDebInfo\
) else if exist "*.geode" (
    set "GEODE_FILE=*.geode"
    echo [FOUND] In build root
<<<<<<< HEAD
) else if exist "..\src\*.geode" (
    set "GEODE_FILE=..\src\*.geode"
    echo [FOUND] In src folder
=======
>>>>>>> f9748aae7b869712d450ac448ae08cd3401f5ab9
) else if exist "..\*.geode" (
    set "GEODE_FILE=..\*.geode"
    echo [FOUND] In project root
) else (
    echo [ERROR] Could not find .geode file!
<<<<<<< HEAD
    echo Searching all subdirectories...
    dir /s /b ..\*.geode
=======
    echo.
    echo Searching all files...
    dir /s /b *.geode
    echo.
    echo If you see your .geode file above, note its location.
>>>>>>> f9748aae7b869712d450ac448ae08cd3401f5ab9
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
<<<<<<< HEAD
        echo [ERROR] Copy failed! Check if GD is running and locking the file.
=======
        echo [ERROR] Copy failed!
>>>>>>> f9748aae7b869712d450ac448ae08cd3401f5ab9
    )
)

echo.
pause