# WCH Wristband

EN | [中文](README_zh.md)

> Smart Wristband Demo based on CH582M.
>
> BLE 5.2. Heart rate monitor, pulse-oxygen monitor, attitude angle detection, Touch-Key and WCH-Link debugger.
>
> Schematic and PCB are available for hardware reference. In addition, the basic sensor peripheral driver is also available.

![封面](image/封面.png)

### Description of files

------

- Hardware： Schematic and PCB files can be found in the wristband packet, opened via Altium Designer. And the library of schematic and PCB can be found in the pcblib packet.
- Firmware：CH582M firmware (IDE: MounRiver Studio).
- Doc：CH582M datasheet and manual of other peripherals.
- Pic：Picture generation tool and the inserted picture.

### Hardware
------

- CH582M: master MCU, a 32-bit RISC-V MCU, with BLE5.2
- ST7735 LCD: 0.96 inch HD IPS displayer, I2C DMA transfer
- MPU9250: 9 DOF IMU Sensor, with functions of wrist lifting detection and step counting
- MAX30102: an integrated pulse oxygen and heart-rate monitor module
- DRV2605: motor driver module, to implement vibration
- TTP223: Touchkey

### Software
------

- IDE: MounRiver Studio
- SPI I2C read/write functions
- Functions of each functional module



