/*!------------------------------------------------
@brief NFCリーダ受信タスク
@note
@author
-------------------------------------------------*/
#include "Task.h"

NfcReader::NfcReader()
	:BaseTask(ST_NFC_MAX, EVT_NFC_MAX, TIMER_NFC_MAX, NFC_EVENT_QUE_SIZE, _pStateString, _pEventString)
{
	//タイマー登録
	this->RegisterTimer(TIMER_FOREVER, EVT_NFC_NOTICE_TIMER);
	
	//停止状態へ各ハンドラ登録
	State* st = this->GetState(ST_NFC_DORMANT);
	st->AddEventHandler(EVT_NFC_START, Dormant_Start);	//!< 開始
	st->AddEventHandler(EVT_NFC_DRVINIT_COMPLETE, Dormant_DriverInitComplete);    //!< NFCドライバ初期化完了通知
	st->AddEventHandler(EVT_NFC_NOTICE_TIMER, Dormant_NoticeTimer);    //!< タイマー通知

	//動作中状態へ各ハンドラ登録
	st = this->GetState(ST_NFC_RUNNING);
	st->AddEventHandler(EVT_NFC_IDLE, Running_Idle);	//!< Idle
	st->AddEventHandler(EVT_NFC_INITIAL_UPDATED, Running_Initial_Updated); //!< イニシャル設定更新通知
	st->AddEventHandler(EVT_NFC_DETECT_CARD, Running_Detect_Card);    //!< カード検出通知
	st->AddEventHandler(EVT_NFC_NOTICE_TIMER, Running_NoticeTimer);	//!< タイマー通知

	//停止中状態へ各ハンドラ登録
	st = this->GetState(ST_NFC_STOP);
	st->AddEventHandler(EVT_NFC_IDLE, Stop_Idle);    //!< Idle

#ifndef WINDOWS_DEBUG
    memset((uint8_t*)&_NfcData, 0x00, sizeof(_NfcData));
	_RemainingTime = NFC_INIT_TIMEOUT;
	memset(_SendData, 0x00, sizeof(_SendData));
#endif
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
void NfcReader::Dormant_Start(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen)
{
    (void)EventId; (void)pData; (void)DataLen;  // 未使用引数のワーニング対策
	NfcReader* pMyTask = (NfcReader*)pOwnTask;
	pMyTask->StartTimer(EVT_NFC_NOTICE_TIMER, 100);  // 100msec 周期タイマー開始

#ifndef WINDOWS_DEBUG
    pMyTask->_RemainingTime = NFC_INIT_TIMEOUT;   // 最後のカード通知からの経過時間

#ifdef DEBUG_NFC_POWER_DIPSW
    //アンテナ調整用動作
    uint8_t DipSw1Info = 0;            //!< DIPSW1(アンテナ最大電圧設定）
    uint8_t DipSw2Info = 0;            //!< DIPSW2(アンテナ最大電流設定）
    for(uint32_t swNo=1; swNo<=2; swNo++)
    {
        for(uint32_t pinNo=1; pinNo<=8; pinNo++)
        {
            bsp_io_port_pin_t portPin
            = (swNo==1)
                ? ((pinNo==1) ? bsp_io_port_pin_t::BSP_IO_PORT_00_PIN_00
                    : (pinNo==2) ? bsp_io_port_pin_t::BSP_IO_PORT_00_PIN_01
                    : (pinNo==3) ? bsp_io_port_pin_t::BSP_IO_PORT_00_PIN_02
                    : (pinNo==4) ? bsp_io_port_pin_t::BSP_IO_PORT_00_PIN_03
                    : (pinNo==5) ? bsp_io_port_pin_t::BSP_IO_PORT_00_PIN_04
                    : (pinNo==6) ? bsp_io_port_pin_t::BSP_IO_PORT_00_PIN_05
                    : (pinNo==7) ? bsp_io_port_pin_t::BSP_IO_PORT_00_PIN_06
                    : bsp_io_port_pin_t::BSP_IO_PORT_00_PIN_07 )
                : ((pinNo==1) ? bsp_io_port_pin_t::BSP_IO_PORT_04_PIN_00
                    : (pinNo==2) ? bsp_io_port_pin_t::BSP_IO_PORT_04_PIN_01
                    : (pinNo==3) ? bsp_io_port_pin_t::BSP_IO_PORT_04_PIN_02
                    : (pinNo==4) ? bsp_io_port_pin_t::BSP_IO_PORT_04_PIN_03
                    : (pinNo==5) ? bsp_io_port_pin_t::BSP_IO_PORT_04_PIN_04
                    : (pinNo==6) ? bsp_io_port_pin_t::BSP_IO_PORT_04_PIN_05
                    : (pinNo==7) ? bsp_io_port_pin_t::BSP_IO_PORT_04_PIN_06
                    : bsp_io_port_pin_t::BSP_IO_PORT_04_PIN_07 );

            bsp_io_level_t val = bsp_io_level_t::BSP_IO_LEVEL_HIGH;
            if(E_OK == G_pDrvIOPort->Read(portPin, &val))
            {
                if(val == bsp_io_level_t::BSP_IO_LEVEL_LOW)
                {
                    if(swNo==1){
                        DipSw1Info |= (uint8_t)(0x80 >> (pinNo-1));
                    }else{
                        DipSw2Info |= (uint8_t)(0x80 >> (pinNo-1));
                    }
                }
            }
            else
            {   // DIPSWが読めないのは、HWエラー
                //THROW_FATAL_ERROR(GetTaskName(), "DIPSW ReadError");
            }
        }
    }
    uint8_t Vol = NFC_OUTPUT_VOLTAGE_MAX;   //デフォルト値(5.7V)
    uint16_t Cur = NFC_OUTPUT_CURRENT_HIGH; //デフォルト値(306mA)
    //電圧はDIPSW値の1-42までを設定値とする
    if(DipSw1Info <= NFC_OUTPUT_VOLTAGE_MAX )
    {
        Vol = DipSw1Info;
    }
    //電流はDIPSW値の2倍を設定値とする
    uint16_t Val = (DipSw2Info*2);
    if(Val > 0 && Val <= NFC_OUTPUT_CURRENT_MAX )
    {
        Cur = Val;
    }

#else
    //アンテナ出力指示
    uint8_t Vol = NFC_OUTPUT_VOLTAGE;
    uint16_t Cur = (uint16_t)NFC_OUTPUT_CURRENT;
#endif /* DEBUG_NFC_RF_CONTINUOUS_OUTPUT */

    DebugCommand::SendPrintSci("NFCSetOutputPowerVoltage: ");
    ClsCommon::DebugPrintNum(DebugCommand::SendPrintSci, Vol);
    DebugCommand::SendPrintSci("\r\n");
    DebugCommand::SendPrintSci("NFCSetOutputPowerCurrent: ");
    ClsCommon::DebugPrintNum(DebugCommand::SendPrintSci, Cur);
    DebugCommand::SendPrintSci("\r\n");

    NFCSetOutputPowerVoltage( Vol );
    NFCSetOutputPowerCurrent( Cur );

    //DIPSW1設定がイニシャル設定の場合、Mifare,FeliCaの読出し情報を初期化(上位より読出しコマンド設定がされるまで読み出せない)
    if( SEL_FUNC_FELICA_INITIAL_COM == G_pDataManager->GetSelFuncFelica() )
    {
        NFCClearParameterFeliCa();
    }
    if( SEL_FUNC_MIFARE_INITIAL_COM == G_pDataManager->GetSelFuncMifare() )
    {
        NFCClearParameterMifare();
    }

    //DIPSW1読取り対象カード指定
    uint8_t Target = pMyTask->GetTargetCode( G_pDataManager->GetSelFuncMifare(),
                                             G_pDataManager->GetSelFuncFelica() );
    NFCSetTargetCard(Target);

	//NFCドライバ開始指示
	G_NFCInitStart = true;
#else
    G_pDataManager->SendEvent(EVT_DATA_NFC_STARTED);
    pMyTask->SetNextStateId(ST_NFC_RUNNING);
#endif
}

/*!------------------------------------------------
@brief 停止状態 － NFCドライバ初期化完了通知イベント処理
@note
@param pOwnTask 自タスクオブジェクトのポインタ
@param EventId イベントID
@param pData : 使用しない
@param DataLen : 使用しない
@return なし
@author
-------------------------------------------------*/
void NfcReader::Dormant_DriverInitComplete(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen)
{
    (void)EventId; (void)pData; (void)DataLen;  // 未使用引数のワーニング対策
    NfcReader* pMyTask = (NfcReader*)pOwnTask;
#ifndef WINDOWS_DEBUG

    uint16_t wFwVersion = NFCGetFwVersion();
    DebugCommand::SendPrintSci("NFC PN5190 FW Version: ");
    ClsCommon::DebugPrintNum(DebugCommand::SendPrintSci, wFwVersion>>8);
    DebugCommand::SendPrintSci(".");
    ClsCommon::DebugPrintNum(DebugCommand::SendPrintSci, wFwVersion&0xff);
    DebugCommand::SendPrintSci("\r\n");

    G_pDataManager->SendEvent(EVT_DATA_NFC_STARTED);
    //直ちに送信可能
    pMyTask->_RemainingTime = 0;
    pMyTask->SetNextStateId(ST_NFC_RUNNING);
#endif
}

/*!------------------------------------------------
@brief 停止状態 － タイマー通知イベント処理
@note 100msec周期
@param pOwnTask 自タスクオブジェクトのポインタ
@param EventId イベントID
@param pData : 使用しない
@param DataLen : 使用しない
@return なし
@author
-------------------------------------------------*/
void NfcReader::Dormant_NoticeTimer(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen)
{
    (void)EventId; (void)pData; (void)DataLen;  // 未使用引数のワーニング対策
    NfcReader* pMyTask = (NfcReader*)pOwnTask;
#ifndef WINDOWS_DEBUG
    if(pMyTask->_RemainingTime > 0)
    {
        pMyTask->_RemainingTime--;
        if( 0 == pMyTask->_RemainingTime)
        {
            //ドライバ初期化タイムアウト
            pMyTask->StopTimer(EVT_NFC_NOTICE_TIMER);
            //タスクを停止状態へ遷移
            pMyTask->SetNextStateId(ST_NFC_STOP);
        }
    }
#endif
}

/*!------------------------------------------------
@brief 動作中状態 － Idle処理
@note
@param pOwnTask 自タスクオブジェクトのポインタ
@param EventId イベントID
@param pData : 使用しない
@param DataLen : 使用しない
@return なし
@author
-------------------------------------------------*/
void NfcReader::Running_Idle(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen)
{
    (void)EventId; (void)pData; (void)DataLen;  // 未使用引数のワーニング対策
    (void)pOwnTask;
#ifndef WINDOWS_DEBUG
#endif
}

/*!------------------------------------------------
@brief 動作中状態 － タイマー通知イベント処理
@note 100msec周期
@param pOwnTask 自タスクオブジェクトのポインタ
@param EventId イベントID
@param pData : 使用しない
@param DataLen : 使用しない
@return なし
@author
-------------------------------------------------*/
void NfcReader::Running_NoticeTimer(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen)
{
    (void)EventId; (void)pData; (void)DataLen;  // 未使用引数のワーニング対策
#ifndef WINDOWS_DEBUG
    NfcReader* pMyTask = (NfcReader*)pOwnTask;
    if(pMyTask->_RemainingTime > 0)
    {
        pMyTask->_RemainingTime--;
    }
    else
    {
#ifdef DEBUG_NFC_POWER_DIPSW //　DIPSWでアンテナを調整時、ビープ音を鳴らす 
        G_pDrvPWM_Buzzer->SetDuty(0); // 定期的に止める
        G_pDrvPWM_LED_Frame_B->SetDuty(0);   // Frame LED ON
#endif
    }

#endif
}

/*!------------------------------------------------
@brief 動作中状態 － イニシャル設定更新通知イベント処理
@note
@param pOwnTask 自タスクオブジェクトのポインタ
@param EventId イベントID
@param pData : 使用しない
@param DataLen : 使用しない
@return なし
@author
-------------------------------------------------*/
void NfcReader::Running_Initial_Updated(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen)
{
    (void)EventId; (void)pData; (void)DataLen;  // 未使用引数のワーニング対策
    (void)pOwnTask;
    InitialInfo *pInf = (InitialInfo*)G_pDataManager->GetInitialInfo();   //イニシャル設定取得

#ifndef WINDOWS_DEBUG
#if 0 //デバッグ用ダミーデータ

    //Mifare
    pInf->Myfare1_Block = 8;
    pInf->Mifare1_KeyType = '0';
    memset(pInf->Mifare1_KeyData,0xFF,6);
    pInf->Myfare2_Block = 9;
    pInf->Mifare2_KeyType = '0';
    memset(pInf->Mifare2_KeyData,0xFF,6);

    //FeliCa
    pInf->Ferica_ValidBlock1 = 'V';
    pInf->Ferica_ValidBlock2 = 0;
    pInf->Ferica_SystemCd[0]=0xFE;
    pInf->Ferica_SystemCd[0]=0x00;
    pInf->Ferica_ServiceCd[0] = 0x4B;
    pInf->Ferica_ServiceCd[1] = 0x49;
    pInf->Ferica_Block1 = 1;
    pInf->Ferica_Block2 = 9;
#endif

    //Mifareパラメータ設定
    NFCSetParameterMifare(pInf->Myfare1_Block, pInf->Mifare1_KeyType, pInf->Mifare1_KeyData,
                            pInf->Myfare2_Block, pInf->Mifare2_KeyType, pInf->Mifare2_KeyData);
    //FeliCaパラメータ設定
    NFCSetParameterFeliCa(pInf->Ferica_ValidBlock1 ,pInf->Ferica_ValidBlock2,
                             pInf->Ferica_SystemCd, pInf->Ferica_ServiceCd,
                             pInf->Ferica_Block1 ,pInf->Ferica_Block2);
#endif
}
/*!------------------------------------------------
@brief 動作中状態 － カード検出通知イベント処理
@note
@param pOwnTask 自タスクオブジェクトのポインタ
@param EventId イベントID
@param pData : 使用しない
@param DataLen : 使用しない
@return なし
@author
-------------------------------------------------*/
void NfcReader::Running_Detect_Card(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen)
{
    (void)EventId; (void)DataLen;  // 未使用引数のワーニング対策
#ifndef WINDOWS_DEBUG
    NfcReader* pMyTask = (NfcReader*)pOwnTask;
    TYPE_NFC_DATA *p = (TYPE_NFC_DATA*)pData;

#ifdef NFC_DEBUG_PRINT_ENABLE  //検出カードデータをデバッグ用UARTへ出力  デバッグ用
    pMyTask->DebugPrintDetectCardData(p);
#endif

#ifdef DEBUG_NFC_POWER_DIPSW //　DIPSWでアンテナを調整時、ビープ音を鳴らす 
    G_pDrvPWM_Buzzer->SetDuty(50); // 検出時鳴らす
    G_pDrvPWM_LED_Frame_B->SetDuty(100);   // Frame LED OFF
#endif

#ifdef NFC_DEBUG_REPLACE_CHAR
    p->BlockData[2] = 0x1F;
    p->BlockData[6] = 0x7F;
#endif /* NFC_DEBUG_REPLACE_CHAR */
    G_pDebugCommand->SetDebugTime(2);
    //読取りデータとDIPSW1の状態を比較し、送信可能であれば送信する
    if( pMyTask->GetSendData(p, pMyTask->_SendData, NFC_SEND_BUF_SIZE) )
    {
        //以下の２つの条件のどちらかを満たせばカード情報を送信
        //前回送信時のUID(IDM)と今回通知のUID(IDM)が不一致
        //前回送信から送信停止時間以上経過している
        if((0 != memcmp(pMyTask->_NfcData.UidIdm, p->UidIdm, p->UidIdmLength))
             || (0 == pMyTask->_RemainingTime)
        )
        {
            G_pDebugCommand->SetDebugTime(3);
            //EventSenderに通知
            G_pEventSender->SendEvent(EVT_EVTSND_REQ_READ_DATA, pMyTask->_SendData, NFC_SEND_BUF_SIZE);

            memcpy((uint8_t *)&pMyTask->_NfcData, (uint8_t *)p, sizeof(TYPE_NFC_DATA));
            //送信停止時間リセット
            pMyTask->_RemainingTime = NFC_SEND_STOP_INTERVAL;

#ifdef NFC_DEBUG_PRINT_ENABLE       //上位送信データをデバッグ用UARTへ出力  デバッグ用
            pMyTask->DebugPrintSendData(pMyTask->_SendData, NFC_SEND_BUF_SIZE);
#endif
        }
    }
#endif
}

/*!------------------------------------------------
@brief 停止中状態 － Idle処理
@note
@param pOwnTask 自タスクオブジェクトのポインタ
@param EventId イベントID
@param pData : 使用しない
@param DataLen : 使用しない
@return なし
@author
-------------------------------------------------*/
void NfcReader::Stop_Idle(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen)
{
    (void)EventId; (void)pData; (void)DataLen;  // 未使用引数のワーニング対策
    (void)pOwnTask;
#ifndef WINDOWS_DEBUG
#endif
}

/*!------------------------------------------------
@brief 強制停止指示
@note　動作を強制的に停止する
@param なし
@author
-------------------------------------------------*/
void NfcReader::SetForcedStop(void)
{
#ifndef WINDOWS_DEBUG
    //タスクを停止状態へ遷移
    this->SetNextStateId(ST_NFC_STOP);

    G_NFCForceStop = true;
    //NFCドライバの外部割り込み停止
    R_ICU_ExternalIrqDisable(&g_external_irq1_ctrl);
#endif
}

/*!------------------------------------------------
@brief 検出カードタイプとDIPSW1設定から上位へ送信する識別コードを取得する
@note　動作を強制的に停止する
@param CardType　読み出したカードの受信データタイプ
@param FuncMifare　DIPSW1のMifare上位通知設定
@param FuncFeliCa　DIPSW1のFeliCa上位通知設定
@return 0:送信対象データ無し、0以外:送信識別コード
@author
-------------------------------------------------*/
#ifndef WINDOWS_DEBUG
uint8_t NfcReader::GetSCode(uint8_t CardType, ENUM_SEL_FUNC_MIFARE FuncMifare, ENUM_SEL_FUNC_FELICA FuncFeliCa)
{
    uint8_t SCode = 0;  //識別用コード格納変数
    //DIP SW設定に従って上位へ通知判定と送信情報を決定
    if(IS_MIFARE(CardType))
    {
        //Mifareを上位に送信可能か判定
        switch(FuncMifare)
        {
        case SEL_FUNC_MIFARE_READ:
            // 2ブロック分読み取り　breakしない
        case SEL_FUNC_MIFARE_INITIAL_COM:
            // イニシャルコマンドで指定した同一サービス内の 2 ブロック分読み取り（読めない場合、指定なしの場合は送信しない）
            SCode = ((IS_MIFARE_1K(CardType) || IS_MIFARE_4K(CardType)) ? NFC_SCODE_MIFARE : NFC_SCODE_NONE); break;
            break;
        default:
            //DIPSW設定不正、または読取り無し
            break;
        }
    }
    else if(IS_FELICA(CardType))
    {
        //FeliCaを上位に送信可能か判定
        switch(FuncFeliCa)
        {
        // FTSic(2,4 ブロック)、FTSicカード以外は無応答
        case SEL_FUNC_FELICA_FTSIC: SCode = (IS_FELICA_FTS(CardType) ? NFC_SCODE_FTSIC : NFC_SCODE_NONE); break;

        // サーパス向け(2,4 ブロック) 、サーパスカード以外は無応答
        case SEL_FUNC_FELICA_SURPASS: SCode = (IS_FELICA_SP(CardType) ? NFC_SCODE_SURPASS : NFC_SCODE_NONE); break;

        // IDm
        case SEL_FUNC_IDM: SCode = NFC_SCODE_IDM; break;

        // FTSic(2,4 ブロック) + IDm 、FTSicカード以外はIDmを応答
        case SEL_FUNC_FELICA_FTSIC_IDM: SCode = (IS_FELICA_FTS(CardType) ? NFC_SCODE_FTSIC : NFC_SCODE_IDM); break;

        // サーパス向け(2,4 ブロック) + IDm  、サーパスカード以外はIDmを応答
        case SEL_FUNC_FELICA_SURPASS_IDM: SCode = (IS_FELICA_SP(CardType) ? NFC_SCODE_SURPASS : NFC_SCODE_IDM); break;

        // イニシャルコマンドで指定した同一サービス内の 2 ブロック分読み取り（読めない場合、指定なしの場合は送信しない）
        //　読み込めた場合はFTSic or サーパス、読み込めなかった場合はその他として通知される
        case SEL_FUNC_FELICA_INITIAL_COM: SCode = ((IS_FELICA_FTS(CardType) || IS_FELICA_SP(CardType)) ? NFC_SCODE_FINITIAL : NFC_SCODE_NONE); break;

        case SEL_FUNC_FELICA_NO_READ:   //読込なし
        default:
            //DIPSW設定不正、または読取り無し
            SCode = NFC_SCODE_NONE;
            break;
        }
    }
    return SCode;
}
/*!------------------------------------------------
@brief DIPSW1設定から読取り対象コードを取得する
@note
@param FuncMifare　DIPSW1のMifare上位通知設定
@param FuncFeliCa　DIPSW1のFeliCa上位通知設定
@return 読取り対象コード
@author
-------------------------------------------------*/
uint8_t NfcReader::GetTargetCode(ENUM_SEL_FUNC_MIFARE FuncMifare, ENUM_SEL_FUNC_FELICA FuncFeliCa)
{
    uint8_t TargetCode = 0;  //ターゲットコード格納変数

    //Mifareを読取り対象判定
    switch(FuncMifare)
    {
    case SEL_FUNC_MIFARE_READ:
        // 2ブロック分読み取り　breakしない
    case SEL_FUNC_MIFARE_INITIAL_COM:
        // イニシャルコマンドで指定した同一サービス内の 2 ブロック分読み取り
        TargetCode = DEV_TYPE_MIFARE;
        break;
    default:
        //DIPSW設定不正、または読取り無し
        break;
    }
    //FeliCaを読取り対象判定
    switch(FuncFeliCa)
    {
    case SEL_FUNC_FELICA_FTSIC:
    case SEL_FUNC_FELICA_FTSIC_IDM:
    case SEL_FUNC_FELICA_INITIAL_COM:
        //FTSic(イニシャル設定指定もFTSicとして扱う)
        TargetCode |= DEV_TYPE_FELICA_FTS;
        break;
    case SEL_FUNC_FELICA_SURPASS:
    case SEL_FUNC_FELICA_SURPASS_IDM:
        // サーパス
        TargetCode |= DEV_TYPE_FELICA_SP;
        break;
    case SEL_FUNC_IDM:
        // IDm
        TargetCode |= DEV_TYPE_FELICA;
        break;
    case SEL_FUNC_FELICA_NO_READ:
        //読込なし
    default:
        //DIPSW設定不正、または読取り無し
        break;
    }
    return TargetCode;
}
#endif
/*!------------------------------------------------
@brief 送信データ作成
@note
@param pNfcData 読取りカードデータ格納構造体ポインタ
@param pSendData 送信データ格納先頭ポインタ
@param DataLen 送信データバッファ長
@return true:送信可能、false:送信不可
@author
-------------------------------------------------*/
#ifndef WINDOWS_DEBUG
bool NfcReader::GetSendData(TYPE_NFC_DATA *pNfcData, uint8_t *pSendData, uint8_t DataLen)
{
    uint8_t SCode = 0;  //識別用コード格納変数

    if(NULL == pNfcData || NULL == pSendData || pNfcData->NumBlocks > 2)
    {
        return false;
    }
    //DIPSW1設定に従って上位へ通知判定
    SCode = GetSCode(
                pNfcData->Type,
                G_pDataManager->GetSelFuncMifare(),
                G_pDataManager->GetSelFuncFelica());

    //上位へ通知対象であればバッファにセット
    if(NFC_SCODE_NONE != SCode)
    {
        memset(pSendData, 0x20, DataLen);
        *pSendData = SCode;
        pSendData++;

        if(SCode == NFC_SCODE_IDM)
        {
            //読取りブロック数が0ならIDmをセット
            //バイナリ->16進数文字に変換
            for(uint8_t i=0;i<pNfcData->UidIdmLength;i++)
            {
                //IDmの先頭データの上位4ビットは仕様により0とする
                uint8_t tempData = pNfcData->UidIdm[i] & (uint8_t)(i==0 ? 0x0F : 0xFF);
                ClsCommon::I2HexAsciiByte(tempData, (char*)(pSendData + i*2), (char*)(pSendData + i*2 + 1));
            }
        }
        else
        {
            //読取りブロックデータをセット
            if(pNfcData->NumBlocks > 0)
            {
                memcpy(pSendData, pNfcData->BlockData, pNfcData->NumBlocks * NFC_BLOCK_SIZE);
            }
            else
            {
                //読取りデータが無い(読取り失敗)
                return false;
            }
        }

        //0x20-0x7F以外は全て0x20に変換
        ClsCommon::ConvertSendData( pSendData, DataLen );
        return true;
    }
    return false;
}
#endif
/*!------------------------------------------------
@brief 検出カードデータデバッグ出力
@note
@param pNfcData 検出カードデータ格納先頭ポインタ
@return なし
@author
-------------------------------------------------*/
#ifndef WINDOWS_DEBUG
void NfcReader::DebugPrintDetectCardData(TYPE_NFC_DATA *pNfcData)
{
    G_pDebugCommand->SendPrintSciEx("\r\n----------NFC Card Detected----------\r\n");
    const char *pCardType = NULL;
    switch(pNfcData->Type)
    {
        case DEV_TYPE_MIFARE_1K:
            pCardType = "MIFARE 1K";
            break;
        case DEV_TYPE_MIFARE_4K:
            pCardType = "MIFARE 4K";
            break;
        case DEV_TYPE_MIFARE:
            pCardType = "MIFARE read failed";
            break;
        case DEV_TYPE_FELICA_FTS:
            pCardType = "FELICA FTSic";
            break;
        case DEV_TYPE_FELICA_SP:
            pCardType = "FELICA SURPASS";
            break;
        case DEV_TYPE_FELICA:
            pCardType = "FELICA read failed";
            break;
        default:
            pCardType = "UNKNOWN CARD";
            break;
    }
    G_pDebugCommand->SendPrintSciEx("Type     :%02X %s\r\n", pNfcData->Type, pCardType);
    G_pDebugCommand->SendPrintSciEx("UID/IDM  :");
    for(uint8_t i=0; i<pNfcData->UidIdmLength;i++)
    {
        G_pDebugCommand->SendPrintSciEx("%02X ",pNfcData->UidIdm[i]);
    }
    G_pDebugCommand->SendPrintSciEx("\r\n");
    G_pDebugCommand->SendPrintSciEx("BlockNum :%d\r\n", pNfcData->NumBlocks);
    G_pDebugCommand->SendPrintSciEx("BlockData:");
    for(uint8_t i=0; i<(pNfcData->NumBlocks * 16);i++)
    {
        G_pDebugCommand->SendPrintSciEx("%02X ",pNfcData->BlockData[i]);
    }
    G_pDebugCommand->SendPrintSciEx("\r\n");
}
#endif

/*!------------------------------------------------
@brief 送信データデバッグ出力
@note
@param pData 送信データ格納先頭ポインタ
@param DataLen 送信データバッファ長
@return なし
@author
-------------------------------------------------*/
#ifndef WINDOWS_DEBUG
void NfcReader::DebugPrintSendData(uint8_t *pData, uint8_t DataLen)
{
    G_pDebugCommand->SendPrintSciEx("\r\n##########(NFC Send Data)##########\r\n");
    const char *pSendType = NULL;
    switch(pData[0])
    {
        case NFC_SCODE_IDM:
            pSendType = "IDM";      //!< IDM
            break;
        case NFC_SCODE_FTSIC:
            pSendType = "FTSIC";    //!< FTSic
            break;
        case NFC_SCODE_SURPASS:
            pSendType = "SURPASS";  //!< SURPASS
            break;
        case NFC_SCODE_MIFARE:
            pSendType = "MIFARE";   //!< Mifare
            break;
        case NFC_SCODE_FINITIAL:
            pSendType = "FINITIAL"; //!< FINITIAL
            break;
        default:
            pSendType = "Unknown";
            break;
    }
    G_pDebugCommand->SendPrintSciEx("DataType:%s\r\n",pSendType);
    for(uint8_t i=0; i<DataLen;i++)
    {
        G_pDebugCommand->SendPrintSciEx("%02X ",pData[i]);
    }
    G_pDebugCommand->SendPrintSciEx("\r\n");
}
#endif


/*!------------------------------------------------
@brief タスクの情報をデバッグコマンドでデバッグ出力する
@note
@return なし
@author
-------------------------------------------------*/
void NfcReader::PrintInfo(void (*funcPrint)(const char*))
{
    (void)funcPrint;
}
