@echo off

rem This script controls configuring the LDSP build system, building a project
rem using LDSP, and running the resulting binary on a phone.

rem Before running, you need to set the path to the Android NDK, e.g.:
rem set NDK=C:\Users\%USERNAME%\...\android-ndk-r25b

rem this is created by this script via configure()
set "settings_file=ldsp_settings.conf"
rem this is created by CMake after configure()
set "dependencies_file=ldsp_dependencies.conf"

goto main

:get_api_level
  rem Convert a human-readable Android version (e.g. 13, 6.0.1, 4.4) into an API level.
  for /f "tokens=1-3 delims=." %%a in ("%version%") do (
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



rem Retrieve the correct version of the onnxruntime library, based on Android version
:get_onnx_version
    rem Use numeric exit codes to represent different onnx_version values
    if %api_level% geq 24 (
        exit /b 1  rem Error code 1 represents "aboveorEqual24"
    ) else (
        exit /b 0  rem Error code 0 represents "below24"
    )

rem End of :get_onnx_version



:install_scripts
  rem Install the LDSP scripts on the phone.

  rem create temp folder on sdcard
  adb shell "su -c 'mkdir -p /sdcard/ldsp/scripts'"
  rem  push scripts there 
  powershell -Command "Get-ChildItem .\scripts\ldsp_* | ForEach-Object { adb push $_.FullName /sdcard/ldsp/scripts/ }"
  rem create ldsp scripts folder
  adb shell "su -c 'mkdir -p /data/ldsp/scripts'" 
  rem copy scripts to ldsp scripts folder
  adb shell "su -c 'cp /sdcard/ldsp/scripts/* /data/ldsp/scripts'" 
  rem remove temp folder from sd card
  adb shell "su -c 'rm -r /sdcard/ldsp'" 

  exit /b 0
rem End of :install_scripts

:configure
  rem Configure the LDSP build system to build for the given phone model, Android version, and project path.
  set config=%~1
  set version=%~2
  set project=%~3
  set no_neon_audio=%~4

  if "%config%" == "" (
    echo Cannot configure: harwdware configuration file path not specified
    echo Please specify a harwdware configuration file path
    exit /b 1
  )

  rem convert config to an absolute path
  rem 1 Save the current directory
  set current_dir=%CD%
  rem 2 Change to the project directory
  pushd "%config%"
  rem 3 Get the absolute path
  set config_dir=%CD%
  rem 4 Return to the original directory
  popd

  if not exist "%config_dir%" (
    echo Cannot configure: hadrware configuration directory does not exist
    echo Please specify a valid harwdware configuration file path
    exit /b 1
  )

  set hw_config=%config_dir%\ldsp_hw_config.json

  if not exist "%hw_config%" (
    echo Cannot configure: hardware configuration file not found
    echo Please ensure that an ldsp_hw_config.json file exists in "%config%"
    exit /b 1
  )

  rem target ABI
  for /f "tokens=2 delims=:" %%i in ('type "%hw_config%" ^| findstr /r "target architecture"') do set arch=%%i

  set arch=%arch:"=%
  set arch=%arch:,=%
  set arch=%arch: =%

  if "%arch%" == "armv7a" set "abi=armeabi-v7a"
  if "%arch%" == "aarch64" set "abi=arm64-v8a"
  if "%arch%" == "x86" set "abi=x86"
  if "%arch%" == "x86_64" set "abi=x86_64"
  if "%abi%" == "" (
    echo Cannot configure: unknown target architecture "%arch%
    exit /b 1
  )

  rem target Android version
  call :get_api_level
  set "api_level=%ERRORLEVEL%"
  if "%api_level%" == "0" (
    echo Cannot configure: unknown Android version "%version%
    exit /b 1
  )

  rem support for NEON floating-point unit
  for /f "tokens=2 delims=:" %%i in ('type "%hw_config%" ^| findstr /r "supports neon floating point unit"') do set neon_setting=%%i

  set neon_setting=%neon_setting:"=%
  set neon_setting=%neon_setting:,=%
  set neon_setting=%neon_setting: =%

  if /I "%neon_setting%" == "true" (
    set "neon=ON"
  ) else if /I "%neon_setting%" == "yes" (
    set "neon=ON"
  ) else if "%neon_setting%" == "1" (
    set "neon=ON"
  ) else (
    echo NEON floating-point unit not present on phone
    set "neon=OFF"
  )

  rem Passing the --no-neon-audio-format flag configures to not use parallel sample formatting with NEON
  if "%no_neon_audio%" == "--no-neon-audio-format" (
    set "neon_audio_format=OFF"

    rem no need to print this if we already printed that neon is not supported
    if "%neon%" == ON (
      echo Configuring to not use NEON audio formatting
    )
  ) else (
    set "neon_audio_format=ON"
  )



  if "%project%" == "" (
    echo Cannot configure: project path not specified
    echo Please specify a project path with --project
    exit /b 1
  )

  rem convert project to an absolute path
  rem 1 Save the current directory
  set current_dir=%CD%
  rem 2 Change to the project directory
  pushd "%project%"
  rem 3 Get the absolute path
  set project_dir=%CD%
  rem 4 Return to the original directory
  popd

  if not exist "%project_dir%" (
    echo Cannot configure: project path not found
    echo Please ensure that the project path exists
    exit /b 1
  )

  rem Extract the last part of the path for project_name
  FOR %%I IN ("%project_dir%") DO set "project_name=%%~nI"


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

  call :get_onnx_version
  if %ERRORLEVEL% == 1 (
      set "onnx_version=aboveOrEqual24"
  ) else (
      set "onnx_version=below24"
  )

  
  set "dir_path=build"
  if not exist "%dir_path%" (
	  mkdir "%dir_path%"
  )
  cd build
  rem store custom build dir
  set build_dir=%CD%

  rem Run CMake configuration
  cmake -DCMAKE_TOOLCHAIN_FILE="%NDK%/build/cmake/android.toolchain.cmake" ^
        -DDEVICE_ARCH=%arch% -DANDROID_ABI=%abi% -DANDROID_PLATFORM=android-%api_level% ^
        -DANDROID_NDK="%NDK%" -DEXPLICIT_ARM_NEON=%neon% -DNEON_AUDIO_FORMAT=%neon_audio_format% ^
        -DLDSP_PROJECT="%project_dir%" -DONNX_VERSION=%onnx_version% ^
        -G Ninja -B"%build_dir%" -S".."


  if not %ERRORLEVEL% == 0 (
    echo Cannot configure: CMake failed
    exit /b %ERRORLEVEL%
  )

  rem back to root dir
  cd ..

  rem store settings
  echo project_dir="%project_dir%">>"%settings_file%"
  echo project_name="%project_name%">>"%settings_file%"
  echo hw_config="%hw_config%">>"%settings_file%"
  echo arch="%arch%">>"%settings_file%"
  echo ndk="%NDK%">>"%settings_file%"
  echo api_level="%api_level%">>"%settings_file%"
  echo onnx_version="%onnx_version%">>"%settings_file%"

  exit /b 0

rem End of :configure

:build
  rem Build the user project.
  
  rem Check if settings file exists
  IF NOT EXIST "%settings_file%" (
    echo Cannot build: project not configured. Please run "ldsp.bat configure" first.
    exit /b 1
  )

  cd build
  ninja
  cd ..

  if not %ERRORLEVEL% == 0 (
    echo Cannot build: Ninja failed
    exit /b %ERRORLEVEL%
  )

  exit /b 0
rem End of :build


:push_scripts
  rem Create a directory on the SD card using `adb shell` with `mkdir`
  adb shell "su -c 'mkdir -p /sdcard/ldsp/scripts'"

  rem Push scripts matching the pattern `ldsp_*` to the created folder
  adb push .\scripts\ldsp_* /sdcard/ldsp/scripts/

  exit /b
rem End of :push_scripts

:push_resources
  if /i "%ADD_SEASOCKS%"=="TRUE" (
    rem Create a directory on the SD card using `adb shell` with `mkdir`
    adb shell "su -c 'mkdir -p /sdcard/ldsp/resources'"
    
    rem Push resources to the SD card
    adb push resources /sdcard/ldsp/
  )
  exit /b
rem End of :push_resources


:push_debugserver
  setlocal enabledelayedexpansion

  rem Create a directory on the SD card using `adb shell` with `mkdir`
  adb shell "su -c 'mkdir -p /sdcard/ldsp/debugserver'"

  set "debugserver="

  rem different versions of the lldb-server bin can be found in the NDK, depending on the arch
  if "%arch%"=="armv7a" (
    set "dir=arm"
  ) else if "%arch%"=="aarch64" (
    set "dir=aarch64"
  ) else if "%arch%"=="x86" (
    set "dir=i386"
  ) else if "%arch%"=="x86_64" (
    set "dir=x86_64"
  ) else (
    echo Error: Unsupported architecture "%arch%"
    exit /b 1
  )

  rem look for all the available debug servers in the NDK and choose the one that matches the phone's architecture
  for /r "%ndk%" %%f in (lldb-server) do (
      echo %%f | findstr /i "\\%dir%\\" >nul
      if not errorlevel 1 (
          set "debugserver=%%f"
          goto :found
      )
  )

  echo No matching lldb-server found for "%arch%" (directory: "%dir%").
  exit /b 1

:found
  echo Using debugserver: !debugserver!
  adb push "!debugserver!" /sdcard/ldsp/debugserver

  exit /b

rem End of :push_debugserver


:push_onnxruntime
  if /i "%ADD_ONNX%" == "TRUE" (
    rem Create directory on the SD card
    adb shell "su -c 'mkdir -p /sdcard/ldsp/onnxruntime'"

    rem Set the path to the ONNX runtime library
    set "onnx_path=.\dependencies\onnxruntime\%arch%\%onnx_version%\libonnxruntime.so"

    rem Push the ONNX runtime library to the SD card
    adb push "%onnx_path%" /sdcard/ldsp/onnxruntime/libonnxruntime.so
  )
  exit /b
rem End of :push_onnxruntime

:install
  rem Install the user project, LDSP hardware config and resources to the phone.
  if not exist "build\bin\ldsp" (
    echo Cannot install: no ldsp executable found. Please run "ldsp.bat configure" and "ldsp.bat build" first.
    exit /b 1
  )

  rem Retrieve variables from settings file
  IF EXIST "%settings_file%" (
    FOR /F "tokens=1* delims==" %%G IN ("%settings_file%") DO (
      IF "%%G"=="project_dir" set "project_dir=%%H"
      IF "%%G"=="project_name" set "project_name=%%H"
      IF "%%G"=="hw_config" set "hw_config=%%H"
      IF "%%G"=="arch" set "arch=%%H"
      IF "%%G"=="ndk" set "ndk=%%H"
      IF "%%G"=="api_level" set "api_level=%%H"
      IF "%%G"=="onnx_version" set "onnx_version=%%H"
    )
  ) ELSE (
    echo Cannot install: project not configured (no ldsp_settings.conf found). Please run "ldsp.bat configure" first.
    exit /b 1
  )

  rem Retrieve variables from dependencies file
  IF EXIST "%dependencies_file%" (
    FOR /F "tokens=1* delims==" %%G IN ("%dependencies_file%") DO (
      IF "%%G"=="ADD_SEASOCKS" set "ADD_SEASOCKS=%%H"
      IF "%%G"=="ADD_FFTW3" set "ADD_FFTW3=%%H"
      IF "%%G"=="ADD_ONNX" set "ADD_ONNX=%%H"
      IF "%%G"=="ADD_LIBPD" set "ADD_LIBPD=%%H"
    )
  ) ELSE (
    echo Cannot install: project not configured (no ldsp_dependencies.conf found). Please run "ldsp.bat configure" first.
    exit /b 1
  )

  rem create temp ldsp folder on sdcard
  adb shell "su -c \"mkdir -p '/sdcard/ldsp/projects/%project_name%'\""
  
  rem push hardware config file
  adb push %hw_config% /sdcard/ldsp/

  rem Push all project resources, including Pd files in Pd projects, but excluding C/C++ and assembly files, and folders that contain those files
  rem first folders
  @echo off
  for /F "delims=" %%i in ('dir /B /A:D "%project_dir%"') do (
      dir /B /A "%project_dir%\%%i\*.cpp" "%project_dir%\%%i\*.c" "%project_dir%\%%i\*.h" "%project_dir%\%%i\*.hpp" "%project_dir%\%%i\*.S" "%project_dir%\%%i\*.s" "%project_dir%\%%i\*.sh" >nul 2>&1
      if errorlevel 1 (
          adb push "%project_dir%\%%i" "/sdcard/ldsp/projects/%project_name%/"
      )
  )
  rem then files
  for %%i in ("%project_dir%\*") do (
      if /I not "%%~xi" == ".cpp" if /I not "%%~xi" == ".c" if /I not "%%~xi" == ".h" if /I not "%%~xi" == ".hpp" if /I not "%%~xi" == ".S" if /I not "%%~xi" == ".s" if /I not "%%~xi" == ".sh" (
          adb push "%%i" "/sdcard/ldsp/projects/%project_name%/"
      )
  )

  rem now the ldsp bin
  adb push build\bin\ldsp "/sdcard/ldsp/projects/%project_name%/"

  rem now all resources that do not need to be updated at every build
  rem first check if /data/ldsp exists
  adb shell "su -c 'ls /data | grep ldsp'" > nul 2>&1

  if %ERRORLEVEL% neq 0 (
      rem If `/data/ldsp` doesn't exist, create it
      adb shell "su -c 'mkdir -p /data/ldsp'"

      rem Call functions to push all resources
      call :push_scripts
      call :push_resources
      call :push_debugserver
      call :push_onnxruntime

  ) else (
      rem If `/data/ldsp` exists, check and push missing subdirectories/files
      rem Check for `scripts`
      adb shell "su -c 'ls /data/ldsp' 2>/dev/null" | find "scripts" > nul 2>&1
      if %ERRORLEVEL% neq 0 call :push_scripts

      rem Check for `resources`
      adb shell "su -c 'ls /data/ldsp' 2>/dev/null" | find "resources" > nul 2>&1
      if %ERRORLEVEL% neq 0 call :push_resources

      rem Check for `debugserver`
      adb shell "su -c 'ls /data/ldsp' 2>/dev/null" | find "debugserver" > nul 2>&1
      if %ERRORLEVEL% neq 0 call :push_debugserver

      rem Check for `onnxruntime`
      adb shell "su -c 'ls /data/ldsp' 2>/dev/null" | find "onnxruntime" > nul 2>&1
      if %ERRORLEVEL% neq 0 call :push_onnxruntime
  )

  rem create ldsp folder
  adb shell "su -c \"mkdir -p '/data/ldsp/projects/%project_name%'\""
  rem cp all files from sd card temp folder to ldsp folder
  adb shell "su -c 'cp -r /sdcard/ldsp/* /data/ldsp'" 
  rem add exe flag to ldsp bin
  adb shell "su -c \"chmod 777 '/data/ldsp/projects/%project_name%/ldsp'\""

  rem remove temp folder from sdcard
  adb shell "su -c 'rm -r /sdcard/ldsp'" 

  exit /b 0
rem End of :install

:run
  rem Run the user project on the phone.

  set args=%~1

  rem Retrieve variables from settings file
  IF EXIST "%settings_file%" (
    FOR /F "tokens=1* delims==" %%G IN ("%settings_file%") DO (
      IF "%%G"=="project_name" set "project_name=%%H"
    )
  ) ELSE (
    echo Cannot run: project not configured. Please run "ldsp.bat configure" first, then build, install and run.
    exit /b 1
  )

  adb shell "su -c \"cd '/data/ldsp/projects/%project_name%' && export LD_LIBRARY_PATH='/data/ldsp/onnxruntime' && ./ldsp %args%\""
  exit /b 0
rem End of :run

:stop
  rem Stop the currently-running user project on the phone.

  echo "Stopping LDSP..."
  adb shell "su -c 'sh /data/ldsp/scripts/ldsp_stop.sh'"

  exit /b 0
rem End of :Stop

:clean
  rem Clean project.

  cd build
  ninja clean
  cd ..
  rmdir /S /Q .\build
  del "%settings_file%"
  del "%dependencies_file%"

  exit /b 0
rem End of :clean

:clean_phone
  rem Remove current project from directory from the device

  rem Check if settings file exists
  IF NOT EXIST "%settings_file%" (
    echo Cannot clean phone: project not configured. Please run "ldsp.bat configure" first.
    exit /b 1
  )

  rem Retrieve variable from settings file
  FOR /F "tokens=1* delims==" %%G IN ("%settings_file%") DO (
    IF "%%G"=="project_name" set "project_name=%%H"
  )

  adb shell "su -c 'rm -r /data/ldsp/projects/%project_name%'" 

  exit /b 0
rem End of :clean_phone

:clean_ldsp
  rem Remove the ldsp directory from the device.

  adb shell "su -c 'rm -r /data/ldsp/'" 

  exit /b 0
rem End of :clean_phone

:debugserver_start
  rem Prepare for remote debugging

  rem Retrieve variables from settings file
  if exist "%settings_file%" (
      for /f "tokens=1,* delims==" %%G in ("%settings_file%") do (
          if "%%G"=="project_name" set "project_name=%%H"
      )
  ) else (
      echo Cannot start debug server on phone: project not configured. Please run "ldsp.bat configure" first.
      exit /b 1
  )

  rem We're going to work on port 12345, which incidentally is also my Wi-Fi pwd
  set port=12345
  adb forward tcp:%port% tcp:%port%

  rem The debugger can only run bins that are in an accessible directory on the phone
  adb shell "su -c 'cp /data/ldsp/projects/%project_name%/ldsp /data/local/tmp'"

  rem Start the debug server on the phone
  adb shell "su -c '/data/ldsp/debugserver/lldb-server platform --server --listen *:%port%'"

  exit /b
rem End of :debugserver_start

:debugserver_stop
  rem Clean up remote debugging

  echo Stopping debug server...
  adb shell "su -c 'sh /data/ldsp/scripts/ldsp_debugserver_stop.sh'"

  rem Remove the copied binary from the phone
  adb shell "su -c 'rm /data/local/tmp/ldsp'"

  rem Make sure this is the same port used in debugserver_start
  set port=12345
  adb forward --remove tcp:%port%

exit /b
rem End of :debugserver_stop

:install_scripts
  rem Run ldsp_phoneDetails.sh script on the phone

  adb shell "su -c 'sh /data/ldsp/scripts/ldsp_phoneDetails.sh'"

  exit /b 0
rem End of :install_scripts

:mixer_paths
  rem Run ldsp_mixerPaths.sh script on the phone, with optional argument

  rem if "%~1"=="" (
  rem    adb shell "su -c \"sh /data/ldsp/scripts/ldsp_mixerPaths.sh\""
  rem) else (
      adb shell "su -c \"sh /data/ldsp/scripts/ldsp_mixerPaths.sh %~1\""
rem)
exit /b 0
rem End of :mixer_paths

:mixer_paths_rec
  rem # Run ldsp_mixerPaths_recursive.sh script on the phone, with argument

  if "%~1"=="" (
      adb shell "su -c \"sh /data/ldsp/scripts/ldsp_mixerPaths_recursive.sh\""
  ) else (
      adb shell "su -c \"sh /data/ldsp/scripts/ldsp_mixerPaths_recursive.sh %~1\""
)
exit /b 0
rem End of :mixer_paths



:help
  rem Print usage information.
  echo Usage:
  echo   ldsp.bat install_scripts
  echo   ldsp.bat configure [configuration] [version] [project] [--no-neon-audio-format (optional)]
  echo   ldsp.bat build
  echo   ldsp.bat install
  echo   ldsp.bat run ^"[list of arguments]^"
  echo   ldsp.bat stop
  echo   ldsp.bat clean
  echo   ldsp.bat clean_phone
  echo   ldsp.bat clean_ldsp
  echo   ldsp.bat phone_details
  echo   ldsp.bat mixer_paths [optional dir]
  echo   ldsp.bat mixer_paths_recursive [dir]
  echo
  echo Description:
  echo   install_scripts        Install the LDSP scripts on the phone.
  echo   configure              Configure the LDSP build system for the specified phone and project. It requires the following settings, in this order:
  echo                            the path to the folder containing the hardware configuration file of the chosen phone 
  echo                            Android version running on the phone
  echo                            the path to the project to build
  echo                            the optional flag to not use NEON parallel sample formatting (--no-neon-audio-format)
  echo   build                  Build the configured project.
  echo   install                Install the configured project, LDSP hardware config, scripts and resources to the phone.
  echo   run                    Run the configured project on the phone.
  echo                            (Any arguments passed after "run" within quotes are passed to the project)
  echo   stop                   Stop the currently-running project on the phone.
  echo   clean                  Clean the configured project.
  echo   clean_phone            Remove all project files from phone.
  echo   clean_ldsp             Remove all LDSP files from phone.
  echo   debugserver_start      Prepare and run the remote debug server (lldb-server).
  echo   debugserver_stop       Stop the remote debug server.
  echo   phone_details          Get phone details to populate hardware configuration file.
  echo   mixer_paths            Search on phone for mixer_paths.xml candidates, either in default dirs or in the one passed as argument.
  echo   mixer_paths_recursive  Search on phone for mixer_paths.xml candidates in the passed dir and all its subdirs.
  exit /b 0
rem End of :help


:main

rem Call the appropriate function based on the first argument.
if "%1" == "install_scripts" (
  call :install_scripts
  exit /b %ERRORLEVEL%
) else if "%1" == "configure" (
  call :configure %2 %3 %4 %5
  exit /b %ERRORLEVEL%
) else if "%1" == "build" (
  call :build
  exit /b %ERRORLEVEL%
) else if "%1" == "install" (
  call :install
  exit /b %ERRORLEVEL%
) else if "%1" == "run" (
  call :run %2
  exit /b %ERRORLEVEL%
) else if "%1" == "stop" (
  call :stop
  exit /b %ERRORLEVEL%
) else if "%1" == "clean" (
  call :clean
  exit /b %ERRORLEVEL%
) else if "%1" == "clean_phone" (
  call :clean_phone
  exit /b %ERRORLEVEL%
) else if "%1" == "clean_ldsp" (
  call :clean_ldsp
  exit /b %ERRORLEVEL%
) else if "%1" == "debugserver_start" (
  call :debugserver_start
  exit /b %ERRORLEVEL%
) else if "%1" == "debugserver_stop" (
  call :debugserver_stop
  exit /b %ERRORLEVEL%
) else if "%1" == "phone_details" (
  call :phone_details
  exit /b %ERRORLEVEL%
) else if "%~1" == "mixer_paths" (
  call :mixer_paths %2
  exit /b %ERRORLEVEL%
) else if "%~1" == "mixer_paths_recursive" (
  call :mixer_paths_rec %2
  exit /b %ERRORLEVEL%
) else if "%1" == "help" (
  call :help
  exit /b %ERRORLEVEL%
) else (
  echo Unknown command: %1
  call :help
  exit /b 1
)

rem TODO allow for concatenated commands, possibly add run_persistent
