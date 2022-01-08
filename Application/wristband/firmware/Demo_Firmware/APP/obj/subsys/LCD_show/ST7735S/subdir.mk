################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
C:/Users/10545/Documents/mywork/Wristband_project/26.\ wristband_wch/firmware/Demo_Firmware/subsys/LCD_show/ST7735S/ST7735_test.c \
C:/Users/10545/Documents/mywork/Wristband_project/26.\ wristband_wch/firmware/Demo_Firmware/subsys/LCD_show/ST7735S/lcd.c \
C:/Users/10545/Documents/mywork/Wristband_project/26.\ wristband_wch/firmware/Demo_Firmware/subsys/LCD_show/ST7735S/lcd_init.c 

C_DEPS += \
./subsys/LCD_show/ST7735S/ST7735_test.d \
./subsys/LCD_show/ST7735S/lcd.d \
./subsys/LCD_show/ST7735S/lcd_init.d 

OBJS += \
./subsys/LCD_show/ST7735S/ST7735_test.o \
./subsys/LCD_show/ST7735S/lcd.o \
./subsys/LCD_show/ST7735S/lcd_init.o 


# Each subdirectory must supply rules for building sources it contributes
subsys/LCD_show/ST7735S/ST7735_test.o: C:/Users/10545/Documents/mywork/Wristband_project/26.\ wristband_wch/firmware/Demo_Firmware/subsys/LCD_show/ST7735S/ST7735_test.c
	@	@	riscv-none-embed-gcc -march=rv32imac -mabi=ilp32 -mcmodel=medany -msmall-data-limit=8 -mno-save-restore -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common  -g -DCONFIG_RISCV -DLOG -DDEBUG=1 -DCONFIG_ZTEST -I"C:\Users\10545\Documents\mywork\Wristband_project\26. wristband_wch\firmware\Demo_Firmware\StdPeriphDriver\inc" -I"C:\Users\10545\Documents\mywork\Wristband_project\26. wristband_wch\firmware\Demo_Firmware\subsys\HAL" -I"C:\Users\10545\Documents\mywork\Wristband_project\26. wristband_wch\firmware\Demo_Firmware\subsys\BLE" -I"C:\Users\10545\Documents\mywork\Wristband_project\26. wristband_wch\firmware\Demo_Firmware\subsys\BLE\profile" -isystem"../../boards" -isystem"../../LIB" -isystem"../../drivers" -isystem"../../include" -isystem"../../kernel" -isystem"../../subsys" -isystem"../../soc" -isystem"../../StdPeriphDriver" -isystem"../src" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
	@	@
subsys/LCD_show/ST7735S/lcd.o: C:/Users/10545/Documents/mywork/Wristband_project/26.\ wristband_wch/firmware/Demo_Firmware/subsys/LCD_show/ST7735S/lcd.c
	@	@	riscv-none-embed-gcc -march=rv32imac -mabi=ilp32 -mcmodel=medany -msmall-data-limit=8 -mno-save-restore -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common  -g -DCONFIG_RISCV -DLOG -DDEBUG=1 -DCONFIG_ZTEST -I"C:\Users\10545\Documents\mywork\Wristband_project\26. wristband_wch\firmware\Demo_Firmware\StdPeriphDriver\inc" -I"C:\Users\10545\Documents\mywork\Wristband_project\26. wristband_wch\firmware\Demo_Firmware\subsys\HAL" -I"C:\Users\10545\Documents\mywork\Wristband_project\26. wristband_wch\firmware\Demo_Firmware\subsys\BLE" -I"C:\Users\10545\Documents\mywork\Wristband_project\26. wristband_wch\firmware\Demo_Firmware\subsys\BLE\profile" -isystem"../../boards" -isystem"../../LIB" -isystem"../../drivers" -isystem"../../include" -isystem"../../kernel" -isystem"../../subsys" -isystem"../../soc" -isystem"../../StdPeriphDriver" -isystem"../src" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
	@	@
subsys/LCD_show/ST7735S/lcd_init.o: C:/Users/10545/Documents/mywork/Wristband_project/26.\ wristband_wch/firmware/Demo_Firmware/subsys/LCD_show/ST7735S/lcd_init.c
	@	@	riscv-none-embed-gcc -march=rv32imac -mabi=ilp32 -mcmodel=medany -msmall-data-limit=8 -mno-save-restore -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common  -g -DCONFIG_RISCV -DLOG -DDEBUG=1 -DCONFIG_ZTEST -I"C:\Users\10545\Documents\mywork\Wristband_project\26. wristband_wch\firmware\Demo_Firmware\StdPeriphDriver\inc" -I"C:\Users\10545\Documents\mywork\Wristband_project\26. wristband_wch\firmware\Demo_Firmware\subsys\HAL" -I"C:\Users\10545\Documents\mywork\Wristband_project\26. wristband_wch\firmware\Demo_Firmware\subsys\BLE" -I"C:\Users\10545\Documents\mywork\Wristband_project\26. wristband_wch\firmware\Demo_Firmware\subsys\BLE\profile" -isystem"../../boards" -isystem"../../LIB" -isystem"../../drivers" -isystem"../../include" -isystem"../../kernel" -isystem"../../subsys" -isystem"../../soc" -isystem"../../StdPeriphDriver" -isystem"../src" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
	@	@

