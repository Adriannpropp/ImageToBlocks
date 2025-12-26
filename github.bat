@echo off
setlocal

:: 1. Get the commit message via Popup
for /f "delims=" %%I in ('powershell -Command "Add-Type -AssemblyName Microsoft.VisualBasic; [Microsoft.VisualBasic.Interaction]::InputBox('Enter your commit message:', 'GitHub Upload')"') do set msg=%%I

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
:: Now that your local work is committed, Git will allow the pull.
:: This will merge GitHub's files (like the README) with your local code.
git pull origin main --no-rebase

echo.
echo --- 3. PUSHING EVERYTHING TO GITHUB ---
git push origin main

echo.
echo --- DONE ---
pause