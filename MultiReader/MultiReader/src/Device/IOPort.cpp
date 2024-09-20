/*!------------------------------------------------
@brief IOPortクラス
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
IOPort::IOPort(uart_ctrl_t  *pIoPortCtrl) : _pIoPortCtrl(pIoPortCtrl)
{
}

/*!------------------------------------------------
@brief 初期化
@note  
@param なし
@return エラーコード
@author
-------------------------------------------------*/
TYPE_ERROR IOPort::Init(void)
{
	fsp_err_t err = FSP_SUCCESS;
	
	err = R_IOPORT_Open(_pIoPortCtrl, &IOPORT_CFG_NAME);
    if (FSP_SUCCESS != err)
	{
		return E_SYS;
	}
    return E_OK;
}

/*!------------------------------------------------
@brief ポート出力
@param IoPortPin　ポートピン
@param Val　出力ポート状態(HIGH/LOW)
@return エラーコード
@author
-------------------------------------------------*/
TYPE_ERROR IOPort::Write(bsp_io_port_pin_t PortPin, bsp_io_level_t Val)
{
	fsp_err_t err = R_IOPORT_PinWrite(_pIoPortCtrl, (bsp_io_port_pin_t)PortPin, Val);
	if (FSP_SUCCESS != err)
	{
		return E_SYS;
	}
	//書込み値と比較(2回)
	bsp_io_level_t val1,val2;
	val1 = val2 = BSP_IO_LEVEL_LOW;
	err = R_IOPORT_PinRead(_pIoPortCtrl, (bsp_io_port_pin_t)PortPin, &val1);
	if (FSP_SUCCESS != err)
	{
		return E_SYS;
	}
	err = R_IOPORT_PinRead(_pIoPortCtrl, (bsp_io_port_pin_t)PortPin, &val2);
	if (FSP_SUCCESS != err)
	{
		return E_SYS;
	}
	if(Val != val1 || Val != val2)
	{
		return E_SYS;
	}
    return E_OK;
}

/*!------------------------------------------------
@brief ポート状態読出し
@note  
@param IoPortPin　ポートピン
@param pVal　ポート状態格納ポインタ
@return エラーコード
@author
-------------------------------------------------*/
TYPE_ERROR IOPort::Read(bsp_io_port_pin_t PortPin, bsp_io_level_t *pVal)
{
	fsp_err_t err = R_IOPORT_PinRead(_pIoPortCtrl, (bsp_io_port_pin_t)PortPin, pVal);
	if (FSP_SUCCESS != err)
	{
		return E_SYS;
	}
	return E_OK;
}


