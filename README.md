# LDSP

A C++ software environment to hack your Android phone and program it as a low-level embedded audio board.

## Table of Contents

- [Introduction](docs/0_introduction.md)
- [Dependencies](docs/1_dependencies.md)
- [Installation](docs/2_installation.md)
- [Phone Configuration](docs/3_phone_config.md)
- [Usage](docs/4_usage.md)

## In a Nutshell
LDSP enables developers and creatives to **design interactive audio applications entirely in C++** (or Pure Data). The outcome **is not an Android app**, but a command line binary file, similar to those used on Linux computers. However, like apps, LDSP applications have access to audio input/output and can interface with sensors and other goodies including the touchscreen and the vibration control.

LDSP is not intended to replace the Android app workflow and ecosystem. Instead, it is a thought-provoking project that allows us to repurpose our phones—both new and old—into dedicated audio boards.

The main strengths of LDSP lie in its ability to better leverage the **computational power** of Android phones by bypassing several internal Android audio layers, and the great freedom it provides in terms of **development environment** (no Android Studio required!). However, it does require the phone to be *rooted* along with some manual configuration, which really feels like *hacking*.

## More?
You can find more about LDSP's technical specfications, the objectives and the ethos of the project in the following publications:

- @inproceedings{tapparo2023leveraging,
    title={Leveraging Android Phones to Democratize Low-level Audio Programming},
    author={Tapparo, Carla Sophie and Chalmers, Brooke and Zappi, Victor},
    booktitle={Proceedings of the International Conference on New Interfaces for Musical Expression},
    year={2023}
  }<br>
  [https://www.nime.org/proceedings/2023/nime2023_41.pdf](https://www.nime.org/proceedings/2023/nime2023_41.pdf)

- @inproceedings{zappi2023upcycling,
    title={Upcycling Android Phones into Embedded Audio Platforms},
    author={Zappi, Victor and Tapparo, Carla Sophie},
    booktitle={Proceedings of the 26th International Conference on Digital Audio Effects},
    year={2023}
  }<br>
  [https://www.dafx.de/paper-archive/2023/DAFx23_paper_44.pdf](https://www.dafx.de/paper-archive/2023/DAFx23_paper_44.pdf)

- @inproceedings{zappi2025linux,
  title={Linux Audio on Android},
  author={Zappi, Victor},
  booktitle={Proceedings of the 19th Linux Audio Conference},
  year={2025}
}<br>
[https://hal.science/hal-05096046/document](https://hal.science/hal-05096046/document)




