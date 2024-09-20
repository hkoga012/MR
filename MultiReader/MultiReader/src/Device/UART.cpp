/*!------------------------------------------------
@brief UARTクラス
@note　
@author
-------------------------------------------------*/
#include "DeviceDriver.h"

void uart_interrupt_callback(uart_callback_args_t *);
/*!------------------------------------------------
@brief コンストラクタ
@note
@param RxBufSize 受信バッファサイズ
@param TxBufSize 送信バッファサイズ
@param pApiCtrl UART制御構造体ポインタ
@param pCfg UART設定構造体ポインタ
@return
@author
-------------------------------------------------*/
UART::UART(uart_ctrl_t *pApiCtrl, uart_cfg_t *pCfg, uint32_t RxBufSize, uint32_t TxBufSize)
{
    _rxBuffer.pBuf = new uint8_t[RxBufSize];
    _txBuffer.pBuf = new uint8_t[TxBufSize];
	
	_rxBufSize = RxBufSize;
	_txBufSize = TxBufSize;
	_pApiCtrl = pApiCtrl;
	_pCfg = pCfg;
	
    _SendingFlag = false;

    _pRxTop = &_rxBuffer.pBuf[0];
    _pRxEnd = &_rxBuffer.pBuf[RxBufSize-1];
    _pTxTop = &_txBuffer.pBuf[0];
    _pTxEnd = &_txBuffer.pBuf[TxBufSize-1];
	
	_rxBuffer.pW = _rxBuffer.pR = _pRxTop;
	_rxBuffer.len = 0;
	
	_txBuffer.pW = _txBuffer.pR = _pTxTop;
	_txBuffer.len = 0;

	_UARTError = UART_ERR_NONE;
}

/*!------------------------------------------------
@brief リセット
@note
@param なし
@return エラーコード
@author
-------------------------------------------------*/
TYPE_ERROR UART::Reset(void)
{
    fsp_err_t err = FSP_SUCCESS;

    //割り込み禁止
    DEFINE_INT();
    DISABLE_INT();
    //5回までリトライ
    for(uint8_t i=0;i<5;i++)
    {
        //クローズはエラー無視
        R_SCI_UART_Close(_pApiCtrl);

        //再開
        err = R_SCI_UART_Open(_pApiCtrl, _pCfg);
        if (FSP_SUCCESS == err)
        {
            break;
        }
    }
    //割り込み許可
    ENABLE_INT();
    //リセット成功
    if (FSP_SUCCESS == err)
    {
        _UARTError = UART_ERR_NONE;
        return E_OK;
    }

    //失敗
    return E_SYS;
}

/*!------------------------------------------------
@brief 初期化
@note  g_uart1_ctrl/p_cfg
@param なし
@return エラーコード
@author
-------------------------------------------------*/
TYPE_ERROR UART::Init(void)
{
	fsp_err_t err = FSP_SUCCESS;

	err = R_SCI_UART_Open(_pApiCtrl, _pCfg);
    if (FSP_SUCCESS != err)
    {
        return E_SYS;
    }
    return E_OK;
}

/*!------------------------------------------------
@brief オープン
@note  g_uart1_ctrl/p_cfg
@param BaudRate ボーレート
@param DataBits データビット長(UART_DATA_BITS_9/ UART_DATA_BITS_8/ UART_DATA_BITS_7)
@param Parity パリティ(UART_PARITY_OFF/ UART_PARITY_EVEN/ UART_PARITY_ODD)
@param Stop ストップビット(UART_STOP_BITS_1/ UART_STOP_BITS_2)
@return エラーコード
@author
-------------------------------------------------
TYPE_ERROR UART::Open(ENUM_UART_BAUDRATE BaudRate, uart_data_bits_t DataBits, uart_parity_t Parity, uart_stop_bits_t Stop)
{
    fsp_err_t err = FSP_SUCCESS;
    baud_setting_t baud_setting;

    //5000は許容誤差(%)を1000倍した値
    err = R_SCI_UART_BaudCalculate(BaudRate, false, 5000, &baud_setting);
    if (FSP_SUCCESS != err)
    {
        return E_PAR;
    }
    //ボーレートを設定
    err = R_SCI_UART_BaudSet(_pApiCtrl, (void *) &baud_setting);
    if (FSP_SUCCESS != err)
    {
        return E_PAR;
    }

    _pCfg->data_bits = DataBits;
    _pCfg->parity = Parity;
    _pCfg->stop_bits = Stop;

    err = R_SCI_UART_Open(_pApiCtrl, _pCfg);
    if (FSP_SUCCESS != err)
    {
        return E_SYS;
    }

    return E_OK;
}
*/
/*!------------------------------------------------
@brief 読出し
@note  
@param pData　読出しデータ格納先頭ポインタ
@param Len　読出しデータ長
@return 読出したデータ長
@author
-------------------------------------------------*/
uint32_t UART::Read(uint8_t *pData, uint32_t Len)
{
	uint32_t setSize = 0;
	
	//読出しデータ長0
    if(0 == Len)
    {
        return setSize;
    }

	//受信データなし
	if(0 == _rxBuffer.len)
	{
		return setSize;	
	}
	//割り込み禁止
	DEFINE_INT();
	DISABLE_INT();
	//読込み可能サイズ算出
	if( _rxBuffer.len < Len )
	{
	    Len = _rxBuffer.len;
	}
	for(uint32_t i=0;i<Len;i++)
	{
		*pData = *_rxBuffer.pR;
		_rxBuffer.len--;
		_rxBuffer.pR++;
		if(_rxBuffer.pR > _pRxEnd)
		{
			_rxBuffer.pR = _pRxTop;
		}
		pData++;
		setSize++;
	}
	//割り込み許可
	ENABLE_INT();
	return setSize;
}

/*!------------------------------------------------
@brief 書込み
@note  
@param pData　書込みデータ格納先頭ポインタ
@param Len　書込みデータ長
@return 書込みしたデータ長
@author
-------------------------------------------------*/
uint32_t UART::Write(uint8_t *pData, uint32_t Len)
{
	uint32_t setSize = 0;

	//データ長0
	if(0 == Len)
    {
        return setSize;
    }
	
	//バッファ満
	if(_txBuffer.len >= _txBufSize)
	{
		return setSize;
	}
	//割り込み禁止
	DEFINE_INT();
	DISABLE_INT();
	//書込み可能サイズ算出
    if((_txBuffer.len + Len) > _txBufSize)
    {
        Len = _txBufSize - _txBuffer.len;
    }
	for(uint32_t i=0;i<Len;i++)
	{
		*_txBuffer.pW = *pData;
		_txBuffer.pW++;
		_txBuffer.len++;
		if(_txBuffer.pW > _pTxEnd)
		{
			_txBuffer.pW = _pTxTop;
		}
		pData++;
		setSize++;
	}

	//送信中でないなら送信割り込みを発生させるため1バイト送信する
    if( false == _SendingFlag )
    {
        _SendingFlag = true;
        R_SCI_UART_Write (_pApiCtrl, _txBuffer.pR, 1);
        _txBuffer.pR++;
        _txBuffer.len--;
        if(_txBuffer.pR > _pTxEnd)
        {
            _txBuffer.pR = _pTxTop;
        }
    }
	//割り込み許可
    ENABLE_INT();
	return setSize;
}

/*!------------------------------------------------
@brief 受信データセット(受信割り込み内で呼び出される)
-------------------------------------------------*/
void UART::RxSet(uint8_t Data)
{
	//受信バッファ満
	if( _rxBuffer.len >= _rxBufSize )
	{
		return;
	}

	*_rxBuffer.pW = Data;
	_rxBuffer.pW++;
	_rxBuffer.len++;
	if(_rxBuffer.pW > _pRxEnd)
	{
		_rxBuffer.pW = _pRxTop;
	}
}

/*!------------------------------------------------
@brief 送信データゲット(送信割り込み内で呼び出される)
-------------------------------------------------*/
void UART::TxSet(void)
{
	if (_txBuffer.len > 0)
    {
    	R_SCI_UART_Write (_pApiCtrl, _txBuffer.pR, 1);
        _txBuffer.pR++;
        _txBuffer.len--;
    	if(_txBuffer.pR > _pTxEnd)
    	{
			_txBuffer.pR = _pTxTop;
		}
    }
	else
    {
	    //送信中フラグOFF
        _SendingFlag = false;
    }
}

/*!------------------------------------------------
@brief UARTエラーセット(受信割り込み内で呼び出される)
-------------------------------------------------*/
void UART::SetError(uart_event_t Event)
{
    switch(Event)
    {
        case UART_EVENT_ERR_PARITY:
            _UARTError = UART_ERR_PARITY;
            break;
        case UART_EVENT_ERR_FRAMING:
            _UARTError = UART_ERR_FRAMING;
            break;
        case UART_EVENT_ERR_OVERFLOW:
            _UARTError = UART_ERR_OVERFLOW;
            break;
        case UART_EVENT_BREAK_DETECT:
            _UARTError = UART_BREAK_DETECT;
            break;
        default:
            //エラー
            break;
    }
}

/*!------------------------------------------------
@brief UARTエラー取得
@note
@param なし
@return
@author
-------------------------------------------------*/
ENUM_UART_ERROR UART::GetError(void)
{
    return _UARTError;
}

///////////////////////////////////////////////////
/// 以下UART割り込みハンドラ定義(使用chanel毎に定義すること)
///////////////////////////////////////////////////
/*!------------------------------------------------
@brief UART割り込みハンドラ
@note   uart_event_t
@param なし
@return なし
@author
-------------------------------------------------*/
void uart_interrupt_callback(uart_callback_args_t *p_args)
{
    UART *p=NULL;
    //割り込み発生チャンネルによって制御対象を変更
    switch(p_args->channel)
    {
        case 0:
            //RS485
            p = G_pDrvUART_RS485;
            break;
        case 1:
            //スペアポート(将来的にはBLEポート)
            p = G_pDrvUART_BLE;
            break;
        case 2:
            //予備ポート(未使用)
            p = G_pDrvUART_Reserved;
            break;
        case 3:
            //QRリーダーポート
            p = G_pDrvUART_QRReader;
            break;
        default:
            return; //エラー
    }

    //発生イベント毎の処理
    switch(p_args->event)
    {
        case UART_EVENT_RX_COMPLETE:
            //受信完了(未使用)
            break;

        case UART_EVENT_RX_CHAR:
            //データ受信
            p->RxSet((uint8_t)p_args->data);
            break;

        case UART_EVENT_ERR_PARITY:
        case UART_EVENT_ERR_FRAMING:
        case UART_EVENT_ERR_OVERFLOW:
        case UART_EVENT_BREAK_DETECT:
            //エラー
            p->SetError(p_args->event);
            break;

        case UART_EVENT_TX_COMPLETE:
        case UART_EVENT_TX_DATA_EMPTY:
            //送信関連
            p->TxSet();
            break;

        default:
            //あり得ない状態
            break;
    }
}

/*!------------------------------------------------
@brief UART送信中フラグをゲット
@note
@param なし
@return bool
@author
-------------------------------------------------*/
bool UART::IsSending()
{
    bool buf = false;
    //割り込み禁止
    DEFINE_INT();
    DISABLE_INT();
    buf=_SendingFlag;
    //割り込み許可
    ENABLE_INT();
    return buf;
}

/*!------------------------------------------------
@brief 受信バッファをクリア
@note
@param なし
@return 無し
@author
-------------------------------------------------*/
void UART::ClearRxBuffer()
{
    //割り込み禁止
    DEFINE_INT();
    DISABLE_INT();
    //受信バッファをクリア
    memset(_pRxTop, 0, _rxBufSize);
    //書込み位置と読込位置を初期化
    _rxBuffer.pW = _rxBuffer.pR = _pRxTop;
    //格納データ長を初期化
    _rxBuffer.len = 0;
    //割り込み許可
    ENABLE_INT();
}

