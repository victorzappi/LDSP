@echo off

rem Before running, you need to set the path to the Android NDK, e.g.:
rem set NDK=C:\Users\%USERNAME%\android-ndk-r25b

goto main

:get_api_level
  rem Convert a human-readable Android version (e.g. 13, 6.0.1, 4.4) into an API level.
  set version_full=%~1

  set major=0
  set minor=0
  set patch=0

  for /f "tokens=1-3 delims=." %%a in ("%version_full%") do (
    set version_major=%%a
    set version_minor=%%b
    set version_patch=%%c
  )
  if "%version_major%" == "1" (
    if "%version_minor%" == "1" exit /b 2
    if "%version_minor%" == "5" exit /b 3
    if "%version_minor%" == "6" exit /b 4
    exit /b 1
  )
  if "%version_major%" == "2" (
    if "%version_minor%" == "0" (
      if "%version_patch%" == "1" exit /b 6
      exit /b 5
    )
    if "%version_minor%" == "1" exit /b 7
    if "%version_minor%" == "2" exit /b 8
    if "%version_minor%" == "3" (
      if "%version_patch%" == "3" exit /b 10
      if "%version_patch%" == "4" exit /b 10
      exit /b 9
    )
  )
  if "%version_major%" == "3" (
    if "%version_minor%" == "1" exit /b 12
    if "%version_minor%" == "2" exit /b 13
    exit /b 11
  )
  if "%version_major%" == "4" (
    if "%version_minor%" == "0" (
      if "%version_patch%" == "3" exit /b 15
      if "%version_patch%" == "4" exit /b 15
      exit /b 14
    )
    if "%version_minor%" == "1" exit /b 16
    if "%version_minor%" == "2" exit /b 17
    if "%version_minor%" == "3" exit /b 18
    if "%version_minor%" == "4" exit /b 19
  )
  rem API level 20 corresponds to Android 4.4W, which isn't relevant to us.
  if "%version_major%" == "5" (
    if "%version_minor%" == "1" exit /b 22
    exit /b 21
  )
  if "%version_major%" == "6" exit /b 23
  if "%version_major%" == "7" (
    if "%version_minor%" == "1" exit /b 25
    exit /b 24
  )
  if "%version_major%" == "8" (
    if "%version_minor%" == "1" exit /b 27
    exit /b 26
  )
  if "%version_major%" == "9" exit /b 28
  if "%version_major%" == "10" exit /b 29
  if "%version_major%" == "11" exit /b 30
  if "%version_major%" == "12" (
    rem Android 12 uses both API_LEVEL 31 and 32
    rem Default to 31 but allow user to say "12.1" to get 32
    if "%version_minor%" == "1" exit /b 32
    exit /b 31
  )
  if "%version_major%" == "13" exit /b 33
  exit /b 0
rem End of :get_api_level

:configure
  rem Configure the LDSP build system to build for the given phone model, Android version, and project path.

  set vendor=%~1
  set model=%~2
  set version=%~3
  set project=%~4

  if "%vendor%" == "" (
    echo Cannot configure: Vendor not specified
    echo Please specify a vendor with --vendor
    exit /b 1
  )

  if "%model%" == "" (
    echo Cannot configure: Model not specified
    echo Please specify a phone model with --model
    exit /b 1
  )

  set hw_config=".\phones\%vendor%\%model%\ldsp_hw_config.json"

  if not exist %hw_config% (
    echo Cannot configure: Hardware config file not found
    echo Please ensure that an ldsp_hw_config.json file exists for "%vendor% %model%"
    exit /b 1
  )

  rem target ABI
  for /f "tokens=2 delims=:" %%i in ('type %hw_config% ^| findstr /r "target architecture"') do set arch=%%i

  set arch=%arch:"=%
  set arch=%arch:,=%
  set arch=%arch: =%

  if "%arch%" == "armv7a" set "abi=armeabi-v7a"
  if "%arch%" == "aarch64" set "abi=arm64-v8a"
  if "%arch%" == "x86" set "abi=x86"
  if "%arch%" == "x86_64" set "abi=x86_64"
  if "%abi%" == "" (
    echo Cannot configure: Unknown target architecture "%arch%
    exit /b 1
  )

  rem target Android version
  call :get_api_level %version%
  set "api_level=%ERRORLEVEL%"
  if "%api_level%" == "0" (
    echo Cannot configure: Unknown Android version "%version%
    exit /b 1
  )

  set "api_define=-DAPI=%api_level%"


  rem support for NEON floating-point unit
  for /f "tokens=2 delims=:" %%i in ('type %hw_config% ^| findstr /r "supports neon floating point unit"') do set neon_setting=%%i

  set neon_setting=%neon_setting:"=%
  set neon_setting=%neon_setting:,=%
  set neon_setting=%neon_setting: =%

  if "%arch%" == "armv7a" (
  if "%neon_setting%" == "true" (
    set "neon=-DANDROID_ARM_NEON=ON"
    set "explicit_neon=-DEXPLICIT_ARM_NEON=1"
  ) else if "%neon_setting%" == "True" (
    set "neon=-DANDROID_ARM_NEON=ON"
    set "explicit_neon=-DEXPLICIT_ARM_NEON=1"
  ) else if "%neon_setting%" == "yes" (
    set "neon=-DANDROID_ARM_NEON=ON"
    set "explicit_neon=-DEXPLICIT_ARM_NEON=1"
  ) else if "%neon_setting%" == "Yes" (
    set "neon=-DANDROID_ARM_NEON=ON"
    set "explicit_neon=-DEXPLICIT_ARM_NEON=1"
  ) else if "%neon_setting%" == "1" (
    set "neon=-DANDROID_ARM_NEON=ON"
    set "explicit_neon=-DEXPLICIT_ARM_NEON=1"
  ) else (
    set "neon="
    "explicit_neon=-DEXPLICIT_ARM_NEON=0"
  )
  ) else (
    set "neon="
    set "explicit_neon=-DEXPLICIT_ARM_NEON=0"
  )

  if "%project%" == "" (
    echo Cannot configure: Project path not specified
    echo Please specify a project path with --project
    exit /b 1
  )

  if not exist "%project%" (
    echo Cannot configure: Project path not found
    echo Please ensure that the project path exists
    exit /b 1
  )

  if "%NDK%" == "" (
    echo Cannot configure: NDK not specified
    echo Please specify a valid NDK path with
    echo     set NDK=path to NDK
    exit /b 1
  )

  if not exist "%NDK%" (
    echo Cannot configure: NDK not found
    echo Please specify a valid NDK path with
    echo     set NDK=path to NDK
    exit /b 1
  )

  cmake "-DCMAKE_TOOLCHAIN_FILE=%NDK%\build\cmake\android.toolchain.cmake" -DANDROID_ABI=%abi% -DANDROID_PLATFORM=android-%api_level% "-DANDROID_NDK=%NDK%" %explicit_neon% %api_define% %neon% "-DLDSP_PROJECT=%project%" -G Ninja .

  if not %ERRORLEVEL% == 0 (
    echo Cannot configure: CMake failed
    exit /b %ERRORLEVEL%
  )

  exit /b 0
rem End of :configure

:build
  rem Build the user project.

  ninja
  if not %ERRORLEVEL% == 0 (
    echo Cannot build: Ninja failed
    exit /b %ERRORLEVEL%
  )

  exit /b 0
rem End of :build

:stop
  rem Stop the currently-running user project on the phone.

  echo "Stopping LDSP..."
  adb shell "su -c 'sh /data/ldsp/scripts/ldsp_stop.sh'"

  exit /b 0
rem Ecd of :Stop

:install
  rem Install the user project, LDSP hardware config and resources to the phone.
  if not exist "bin\ldsp" (
    echo Cannot push: No ldsp executable found. Please run "ldsp build\ first.
    exit /b 1
  )

  set vendor=%~1
  set model=%~2
  set project=%~3

  set hw_config=".\phones\%vendor%\%model%\ldsp_hw_config.json"

  rem adb root
  rem create temp folder on sdcard
  adb shell "su -c 'mkdir -p /sdcard/ldsp'"

  if not exist %hw_config% (
    echo WARNING: Hardware config file not found, skipping...
  ) else (
    adb push %hw_config% /sdcard/ldsp/ldsp_hw_config.json
  )

  rem Push all project resources, including Pd files in Pd projects, but excluding C/C++ files and folders that contain those files
  rem first folders
  @echo off
  for /F "delims=" %%i in ('dir /B /A:D "%project%"') do (
      dir /B /A "%project%\%%i\*.cpp" "%project%\%%i\*.c" "%project%\%%i\*.h" "%project%\%%i\*.hpp" >nul 2>&1
      if errorlevel 1 (
          adb push "%project%\%%i" /sdcard/ldsp/
      )
  )
  rem then files
  for %%i in ("%project%\*") do (
      if /I not "%%~xi" == ".cpp" if /I not "%%~xi" == ".c" if /I not "%%~xi" == ".h" if /I not "%%~xi" == ".hpp" if /I not "%%~xi" == ".S" if /I not "%%~xi" == ".s" (
          adb push "%%i" /sdcard/ldsp/
      )
  )
  rem finally the ldsp bin
  adb push bin\ldsp /sdcard/ldsp/ldsp

  adb shell "su -c 'mkdir -p /data/ldsp'" rem create ldsp folder
  adb shell "su -c 'cp -r /sdcard/ldsp/* /data/ldsp'" rem cp all files from sd card temp folder to ldsp folder
  adb shell "su -c 'rm -r /sdcard/ldsp'" rem remove temp folder from sdcard

  adb shell "su -c 'chmod 777 /data/ldsp/ldsp'" rem add exe flag to ldsp bin
  exit /b 0
rem End of :install

:run
  rem Run the user project on the phone.

  set args=%~1

  adb shell "su -c 'cd /data/ldsp && ./ldsp %args%'"
  exit /b 0
rem End of :run

:stop
  rem Stop the currently-running user project on the phone.

  echo "Stopping LDSP..."
  adb shell "su -c 'sh /data/ldsp/scripts/ldsp_stop.sh'"

  exit /b 0
rem End of :Stop

:install_scripts
  rem Install the LDSP scripts on the phone.

  adb shell "su -c 'mkdir -p /sdcard/ldsp/scripts'" rem create temp folder on sdcard
  adb push .\scripts\ldsp_* /sdcard/ldsp/scripts/ rem  push scripts there
  adb shell "su -c 'mkdir -p /data/ldsp/scripts'" rem create ldsp scripts folder
  adb shell "su -c 'cp /sdcard/ldsp/scripts/* /data/ldsp/scripts'" rem copy scripts to ldsp scripts folder
  adb shell "su -c 'rm -r /sdcard/ldsp'" rem remove temp folder from sd card

  exit /b 0
rem End of :install_scripts

:clean
  rem Clean project.

  ninja clean

  exit /b 0
rem End of :clean

:help
  rem Print usage information.
  echo usage:
  echo   ldsp configure [vendor] [model] [version] [project]
  echo   ldsp build
  echo   ldsp install [vendor] [model] [project]
  echo   ldsp run ^"[list of arguments]^"
  echo   ldsp stop
  echo   ldsp clean
  echo   ldsp install_scripts
  exit /b 0
rem End of :help

:main

rem Call the appropriate function based on the first argument.

if "%1" == "configure" (
  call :configure %2 %3 %4 %5
  exit /b %ERRORLEVEL%
)

if "%1" == "build" (
  call :build
  exit /b %ERRORLEVEL%
)

if "%1" == "install" (
  call :install %2 %3 %4
  exit /b %ERRORLEVEL%
)

if "%1" == "run" (
  call :run %2
  exit /b %ERRORLEVEL%
)

if "%1" == "stop" (
  call :stop
  exit /b %ERRORLEVEL%
)

if "%1" == "clean" (
  call :clean
  exit /b %ERRORLEVEL%
)

if "%1" == "install_scripts" (
  call :install_scripts
  exit /b %ERRORLEVEL%
)

rem TODO possibly run_persistent
