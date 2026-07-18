# PROS Toggle Uploader

A small C++/Qt desktop app that:

- selects a VEX V5 PROS project folder;
- creates `include/app_features.hpp` from GUI checkboxes;
- runs `pros make`;
- runs `pros upload` to a connected V5 Brain;
- shows live compiler and upload output;
- stops before uploading when compilation fails.

## Important limitation

PROS builds a **project**, not an arbitrary standalone C++ file. Select the folder containing `project.pros` and the PROS Makefile. The GUI does not automatically discover or remove functions from your source code. Your robot code must use the generated macros around each optional subsystem.

## Requirements

- A working PROS CLI installation.
- Qt 6.5 or newer with the Widgets component.
- CMake 3.21 or newer.
- A C++17 compiler.
- Ninja is recommended but not required.

The app invokes the installed PROS CLI rather than reimplementing the PROS compiler and V5 USB protocol.

## Build on macOS

With Homebrew:

```bash
brew install qt cmake ninja
cd ProsToggleUploader
./scripts/build-macos.sh
open build/ProsToggleUploader.app
```

If the app cannot find `pros`, enter the full path to the PROS executable and press **Check PROS**. A GUI app may not inherit the same PATH as Terminal.

## Build on Windows

Install Qt 6 with the Qt Online Installer, install CMake and Ninja, then run from a Developer PowerShell:

```powershell
cd ProsToggleUploader
.\scripts\build-windows.ps1 -QtPrefix "C:\Qt\6.x.x\msvc2022_64"
.\build\ProsToggleUploader.exe
```

Use the actual Qt kit folder installed on your computer.

## Connect the toggles to the robot code

1. Open the app and select your PROS project folder.
2. Choose the enabled features.
3. Press **Build** once. This generates:

```text
<your project>/include/app_features.hpp
```

4. Include the generated header from your robot code:

```cpp
#include "app_features.hpp"
```

5. Guard optional code with the generated macros:

```cpp
void autonomous() {
#if ROBOT_FEATURE_AUTONOMOUS
    run_selected_autonomous();
#endif
}

void opcontrol() {
    while (true) {
#if ROBOT_FEATURE_DRIVE
        update_drive();
#endif

#if ROBOT_FEATURE_INTAKE
        update_intake();
#endif

        pros::delay(10);
    }
}
```

A fuller example is in `examples/pros_project/main.cpp`.

The generated macros are:

```text
ROBOT_FEATURE_AUTONOMOUS
ROBOT_FEATURE_DRIVE
ROBOT_FEATURE_INTAKE
ROBOT_FEATURE_LIFT
ROBOT_FEATURE_PNEUMATICS
ROBOT_FEATURE_ODOMETRY
ROBOT_FEATURE_DEBUG_LOGGING
```

Each value is `1` or `0`. The generated header also exposes `robot_features::drive`, `robot_features::intake`, and similar `constexpr bool` values.

For debug output:

```cpp
ROBOT_DEBUG_LOG("Position: %f\n", position);
```

The call compiles to nothing when **Debug logging** is disabled.

## Uploading

Connect the V5 Brain by USB, close software that may already be using the serial port, and press **Build + Upload**. The app executes these commands in the selected project folder:

```text
pros make
pros upload
```

**Upload Existing** only uploads the last successful build; changing checkboxes has no effect until another build is run.

Use the optional argument fields for flags supported by your installed PROS CLI. Arguments are passed directly without a shell.

## Changing the available toggles

Edit `kFeatureSpecs` in `src/MainWindow.hpp`, then rebuild this desktop app. Add matching `#if` blocks to the PROS robot project.

## Troubleshooting

- **Could not start PROS:** set the executable box to the full path of `pros` or `prosv5`.
- **Not a PROS project:** select the project root, not the `src` folder.
- **USB/COM port access denied:** close VEXcode, VEX Device Utility, another PROS terminal, or any program using the Brain's port.
- **Build succeeds but toggles do nothing:** include `app_features.hpp` and add the required `#if` blocks.
- **Upload fails:** run the same printed `pros upload` command in a terminal to see whether the issue is the PROS installation, cable, V5 Brain, permissions, or project.
