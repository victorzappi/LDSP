# PHONE CONFIGURATION
## How to configure (hack!) your phone to work with LDSP

As discussed earlier ([Introduction](0_introduction.md)), the foremost requirement consists of rooting the phone. The XDA forum is the best place to look for guides/how-tos and get help:
[https://forum.xda-developers.com/](https://forum.xda-developers.com/)

Then, to configure a phone, we need to populate a hardware configuration file, called ***ldsp_hw_config.json***, that provides LDSP with the info necessary to access low-level audio, effectively bypassing Android’s sandboxed audio stack. Every model requires a specific hardware configuration file; in the next sections of this file we will explain how to generate one for your phone! 

>**Note:** a list of pre-generated files is available in the repo, at *[LDSP/phones/](../phones)*. If your phone is in the list, you're all set and you can skip to SECTION...?

The process is a bit challenging but fun (it really feels like hacking!), and it will allow you to better understand how audio works on Android. 

As briefly mentioned in the introduction, unfortunately not all Android builds can be hacked, meaning that some of the reasorces that should be pointed out in the configuration file are not present/available on the phone. This guide will show you how to spot these unfortunate edge cases too.
 


<br>

### 0. Template and Scripts

A template hardware configuration file can be found in [LDSP/phones/_template](../phones/_template). Its a JSON file (*ldsp_hw_config.json*) with several entries, but only a subset are mandatory, namely those that are not surrounded by square brakets. In this guide, we will cover for the most part mandatory entries. For a detailed explanation of all entries, please refer to [LDSP/phones/_template/ldsp_hw_config_README.md](../phones/_template/ldsp_hw_config_README.md) [UNDER CONSTRUCTION]. All the entries are represented by JSON key-value pairs, yet values are all empty in the template. 

Before start populating the entries with the info about your phone, we should:

- **Create a new folder** where to store the new configuration file. We recommend following the same logic we are using in the repo, i.e., in [LDSP/phones/](../phones/) create a parent folder named after your phone's vendor (or simply use an existing one if there already), then a subfolder named after the model. We put the *codename* of the model in parenthesis—e.g., *Moto G7 (river)*—but you can skip this detail. Most codenanames can be sourced here: [wiki.lineageos.org/devices/](wiki.lineageos.org/devices/)

- **Make a copy of the template** in the newly created folder and simply call it *ldsp_hw_config.json*.

- **Install LDSP scripts on the phone:** to ease out the process of gathering info/fill out the entries of the file (hence facilitating hacking), we created a set of convenient 'hacking' scripts (*LDSP scripts*) that are designed to run on the phone. To install them:
    - Open a **shell** on your computer (on Windows, we recommend PowerShell) and navigate to the **LDSP folder**, where you downladed the content of the repo.
    - We'll use the main *ldsp script* to install the LDSP scripts to the phone (more details about the main script in [the next doc page](4_usage.md)). Make sure that your phone is connected via USB and then type:
            
        #### macOS and Linux
        ```console
        ./scripts/ldsp.sh install_scripts
        ```

        #### Windows
        ```console
        .\scripts\ldsp.bat install_scripts
        ```
    While all the LDSP scripts are supposed to run **on the phone**, we will invoke them via the main *ldsp script* that is part of your local LDSP installation. This script will in turn access the phone and run the scripts remotely. 

<br>

### 1. Phone Details
The first two entries regard some general phone details and are:
```json
"device": "", ---> name of the device
"target architecture": "", ---> in most cases aarch64 (64 bits) or armv7a (32 bits)
```
They can be easily retrieved via one of the LDSP scripts (*ldsp_phoneDetails.sh*). To run the script on the phone, simply open a **shell** on your computer (i.e., a 'local' shell), navigate to the **LDSP folder** and type:

#### macOS and Linux
```console
./scripts/ldsp.sh phone_details
```

#### Windows
```console
.\scripts\ldsp.bat phone_details
```

>**Note:** alternatively, you can run all LDSP scripts directly from the phone; just open a shell on the phone via `adb shell` (i.e., a 'remote' shell) and run the scripts directly from */data/ldsp/scripts*, with the command `sh ./ldsp_phoneDetails`. You're more than welcome to peek into the source code of the scripts too! Sometimes they look 'weird', but this is due to the fact that many Android phones ship with only a minimal set of standard command-line utilities and ```grep``` is most often the way to go.

The output should look something like this (for example, on a Moto G7 phone):
```console
Entries for ldsp_hw_config.json:
	device: motorola moto g(7)
	target architecture: aarch64
Additional info:
    NEON floating point unit present
    Android version: 14

```
You can now copy the first two entries in your configuration file, like this:

```json
"device": "motorola moto g(7)",
"target architecture": "aarch64",
```
The name of the device is arbitrary, meaning that you can choose whatever you want (: But the script returns what found in the phone's properties.

The last two lines outputted from the script ("Additional info") are not needed for the configuration file, but provide more insights about the capabilities of the phone; additionally, the Android version will come in handy when building the LDSP applications (see [usage doc](4_usage.md)).

<br>


### 2. Mixer Settings
Every Android phone is equipped with a hardware **audio mixer**. This device carries out a variety of important tasks, including routing various inputs and outputs (e.g headphones versus speakers, long- versus short-range microphones) and adjusting gain levels. 

**LDSP applications are designed to output audio to the mixer (plaback) and input audio from the mixer (capture)**, directly. Under standard operating conditions, user applications cannot access it—and here comes hacking! We need to source all **the info necessary for LDSP to access and properly operate the mixer**. This requires a few steps, all represented by some entries in the configuration file and outlined in the next subsections.

- ### 2a. Mixer Paths XML File
    While carrying out the same general operations, the audio mixer of each phone works slightly different. This means that the commands that LDSP applications have to dispatch to it to control audio are hardware dependent. On Android, the full list of commands tailored for the underlying hardware mixer are stored in an XML file, generally called *mixer_paths.xml*. The first *mixer settings* entry in our configuration file is the **location of this XML file within the file system**:

    ```json
    "mixer settings":
    {
        "mixer paths xml file": "",
        ...
    ```
    The location may vary, depending on vendor and model. LDSP comes with a script that helps us find it!

    Open a **local shell**, navigate to the **LDSP folder** and type:

    #### macOS and Linux
    ```console
    ./scripts/ldsp.sh mixer_paths
    ```

    #### Windows
    ```console
    .\scripts\ldsp.bat mixer_paths
    ```

    The script searches two common directories where vendors typically place mixer paths files—*/etc/* and */vendor/etc*—and, if successful, it will return the full path of the file. For example:
    ```console
    ldsp.sh mixer_paths
    Searching mixer paths files in /etc/
    No mixer paths files found

    Searching mixer paths files in /etc/vendor
    Mixer paths files found:
        /etc/vendor/mixer_paths.xml
    ```
    The path can then be copied into the XML entry:

    ```json
    "mixer settings":
    {
        "mixer paths xml file": "/etc/vendor/mixer_paths.xml",
        ...
    ```
    Notice that the name of the file may be different than simply *mixer_paths.xml*.

    Once the mixer paths file is found, it is convenient to **copy it over your host computer**, because in the [next step](#2b-playback-and-capture-paths) it is necessary to analyze its content. You can use `adb` and ask it to pull the file to any local folder, for example:
    ```console
    adb pull /etc/vendor/mixer_paths.xml ~/Downloads
    ```


    If **no mixer paths file is found in the default directories**, you can specify a custom one as a command line argument for the script. Additionally, LDSP comes with a **recursive version of the script** that searches in the specified directory and all its subdirectories. Here is, for example, how to search recursively in */vendor/etc* (another common dir) on macOS and Linux: `./scripts/ldsp.sh mixer_paths_recursive /vendor/etc` (the script can be called on Windows too, via `ldsp.bat`). And here is the example output on the Moto G7:
    ```console
    Searching for mixer XML files in /vendor/etc
    Found mixer paths file: /vendor/etc/mixer_paths.xml
    Found mixer paths file: /vendor/etc/mixer_paths_madera_epout.xml

    ```
    > #### What if more than one mixer paths file is found as in this case?
    > Simply choose one from the list, copy its path in the XML entry and pull the file to your computer. The analysis at the [next step](#2b-playback-and-capture-paths) will unveil if you chose the correct one!

    > #### What if no mixer paths file is found, even when searching recursively in the phone's root directory ( */* )?
    > Unfortunately, this means that the Android version running on this phone does not comply with Google's audio standards and is <strong style="color: red;">not compatible with LDSP ):</strong>



- ### 2b. Playback and Capture Paths
    The next portion of the mixer settings within the configuration file pertains to the activation of mixer routes. Specifically, it is designed to allow LDSP to route the mixer's output (**playback**) to physical destinations (speaker, headphone/line out, or headset) and how to route input from physical sources (long-range mic, line in, or headset/short-range mic) into the mixer (**capture**). 
    ```json
    "playback path names":
    { 
        "speaker": "",
        "line-out": "",
        "handset": ""
    },
    "capture path names":
    {
        "mic": "",
        "line-in": "",
        "handset": ""
    }
    ```

    In Android, these routes are called **paths**, hence the names of the entries to fill in. Here is a visual depiction of generic playback routes/paths:
    <img src="images/mixer_playback_paths.png" alt="Alt text" width="500"/>

    >**Note:** the headphone socket of all phones supports *combo jacks*, hence it works as **both stereo output (for headphones) and mono input (for wired mics)**! Combo headphones (which combine headphones and a wired mic) allow to use this socket in both directions at the same time.

    As the name suggests, the *mixer paths* XML file contains all the commands necessary to activate the paths. In most cases, a single path requires that a sequence of commands is invoked on the mixer. For this reason, the XML file is organized in *path* elements, each containing several commands and characaterized by a name that reflects the nature of the path. Here are the three main playback path elements found in the Moto G7's *mixer_paths.xml* file:
    ```xml
    <path name="speaker">
        <ctl name="SLIM_0_RX Channels" value="One" />
        <ctl name="SPK AMP PCM Gain" value="18" />
        <ctl name="AIF1TX1 Input 1 Volume" value="36" />
        <ctl name="SPK PCM Source" value="DSP" />
        <ctl name="AIF1TX1 Input 1" value="SLIMRX1" />
        <ctl name="SPK AMP Enable Switch" value="1" />
    </path>

    <path name="headphones">
        <ctl name="SLIM_5_RX Channels" value="Two" />
        <ctl name="ISRC2INT1 Input 1" value="SLIMRX5" />
        <ctl name="HPOUT1L Input 1" value="ISRC2INT1" />
        <ctl name="ISRC2INT2 Input 1" value="SLIMRX6" />
        <ctl name="HPOUT1R Input 1" value="ISRC2INT2" />
        <ctl name="HPOUT1 Digital Volume" id="0" value="120" />
        <ctl name="HPOUT1 Digital Volume" id="1" value="120" />
        <ctl name="HPOUT1 Digital Switch" id="0" value="1" />
        <ctl name="HPOUT1 Digital Switch" id="1" value="1" />
    </path>

    <path name="handset-spkout">
        <ctl name="SPKOUT Input 3" value="None" />
        <ctl name="Speaker Digital Volume" value="160" />
        <ctl name="SPKOUT Input 1" value="SLIMRX1" />
        <ctl name="Speaker Digital Switch" value="1" />
    </path>
    ```
    The names of these three paths—which are device dependent—must be **copied in the playback path names entries** of the configuration file, like this:
    ```json
    "playback path names":
    { 
        "speaker": "speaker",
        "line-out": "headphones",
        "handset": "handset-spkout"
    }
    ```
    This allows LDSP to activate whatever playback destination on the phone at run time (see [usage doc](4_usage.md))! 

    Likewise, the phone's **XML file needs to be analyzed in search of the three capture paths** and their names must be ported to the configuration file, to allow for runtime capture control. Here are the capture paths for the Moto G7:
    ```json
    "capture path names":
    {
        "mic": "speaker-mic",
        "line-in": "headset-mic",
        "handset": "handset-mic"
    }
    ```
    > #### What if my phone does not come with an headphone socket (line in/out)?
    > In that case, the *line-out* and *line-in* paths are not present in the mixer, hence you can leave those configuration entries empty. You can though leverage line input/output via a USB-to-headphones adapter. Effectively, the adapter works as an external USB audio interface and LDSP supports this configuration (see [usage doc – USB audio](4_usage.md#what-if-i-want-to-use-a-usb-to-headphones-adapter-or-an-external-usb-audio-interface))!

    #### How to Recognize the Paths within the XML File
    We can open the XML file that we pulled from the phone with any text editor. Yet, finding the right paths to port to configuration file entries **may not be straighforward**! 
    
    In general, mixer paths XML files contain a huge number of paths that describe all the audio functionalities of the phone, including those that go beyond the current scope of LDSP (e.g., calls, VoIP, Bluetooth). While some path names may ring a bell (e.g., *speaker*, *headset-mic*), in many cases vendors use pretty obfuscated naming conventions, making it difficult to spot the right paths. For example, although counterintuitive, the term 'record' is almost certainly not referring to any mic or line-in paths (see SECTION...?)!

    The quickest way to recnognize which paths shold be ported to the configuration file consists of *sniffing* the ones used by Android audio apps across the different playback and capture configurations. This hacking process is quite simple and leverages `logcat`, a command-line utility included in Android.

    By running `logcat` on the phone, it is possible to see all the debug information printed within Android in real-time (the 'log'). These often include the names of the paths currently in use! The process is the following.

    - Open an audio app capable of both capture and playback, for example the audio recorder app often installed on stock Android versions (you could also use two separate apps, one for capture and the other for playback). **For now do not record nor play anything**.
    
    - Make sure **headphones are not connected** to the phone, for we are going to target *mic* and *speaker* paths first. 

    - Open a remote shell via `adb shell`, get super-user permissions with `su` and clear the log by typing:
        ```console
        logcat -c
        ```
    - Start monitoring the log via:
        ```console
        logcat
        ```
    - We start with the ***mic* path**. First record a sound using the built-in mic and then stop. You should see several debug printouts popping on the screen. It is convenient to stop the log, either by pressing *Ctrl+c* or by disconnecting the phone from USB.
    
    - In the log, look for lines that include the keywords *audio_hw_primary* and *snd_device*; *path* can help too, but it might be misleading on its own(see the next note). For example, the Moto G7 logs:
        ```console
            ...
            06-24 09:48:24.294  3580 12646 D audio_hw_primary: enable_snd_device: snd_device(91: speaker-mic)
            06-24 09:48:24.294  3580 12646 D audio_route: Apply path: speaker-mic
            ...
        ```
        ---> From this log, we can deduce that a path called **speaker-mic** was activated on the mixer when we recorded—that is our path name for the *mic* entry! We should be able to find this path within the XML file too.

        >**Note:** checking **the correct keywords** is of utmost importance! Most phones activate additional audio paths within the mixer when capture and playaback are set up (see the next mixer settings subsections), all logged with the tag *audio_hw_primary* and alongside the *path* keywords, but with different additional keywords like *audio_route*.
        
    <br>

    - Now, to find the ***speaker* path**, open a **new remote shell** and become superuser. While a new shell is not mandatory, it clears up the screen from the previous log making our life easier! 
    
    - Clean and restart the log:
        ```console
        logcat -c
        logcat
        ```
    
    - Then play the recording back on the app and analyze the log, looking for the same keywords. On the Moto G7:
        ```console
        06-24 10:43:44.950  3580 15569 D audio_hw_primary: enable_snd_device: snd_device(2: speaker)
        06-24 10:43:44.950  3580 15569 D audio_route: Apply path: speaker
        ```
        ---> In this case, the name of the *speaker* path is simply ***speaker***!

    <br>
    The same process can be repeated:
    
    - **To fill *line-in* and *line-out* paths**, with combo headphones plugged in (those equipped with a line-in mic).

    - **To fill *handset* and *handset-mic* paths**, with no headphones but while operating the phone close to the ear (the proximity sensor should switch configuration automatically).

    > #### What if the paths referenced in the log are not present in the XML file?
    > This very likely means that your phone has more than one mixer paths XML files and you chose the wrong one! Go back to the [XML file selection step](#2a-mixer-paths-xml-file), find/choose a different file and try again the whole 'sniffing' process (;


- ### 2c. Device Activation
    Next in the mixer settings portion of the configuration file is the **activation of the playback and capture *devices***.
    ```json
    "[mixer playback device activation]": "",
	"[mixer capture device activation]": ""
    ```
    These entries are in square brackets—this means that they are not mandatory! Yet, they are need on the majority of the phones we tested. 

    Every Android phone comes with an embedded audio *card* that manages the streams going to and coming from the mixer. Yet, given the wide variety of audio related tasks that phones must carry out, this card is often equipped with a large selection of *audio devices*, i.e., subcomponents of the card each designed for a specific usecase (e.g., media playback, calls, notifications). 
    
    While LDSP applications can choose which (card and) playback/capture devices to use at runtime (see [usage doc – Run LDSP](4_usage.md#run)), these choices need to be reflected in the mixer, typically via the activation of related paths. Here is a depiction of the playback devices of a generic card:
    <img src="images/device_playback_paths.png" alt="Alt text" width="500"/>

    If your phone's is equipped with a single playback device and a single capture deivce, then these paths are not required and the device activation entries can be left empty, or they can even be completely removed from the configuration file.

    script ldsp_audioDevices.sh...


     


[UNDER CONSTRUCTION]

 [Previous: Installation](2_installation.md) | [Next: Usage](4_usage.md)