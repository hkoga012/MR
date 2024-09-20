################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

-include makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include src/subdir.mk
-include ra_gen/subdir.mk
-include ra/mcu-tools/MCUboot/sim/mcuboot-sys/csupport/subdir.mk
-include ra/mcu-tools/MCUboot/ext/tinycrypt/lib/source/subdir.mk
-include ra/mcu-tools/MCUboot/ext/mbedtls-asn1/src/subdir.mk
-include ra/mcu-tools/MCUboot/boot/bootutil/src/subdir.mk
-include ra/fsp/src/rm_mcuboot_port/custom_crypto_stacks/protected_mode/subdir.mk
-include ra/fsp/src/rm_mcuboot_port/subdir.mk
-include ra/fsp/src/r_flash_lp/subdir.mk
-include ra/fsp/src/bsp/mcu/ra2l1/subdir.mk
-include ra/fsp/src/bsp/mcu/all/subdir.mk
-include ra/fsp/src/bsp/cmsis/Device/RENESAS/Source/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(JMP_DEPS)),)
-include $(JMP_DEPS)
endif
ifneq ($(strip $(FSY_DEPS)),)
-include $(FSY_DEPS)
endif
ifneq ($(strip $(C_UPPER_DEPS)),)
-include $(C_UPPER_DEPS)
endif
ifneq ($(strip $(SRC_UPPER_DEPS)),)
-include $(SRC_UPPER_DEPS)
endif
ifneq ($(strip $(P_UPPER_DEPS)),)
-include $(P_UPPER_DEPS)
endif
ifneq ($(strip $(S_DEPS)),)
-include $(S_DEPS)
endif
ifneq ($(strip $(PP_UPPER_DEPS)),)
-include $(PP_UPPER_DEPS)
endif
ifneq ($(strip $(P_DEPS)),)
-include $(P_DEPS)
endif
ifneq ($(strip $(FSY_UPPER_DEPS)),)
-include $(FSY_UPPER_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
ifneq ($(strip $(SRC_DEPS)),)
-include $(SRC_DEPS)
endif
ifneq ($(strip $(JMP_UPPER_DEPS)),)
-include $(JMP_UPPER_DEPS)
endif
ifneq ($(strip $(PP_DEPS)),)
-include $(PP_DEPS)
endif
ifneq ($(strip $(ASM_UPPER_DEPS)),)
-include $(ASM_UPPER_DEPS)
endif
ifneq ($(strip $(SX_DEPS)),)
-include $(SX_DEPS)
endif
ifneq ($(strip $(SX_UPPER_DEPS)),)
-include $(SX_UPPER_DEPS)
endif
ifneq ($(strip $(ASM_DEPS)),)
-include $(ASM_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 
SECONDARY_FLASH += \
mcuboot_ra2l1.srec \

SECONDARY_SIZE += \
mcuboot_ra2l1.siz \


# All Target
all:
	$(MAKE) --no-print-directory pre-build
	$(MAKE) --no-print-directory main-build

# Main-build Target
main-build: mcuboot_ra2l1.elf secondary-outputs

# Tool invocations
mcuboot_ra2l1.elf: $(OBJS) $(USER_OBJS) $(LINKER_SCRIPT)
	$(file > mcuboot_ra2l1.elf.in,-mcpu=cortex-m23 -mthumb -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -Wunused -Wuninitialized -Wall -Wextra -Wmissing-declarations -Wconversion -Wpointer-arith -Wshadow -Wlogical-op -Waggregate-return -Wfloat-equal  -g -gdwarf-4 -T "fsp.ld" -Xlinker --gc-sections -L "C:\\workspace\\e2_studio\\workspace\\MultiReader\\mcuboot_ra2l1/script" -Wl,-Map,"mcuboot_ra2l1.map" --specs=nano.specs --specs=rdimon.specs -o "mcuboot_ra2l1.elf" -Wl,--start-group $(OBJS) $(USER_OBJS) $(LIBS) -Wl,--end-group)
	@echo Building target: $@ && arm-none-eabi-gcc @"mcuboot_ra2l1.elf.in"

mcuboot_ra2l1.srec: mcuboot_ra2l1.elf
	arm-none-eabi-objcopy -O srec "mcuboot_ra2l1.elf"  "mcuboot_ra2l1.srec"

mcuboot_ra2l1.siz: mcuboot_ra2l1.elf
	arm-none-eabi-size --format=berkeley "mcuboot_ra2l1.elf"

# Other Targets
clean:
	-$(file > clean_file.tmp,  $(JMP_DEPS)  $(FSY_DEPS)  $(C_UPPER_DEPS)  $(SECONDARY_SIZE)  $(SRC_UPPER_DEPS)  $(P_UPPER_DEPS)  $(S_DEPS)  $(PP_UPPER_DEPS)  $(P_DEPS)  $(FSY_UPPER_DEPS)  $(C_DEPS)  $(SRC_DEPS)  $(JMP_UPPER_DEPS)  $(PP_DEPS)  $(ASM_UPPER_DEPS)  $(SX_DEPS)  $(SX_UPPER_DEPS)  $(OBJS)  $(SECONDARY_FLASH)  $(ASM_DEPS)  $(SREC)  $(S_UPPER_DEPS)  $(MAP) )
	-@ xargs -P 2 -s 32000 -t -a clean_file.tmp $(RM)
	-@$(RM) clean_file.tmp
	-$(RM)  mcuboot_ra2l1.elf
	-@echo ' '

pre-build:
	-del mcuboot_ra2l1.elf
	-@echo ' '

secondary-outputs: $(SECONDARY_FLASH) $(SECONDARY_SIZE)

.PHONY: all clean dependents main-build pre-build

-include ../makefile.targets
