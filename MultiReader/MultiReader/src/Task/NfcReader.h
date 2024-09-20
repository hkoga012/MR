/*!------------------------------------------------
@brief NFCリーダ受信タスクヘッダ
@note  
@author
-------------------------------------------------*/
#ifndef NFC_READER_H
#define NFC_READER_H
#include "../StateTransition/BaseTask.h"
#ifndef WINDOWS_DEBUG
#include "./nfc/intfs/nfc_ctrl.h"
#endif

//******************************************
// 状態は最低一つ登録する。起動直後は先頭に登録された状態(値=0)に遷移する
//*****************************************
//!< 状態定義
typedef enum {
    ST_NFC_DORMANT = 0,		//!< 休止（必須）
    ST_NFC_RUNNING,			//!< 動作中
    ST_NFC_STOP,            //!< 停止
    ST_NFC_MAX,				//!< イベント種別の最大数(削除禁止)
}ENUM_STATE_NFC;

//******************************************
// イベントを定義する際は必ずIdle(値=0)を定義すること
//*****************************************
//!< イベント定義
constexpr uint32_t NFC_EVENT_QUE_SIZE = 10; //!< イベントバッファサイズ
typedef enum {
    EVT_NFC_IDLE = 0,       //!< イベント無し(必須) ― イベントが無い（Idle状態の）時に実行するハンドラを登録する為に設けたダミーイベント

//《タスク外から通知されるイベント》
    EVT_NFC_START,          //!< 開始
    EVT_NFC_INITIAL_UPDATED,//!< イニシャル設定更新通知
    EVT_NFC_DRVINIT_COMPLETE,  //!< NFCドライバ初期化完了通知
    EVT_NFC_DETECT_CARD,    //!< カード検出通知

    //《タスク内部イベント》
    EVT_NFC_NOTICE_TIMER,   //!< タイマー通知(使用しない場合は削除)

    EVT_NFC_MAX,            //!< タスクイベント種別最大数(削除禁止)
}ENUM_EVT_NFC;

//上位送信データ識別コード
enum {
    NFC_SCODE_NONE = 0,         //!< 識別コード無し
    NFC_SCODE_IDM = 0x30,       //!< IDM
    NFC_SCODE_FTSIC = 0x32,     //!< FTSic
    NFC_SCODE_SURPASS = 0x33,   //!< サーパス
    NFC_SCODE_MIFARE = 0x34,    //!< Mifare
    //NFC_SCODE_QR = 0x35,        //!< QRコード NFCで使用しないためコメント
    NFC_SCODE_FINITIAL = 0x36,  //!< イニシャル設定読出しデータ
};
constexpr uint32_t TIMER_NFC_MAX = 1;	//!< タイマー登録数
constexpr uint32_t NFC_SEND_STOP_INTERVAL = 30;  //!< 同一カード送信停止間隔(30 * 100msec = 3sec)
constexpr uint32_t NFC_INIT_TIMEOUT = 30;    //!< NFC初期化タイムアウト時間(30 * 100msec = 3sec)
constexpr uint32_t NFC_SEND_BUF_SIZE = 33;   //!< NFC送信バッファサイズ(識別用コード(1)+2Block(32) = 33)
constexpr uint32_t NFC_BLOCK_SIZE = 16;      //!< NFCブロックサイズ
extern bool G_NFCInitStart;                  //!< NFC初期化指示フラグ(main関数で参照)
extern bool G_NFCForceStop;                  //!< NFC強制停止指示フラグ(main関数で参照)
/*!------------------------------------------------
@brief NFCリーダ受信タスククラス
@note
@author
-------------------------------------------------*/
class NfcReader : public BaseTask
{
private:
    //デバッグ用状態文字定義
    //文字列配列_pStateString[]の文字列は必ずenumの状態定義順に合わせること
    const char* _pStateString[ST_NFC_MAX] = {
        "DORMANT",	        //!< 休止
        "RUNNUNG",	        //!< 動作中
        "STOP",             //!< 停止中
    };

    //デバッグ用イベント文字定義
    //文字列配列_pEventString[]の文字列は必ずenumのイベント定義順に合わせること
    const char* _pEventString[EVT_NFC_MAX] = {
        "IDLE",			    //!< イベント無し(必須)
        "START",			//!< 開始
        "INITIAL_UPDATED",  //!< イニシャル設定更新通知
        "DRVINIT_COMPLETE", //!< NFCドライバ初期化完了通知
        "DETECT_CARD",      //!< カード検出通知
        "NOTICE_TIMER",	    //!< タイマー通知(使用しない場合は削除)
    };

#ifndef WINDOWS_DEBUG
    TYPE_NFC_DATA _NfcData; //NFC受信データ格納(Mifare/FeliCa共通)
    uint32_t _RemainingTime;  //最後のカード通知からタイムアウトまでの残り時間(100msec単位 値：1 = 100msec)
    uint8_t  _SendData[NFC_SEND_BUF_SIZE];  //送信データ格納バッファ
    uint8_t GetSCode(uint8_t CardType, ENUM_SEL_FUNC_MIFARE FuncMifare, ENUM_SEL_FUNC_FELICA FuncFeliCa);
    uint8_t GetTargetCode(ENUM_SEL_FUNC_MIFARE FuncMifare, ENUM_SEL_FUNC_FELICA FuncFeliCa);
    bool GetSendData(TYPE_NFC_DATA *pNfcData, uint8_t *pSendData, uint8_t DataLen);
    void DebugPrintSendData(uint8_t *pData, uint8_t DataLen);
    void DebugPrintDetectCardData(TYPE_NFC_DATA *pNfcData);
#endif
public:

    /*----------------------------
    // 状態遷移ハンドラ
    // ハンドラ名は、基本的に以下で定義する。
    // 　Eventハンドラ：「状態_イベント」
    // 　Enterハンドラ：「状態_Enter」
    // 　Exitハンドラ：「状態_Exit」
    // 複数の状態やイベントで同じハンドラを使う場合は、それが判るような名前を付ける。
    // イベントが発生しても処理しない場合は、登録しない。
    -----------------------------*/

    // 休止 状態
    static void Dormant_Start(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen); // 開始
    static void Dormant_DriverInitComplete(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen);  // NFCドライバ初期化完了通知
    static void Dormant_NoticeTimer(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen); // タイマー通知
    // 動作中 状態
    static void Running_Idle(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen);  // Idle
    static void Running_NoticeTimer(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen);  // タイマー通知
    static void Running_Initial_Updated(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen); // イニシャル設定更新通知
    static void Running_Detect_Card(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen); // カード検出通知
    // 停止中 状態
    static void Stop_Idle(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen);  // Idle

    /*----------------------------
    //コンストラクタ
    -----------------------------*/
    NfcReader();
    virtual ~NfcReader() {}

    /*!------------------------------------------------
    @brief タスク名
    -------------------------------------------------*/
    const char* GetTaskName() { return "NfcReader"; }

    /*----------------------------
    // 強制停止指示
    -----------------------------*/
     void SetForcedStop(void);

     // タスクの情報をデバッグ出力する
     void PrintInfo(void (*funcPrint)(const char*));
};

#endif
