################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include Startup/subdir.mk
-include Src/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 
EXECUTABLES += \
progetto_saumura.elf \

SIZE_OUTPUT += \
default.size.stdout \

OBJDUMP_LIST += \
progetto_saumura.list \

OBJCOPY_BIN += \
progetto_saumura.bin \


# All Target
all: progetto_saumura.elf secondary-outputs

# Tool invocations
progetto_saumura.elf: $(OBJS) $(USER_OBJS) C:\Users\augus\Desktop\Progetto\ SMC-ACE\Codici\ -\ STM\ e\ ESP\progetto_saumura\ -\ codice\ nucleo\ 04\ 03\STM32F030R8TX_FLASH.ld
	arm-none-eabi-gcc -o "progetto_saumura.elf" @"objects.list" $(USER_OBJS) $(LIBS) -mcpu=cortex-m0 -T"C:\Users\augus\Desktop\Progetto SMC-ACE\Codici - STM e ESP\progetto_saumura - codice nucleo 04 03\STM32F030R8TX_FLASH.ld" --specs=nosys.specs -Wl,-Map="progetto_saumura.map" -Wl,--gc-sections -static --specs=nano.specs -mfloat-abi=soft -mthumb -Wl,--start-group -lc -lm -Wl,--end-group
	@echo 'Finished building target: $@'
	@echo ' '

default.size.stdout: $(EXECUTABLES)
	arm-none-eabi-size  $(EXECUTABLES)
	@echo 'Finished building: $@'
	@echo ' '

progetto_saumura.list: $(EXECUTABLES)
	arm-none-eabi-objdump -h -S $(EXECUTABLES) > "progetto_saumura.list"
	@echo 'Finished building: $@'
	@echo ' '

progetto_saumura.bin: $(EXECUTABLES)
	arm-none-eabi-objcopy  -O binary $(EXECUTABLES) "progetto_saumura.bin"
	@echo 'Finished building: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) *
	-@echo ' '

secondary-outputs: $(SIZE_OUTPUT) $(OBJDUMP_LIST) $(OBJCOPY_BIN)

.PHONY: all clean dependents
.SECONDARY:

-include ../makefile.targets
