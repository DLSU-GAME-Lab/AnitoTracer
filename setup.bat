@echo off
setlocal enabledelayedexpansion

:: --- CONFIGURATION ---
set "VCPKG_DIR=vcpkg"
set "HASH_TO_CHECKOUT=908da3a305a0a8028d9602ab241b433652b3df69"
:: ---------------------

if exist "%VCPKG_DIR%\.git" (
    echo [INFO] vcpkg directory already exists. Skipping download.
    cd /d "%VCPKG_DIR%"
) else (
    echo [1/4] Initializing vcpkg repository...
    if exist "%VCPKG_DIR%" rd /s /q "%VCPKG_DIR%"
    mkdir "%VCPKG_DIR%"
    cd /d "%VCPKG_DIR%"
    git init
    git remote add origin https://github.com/Microsoft/vcpkg.git
    
    echo [2/4] Fetching specific vcpkg hash: %HASH_TO_CHECKOUT%...
    git fetch --depth 1 origin %HASH_TO_CHECKOUT%
    if %ERRORLEVEL% neq 0 (
        echo Error: Failed to fetch target hash.
        exit /b %ERRORLEVEL%
    )
    git checkout FETCH_HEAD
    if %ERRORLEVEL% neq 0 (
        echo Error: Failed to checkout hash.
        exit /b %ERRORLEVEL%
    )
    
    echo [3/4] Running vcpkg bootstrap script...
    call .\bootstrap-vcpkg.bat
    if %ERRORLEVEL% neq 0 (
        echo Error: Bootstrap failed.
        exit /b %ERRORLEVEL%
    )
    echo [4/4] Success! vcpkg is ready to use.
)

echo ===================================================
echo    Clang, CMake, and Ninja Automated Setup
echo ===================================================
echo.

:: Check for Administrative Privileges
net session >nul 2>&1
if %errorLevel% neq 0 (
    echo [ERROR] Please right-click this file and select "Run as administrator".
    echo.
    pause
    exit /b
)

:: FIX 0x8a15005e: Temporarily disable SSL/MSStore certificate pinning
echo Applying WinGet Certificate Error Patch...
winget settings --enable BypassCertificatePinningForMicrosoftStore
echo.

:: 1. Install Tools via Winget forcing the main 'winget' repository source
echo [1/3] Installing LLVM (Clang Compiler)...
winget install LLVM.LLVM --source winget --silent --accept-source-agreements --accept-package-agreements
if %errorLevel% neq 0 echo [WARNING] LLVM install exited with code %errorLevel%
echo.

echo [2/3] Installing CMake...
winget install Kitware.CMake --source winget --silent --accept-source-agreements --accept-package-agreements
if %errorLevel% neq 0 echo [WARNING] CMake install exited with code %errorLevel%
echo.

echo [3/3] Installing Ninja Build System...
winget install Ninja-build.Ninja --source winget --silent --accept-source-agreements --accept-package-agreements
if %errorLevel% neq 0 echo [WARNING] Ninja install exited with code %errorLevel%
echo.

:: Re-enable certificate pinning for system safety
echo Restoring standard certificate configurations...
winget settings --disable BypassCertificatePinningForMicrosoftStore
echo.

:: 2. Dynamically update PATH for the current CMD window session
echo Refreshing PATH environment variables for this window...
for /f "tokens=2*" %%A in ('reg query "HKLM\System\CurrentControlSet\Control\Session Manager\Environment" /v Path') do set "SYS_PATH=%%B"
for /f "tokens=2*" %%A in ('reg query "HKCU\Environment" /v Path') do set "USER_PATH=%%B"
set "PATH=%SYS_PATH%;%USER_PATH%;C:\Program Files\LLVM\bin"

:: 3. Verification Test
echo.
echo ===================================================
echo                Verification Test
echo ===================================================

where clang++ >nul 2>&1
if %errorLevel% equ 0 (
    for /f "delims=" %%A in ('clang++ --version') do (
        echo [SUCCESS] Clang Installed: %%A
        goto :check_cmake
    )
) else (
    echo [FAIL] Clang compiler was not found in PATH.
)

:check_cmake
where cmake >nul 2>&1
if %errorLevel% equ 0 (
    for /f "delims=" %%A in ('cmake --version') do (
        echo [SUCCESS] CMake Installed: %%A
        goto :check_ninja
    )
) else (
    echo [FAIL] CMake was not found in PATH.
)

:check_ninja
where ninja >nul 2>&1
if %errorLevel% equ 0 (
    for /f "delims=" %%A in ('ninja --version') do (
        echo [SUCCESS] Ninja Installed: %%A
        goto :end
    )
) else (
    echo [FAIL] Ninja was not found in PATH.
)

:end

echo.
echo ===================================================
echo GIT LFS
echo ===================================================
echo.

echo Changing to script directory for Git LFS operations...
cd /d "%~dp0"

where git >nul 2>&1
if %errorLevel% neq 0 (
    echo [FAIL] Git is not installed or not found in PATH.
    echo Please install Git and ensure it's in your system PATH.
    goto :end
)

echo Checking for Git LFS...
git lfs --version >nul 2>&1
if %errorLevel% neq 0 (
    echo [FAIL] Git LFS is not installed.
    echo Please install Git LFS from https://git-lfs.github.com/
    goto :end_script
)

echo Initializing Git LFS...
call git lfs install

echo Downloading LFS files...
call git lfs pull
if %errorLevel% neq 0 (
    echo [WARNING] 'git lfs pull' failed. This can happen if you need to authenticate.
    echo You may need to run 'git lfs pull' manually in a terminal.
)


echo ===================================================
echo Setup complete. Please RESTART VS Code before usage!
echo ===================================================
echo.
:end_script
pause