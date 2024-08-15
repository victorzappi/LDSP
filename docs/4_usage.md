# USAGE
## How to build and use example LDSP applications on your phone

LDSP comes with a variety of example projects and libraries that can be used out of the box. Let's see how to build and play around with the *sine* example that you can find in *[LDSP/examples/sine](../examples/sine)*. Note that all LDSP C++ projects (including examples) consist of a dedicated folder with inside at least a source file called *render.cpp* (more on this in the [next section](#how-to-design-your-own-ldsp-application)).


Here is an overview of all the steps necessary to 'use' an LDSP application:

1. **[Configure](#1-configure)**: select a project and a target phone
2. **[Build](#2-build)**: create an executable binary file (i.e., a *bin*)
3. **[Install](#3-install)**: push the bin to the phone
4. **[Run](#4-run)**: launch the application by executing the bin, optionally passing run-time parameters
5. **[Stop](#5-stop)**: stop the application!

LDSP comes with a main script that facilitates these steps! Let's see them in detail for the case of the *sine* example.
There are two versions of the main script, which were designed to work on different systems/configurations:
- **macOS/Linux users:** for you, the main script is called *[ldsp.sh](../scripts/ldsp.sh)* (shell script version). You should be able to use it from any terminal application.

- **Windows users:** you can use the version of the main script called *[ldsp.bat](../scripts/ldsp.bat)* (batch script version), either from a Command Prompt or from a PowerShell—we reccomend the latter. However, if you have a Unix-like environment installed, like *Cygwin* or *MinGW*, you should be able to use *ldsp.sh* too.

>**Note:** the two versions of the scripts are *almost* equivalent; they  differ slightly in synatx. The following example steps will showcase both versions!
 


<br>

### 1. Configure
Open a shell on your computer (i.e., a 'local' shell) and navigate to the LDSP folder, where you downladed the content of the repo. From here, we configure the project to build the #sine# example for the LG Nexus 4 phone (we tested this phone already and LDSP includes a pre-made configuration file for it—check [Phone Configuration](3_phone_config.md)). We'll be using the *ldsp* script (found in [LDSP/scripts/](../scripts)), which expects:
- the *configure* command
- the path to the folder containing the phone's hardware configuration file, either relative to the current position or absolute
- the version of Android running on the chosen phone (in this case 5.1.1)
- the path to the project folder

#### macOS and Linux
```console
./scripts/ldsp.sh configure --configuration="./phones/LG/Nexus 4 (mako)" --version=5.1.1 --project=./examples/Fundamentals/sine 
```

#### Windows
```console
.\scripts\ldsp.bat configure ".\phones\LG\Nexus 4 (mako)" "5.1.1" ".\examples\Fundamentals\sine" 
```

This step will set up the building environment and will generate a persistent configuration file, hence it needs to be repeated only if we switch to a different phoen or project!



<br>

### 2. Build
Once configured, a project can be built simply via:

#### macOS and Linux
```console
./scripts/ldsp.sh build
```

#### Windows
```console
.\scripts\ldsp.bat build
```

If everything goes write, a new bin file called *ldsp* will appear in the newly created *LDSP/build/bin* folder!



<br>

### 3. Install
Make sure that your phone is connected to your computer via USB. Then, we keep leveraging the *ldsp* script to push the bin to the phone, like this:

#### macOS and Linux
```console
./scripts/ldsp.sh install
```

#### Windows
```console
.\scripts\ldsp.bat install
```

The script will push the bin to */data/ldsp/projects/sine*. You can check it by opening a shell on the phone (i.e., a 'remote' shell) via ADB and navigating there.
Notice that no additional parameters need to be passed to the script, because it uses the current configuration!



<br>

### 4. Run
Let's run the *sine* project so that we hear a vanilla sinusoid coming out of the line-out/headphone jack of your phone. There are several options to run an LDSP application:

- **Run from your host computer** by using the LDSP script. From your local shell in the LDSP folder, type:
    #### macOS and Linux
    ```console
    ./scripts/ldsp.sh run "-p 512 -o line-out"
    ```
    #### Windows
    ```console
    .\scripts\ldsp.bat run "-p 512 -o line-out"
    ```
    <br>
    A few notes:
    
    - If the USB gets disconnected, the application will stop! If you don't want to depend on a tethered connection with a host computer and you're on macOS or Linux, you can swap the command *run* with *run_persistent* (still in progress on Windows). Alternatively, check the other options to run the application that are listed next.

    - Here, we are also passing some optional command line arguments/flags, i.e., we are setting the period size (number of audio frames) and we are asking the phone to output audio from the line-out/headphone jack. This is just an example and the arguments can be omitted if not needed.

    - If you remove the *-o line-out* argument, the phone will deafult to outputting audio from the built-in speaker.

        For a list of all command line arguments and default values simply pass *--help* or *-h* as argument. A very useful argument is *--verbose* or *-h*, which prints lots of useful info about the phone and the current settings!

- **Run through a remote shell**. From anywhere on your computer, open a *remote shell* via ADB:
    ```console
    adb shell
    ```
    Remember, this will open a shell on your phone! First things first, become *root*:
    ```console
    su
    ```
    Then move to the project's directory where the bin is located (in our case, the project is called *sine*):
    ```console
    cd /data/ldsp/projects/sine
    ```
    The bin is always called *ldsp*; run it, with or without arguments:
    ```console
    ./ldsp -p 512 -o line-out
    ```

    Much like the previous option, execution persists as long as the computer and the phone are connected via USB. This limitation is absent from the next options!

- **Run locally from your phone**. For this option, you need to install any terminal emulator app on your phone. Some examples are [Termux](https://play.google.com/store/apps/details?id=com.termux) and the good old [Terminal Emulator for Android](https://f-droid.org/packages/jackpal.androidterm/).

    Then, open it and follow **the same steps as for the previous option**, starting from becoming *root*. Notice that according to the method you used to root your phone, you might be asked to grant root permissions to the terminal app.

- **Run remotely over WiFi**. This is a more advanced option that may not work on very old phones/Android versions. 

    ABD allows for untethered connection via TCP when computer and phone are connected to the same WiFi network (including any hostpost generated by the phone itself). You can find instructions on how to set up this connection here:
    [https://developer.android.com/tools/adb#wireless](https://developer.android.com/tools/adb#wireless)

    Once you are connected, you can use either the first or the second option to run LDSP applications seamlessly, even if no USB cable is attached!


<br>

### 5. Stop
[UNDER CONSTRUCTION]


## How to design your own LDSP application
[UNDER CONSTRUCTION]

## How to use LDSP with Pure Data
[UNDER CONSTRUCTION]

[Previous: Phone Configuration](3_phone_config.md)