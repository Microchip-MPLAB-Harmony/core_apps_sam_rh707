---
parent: Harmony 3 driver and system service application examples for SAM RH707 family
title: SPI Driver asynchronous - Self loopback multi client 
has_children: false
has_toc: false
---

[![MCHP](https://www.microchip.com/ResourcePackages/Microchip/assets/dist/images/logo.png)](https://www.microchip.com)

# SPI Driver asynchronous - Self loopback multi client

This example demonstrates how to use the SPI driver in asynchronous mode to achieve self-loop back between multiple clients

## Description

- This example writes and reads back the same data (self loop back) for two different clients connected over the same SPI bus by using the multi client feature of the driver

- It uses the request (write and read request) queuing feature of the asynchronous driver and does not waste CPU bandwidth in waiting for previous request completion

- The example also demonstrates how to setup two different client transfers at different baud rates

- This example performs self loop back only once after a power on reset
- Success is indicated when a successful self loop back is reported by both the clients
- After the loop back test is complete, the application remains in the idle state

## Downloading and building the application

To clone or download this application from Github, go to the [main page of this repository](https://github.com/Microchip-MPLAB-Harmony/core_apps_sam_rh707) and then click Clone button to clone this repository or download as zip file.
This content can also be downloaded using content manager by following these [instructions](https://github.com/Microchip-MPLAB-Harmony/contentmanager/wiki).

Path of the application within the repository is **apps/driver/spi/async/spi_self_loopback_multi_client/firmware** .

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

- Use jumper wire to Connect "Pin 16 of J18 header" to "Pin 17 of J18 header" 
    - FLEXCOM3_IO0 signal as MOSI signal and it is mapped to PB22 that is routed to "Pin 16 of J18 header" 
    - FLEXCOM3_IO1 signal as MISO signal and it is mapped to PB21 that is routed to "Pin 17 of J18 header" 
- Connect the debugger probe to J5 connector
- Connect the J1 (USB-C) port on the board to the computer using a USB-C cable

## Running the Application

1. Build and program the application using its IDE
2. The LED is turned ON on Success

Refer to the following table for LED name:

| Board | LED Name |
| ----- | -------- |
|  [SAM RH707 Evaluation Kit](https://www.microchip.com/en-us/development-tool/SAMRH707F18-EK) | LED0 |
|||
