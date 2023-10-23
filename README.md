# LDSP

A C++ software environment to hack your Android phone and program it as a low-level embedded audio board.

## BEFORE YOU START!
Your Android Phone needs to be rooted. This process highly depends on the specific brand and model of your phone, and may take from 5 minutes to several days. The XDA forum is the best place to look for guides/how-tos and get help:
[https://forum.xda-developers.com/](https://forum.xda-developers.com/)

Then, make sure that you have Developer Options enabled on the phone, then inside Developer options activate USB debugging.

- **Phones we have tested**:
- - Asus ZenFone 2 Laser (ZE500KL - Z00ED)
- - Asus ZenFone Go (ZB500KG - X00BD)
- - Huawei P8 Lite (alice)
- - Nexus G2 Mini (g2m)
- - Nexus 4 (mako)
- - Nexus 5 (bullhead)
- - Samsung Galaxy Tab 4 (SM-T237P)
- - Xiaomi Mi8 Lite (platina)

If you configure a new phone, please let us know and we will add the configuration file you have populated (more on that later). 

## DEPENDENCIES 

- **Git [optional]**: while not needed to build/run LDSP applications, Git is very useful to download the LDSP environment from this repo and keep it up-to-date:
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
To find the installation path, open a terminal and try (the NDK contains several versions of clang for Android cross-compilation): 
  ```console
  sudo find / -name 'clang++' 
  ```

- **CMake**: a tool that makes it easier to build projects in C++. It is officially supported by Google for Android app development and it comes in very handy to run the Android NDK C++ toolchain!
Download: [https://cmake.org/download/](https://cmake.org/download/)
Install: [https://cmake.org/install/](https://cmake.org/install/)

  Then check that the CMake binary was successfully installed, by openinig a terminal/command prompt (i.e., a shell) and typing:
  ```console
  cmake --version
  ```
  You should see the number of the installed version!

- **Ninja Build System**: we use this tool to speed up the building process of LDSP applications. The LDSP environment includes the NDK and some third-party libraries (all open-source) that need to be built and linked against, and [Ninja](https://ninja-build.org/) makes this almost effortless!

    - Linux: install via your package manager
    - macOS: install via Homebrew or MacPorts; otherwise follow Windows’ guidelines
    - Windows: download from here [https://github.com/ninja-build/ninja/releases](https://github.com/ninja-build/ninja/releases) and then add the path to the binary to your [PATH environment variable](https://stackoverflow.com/a/69383805)

  Then check that the Ninja binary was successfully installed, by openinig a shell and typing:
  ```console
  ninja --version
  ```

- **ADB (Android device bridge)**: a small tool that allows you to open a shell on your phone via USB. We mainly use it to install, run and stop LDSP applications during development. You can also set up an SSH server on your phone to do all these things wirelessly, but ADB is simpler and in most cases more convenient.

    - Linux: install via your package manager
    - macOS: install via Homebrew or MacPorts; otherwise follow Windows’ guidelines
    - Windows: download the Android SDK, adb we'll be there: [https://developer.android.com/tools/releases/platform-tools](https://developer.android.com/tools/releases/platform-tools), then add the path to the binary to your PATH environment variable

  Then check that the adb binary was successfully installed, by openinig a shell and typing:
  ```console
  adb --version
  ```

- **ADBD Insecure [optional]**: a lightweight Android app massively used by XDA developers that enables ADB shells to do way more things (it remounts the phone’s file system as r/w and automatically invokes root privileges for ADB shells). Highly recommended!
We will install it on the phone. But, for now, just download the apk from here:
[https://forum.xda-developers.com/t/2014-11-10-root-adbd-insecure-v2-00.1687590/](https://forum.xda-developers.com/t/2014-11-10-root-adbd-insecure-v2-00.1687590/)

- **Pure Data “vanilla” [optional]**:  in case you want to code LDSP applications in Pd rather than C++: [https://puredata.info/downloads](https://puredata.info/downloads)

## INSTALL LDSP ON LAPTOP

- **Clone the repo**: open a shell in the folder where you want to clone the GitHub repository. Choose a convenient location, since here you will find code examples and it is also where you will likely write your own code/projects.

  Clone the repository from Github:
  ```console
  git clone https://github.com/victorzappi/LDSP.git
  ```
- **Init submodules**: the repo includes some submodules, i.e., code stored in other repos that is compiled as libraries used by LDSP. This is done totally transparently! You just need to use the shell to enter the LDSP folder you just downloadewd and clone all the submodules from there with a single command:
  ```console
  cd LDSP
  git submodule update --init --recursive
  ```

- **Export the NDK var**: finally, export a new environment variable called NDK, with the path to the actual content of the Android NDK you downloaded from the dependecies list. Specifically, export the full path that points to where the 'toolchains' and the 'sources' folders can be found (i.e., path to the folder that contains them). This allows LDSP to use the toolchain and the C++ run-time!
We recommend you make the variable persistent, otherwise you will have to export it again every time you open a new shell to build an LDSP application. This can be done on [Linux](https://stackoverflow.com/a/13046663), [macOS](https://support.apple.com/guide/terminal/use-environment-variables-apd382cc5fa-4f58-4449-b20a-41c53c006f8f/mac) as well as [Windows](https://learn.microsoft.com/en-us/powershell/module/microsoft.powershell.core/about/about_environment_variables?view=powershell-7.3#saving-environment-variables-with-the-system-control-panel).



[UNDER CONSTRUCTION FROM HERE ON]

## PHONE CONFIGURATION


Now we can install LDSP scripts on the phone, that will help us configure LDSP for the specific phone. We open a shell in the LDSP’s folder (where you cloned the repo) and run:
 ```console
sh scripts/ldsp.sh install_scripts
 ```
In the terminal we were just using we will write:
```console
adb shell
```

We are inside our phone! It probably says something like root@mako for example.

We will go to the LDSP scripts from this terminal in the phone and we can check the architecture of the phone as well as the android version. This will be useful for the configuration file of the specific phone and the configure build and run of projects:
```console
cd /data/ldsp/scripts
sh ldsp_architecture.sh 
sh ldsp_androidVersion.sh 
```

Some phones do not have their architecture, but it is something we can also search online!
We can run a script to see the audio devices divided in Playback and Capture devices. We want to focus on the ones with an id along the lines of MultiMedia in the Playback devices, as its number will serve us later for our configuration file. 
```console
sh ldsp_audioDevices.sh
```

For example, the playback device with id = multimedia5 which is number 14, we end up choosing as our “default playback device” (line 21) in the configuration json for  the LG Nexus 4.

This is the .json file for configuring a phone.  You can copy one of another phones in LDSP > phones > Vendor > Model, to another folder while you search for all of this, and then copy+paste it in its own folder inside the corresponding vendor. 

We need to find the path to a .xml file where we will find all this information in a more clear manner, to also retrieve the playback path names and capture path names. 
```console
sh ldsp_mixerPaths.sh 
```

If you are not able to find it like that, you can run:
```console
find / -name mixer_paths.xml.
```

Once you find it, you have to retrieve it into your computer. This can be done within terminal -in the folder on your computer you desire the archive to be in- by running:
``` console
adb pull path/to/xml/in/phone
```

We will use a .json configure file of another phone as a template, which you can find in any of the specific phones already configured.
We will populate the the name of device, target architecture, floating point support, the path to the xml file as well as the playback paths names and capture path names, default playback device number and default capture device number. The mixer playback/capture device activation are usually populated through the scripts -you can leave it as the template-, though in some cases, like the Nexus 5, you will have to populate it yourself as well as the secondary activation. Be mindful of the placeholders. Control outputs are also populated by the script -it's better to delete whatever the template .json file had. 

With the mixer_paths.xml and the output from the ldsp_audioDevices.sh, we can infere the deafault playback and capture number. We are looking, in the xml file, for a deep buffer playback or a low latency playback usually by the identifier MultiMediaNumber. We can then see the number it corresponds to in the output from our LDSP script. For example, you might find a deep-buffer-playback by the id of MultiMedia1, you search for it in the output to find it has the number 15. This is the number you put in either the playback or capture number. 

We also need to change the name of the playback paths names and capture paths names, according to how they are named by the vendor in each specific model. For example, we can assume the line-out corresponds to the headphones or line-in to a headset-mic, so we can search for those types of keywords in our mixer_paths.xml file and populate our configuration file accordingly. 

This configuration .json file should be in your LDSP/phones/vendor/model folder, in order to be used. If you configure a phone, please let us know! So we can add it to the repository, to the list, and make it more accesible to other people. 

We have had the experience of certain phones that do not have any type of mixer_paths.xml -or even .xml files at all. This makes it impossible, as of now, to configure the phone for proper LDSP use. The phones in question are:
- - LG Optimus L3 
- - MEDION_E4504_S13A_206_160302 
- - Xiaomi Redmi 9
- - Samsung Galaxy S3 mini



## RUNNING EXAMPLES


In order to try LDSP with our rooted phone, we can try the sine example that you find in LDSP/examples/sine. From the LDSP folder’s terminal with command
```console
./scripts/ldsp.sh --vendor=nameOfVendor --model="Name of Model" --version=x.x.x --project=./examples/Fundamentals/sine configure build
```

The vendor, model and version you can find with the commands explained before or in the configuration file, and folder of the specific phone. 

The first time you configure and build it is also going to install and check dependencies and so on, so it might take longer. 
We will later push the sine example to our phone with the command
```console
./scripts/ldsp.sh --vendor=nameOfVendor --model="Name of Model"  --version=x.x.x --project=./examples/Fundamentals/sine install 
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
./scripts/ldsp.sh --vendor=nameOfVendor —model="Name of Model" --version=x.x.x --project=./examples/Fundamentals/passthrough configure build

./scripts/ldsp.sh --vendor=nameOfVendor —model="Name of Model" --version=x.x.x --project=./examples/Fundamentals/passthrough install
```

We can run it choosing the line-out as output (meaning, the headphones) and the mic as input. We can also change the buffer size to 512 with -p, the default being 256.
```console
./scripts/ldsp.sh run "-o line-out -i mic -p 512"
```


## OPTIONS 

 
In order to see the available options you can tinker around with, you can run 
  ```console
./scripts/ldsp.sh run "-h" 
```

-h stands for help.

