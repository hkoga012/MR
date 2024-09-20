/*----------------------------------------------------------------------------*/
/* Copyright 2017-2021 NXP                                                    */
/*                                                                            */
/* NXP Confidential. This software is owned or controlled by NXP and may only */
/* be used strictly in accordance with the applicable license terms.          */
/* By expressly accepting such terms or by downloading, installing,           */
/* activating and/or otherwise using the software, you are agreeing that you  */
/* have read, and that you agree to comply with and are bound by, such        */
/* license terms. If you do not agree to be bound by the applicable license   */
/* terms, then you may not retain, install, activate or otherwise use the     */
/* software.                                                                  */
/*----------------------------------------------------------------------------*/

/** \file
* Generic phDriver Component of Reader Library Framework.
* $Author$
* $Revision$
* $Date$
*
*/

#ifndef BOARD_MULTIREADER_H_
#define BOARD_MULTIREADER_H_

#include "hal_data.h"

#define PORT0               0          /**< Port0. */
#define PORT1               1          /**< Port1. */
#define PORT2               2          /**< Port2. */
#define PORT3               3          /**< Port3. */
#define PORT4               4          /**< Port4. */


/******************************************************************
 * Board Pin/Gpio configurations
 ******************************************************************/

#define PHDRIVER_PIN_DWL           ((PORT3<<8) | 5)   /**< Download mode pin of Frontend, Port3, pin5 未使用 */
#define PHDRIVER_PIN_RESET         ((PORT2<<8) | 1)    /**< Port2, pin01 NFC_RST*/
#define PHDRIVER_PIN_IRQ           ((PORT2<<8) | 5)   /**< Interrupt pin from Frontend to Host, Port2, pin5 */
#define PHDRIVER_PIN_BUSY          PHDRIVER_PIN_IRQ    /**< No busy pin in CERES/PN5190, handled by IRQ pin itself. */
#define EXT_IRQ_CHANNEL            (1)                 /* external irq stacksで指定した外部IRQチャネルを指定 */


/******************************************************************
 * PIN Pull-Up/Pull-Down configurations.
 ******************************************************************/
//e2studioで設定
#define PHDRIVER_PIN_RESET_PULL_CFG    PH_DRIVER_PULL_UP
#define PHDRIVER_PIN_IRQ_PULL_CFG      PH_DRIVER_PULL_DOWN
#define PHDRIVER_PIN_BUSY_PULL_CFG     PH_DRIVER_PULL_DOWN
#define PHDRIVER_PIN_DWL_PULL_CFG      PH_DRIVER_PULL_UP
#define PHDRIVER_PIN_NSS_PULL_CFG      PH_DRIVER_PULL_UP

/******************************************************************
 * IRQ PIN NVIC settings
 ******************************************************************/
//割り込みはe2studioで設定IRQ1、P205 を使用 優先度2
#define PIN_IRQ_TRIGGER_TYPE    PH_DRIVER_INTERRUPT_RISINGEDGE  /**< IRQ pin RISING edge interrupt */

/*****************************************************************
 * Front End Reset logic level settings
 ****************************************************************/
#define PH_DRIVER_SET_HIGH            1          /**< Logic High. */
#define PH_DRIVER_SET_LOW             0          /**< Logic Low. */
#define RESET_POWERDOWN_LEVEL PH_DRIVER_SET_LOW
#define RESET_POWERUP_LEVEL   PH_DRIVER_SET_HIGH


/*****************************************************************
 * SPI Configuration
 ****************************************************************/
 //SPI設定はe2studioで設定　割り込み優先度1
#define PHDRIVER_PIN_SSEL       (((PORT3<<8)| 6))  /**< Slave select, Port3, pin6, Pin function 0 */

/*****************************************************************
 * Timer Configuration
 ****************************************************************/
//Timerはe2studioで設定

#endif /* BOARD_MULTIREADER_H_ */
