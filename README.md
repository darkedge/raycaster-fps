# Project structure
Folders:
* 3rdparty - Contains submodules
* assets - Binary resources
* doc - Documentation
* linux - Linux project files
* src - Source code
* tools - Asset pipeline binaries
* tracy - Tracy submodule
* vs - Windows project files

1. When doing a fresh checkout, sync all submodules by running 3rdparty/sync_submodules.ps1 (Windows), or run the command manually:

    ```git submodule update --init --recursive```

2. Manually copy SDL.dll from 3rdparty/SDL2-2.0.12/lib/x64 to the output directory

# Graphics conventions
* Left-handed Y-up coordinate system
* NDC range: [0, +1]
