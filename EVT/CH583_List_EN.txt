
Nanjing Qinheng Microelectronics Co., Ltd. 2022.03

http://wch-ic.com


Directory
  |
  |-- CH583: Bluetooth Low Energy - 32-bit RISC-V MCU
  |      |-- CH583 Android OTA Update Tool V1.1: APP update tool and source of CH583 OTA related software routines
  |      |-- EVT: CH583 related software routines
  |      |      |-- EXAM: 
  |      |      |      |-- SRC  
  |      |      |      |      |-- Ld: link file
  |      |      |      |      |-- RVMSIS: kernal system header file
  |      |      |      |      |-- Startup: CH58x startup file
  |      |      |      |      |-- StdPeriphDriver: basic peripheral driver source file and header file
  |      |      |      |-- ADC: ADC sampling routines, including temperature detection, single channel detection, differential channel detection, TouchKey detection and interrupt sampling
  |      |      |      |-- FLASH: on-chip flash routines: erase/read/write Code area and DataFlash area
  |      |      |      |-- FreeRTOS: FreeRTOS porting routine
  |      |      |      |-- I2C: I2C routines, master/slave mode, data transceiver
  |      |      |      |-- IAP
  |      |      |      |      |-- APP: APP program routine used with IAP 
  |      |      |      |      |-- USB_IAP: Routine to update on-chip program via USB 
  |      |      |      |      |-- UART_IAP: Routines to update the on-chip program via UART 
  |      |      |      |      |-- WCHMcuIAP_WinAPP: IAP PC tools and source code
  |      |      |      |-- PM: Syetem sleep mode and wake-up routine: GPIOA_5 as wake-up source, four power consumption levels
  |      |      |      |-- PWMX: PWM4-11 output routine
  |      |      |      |-- SPI0: SPI0 routine, master/slave mode, data transceiver
  |      |      |      |-- TMR: Timer routine  
  |      |      |      |-- UART1: UART1 receive/transmit routine
  |      |      |      |-- USB
  |      |      |      |      |-- Device
  |      |      |      |      |      |-- COM£ºUSB simulate CDC device routines
  |      |      |      |      |      |-- VendorDefinedDev: simulate custom USB devices (CH372 device) routine, provide eight non-zero channels (upload + download), implement that data is first downloaded, and then the data content is reversed and uploaded
  |      |      |      |      |      |-- CompoundDev: simulate keyboard/mouse routine, support HID class commands 
  |      |      |      |      |      |-- CompoundU2Dev: USB2 simulate keyboard/mouse routine, support HID class commands 
  |      |      |      |      |      |-- HID_CompliantDev£ºUSB simulate HID compliant device routines
  |      |      |      |      |-- Host
  |      |      |      |      |      |-- HostEnum:  simple enumeration procedure routine for USB devices
  |      |      |      |      |      |-- HostU2Enum: USB2 simple enumeration procedure routine for USB devices
  |      |      |      |      |      |-- HostAOA: USB host application routine, support connect to Android devices and communicate with APP
  |      |      |      |      |      |-- U_DISK: U disk file system routine
  |      |      |      |      |      |      |-- EXAM1.C: C Programming Language exam, read/write file in bytes, including creating files, deleting files, modifying file attribute, and modifying file name  
  |      |      |      |      |      |      |-- EXAM10.C: C Programming Language exam, including creating files, deleting files, modifying file attribute, and modifying file name  
  |      |      |      |      |      |      |-- EXAM11.C: C Programming Language exam, enumerate files in the root directory or specified directory  
  |      |      |      |      |      |      |-- EXAM13.C: C Programming Language exam, create files with long file name  
  |      |      |      |      |      |-- USB_LIB: U disk file system library file
  |      |      |      |-- BLE
  |      |      |      |      |-- Broadcaster: broadcaster routine, always broadcast when in broadcast status
  |      |      |      |      |-- CyclingSensor: cycling sensor routine, upload speed and cadence regularly after connected to the host
  |      |      |      |      |-- CentPeri: central-peripheral routine, integrate the function of central routine and peripheral routine and run simultaneously
  |      |      |      |      |-- Central: central routine, actively scan surrounding devices, conect to the specified peripheral address, search custom service and characteristic, execute read/write commands, peripheral routine is needed, and modify the peripheral address to the routine target address, (84:C2:E4:03:02:02) by default
  |      |      |      |      |-- HeartRate: heart rate routine, upload heart rate regularly after connected to the central
  |      |      |      |      |-- Peripheral: peripheral role routine, custom including five services with different  attributes, including read attribute, write attribute, notify attribute, read/write attribute, and safe and readable attribute
  |      |      |      |      |-- peripheral_ancs_client: peripheral role routine, including Apple Notification Center Service
  |      |      |      |      |-- RunningSensor: running sensor routine,upload rate regularly after connected to the central 
  |      |      |      |      |-- HID_Keyboard: BLE keyboard routine, simulate a keyboard device, upload key value regularly after connected to the central
  |      |      |      |      |-- HID_Mouse: BLE mouse routine, simulate a mouse device, upload key value regularly after connected to the central
  |      |      |      |      |-- HID_Consumer: BLE consumer routine, simulate user control device, upload volume key down key regularly after connected to the central
  |      |      |      |      |-- HID_Touch: BLE touch routine, simulate a touch pencil device,  upload touch value regularly after connected to the central
  |      |      |      |      |-- MultiCentral: multicentral routine, actively scan surrounding devices, conect to the specified three peripheral addresses, search custom services and characteristics, execute read/write commands, peripheral routine is needed, and modify the peripheral address to this routine target address, the addresses of these peripherals are (84:C2:E4:03:02:02), (84:C2:E4:03:02:03) and (84:C2:E4:03:02:04) by default
  |      |      |      |      |-- Observer: observer routine, scan regularly, print the scanned broadcast address if the scanning result is not empty
  |      |      |      |      |-- Direct_Test_Mode£ºDTM test routine, combined with RF test tools
  |      |      |      |      |-- RF_PHY: non-standard wireless transceiver routines
  |      |      |      |      |-- RF_PHY_Hop: non-standard wireless frequency hopping transceiver routine
  |      |      |      |      |-- MESH
  |      |      |      |      |      |-- adv_ali_light: Tmall Genie light routine, devices can be found and provisioned network via Tmall Genie, to control the switch state. By default, only switch attribute can be controlled. If other attributes (brightness, power, temperature, etc.) are needed, add the corresponding processing function and status report function according to the attribute description of the Alibaba Cloud product configuration.
  |      |      |      |      |      |-- adv_ali_light_add_lightness: MESH general attribute adding routine. On the basis of the Tmall Genie light routine, the brightness attribute is added, which is used to compare the original Tmall Genie light routine to quickly become familiar with the method of adding other MESH general attributes.
  |      |      |      |      |      |-- adv_ali_light_add_windspeed: Tmall definition attribute adding routine. On the basis of the Tmall Genie light routine, the wind speed attribute is added, which is used to compare the original Tmall Genie light routine to quickly become familiar with the method of adding other Tmall definition attributes.
  |      |      |      |      |      |-- adv_ali_light_multi_element: Multi-element fan lamp, which is used to compare the original Tmall Genie light routine to quickly become familiar with the method of adding multi-element Tmall definition attributes.
  |      |      |      |      |      |-- adv_ali_light_with_peripheral: Based on the Tmall Genie light routine, add brightness and color temperature controls, it supports the connection control of BLE debugging tool on the mobile phone.
  |      |      |      |      |      |-- adv_proxy: proxy node routine, which can be used to provision network through the PV_GATT layer (BLE connection).
  |      |      |      |      |      |-- adv_vendor: vendor-defined model routine, used with self_provisioner_vendor, supports two communication attributes of response transmission and non-response transparent transmission, and develops the communication protocol by yourself.
  |      |      |      |      |      |-- adv_vendor_friend: based on vendor-defined model routine, support friend node function
  |      |      |      |      |      |-- adv_vendor_low_power: On the basis of the vendor custom model routine, support low-power node functions and should be used with friend nodes
  |      |      |      |      |      |-- adv_vendor_self_provision: On the basis of the vendor custom model routine, support local provision
  |      |      |      |      |      |-- adv_vendor_self_provision_IAP£ºMESH backup wireless upgrade IAP routine, detect the current code flag, judge whether to move the backup area code to the user area and run the user area code
  |      |      |      |      |      |-- adv_vendor_self_provision_JumpIAP£ºMESH backup wireless upgrade jump IAP routines, placed at the start address of the code,  for jumping to the IAP program
  |      |      |      |      |      |-- adv_vendor_self_provision_with_peripheral: MESH backup wireless upgrade user routine, On the basis of the vendor custom model routine, supports the connection control of the mobile phone BLE debugging assistant, receives the distribution network information through BLE and distributes the network itself. It is suitable for terminal control networking applications. It can formulate the communication protocol by itself to realize the mobile phone control all devices in the mesh network.
  |      |      |      |      |      |-- self_provisioner_vendor: vendor-defined model self-provisioning network initiator routine, used with adv_vendor, automatically provision network to the surrounding devices without network, and add it to its own mesh network, support provision network to six devices by default . The default configuration device is bound with single APPKEY, which is used for response transmission and non-response transparent transmission, and the configuration device is bound with single subscription address, which is used for mass sending of unanswered messages
  |      |      |      |      |      |-- self_provisioner_vendor_with_peripheral: Based on the vendor-defined model self-provisioning network initiator routine, can be connected and controlled by BLE debugging tool on the mobile phone, transfer the communication between the mobile phone and the mesh network, and can draw up the communication protocol to implement that all devices in mesh network can be controlled the mobile phone.
  |      |      |      |      |      |-- MESH_LIB: MESH protocol stack library file and header file
  |      |      |      |      |      |-- Qinheng MESH APP Management Distribution Network Application Manual.pdf
  |      |      |      |      |      |-- Qinheng Low Energy Bluetooth MESH Software Development Reference Manual.pdf
  |      |      |      |      |-- BackupUpgrade_IAP: backup wireless upgrade IAP routine, detect the current code flag, judge whether to move the backup area code to the user area and run the user area code
  |      |      |      |      |-- BackupUpgrade_JumpIAP: backup wireless upgrade jump IAP routines, placed at the start address of the code,  for jumping to the IAP program
  |      |      |      |      |-- BackupUpgrade_OTA: backup wireless upgrade user routine, add OTA function based on peripheral slave routines, store the upgraded firmware to the backup area and jump to the IAP program to upgrade
  |      |      |      |      |-- OnlyUpdateApp_IAP: the only library wireless upgrade IAP routine, with OTA function, upgrade the user area code after receiving the upgraded firmware
  |      |      |      |      |-- OnlyUpdateApp_JumpIAP: the only library wireless upgrade and jump IAP routine, placed at the start address of the codes, for jumping to the IAP program
  |      |      |      |      |-- OnlyUpdateApp_Peripheral: the only library wireless upgrade user routine, on the basis of peripheral routine, the jumping to IAP program is added for subsequent upgrades
  |      |      |      |      |-- BLE_UART: BLE and UART transparent transmission routine, for detailed instructions, please refer to the <description.txt> document in the root directory
  |      |      |      |      |-- BLE_USB: Bluetooth with USB routine, USB emulation 340 device forwards bluetooth data 
  |      |      |      |      |-- SYNC_ADV: cycle synchronization advertising routine
  |      |      |      |      |-- SYNC_SCAN: cycle synchronization scanning routine
  |      |      |      |      |-- HAL: Hardware-related files shared by routines
  |      |      |      |      |-- LIB: BLE protocol stack library file and header file
  |      |      |      |      |-- Qinheng Low Energy Bluetooth Software Development Reference Manual.pdf
  |      |      |      |      |-- WCH Bluetooth Over-the-Air Upgrade (BLE OTA).PDF
  |      |      |      |      |-- BLE Certification: Product: WCH CH58x QDID: 179771
  |      |      |-- PUB: CH583EVT evaluation board schematic and manual
  |
