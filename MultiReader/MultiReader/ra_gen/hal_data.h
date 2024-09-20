/* generated HAL header file - do not edit */
#ifndef HAL_DATA_H_
#define HAL_DATA_H_
#include <stdint.h>
#include "bsp_api.h"
#include "common_data.h"
#include "r_adc.h"
#include "r_adc_api.h"
#include "r_gpt.h"
#include "r_timer_api.h"
#include "r_agt.h"
#include "r_timer_api.h"
#include "r_flash_lp.h"
#include "r_flash_api.h"
#include "rm_vee_flash.h"
#include "r_wdt.h"
#include "r_wdt_api.h"
#include "r_dtc.h"
#include "r_transfer_api.h"
#include "r_spi.h"
#include "r_sci_uart.h"
#include "r_uart_api.h"
FSP_HEADER
/** ADC on ADC Instance. */
extern const adc_instance_t g_adc0;

/** Access the ADC instance using these structures when calling API functions directly (::p_api is not used). */
extern adc_instance_ctrl_t g_adc0_ctrl;
extern const adc_cfg_t g_adc0_cfg;
extern const adc_channel_cfg_t g_adc0_channel_cfg;

#ifndef NULL
void NULL(adc_callback_args_t *p_args);
#endif

#ifndef NULL
#define ADC_DMAC_CHANNELS_PER_BLOCK_NULL  2
#endif
/** Timer on GPT Instance. */
extern const timer_instance_t g_timer10;

/** Access the GPT instance using these structures when calling API functions directly (::p_api is not used). */
extern gpt_instance_ctrl_t g_timer10_ctrl;
extern const timer_cfg_t g_timer10_cfg;

#ifndef g_timer10_callback
void g_timer10_callback(timer_callback_args_t *p_args);
#endif
/** AGT Timer Instance */
extern const timer_instance_t g_timer3;

/** Access the AGT instance using these structures when calling API functions directly (::p_api is not used). */
extern agt_instance_ctrl_t g_timer3_ctrl;
extern const timer_cfg_t g_timer3_cfg;

#ifndef g_timer3_callback
void g_timer3_callback(timer_callback_args_t *p_args);
#endif
/* Flash on Flash LP Instance. */
extern const flash_instance_t g_flash1;

/** Access the Flash LP instance using these structures when calling API functions directly (::p_api is not used). */
extern flash_lp_instance_ctrl_t g_flash1_ctrl;
extern const flash_cfg_t g_flash1_cfg;

#ifndef rm_vee_flash_callback
void rm_vee_flash_callback(flash_callback_args_t *p_args);
#endif
extern const rm_vee_instance_t g_vee0;
extern rm_vee_flash_instance_ctrl_t g_vee0_ctrl;
extern const rm_vee_cfg_t g_vee0_cfg;

/** Callback used by VEE Instance. */
#ifndef vee_callback
void vee_callback(rm_vee_callback_args_t *p_args);
#endif
/* Flash on Flash LP Instance. */
extern const flash_instance_t g_flash0;

/** Access the Flash LP instance using these structures when calling API functions directly (::p_api is not used). */
extern flash_lp_instance_ctrl_t g_flash0_ctrl;
extern const flash_cfg_t g_flash0_cfg;

#ifndef NULL
void NULL(flash_callback_args_t *p_args);
#endif
/** WDT on WDT Instance. */
extern const wdt_instance_t g_wdt0;

/** Access the WDT instance using these structures when calling API functions directly (::p_api is not used). */
extern wdt_instance_ctrl_t g_wdt0_ctrl;
extern const wdt_cfg_t g_wdt0_cfg;

#ifndef NULL
void NULL(wdt_callback_args_t *p_args);
#endif
/** Timer on GPT Instance. */
extern const timer_instance_t g_timer8;

/** Access the GPT instance using these structures when calling API functions directly (::p_api is not used). */
extern gpt_instance_ctrl_t g_timer8_ctrl;
extern const timer_cfg_t g_timer8_cfg;

#ifndef NULL
void NULL(timer_callback_args_t *p_args);
#endif
/** Timer on GPT Instance. */
extern const timer_instance_t g_timer7;

/** Access the GPT instance using these structures when calling API functions directly (::p_api is not used). */
extern gpt_instance_ctrl_t g_timer7_ctrl;
extern const timer_cfg_t g_timer7_cfg;

#ifndef NULL
void NULL(timer_callback_args_t *p_args);
#endif
/** Timer on GPT Instance. */
extern const timer_instance_t g_timer6;

/** Access the GPT instance using these structures when calling API functions directly (::p_api is not used). */
extern gpt_instance_ctrl_t g_timer6_ctrl;
extern const timer_cfg_t g_timer6_cfg;

#ifndef NULL
void NULL(timer_callback_args_t *p_args);
#endif
/** Timer on GPT Instance. */
extern const timer_instance_t g_timer4;

/** Access the GPT instance using these structures when calling API functions directly (::p_api is not used). */
extern gpt_instance_ctrl_t g_timer4_ctrl;
extern const timer_cfg_t g_timer4_cfg;

#ifndef NULL
void NULL(timer_callback_args_t *p_args);
#endif
/** Timer on GPT Instance. */
extern const timer_instance_t g_timer5;

/** Access the GPT instance using these structures when calling API functions directly (::p_api is not used). */
extern gpt_instance_ctrl_t g_timer5_ctrl;
extern const timer_cfg_t g_timer5_cfg;

#ifndef NULL
void NULL(timer_callback_args_t *p_args);
#endif
/** Timer on GPT Instance. */
extern const timer_instance_t g_timer2;

/** Access the GPT instance using these structures when calling API functions directly (::p_api is not used). */
extern gpt_instance_ctrl_t g_timer2_ctrl;
extern const timer_cfg_t g_timer2_cfg;

#ifndef NULL
void NULL(timer_callback_args_t *p_args);
#endif
/** Timer on GPT Instance. */
extern const timer_instance_t g_timer1;

/** Access the GPT instance using these structures when calling API functions directly (::p_api is not used). */
extern gpt_instance_ctrl_t g_timer1_ctrl;
extern const timer_cfg_t g_timer1_cfg;

#ifndef NULL
void NULL(timer_callback_args_t *p_args);
#endif
/* Transfer on DTC Instance. */
extern const transfer_instance_t g_transfer1;

/** Access the DTC instance using these structures when calling API functions directly (::p_api is not used). */
extern dtc_instance_ctrl_t g_transfer1_ctrl;
extern const transfer_cfg_t g_transfer1_cfg;
/* Transfer on DTC Instance. */
extern const transfer_instance_t g_transfer0;

/** Access the DTC instance using these structures when calling API functions directly (::p_api is not used). */
extern dtc_instance_ctrl_t g_transfer0_ctrl;
extern const transfer_cfg_t g_transfer0_cfg;
/** SPI on SPI Instance. */
extern const spi_instance_t g_spi1;

/** Access the SPI instance using these structures when calling API functions directly (::p_api is not used). */
extern spi_instance_ctrl_t g_spi1_ctrl;
extern const spi_cfg_t g_spi1_cfg;

/** Callback used by SPI Instance. */
#ifndef g_spi1_callback
void g_spi1_callback(spi_callback_args_t *p_args);
#endif

#define RA_NOT_DEFINED (1)
#if (RA_NOT_DEFINED == g_transfer0)
    #define g_spi1_P_TRANSFER_TX (NULL)
#else
#define g_spi1_P_TRANSFER_TX (&g_transfer0)
#endif
#if (RA_NOT_DEFINED == g_transfer1)
    #define g_spi1_P_TRANSFER_RX (NULL)
#else
#define g_spi1_P_TRANSFER_RX (&g_transfer1)
#endif
#undef RA_NOT_DEFINED
/** UART on SCI Instance. */
extern const uart_instance_t g_uart0;

/** Access the UART instance using these structures when calling API functions directly (::p_api is not used). */
extern sci_uart_instance_ctrl_t g_uart0_ctrl;
extern const uart_cfg_t g_uart0_cfg;
extern const sci_uart_extended_cfg_t g_uart0_cfg_extend;

#ifndef uart_interrupt_callback
void uart_interrupt_callback(uart_callback_args_t *p_args);
#endif
/** UART on SCI Instance. */
extern const uart_instance_t g_uart2;

/** Access the UART instance using these structures when calling API functions directly (::p_api is not used). */
extern sci_uart_instance_ctrl_t g_uart2_ctrl;
extern const uart_cfg_t g_uart2_cfg;
extern const sci_uart_extended_cfg_t g_uart2_cfg_extend;

#ifndef uart_interrupt_callback
void uart_interrupt_callback(uart_callback_args_t *p_args);
#endif
/** UART on SCI Instance. */
extern const uart_instance_t g_uart3;

/** Access the UART instance using these structures when calling API functions directly (::p_api is not used). */
extern sci_uart_instance_ctrl_t g_uart3_ctrl;
extern const uart_cfg_t g_uart3_cfg;
extern const sci_uart_extended_cfg_t g_uart3_cfg_extend;

#ifndef uart_interrupt_callback
void uart_interrupt_callback(uart_callback_args_t *p_args);
#endif
/** UART on SCI Instance. */
extern const uart_instance_t g_uart1;

/** Access the UART instance using these structures when calling API functions directly (::p_api is not used). */
extern sci_uart_instance_ctrl_t g_uart1_ctrl;
extern const uart_cfg_t g_uart1_cfg;
extern const sci_uart_extended_cfg_t g_uart1_cfg_extend;

#ifndef uart_interrupt_callback
void uart_interrupt_callback(uart_callback_args_t *p_args);
#endif
/** AGT Timer Instance */
extern const timer_instance_t g_timer0;

/** Access the AGT instance using these structures when calling API functions directly (::p_api is not used). */
extern agt_instance_ctrl_t g_timer0_ctrl;
extern const timer_cfg_t g_timer0_cfg;

#ifndef timer0_Interrupt_callback
void timer0_Interrupt_callback(timer_callback_args_t *p_args);
#endif
void hal_entry(void);
void g_hal_init(void);
FSP_FOOTER
#endif /* HAL_DATA_H_ */
