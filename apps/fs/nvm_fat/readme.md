---
parent: Harmony 3 driver and system service application examples for SAM RH707 family
title: FAT filesystem using NVM Media 
has_children: false
has_toc: false
---

[![MCHP](https://www.microchip.com/ResourcePackages/Microchip/assets/dist/images/logo.png)](https://www.microchip.com)

# FAT filesystem using NVM Media

This application shows an example of implementing a FAT disk in the device internal Flash memory.

## Description

### File System Operations on NVM:

- Application places a FAT disk image consisting of a Master Boot Record (MBR) sector, Logical Boot Sector, File allocation Table, and Root Directory Area in the internal Flash memory (NVM)

- During execution, the application first opens an existing file named **FILE.TXT** and performs following file system related operations:
    - **SYS_FS_FileStat**
    - **SYS_FS_FileSize**
    - **SYS_FS_FileSeek**
    - **SYS_FS_FileEOF**

- It tries to read the file and checks if string **"Data"** is present. If present it continues to next step or it fails the application

- Finally, the string **"Hello World"** is written to this file. The string is then read and compared with the string that was written to the file. If the string compare is successful, An LED indication is provided

### File system layer uses:

- Memory driver to communicate with underlying NVM media

## Downloading and building the application

To clone or download this application from Github, go to the [main page of this repository](https://github.com/Microchip-MPLAB-Harmony/core_apps_sam_rh707) and then click Clone button to clone this repository or download as zip file.
This content can also be downloaded using content manager by following these [instructions](https://github.com/Microchip-MPLAB-Harmony/contentmanager/wiki).

Path of the application within the repository is **apps/fs/nvm_fat/firmware** .

To build the application, refer to the following table and open the project using its IDE.

| Project Name      | Description                                    |
| ----------------- | ---------------------------------------------- |
| sam_rh707_ek.X | MPLABX project for [SAM RH707 Evaluation Kit](https://www.microchip.com/en-us/development-tool/SAMRH707F18-EK) |
|||

## Setting up the hardware

The following table shows the target hardware for the application projects.

| Project Name| Board|
|:---------|:---------:|
| sam_rh707_ek.X | [SAM RH707 Evaluation Kit](https://www.microchip.com/en-us/development-tool/SAMRH707F18-EK) |
|||

### Setting up [SAM RH707 Evaluation Kit](https://www.microchip.com/en-us/development-tool/SAMRH707F18-EK)

- Connect the debugger probe to J5 connector
- Connect the J1 (USB-C) port on the board to the computer using a USB-C cable

## Running the Application

1. Build and program the application using its IDE
2. Press Switch to start the file operations 
3. The LED is turned ON when the File **FILE.TXT** has the app data **"Hello World"** written in it 

Refer to the following table for switch and LED name:

| Board | Switch name | LED Name |
| ----- | -------- | --------- |
|  [SAM RH707 Evaluation Kit](https://www.microchip.com/en-us/development-tool/SAMRH707F18-EK) | PB0 | LED1 |
|||