/*!------------------------------------------------
@brief UARTクラスヘッダ
@note　
@author
-------------------------------------------------*/

#ifndef UART_H
#define UART_H

//リングバッファ書込み動作
enum ENUM_UART_BAUDRATE {
	UART_BAUDRATE_9600 = 9600,
	UART_BAUDRATE_19200 = 19200,
	UART_BAUDRATE_38400 = 38400,
	UART_BAUDRATE_57600 = 57600,
	UART_BAUDRATE_115200 = 115200,
};

//リングバッファ書込み動作
enum ENUM_UART_ERROR {
    UART_ERR_NONE = 0,
    UART_ERR_PARITY,
    UART_ERR_FRAMING,
    UART_ERR_OVERFLOW,
    UART_BREAK_DETECT,
};

//template <>
class UART {
	//受信管理
	struct RxBuffer{
		uint8_t *pBuf;
		uint8_t *pW;		//!< 書込み位置
		uint8_t *pR;		//!< 読込み位置
		uint32_t len;		//!< 格納データ長
	};
	
	//送信管理
	struct TxBuffer{
		uint8_t *pBuf;
		uint8_t *pW;		//!< 書込み位置
		uint8_t *pR;		//!< 読込み位置
		uint32_t len;		//!< 格納データ長
	};
	RxBuffer _rxBuffer;
	TxBuffer _txBuffer;
	uint32_t _rxBufSize;
	uint32_t _txBufSize;
	uart_ctrl_t *_pApiCtrl;
	uart_cfg_t *_pCfg;
	
	bool _SendingFlag;
	
	uint8_t* _pRxTop;       //!< 受信バッファ先頭ポインタ
	uint8_t* _pRxEnd;       //!< 受信バッファ最終ポイン
	uint8_t* _pTxTop;       //!< 送信バッファ先頭ポインタ
	uint8_t* _pTxEnd;       //!< 送信バッファ最終ポインタ
	
	ENUM_UART_ERROR _UARTError; //!< UARTエラーコード格納

public:
    /*!------------------------------------------------
    @brief コンストラクタ
    -------------------------------------------------*/
    UART(uart_ctrl_t *const pApiCtrl, uart_cfg_t *pCfg, uint32_t RxBufSize, uint32_t TxBufSize);

    /*!------------------------------------------------
    @brief デストラクタ
    -------------------------------------------------*/
    ~UART() = default;
    
    /*!------------------------------------------------
    @brief リセット
    -------------------------------------------------*/
    TYPE_ERROR Reset(void);

    /*!------------------------------------------------
	@brief 初期化
	-------------------------------------------------*/
    TYPE_ERROR Init(void);

	/*!------------------------------------------------
    @brief UARTオープン
    -------------------------------------------------
    TYPE_ERROR Open(ENUM_UART_BAUDRATE BaudRate, uart_data_bits_t DataBits, uart_parity_t Parity, uart_stop_bits_t Stop);
	 */
	/*!------------------------------------------------
	@brief 読出し
	-------------------------------------------------*/
	uint32_t Read(uint8_t *pData, uint32_t Len);
	
	/*!------------------------------------------------
	@brief 書込み
	-------------------------------------------------*/
	uint32_t Write(uint8_t *pData, uint32_t Len);
	
	/*!------------------------------------------------
	@brief 受信データセット(受信割り込み内で呼び出される)
	-------------------------------------------------*/
	void RxSet(uint8_t Data);
	
	/*!------------------------------------------------
	@brief 送信データゲット(送信割り込み内で呼び出される)
	-------------------------------------------------*/
	void TxSet(void);

	/*!------------------------------------------------
	@brief UARTエラーセット(受信割り込み内で呼び出される)
	-------------------------------------------------*/
    void SetError(uart_event_t Event);

	/*!------------------------------------------------
	@brief UARTエラー取得
	-------------------------------------------------*/
	ENUM_UART_ERROR GetError(void);

	/*!------------------------------------------------
    @brief 送信中フラグをゲット
    -------------------------------------------------*/
    bool IsSending(void);

    /*!------------------------------------------------
    @brief 受信バッファをクリア
    -------------------------------------------------*/
    void ClearRxBuffer(void);
};
#endif

