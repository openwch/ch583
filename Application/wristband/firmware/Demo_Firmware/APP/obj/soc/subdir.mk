################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
C:/Users/10545/Documents/mywork/Wristband_project/26.\ wristband_wch/firmware/Demo_Firmware/soc/soc.c 

C_DEPS += \
./soc/soc.d 

OBJS += \
./soc/soc.o 


# Each subdirectory must supply rules for building sources it contributes
soc/soc.o: C:/Users/10545/Documents/mywork/Wristband_project/26.\ wristband_wch/firmware/Demo_Firmware/soc/soc.c
	@	@	riscv-none-embed-gcc -march=rv32imac -mabi=ilp32 -mcmodel=medany -msmall-data-limit=8 -mno-save-restore -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common  -g -DCONFIG_RISCV -DLOG -DDEBUG=1 -DCONFIG_ZTEST -I"C:\Users\10545\Documents\mywork\Wristband_project\26. wristband_wch\firmware\Demo_Firmware\StdPeriphDriver\inc" -I"C:\Users\10545\Documents\mywork\Wristband_project\26. wristband_wch\firmware\Demo_Firmware\subsys\HAL" -I"C:\Users\10545\Documents\mywork\Wristband_project\26. wristband_wch\firmware\Demo_Firmware\subsys\BLE" -I"C:\Users\10545\Documents\mywork\Wristband_project\26. wristband_wch\firmware\Demo_Firmware\subsys\BLE\profile" -isystem"../../boards" -isystem"../../LIB" -isystem"../../drivers" -isystem"../../include" -isystem"../../kernel" -isystem"../../subsys" -isystem"../../soc" -isystem"../../StdPeriphDriver" -isystem"../src" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
	@	@

