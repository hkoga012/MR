/*----------------------------------------------------------------------------*/
/* Copyright 2017-2022 NXP                                                    */
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
/*!------------------------------------------------
@brief NFC用IO制御
@note　phDriver_LPCOpen.cをベースに作成
@author
-------------------------------------------------*/

#include "phDriver.h"
#include "hal_data.h"

#include "BoardSelection.h"
#include "../../../../DebugOption.h"


static pphDriver_TimerCallBck_t pTimerIsrCallBack;
static volatile uint32_t dwTimerExp;

//タイマー割り込みコールバック
static void phDriver_TimerIsrCallBack(void);
/*
NFC用タイマー開始(リセット動作含む)
eTimerUnit：タイマー計測単位
dwTimePeriod: タイマー待ち時間(msec)
pTimerCallBack
*/
static timer_info_t  one_shot_info =
{
 .clock_frequency = 0,
 .count_direction = (timer_direction_t) 0,
 .period_counts = 0,
};
//ワンショットタイマー開始
extern void NFCDoTask(void);
phStatus_t phDriver_TimerStart(phDriver_Timer_Unit_t eTimerUnit, uint32_t dwTimePeriod, pphDriver_TimerCallBck_t pTimerCallBack)
{
	fsp_err_t err = FSP_SUCCESS;
	uint32_t clock_freq = 0;
    uint32_t raw_counts_oneshot = 0;
	
	if(pTimerCallBack == NULL)
    {
        dwTimerExp = 0;
        pTimerIsrCallBack = phDriver_TimerIsrCallBack;
    }else
    {
        pTimerIsrCallBack = pTimerCallBack;
    }
	
	R_GPT_Stop(&g_timer10_ctrl);

	//タイマー情報取得
	err = R_GPT_InfoGet(&g_timer10_ctrl, &one_shot_info);
    if(FSP_SUCCESS != err)
    {
    	//エラー
    	return PH_DRIVER_FAILURE;
    }
	
	//待ち時間を設定 最大　89.478sec　0xFFFFFFFF/(48MHz)
    clock_freq = one_shot_info.clock_frequency;
    raw_counts_oneshot = (uint32_t)(dwTimePeriod * (clock_freq  / eTimerUnit));
    err = R_GPT_PeriodSet(&g_timer10_ctrl, raw_counts_oneshot);
    if (FSP_SUCCESS != err)
    {
    	return PH_DRIVER_FAILURE;
    }

	//タイマー開始
    err = R_GPT_Start(&g_timer10_ctrl);
    if (FSP_SUCCESS != err)
    {
    	return PH_DRIVER_FAILURE;
    }
    
	//指定時間経過待ち
    if(pTimerCallBack == NULL)
    {
        while(!dwTimerExp)
    	{
    		//5msec以上待つ場合(48Mhz * 0.005sec =240000)はマルチリーダータスクを呼び出す
    		//全タスク実行時間が大きいとタイマーの精度が悪くなるため注意すること。
    		//NFCDoTaskの実行時間は最大5msec以下にしたい。
    		if(raw_counts_oneshot > 240000)
    		{
    			//状態遷移タスクを実行
    			NFCDoTask();
    		}
    	};
    }
    return PH_DRIVER_SUCCESS;
}
//ワンショットタイマー停止
phStatus_t phDriver_TimerStop(void)
{
    fsp_err_t err = R_GPT_Stop(&g_timer10_ctrl);
	if (FSP_SUCCESS != err)
    {
    	return PH_DRIVER_FAILURE;
    }

    return PH_DRIVER_SUCCESS;
}

//割り込みピン設定
static phStatus_t phDriver_PinConfigInterrupt(uint32_t dwPinNumber, phDriver_Pin_Config_t *pPinConfig)
{
	fsp_err_t err = FSP_SUCCESS;
	//割り込みピン設定はここで設定せず、e2studioのStack設定で行っていることとする
    err = R_ICU_ExternalIrqEnable(&g_external_irq1_ctrl);
	if (FSP_SUCCESS != err)
    {
    	return PH_DRIVER_FAILURE;
    }
	
    return PH_DRIVER_SUCCESS;
}

//GPIOピン設定
//e2studioのStack設定で行っていることとする
static phStatus_t phDriver_PinConfigGpio(uint32_t dwPinNumber, phDriver_Pin_Func_t ePinFunc,
        phDriver_Pin_Config_t *pPinConfig)
{
    uint32_t val = 0;
    phStatus_t wStatus = PH_DRIVER_SUCCESS;
	fsp_err_t err = FSP_SUCCESS;

    switch(ePinFunc)
    {
    case PH_DRIVER_PINFUNC_INPUT:
    	val = (uint32_t)IOPORT_CFG_PORT_DIRECTION_INPUT;
        err = R_IOPORT_PinCfg((ioport_ctrl_t * const)&g_bsp_pin_cfg, dwPinNumber , val);
        break;

    case PH_DRIVER_PINFUNC_OUTPUT:
    	//出力
    	val = (uint32_t)IOPORT_CFG_PORT_DIRECTION_OUTPUT;
    	
    	//出力初期値
    	if( PH_DRIVER_SET_HIGH == pPinConfig->bOutputLogic )
		{
			val |= (uint32_t)IOPORT_CFG_PORT_OUTPUT_HIGH;
		}
		else 
		{
			val |= (uint32_t)IOPORT_CFG_PORT_OUTPUT_LOW;
		}
    	err = R_IOPORT_PinCfg((ioport_ctrl_t * const)&g_bsp_pin_cfg, dwPinNumber , val);
    	if (FSP_SUCCESS != err)
    	{
    		wStatus = PH_DRIVER_ERROR;
    	}
        break;

    default:
        wStatus = PH_DRIVER_ERROR;
    }

    return wStatus;
}

phStatus_t phDriver_PinConfig(uint32_t dwPinNumber, phDriver_Pin_Func_t ePinFunc, phDriver_Pin_Config_t *pPinConfig)
{
    //uint8_t bPortNum;
    //uint8_t bPinNum;
    phStatus_t wStatus;

    //bPortNum = (uint8_t)((dwPinNumber & 0x0000FF00) >> 8);
    //bPinNum = (uint8_t)(dwPinNumber & 0x000000FF);
    if(pPinConfig == NULL)
    {
        wStatus = PH_DRIVER_ERROR;
        return wStatus;
    }

    if(ePinFunc == PH_DRIVER_PINFUNC_INTERRUPT)
    {
        /* Level triggered interrupts are NOT supported in LPC1769. */
        if((pPinConfig->eInterruptConfig == PH_DRIVER_INTERRUPT_LEVELZERO) ||
                (pPinConfig->eInterruptConfig == PH_DRIVER_INTERRUPT_LEVELONE))
        {
            wStatus = PH_DRIVER_ERROR;
        }
        else
        {
            wStatus = phDriver_PinConfigInterrupt(dwPinNumber & 0x0000FFFF, pPinConfig);
        }
    }
    else
    {
        wStatus = phDriver_PinConfigGpio(dwPinNumber & 0x0000FFFF, ePinFunc, pPinConfig);
    }

    return wStatus;
}

//ピン状態読出し
//割り込みピンも同じように読み出す
uint8_t phDriver_PinRead(uint32_t dwPinNumber, phDriver_Pin_Func_t ePinFunc)
{
    (void)ePinFunc;
    bsp_io_level_t Val;
    uint8_t bPinStatus = 0;
	fsp_err_t err = R_IOPORT_PinRead((ioport_ctrl_t * const)&g_bsp_pin_cfg, (bsp_io_port_pin_t)dwPinNumber, &Val);
	if (FSP_SUCCESS == err)
	{
		if(BSP_IO_LEVEL_HIGH == Val)
		{
			bPinStatus = 1;
		}
		else
		{
			bPinStatus = 0;
		}
	}

    return bPinStatus;
}

phStatus_t phDriver_IRQPinPoll(uint32_t dwPinNumber, phDriver_Pin_Func_t ePinFunc, phDriver_Interrupt_Config_t eInterruptType)
{
    uint8_t    bGpioState = 0;

    if ((eInterruptType != PH_DRIVER_INTERRUPT_RISINGEDGE) && (eInterruptType != PH_DRIVER_INTERRUPT_FALLINGEDGE))
    {
        return PH_DRIVER_ERROR;
    }

    if (eInterruptType == PH_DRIVER_INTERRUPT_FALLINGEDGE)
    {
        bGpioState = 1;
    }

	while(phDriver_PinRead(dwPinNumber, ePinFunc) == bGpioState);

    return PH_DRIVER_SUCCESS;
}
extern void NFCResetPinWriteError(void);
void phDriver_PinWrite(uint32_t dwPinNumber, uint8_t bValue)
{
	bsp_io_level_t Val;
    //uint8_t bPortNum;
    //uint8_t bPinNum;

    //bPortNum = (uint8_t)((dwPinNumber & 0x0000FF00) >> 8);
    //bPinNum = (uint8_t)(dwPinNumber & 0x000000FF);

	if(PH_DRIVER_SET_HIGH == bValue)
	{
		Val = BSP_IO_LEVEL_HIGH;
	}
	else
	{
		Val = BSP_IO_LEVEL_LOW;
	}
	bool IsSuccess = false;
	fsp_err_t err = R_IOPORT_PinWrite((ioport_ctrl_t * const)&g_bsp_pin_cfg, (bsp_io_port_pin_t)dwPinNumber & 0x0000FFFF, Val);
	if (FSP_SUCCESS == err)
	{
		//書込み値と比較(2回)
		bsp_io_level_t val1,val2;
		val1 = val2 = BSP_IO_LEVEL_LOW;
		err = R_IOPORT_PinRead((ioport_ctrl_t * const)&g_bsp_pin_cfg, (bsp_io_port_pin_t)dwPinNumber, &val1);
		if (FSP_SUCCESS == err)
		{
			
			err = R_IOPORT_PinRead((ioport_ctrl_t * const)&g_bsp_pin_cfg, (bsp_io_port_pin_t)dwPinNumber, &val2);
			if (FSP_SUCCESS == err)
			{
				
				if(Val == val1 && Val == val2)
				{
					//成功
				    IsSuccess = true;
				}
			}
		}
	}
#ifdef NFC_ERROR_DEBUG_RW_RESET
	IsSuccess = false;
#endif /* NFC_ERROR_DEBUG_RW_RESET */
	if(false == IsSuccess)
	{
		//リセットピン出力エラー通知
		if(dwPinNumber == PHDRIVER_PIN_RESET)
		{
		    NFCResetPinWriteError();
		}
	}
}

void phDriver_PinClearIntStatus(uint32_t dwPinNumber)
{
    (void)dwPinNumber;
    //未使用
}

//Timer0コールバック関数(指定時間経過後に呼び出される)
void g_timer10_callback(timer_callback_args_t *p_args)
{
    if(TIMER_EVENT_CYCLE_END == p_args->event)
    {
        pTimerIsrCallBack();
    }
}

static void phDriver_TimerIsrCallBack(void)
{
    dwTimerExp = 1;
}

void phDriver_EnterCriticalSection(void)
{
    __disable_irq();
}

void phDriver_ExitCriticalSection(void)
{
    __enable_irq();
}

phStatus_t phDriver_IRQPinRead(uint32_t dwPinNumber)
{
	phStatus_t bGpioVal = false;

	bGpioVal = phDriver_PinRead(dwPinNumber, PH_DRIVER_PINFUNC_INPUT);

	return bGpioVal;
}

