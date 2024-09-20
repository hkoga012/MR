################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../ra/mcu-tools/MCUboot/ext/tinycrypt/lib/source/aes_decrypt.c \
../ra/mcu-tools/MCUboot/ext/tinycrypt/lib/source/aes_encrypt.c \
../ra/mcu-tools/MCUboot/ext/tinycrypt/lib/source/cbc_mode.c \
../ra/mcu-tools/MCUboot/ext/tinycrypt/lib/source/ccm_mode.c \
../ra/mcu-tools/MCUboot/ext/tinycrypt/lib/source/cmac_mode.c \
../ra/mcu-tools/MCUboot/ext/tinycrypt/lib/source/ctr_mode.c \
../ra/mcu-tools/MCUboot/ext/tinycrypt/lib/source/ctr_prng.c \
../ra/mcu-tools/MCUboot/ext/tinycrypt/lib/source/ecc.c \
../ra/mcu-tools/MCUboot/ext/tinycrypt/lib/source/ecc_dh.c \
../ra/mcu-tools/MCUboot/ext/tinycrypt/lib/source/ecc_dsa.c \
../ra/mcu-tools/MCUboot/ext/tinycrypt/lib/source/ecc_platform_specific.c \
../ra/mcu-tools/MCUboot/ext/tinycrypt/lib/source/gcm_mode.c \
../ra/mcu-tools/MCUboot/ext/tinycrypt/lib/source/hmac.c \
../ra/mcu-tools/MCUboot/ext/tinycrypt/lib/source/hmac_prng.c \
../ra/mcu-tools/MCUboot/ext/tinycrypt/lib/source/sha256.c \
../ra/mcu-tools/MCUboot/ext/tinycrypt/lib/source/utils.c 

C_DEPS += \
./ra/mcu-tools/MCUboot/ext/tinycrypt/lib/source/aes_decrypt.d \
./ra/mcu-tools/MCUboot/ext/tinycrypt/lib/source/aes_encrypt.d \
./ra/mcu-tools/MCUboot/ext/tinycrypt/lib/source/cbc_mode.d \
./ra/mcu-tools/MCUboot/ext/tinycrypt/lib/source/ccm_mode.d \
./ra/mcu-tools/MCUboot/ext/tinycrypt/lib/source/cmac_mode.d \
./ra/mcu-tools/MCUboot/ext/tinycrypt/lib/source/ctr_mode.d \
./ra/mcu-tools/MCUboot/ext/tinycrypt/lib/source/ctr_prng.d \
./ra/mcu-tools/MCUboot/ext/tinycrypt/lib/source/ecc.d \
./ra/mcu-tools/MCUboot/ext/tinycrypt/lib/source/ecc_dh.d \
./ra/mcu-tools/MCUboot/ext/tinycrypt/lib/source/ecc_dsa.d \
./ra/mcu-tools/MCUboot/ext/tinycrypt/lib/source/ecc_platform_specific.d \
./ra/mcu-tools/MCUboot/ext/tinycrypt/lib/source/gcm_mode.d \
./ra/mcu-tools/MCUboot/ext/tinycrypt/lib/source/hmac.d \
./ra/mcu-tools/MCUboot/ext/tinycrypt/lib/source/hmac_prng.d \
./ra/mcu-tools/MCUboot/ext/tinycrypt/lib/source/sha256.d \
./ra/mcu-tools/MCUboot/ext/tinycrypt/lib/source/utils.d 

OBJS += \
./ra/mcu-tools/MCUboot/ext/tinycrypt/lib/source/aes_decrypt.o \
./ra/mcu-tools/MCUboot/ext/tinycrypt/lib/source/aes_encrypt.o \
./ra/mcu-tools/MCUboot/ext/tinycrypt/lib/source/cbc_mode.o \
./ra/mcu-tools/MCUboot/ext/tinycrypt/lib/source/ccm_mode.o \
./ra/mcu-tools/MCUboot/ext/tinycrypt/lib/source/cmac_mode.o \
./ra/mcu-tools/MCUboot/ext/tinycrypt/lib/source/ctr_mode.o \
./ra/mcu-tools/MCUboot/ext/tinycrypt/lib/source/ctr_prng.o \
./ra/mcu-tools/MCUboot/ext/tinycrypt/lib/source/ecc.o \
./ra/mcu-tools/MCUboot/ext/tinycrypt/lib/source/ecc_dh.o \
./ra/mcu-tools/MCUboot/ext/tinycrypt/lib/source/ecc_dsa.o \
./ra/mcu-tools/MCUboot/ext/tinycrypt/lib/source/ecc_platform_specific.o \
./ra/mcu-tools/MCUboot/ext/tinycrypt/lib/source/gcm_mode.o \
./ra/mcu-tools/MCUboot/ext/tinycrypt/lib/source/hmac.o \
./ra/mcu-tools/MCUboot/ext/tinycrypt/lib/source/hmac_prng.o \
./ra/mcu-tools/MCUboot/ext/tinycrypt/lib/source/sha256.o \
./ra/mcu-tools/MCUboot/ext/tinycrypt/lib/source/utils.o 

SREC += \
mcuboot_ra2l1.srec 

MAP += \
mcuboot_ra2l1.map 


# Each subdirectory must supply rules for building sources it contributes
ra/mcu-tools/MCUboot/ext/tinycrypt/lib/source/%.o: ../ra/mcu-tools/MCUboot/ext/tinycrypt/lib/source/%.c
	$(file > $@.in,-mcpu=cortex-m23 -mthumb -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -Wunused -Wuninitialized -Wall -Wextra -Wmissing-declarations -Wconversion -Wpointer-arith -Wshadow -Wlogical-op -Waggregate-return -Wfloat-equal  -g -gdwarf-4 -D_RENESAS_RA_ -D_RA_CORE=CM23 -I"C:/workspace/e2_studio/workspace/MultiReader/mcuboot_ra2l1/src" -I"C:/workspace/e2_studio/workspace/MultiReader/mcuboot_ra2l1/ra/fsp/inc" -I"C:/workspace/e2_studio/workspace/MultiReader/mcuboot_ra2l1/ra/fsp/inc/api" -I"C:/workspace/e2_studio/workspace/MultiReader/mcuboot_ra2l1/ra/fsp/inc/instances" -I"C:/workspace/e2_studio/workspace/MultiReader/mcuboot_ra2l1/ra/arm/CMSIS_5/CMSIS/Core/Include" -I"C:/workspace/e2_studio/workspace/MultiReader/mcuboot_ra2l1/ra_gen" -I"C:/workspace/e2_studio/workspace/MultiReader/mcuboot_ra2l1/ra_cfg/fsp_cfg/bsp" -I"C:/workspace/e2_studio/workspace/MultiReader/mcuboot_ra2l1/ra_cfg/fsp_cfg" -I"C:/workspace/e2_studio/workspace/MultiReader/mcuboot_ra2l1/ra/mcu-tools/MCUboot/boot/bootutil/src" -I"C:/workspace/e2_studio/workspace/MultiReader/mcuboot_ra2l1/ra_cfg/mcu-tools/include" -I"C:/workspace/e2_studio/workspace/MultiReader/mcuboot_ra2l1/ra/mcu-tools/MCUboot/boot/bootutil/include" -I"C:/workspace/e2_studio/workspace/MultiReader/mcuboot_ra2l1/ra/fsp/src/rm_mcuboot_port" -I"C:/workspace/e2_studio/workspace/MultiReader/mcuboot_ra2l1/ra/mcu-tools/MCUboot/ext/tinycrypt/lib/include" -I"C:/workspace/e2_studio/workspace/MultiReader/mcuboot_ra2l1/ra/mcu-tools/MCUboot/ext/mbedtls-asn1/include" -I"C:/workspace/e2_studio/workspace/MultiReader/mcuboot_ra2l1/ra_cfg/mcu-tools/include/mcuboot_config" -I"C:/workspace/e2_studio/workspace/MultiReader/mcuboot_ra2l1/ra_cfg/mcu-tools/include/sysflash" -std=c99 -w -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -c -o "$@" -x c "$<")
	@echo Building file: $< && arm-none-eabi-gcc @"$@.in"

