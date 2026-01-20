@echo off
setlocal enabledelayedexpansion

rem This script controls configuring the LDSP build system, building a project
rem using LDSP, and running the resulting binary on a phone.

rem Before running, you need to set the path to the Android NDK, e.g.:
rem set NDK=C:\Users\%USERNAME%\...\android-ndk-r25b

rem this is created by this script via :configure
set "settings_file=ldsp_settings.conf"
rem this is created by CMake after :configure
set "dependencies_file=ldsp_dependencies.conf"

goto main

rem Build the adb command with optional device serial
rem Command-line arg (PHONE_SERIAL) takes priority over saved setting (phone_serial)
:setup_adb
  if defined PHONE_SERIAL (
    set "ADB=adb -s %PHONE_SERIAL%"
  ) else if defined phone_serial (
    set "ADB=adb -s %phone_serial%"
  ) else (
    set "ADB=adb"
  )
  exit /b 0

:get_api_level
  rem — Split version (e.g. "13", "6.0.1", "4.4") into major/minor/patch
  for /f "tokens=1-3 delims=." %%a in ("%version%") do (
    set "version_major=%%a"
    set "version_minor=%%b"
    set "version_patch=%%c"
  )

  rem — Android 1.x (API 1–4)
  if "%version_major%"=="1" (
    if "%version_minor%"=="1" exit /b 2
    if "%version_minor%"=="5" exit /b 3
    if "%version_minor%"=="6" exit /b 4
    exit /b 1
  )

  rem — Android 2.x (API 5–10)
  if "%version_major%"=="2" (
    if "%version_minor%"=="0" (
      if "%version_patch%"=="1" exit /b 6
      exit /b 5
    )
    if "%version_minor%"=="1" exit /b 7
    if "%version_minor%"=="2" exit /b 8
    if "%version_minor%"=="3" (
      if "%version_patch%" gtr "2" exit /b 10
      exit /b 9
    )
  ) 

  rem — Android 3.x (API 11–13)
  if "%version_major%"=="3" (
    if "%version_minor%"=="1" exit /b 12
    if "%version_minor%"=="2" exit /b 13
    exit /b 11
  )

  rem — Android 4.0–4.4 (API 14–19)
  if "%version_major%"=="4" (
    if "%version_minor%"=="0" (
      if "%version_patch%"=="3" exit /b 15
      if "%version_patch%"=="4" exit /b 15
      exit /b 14
    )
    if "%version_minor%"=="1" exit /b 16
    if "%version_minor%"=="2" exit /b 17
    if "%version_minor%"=="3" exit /b 18
    if "%version_minor%"=="4" exit /b 19
  )

  rem — Android 5.x (API 21–22)
  if "%version_major%"=="5" (
    if "%version_minor%"=="1" exit /b 22
    exit /b 21
  )

  rem — Android 6.0 (API 23)
  if "%version_major%"=="6" exit /b 23

  rem — Android 7.x (API 24–25)
  if "%version_major%"=="7" (
    if "%version_minor%"=="1" exit /b 25
    exit /b 24
  )

  rem — Android 8.x (API 26–27)
  if "%version_major%"=="8" (
    if "%version_minor%"=="1" exit /b 27
    exit /b 26
  )

  rem — Android 9 (API 28)
  if "%version_major%"=="9" exit /b 28

  rem — Android 10 (API 29)
  if "%version_major%"=="10" exit /b 29

  rem — Android 11 (API 30)
  if "%version_major%"=="11" exit /b 30

  rem — Android 12 (API 31–32)
  if "%version_major%"=="12" (
    if "%version_minor%"=="1" exit /b 32
    exit /b 31
  )

  rem — Android 13 (API 33)
  if "%version_major%"=="13" exit /b 33

  rem — Android 14 (API 34)
  if "%version_major%"=="14" exit /b 34

  rem — Android 15 (API 35)
  if "%version_major%"=="15" exit /b 35

  rem — Unknown version
  exit /b 0
rem — End of :get_api_level



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

  call :setup_adb

  rem create temp folder on sdcard
  %ADB% shell "su -c 'mkdir -p /sdcard/ldsp/scripts'"
  rem  push scripts there 
  powershell -Command "Get-ChildItem .\scripts\ldsp_* | ForEach-Object { %ADB% push $_.FullName /sdcard/ldsp/scripts/ }"
  rem create ldsp scripts folder
  %ADB% shell "su -c 'mkdir -p /data/ldsp/scripts'" 
  rem copy scripts to ldsp scripts folder
  %ADB% shell "su -c 'cp /sdcard/ldsp/scripts/* /data/ldsp/scripts'" 
  rem remove temp folder from sd card
  %ADB% shell "su -c 'rm -r /sdcard/ldsp'" 

  exit /b 0
rem End of :install_scripts

:configure
  rem Configure the LDSP build system to build for the given phone model, Android version, and project path.
  
  rem Use global variables parsed in :main
  set "config=%CONFIG%"
  set "version=%VERSION%"
  set "project=%PROJECT%"
  
  rem defaults
  set "neon_audio_format=ON"
  set "neon_fft=ON"
  set "build_type=Release"
  
  rem Apply flags from global parsing
  if defined NO_NEON_AUDIO set "neon_audio_format=OFF"
  if defined NO_NEON_FFT set "neon_fft=OFF"
  if defined DEBUG set "build_type=Debug"

  if "%config%" == "" (
    echo Cannot configure: hardware configuration file path not specified
    echo Please specify a hardware configuration file path with --configuration
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
    echo Cannot configure: hardware configuration directory does not exist
    echo Please specify a valid hardware configuration file path with --configuration
    exit /b 1
  )

  set hw_config=%config_dir%\ldsp_hw_config.json

  if not exist "%hw_config%" (
    echo Cannot configure: hardware configuration file not found
    echo Please ensure that an ldsp_hw_config.json file exists in "%config%"
    exit /b 1
  )

  rem get major NDK version
	rem — 1) Grab the full revision string (e.g. "25.2.9519653")
	for /f "tokens=2 delims==" %%A in ('
			findstr /R "^Pkg\.Revision" "%NDK%\source.properties"
	') do (
			set "full_rev=%%A"
			goto :gotFull
	)
	echo ERROR: could not parse NDK version
	exit /b 1

	:gotFull
	rem full_rev now holds something like 25.2.9519653
	rem — 2) Extract the part before the first dot: the major version
	for /f "tokens=1 delims=." %%B in ("%full_rev%") do (
			set "ndk_version=%%B"
	)

	
	
  rem target ABI
  for /f "tokens=2 delims=:" %%I in ('
      type "%hw_config%" ^| findstr /ri "target architecture"
  ') do set "arch=%%I"

  rem strip quotes, commas and spaces
  set "arch=%arch:"=%"
  set "arch=%arch:,=%"
  set "arch=%arch: =%"

  if /i "%arch%"=="aarch64" (
      set "abi=arm64-v8a"
      set "neon=ON"
  ) else if /i "%arch%"=="armv7a" (
      set "abi=armeabi-v7a"
      set "neon=ON"
  ) else if /i "%arch%"=="armv5te" (
      set "abi=armv5te"    & rem or whatever you actually want here
      set "neon=OFF"
      rem this abi (with no NEON support) was dropped in NDK 17
      rem we need to check if the current NDK is <=17 and warn the user otherwise!
      if %ndk_version% GEQ 17 (
        echo You are trying to use NDK r%ndk_version% to build for a very old 32‑bit phone, which is not supported anymore!
        echo But don't worry! Try NDK version 16 or lower
        exit /b 1
      )
  ) else if /i "%arch%"=="x86" (
      set "abi=x86"
      set "neon=OFF"
  ) else if /i "%arch%"=="x86_64" (
      set "abi=x86_64"
      set "neon=OFF"
  ) else (
      echo Cannot configure: unknown target architecture "%arch%"
      exit /b 1
  )
  
	rem Neon settings
  if "%neon%"=="ON" (
    if "%neon_audio_format%"=="OFF" (
      rem Passing the --no-neon-audio-format flag configures to not use parallel audio streams formatting with NEON
      echo Configuring to not use NEON audio streams formatting
    )
    if "%neon_fft%"=="OFF" (
      rem Passing the --no-neon-fft flag configures to not use parallel fft with NEON (uses fftw instead)
      echo Configuring to not use NEON to parallelize FFT
    )
  ) else (
    echo NEON floating-point unit not present on phone
    set "neon_audio_format=OFF"
    set "neon_fft=OFF"
  )

  rem target Android version
  if "%version%" == "" (
    echo Cannot configure: Android version not specified
    echo Please specify an Android version with --version
    exit /b 1
  )
  
  call :get_api_level
  set "api_level=%ERRORLEVEL%"
  if "%api_level%" == "0" (
    echo Cannot configure: unknown Android version "%version%"
    exit /b 1
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

  rem we need to force Clang as toolchain in case we use older NDKs!
  if %ndk_version% LSS 19 (
    set "TOOLCHAIN_VER=-DCMAKE_ANDROID_NDK_TOOLCHAIN_VERSION=clang"
  ) else (
    set "TOOLCHAIN_VER="
  )

  rem print phone serial info
  if defined PHONE_SERIAL (
    set "serial_display=%PHONE_SERIAL%"
  ) else (
    set "serial_display=(not set)"
  )

  echo.
  echo LDSP configuration:
  echo   Project:        %project_name%
  echo   Project path:   %project_dir%
  echo   HW config:      %hw_config%
  echo   Architecture:   %arch% (%abi%)
  echo   Android API:    %api_level% (version %version%)
  echo   NDK:            %NDK% (r%ndk_version%)
  echo   NEON supported: %neon%
  echo   NEON audio fmt: %neon_audio_format%
  echo   NEON FFT:       %neon_fft%
  echo   Build type:     %build_type%
  echo   Phone serial:   %serial_display%

  echo.
  echo CMake configuration:
  echo.
  rem Run CMake configuration
  cmake -DCMAKE_TOOLCHAIN_FILE="%NDK%/build/cmake/android.toolchain.cmake" ^
        -DDEVICE_ARCH=%arch% -DANDROID_ABI=%abi% -DANDROID_PLATFORM=android-%api_level% -DANDROID_NDK="%NDK%" %TOOLCHAIN_VER% ^
        -DNEON_SUPPORTED=%neon% -DNEON_AUDIO_FORMAT=%neon_audio_format% -DNE10_FFT=%neon_fft%^
        -DLDSP_PROJECT="%project_dir%" -DONNX_VERSION=%onnx_version% ^
        -G Ninja -B"%build_dir%" -S".." -DCMAKE_BUILD_TYPE=%build_type%


  if not %ERRORLEVEL% == 0 (
    echo Cannot configure: CMake failed
    exit /b %ERRORLEVEL%
  )

  rem back to root dir
  cd ..

  rem store settings
  echo project_dir="%project_dir%">"%settings_file%"
  echo project_name="%project_name%">>"%settings_file%"
  echo hw_config="%hw_config%">>"%settings_file%"
  echo arch="%arch%">>"%settings_file%"
  echo ndk="%NDK%">>"%settings_file%"
  echo api_level="%api_level%">>"%settings_file%"
  echo onnx_version="%onnx_version%">>"%settings_file%"
  echo phone_serial="%PHONE_SERIAL%">>"%settings_file%"

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
  %ADB% shell "su -c 'mkdir -p /sdcard/ldsp/scripts'"

  rem Push all scripts to the created folder (batch does not support *)
  for %%f in (scripts\ldsp_*) do (
    %ADB% push "%%f" /sdcard/ldsp/scripts/
	)
	
  exit /b
rem End of :push_scripts

:push_resources
  if /i %ADD_SEASOCKS%=="TRUE" (
    rem Create a directory on the SD card using `adb shell` with `mkdir`
    %ADB% shell "su -c 'mkdir -p /sdcard/ldsp/resources'"
    
    rem Push resources to the SD card
    %ADB% push resources /sdcard/ldsp/
  )
  exit /b
rem End of :push_resources


:push_debugserver

  rem Create a directory on the SD card using `adb shell` with `mkdir`
  %ADB% shell "su -c 'mkdir -p /sdcard/ldsp/debugserver'"

  set "debugserver="

  rem different versions of the lldb-server bin can be found in the NDK, depending on the arch
  if %arch%=="armv7a" (
    set "dir=arm"
  ) else if %arch%=="aarch64" (
    set "dir=aarch64"
  ) else if %arch%=="x86" (
    set "dir=i386"
  ) else if %arch%=="x86_64" (
    set "dir=x86_64"
  ) else (
    echo Error: Unsupported architecture "%arch%"
    exit /b 1
  )

  rem look for all the available debug servers in the NDK and choose the one that matches the phone's architecture
	for /r "%ndk%" %%f in (lldb-server) do (
		if exist "%%f" (
			set "debugserver=%%f"
			goto :found
		)
	)

  echo No matching lldb-server found for %arch% (directory: "%dir%").
  exit /b 1

:found
  echo Using debugserver: !debugserver!
  %ADB% push "!debugserver!" /sdcard/ldsp/debugserver

  exit /b

rem End of :push_debugserver


:push_onnxruntime
  if /i %ADD_ONNX%=="TRUE" (
		rem strip quotes from vars and combine them
    set "arch=!arch:"=!"
    set "onnx_version=!onnx_version:"=!"
    set "onnx_path=.\dependencies\onnxruntime\!arch!\!onnx_version!\libonnxruntime.so"

    %ADB% shell "su -c 'mkdir -p /sdcard/ldsp/onnxruntime'"
    %ADB% push "!onnx_path!" /sdcard/ldsp/onnxruntime/libonnxruntime.so
  )
  exit /b


:install
	rem Install the user project, LDSP hardware config and resources to the phone.
	setlocal EnableDelayedExpansion
 
  if not exist "build\bin\ldsp" (
    echo Cannot install: no ldsp executable found. Please run "ldsp.bat configure" and "ldsp.bat build" first.
    exit /b 1
  )

  rem Retrieve variables from settings file
	IF EXIST "%settings_file%" (
		FOR /F "usebackq tokens=1* delims==" %%G IN ("%settings_file%") DO (
			set "key=%%G"
			set "val=%%H"

			if /I "!key!"=="project_dir"     set "project_dir=!val!"
			if /I "!key!"=="project_name"    set "project_name=!val!"
			if /I "!key!"=="hw_config"       set "hw_config=!val!"
			if /I "!key!"=="arch"            set "arch=!val!"
			if /I "!key!"=="ndk"             set "ndk=!val!"
			if /I "!key!"=="api_level"       set "api_level=!val!"
			if /I "!key!"=="onnx_version"    set "onnx_version=!val!"
			if /I "!key!"=="phone_serial"    set "phone_serial=!val!"
		)
	) ELSE (
		echo Cannot install: config file not found.
		exit /b 1
	)

  rem Retrieve variables from dependencies file
	IF EXIST "%dependencies_file%" (
		FOR /F "usebackq tokens=1* delims==" %%G IN ("%dependencies_file%") DO (
			set "key=%%G"
			set "val=%%H"

			if /I "!key!"=="ADD_SEASOCKS" set "ADD_SEASOCKS=!val!"
			rem if /I "!key!"=="ADD_FFTW3"    set "ADD_FFTW3=!val!"
			if /I "!key!"=="ADD_ONNX"     set "ADD_ONNX=!val!"
			rem if /I "!key!"=="ADD_LIBPD"    set "ADD_LIBPD=!val!"
		)
	) ELSE (
		echo Cannot install: config file not found.
		exit /b 1
	)

  call :setup_adb
	
  rem create temp ldsp folder on sdcard
  %ADB% shell "su -c \"mkdir -p '/sdcard/ldsp/projects/%project_name%'\""
  
  rem push hardware config file
  %ADB% push %hw_config% /sdcard/ldsp/

  rem Push all project resources, including Pd files in Pd projects, but excluding C/C++ and assembly files, and folders that contain those files
  rem first folders
  
  for /F "delims=" %%i in ('dir /B /A:D "%project_dir%"') do (
		dir /B /A "%project_dir%\%%i\*.cpp" "%project_dir%\%%i\*.c" "%project_dir%\%%i\*.h" "%project_dir%\%%i\*.hpp" "%project_dir%\%%i\*.S" "%project_dir%\%%i\*.s" "%project_dir%\%%i\*.sh" >nul 2>&1
		if errorlevel 1 (
				%ADB% push "%project_dir%\%%i" "/sdcard/ldsp/projects/%project_name%/"
		)
  )
  rem then files
  for %%i in ("%project_dir%\*") do (
		if /I not "%%~xi" == ".cpp" if /I not "%%~xi" == ".c" if /I not "%%~xi" == ".h" if /I not "%%~xi" == ".hpp" if /I not "%%~xi" == ".S" if /I not "%%~xi" == ".s" if /I not "%%~xi" == ".sh" (
				%ADB% push "%%i" "/sdcard/ldsp/projects/%project_name%/"
		)
  )

  rem now the ldsp bin
  %ADB% push build\bin\ldsp "/sdcard/ldsp/projects/%project_name%/"

  rem now all resources that do not need to be updated at every build
  rem first check if /data/ldsp exists
  rem adb shell "su -c 'ls /data | grep ldsp'" > nul 2>&1
	set "found_dir="
	for /f %%i in ('%ADB% shell "su -c 'ls /data | grep ldsp'"') do (
		set "found_dir=%%i"
	)

	if not defined found_dir (
		rem If `/data/ldsp` doesn't exist, create it
		%ADB% shell "su -c 'mkdir -p /data/ldsp'"

		rem Call functions to push all resources
		call :push_scripts
		call :push_resources
		call :push_debugserver
		call :push_onnxruntime

  ) else (
		rem If `/data/ldsp` exists, check and push missing subdirectories/files
		rem Check for `scripts`
		set "found_dir="
		for /f %%i in ('%ADB% shell "su -c 'ls /data/ldsp | grep scripts'"') do (
			set "found_dir=%%i"
		)
		if not defined found_dir call :push_scripts

		rem Check for `resources`
		set "found_dir="
		for /f %%i in ('%ADB% shell "su -c 'ls /data/ldsp | grep resources'"') do (
			set "found_dir=%%i"
		)
		if not defined found_dir call :push_resources

		rem Check for `debugserver`
		set "found_dir="
		for /f %%i in ('%ADB% shell "su -c 'ls /data/ldsp | grep debugserver'"') do (
			set "found_dir=%%i"
		)
		if not defined found_dir call :push_debugserver

		rem Check for `onnxruntime`
		set "found_dir="
		for /f %%i in ('%ADB% shell "su -c 'ls /data/ldsp | grep onnxruntime'"') do (
			set "found_dir=%%i"
		)
		if not defined found_dir (call :push_onnxruntime)
  )

  rem create ldsp folder
  %ADB% shell "su -c \"mkdir -p '/data/ldsp/projects/%project_name%'\""
  rem cp all files from sd card temp folder to ldsp folder
  %ADB% shell "su -c 'cp -r /sdcard/ldsp/* /data/ldsp'" 
  rem add exe flag to ldsp bin
  %ADB% shell "su -c \"chmod 777 '/data/ldsp/projects/%project_name%/ldsp'\""
  rem add exe flag to server bin
  %ADB% shell "su -c 'chmod 777 /data/ldsp/debugserver/lldb-server'"

  rem remove temp folder from sdcard
  %ADB% shell "su -c 'rm -r /sdcard/ldsp'" 

  exit /b 0
rem End of :install

:run
  rem Run the user project on the phone.
	setlocal EnableDelayedExpansion

  set args=%~1
	
  rem Retrieve variables from settings file
	IF EXIST "%settings_file%" (
		FOR /F "usebackq tokens=1* delims==" %%G IN ("%settings_file%") DO (
			set "key=%%G"
			set "val=%%H"

			if /I "!key!"=="project_name"    set "project_name=!val!"
			if /I "!key!"=="phone_serial"    set "phone_serial=!val!"
		)
  ) ELSE (
    echo Cannot run: project not configured. Please run "ldsp.bat configure" first, then build, install and run.
    exit /b 1
  )

  call :setup_adb

	%ADB% shell "su -c 'cd \"/data/ldsp/projects/%project_name%\" && export LD_LIBRARY_PATH=\"/data/ldsp/onnxruntime/\" && ./ldsp %args%'"

  exit /b 0
rem End of :run

:stop
  rem Stop the currently-running user project on the phone.

  rem Retrieve variables from settings file (for phone_serial)
  IF EXIST "%settings_file%" (
    FOR /F "usebackq tokens=1* delims==" %%G IN ("%settings_file%") DO (
      set "key=%%G"
      set "val=%%H"

      if /I "!key!"=="phone_serial"    set "phone_serial=!val!"
    )
  )

  call :setup_adb

  echo "Stopping LDSP..."
  %ADB% shell "su -c 'sh /data/ldsp/scripts/ldsp_stop.sh'"

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
	setlocal EnableDelayedExpansion
	
	rem Retrieve variables from settings
	IF EXIST "%settings_file%" (
		FOR /F "usebackq tokens=1* delims==" %%G IN ("%settings_file%") DO (
			set "key=%%G"
			set "val=%%H"

			if /I "!key!"=="project_name"    set "project_name=!val!"
			if /I "!key!"=="phone_serial"    set "phone_serial=!val!"
		)
  ) ELSE (
    echo Cannot clean phone: project not configured. Please run "ldsp.bat configure" first.
    exit /b 1
  )

  call :setup_adb
  
  %ADB% shell "su -c 'rm -r /data/ldsp/projects/%project_name%'" 

  exit /b 0
rem End of :clean_phone

:clean_ldsp
  rem Remove the ldsp directory from the device.

  rem Retrieve variables from settings file (for phone_serial)
  IF EXIST "%settings_file%" (
    FOR /F "usebackq tokens=1* delims==" %%G IN ("%settings_file%") DO (
      set "key=%%G"
      set "val=%%H"

      if /I "!key!"=="phone_serial"    set "phone_serial=!val!"
    )
  )

  call :setup_adb

  %ADB% shell "su -c 'rm -r /data/ldsp/'" 

  exit /b 0
rem End of :clean_phone

:debugserver_start
  rem Prepare for remote debugging
	setlocal EnableDelayedExpansion

  rem Retrieve variables from settings file
  IF EXIST "%settings_file%" (
		FOR /F "usebackq tokens=1* delims==" %%G IN ("%settings_file%") DO (
			set "key=%%G"
			set "val=%%H"

			if /I "!key!"=="project_name"    set "project_name=!val!"
			if /I "!key!"=="phone_serial"    set "phone_serial=!val!"
		)
  ) else (
      echo Cannot start debug server on phone: project not configured. Please run "ldsp.bat configure" first.
      exit /b 1
  )

  call :setup_adb

  rem We're going to work on port 12345, which incidentally is also my Wi-Fi pwd
  set port=12345
  %ADB% forward tcp:%port% tcp:%port%

  rem The debugger can only run bins that are in an accessible directory on the phone
  %ADB% shell "su -c 'cp /data/ldsp/projects/%project_name%/ldsp /data/local/tmp'"

  rem Start the debug server on the phone
  %ADB% shell "su -c '/data/ldsp/debugserver/lldb-server platform --server --listen *:%port%'"

  exit /b
rem End of :debugserver_start

:debugserver_stop
  rem Clean up remote debugging

  rem Retrieve variables from settings file (for phone_serial)
  IF EXIST "%settings_file%" (
    FOR /F "usebackq tokens=1* delims==" %%G IN ("%settings_file%") DO (
      set "key=%%G"
      set "val=%%H"

      if /I "!key!"=="phone_serial"    set "phone_serial=!val!"
    )
  )

  call :setup_adb

  echo Stopping debug server...
  %ADB% shell "su -c 'sh /data/ldsp/scripts/ldsp_debugserver_stop.sh'"

  rem Remove the copied binary from the phone
  %ADB% shell "su -c 'rm /data/local/tmp/ldsp'"

  rem Make sure this is the same port used in debugserver_start
  set port=12345
  %ADB% forward --remove tcp:%port%

exit /b
rem End of :debugserver_stop

:phone_details
	rem Run ldsp_phoneDetails.sh script on the phone

  rem Retrieve variables from settings file (for phone_serial)
  IF EXIST "%settings_file%" (
    FOR /F "usebackq tokens=1* delims==" %%G IN ("%settings_file%") DO (
      set "key=%%G"
      set "val=%%H"

      if /I "!key!"=="phone_serial"    set "phone_serial=!val!"
    )
  )

  call :setup_adb

  %ADB% shell "su -c 'sh /data/ldsp/scripts/ldsp_phoneDetails.sh'"

  exit /b 0
rem End of :phone_details

:mixer_paths
  rem Run ldsp_mixerPaths.sh script on the phone, with optional argument

  rem Retrieve variables from settings file (for phone_serial)
  IF EXIST "%settings_file%" (
    FOR /F "usebackq tokens=1* delims==" %%G IN ("%settings_file%") DO (
      set "key=%%G"
      set "val=%%H"

      if /I "!key!"=="phone_serial"    set "phone_serial=!val!"
    )
  )

  call :setup_adb

  %ADB% shell "su -c \"sh /data/ldsp/scripts/ldsp_mixerPaths.sh %~1\""

  exit /b 0
rem End of :mixer_paths

:mixer_paths_rec
  rem # Run ldsp_mixerPaths_recursive.sh script on the phone, with argument

  rem Retrieve variables from settings file (for phone_serial)
  IF EXIST "%settings_file%" (
    FOR /F "usebackq tokens=1* delims==" %%G IN ("%settings_file%") DO (
      set "key=%%G"
      set "val=%%H"

      if /I "!key!"=="phone_serial"    set "phone_serial=!val!"
    )
  )

  call :setup_adb

  if "%~1"=="" (
    rem this will simply trigger the script's usage 
    %ADB% shell "su -c \"sh /data/ldsp/scripts/ldsp_mixerPaths_recursive.sh\""
  ) else (
    %ADB% shell "su -c \"sh /data/ldsp/scripts/ldsp_mixerPaths_recursive.sh %~1\""
)
exit /b 0
rem End of :mixer_paths_rec


:mixer_paths_opened
  rem Run ldsp_mixerPaths_opened.sh script on the phone, with optional argument

  rem Retrieve variables from settings file (for phone_serial)
  IF EXIST "%settings_file%" (
    FOR /F "usebackq tokens=1* delims==" %%G IN ("%settings_file%") DO (
      set "key=%%G"
      set "val=%%H"

      if /I "!key!"=="phone_serial"    set "phone_serial=!val!"
    )
  )

  call :setup_adb

  %ADB% shell "su -c \"sh /data/ldsp/scripts/ldsp_mixerPaths_opened.sh %~1\""

  exit /b 0
rem End of :mixer_paths_opened



:help
  rem Print usage information.
  echo Usage:
  echo   ldsp.bat install_scripts
  echo   ldsp.bat configure [settings]
  echo   ldsp.bat build
  echo   ldsp.bat install
  echo   ldsp.bat run ^"[list of arguments]^"
  echo   ldsp.bat stop
  echo   ldsp.bat clean
  echo   ldsp.bat clean_phone
  echo   ldsp.bat clean_ldsp
  echo   ldsp.bat debugserver_start
  echo   ldsp.bat debugserver_stop
  echo   ldsp.bat phone_details
  echo   ldsp.bat mixer_paths [optional dir]
  echo   ldsp.bat mixer_paths_recursive [dir]
  echo   ldsp.bat mixer_paths_opened [optional dir]
  echo.
  echo Settings (used with the 'configure' step):
  echo   --configuration=CONFIGURATION, -c CONFIGURATION   The path to the folder containing the hardware configuration file of the chosen phone.
  echo   --version=VERSION, -a VERSION                     The Android version running on the phone.
  echo   --project=PROJECT, -p PROJECT                     The path to the project to build.
  echo   --no-neon-audio-format                            Configure to not use NEON parallel audio streams formatting
  echo   --no-neon-fft                                     Configure to not use NEON to parallelize FFT
  echo   --debug                                           Switch build type from Release to Debug (debug symbols, no optimizations)
  echo.
  echo Description:
  echo   install_scripts        Install the LDSP scripts on the phone (use --phone-serial when more phones are connected).
  echo   configure              Configure the LDSP build system for the specified phone and project (use --phone-serial when more phones are connected).
  echo                            (The above settings are needed)
  echo   build                  Build the configured project.
  echo   install                Install the configured project, LDSP hardware config, scripts and resources to the phone.
  echo   run                    Run the configured project on the phone.
  echo                            (Any arguments passed after "run" within quotes are passed to the project)
  echo   stop                   Stop the currently-running project on the phone (use --phone-serial when more phones are connected).
  echo   clean                  Clean the configured project.
  echo   clean_phone            Remove all project files from phone (use --phone-serial when more phones are connected).
  echo   clean_ldsp             Remove all LDSP files from phone (use --phone-serial when more phones are connected).
  echo   debugserver_start      Prepare and run the remote debug server (lldb-server).
  echo   debugserver_stop       Stop the remote debug server.
  echo   phone_details          Get phone details to populate hardware configuration file (use --phone-serial when more phones are connected).
  echo   mixer_paths            Search on phone for mixer_paths.xml candidates, either in default dirs or in the one passed as argument (use --phone-serial when more phones are connected).
  echo   mixer_paths_recursive  Search on phone for mixer_paths.xml candidates in the passed dir and all its subdirs (use --phone-serial when more phones are connected).
  echo   mixer_paths_opened     [Android 6.0+] Search on phone for mixer_paths.xml candidates in use by Android, either in default dir or in the one passed as argument (use --phone-serial when more phones are connected).
  echo.
  echo Global options (can be used with any command):
  echo   --phone-serial=SERIAL, --phone-serial SERIAL      Target a specific phone by serial number (from 'adb devices') when more phones are connected.
  exit /b 0
rem End of :help


:main

rem Parse all arguments globally
set "PHONE_SERIAL="
set "CONFIG="
set "VERSION="
set "PROJECT="
set "NO_NEON_AUDIO="
set "NO_NEON_FFT="
set "DEBUG="

set "grab_next="
for %%A in (%*) do (
  set "arg=%%~A"
  if defined grab_next (
    if "!grab_next!"=="config" set "CONFIG=%%~A"
    if "!grab_next!"=="version" set "VERSION=%%~A"
    if "!grab_next!"=="project" set "PROJECT=%%~A"
    if "!grab_next!"=="serial" set "PHONE_SERIAL=%%~A"
    set "grab_next="
  ) else if "%%~A"=="--configuration" (
    set "grab_next=config"
  ) else if "%%~A"=="-c" (
    set "grab_next=config"
  ) else if "%%~A"=="--version" (
    set "grab_next=version"
  ) else if "%%~A"=="-a" (
    set "grab_next=version"
  ) else if "%%~A"=="--project" (
    set "grab_next=project"
  ) else if "%%~A"=="-p" (
    set "grab_next=project"
  ) else if "%%~A"=="--phone-serial" (
    set "grab_next=serial"
  ) else if "%%~A"=="--no-neon-audio-format" (
    set "NO_NEON_AUDIO=1"
  ) else if "%%~A"=="--no-neon-fft" (
    set "NO_NEON_FFT=1"
  ) else if "%%~A"=="--debug" (
    set "DEBUG=1"
  ) else (
    rem Handle --flag=value format (when PowerShell keeps them together)
    if "!arg:~0,16!"=="--configuration=" set "CONFIG=!arg:~16!"
    if "!arg:~0,10!"=="--version=" set "VERSION=!arg:~10!"
    if "!arg:~0,10!"=="--project=" set "PROJECT=!arg:~10!"
    if "!arg:~0,15!"=="--phone-serial=" set "PHONE_SERIAL=!arg:~15!"
  )
)

rem Call the appropriate function based on the first argument.
if "%1" == "install_scripts" (
  call :install_scripts
  exit /b %ERRORLEVEL%
) else if "%1" == "configure" (
  call :configure
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
) else if "%~1" == "mixer_paths_opened" (
  call :mixer_paths_opened %2
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