@echo off

Rem Before running, you need to set the path to the Android NDK, e.g.:
Rem set NDK=C:\Users\%USERNAME%\android-ndk-r25b

goto main

Rem Convert a human-readable Android version (e.g. 13, 6.0.1, 4.4) into an API level.
:get_api_level

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
Rem API level 20 corresponds to Android 4.4W, which isn't relevant to us.
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
  Rem Android 12 uses both API_LEVEL 31 and 32
  Rem Default to 31 but allow user to say "12.1" to get 32
  if "%version_minor%" == "1" exit /b 32
  exit /b 31
)
if "%version_major%" == "13" exit /b 33
exit /b 0
Rem End of :get_api_level

:main
set PROJECT=examples\Hello World

call :get_api_level 6.0.0
set API_LEVEL=%ERRORLEVEL%

cmake -DCMAKE_TOOLCHAIN_FILE=%NDK%/build/cmake/android.toolchain.cmake -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-%API_LEVEL% "-DANDROID_NDK=%NDK%" "-DLDSP_PROJECT=%PROJECT%" -G Ninja .
