################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../ra_gen/common_data.c \
../ra_gen/hal_data.c \
../ra_gen/main.c \
../ra_gen/pin_data.c \
../ra_gen/vector_data.c 

C_DEPS += \
./ra_gen/common_data.d \
./ra_gen/hal_data.d \
./ra_gen/main.d \
./ra_gen/pin_data.d \
./ra_gen/vector_data.d 

OBJS += \
./ra_gen/common_data.o \
./ra_gen/hal_data.o \
./ra_gen/main.o \
./ra_gen/pin_data.o \
./ra_gen/vector_data.o 

SREC += \
mcuboot_ra2l1.srec 

MAP += \
mcuboot_ra2l1.map 


# Each subdirectory must supply rules for building sources it contributes
ra_gen/%.o: ../ra_gen/%.c
	$(file > $@.in,-mcpu=cortex-m23 -mthumb -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -Wunused -Wuninitialized -Wall -Wextra -Wmissing-declarations -Wconversion -Wpointer-arith -Wshadow -Wlogical-op -Waggregate-return -Wfloat-equal  -g -gdwarf-4 -D_RENESAS_RA_ -D_RA_CORE=CM23 -I"C:/workspace/e2_studio/workspace/MultiReader/mcuboot_ra2l1/src" -I"C:/workspace/e2_studio/workspace/MultiReader/mcuboot_ra2l1/ra/fsp/inc" -I"C:/workspace/e2_studio/workspace/MultiReader/mcuboot_ra2l1/ra/fsp/inc/api" -I"C:/workspace/e2_studio/workspace/MultiReader/mcuboot_ra2l1/ra/fsp/inc/instances" -I"C:/workspace/e2_studio/workspace/MultiReader/mcuboot_ra2l1/ra/arm/CMSIS_5/CMSIS/Core/Include" -I"C:/workspace/e2_studio/workspace/MultiReader/mcuboot_ra2l1/ra_gen" -I"C:/workspace/e2_studio/workspace/MultiReader/mcuboot_ra2l1/ra_cfg/fsp_cfg/bsp" -I"C:/workspace/e2_studio/workspace/MultiReader/mcuboot_ra2l1/ra_cfg/fsp_cfg" -I"C:/workspace/e2_studio/workspace/MultiReader/mcuboot_ra2l1/ra/mcu-tools/MCUboot/boot/bootutil/src" -I"C:/workspace/e2_studio/workspace/MultiReader/mcuboot_ra2l1/ra_cfg/mcu-tools/include" -I"C:/workspace/e2_studio/workspace/MultiReader/mcuboot_ra2l1/ra/mcu-tools/MCUboot/boot/bootutil/include" -I"C:/workspace/e2_studio/workspace/MultiReader/mcuboot_ra2l1/ra/fsp/src/rm_mcuboot_port" -I"C:/workspace/e2_studio/workspace/MultiReader/mcuboot_ra2l1/ra/mcu-tools/MCUboot/ext/tinycrypt/lib/include" -I"C:/workspace/e2_studio/workspace/MultiReader/mcuboot_ra2l1/ra/mcu-tools/MCUboot/ext/mbedtls-asn1/include" -I"C:/workspace/e2_studio/workspace/MultiReader/mcuboot_ra2l1/ra_cfg/mcu-tools/include/mcuboot_config" -I"C:/workspace/e2_studio/workspace/MultiReader/mcuboot_ra2l1/ra_cfg/mcu-tools/include/sysflash" -std=c99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -c -o "$@" -x c "$<")
	@echo Building file: $< && arm-none-eabi-gcc @"$@.in"

