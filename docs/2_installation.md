# INSTALLATION
## How to install LDSP on your computer

- **Clone the repo and all the submodules**: open a shell in the folder where you want to clone the GitHub repository. Choose a convenient location, since here you will find code examples and it is also where you will likely write your own code/projects.

  Clone the repository from Github:
  ```console
  git clone --recurse-submodules https://github.com/victorzappi/LDSP.git
  ```
<br>

- **Export the NDK var**: finally, export a new environment variable called **NDK**, with the path to the actual content of the Android NDK you downloaded from the dependecies list. Specifically, export the full path that points to where the **toolchains** and **sources** folders can be found (i.e., path to the folder that contains them). This allows LDSP to use the toolchain and the C++ run-time!

    We recommend you make the variable persistent, otherwise you will have to export it again every time you open a new shell to build an LDSP application. This can be done on [Linux](https://stackoverflow.com/a/13046663), [macOS](https://support.apple.com/guide/terminal/use-environment-variables-apd382cc5fa-4f58-4449-b20a-41c53c006f8f/mac) as well as [Windows](https://learn.microsoft.com/en-us/powershell/module/microsoft.powershell.core/about/about_environment_variables?view=powershell-7.3#saving-environment-variables-with-the-system-control-panel).

    You can check if the variable was successfully exported by openinig a shell and typing:
  ```console
  echo $NDK
  ```
  This should print the path to your NDK!

  
  [Previous: Dependencies](1_dependencies.md) | [Next: Phone Configuration](3_phone_config.md)