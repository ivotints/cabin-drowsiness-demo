# Cabin Drowsiness Demo

C++ / OpenCV demo for automotive-style in-cabin monitoring.

## What it does

This project uses a webcam as a simplified cabin camera and demonstrates:

- face detection
- eye detection
- basic drowsiness state machine
- blink counting
- warning overlay
- real-time video processing

The goal is not production-grade accuracy. The goal is to show:
- embedded-style design
- perception pipeline thinking
- state-based decision logic
- testable C++ code
- real-time frame processing

## Why this project exists

This demo is meant as interview preparation for automotive embedded software roles.

It can be presented as a prototype of:
- DMS: Driver Monitoring System
- cabin monitoring
- perception + decision pipeline
- comfort/safety-related software

## Tech stack

- C++17
- OpenCV
- Linux (Arch)
- webcam
- CMake

## Build on Arch Linux

Install dependencies:

```bash
sudo pacman -S --needed base-devel cmake gcc opencv pkgconf