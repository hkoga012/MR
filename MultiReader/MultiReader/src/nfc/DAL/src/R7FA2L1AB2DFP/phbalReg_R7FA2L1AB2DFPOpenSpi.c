/*!------------------------------------------------
@brief NFC用SPI制御
@note　phbalReg_LpcOpenSpi.cをベースに作成
@author
-------------------------------------------------*/

#include "phDriver.h"
#include "hal_data.h"
#include "BoardSelection.h"
#include "ph_Status.h"

#define MAX_COUNT 0x7FFF
#define MIN_COUNT (0)
static volatile uint32_t g_spi_wait_count = MAX_COUNT;
static volatile spi_event_t g_spi_event_flag;    // Transfer Event completion flag
void spi0_callback(spi_callback_args_t * p_args);

//初期化
phStatus_t phbalReg_Init(
	void * pDataParams,
	uint16_t wSizeOfDataParams
)
{
	fsp_err_t err = FSP_SUCCESS;
    //volatile uint32_t delay;

    if((pDataParams == NULL) || (sizeof(phbalReg_Type_t) != wSizeOfDataParams))
    {
        return (PH_DRIVER_ERROR);
    }

    ((phbalReg_Type_t *)pDataParams)->wId      = PH_COMP_DRIVER;
    ((phbalReg_Type_t *)pDataParams)->bBalType = PHBAL_REG_TYPE_SPI;

    err = R_SPI_Open (&g_spi1_ctrl, &g_spi1_cfg);
    if (FSP_SUCCESS != err)
    {
        R_SPI_Close(&g_spi1_ctrl);
        return (PH_DRIVER_ERROR);;
    }

    /* Wait Startup time */
    //for(delay=0; delay<10000; delay++){}

    return PH_DRIVER_SUCCESS;
}

//SPIデータ送受信
phStatus_t phbalReg_Exchange(
	void * pDataParams,
	uint16_t wOption,
	uint8_t * pTxBuffer,
	uint16_t wTxLength,
	uint16_t wRxBufSize,
	uint8_t * pRxBuffer,
	uint16_t * pRxLength
)
{
    (void)pDataParams;
    (void)wOption;
    fsp_err_t err = FSP_SUCCESS;
    uint16_t xferLen = 0;
	g_spi_event_flag = (spi_event_t)0;
	g_spi_wait_count = MAX_COUNT;

	if ( wRxBufSize == 0 )
    {
	    err = R_SPI_Write(&g_spi1_ctrl, pTxBuffer, wTxLength, SPI_BIT_WIDTH_8_BITS);
        xferLen = wTxLength;
    }
    else
    {
        if ( wTxLength == 0 )
        {
        	//PN1590はReadする際にクロックを発生させるためのダミーデータに0xFFを送信する必要があるため
        	//R_SPI_Read関数内のダミーデータを0x00 -> 0xFFに変更している。
        	//e2studioでSPI設定を変更した場合はこの変更が消えてしまうので注意すること。
        	//場所：\ra\fsp\src\r_spi\r_spi.c の937行目
            err = R_SPI_Read(&g_spi1_ctrl, pRxBuffer, wRxBufSize, SPI_BIT_WIDTH_8_BITS);
            xferLen = (uint16_t)g_spi1_ctrl.rx_count;
        }
        else
        {
            err = R_SPI_WriteRead(&g_spi1_ctrl, pTxBuffer, pRxBuffer, wTxLength, SPI_BIT_WIDTH_8_BITS);
            xferLen = (uint16_t)g_spi1_ctrl.rx_count;
        }
    }
	//送受信開始
    if(FSP_SUCCESS != err)
    {
        return (PH_DRIVER_FAILURE);
    }

	//送信＆受信待ち
    while((SPI_EVENT_TRANSFER_COMPLETE != g_spi_event_flag))
    {
        if ( g_spi_wait_count > MIN_COUNT)
        {
            g_spi_wait_count--;
        }
        if (MIN_COUNT == g_spi_wait_count)
        {
            return (PH_DRIVER_FAILURE);
        }
        else if ((SPI_EVENT_TRANSFER_ABORTED == g_spi_event_flag))
        {
            return (PH_DRIVER_FAILURE);
        }
        else
        {
            /* Do nothing */
        }
    }
	if ( pRxLength != NULL)
	{
    	*pRxLength = xferLen;
	}

    return PH_DRIVER_SUCCESS;
}

//未使用
phStatus_t phbalReg_SetConfig(
	void * pDataParams,
	uint16_t wConfig,
	uint32_t dwValue
)
{
    (void)pDataParams;(void)wConfig;(void)dwValue;
    return PH_DRIVER_SUCCESS;
}
//未使用
phStatus_t phbalReg_GetConfig(
	void * pDataParams,
	uint16_t wConfig,
	uint32_t * pValue
)
{
    (void)pDataParams;(void)wConfig;(void)pValue;
    return PH_DRIVER_SUCCESS;
}


//SPI0割り込みハンドラ
void g_spi1_callback(spi_callback_args_t * p_args)
{
    if (SPI_EVENT_TRANSFER_COMPLETE == p_args->event)
    {
        g_spi_event_flag = SPI_EVENT_TRANSFER_COMPLETE;
    }
    else
    {
        g_spi_event_flag = SPI_EVENT_TRANSFER_ABORTED;
    }
}
