@echo off
echo ============================================
echo Geode Mod Setup Verification
echo ============================================
echo.

REM Check Geode CLI
echo [1/5] Checking Geode CLI...
if exist "C:\geode-sdk\geode.exe" (
    echo [OK] Geode CLI found at C:\geode-sdk\geode.exe
    C:\geode-sdk\geode.exe --version
) else (
    echo [ERROR] Geode CLI not found at C:\geode-sdk\geode.exe
    echo Please install Geode CLI or update the path
)
echo.

REM Check Geode SDK
echo [2/5] Checking Geode SDK...
if exist "C:\GeodeSDK" (
    echo [OK] Geode SDK found at C:\GeodeSDK
) else (
    echo [ERROR] Geode SDK not found at C:\GeodeSDK
    echo Please clone the SDK from: https://github.com/geode-sdk/geode
)
echo.

REM Check GEODE_SDK environment variable
echo [3/5] Checking GEODE_SDK environment variable...
if defined GEODE_SDK (
    echo [OK] GEODE_SDK is set to: %GEODE_SDK%
) else (
    echo [WARNING] GEODE_SDK environment variable not set
    echo Run: setx GEODE_SDK "C:\GeodeSDK" /M
    echo Then restart this terminal
)
echo.

REM Check CMake
echo [4/5] Checking CMake...
cmake --version >nul 2>&1
if %errorlevel% equ 0 (
    echo [OK] CMake is available
    cmake --version
) else (
    echo [WARNING] CMake not found in PATH
    echo CMake should be included with Geode CLI
)
echo.

REM Check Visual Studio
echo [5/5] Checking Visual Studio...
where cl.exe >nul 2>&1
if %errorlevel% equ 0 (
    echo [OK] Visual Studio compiler found
    cl.exe 2>&1 | findstr "Version"
) else (
    echo [WARNING] Visual Studio compiler not in PATH
    echo Please run this from "x64 Native Tools Command Prompt for VS 2022"
    echo Or install Visual Studio 2022 with C++ desktop development
)
echo.

echo ============================================
echo Verification Complete
echo ============================================
echo.

REM Check mod files
echo Checking mod files...
if exist "main.cpp" (
    echo [OK] main.cpp found
) else (
    echo [ERROR] main.cpp not found
)

if exist "mod.json" (
    echo [OK] mod.json found
) else (
    echo [ERROR] mod.json not found
)

if exist "CMakeLists.txt" (
    echo [OK] CMakeLists.txt found
) else (
    echo [ERROR] CMakeLists.txt not found
)
echo.

echo If all checks passed, run: build.bat
echo.
pause
