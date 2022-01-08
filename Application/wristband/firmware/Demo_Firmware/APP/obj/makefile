################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include subsys/sensor/max30102/subdir.mk
-include subsys/sensor/MPU9250/subdir.mk
-include subsys/sensor/subdir.mk
-include subsys/LCD_show/ST7735S/subdir.mk
-include subsys/LCD_show/subdir.mk
-include subsys/HAL/subdir.mk
-include subsys/DRV/DRV2605/subdir.mk
-include subsys/DRV/subdir.mk
-include subsys/BLE/profile/subdir.mk
-include subsys/BLE/subdir.mk
-include src/subdir.mk
-include soc/subdir.mk
-include drivers/dfu/subdir.mk
-include drivers/debug/subdir.mk
-include drivers/SPI/subdir.mk
-include drivers/RingBuffer/subdir.mk
-include drivers/I2C/subdir.mk
-include drivers/HAL_FLASH/subdir.mk
-include arch/subdir.mk
-include StdPeriphDriver/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(CC_DEPS)),)
-include $(CC_DEPS)
endif
ifneq ($(strip $(ASM_UPPER_DEPS)),)
-include $(ASM_UPPER_DEPS)
endif
ifneq ($(strip $(C++_DEPS)),)
-include $(C++_DEPS)
endif
ifneq ($(strip $(C_UPPER_DEPS)),)
-include $(C_UPPER_DEPS)
endif
ifneq ($(strip $(CXX_DEPS)),)
-include $(CXX_DEPS)
endif
ifneq ($(strip $(ASM_DEPS)),)
-include $(ASM_DEPS)
endif
ifneq ($(strip $(S_DEPS)),)
-include $(S_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
ifneq ($(strip $(CPP_DEPS)),)
-include $(CPP_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 
SECONDARY_FLASH += \
wristband.bin \

SECONDARY_LIST += \
wristband.lst \

SECONDARY_SIZE += \
wristband.siz \


# All Target
all: wristband.elf secondary-outputs

# Tool invocations
wristband.elf: $(OBJS) $(USER_OBJS)
	@	@	riscv-none-embed-g++ -march=rv32imac -mabi=ilp32 -mcmodel=medany -msmall-data-limit=8 -mno-save-restore -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common  -g -T "../../soc/Link.ld" -nostartfiles -Xlinker --gc-sections -L"../../LIB" -Xlinker --print-memory-usage -Wl,-Map,"wristband.map" --specs=nano.specs -u _printf_float --specs=nosys.specs -o "wristband.elf" $(OBJS) $(USER_OBJS) $(LIBS)
	@	@
wristband.bin: wristband.elf
	@	riscv-none-embed-objcopy -O binary "wristband.elf"  "wristband.bin"
	@	@
wristband.lst: wristband.elf
	@	riscv-none-embed-objdump --source --all-headers --demangle --line-numbers --wide "wristband.elf" > "wristband.lst"
	@	@
wristband.siz: wristband.elf
	@	riscv-none-embed-size --format=berkeley "wristband.elf"
	@	@
# Other Targets
clean:
	-$(RM) $(C_UPPER_DEPS)$(SECONDARY_LIST)$(SECONDARY_SIZE)$(S_DEPS)$(C_DEPS)$(CC_DEPS)$(ASM_UPPER_DEPS)$(C++_DEPS)$(OBJS)$(CXX_DEPS)$(SECONDARY_FLASH)$(ASM_DEPS)$(S_UPPER_DEPS)$(CPP_DEPS) wristband.elf
	-@
secondary-outputs: $(SECONDARY_FLASH) $(SECONDARY_LIST) $(SECONDARY_SIZE)

.PHONY: all clean dependents

-include ../makefile.targets
