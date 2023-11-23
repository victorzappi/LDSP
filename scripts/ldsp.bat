@echo off

rem Before running, you need to set the path to the Android NDK, e.g.:
rem set NDK=C:\Users\%USERNAME%\android-ndk-r25b

rem Global variable for project settings

SET "project_dir="
SET "project_name="
SET "vendor="
SET "model="

SET "settings_file=ldsp_settings.conf"

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

  set vendor_=%~1
  set model_=%~2
  set version_=%~3
  set project_dir_=%~4

  if "%vendor_%" == "" (
    echo Cannot configure: vendor not specified
    echo Please specify a vendor with --vendor
    exit /b 1
  )

  if "%model_%" == "" (
    echo Cannot configure: model not specified
    echo Please specify a phone model with --model
    exit /b 1
  )

  set hw_config=".\phones\%vendor_%\%model_%\ldsp_hw_config.json"

  if not exist %hw_config% (
    echo Cannot configure: hardware config file not found
    echo Please ensure that an ldsp_hw_config.json file exists for "%vendor_% %model_%"
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
  call :get_api_level %version_%
  set "api_level=%ERRORLEVEL%"
  if "%api_level%" == "0" (
    echo Cannot configure: Unknown Android version "%version_%
    exit /b 1
  )

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

  if "%project_dir_%" == "" (
    echo Cannot configure: Project path not specified
    echo Please specify a project path with --project
    exit /b 1
  )

  if not exist "%project_dir_%" (
    echo Cannot configure: Project path not found
    echo Please ensure that the project path exists
    exit /b 1
  )

  rem Extract the last part of the path for project_name
  FOR %%I IN ("%project_dir_%") DO SET "project_name_=%%~nI"

  rem store settings
  echo project_dir="%project_dir_%" > "%settings_file%"
  echo project_name="%project_name_%" >> "%settings_file%"
  echo vendor="%vendor_%" >> "%settings_file%"
  echo model="%model_%" >> "%settings_file%"


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

  cmake "-DCMAKE_TOOLCHAIN_FILE=%NDK%\build\cmake\android.toolchain.cmake" -DANDROID_ABI=%abi% -DANDROID_PLATFORM=android-%api_level% "-DANDROID_NDK=%NDK%" %explicit_neon% %neon% "-DLDSP_PROJECT=%project_dir_%" -G Ninja .

  if not %ERRORLEVEL% == 0 (
    echo Cannot configure: CMake failed
    exit /b %ERRORLEVEL%
  )

  exit /b 0
rem End of :configure

:build
  rem Build the user project.
  
  REM Check if settings file exists
  IF NOT EXIST "%settings_file%" (
    echo "Cannot build: project not configured. Please run ""ldsp.sh configure [settings]"" first."
    exit /b 1
  )

  ninja
  if not %ERRORLEVEL% == 0 (
    echo Cannot build: Ninja failed
    exit /b %ERRORLEVEL%
  )

  exit /b 0
rem End of :build

:install
  rem Install the user project, LDSP hardware config and resources to the phone.
  if not exist "bin\ldsp" (
    echo Cannot push: No ldsp executable found. Please run "ldsp build\ first.
    exit /b 1
  )

  rem Retrieve variables from settings file
  FOR /F "tokens=1* delims==" %%G IN (%settings_file%) DO (
    IF "%%G"=="project_dir" SET "project_dir=%%H"
    IF "%%G"=="project_name" SET "project_name=%%H"
    IF "%%G"=="vendor" SET "vendor=%%H"
    IF "%%G"=="model" SET "model=%%H"
  )

  set hw_config=".\phones\%vendor%\%model%\ldsp_hw_config.json"
  adb push %hw_config% /sdcard/ldsp/projects/%project_name%/ldsp_hw_config.json

  rem Push all project resources, including Pd files in Pd projects, but excluding C/C++, assembly script files and folders that contain those files
  rem first folders
  @echo off
  for /F "delims=" %%i in ('dir /B /A:D "%project_dir%"') do (
      dir /B /A "%project_dir%\%%i\*.cpp" "%project_dir%\%%i\*.c" "%project_dir%\%%i\*.h" "%project_dir%\%%i\*.hpp" "%project_dir%\%%i\*.S" "%project_dir%\%%i\*.s" >nul 2>&1
      if errorlevel 1 (
          adb push "%project_dir%\%%i" /sdcard/ldsp/projects/%project_name%/
      )
  )
  rem then files
  for %%i in ("%project_dir%\*") do (
      if /I not "%%~xi" == ".cpp" if /I not "%%~xi" == ".c" if /I not "%%~xi" == ".h" if /I not "%%~xi" == ".hpp" if /I not "%%~xi" == ".S" if /I not "%%~xi" == ".s" (
          adb push "%%i" /sdcard/ldsp/projects/%project_name%/
      )
  )

  rem now push the ldsp bin
  adb push bin\ldsp /sdcard/ldsp/projects/%project_name%/ldsp

  rem push gui resources, but only if they are not on phone yet
  rem Check if the folder exists on the Android device
  adb shell "if [ ! -d '/data/ldsp/gui' ]; then echo 'not_exists'; else echo 'exists'; fi" > temp.txt
  rem Read the result into a variable
  set /p FOLDER_STATUS=<temp.txt
  rem Check the folder status
  if "%FOLDER_STATUS%"=="not_exists" (
      adb push resources\gui /sdcard/ldsp/resources/gui
  ) 
  rem Clean up temporary file
  del temp.txt

  adb shell "su -c 'mkdir -p /data/ldsp/projects/%project_name%'" rem create ldsp folder
  adb shell "su -c 'cp -r /sdcard/ldsp/* /data/ldsp'" rem cp all files from sd card temp folder to ldsp folder
  adb shell "su -c 'chmod 777 /data/ldsp/projects/%project_name%/ldsp'" rem add exe flag to ldsp bin
  
  adb shell "su -c 'rm -r /sdcard/ldsp'" rem remove temp folder from sdcard

  exit /b 0
rem End of :install

:run
  rem Run the user project on the phone.

  set args=%~1

  rem Retrieve variables from settings file
  FOR /F "tokens=1* delims==" %%G IN (%settings_file%) DO (
    IF "%%G"=="project_name" SET "project_name=%%H"
  )

  adb shell "su -c 'cd /data/ldsp/projects/%project_name% && ./ldsp %args%'"
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

  del "%settings_file%"

  exit /b 0
rem End of :clean

rem TODO split this into clean_phone ---> project and clean_ldsp ---> ldsp folder AND turn into ldsp script
:clean_phone
  rem Remove the ldsp directory from the device.

  adb shell "su -c 'rm -r /data/ldsp/'" 

  exit /b 0
rem End of :clean_phone

:help
  rem Print usage information.
  echo usage:
  echo   ldsp.bat install_scripts
  echo   ldsp.bat configure [vendor] [model] [version] [project]
  echo   ldsp.bat build
  echo   ldsp.bat install
  echo   ldsp.bat run ^"[list of arguments]^"
  echo   ldsp.bat stop
  echo   ldsp.bat clean
  echo   ldsp.bat clean_phone
  echo.
  echo Description:
  echo   install_scripts    Install the LDSP scripts on the phone.
  echo   configure          Configure the LDSP build system for the specified phone (vendor and model), version and project.
  echo                      (The above settings are needed)
  echo   build              Build the configured user project.
  echo   install            Install the configured user project, LDSP hardware config and resources to the phone.
  echo   run                Run the configured user project on the phone.
  echo                      (Any arguments passed after "run" within quotes are passed to the user project)
  echo   stop               Stop the currently-running user project on the phone.
  echo   clean              Clean the configured user project.
  echo   clean_phone        Remove all LDSP files from phone.
  exit /b 0
rem End of :help

:main

rem Call the appropriate function based on the first argument.

if "%1" == "configure" (
  call :configure %2 %3 %4 %5
  exit /b %ERRORLEVEL%
)
else if "%1" == "build" (
  call :build
  exit /b %ERRORLEVEL%
)
else if "%1" == "install" (
  call :install
  exit /b %ERRORLEVEL%
)
else if "%1" == "run" (
  call :run %2
  exit /b %ERRORLEVEL%
)
else if "%1" == "stop" (
  call :stop
  exit /b %ERRORLEVEL%
)
else if "%1" == "clean" (
  call :clean
  exit /b %ERRORLEVEL%
)
else if "%1" == "clean_phone" (
  call :clean_phone
  exit /b %ERRORLEVEL%
)
else if "%1" == "install_scripts" (
  call :install_scripts
  exit /b %ERRORLEVEL%
)
else if "%1" == "help" (
  call :help
  exit /b %ERRORLEVEL%
) else (
  echo Unknown command: %1
  call :help
  exit /b 1
)

rem TODO possibly run_persistent
