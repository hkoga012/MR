################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../ra/mcu-tools/MCUboot/boot/bootutil/src/boot_record.c \
../ra/mcu-tools/MCUboot/boot/bootutil/src/bootutil_misc.c \
../ra/mcu-tools/MCUboot/boot/bootutil/src/bootutil_public.c \
../ra/mcu-tools/MCUboot/boot/bootutil/src/caps.c \
../ra/mcu-tools/MCUboot/boot/bootutil/src/encrypted.c \
../ra/mcu-tools/MCUboot/boot/bootutil/src/fault_injection_hardening.c \
../ra/mcu-tools/MCUboot/boot/bootutil/src/fault_injection_hardening_delay_rng_mbedtls.c \
../ra/mcu-tools/MCUboot/boot/bootutil/src/image_ec.c \
../ra/mcu-tools/MCUboot/boot/bootutil/src/image_ec256.c \
../ra/mcu-tools/MCUboot/boot/bootutil/src/image_ed25519.c \
../ra/mcu-tools/MCUboot/boot/bootutil/src/image_rsa.c \
../ra/mcu-tools/MCUboot/boot/bootutil/src/image_validate.c \
../ra/mcu-tools/MCUboot/boot/bootutil/src/loader.c \
../ra/mcu-tools/MCUboot/boot/bootutil/src/swap_misc.c \
../ra/mcu-tools/MCUboot/boot/bootutil/src/swap_move.c \
../ra/mcu-tools/MCUboot/boot/bootutil/src/swap_scratch.c \
../ra/mcu-tools/MCUboot/boot/bootutil/src/tlv.c 

C_DEPS += \
./ra/mcu-tools/MCUboot/boot/bootutil/src/boot_record.d \
./ra/mcu-tools/MCUboot/boot/bootutil/src/bootutil_misc.d \
./ra/mcu-tools/MCUboot/boot/bootutil/src/bootutil_public.d \
./ra/mcu-tools/MCUboot/boot/bootutil/src/caps.d \
./ra/mcu-tools/MCUboot/boot/bootutil/src/encrypted.d \
./ra/mcu-tools/MCUboot/boot/bootutil/src/fault_injection_hardening.d \
./ra/mcu-tools/MCUboot/boot/bootutil/src/fault_injection_hardening_delay_rng_mbedtls.d \
./ra/mcu-tools/MCUboot/boot/bootutil/src/image_ec.d \
./ra/mcu-tools/MCUboot/boot/bootutil/src/image_ec256.d \
./ra/mcu-tools/MCUboot/boot/bootutil/src/image_ed25519.d \
./ra/mcu-tools/MCUboot/boot/bootutil/src/image_rsa.d \
./ra/mcu-tools/MCUboot/boot/bootutil/src/image_validate.d \
./ra/mcu-tools/MCUboot/boot/bootutil/src/loader.d \
./ra/mcu-tools/MCUboot/boot/bootutil/src/swap_misc.d \
./ra/mcu-tools/MCUboot/boot/bootutil/src/swap_move.d \
./ra/mcu-tools/MCUboot/boot/bootutil/src/swap_scratch.d \
./ra/mcu-tools/MCUboot/boot/bootutil/src/tlv.d 

OBJS += \
./ra/mcu-tools/MCUboot/boot/bootutil/src/boot_record.o \
./ra/mcu-tools/MCUboot/boot/bootutil/src/bootutil_misc.o \
./ra/mcu-tools/MCUboot/boot/bootutil/src/bootutil_public.o \
./ra/mcu-tools/MCUboot/boot/bootutil/src/caps.o \
./ra/mcu-tools/MCUboot/boot/bootutil/src/encrypted.o \
./ra/mcu-tools/MCUboot/boot/bootutil/src/fault_injection_hardening.o \
./ra/mcu-tools/MCUboot/boot/bootutil/src/fault_injection_hardening_delay_rng_mbedtls.o \
./ra/mcu-tools/MCUboot/boot/bootutil/src/image_ec.o \
./ra/mcu-tools/MCUboot/boot/bootutil/src/image_ec256.o \
./ra/mcu-tools/MCUboot/boot/bootutil/src/image_ed25519.o \
./ra/mcu-tools/MCUboot/boot/bootutil/src/image_rsa.o \
./ra/mcu-tools/MCUboot/boot/bootutil/src/image_validate.o \
./ra/mcu-tools/MCUboot/boot/bootutil/src/loader.o \
./ra/mcu-tools/MCUboot/boot/bootutil/src/swap_misc.o \
./ra/mcu-tools/MCUboot/boot/bootutil/src/swap_move.o \
./ra/mcu-tools/MCUboot/boot/bootutil/src/swap_scratch.o \
./ra/mcu-tools/MCUboot/boot/bootutil/src/tlv.o 

SREC += \
mcuboot_ra2l1.srec 

MAP += \
mcuboot_ra2l1.map 


# Each subdirectory must supply rules for building sources it contributes
ra/mcu-tools/MCUboot/boot/bootutil/src/%.o: ../ra/mcu-tools/MCUboot/boot/bootutil/src/%.c
	$(file > $@.in,-mcpu=cortex-m23 -mthumb -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -Wunused -Wuninitialized -Wall -Wextra -Wmissing-declarations -Wconversion -Wpointer-arith -Wshadow -Wlogical-op -Waggregate-return -Wfloat-equal  -g -gdwarf-4 -D_RENESAS_RA_ -D_RA_CORE=CM23 -I"C:/workspace/e2_studio/workspace/MultiReader/mcuboot_ra2l1/src" -I"C:/workspace/e2_studio/workspace/MultiReader/mcuboot_ra2l1/ra/fsp/inc" -I"C:/workspace/e2_studio/workspace/MultiReader/mcuboot_ra2l1/ra/fsp/inc/api" -I"C:/workspace/e2_studio/workspace/MultiReader/mcuboot_ra2l1/ra/fsp/inc/instances" -I"C:/workspace/e2_studio/workspace/MultiReader/mcuboot_ra2l1/ra/arm/CMSIS_5/CMSIS/Core/Include" -I"C:/workspace/e2_studio/workspace/MultiReader/mcuboot_ra2l1/ra_gen" -I"C:/workspace/e2_studio/workspace/MultiReader/mcuboot_ra2l1/ra_cfg/fsp_cfg/bsp" -I"C:/workspace/e2_studio/workspace/MultiReader/mcuboot_ra2l1/ra_cfg/fsp_cfg" -I"C:/workspace/e2_studio/workspace/MultiReader/mcuboot_ra2l1/ra/mcu-tools/MCUboot/boot/bootutil/src" -I"C:/workspace/e2_studio/workspace/MultiReader/mcuboot_ra2l1/ra_cfg/mcu-tools/include" -I"C:/workspace/e2_studio/workspace/MultiReader/mcuboot_ra2l1/ra/mcu-tools/MCUboot/boot/bootutil/include" -I"C:/workspace/e2_studio/workspace/MultiReader/mcuboot_ra2l1/ra/fsp/src/rm_mcuboot_port" -I"C:/workspace/e2_studio/workspace/MultiReader/mcuboot_ra2l1/ra/mcu-tools/MCUboot/ext/tinycrypt/lib/include" -I"C:/workspace/e2_studio/workspace/MultiReader/mcuboot_ra2l1/ra/mcu-tools/MCUboot/ext/mbedtls-asn1/include" -I"C:/workspace/e2_studio/workspace/MultiReader/mcuboot_ra2l1/ra_cfg/mcu-tools/include/mcuboot_config" -I"C:/workspace/e2_studio/workspace/MultiReader/mcuboot_ra2l1/ra_cfg/mcu-tools/include/sysflash" -std=c99 -w -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -c -o "$@" -x c "$<")
	@echo Building file: $< && arm-none-eabi-gcc @"$@.in"

