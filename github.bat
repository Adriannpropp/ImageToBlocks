@echo off
setlocal

:: Create a popup window for the commit message
for /f "delims=" %%I in ('powershell -Command "Add-Type -AssemblyName Microsoft.VisualBasic; [Microsoft.VisualBasic.Interaction]::InputBox('Enter your commit message:', 'GitHub Upload')"') do set msg=%%I

if "%msg%"=="" (
    echo Upload cancelled.
    pause
    exit /b
)

echo.
echo --- SYNCING WITH GITHUB (PULL) ---
:: This fetches remote changes and merges them
git pull origin main --rebase

echo.
echo --- STAGING CHANGES ---
git add .

echo.
echo --- COMMITTING: %msg% ---
git commit -m "%msg%"

echo.
echo --- PUSHING TO GITHUB ---
git push origin main

echo.
echo --- DONE ---
pause