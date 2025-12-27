@echo off
setlocal

:: 1. Get the commit message via Popup
for /f "delims=" %%I in ('powershell -Command "Add-Type -AssemblyName Microsoft.VisualBasic; [Microsoft.VisualBasic.Interaction]::InputBox('Enter your commit message for V2:', 'GitHub Upload', 'feat: sync updated logic from V2 source') "') do set msg=%%I

if "%msg%"=="" (
    echo Upload cancelled.
    pause
    exit /b
)

echo.
echo --- 1. INITIALIZING REPO ---
:: Initialize if not already a git repo
if not exist ".git" (
    git init
    git remote add origin https://github.com/Adriannpropp/ImageToBlocks.git
)

:: Force branch to main
git branch -M main

echo.
echo --- 2. STAGING AND COMMITTING LOCAL CHANGES ---
git add .
git commit -m "%msg%"

echo.
echo --- 3. SYNCING FROM GITHUB ---
:: We use --allow-unrelated-histories because 'V2' is a new folder
:: We use -X theirs to favor the remote if there are simple conflicts
git pull origin main --allow-unrelated-histories --no-rebase -X theirs

echo.
echo --- 4. PUSHING EVERYTHING TO GITHUB ---
:: '-f' (force) is the "I am the boss" button. 
:: Use this if you want the 'V2' folder to completely replace what is on GitHub.
set /p FORCE="Force push to overwrite GitHub with V2 code? (y/n): "
if /i "%FORCE%"=="y" (
    git push -u origin main --force
) else (
    git push -u origin main
)

echo.
echo --- DONE ---
echo V2 code is now live!
pause