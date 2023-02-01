@echo off

rem Before running, you need to set the path to the Android NDK, e.g.:
rem set NDK=C:\Users\%USERNAME%\android-ndk-r25b

goto main

:get_api_level
  rem Convert a human-readable Android version (e.g. 13, 6.0.1, 4.4) into an API level.
  set version_full=%1

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
  for /f "tokens=4 delims=^\"" %%a in ('find /i "target architecture" %hw_config%') do set "arch=%%a"
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

  rem support for NEON floating-point unit
  for /f "tokens=4 delims=^\"" %%a in ('find /i "supports neon floating point unit" %hw_config%') do set "neon_setting=%%a"
  if "%arch%" == "armv7a" (
  if "%neon_setting%" == "true" (
    set neon="-DANDROID_ARM_NEON=ON"
  ) else if "%neon_setting%" == "True" (
    set neon="-DANDROID_ARM_NEON=ON"
  ) else if "%neon_setting%" == "yes" (
    set neon="-DANDROID_ARM_NEON=ON"
  ) else if "%neon_setting%" == "Yes" (
    set neon="-DANDROID_ARM_NEON=ON"
  ) else if "%neon_setting%" == "1" (
    set neon="-DANDROID_ARM_NEON=ON"
  ) else (
    set neon="-DANDROID_ARM_NEON=OFF"
  )
  ) else (
    set neon=""
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

  if not exist "%NDK" (
    echo Cannot configure: NDK not found
    echo Please specify a valid NDK path with
    echo     export NDK=<path to NDK>
    exit /b 1
  )

  cmake -DCMAKE_TOOLCHAIN_FILE=%NDK%\build\cmake\android.toolchain.cmake -DANDROID_ABI=%abi% -DANDROID_PLATFORM=android-%api_level% "-DANDROID_NDK=%NDK%" %neon% "-DLDSP_PROJECT=%project%" -G Ninja .

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

:push
  rem Push the user project and LDSP hardware config to the phone.
  if not exist "bin\ldsp" (
    echo Cannot push: No ldsp executable found. Please run "ldsp build\ first.
    exit /b 1
  )

  set hw_config=".\phones\%vendor%\%model%\ldsp_hw_config.json"

  adb root
  adb shell "mkdir -p /data/ldsp"

  if not exists %hw_config% (
    echo WARNING: Hardware config file not found, skipping...
  ) else (
    adb push %hw_config% /data/ldsp/ldsp_hw_config.json
  )

  adb push bin\ldsp /data/ldsp/ldsp
  exit /b 0
rem End of :push

:push_sdcard
  rem Push the user project and LDSP hardware config to the phone's SD card.
  if not exist "bin\ldsp" (
    echo Cannot push: No ldsp executable found. Please run "ldsp build\ first.
    exit /b 1
  )

  set hw_config=".\phones\%vendor%\%model%\ldsp_hw_config.json"

  adb root
  adb shell "mkdir -p /sdcard/ldsp"

  if not exists %hw_config% (
    echo WARNING: Hardware config file not found, skipping...
  ) else (
    adb push %hw_config% /sdcard/ldsp/ldsp_hw_config.json
  )

  adb push bin\ldsp /sdcard/ldsp/ldsp
  exit /b 0
rem End of :push_sdcard

:run
  rem Run the user project on the phone.

  adb shell "chmod +x /data/ldsp/ldsp"
  adb shell "/data/ldsp/ldsp"
  exit /b 0
rem End of :run

:help
  rem Print usage information.
  echo usage: ldsp [options] [steps] [run args]
  echo options:
  echo   --vendor=VENDOR, -v VENDOR\tThe vendor of the phone to build for.
  echo   --model=MODEL, -m MODEL\tThe model of the phone to build for.
  echo   --project=PROJECT, -p PROJECT\tThe path to the user project to build.
  echo   --version=VERSION, -a VERSION\tThe Android version to build for.
  echo steps:
  echo   configure\t\t\tConfigure the LDSP build system for the specified phone, version, and project.
  echo   build\t\t\t\tBuild the user project.
  echo   push\t\t\t\tPush the user project and LDSP hardware configuration to the phone.
  echo   push_sdcard\t\t\tPush the user project and LDSP hardware configuration to the phone's SD card.
  echo   run\t\t\t\tRun the user project on the phone.
  echo   \t\t\t\t(Any arguments passed after "run" are passed to the user project.)
  exit /b 0
rem End of :help

:main

rem Parse command line arguments.

setlocal enabledelayedexpansion

set "steps="
set "arg_index=0"
set "skip_next=0"

for %%a in (%*) do (
  shift
  set "arg=%%a"
  set /a "arg_index+=1"

  echo !arg! %!arg_index!%

  if !skip_next! == 1 (
    set "skip_next=0"
  ) else if "!arg!" == "--vendor" (
    set "skip_next=1"
    set "vendor=%!arg_index!%"
  ) else if "!arg!" == "--model" (
    set "skip_next=1"
    set "model=%!arg_index!%"
  ) else if "!arg!" == "--project" (
    set "skip_next=1"
    set "project=%!arg_index!%"
  ) else if "!arg!" == "--version" (
    set "skip_next=1"
    set "version=%!arg_index!%"
  ) else if "!arg!" == "--help" (
    set "steps=!steps! help"
  ) else if "!arg!" == "run" (
    set "steps=!steps! run"
    goto :end_parsing
  ) else (
    set "steps=!steps! !arg!"
  )
)
:end_parsing

if "!steps!" == "" (
  call :help
  exit /b 1
)

for /f "delims=" %%i in ('echo %steps%') do (
  set "step=%%i"
  if "!step!"=="configure" (
    call :configure
  ) else if "!step!"=="build" (
    call :build
  ) else if "!step!"=="push" (
    call :push
  ) else if "!step!"=="push_sdcard" (
    call :push_sdcard
  ) else if "!step!"=="run" (
    call :run %2 %3 %4 %5 %6 %7 %8 %9
  ) else if "!step!"=="help" (
    call :help
  ) else (
    echo Unknown step !step!
    exit /b 1
  )
)
