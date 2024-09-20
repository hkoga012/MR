/*!------------------------------------------------
@brief PWMクラス
@note　
@author
-------------------------------------------------*/
#include "DeviceDriver.h"


/*!------------------------------------------------
@brief コンストラクタ
@note
@param なし
@return
@author
-------------------------------------------------*/
PWM::PWM(timer_ctrl_t * const pTimerCtrl, timer_cfg_t const * const pTimerCfg)
 :_pTimerCtrl(pTimerCtrl),_pTimerCfg(pTimerCfg){}

/*!------------------------------------------------
@brief 初期化
@note  
@param なし
@return エラーコード
@author
-------------------------------------------------*/
TYPE_ERROR PWM::Init(void)
{
	fsp_err_t err = FSP_SUCCESS;
	
	err = R_GPT_Open(_pTimerCtrl, _pTimerCfg);
    if (FSP_SUCCESS != err)
    {
        return E_SYS;
    }
    return E_OK;
}

/*!------------------------------------------------
@brief 開始
@param なし
@return エラーコード
@author
-------------------------------------------------*/
TYPE_ERROR PWM::Start(void)
{
	fsp_err_t err = FSP_SUCCESS;
	err = R_GPT_Start(_pTimerCtrl);
    if (FSP_SUCCESS != err)
    {
        return E_SYS;
    }
    return E_OK;
}

/*!------------------------------------------------
@brief 停止
@param なし
@return エラーコード
@author
-------------------------------------------------*/
TYPE_ERROR PWM::Stop(void)
{
	fsp_err_t err = FSP_SUCCESS;
	err = R_GPT_Stop(_pTimerCtrl);
    if (FSP_SUCCESS != err)
    {
        return E_SYS;
    }
    return E_OK;
}

/*!------------------------------------------------
@brief リセット
@param なし
@return エラーコード
@author
-------------------------------------------------*/
TYPE_ERROR PWM::Reset(void)
{
	fsp_err_t err = FSP_SUCCESS;
	err = R_GPT_Reset(_pTimerCtrl);
    if (FSP_SUCCESS != err)
    {
        return E_SYS;
    }
    return E_OK;
}

/*!------------------------------------------------
@brief デューティ比設定
@param Duty 0-100で指定(単位％)
@return エラーコード
@author
-------------------------------------------------*/
TYPE_ERROR PWM::SetDuty(uint8_t Duty)
{
    fsp_err_t err = FSP_SUCCESS;
	timer_info_t info = {(timer_direction_t)0, 0, 0};
	uint32_t DutyCycle = 0;
    uint32_t PeriodCounts = 0;
	
	err = R_GPT_InfoGet(_pTimerCtrl, &info);
    if (FSP_SUCCESS != err)
    {
    	return E_SYS;
    }
	
	PeriodCounts = info.period_counts;
	DutyCycle =(uint32_t) ((uint64_t) (PeriodCounts * Duty) / GPT_MAX_PERCENT);
	
	//出力先はGTIOCAのみ
	err = R_GPT_DutyCycleSet(_pTimerCtrl, DutyCycle, GPT_IO_PIN_GTIOCA);
    if(FSP_SUCCESS != err)
    {
    	return E_SYS;
    }
	return E_OK;
}


