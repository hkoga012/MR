/*!------------------------------------------------
@brief QRリーダ受信タスク
@note
@author
-------------------------------------------------*/
#include "Task.h"

QrReader::QrReader()
	:BaseTask(ST_QR_MAX, EVT_QR_MAX, TIMER_QR_MAX, QR_EVENT_QUE_SIZE, _pStateString, _pEventString)
{
	//タイマー登録
	this->RegisterTimer(TIMER_FOREVER, EVT_QR_NOTICE_TIMER);
	
	//休止状態へ各ハンドラ登録
	State* st = this->GetState(ST_QR_DORMANT);
	st->AddEventHandler(EVT_QR_START, Dormant_Start);	//!< 開始

	st = this->GetState(ST_QR_INITIALIZING);
	st->AddEventHandler(EVT_QR_IDLE, Initializing_Idle); //!< Idle
	st->AddEventHandler(EVT_QR_NOTICE_TIMER, Initializing_NoticeTimer);  //!< タイマー通知

	//スキャン待ち状態へ各ハンドラ登録
	st = this->GetState(ST_QR_WAIT_SCAN);
	st->AddEventHandler(EVT_QR_IDLE, WaitScan_Idle);	//!< Idle
	st->AddEventHandler(EVT_QR_NOTICE_TIMER, WaitScan_NoticeTimer);	//!< タイマー通知

	//データ送信状態へ各ハンドラ登録
	st = this->GetState(ST_QR_SEND_DATA);
	st->AddEventHandler(EVT_QR_DETECT, SendData_Detect);    //!< 読取り検出

	//停止状態へ各ハンドラ登録
	st = this->GetState(ST_QR_STOP);
	st->AddEventHandler(EVT_QR_IDLE, Stop_Idle); //!< Idle

	_TimeOut = 0;       //!< 応答タイムアウトカウンタ
	memset(_SendData, 0x00, sizeof(_SendData));
	memset(_RecvData, 0x00, sizeof(_RecvData));
	_SendDataLength = 0;        //!< 送信データ長
	_RecvDataLength = 0;        //!< 受信データ長
	_InitCmdIndex = 0;          //!< 初期化コマンドインデックス
	_InitStatus = QR_INIT_NONE; //!< 初期化状態
	_CmdLen = 0;                //!< 送信コマンド文字列長
	_Retry = 0;                 //!< 初期化リトライ回数

}
/*!------------------------------------------------
@brief リトライオーバーチェック
@note　
@param なし
@return true:正常  false:リトライオーバー
@author
-------------------------------------------------*/
bool QrReader::IsRetryOver(void)
{
    _Retry++;
    if(_Retry >= QR_INIT_RETRY)
    {
        //リトライオーバー
        return false;
    }
    return true;
}

/*!------------------------------------------------
@brief リトライリセット
@note　リトライ回数をリセット
@param なし
@return なし
@author
-------------------------------------------------*/
void QrReader::RetryReset(void)
{
    _Retry = 0;
}

/*!------------------------------------------------
@brief UARTエラー検出リセット
@note　エラーを検出したらリセット(Close -> Openする)
        UARTのRead関数呼び出し前に呼び出すこと
@param なし
@author
-------------------------------------------------*/
void QrReader::UartCheckAndReset(void)
{
#ifdef WINDOWS_DEBUG
    //Windows動作時
#else
    UART *pUART = G_pDrvUART_QRReader;
    EVENT_COUNT evt = (EVENT_COUNT)0;
    ERRORFLAG flg = (ERRORFLAG)0;

#ifdef QR_ERROR_DEBUG_QR_OVER
    switch(UART_ERR_OVERFLOW)
#elif defined(QR_ERROR_DEBUG_QR_FER)
    switch(UART_ERR_FRAMING)
#else
    switch(pUART->GetError())
#endif /* QR_ERROR_DEBUG_QR_OVER/QR_ERROR_DEBUG_QR_FER */
    {
        case UART_ERR_NONE:
            return; //エラーなし、これ以上処理しない
            /* returnのためbreakなし */
        case UART_ERR_FRAMING:
            //フレーミングエラー
            flg = ERRORFLAG_QR_FERR;
            evt = EVENT_ERROR_QR_FER;
            break;
        case UART_ERR_OVERFLOW:
            //オーバーフロー
            flg = ERRORFLAG_QR_OVER;
            evt = EVENT_ERROR_QR_OVER;
            break;
        case UART_ERR_PARITY:
        case UART_BREAK_DETECT:
        default:
            /*　記録なし */
            break;
    }

    //記録対象の場合
    if( flg )
    {
        //エラー記録
        G_pDataManager->SetErrorFlag(ERRORFLAG_QR_OFFSET, flg);
        //回数記録
        G_pDataManager->AddEventCount(evt);
    }

    //UARTリセット
    if(E_OK != pUART->Reset())
    {
        //復旧不可
        THROW_FATAL_ERROR("QrReader", "Uart Reset Error");
    }
#endif
}

/*!------------------------------------------------
@brief シリアルポートのデータを読み捨て
@note　受信済みのデータを読み捨てる
@param なし
@return なし
@author
-------------------------------------------------*/
void QrReader::SciDummyRead(void)
{
    uint8_t Dummy[32];  //バッファサイズに特に意味はない
    while(SciRead(Dummy, sizeof(Dummy)) > 0){}
}
/*!------------------------------------------------
@brief シリアルポートからデータ読出し
@note　シリアルポートを容易に切り替えることを目的としたラッパーメソッド
@param pReadData 読出しデータ格納バッファ先頭ポインタ
@param ReadLength 読出しデータ長
@return 読出したデータ長
@author
-------------------------------------------------*/
uint32_t QrReader::SciRead(uint8_t *pReadData, uint8_t ReadLength)
{
#ifdef WINDOWS_DEBUG
    //Windows動作時
    return WinSerialPort::Read(COM_PORT3_QR, (char*)pReadData, (uint32_t)ReadLength);
#else
    //実機用
    return G_pDrvUART_QRReader->Read(pReadData, ReadLength);
#endif
}
/*!------------------------------------------------
@brief シリアルポートへデータ書込み
@note　　シリアルポートを容易に切り替えることを目的としたラッパーメソッド
@param pWriteData 書込みデータ格納バッファ先頭ポインタ
@param WriteLength 書込みデータ長
@return 書込んたデータ長
@author
-------------------------------------------------*/
uint32_t QrReader::SciWrite(uint8_t *pWriteData, uint8_t WriteLength)
{
#ifdef WINDOWS_DEBUG
    //Windows動作時
    WinSerialPort::Write(COM_PORT3_QR, (const uint8_t*)pWriteData, (uint32_t)WriteLength);
    return WriteLength;
#else
    //実機用
    return G_pDrvUART_QRReader->Write(pWriteData, WriteLength);
#endif
}

/*!------------------------------------------------
@brief 応答データ終端検出
@note　QRリーダーからの応答データの終端までをバッファに格納する
@param pRecvData 受信データ格納バッファ先頭ポインタ
@param pRecvDataLength 受信データ長格納変数先頭ポインタ
@return true:応答終端検出、false:応答終端未検出
@author
-------------------------------------------------*/
bool QrReader::IsDetectTermination(uint8_t *pRecvData, uint8_t *pRecvDataLength)
{
    while(SciRead(pRecvData, 1) > 0)
    {
        *pRecvDataLength += 1;  //コンパイラワーニング対策*pRecvDataLength++;とすると変数未使用のワーニングが出る
        if(*pRecvDataLength > 1)
        {
            if( _Suffix[1] == *pRecvData && _Suffix[0] == *(pRecvData-1))
            {
                //終端検出
                return true;
            }
        }
        pRecvData++;
    }
    return false;
}
/*!------------------------------------------------
@brief QRリーダー正常応答検出
@note　QRリーダーへコマンド送信後の応答が正常応答であるか確認する
@param pSendData 送信データ格納バッファ先頭ポインタ
@param pRecvData 受信データ格納バッファ先頭ポインタ
@param RecvDataLength 受信データ長
@param CmdLen コマンド文字列長
@return true:正常検出、false:異常応答検出
@author
-------------------------------------------------*/
bool QrReader::IsResOK(uint8_t *pSendData, uint8_t *pRecvData, uint8_t RecvDataLength, uint8_t CmdLen)
{
    if(RecvDataLength < (QR_PREFIX_LEN + CmdLen + 1))
    {
        //応答データ長不足(PrefixLen + コマンド文字列長 + ACK/NACK長)
        return false;
    }

    //送受信のコマンド文字列が一致するか確認
    //コマンド一致を確認後、ACK/NACKデータを確認し正常であるか確認する
    //送信データ 7E 01 30 30 30 30 XX XX XX XX XX XX @@ 3B 03
    //応答データ 02 01 30 30 30 30 XX XX XX XX XX XX @@ 3B 03
    //         ~~~~~~~~~~~~~~~~~ ~~~~~~~~~~~~~~~~~ ~~ ~~~~~
    //         Prefix(6)         cmd(n)            ack(1) Suffix(2)
    //※ XXはコマンド文字列(可変長)、@@はACK/NACK部(1バイト)
    if( 0 == memcmp( (pSendData + QR_PREFIX_LEN), (pRecvData + QR_PREFIX_LEN), CmdLen ) )
    {
        //ACK/NACK位置の値を確認
        if( *(pRecvData + (QR_PREFIX_LEN + CmdLen)) == 0x06 )
        {
            //正常応答
            return true;
        }
        else
        {
            //それ以外はエラー
            //0x15 or 0x05
        }
    }
    return false;
}
/*!------------------------------------------------
@brief QRリーダー送信データ作成
@note
@param pStrCmd QRリーダーコマンド文字列
@param pSendData 送信データ格納変数先頭ポインタ
@param SendBufSize 送信データ格納変数サイズ
@param pSendDataLength 送信データ長格納変数ポインタ
@param pCmdLen コマンド文字列長格納変数ポインタ
@return true:作成成功、false:作成失敗
@author
-------------------------------------------------*/
bool QrReader::CreateQRReaderData(uint8_t *pStrCmd, uint8_t *pSendData, uint8_t SendBufSize, uint8_t *pSendDataLength, uint8_t *pCmdLen)
{
    uint8_t cmdLen = (uint8_t)strlen((const char*)pStrCmd);
    if( SendBufSize < (QR_PREFIX_LEN + cmdLen + QR_SUFFIX_LEN) )
    {
        //受信バッファサイズを超えた
        return false;
    }
    //Prefix
    memcpy(pSendData, _SndPrefix, QR_PREFIX_LEN);
    pSendData += QR_PREFIX_LEN;

    //Command
    memcpy(pSendData, pStrCmd, cmdLen);
    pSendData += cmdLen;

    //Suffix
    memcpy(pSendData, _Suffix, QR_SUFFIX_LEN);

    //コマンドパケットデータ長
    *pSendDataLength = (QR_PREFIX_LEN + cmdLen + QR_SUFFIX_LEN);

    //コマンド文字列長
    *pCmdLen = cmdLen;

    return true;
}
/*!------------------------------------------------
@brief 上位送信データ作成
@note
@param pSendData 送信データ格納変数先頭ポインタ
@param pRecvData 受信データ格納変数先頭ポインタ
@param RecvDataLength 受信データ長
@param pSendDataLength 送信データ長格納変数ポインタ
@return なし
@author
-------------------------------------------------*/
void QrReader::CreateEventSenderData(uint8_t *pRecvData, uint8_t RecvDataLength, uint8_t* pSendData, uint8_t *pSendDataLength)
{
    uint8_t SendLen = (RecvDataLength >= QR_SEND_MAX_LEN) ? QR_SEND_MAX_LEN : (uint8_t)(RecvDataLength > 0 ? RecvDataLength - 1 : 0);  //RecvDataLength - 1は末尾のキャリッジリターンを除くため
    if( NULL == pSendData || NULL == pRecvData || 0 == SendLen)
    {
        *pSendDataLength = 0;
        return;
    }
    //未設定バッファは全て0x20で埋める
    memset(pSendData, 0x20, QR_SEND_BUFLEN);

    //先頭は識別コード
    *pSendData = 0x35;   //QRデータ
    pSendData++;

    //QR読取りデータをセット(キャリッジリターンを除く)
    memcpy(pSendData, pRecvData, SendLen);

    //0x20-0x7F以外は全て0x20に変換
    ClsCommon::ConvertSendData( pSendData, SendLen );

    //送信データ長算出
    *pSendDataLength = QR_SEND_MAX_LEN + 1; //+1は識別コードの長さ
}
/*!------------------------------------------------
@brief 停止状態 － 開始イベント処理
@note
@param pOwnTask 自タスクオブジェクトのポインタ
@param EventId イベントID
@param pData :
@param DataLen : 
@return なし
@author
-------------------------------------------------*/
void QrReader::Dormant_Start(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen)
{
    (void)EventId; (void)pData; (void)DataLen;  // 未使用引数のワーニング対策
    QrReader* pMyTask = (QrReader*)pOwnTask;
    pMyTask->StartTimer(EVT_QR_NOTICE_TIMER, 100);  // 100msec 周期タイマー開始
    pMyTask->_TimeOut = QR_INIT_TIMEOUT;            // QRリーダーからの応答タイムアウト時間
    pMyTask->_InitCmdIndex = 0;

    //初期化中に遷移
    pOwnTask->SetNextStateId(ST_QR_INITIALIZING);
    pMyTask->_InitStatus = QR_INIT_CMD_SEND; //コマンド送信
    pMyTask->RetryReset();
}

/*!------------------------------------------------
@brief 初期化中状態 － 待機処理
@note
@param pOwnTask 自タスクオブジェクトのポインタ
@param EventId イベントID
@param pData :
@param DataLen :
@return なし
@author
-------------------------------------------------*/
void QrReader::Initializing_Idle(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen)
{
    (void)EventId; (void)pData; (void)DataLen;  // 未使用引数のワーニング対策
    QrReader* pMyTask = (QrReader*)pOwnTask;
    uint32_t len = 0;   //送信データ長

    pMyTask->UartCheckAndReset();

    if( QR_INIT_CMD_SEND == pMyTask->_InitStatus )
    {
        ////////////////////////
        /// コマンド送信
        ////////////////////////
        //送信コマンドデータ作成
        bool res = pMyTask->CreateQRReaderData(
                (uint8_t *)pMyTask->_QrInitCmd[pMyTask->_InitCmdIndex],
                pMyTask->_SendData,
                QR_SEND_BUFLEN,
                &pMyTask->_SendDataLength,
                &pMyTask->_CmdLen);

        if( res )
        {
            //ClsCommon::PrintInfoNum(DebugCommand::SendPrintSci, " Send: ", pMyTask->_Retry);
            //コマンド送信
            len = pMyTask->SciWrite(pMyTask->_SendData, pMyTask->_SendDataLength);
            if(len == pMyTask->_SendDataLength)
            {
                //応答待ちへ遷移
                pMyTask->_InitStatus = QR_INIT_WAIT_RES;
                pMyTask->_TimeOut = QR_INIT_TIMEOUT;

                //受信関連初期化
                memset(pMyTask->_RecvData, 0x00, sizeof(pMyTask->_RecvData));
                pMyTask->_RecvDataLength = 0;
            }
        }

        //送信失敗
        if(QR_INIT_WAIT_RES != pMyTask->_InitStatus)
        {
            if (pMyTask->IsRetryOver() == false)
            {
                //エラー記録
                G_pDataManager->SetErrorFlag(ERRORFLAG_QR_OFFSET, ERRORFLAG_QR_INIT);
                //回数記録
                G_pDataManager->AddEventCount(EVENT_ERROR_QR_INIT);
                //リトライオーバーは停止状態へ遷移
                pMyTask->SetNextStateId(ST_QR_STOP);
            }
        }
    }
    else if( QR_INIT_WAIT_RES == pMyTask->_InitStatus )
    {
        ///////////////////////
        /// 応答待ち
        ///////////////////////
#ifdef QR_ERROR_DEBUG_QR_INIT
        pMyTask->_TimeOut = 0;
#endif
        if( 0 == pMyTask->_TimeOut )
        {
            //応答タイムアウト -> コマンド再送
            pMyTask->_InitStatus = QR_INIT_CMD_SEND;
            if (pMyTask->IsRetryOver() == false)
            {
                //エラー記録
                G_pDataManager->SetErrorFlag(ERRORFLAG_QR_OFFSET, ERRORFLAG_QR_INIT);
                //回数記録
                G_pDataManager->AddEventCount(EVENT_ERROR_QR_INIT);
                //リトライオーバーは停止状態へ遷移
                pMyTask->SetNextStateId(ST_QR_STOP);
            }
            return;
        }

        //Suffix検出待ち
        if( pMyTask->IsDetectTermination((pMyTask->_RecvData + pMyTask->_RecvDataLength), &pMyTask->_RecvDataLength) )
        {
            //受信バッファに格納されたデータの応答チェック確認(ACK/NACK)
            if( pMyTask->IsResOK(pMyTask->_SendData, pMyTask->_RecvData, pMyTask->_RecvDataLength, pMyTask->_CmdLen) )
            {
                //ClsCommon::PrintInfoNum(DebugCommand::SendPrintSci, " - CMDnum: ", pMyTask->_InitCmdIndex);
                //次のコマンド送信
                pMyTask->_InitCmdIndex++;
                pMyTask->_InitStatus = QR_INIT_CMD_SEND;
                //リトライ回数リセット
                pMyTask->RetryReset();

                //全コマンド送信完了
                if(pMyTask->_InitCmdIndex >= QR_INIT_CMD_NUM)
                {
                    //初期化完了をDataManagerへ通知
                    G_pDataManager->SendEvent(EVT_DATA_QR_STARTED);

                    //スキャン待ち状態へ遷移
                    pMyTask->SetNextStateId(ST_QR_WAIT_SCAN);

                    //スキャンタイムアウト設定
                    pMyTask->_TimeOut = QR_SCAN_TIMEOUT;

                    //受信データ初期化
                    pMyTask->_RecvDataLength = 0;
                    memset(pMyTask->_RecvData, 0x00, sizeof(pMyTask->_RecvData));
                }
            }
        }
    }
}
/*!------------------------------------------------
@brief 初期化中状態 － タイマー通知イベント処理
@note
@param pOwnTask 自タスクオブジェクトのポインタ
@param EventId イベントID
@param pData : 使用しない
@param DataLen : 使用しない
@return なし
@author
-------------------------------------------------*/
void QrReader::Initializing_NoticeTimer(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen)
{
    (void)EventId; (void)pData; (void)DataLen;  // 未使用引数のワーニング対策
    QrReader* pMyTask = (QrReader*)pOwnTask;
    if(pMyTask->_TimeOut > 0)
    {
        pMyTask->_TimeOut--;
    }
}

/*!------------------------------------------------
@brief スキャン待ち状態 － Idle処理
@note
@param pOwnTask 自タスクオブジェクトのポインタ
@param EventId イベントID
@param pData : 使用しない
@param DataLen : 使用しない
@return なし
@author
-------------------------------------------------*/
void QrReader::WaitScan_Idle(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen)
{
    (void)EventId; (void)pData; (void)DataLen;  // 未使用引数のワーニング対策
    QrReader* pMyTask = (QrReader*)pOwnTask;
    uint8_t* pRead = (pMyTask->_RecvData + pMyTask->_RecvDataLength);
    uint8_t  ReadVal = 0;
    pMyTask->UartCheckAndReset();
    //受信データ読込
    while(1)
    {
        //受信データを1バイト毎読出し
        if (0 == pMyTask->SciRead(&ReadVal, 1))
        {
            //データなし
            break;
        }

        //1文字でも検出したらタイムアウトリセット
        pMyTask->_TimeOut = QR_SCAN_TIMEOUT;

        pMyTask->_RecvDataLength++;

        //送信可能最大サイズまでバッファに格納
        if(pMyTask->_RecvDataLength <= QR_SEND_MAX_LEN)
        {
            *pRead = ReadVal;
            //読出しアドレス移動
            pRead++;
        }
        //キャリッジリターン検出したら上位へ送信
        if ('\r' == ReadVal )
        {
#ifdef QR_DEBUG_REPLACE_CHAR
            pMyTask->_RecvData[2] = 0x1F;
            pMyTask->_RecvData[6] = 0x7F;
#endif /* QR_DEBUG_REPLACE_CHAR */
            //送信状態へ遷移
            pMyTask->SetNextStateId(ST_QR_SEND_DATA);
            //読込検出イベント通知
            pMyTask->SendEvent(EVT_QR_DETECT, pMyTask->_RecvData, pMyTask->_RecvDataLength);
            break;
        }
    }

    //タイムアウト検出
    if( 0 == pMyTask->_TimeOut )
    {
        //未処理データを破棄
        pMyTask->SciDummyRead();
        pMyTask->_RecvDataLength = 0;
        pMyTask->_TimeOut = QR_SCAN_TIMEOUT;
    }
}

/*!------------------------------------------------
@brief スキャン待ち状態 － タイマー通知イベント処理
@note
@param pOwnTask 自タスクオブジェクトのポインタ
@param EventId イベントID
@param pData : 使用しない
@param DataLen : 使用しない
@return なし
@author
-------------------------------------------------*/
void QrReader::WaitScan_NoticeTimer(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen)
{
    (void)EventId; (void)pData; (void)DataLen;  // 未使用引数のワーニング対策
    QrReader* pMyTask = (QrReader*)pOwnTask;
    if(pMyTask->_TimeOut > 0)
    {
        pMyTask->_TimeOut--;
    }
}

/*!------------------------------------------------
@brief 送信状態 － スキャン検出通知イベント処理
@note
@param pOwnTask 自タスクオブジェクトのポインタ
@param EventId イベントID
@param pData : 使用しない
@param DataLen : 使用しない
@return なし
@author
-------------------------------------------------*/
void QrReader::SendData_Detect(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen)
{
    (void)EventId; (void)pData; (void)DataLen;  // 未使用引数のワーニング対策
    QrReader* pMyTask = (QrReader*)pOwnTask;
    uint8_t SendLen = 0;

    ///////////////////////
    /// 送信データ作成 -> 上位へ送信指示
    ///////////////////////
    pMyTask->CreateEventSenderData((uint8_t*)pData, (uint8_t)DataLen, pMyTask->_SendData, &SendLen);
    if( SendLen > 0 )
    {
        //上位へ送信(QR読出し間隔は3秒あるのでリングバッファは不要(上位へ送信に3秒もかからない為))
        G_pEventSender->SendEvent(EVT_EVTSND_REQ_READ_DATA, pMyTask->_SendData, SendLen);
#ifdef WINDOWS_DEBUG
        //Windows動作時
        uint8_t HexStr[3];
        G_pDebugCommand->SendPrintSci("----------(QR Send Data)----------\r\n");
        for (uint8_t i = 0; i < SendLen;i++)
        {
            ClsCommon::I2HexAsciiByte(pMyTask->_SendData[i], (char*)&HexStr[0], (char*)&HexStr[1]);
            HexStr[2] = '\0';
            G_pDebugCommand->SendPrintSci((const char*)HexStr);
        }
        G_pDebugCommand->SendPrintSci("\r\n");
#else
        G_pDebugCommand->SendPrintSciEx("----------(QR Send Data)----------\r\n");
        for (uint8_t i = 0; i < SendLen;i++)
        {
            G_pDebugCommand->SendPrintSciEx("%02X ", pMyTask->_SendData[i]);
        }
        G_pDebugCommand->SendPrintSciEx("\r\n");
#endif
        //残りデータを全て読み捨て
        pMyTask->SciDummyRead();

        //タイムアウトリセット
        pMyTask->_TimeOut = QR_SCAN_TIMEOUT;
        //受信データ長リセット
        pMyTask->_RecvDataLength = 0;
        memset(pMyTask->_RecvData, 0x00, QR_RECV_BUFLEN);
        //スキャン待ち状態へ遷移
        pMyTask->SetNextStateId(ST_QR_WAIT_SCAN);
    }
}

/*!------------------------------------------------
@brief 停止状態 － Idle通知イベント処理
@note
@param pOwnTask 自タスクオブジェクトのポインタ
@param EventId イベントID
@param pData : 使用しない
@param DataLen : 使用しない
@return なし
@author
-------------------------------------------------*/
void QrReader::Stop_Idle(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen)
{
    (void)pOwnTask; (void)EventId; (void)pData; (void)DataLen;  // 未使用引数のワーニング対策
    //リセット待ち 処理なし
}

/*!------------------------------------------------
@brief 強制停止指示
@note　動作を強制的に停止する
@param なし
@author
-------------------------------------------------*/
void QrReader::SetForcedStop(void)
{
    //タスクを停止状態へ遷移
    this->SetNextStateId(ST_QR_STOP);
}

/*!------------------------------------------------
@brief タスクの情報をデバッグコマンドでデバッグ出力する
@note
@return なし
@author
-------------------------------------------------*/
void QrReader::PrintInfo(void (*funcPrint)(const char*))
{
    (void)funcPrint;
}
