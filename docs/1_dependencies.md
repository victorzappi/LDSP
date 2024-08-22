# DEPENDENCIES
## Required and optional tools to install beforehand

- **Git**: while not needed to build/run LDSP applications, Git is very useful to download the LDSP environment from this repo and keep it up-to-date:
[https://git-scm.com/book/en/v2/Getting-Started-Installing-Git](https://git-scm.com/book/en/v2/Getting-Started-Installing-Git)
Also, if you are on Windows, by installing Git you will have access to Git Bash, which is a very useful Unix-like terminal environment that can be used instead of the command prompt!
<br><br>

- **Android NDK**: includes the C++ compiler (the toolchain) and the run-time libraries necessary to build from your computer/laptop LDSP applications that can run on your phone.
[https://developer.android.com/ndk/downloads](https://developer.android.com/ndk/downloads)
<br>*--> Once downloaded, note down the path of the NDK, it will be needed later! <---*

  - If you are on macOS, you may encounter problems when installing the NDK dmg from the official link. Luckily, Google offers an unofficial link from where you can download a zip file directly, which just needs to be extracted and moved to your preferred location.
As of July 2024, the latest version of the macOS NDK zip file can be found here: 
[https://dl.google.com/android/repository/android-ndk-r26d-darwin.zip](https://dl.google.com/android/repository/android-ndk-r26d-darwin.zip)
You can potentially edit this link to download future NDK releases.
Alternatively, 
you can install the NDK via Homebrew, here is how: [https://macappstore.org/android-ndk/](https://macappstore.org/android-ndk/)
To find the installation path, open a terminal and try (the NDK contains several versions of clang for Android cross-compilation): 
  ```console
  sudo find / -name 'clang++' 
  ```

  > **Note:** from NDK r26, [the minimum supported Android version is Android 5.0 Lollipop (API level 21)](https://github.com/android/ndk/issues/1751). If you want to use LDSP on a phone running Android 4.4 KitKat (API level 19) or lower, we suggest NDK version r25c, accessible [here](https://github.com/android/ndk/wiki/Unsupported-Downloads). And here is the [macOS zip download link](https://dl.google.com/android/repository/android-ndk-r25c-darwin.zip).

<br>

- **CMake**: a tool that makes it easier to build projects in C++. It is officially supported by Google for Android app development and it comes in very handy to run the Android NDK C++ toolchain!

    - Linux: install via your package manager
    - macOS: install via Homebrew or MacPorts; otherwise follow Windows’ guidelines
    - Windows: download the *binary distribution* installer that matches your system from here [https://cmake.org/download/](https://cmake.org/download/) and run it

  Then check that the CMake binary (*cmake*) was successfully installed, by openinig a terminal/command prompt (i.e., a shell) and typing:
  ```console
  cmake --version
  ```
  You should see the number of the installed version!
<br><br>

- **Ninja Build System**: we use this tool to speed up the building process of LDSP applications. The LDSP environment includes the NDK and some third-party libraries (all open-source) that need to be built and linked against, and [Ninja](https://ninja-build.org/) makes this almost effortless!

    - Linux: install via your package manager
    - macOS: install via Homebrew or MacPorts; otherwise download the zip from here [https://github.com/ninja-build/ninja/releases](https://github.com/ninja-build/ninja/releases), uncompress and place it wherever you prefer; then add to your [PATH environment variable](https://www.architectryan.com/2012/10/02/add-to-the-path-on-mac-os-x-mountain-lion/#.Uydjga1dXDg) the path to the folder that contains the *ninja* binary
    - Windows: download the zip from here [https://github.com/ninja-build/ninja/releases](https://github.com/ninja-build/ninja/releases), uncompress and place it wherever you prefer, and then add to your [PATH environment variable](https://stackoverflow.com/a/44272417) the path to the folder that contains the *ninja* binary (.exe)

  Then check that *ninja* was successfully installed, by openinig a **new shell** and typing:
  ```console
  ninja --version
  ```
<br>

- **ADB (Android device bridge)**: a small tool that allows you to open a shell on your phone via USB. We mainly use it to install, run and stop LDSP applications during development. 

    - Linux: install via your package manager
    - macOS: install via Homebrew or MacPorts; otherwise use Windows’ guidelines
    - Windows: download the Android SDK, *adb* binary we'll be there: [https://developer.android.com/tools/releases/platform-tools](https://developer.android.com/tools/releases/platform-tools), uncompress and then add to your PATH environment variable the path to the folder contaning the binary

  Then check that the *adb* binary was successfully installed, by openinig a **new shell** and typing:
  ```console
  adb --version
  ```
  Finally, check if ADB can actually connect to your phone (on Windows this procedure may fail due to the lack of proper drivers—in case, [see the next point](#adb-drivers-windows)): 
    - Activate developer options on your phone: [https://developer.android.com/studio/debug/dev-options](https://developer.android.com/studio/debug/dev-options)
    - Connect your phone to your computer with a USB cable capable of transfering data (e.g., pictures), as opposed to one that can only charge the phone
    - Open a shell on your computer and try to connect/open a shell on your phone with:
      ```console
      adb shell
      ```
  Check out your phone's screen, as Android will likely ask you for permission before allowing the connection. Tick the box that grants permission indefinitely.
  If everything goes fine, your shell should be replaced by a new one opened on the phone, where the default phone user is logged in! You can type the command *exit* to return to your local shell.
<br><br>

<a name="adb-drivers-windows"></a>
- **ADB drivers [Windows only]**: on Windows, you may need to install some drivers for ADB to connect to your phone. Drivers tend to be phone-specific, so one solution is to look for any official distribution that matches your phone's vendor and model. Otherwise, you can try these more generic options: 

    - Universal ADB Drivers: these should work with a large number of phones, [https://adb.clockworkmod.com/](https://adb.clockworkmod.com/)
    - Google USB Driver: for Google devices, [https://developer.android.com/studio/run/win-usb](https://developer.android.com/studio/run/win-usb)
    - OEM USB Drivers: these are the drivers provided directly by the phones' manufacturers, [https://developer.android.com/studio/run/oem-usb#Drivers](https://developer.android.com/studio/run/oem-usb#Drivers)
    
  This link explains how to install the ADB driver for a phone once dowloaded: [https://www.auslogics.com/en/articles/install-adb-driver-on-windows-10/#step-1-check-if-you-already-have-the-adb-driver-installed-on-your-pc](https://www.auslogics.com/en/articles/install-adb-driver-on-windows-10/#step-1-check-if-you-already-have-the-adb-driver-installed-on-your-pc)
<br><br>

- **Pure Data “vanilla” [optional]**:  in case you want to code LDSP applications in Pd rather than C++: [https://puredata.info/downloads](https://puredata.info/downloads)


[Previous: Introduction](0_introduction.md) | [Next: Installation](2_installation.md)