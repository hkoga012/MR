/*!------------------------------------------------
@brief WDTクラス
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
WDT::WDT()
{
}

/*!------------------------------------------------
@brief 初期化
@note  
@param なし
@return エラーコード
@author
-------------------------------------------------*/
TYPE_ERROR WDT::Init(void)
{
	fsp_err_t err = FSP_SUCCESS;
	
	err = R_WDT_Open(&g_wdt0_ctrl, &g_wdt0_cfg);
    if (FSP_SUCCESS != err)
    {
        return E_SYS;
    }

    return E_OK;
}

/*!------------------------------------------------
@brief WDTリフレッシュ
@param なし
@return エラーコード
@author
-------------------------------------------------*/
TYPE_ERROR WDT::Refresh(void)
{
	fsp_err_t err = FSP_SUCCESS;
	err = R_WDT_Refresh(&g_wdt0_ctrl);
    if (FSP_SUCCESS != err)
    {
		return E_SYS;
    }
	return E_OK;
}

