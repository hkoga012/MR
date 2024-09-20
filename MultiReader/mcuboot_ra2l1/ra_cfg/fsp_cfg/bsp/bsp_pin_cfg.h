/* generated configuration header file - do not edit */
#ifndef BSP_PIN_CFG_H_
#define BSP_PIN_CFG_H_
#include "r_ioport.h"

/* Common macro for FSP header files. There is also a corresponding FSP_FOOTER macro at the end of this file. */
FSP_HEADER

#define SW1_1 (BSP_IO_PORT_00_PIN_00)
#define SW1_2 (BSP_IO_PORT_00_PIN_01)
#define SW1_3 (BSP_IO_PORT_00_PIN_02)
#define SW1_4 (BSP_IO_PORT_00_PIN_03)
#define SW1_5 (BSP_IO_PORT_00_PIN_04)
#define SW1_6 (BSP_IO_PORT_00_PIN_05)
#define SW1_7 (BSP_IO_PORT_00_PIN_06)
#define SW1_8 (BSP_IO_PORT_00_PIN_07)
#define BD_SEL1 (BSP_IO_PORT_00_PIN_08)
#define BD_SEL2 (BSP_IO_PORT_00_PIN_10)
#define BD_SEL3 (BSP_IO_PORT_00_PIN_11)
#define TEST1 (BSP_IO_PORT_00_PIN_12)
#define TEST2 (BSP_IO_PORT_00_PIN_13)
#define TEST3 (BSP_IO_PORT_00_PIN_14)
#define TEST4 (BSP_IO_PORT_00_PIN_15)
#define LED_Left_R (BSP_IO_PORT_01_PIN_00)
#define LED_Left_G (BSP_IO_PORT_01_PIN_01)
#define LED_Left_B (BSP_IO_PORT_01_PIN_02)
#define LED_Center_G (BSP_IO_PORT_01_PIN_03)
#define LED_Center_R (BSP_IO_PORT_01_PIN_04)
#define LED_Right_G (BSP_IO_PORT_01_PIN_05)
#define LED_Center_B (BSP_IO_PORT_01_PIN_06)
#define LED_Frame_G (BSP_IO_PORT_01_PIN_07)
#define Emulator_SWDIO (BSP_IO_PORT_01_PIN_08)
#define Emulator_TXD (BSP_IO_PORT_01_PIN_09)
#define Emulator_RXD (BSP_IO_PORT_01_PIN_10)
#define Buzzer (BSP_IO_PORT_01_PIN_15)
#define NFC_RST (BSP_IO_PORT_02_PIN_01)
#define SPI_MISO (BSP_IO_PORT_02_PIN_02)
#define SPI_MOSI (BSP_IO_PORT_02_PIN_03)
#define SPI_CLK (BSP_IO_PORT_02_PIN_04)
#define IRQ (BSP_IO_PORT_02_PIN_05)
#define Emulator_SWCLK (BSP_IO_PORT_03_PIN_00)
#define no_use_rxd2 (BSP_IO_PORT_03_PIN_01)
#define no_use_txd2 (BSP_IO_PORT_03_PIN_02)
#define SPI_SS (BSP_IO_PORT_03_PIN_06)
#define SW2_1 (BSP_IO_PORT_04_PIN_00)
#define SW2_2 (BSP_IO_PORT_04_PIN_01)
#define SW2_3 (BSP_IO_PORT_04_PIN_02)
#define SW2_4 (BSP_IO_PORT_04_PIN_03)
#define SW2_5 (BSP_IO_PORT_04_PIN_04)
#define SW2_6 (BSP_IO_PORT_04_PIN_05)
#define SW2_7 (BSP_IO_PORT_04_PIN_06)
#define SW2_8 (BSP_IO_PORT_04_PIN_07)
#define QR_Rx (BSP_IO_PORT_04_PIN_08)
#define QR_Tx (BSP_IO_PORT_04_PIN_09)
#define RS485_RO (BSP_IO_PORT_04_PIN_10)
#define RS485_DI (BSP_IO_PORT_04_PIN_11)
#define RS485_RE (BSP_IO_PORT_04_PIN_12)
#define RS485_DE (BSP_IO_PORT_04_PIN_13)
#define RS485_PO (BSP_IO_PORT_04_PIN_14)
#define SP_Tx (BSP_IO_PORT_05_PIN_01)
#define SP_Rx (BSP_IO_PORT_05_PIN_02)
#define LED_Right_R (BSP_IO_PORT_06_PIN_00)
#define LED_Frame_R (BSP_IO_PORT_06_PIN_01)
#define LED_Right_B (BSP_IO_PORT_06_PIN_02)
#define LED_Frame_B (BSP_IO_PORT_06_PIN_03)
#define AutoDoor (BSP_IO_PORT_06_PIN_08)
extern const ioport_cfg_t g_bsp_pin_cfg; /* R7FA2L1AB2DFP.pincfg */

void BSP_PinConfigSecurityInit();

/* Common macro for FSP header files. There is also a corresponding FSP_HEADER macro at the top of this file. */
FSP_FOOTER

#endif /* BSP_PIN_CFG_H_ */
