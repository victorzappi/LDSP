# PHONE CONFIGURATION
## How to configure (hack!) your phone to work with LDSP

As discussed earlier ([Introduction](0_introduction.md)), the foremost requirement consists of rooting the phone. The XDA forum is the best place to look for guides/how-tos and get help:
[https://forum.xda-developers.com/](https://forum.xda-developers.com/)

Then, to configure a phone, we need to populate a hardware configuration file, called ***ldsp_hw_config.json***, that provides LDSP with the info necessary to access low-level audio, effectively bypassing Android’s sandboxed audio stack. Every model requires a specific hardware configuration file; in the next sections of this file we will explain how to generate one for your phone! 

>**Note:** a list of pre-generated files is available in the repo, at *[LDSP/phones/](../phones)*. If your phone is in the list, you're all set and you can skip to SECTION...

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

### 1. Phone Details Entries
The first three entries regard some general phone details and are:
```json
"device": "", ---> name of the device
"target architecture": "", ---> 32 bits (armv7a) or 64 bits (aarch64)
"supports neon floating point unit": ""  ---> true or false
```
They can be easily retrieved via one of the LDSP scripts (*ldsp_phoneDetails.sh*). To run the script on the phone, simply open a **shell** on your computer (i.e., a 'local' shell), navigate to the **LDSP folder** and type:

#### macOS and Linux
```console
./scripts/ldsp.sh phone_details
```

#### Windows
```console
.\scripts\ldsp.bat install_scripts
```

>**Note:** alternatively, you can run all LDSP scripts directly from the phone; just open a shell on the phone via `adb shell` (i.e., a 'remote' shell) and run the scripts directly from */data/ldsp/scripts*, with the command `sh ./ldsp_phoneDetails`. You're more than welcome to peek into the source code of the scripts too! Sometimes they look 'weird', but this is due to the fact that many Android phones ship with only a minimal set of standard command-line utilities and ```grep``` is most often the way to go.

The output should look something like this (for example, on a Moto G7 phone):
```console
Entries for ldsp_hw_config.json:
	device: motorola moto g(7)
	target architecture: aarch64
	supports neon floating point unit: true
Android version: 14

```
You can now copy the first three entries in your configuration file, like this:

```json
"device": "motorola moto g(7)",
"target architecture": "aarch64",
"supports neon floating point unit": "true"
```
The name of the device is arbitrary, meaning that you can choose whatever you want (: But the script returns what found in the phone's properties.

The last line outputted from the script is not needed for the configuration file, but it will come in handy when running the LDSP applications (see [usage doc](4_usage.md)).

<br>

### 2. Mixer Settings
Every Android phone is equipped with a hardware **audio mixer**. This device carries out a variety of important tasks, including routing various inputs and outputs (e.g headphones versus speakers, long- versus short-range microphones) and adjusting gain levels. 

Under standard operating conditions, user applications cannot access it—and here comes hacking! We need to source all the info necessary for LDSP to access and properly operate the mixer. This requires a few steps, all represented by some entries in the configuration file and outlined in the next subsections.

#### 2a. Mixer Paths XML File



[UNDER CONSTRUCTION]

 [Previous: Installation](2_installation.md) | [Next: Usage](4_usage.md)