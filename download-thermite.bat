@echo off
:: ============================================================
:: Thermite Release Downloader
:: Generated with AI assistance
:: Model: Claude Opus 4.5 (Anthropic)
:: Date of first prompt: February 3, 2026
:: First prompt: "what permission for personal access tokens 
::               are needed to download via script the releases?"
:: ============================================================

setlocal enabledelayedexpansion

set GITHUB_TOKEN=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
set REPO=BredaUniversityGames/thermite

set SCRIPT_NAME=%~nx0
set FILENAME=thermite-Developer-Packaging.zip


echo ========================================
echo Thermite Release Downloader
echo ========================================
echo.

echo Fetching latest release info...
curl -s -H "Authorization: Bearer %GITHUB_TOKEN%" ^
  "https://api.github.com/repos/%REPO%/releases" -o releases.json

:: Extract tag_name (version)
for /f "tokens=2 delims=:," %%a in ('findstr /i "tag_name" releases.json') do (
    set "VERSION=%%~a"
    set "VERSION=!VERSION:"=!"
    set "VERSION=!VERSION: =!"
    goto :getasset
)

:getasset
:: Extract the asset ID (number after /assets/)
for /f "tokens=*" %%a in ('findstr /C:"releases/assets" releases.json') do (
    set "line=%%a"
    set "line=!line:*assets/=!"
    for /f "tokens=1 delims=," %%b in ("!line!") do (
        set "ASSET_ID=%%b"
        set "ASSET_ID=!ASSET_ID:"=!"
        goto :download
    )
)

:download
del releases.json

if "%ASSET_ID%"=="" (
    echo Could not find asset ID!
    pause
    exit /b 1
)

echo Found release: %VERSION%
echo Asset ID: %ASSET_ID%
echo Downloading %FILENAME%...
echo.

curl -L ^
  -H "Authorization: Bearer %GITHUB_TOKEN%" ^
  -H "Accept: application/octet-stream" ^
  -o "%FILENAME%" ^
  "https://api.github.com/repos/%REPO%/releases/assets/%ASSET_ID%" ^
  --progress-bar

if not exist "%FILENAME%" (
    echo Download failed!
    pause
    exit /b 1
)

echo.
echo Cleaning old files (keeping assets folder and this script)...

for %%f in (*) do (
    if /i not "%%f"=="%SCRIPT_NAME%" if /i not "%%f"=="%FILENAME%" del "%%f"
)

for /d %%d in (*) do (
    if /i not "%%d"=="assets" rmdir /s /q "%%d"
)

echo Extracting %FILENAME%...
tar -xf "%FILENAME%"

del "%FILENAME%"

echo.
echo ========================================
echo Done! Thermite %VERSION% installed.
echo ========================================
pause