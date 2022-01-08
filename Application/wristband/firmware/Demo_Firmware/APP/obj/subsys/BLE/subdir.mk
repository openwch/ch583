################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
C:/Users/10545/Documents/mywork/Wristband_project/26.\ wristband_wch/firmware/Demo_Firmware/subsys/BLE/CTS.c \
C:/Users/10545/Documents/mywork/Wristband_project/26.\ wristband_wch/firmware/Demo_Firmware/subsys/BLE/battery.c \
C:/Users/10545/Documents/mywork/Wristband_project/26.\ wristband_wch/firmware/Demo_Firmware/subsys/BLE/dfu.c \
C:/Users/10545/Documents/mywork/Wristband_project/26.\ wristband_wch/firmware/Demo_Firmware/subsys/BLE/heartrate.c \
C:/Users/10545/Documents/mywork/Wristband_project/26.\ wristband_wch/firmware/Demo_Firmware/subsys/BLE/peripheral.c 

C_DEPS += \
./subsys/BLE/CTS.d \
./subsys/BLE/battery.d \
./subsys/BLE/dfu.d \
./subsys/BLE/heartrate.d \
./subsys/BLE/peripheral.d 

OBJS += \
./subsys/BLE/CTS.o \
./subsys/BLE/battery.o \
./subsys/BLE/dfu.o \
./subsys/BLE/heartrate.o \
./subsys/BLE/peripheral.o 


# Each subdirectory must supply rules for building sources it contributes
subsys/BLE/CTS.o: C:/Users/10545/Documents/mywork/Wristband_project/26.\ wristband_wch/firmware/Demo_Firmware/subsys/BLE/CTS.c
	@	@	riscv-none-embed-gcc -march=rv32imac -mabi=ilp32 -mcmodel=medany -msmall-data-limit=8 -mno-save-restore -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common  -g -DCONFIG_RISCV -DLOG -DDEBUG=1 -DCONFIG_ZTEST -I"C:\Users\10545\Documents\mywork\Wristband_project\26. wristband_wch\firmware\Demo_Firmware\StdPeriphDriver\inc" -I"C:\Users\10545\Documents\mywork\Wristband_project\26. wristband_wch\firmware\Demo_Firmware\subsys\HAL" -I"C:\Users\10545\Documents\mywork\Wristband_project\26. wristband_wch\firmware\Demo_Firmware\subsys\BLE" -I"C:\Users\10545\Documents\mywork\Wristband_project\26. wristband_wch\firmware\Demo_Firmware\subsys\BLE\profile" -isystem"../../boards" -isystem"../../LIB" -isystem"../../drivers" -isystem"../../include" -isystem"../../kernel" -isystem"../../subsys" -isystem"../../soc" -isystem"../../StdPeriphDriver" -isystem"../src" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
	@	@
subsys/BLE/battery.o: C:/Users/10545/Documents/mywork/Wristband_project/26.\ wristband_wch/firmware/Demo_Firmware/subsys/BLE/battery.c
	@	@	riscv-none-embed-gcc -march=rv32imac -mabi=ilp32 -mcmodel=medany -msmall-data-limit=8 -mno-save-restore -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common  -g -DCONFIG_RISCV -DLOG -DDEBUG=1 -DCONFIG_ZTEST -I"C:\Users\10545\Documents\mywork\Wristband_project\26. wristband_wch\firmware\Demo_Firmware\StdPeriphDriver\inc" -I"C:\Users\10545\Documents\mywork\Wristband_project\26. wristband_wch\firmware\Demo_Firmware\subsys\HAL" -I"C:\Users\10545\Documents\mywork\Wristband_project\26. wristband_wch\firmware\Demo_Firmware\subsys\BLE" -I"C:\Users\10545\Documents\mywork\Wristband_project\26. wristband_wch\firmware\Demo_Firmware\subsys\BLE\profile" -isystem"../../boards" -isystem"../../LIB" -isystem"../../drivers" -isystem"../../include" -isystem"../../kernel" -isystem"../../subsys" -isystem"../../soc" -isystem"../../StdPeriphDriver" -isystem"../src" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
	@	@
subsys/BLE/dfu.o: C:/Users/10545/Documents/mywork/Wristband_project/26.\ wristband_wch/firmware/Demo_Firmware/subsys/BLE/dfu.c
	@	@	riscv-none-embed-gcc -march=rv32imac -mabi=ilp32 -mcmodel=medany -msmall-data-limit=8 -mno-save-restore -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common  -g -DCONFIG_RISCV -DLOG -DDEBUG=1 -DCONFIG_ZTEST -I"C:\Users\10545\Documents\mywork\Wristband_project\26. wristband_wch\firmware\Demo_Firmware\StdPeriphDriver\inc" -I"C:\Users\10545\Documents\mywork\Wristband_project\26. wristband_wch\firmware\Demo_Firmware\subsys\HAL" -I"C:\Users\10545\Documents\mywork\Wristband_project\26. wristband_wch\firmware\Demo_Firmware\subsys\BLE" -I"C:\Users\10545\Documents\mywork\Wristband_project\26. wristband_wch\firmware\Demo_Firmware\subsys\BLE\profile" -isystem"../../boards" -isystem"../../LIB" -isystem"../../drivers" -isystem"../../include" -isystem"../../kernel" -isystem"../../subsys" -isystem"../../soc" -isystem"../../StdPeriphDriver" -isystem"../src" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
	@	@
subsys/BLE/heartrate.o: C:/Users/10545/Documents/mywork/Wristband_project/26.\ wristband_wch/firmware/Demo_Firmware/subsys/BLE/heartrate.c
	@	@	riscv-none-embed-gcc -march=rv32imac -mabi=ilp32 -mcmodel=medany -msmall-data-limit=8 -mno-save-restore -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common  -g -DCONFIG_RISCV -DLOG -DDEBUG=1 -DCONFIG_ZTEST -I"C:\Users\10545\Documents\mywork\Wristband_project\26. wristband_wch\firmware\Demo_Firmware\StdPeriphDriver\inc" -I"C:\Users\10545\Documents\mywork\Wristband_project\26. wristband_wch\firmware\Demo_Firmware\subsys\HAL" -I"C:\Users\10545\Documents\mywork\Wristband_project\26. wristband_wch\firmware\Demo_Firmware\subsys\BLE" -I"C:\Users\10545\Documents\mywork\Wristband_project\26. wristband_wch\firmware\Demo_Firmware\subsys\BLE\profile" -isystem"../../boards" -isystem"../../LIB" -isystem"../../drivers" -isystem"../../include" -isystem"../../kernel" -isystem"../../subsys" -isystem"../../soc" -isystem"../../StdPeriphDriver" -isystem"../src" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
	@	@
subsys/BLE/peripheral.o: C:/Users/10545/Documents/mywork/Wristband_project/26.\ wristband_wch/firmware/Demo_Firmware/subsys/BLE/peripheral.c
	@	@	riscv-none-embed-gcc -march=rv32imac -mabi=ilp32 -mcmodel=medany -msmall-data-limit=8 -mno-save-restore -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common  -g -DCONFIG_RISCV -DLOG -DDEBUG=1 -DCONFIG_ZTEST -I"C:\Users\10545\Documents\mywork\Wristband_project\26. wristband_wch\firmware\Demo_Firmware\StdPeriphDriver\inc" -I"C:\Users\10545\Documents\mywork\Wristband_project\26. wristband_wch\firmware\Demo_Firmware\subsys\HAL" -I"C:\Users\10545\Documents\mywork\Wristband_project\26. wristband_wch\firmware\Demo_Firmware\subsys\BLE" -I"C:\Users\10545\Documents\mywork\Wristband_project\26. wristband_wch\firmware\Demo_Firmware\subsys\BLE\profile" -isystem"../../boards" -isystem"../../LIB" -isystem"../../drivers" -isystem"../../include" -isystem"../../kernel" -isystem"../../subsys" -isystem"../../soc" -isystem"../../StdPeriphDriver" -isystem"../src" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
	@	@

