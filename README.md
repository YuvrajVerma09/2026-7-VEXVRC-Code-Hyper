# 2026-7-VEXVRC-Code-Hyper
Primary Features
Custom-built & tuned PID Controller (integrated with IMU, IME and dedicated odometry encoders) for autonomous routines
Multi-mode operator drivetrain control with easy switching between Arcade, Tank, Curved Arcade including built-in dual sigmoid sensitivity smoothing curves
Thread-independent concurrent task scheduling system eliminates race conditions when working with our component system
Component-based class system (together with a plethora of base abstract classes) also provides easier programming of newly engineered subsystems
More coming soon further into the season!

Included Libraries
Created with PROS API as this has better documentation than the official VEX API.
Uses nlohmann/json to save diagnostic data to disk.
C++20 Standard Library as provided by ARM GCC
Outside of PROS, our code uses no external VEX helper library such as Lemlib to write our code - all features are custom-built entirely in-house.
Project Structure
To run the Makefile, first install ARM G++.

Main file at src/main.cpp.
Includes in include/main.h.
Options (e.g. motor ports) in include/options.h
