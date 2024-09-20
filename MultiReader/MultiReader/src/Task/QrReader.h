/*!------------------------------------------------
@brief QRリーダ受信タスクヘッダ
@note  
@author
-------------------------------------------------*/
#ifndef QR_READER_H
#define QR_READER_H
#include "../StateTransition/BaseTask.h"
//******************************************
// 状態は最低一つ登録する。起動直後は先頭に登録された状態(値=0)に遷移する
//*****************************************
//!< 状態定義
typedef enum {
    ST_QR_DORMANT = 0,		//!< 休止（必須）
	ST_QR_INITIALIZING,		//!< 初期化中
    ST_QR_WAIT_SCAN,		//!< QRスキャン待ち
    ST_QR_SEND_DATA,        //!< 上位へデータ送信
	ST_QR_STOP,				//!< 停止
    ST_QR_MAX,				//!< イベント種別の最大数(削除禁止)
}ENUM_STATE_QR;

constexpr uint32_t QR_INIT_TIMEOUT = 4;    //!< QR初期化タイムアウト時間(1 * 100msec = 100msec)
constexpr uint32_t QR_SCAN_TIMEOUT = 3;    //!< スキャンタイムアウト時間(3 * 100msec = 300msec)

//******************************************
// イベントを定義する際は必ずIdle(値=0)を定義すること
//*****************************************
//!< イベント定義
constexpr uint32_t QR_EVENT_QUE_SIZE = 10; //!< イベントバッファサイズ
typedef enum {
    EVT_QR_IDLE = 0,        //!< イベント無し(必須) ― イベントが無い（Idle状態の）時に実行するハンドラを登録する為に設けたダミーイベント
    EVT_QR_START,           //!< 開始

//《タスク外から通知されるイベント》
	EVT_QR_DETECT,			//!< QR読取り検出
//《タスク内部イベント》
    EVT_QR_NOTICE_TIMER,    //!< タイマー通知(使用しない場合は削除)

    EVT_QR_MAX,             //!< タスクイベント種別最大数(削除禁止)
}ENUM_EVT_QR;

//QRリーダー初期化状態
typedef enum {
    QR_INIT_NONE = 0,        //!< 応答なし
    QR_INIT_CMD_SEND,        //!< コマンド送信
    QR_INIT_WAIT_RES,        //!< コマンド応答待ち
    QR_INIT_MAX,              //!< 最大数(削除禁止)
}ENUM_QR_INIT;


constexpr uint32_t TIMER_QR_MAX = 1;	//!< タイマー登録数
constexpr uint8_t QR_SEND_BUFLEN = 64;   //!< 送信バッファサイズ
constexpr uint8_t QR_RECV_BUFLEN = 64;   //!< 受信バッファサイズ
constexpr uint8_t QR_SEND_MAX_LEN = 32;   //!< 上位送信最大サイズ
constexpr uint8_t QR_PREFIX_LEN = 6;   //!< Prefixサイズ
constexpr uint8_t QR_SUFFIX_LEN = 2;   //!< Suffixサイズ
constexpr uint8_t QR_INIT_CMD_NUM = 7;   //!< 初期化コマンド数
constexpr uint8_t QR_INIT_RETRY = 5;   //!< 初期化リトライ回数
/*!------------------------------------------------
@brief QRリーダ受信タスククラス
@note
@author
-------------------------------------------------*/
class QrReader : public BaseTask
{
    //デバッグ用状態文字定義
    //文字列配列_pStateString[]の文字列は必ずenumの状態定義順に合わせること
    const char* _pStateString[ST_QR_MAX] = {
        "DORMANT",	        //!< 休止
    	"INITIALIZING",		//!< 初期化中
        "SCAN_WAIT",	    //!< QRスキャン待ち
        "DATA_SEND",        //!< 上位へデータ送信
    	"STOP"				//!< 停止
    };

    //デバッグ用イベント文字定義
    //文字列配列_pEventString[]の文字列は必ずenumのイベント定義順に合わせること
    const char* _pEventString[EVT_QR_MAX] = {
        "IDLE",			    //!< イベント無し(必須)
        "START",			//!< 開始
    	"DETECT",			//!< QR読取り検出
        "NOTICE_TIMER",	    //!< タイマー通知(使用しない場合は削除)
    };
    uint8_t _TimeOut;       //!< 応答タイムアウトカウンタ
    uint8_t _SendData[QR_SEND_BUFLEN];       //!< 送信バッファ
    uint8_t _RecvData[QR_RECV_BUFLEN];       //!< 受信バッファ
    uint8_t _SendDataLength;        //!< 送信データ長
    uint8_t _RecvDataLength;        //!< 受信データ長
    uint8_t _InitCmdIndex;          //!< 初期化コマンドインデックス
    ENUM_QR_INIT _InitStatus;       //!< 初期化状態
    uint8_t _CmdLen;                //!< 送信コマンド文字列長
    uint8_t _Retry;                //!< 初期化リトライ回数
    const uint8_t _SndPrefix[QR_PREFIX_LEN] = {0x7E, 0x01, 0x30, 0x30, 0x30, 0x30}; //!<送信Prefix
    const uint8_t _ResPrefix[QR_PREFIX_LEN] = {0x02, 0x01, 0x30, 0x30, 0x30, 0x30}; //!<受信Prefix
    const uint8_t _Suffix[QR_SUFFIX_LEN] = {0x3B, 0x03};    //!<受送信共通 Suffix
    const uint8_t _QrInitCmd[QR_INIT_CMD_NUM][12] = {
                           "@FACDEF",       //!< Restore All Factory Defaults(※工場出荷は、リトライ送信したときに工場出荷のレスポンスがあります。最初はタイムアウトさせます)
                           "@RRDENA1",      //!< Reread Timeout(許可)
                           "@RRDDUR3000",   //!< Reread Timeout(時間): 同一データ再読み込み時間3秒
                           "@GRBENA0",      //!< Good Read Beep: ビープ音なし
                           "@GRLDUR320",    //!< Good Read LED Duration: 320ms点灯
                           "@SCNMOD2",      //!< Scan Mode: Sense Mode(工場出荷値ですが明示的に送信)
                           "@SENLVL5",      //!< Sensitivity: Sensitivity5
    };
    bool IsDetectTermination(uint8_t *pRecvData, uint8_t *pRecvDataLength);
    bool IsResOK(uint8_t *pSendData, uint8_t *pRecvData, uint8_t RecvDataLength, uint8_t CmdLen);
    bool CreateQRReaderData(uint8_t *pStrCmd, uint8_t *pSendData, uint8_t SendBufSize, uint8_t *pSendDataLength, uint8_t *pCmdLen);
    void CreateEventSenderData(uint8_t* pRecvData, uint8_t RecvDataLength, uint8_t* pSendData, uint8_t* pSendDataLength);
    void UartCheckAndReset(void);
    void SciDummyRead(void);        //シリアルポートデータ読み捨て関数
    uint32_t SciRead(uint8_t *pReadData, uint8_t ReadLength);       //シリアルポートデータ読込ラッパー関数
    uint32_t SciWrite(uint8_t *pWriteData, uint8_t WriteLength);    //シリアルポートデータ書込みラッパー関数
    bool IsRetryOver(void);      //リトライオーバーチェック
    void RetryReset(void);      //リトライ回数をリセット
public:

    /*----------------------------
    // 状態遷移ハンドラs
    // ハンドラ名は、基本的に以下で定義する。
    // 　Eventハンドラ：「状態_イベント」
    // 　Enterハンドラ：「状態_Enter」
    // 　Exitハンドラ：「状態_Exit」
    // 複数の状態やイベントで同じハンドラを使う場合は、それが判るような名前を付ける。
    // イベントが発生しても処理しない場合は、登録しない。
    -----------------------------*/

    // 停止 状態
    static void Dormant_Start(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen); // 開始

    // 初期化中 状態
    static void Initializing_Idle(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen);  // Idle
    static void Initializing_NoticeTimer(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen);  // タイマー通知

    // QRスキャン待ち 状態
    static void WaitScan_Idle(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen);  // Idle
    static void WaitScan_NoticeTimer(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen);  // タイマー通知

    // 上位へデータ送信 状態
    static void SendData_Detect(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen);  // 読取り検出通知

    // 停止中 状態
    static void Stop_Idle(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen);  // Idle

    /*----------------------------
    //コンストラクタ
    -----------------------------*/
    QrReader();
    virtual ~QrReader() {}

    /*!------------------------------------------------
    @brief タスク名
    -------------------------------------------------*/
    const char* GetTaskName() { return "QrReader"; }

    /*----------------------------
    // 強制停止指示
    -----------------------------*/
    void SetForcedStop();

    // タスクの情報をデバッグ出力する
    void PrintInfo(void (*funcPrint)(const char*));
};

#endif
