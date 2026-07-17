@echo off
setlocal enabledelayedexpansion

:: --- CONFIGURATION ---
set "VCPKG_DIR=vcpkg"
set "HASH_TO_CHECKOUT=908da3a305a0a8028d9602ab241b433652b3df69"
:: ---------------------

echo [1/4] Cloning vcpkg repository...
git clone https://github.com/Microsoft/vcpkg.git %VCPKG_DIR%
if %ERRORLEVEL% neq 0 (
    echo Error: Failed to clone repository.
    exit /b %ERRORLEVEL%
)

cd /d %VCPKG_DIR%

echo [2/4] Checking out specific hash: %HASH_TO_CHECKOUT%...
git checkout %HASH_TO_CHECKOUT%
if %ERRORLEVEL% neq 0 (
    echo Error: Failed to checkout hash.
    exit /b %ERRORLEVEL%
)

echo [3/4] Running bootstrap script...
call .\bootstrap-vcpkg.bat
if %ERRORLEVEL% neq 0 (
    echo Error: Bootstrap failed.
    exit /b %ERRORLEVEL%
)

echo [4/4] Success! vcpkg is ready to use.
pause