/*!------------------------------------------------
@brief AGTクラス
@note　メインタイマークラス
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
AGT::AGT()
{
}

/*!------------------------------------------------
@brief AGTタイマー初期化
@note  
@param なし
@return エラーコード
@author
-------------------------------------------------*/
TYPE_ERROR AGT::Init(void)
{
	fsp_err_t err = FSP_SUCCESS;     // Error status
	
	err = R_AGT_Open(&g_timer0_ctrl, &g_timer0_cfg);
    if (FSP_SUCCESS != err)
	{
		return E_SYS;
	}
    return E_OK;
}

/*!------------------------------------------------
@brief AGTタイマー開始
@note  
@param なし
@return エラーコード
@author
-------------------------------------------------*/
TYPE_ERROR AGT::Start(void)
{
	fsp_err_t err = R_AGT_Start(&g_timer0_ctrl);
	if (FSP_SUCCESS != err)
	{
		return E_SYS;
	}
    return E_OK;
}

/*!------------------------------------------------
@brief AGTタイマー停止
@note  
@param なし
@return エラーコード
@author
-------------------------------------------------*/
TYPE_ERROR AGT::Stop(void)
{
	fsp_err_t err = R_AGT_Stop(&g_timer0_ctrl);
	if (FSP_SUCCESS != err)
	{
		return E_SYS;
	}
    return E_OK;
}



