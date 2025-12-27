@echo off
setlocal

:: 1. Get the commit message via Popup with a professional default
:: I added a default message so you don't have to type much for this specific update.
for /f "delims=" %%I in ('powershell -Command "Add-Type -AssemblyName Microsoft.VisualBasic; [Microsoft.VisualBasic.Interaction]::InputBox('Enter your commit message:', 'GitHub Upload', 'feat: add Greedy Meshing optimization and fix editor crash conflicts') "') do set msg=%%I

if "%msg%"=="" (
    echo Upload cancelled.
    pause
    exit /b
)

echo.
echo --- 1. STAGING AND COMMITTING LOCAL CHANGES ---
git add .
git commit -m "%msg%"

echo.
echo --- 2. SYNCING FROM GITHUB (PULLING REMOTE CHANGES) ---
:: This ensures you get any feedback the Geode admins left on your PR
git pull origin main --no-rebase

echo.
echo --- 3. PUSHING EVERYTHING TO GITHUB ---
git push origin main

echo.
echo --- DONE ---
echo Your optimized, nuke-proof mod is now on GitHub!
pause