@echo off
setlocal

:: Create a popup window for the commit message
for /f "delims=" %%I in ('powershell -Command "Add-Type -AssemblyName Microsoft.VisualBasic; [Microsoft.VisualBasic.Interaction]::InputBox('Enter your commit message:', 'GitHub Upload')"') do set msg=%%I

:: Check if the user cancelled or left it empty
if "%msg%"=="" (
    echo Upload cancelled: No message provided.
    pause
    exit /b
)

echo.
echo --- STAGING CHANGES ---
git add .

echo.
echo --- COMMITTING: %msg% ---
git commit -m "%msg%"

echo.
echo --- PUSHING TO GITHUB ---
git push

echo.
echo --- DONE ---
pause