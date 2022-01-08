################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
C:/Users/10545/Documents/mywork/Wristband_project/26.\ wristband_wch/firmware/Demo_Firmware/drivers/HAL_FLASH/easyflash.c \
C:/Users/10545/Documents/mywork/Wristband_project/26.\ wristband_wch/firmware/Demo_Firmware/drivers/HAL_FLASH/ef_env.c \
C:/Users/10545/Documents/mywork/Wristband_project/26.\ wristband_wch/firmware/Demo_Firmware/drivers/HAL_FLASH/ef_log.c \
C:/Users/10545/Documents/mywork/Wristband_project/26.\ wristband_wch/firmware/Demo_Firmware/drivers/HAL_FLASH/ef_port.c \
C:/Users/10545/Documents/mywork/Wristband_project/26.\ wristband_wch/firmware/Demo_Firmware/drivers/HAL_FLASH/ef_utils.c 

C_DEPS += \
./drivers/HAL_FLASH/easyflash.d \
./drivers/HAL_FLASH/ef_env.d \
./drivers/HAL_FLASH/ef_log.d \
./drivers/HAL_FLASH/ef_port.d \
./drivers/HAL_FLASH/ef_utils.d 

OBJS += \
./drivers/HAL_FLASH/easyflash.o \
./drivers/HAL_FLASH/ef_env.o \
./drivers/HAL_FLASH/ef_log.o \
./drivers/HAL_FLASH/ef_port.o \
./drivers/HAL_FLASH/ef_utils.o 


# Each subdirectory must supply rules for building sources it contributes
drivers/HAL_FLASH/easyflash.o: C:/Users/10545/Documents/mywork/Wristband_project/26.\ wristband_wch/firmware/Demo_Firmware/drivers/HAL_FLASH/easyflash.c
	@	@	riscv-none-embed-gcc -march=rv32imac -mabi=ilp32 -mcmodel=medany -msmall-data-limit=8 -mno-save-restore -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common  -g -DCONFIG_RISCV -DLOG -DDEBUG=1 -DCONFIG_ZTEST -I"C:\Users\10545\Documents\mywork\Wristband_project\26. wristband_wch\firmware\Demo_Firmware\StdPeriphDriver\inc" -I"C:\Users\10545\Documents\mywork\Wristband_project\26. wristband_wch\firmware\Demo_Firmware\subsys\HAL" -I"C:\Users\10545\Documents\mywork\Wristband_project\26. wristband_wch\firmware\Demo_Firmware\subsys\BLE" -I"C:\Users\10545\Documents\mywork\Wristband_project\26. wristband_wch\firmware\Demo_Firmware\subsys\BLE\profile" -isystem"../../boards" -isystem"../../LIB" -isystem"../../drivers" -isystem"../../include" -isystem"../../kernel" -isystem"../../subsys" -isystem"../../soc" -isystem"../../StdPeriphDriver" -isystem"../src" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
	@	@
drivers/HAL_FLASH/ef_env.o: C:/Users/10545/Documents/mywork/Wristband_project/26.\ wristband_wch/firmware/Demo_Firmware/drivers/HAL_FLASH/ef_env.c
	@	@	riscv-none-embed-gcc -march=rv32imac -mabi=ilp32 -mcmodel=medany -msmall-data-limit=8 -mno-save-restore -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common  -g -DCONFIG_RISCV -DLOG -DDEBUG=1 -DCONFIG_ZTEST -I"C:\Users\10545\Documents\mywork\Wristband_project\26. wristband_wch\firmware\Demo_Firmware\StdPeriphDriver\inc" -I"C:\Users\10545\Documents\mywork\Wristband_project\26. wristband_wch\firmware\Demo_Firmware\subsys\HAL" -I"C:\Users\10545\Documents\mywork\Wristband_project\26. wristband_wch\firmware\Demo_Firmware\subsys\BLE" -I"C:\Users\10545\Documents\mywork\Wristband_project\26. wristband_wch\firmware\Demo_Firmware\subsys\BLE\profile" -isystem"../../boards" -isystem"../../LIB" -isystem"../../drivers" -isystem"../../include" -isystem"../../kernel" -isystem"../../subsys" -isystem"../../soc" -isystem"../../StdPeriphDriver" -isystem"../src" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
	@	@
drivers/HAL_FLASH/ef_log.o: C:/Users/10545/Documents/mywork/Wristband_project/26.\ wristband_wch/firmware/Demo_Firmware/drivers/HAL_FLASH/ef_log.c
	@	@	riscv-none-embed-gcc -march=rv32imac -mabi=ilp32 -mcmodel=medany -msmall-data-limit=8 -mno-save-restore -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common  -g -DCONFIG_RISCV -DLOG -DDEBUG=1 -DCONFIG_ZTEST -I"C:\Users\10545\Documents\mywork\Wristband_project\26. wristband_wch\firmware\Demo_Firmware\StdPeriphDriver\inc" -I"C:\Users\10545\Documents\mywork\Wristband_project\26. wristband_wch\firmware\Demo_Firmware\subsys\HAL" -I"C:\Users\10545\Documents\mywork\Wristband_project\26. wristband_wch\firmware\Demo_Firmware\subsys\BLE" -I"C:\Users\10545\Documents\mywork\Wristband_project\26. wristband_wch\firmware\Demo_Firmware\subsys\BLE\profile" -isystem"../../boards" -isystem"../../LIB" -isystem"../../drivers" -isystem"../../include" -isystem"../../kernel" -isystem"../../subsys" -isystem"../../soc" -isystem"../../StdPeriphDriver" -isystem"../src" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
	@	@
drivers/HAL_FLASH/ef_port.o: C:/Users/10545/Documents/mywork/Wristband_project/26.\ wristband_wch/firmware/Demo_Firmware/drivers/HAL_FLASH/ef_port.c
	@	@	riscv-none-embed-gcc -march=rv32imac -mabi=ilp32 -mcmodel=medany -msmall-data-limit=8 -mno-save-restore -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common  -g -DCONFIG_RISCV -DLOG -DDEBUG=1 -DCONFIG_ZTEST -I"C:\Users\10545\Documents\mywork\Wristband_project\26. wristband_wch\firmware\Demo_Firmware\StdPeriphDriver\inc" -I"C:\Users\10545\Documents\mywork\Wristband_project\26. wristband_wch\firmware\Demo_Firmware\subsys\HAL" -I"C:\Users\10545\Documents\mywork\Wristband_project\26. wristband_wch\firmware\Demo_Firmware\subsys\BLE" -I"C:\Users\10545\Documents\mywork\Wristband_project\26. wristband_wch\firmware\Demo_Firmware\subsys\BLE\profile" -isystem"../../boards" -isystem"../../LIB" -isystem"../../drivers" -isystem"../../include" -isystem"../../kernel" -isystem"../../subsys" -isystem"../../soc" -isystem"../../StdPeriphDriver" -isystem"../src" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
	@	@
drivers/HAL_FLASH/ef_utils.o: C:/Users/10545/Documents/mywork/Wristband_project/26.\ wristband_wch/firmware/Demo_Firmware/drivers/HAL_FLASH/ef_utils.c
	@	@	riscv-none-embed-gcc -march=rv32imac -mabi=ilp32 -mcmodel=medany -msmall-data-limit=8 -mno-save-restore -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common  -g -DCONFIG_RISCV -DLOG -DDEBUG=1 -DCONFIG_ZTEST -I"C:\Users\10545\Documents\mywork\Wristband_project\26. wristband_wch\firmware\Demo_Firmware\StdPeriphDriver\inc" -I"C:\Users\10545\Documents\mywork\Wristband_project\26. wristband_wch\firmware\Demo_Firmware\subsys\HAL" -I"C:\Users\10545\Documents\mywork\Wristband_project\26. wristband_wch\firmware\Demo_Firmware\subsys\BLE" -I"C:\Users\10545\Documents\mywork\Wristband_project\26. wristband_wch\firmware\Demo_Firmware\subsys\BLE\profile" -isystem"../../boards" -isystem"../../LIB" -isystem"../../drivers" -isystem"../../include" -isystem"../../kernel" -isystem"../../subsys" -isystem"../../soc" -isystem"../../StdPeriphDriver" -isystem"../src" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
	@	@

