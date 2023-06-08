# LDSP

A C++ software environment to hack your Android phone and program it as a low-level embedded audio board.

## BEFORE YOU START!
Your Android Phone needs to be rooted. This process highly depends on the specific brand and model of your phone, and may take from 5 minutes to several days. The XDA forum is the best place to look for guides/how-tos and get help:
[https://forum.xda-developers.com/](https://forum.xda-developers.com/)

Then, make sure that you have Developer Options enabled on the phone, then inside Developer options activate USB debugging.

## DEPENDENCIES 

- **GIt [optional]**: while not needed to build/run LDSP applications, Git is very useful to download the LDSP environment and keep it up-to-date:
[https://git-scm.com/book/en/v2/Getting-Started-Installing-Git](https://git-scm.com/book/en/v2/Getting-Started-Installing-Git)

- **Android NDK**: includes the C++ compiler (the toolchain) and the run-time libraries necessary to build from your laptop LDSP applications that can run on your phone.
[https://developer.android.com/ndk/downloads](https://developer.android.com/ndk/downloads)
--> Once downloaded, note down the path of the NDK, it will be needed later! <---

  - If you are on macOS, you may encounter problems when installing the NDK dmg from the official link. Luckily, Google offers an unofficial link from where you can download a zip file directly, which just needs to be extracted and moved to your preferred location.
As of May 2023, the latest version of the macOS NDK zip file can be found here: 
[https://dl.google.com/android/repository/android-ndk-r25c-darwin.zip](https://dl.google.com/android/repository/android-ndk-r25c-darwin.zip)
You can potentially edit this link to download future NDK releases.
Alternatively, 
you can install the NDK via Homebrew, here is how: [https://macappstore.org/android-ndk/](https://macappstore.org/android-ndk/)
To find the installation path, try: 
  ```console
	sudo find / -name 'clang++' 
  ```

- **CMake**: a tool that makes it easier to build projects in C++. It is officially supported by Google for Android app development and it comes in very hand to run the Android NDK C++ tool chain!
Download: [https://cmake.org/download/](https://cmake.org/download/)
Install: [https://cmake.org/install/](https://cmake.org/install/)

  Then check that the CMake binary was successfully installed, by openinig a terminal/command prompt (i.e., a shell) and typing:
  ```console
  cmake --version
  ```
  You should see the number of the installed version!

- **Ninja Build System**: we use this tool to speed up the building process of LDSP applications. The LDSP environment includes the NDK and some third-party libraries (all open-source) that need to be built and linked against, and [Ninja](https://ninja-build.org/) makes this almost effortlessly!

    - Linux: install via your package manager
    - macOS: install via Homebrew or MacPorts; otherwise follow Windows’ guidelines
    - Windows: download from here [https://github.com/ninja-build/ninja/releases](https://github.com/ninja-build/ninja/releases) and then add the path to the binary to your [PATH environment variable](https://stackoverflow.com/a/69383805)

  Then check that the Ninja binary was successfully installed, by openinig a shell and typing:
  ```console
  ninja --version
  ```

- **ADB (Android device bridge)**: a small tool that allows you to open a shell on your phone via USB. We use it to install, run and stop LDSP applications during development. You can then set up your SSH server to run/stop LDSP applications.

    - Linux: install via your package manager
    - macOS: install via Homebrew or MacPorts; otherwise follow Windows’ guidelines
    - Windows: download the Android SDK, adb we'll be there: [https://developer.android.com/tools/releases/platform-tools](https://developer.android.com/tools/releases/platform-tools), then add the path to the binary to your PATH environment variable

  Then check that the adb binary was successfully installed, by openinig a shell and typing:
  ```console
  adb --version
  ```

- **ADBD Insecure**: a lightweight Android app massively used by XDA developers that enables ADB shells to do way more things (it remounts the phone’s file system as r/w and automatically invokes root privileges for ADB shells). 
For now, just download the apk from here:
[https://forum.xda-developers.com/t/2014-11-10-root-adbd-insecure-v2-00.1687590/](https://forum.xda-developers.com/t/2014-11-10-root-adbd-insecure-v2-00.1687590/)

- **Pure Data “vanilla” [optional]**:  in case you want to code LDSP applications in Pd rather than C++: [https://puredata.info/downloads](https://puredata.info/downloads)

## INSTALL LDSP ON LAPTOP

- Install on laptop: open a shell in the folder where you want to clone the Github repository. Choose a convenient location, since here you will find code examples and where you will likely write your code.

Clone the repository from Github:
	git clone https://github.com/victorzappi/LDSP.git

Then export a new environment variable called NDK, with the path to its actual content, i..e, the path where the toolchain folder can be found.
We recommend you make the variable persistent, otherwise you will have to export it again every time you open a new shell to build an LDSP application. This can be done on [Linux](https://stackoverflow.com/a/13046663), [macOS](https://support.apple.com/guide/terminal/use-environment-variables-apd382cc5fa-4f58-4449-b20a-41c53c006f8f/mac) as well as [Windows](https://learn.microsoft.com/en-us/powershell/module/microsoft.powershell.core/about/about_environment_variables?view=powershell-7.3#saving-environment-variables-with-the-system-control-panel).



[UNDER CONSTRUCTION FROM HERE ON]

## PHONE CONFIGURATION

First, we will install ADB Insecure on the phone and activate it. With the phone connected to the computer via USB, open a shell in the folder where you downloaded the apk and run:
```console
adb install adbd-Insecure-v2.00.apk
```

Take a look at your phone, it may ask for permission! Once installation is done (this usually takes about 5 seconds or less), find the app on the phone, open it and tick 'enable insecure adbd' and 'enable at boot'. The latter option often does not work, so if you reboot your phone you may need to re-enable ADBD Insecure.

Now we can install LDSP scripts on the phone, that will help us configure LDSP for the specific phone. We open a shell in the LDSP’s folder (where you clone the repo) and run:

sh scripts/ldsp.sh install_scripts

In the terminal we were just using we will write:
```console
adb shell
```

We are inside our phone! It probably says something like root@mako for example.

We will go to the LDSP scripts from this terminal in the phone and we can check the architecture of the phone, android version that will be useful for the configuration file of the specific phone and the configure build and run of projects:
```console
cd /data/ldsp/scripts
sh ldsp_architecture.sh 
sh ldsp_androidVersion.sh 
```

In this case, LF Nexus 4, the architecture is armv7a (it’s missing an a), has value ‘true’ for supporting neon floating point unit, and the Android version is 5.1.1.
We can run a script to see the audio devices divided in Playback and Capture devices. We want to focus on the ones with an id along the lines of MultiMedia in the Playback devices, as its number will serve us later for our configuration file. 
```console
sh ldsp_audioDevices.sh
```

For example, the playback device with id = multimedia5 which is number 14, we end up choosing as our “default playback device” (line 21) in the configuration json for  the LG Nexus 4.

This is the .json file for configuring a phone.  You can copy one of another phones in LDSP > phones > Vendor > Model, to another folder while you search for all of this, and then copy+paste it in its own folder inside the corresponding vendor. 

We need to find the .xml where we will find all this information in a more clear manner, to also retrieve the playback path names and capture path names. 
```console
sh ldsp_mixerPaths.sh 
```

We will retrieve that file and copy it in our working folder (not the one that will be in LDSP!). We open it with our text editor of preference. 
The example that we used before for the ‘default playback device number’ in our configuration .json can be seen here again. We should choose for that a deep buffer playback or a low latency playback which is better. In this case we have both, being MultiMedia1 deep-buffer-playback, with number 0, and MultiMedia5, being low-latency-playback, with number 14.  

We also need to probably change the name of the playback paths names, according to what the vendor put in this model. For example, we can assume the line-out corresponds to the headphones. 

And we need to do the same with the capture path names. For example we can assume that the ‘line-in’ is the headset-mic.

After this, go to the configuration .json file you have opened in your text editor. If you copies it from another phone, as requested before, you can delete what is in brackets after control inputs, as it will be automatically populated through use. You can now paste it inside of the LDSP/phones/vendor/model folder. 


## RUNNING EXAMPLES


In order to try LDSP with our rooted phone, we can try the sine example that you find in LDSP/examples/sine. From the LDSP folder’s terminal with command
```console
./scripts/ldsp.sh --vendor=nameOfVendor —model="Name of Model" —version=x.x.x --project=./examples/Fundamentals/sine configure build
```

The vendor, model and version you can find with the commands explained before or in the configuration file, and folder of the specific phone. 

The first time you configure and build it is also going to install and check dependencies and so on, so it might take longer. 
We will later push the sine example to our phone with the command
```console
./scripts/ldsp.sh --vendor=nameOfVendor —model="Name of Model"  —version=x.x.x —project=./examples/Fundamentals/sine install 
```
We can run it in our phones from this same terminal with
```console
./scripts/ldsp.sh run "-o speaker -O" 
```

With the -o we are declaring which output we want to use (in this case, speaker) and by declaring -O we are shutting any input. 
You will able to see a list of the control inputs and sensors after running a project!
You can stop the project with control + c.
To check the line between headphones for example and mic we can use the example ‘passthrough’. We build and configure same as before, and push same as before. 
```console
./scripts/ldsp.sh --vendor=nameOfVendor —model="Name of Model" —version=x.x.x --project=./examples/Fundamentals/passthrough configure build

./scripts/ldsp.sh --vendor=nameOfVendor —model="Name of Model" —version=x.x.x —project=./examples/Fundamentals/passthrough install
```

We can run it choosing the line-out as output (meaning, the headphones) and the mic as input. We can also change the buffer size to 512 with -p, the default being 256.
```console
./scripts/ldsp.sh run "-o line-out -i mic -p 512"
```

