/*!------------------------------------------------
@brief ADCクラス 単発で読むだけ
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
ADC::ADC()
{
}

/*!------------------------------------------------
@brief 初期化
@note  
@param なし
@return エラーコード
@author
-------------------------------------------------*/
TYPE_ERROR ADC::Init(void)
{
	fsp_err_t err = R_ADC_Open(&g_adc0_ctrl, &g_adc0_cfg);
    if (err != FSP_SUCCESS)
    {
        return E_SYS;
    }

    err = R_ADC_ScanCfg(&g_adc0_ctrl, &g_adc0_channel_cfg);
    if (err != FSP_SUCCESS)
    {
        return E_SYS;
    }

    // 連続スキャンモードで開始する。値は上書き
    (void) R_ADC_ScanStart(&g_adc0_ctrl);

    return E_OK;
}

/*!------------------------------------------------
@brief 素のAD値を返す
@return AD値
@author
-------------------------------------------------*/
uint16_t ADC::Read()
{
    uint16_t value = 0x1234;
    fsp_err_t err = R_ADC_Read (&g_adc0_ctrl, ADC_CHANNEL_20, &value);

    return ((err == FSP_SUCCESS) ? value : 0xffff);
}

/*!------------------------------------------------
@brief AD値を有効範囲で変換し割合を返す
@param minValue 有効範囲最小
@param maxValue 有効範囲最大
@return 変換した割合（0～100%）
@author
-------------------------------------------------*/
uint8_t ADC::ReadRate(uint16_t minValue, uint16_t maxValue)
{
    uint8_t rate = 50;
    uint16_t value = Read();
    if(value != 0xffff)
    {
        if(value > maxValue)
        {
            value = maxValue;
        }
        if(value < minValue)
        {
            value = minValue;
        }
        rate = (uint8_t)(((uint32_t)(value - minValue))*100 /(maxValue - minValue));
    }

    return rate;
}

