/*!------------------------------------------------
@brief デバイスドライバヘッダ
@note　
@author
-------------------------------------------------*/

#ifndef DEVICE_DRIVER_H
#define DEVICE_DRIVER_H
#include "../StateTransition/common.h"

#ifndef WINDOWS_DEBUG
#include "WDT.h"
#include "PWM.h"
#include "IOPort.h"
#include "AGT.h"
#include "UART.h"
#include "ADC.h"

extern WDT *G_pDrvWDT;

extern PWM *G_pDrvPWM_LED_Left_G;
extern PWM *G_pDrvPWM_LED_Center_G;
extern PWM *G_pDrvPWM_LED_Right_G;
extern PWM *G_pDrvPWM_LED_Frame_G;
extern PWM *G_pDrvPWM_LED_Frame_R;
extern PWM *G_pDrvPWM_LED_Frame_B;
extern PWM *G_pDrvPWM_Buzzer;

extern IOPort *G_pDrvIOPort;

extern AGT *G_pDrvAGT;

extern UART *G_pDrvUART_BLE;
extern UART *G_pDrvUART_Reserved;
extern UART *G_pDrvUART_QRReader;
extern UART *G_pDrvUART_RS485;

extern ADC *G_pDrvADC_Vol;

#endif
#endif /*WINDOWS_DEBUG*/

