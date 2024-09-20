/*!------------------------------------------------
@brief データ管理タスクヘッダ
@note  
@author
-------------------------------------------------*/
#ifndef DATA_MANAGER_H
#define DATA_MANAGER_H
#include "../StateTransition/BaseTask.h"
#include "../version.h"

#ifdef WINDOWS_DEBUG
#include <thread>
#endif

//******************************************
// 状態は最低一つ登録する。起動直後は先頭に登録された状態(値=0)に遷移する
//*****************************************
//!< 状態定義
typedef enum {
    ST_DATA_DORMANT = 0,		    //!< 停止（必須）
    ST_DATA_WAIT_READER_STARTED,    //!< リーダ起動待ち（NFC、QR 両方待ち）
    ST_DATA_WAIT_RESETDATA_SENDED,  //!< リセットデータ送信待ち
    ST_DATA_RUNNING,			    //!< 動作中
    ST_DATA_STOP,			        //!< 停止中（FW更新タスク動作中）
    ST_DATA_MAX,				    //!< イベント種別の最大数(削除禁止)
}ENUM_STATE_DATA;

//******************************************
// イベントを定義する際は必ずIdle(値=0)を定義すること
//*****************************************
//!< イベント定義
constexpr uint32_t DATA_EVENT_QUE_SIZE = 10; //!< イベントバッファサイズ
typedef enum {
    EVT_DATA_IDLE = 0,          //!< イベント無し(必須) ― イベントが無い（Idle状態の）時に実行するハンドラを登録する為に設けたダミーイベント
    EVT_DATA_START,			    //!< 開始

//《タスク外から通知されるイベント》
    EVT_DATA_QR_STARTED,        //!< QRリーダ 起動完了通知
    EVT_DATA_NFC_STARTED,       //!< NFCリーダ 起動完了通知
    EVT_DATA_RESETDATA_SENDED,  //!< リセットデータ送信完了通知
    EVT_DATA_LED,             	//!< LED更新指示	パラメータ4バイト([0:Left、1:Center、2:Right、3:Frame]: 点灯指示文字 '0', 'T'-'X' それ以外は無視
    EVT_DATA_SET_INITIAL,       //!< イニシャル設定    上位から渡されるパケットのデータ部をそのまま渡す
    EVT_DATA_CLS_CUMULATIVE,    //!< 累積回数クリア

//《タスク内部イベント》
    EVT_DATA_TMOUT_WAIT_READER_STARTED, //!< リーダ起動待ちタイムアウト（TIMER）
    EVT_DATA_TMOUT_WAIT_BOOT,           //!< 装置起動待ちタイムアウト（TIMER）
    EVT_DATA_TM_SHORTEST_TIME_TO_START, //!< 最短起動時間経過通知（TIMER）
    EVT_DATA_TM_INTERVAL_ROM_UPDATE,    //!< ROM更新インターバル通知（TIMER）

    EVT_DATA_MAX,               //!< テストタスクイベント種別最大数(削除禁止)
}ENUM_EVT_DATA;

constexpr uint32_t TIMER_DATA_MAX = 4;	//!< タイマー登録数


//*****************************************
// FeliCa 読み取り設定
//*****************************************
typedef enum {
    SEL_FUNC_FELICA_NO_READ = 0,    //!< 読み取り無し（設定無効も含む）
    SEL_FUNC_FELICA_FTSIC,          //!< FTSic(2,4 ブロック)、FTSicカード以外は無応答
    SEL_FUNC_FELICA_SURPASS,        //!< サーパス向け(2,4 ブロック) 、サーパスカード以外は無応答
    SEL_FUNC_IDM,                   //!< IDm
    SEL_FUNC_FELICA_FTSIC_IDM,      //!< FTSic(2,4 ブロック) + IDm 、FTSicカード以外はIDmを応答
    SEL_FUNC_FELICA_SURPASS_IDM,    //!< サーパス向け(2,4 ブロック) + IDm  、サーパスカード以外はIDmを応答
    SEL_FUNC_FELICA_INITIAL_COM,    //!< イニシャルコマンドで指定した同一サービス内の 2 ブロック分読み取り（読めない場合、指定なしの場合は送信しない）
    SEL_FUNC_FELICA_MAX,	        //!< イベント種別の最大数(削除禁止)
}ENUM_SEL_FUNC_FELICA;


//*****************************************
// Mifare 読み取り設定
//*****************************************
typedef enum {
    SEL_FUNC_MIFARE_NO_READ = 0,    //!< 読み取り無し（設定無効も含む）
    SEL_FUNC_MIFARE_READ,           //!< 2ブロック分読み取り
    SEL_FUNC_MIFARE_INITIAL_COM,    //!< イニシャルコマンドで指定した同一サービス内の 2 ブロック分読み取り（読めない場合、指定なしの場合は送信しない）
    SEL_FUNC_MIFARE_MAX,	        //!< イベント種別の最大数(削除禁止)
}ENUM_SEL_FUNC_MIFARE;


//*****************************************
// イベントフラグ・累積回数 メモリマップ
//*****************************************
constexpr uint32_t SZ_ROM_MEM_BUF_SIZE = 160;	//!< イベントフラグ・累積回数 メモリ データサイズ

// === 各ブロックの位置 メモリマップ ===
#define EEPROM_MAP_OUTPUT_ERROR_OFFSET  0x0010
#define EEPROM_MAP_COUNTER_OFFSET       0x0020

// === イベントフラグ（出力エラー）情報 メモリマップ ===
typedef enum : uint16_t {
    ERRORFLAG_PORT_485_OFFSET = 0       //! RS485制御ポート異常
    , ERRORFLAG_PORT_LED_OFFSET1 = 1    //! 【未使用】LEDポート出力異常1（未使用）
    , ERRORFLAG_PORT_LED_OFFSET2 = 2    //! 【未使用】LEDポート出力異常2（未使用）
    , ERRORFLAG_PORT_RELAY_OFFSET = 3   //! リレー出力異常
    , ERRORFLAG_PORT_RW_OFFSET = 4      //! ICリーダ リセットポート出力異常
    , ERRORFLAG_HOST_PRI_OFFSET = 8     //! RS485 上位プライマリ回線通信エラーイベント
    , ERRORFLAG_RW_OFFSET = 9           //! ICリーダ 通信回線通信エラーイベント
    , ERRORFLAG_QR_OFFSET = 10          //! QRリーダ 回線通信エラーイベント
} ERRORFLAG_OFFSET;

typedef enum : uint8_t {
    ERRORFLAG_PORT_485_DE_A = 0x80      //! RS485 Primary DEポート出力異常
    , ERRORFLAG_PORT_485_RE_A = 0x40    //! RS485 Primary REポート出力異常
    , ERRORFLAG_PORT_485_FET = 0x20     //! RS485 FETポート出力異常

    , ERRORFLAG_PORT_RELAY1 = 0x80      //! PB0 リレー出力１ポート出力異常

    , ERRORFLAG_PORT_RW_RESET = 0x80    //! ICリーダ リセットポート出力異常

    , ERRORFLAG_HOST_OVER = 0x80        //! RS485 上位通信受信オーバーランエラー
    , ERRORFLAG_HOST_PERR = 0x40        //! 【未使用】上位通信受信パリティエラー(未使用：パリティ無し)
    , ERRORFLAG_HOST_FERR = 0x20        //! RS485 上位通信受信フレーミングエラー
    , ERRORFLAG_HOST_COLLISION = 0x10   //! RS485 上位通信衝突リトライオーバーエラー
    , ERRORFLAG_HOST_TIMEOUT = 0x08     //! 【未使用】上位通信送信タイムアウトエラー

    , ERRORFLAG_RW_INIT = 0x80          //! ICリーダ 通信初期化エラー
    , ERRORFLAG_RW_OVER = 0x40          //! 【未使用】R/W 通信受信オーバーランエラー
    , ERRORFLAG_RW_PERR = 0x20          //! 【未使用】R/W 通信受信パリティエラー(未使用：パリティ無し)
    , ERRORFLAG_RW_FERR = 0x10          //! 【未使用】R/W 通信受信フレーミングエラー
    , ERRORFLAG_RW_RETRYOVER = 0x08     //! 【未使用】R/W 通信リトライオーバーエラー

    , ERRORFLAG_QR_INIT = 0x80          //! QRリーダ 通信初期化エラー
    , ERRORFLAG_QR_OVER = 0x40          //! QRリーダ 通信受信オーバーランエラー
    , ERRORFLAG_QR_PERR = 0x20          //! 【未使用】QRリーダ 通信受信パリティエラー(未使用：パリティ無し)
    , ERRORFLAG_QR_FERR = 0x10          //! QRリーダ 通信受信フレーミングエラー
} ERRORFLAG;

// === 累積回数メモリマップ ===
typedef enum {  // 空きは予備、１つの項目は2byteで構成される
    EVENT_RESET = 0            	        //  0:本装置起動回数
    , EVENT_SENDRESET           	    //  1:リセットデータ 正常送信回数
    , EVENT_RSVSTATUS           	    //  2:ステータス要求 正常受信回数
    , EVENT_RSVCONTROL          	    //  3:指示コマンド 正常受信回数
    , EVENT_RELAYOPEN           	    //  4:メイク信号出力指示 正常出力回数
    , EVENT_RELAYCLOSE          	    //  5:ブレイク信号出力指示 正常出力回数
    , EVENT_SENDCARD            	    //  6:ICリーダ 読み取りデータ 正常送信回数
    , EVENT_SENDQR                 	    //  7:QRリーダ 読み取りデータ 正常送信回数
    , EVENT_SENDLIDOPEN         	    //  8:【未使用】蓋開データ送信回数
    , EVENT_SENDLIDCLOSE        	    //  9:【未使用】蓋開データ送信回数
    , EVENT_COLLISION           	    // 10:送信データ衝突回数
    , EVENT_SENDTIMEOUT           	    // 11:送信タイム衝突リトライオーバの回数
    , EVENT_RSVINITIAL                  // 12:イニシャルコマンド受信回数

    , EVENT_ERROR_HOST_PRI_OVER = 16    // 16:RS485 上位システムとの通信エラー オーバーラン検出 回数
    , EVENT_ERROR_HOST_PRI_PER          // 17:【未使用】RS485 上位システムとの通信エラー パリティエラー検出回数（パリティなし）
    , EVENT_ERROR_HOST_PRI_FER          // 18:RS485 上位システムとの通信エラー上位プライマリ回線通信フレーミングエラー検出回数

    , EVENT_ERROR_QR_INIT = 24          // 24:QRリーダー初期化異常検出(タイムアウト)
    , EVENT_ERROR_QR_OVER               // 25:QRリーダー通信オーバーラン検出回数
    , EVENT_ERROR_QR_PER                // 26:【未使用】QRリーダー通信パリティエラー検出回数
    , EVENT_ERROR_QR_FER                // 27:QRリーダー通信フレーミングエラー検出回数

    , EVENT_ERROR_RW_INIT = 32          // 32:ICリーダ通信 エラー回数(Felica 初期設定中)
    , EVENT_ERROR_RW_POL                // 33:ICリーダ通信 エラー回数(Felica ポーリング中:カード捕捉)
    , EVENT_ERROR_RW_SELECT             // 34:ICリーダ通信 エラー回数(Felica カードセレクト中)
    , EVENT_ERROR_RW_POL2               // 35:ICリーダ通信 エラー回数(Felica ポーリング中:共通領域)
    , EVENT_ERROR_RW_DATA               // 36:ICリーダ通信 エラー回数(Felica ブロック２リード中)
    , EVENT_ERROR_RW_DATA4              // 37:【未使用】ICリーダ通信 エラー回数(Felica ブロック４リード中)
    , EVENT_ERROR_RW_RELEASE            // 38:ICリーダ通信 エラー回数(Felica カードリリース)
    , EVENT_ERROR_RW_POL3               // 39:ICリーダ通信 エラー回数(Felica ポーリング中:カードリリース)
    , EVENT_ERROR_RW_M_POL              // 40:【未使用】ICリーダ通信 エラー回数(MIFARE ポーリング中:カード捕捉)
    , EVENT_ERROR_RW_M_AUTH1            // 41:ICリーダ通信 エラー回数(MIFARE 1ブロック目認証)
    , EVENT_ERROR_RW_M_AUTH2            // 42:ICリーダ通信 エラー回数(MIFARE 2ブロック目認証)
    , EVENT_ERROR_RW_M_READ1            // 43:ICリーダ通信 エラー回数(MIFARE 1ブロック目リード)
    , EVENT_ERROR_RW_M_READ2            // 44:ICリーダ通信 エラー回数(MIFARE 2ブロック目リード)

    , EVENT_ERROR_HOST_NO_COM = 48      // 48:上位から指示コマンドなし

    , EVENT_DOWNLOAD_ERASE = 62         // 62:ダウンロード開始（消去）受信回数
    , EVENT_DOWNLOAD_UPDATE             // 63:ダウンロード更受信新回数
    , EVENT_COUNT_NUM = 64      	    // 64:累計回数の個数
} EVENT_COUNT;

constexpr uint8_t LEN_INI_MIFARE_KEY = 6;       //! イニシャル設定 読取位置キーデータ
constexpr uint8_t LEN_INI_FERICA_SYSTEM = 2;    //! イニシャル設定 システムコード
constexpr uint8_t LEN_INI_FERICA_SERVICE = 2;   //! イニシャル設定 サービスコード

constexpr uint8_t POS_MIFARE1_KEY_TYPE = 0;     //!< MIFARE 1ブロック目 キータイプ 位置
constexpr uint8_t POS_MIFARE1_KEY_DATA = 1;     //!< MIFARE 1ブロック目 キーデータ 位置
constexpr uint8_t POS_MIFARE1_BLOCK = 13;       //!< MIFARE 1ブロック目 ブロック番号 位置
constexpr uint8_t POS_MIFARE2_KEY_TYPE = 15;    //!< MIFARE 2ブロック目 キータイプ 位置
constexpr uint8_t POS_MIFARE2_KEY_DATA = 16;    //!< MIFARE 2ブロック目 キーデータ 位置
constexpr uint8_t POS_MIFARE2_BLOCK = 28;       //!< MIFARE 2ブロック目 ブロック番号 位置
constexpr uint8_t POS_FERICA_SYSTEM_CD = 30;    //!< FERICA システムコード キータイプ 位置
constexpr uint8_t POS_FERICA_SERVICE_CD = 34;   //!< FERICA サービスコード キーデータ 位置
constexpr uint8_t POS_FERICA_BLOCK1 = 38;       //!< FERICA 1ブロック目 ブロック番号 位置
constexpr uint8_t POS_FERICA_BLOCK2 = 40;       //!< FERICA 2ブロック目 ブロック番号 位置

constexpr uint32_t MS_TMOUT_WAIT_READER_STARTED = 4100; //!< リーダ起動待ちタイムアウト時間(ms)
constexpr uint32_t MS_TM_SHORTEST_TIME_TO_START = 4000; //!< 最短起動時間(ms)
constexpr uint32_t MS_TMOUT_WAIT_BOOT = 10000; //!< ＦＷ起動タイムアウト時間 (ms) － ここまで起動しなかったら再起動（あくまで保険）

#ifdef DEBUG_DFLASH_WRITE_INTERVAL_SHORTEN
constexpr uint32_t MS_TMOUT_ROM_UPDATE = 15*1000; //!< ROM更新インターバルタイマー周期(ms) --- 加速評価
#else
constexpr uint32_t MS_TMOUT_ROM_UPDATE = 60*1000; //!< ROM更新インターバルタイマー周期(ms)
#endif

constexpr uint32_t MS_LIMIT_SYS_ERROR_ROM_UPDATE = 10; //!< システムエラー発生時のROM更新リミット値

constexpr uint8_t INITIAL_MIFARE_KEYA = '0';      // イニシャル設定 MIFARE KEYA
constexpr uint8_t INITIAL_MIFARE_KEYB = '1';      // イニシャル設定 MIFARE KEYA
constexpr uint8_t INITIAL_FERICA_VALID = 'V';     // イニシャル設定 FERICA 有効
constexpr uint8_t INITIAL_FERICA_INVALID = 0x20;  // イニシャル設定 FERICA 無効(設定用)

/*!------------------------------------------------
@brief イニシャル設定情報クラス
@note
@author
-------------------------------------------------*/
class InitialInfo
{
public:
    uint8_t Mifare1_KeyType;                             // Mifare 1ブロック目 キータイプ : '0'(0x30):KeyA / '1'(0x31):KeyB / それ以外:無効
    uint8_t Mifare1_KeyData[LEN_INI_MIFARE_KEY];         // Mifare 1ブロック目 読取位置のキーデータ(バイナリ6バイト)
    uint8_t Myfare1_Block;                               // Mifare 1ブロック目 ブロック番号 : 0x00～0x3E

    uint8_t Mifare2_KeyType;                             // Mifare 2ブロック目 キータイプ : '0'(0x30):KeyA / '1'(0x31):KeyB / それ以外:無効
    uint8_t Mifare2_KeyData[LEN_INI_MIFARE_KEY];         // Mifare 2ブロック目 読取位置のキーデータ(バイナリ6バイト)
    uint8_t Myfare2_Block;                               // Mifare 2ブロック目 ブロック番号 : 0x00～0x3E

    uint8_t Ferica_ValidBlock1;                          // Ferica 1ブロック目 が有効 'V'(0x56):有効 / それ以外:無効
    uint8_t Ferica_ValidBlock2;                          // Ferica 2ブロック目 が有効 'V'(0x56):有効 / それ以外:無効
    uint8_t Ferica_SystemCd[LEN_INI_FERICA_SYSTEM];      // Ferica システムコード(バイナリ2バイト)
    uint8_t Ferica_ServiceCd[LEN_INI_FERICA_SERVICE];    // Ferica サービスコード(バイナリ2バイト)
    uint8_t Ferica_Block1;                               // Ferica 1ブロック目 ブロックコード: 0x00～0xFF
    uint8_t Ferica_Block2;                               // Ferica 2ブロック目 ブロックコード: 0x00～0xFF

    //! コンストラクタ
    InitialInfo() { Clear(); }

    //! 全データクリア
    void Clear()
    {
        memset(this, 0, sizeof(InitialInfo));
    }

    //! データ有効確認
    uint8_t IsValidData(const uint8_t* buf, uint32_t len)
    {
        for (uint32_t i = 0; i < len; i++) 
        {
            if (buf[i] == 0x20) 
            {
                return 0;   // 無効
            }
        }
        return 1;   // 有効
    }
};


constexpr uint32_t SZ_LED_BUF_SIZE = 4; // LED表示状態格納領域サイズ
constexpr uint32_t SZ_FILE_NAME_WHEN_SYS_ERROR_OCUURS = 32; 
constexpr uint32_t SZ_TASKNAME_WHEN_SYS_ERROR_OCUURS = 24;
constexpr uint32_t SZ_MESSAGE_WHEN_SYS_ERROR_OCUURS = 32;


/*!------------------------------------------------
@brief データFlash格納の基本クラス
@note 格納するデータ、及び、レコードIDは派生クラスで宣言
@author
-------------------------------------------------*/
class BaseInDataFlash
{
protected:
    virtual uint32_t GetVeeRecId() = 0;     // データフラッシュ管理ドライバで読み書きするレコードのID（派生クラスで実装）
    virtual uint32_t GetRecSize() = 0;      // データフラッシュ管理ドライバで読み書きするレコードのサイズ（派生クラスで実装）
    virtual void* GetBufferPointer() = 0;   // データフラッシュ管理ドライバで読み書きするレコードのメモリ上のバッファポインタ
    virtual void AfterLoad() { }            // データフラッシュロード後の処理（派生先で実装しなければ空）

public:
    // 格納するデータは派生クラスで実装

    BaseInDataFlash() { }       // コンストラクタ
    virtual ~BaseInDataFlash() {}   // デストラクタ

    TYPE_ERROR Load();          // ROMに保存されている情報をロードする。必要に応じてチェックサムも計算する。
    void Update();              // 情報が変化していたら、データROMを更新する
    static void Format();       // データFlashの強制消去（通常はライブラリが自動的に消去する）
};

constexpr uint32_t SZ_ROM_WTMP_BUF_SIZE = 300;   //!< ROM書込時の排他対策用一時バッファ


class StoredDataInfo
{
public:
    //! イベントフラグ・累積回数 データ保持用バッファ
    uint8_t MemoryBuf[SZ_ROM_MEM_BUF_SIZE];

    //! LED表示状態 - 次回起動時の初期表示用（指示文字が0,T,U,V,W,Xの場合のみ）
    uint8_t ArrayLedStatusForInit[SZ_LED_BUF_SIZE];

    StoredDataInfo();           // コンストラクタ
    void Clear();               // 初期化
    void SetHeaderData();       // ヘッダデータをセット
};


/*!------------------------------------------------
@brief データFlashに格納するデータクラス
@note
@author
-------------------------------------------------*/
class DataStoredInDataFlash : public BaseInDataFlash
{
protected:
    virtual uint32_t GetVeeRecId() { return 1; }    // レコードID = データクラス
    virtual uint32_t GetRecSize() { return sizeof(info); }
    virtual void* GetBufferPointer() { return &info; }
    virtual void AfterLoad() { info.SetHeaderData(); }  // ロード後にヘッダ情報を上書き （ロード後処理としてヘッダ情報をセット）

public:
    StoredDataInfo info;

    DataStoredInDataFlash();            // コンストラクタ
    virtual ~DataStoredInDataFlash() {} // デストラクタ
    void Clear(){info.Clear();}         // 初期化（フォーマット時、イベントフラグ・累積回数初期化 時に呼ぶ）
};

class SystemErrorInfo
{
public:
    //! システムエラー発生カウンタ
    //! システムエラーのデータFlash領域への保存は最後の１個のみ。
    //! マルチリーダは起動からの時刻しか持たないので、発生する度にカウンタをアップする。
    uint32_t CounterSysErrorOcuurs;

    //! システムエラーが発生した時点の起動からの時間
    uint32_t TimeTickWhenSysErrorOcuurs;

    //! システムエラーが発生したソースファイル（\0終端あり)
    char FileNameWhenSysErrOccurs[SZ_FILE_NAME_WHEN_SYS_ERROR_OCUURS];

    //! システムエラーが発生したソースファイルの行
    uint32_t LineWhenSysErrOccurs;

    //! システムエラー対象のタスク名（\0終端あり）
    char TaskNameWhenSysErrOccurs[SZ_TASKNAME_WHEN_SYS_ERROR_OCUURS];

    //! システムエラーのメッセージ（\0終端あり）
    char MessageWhenSysErrOccurs[SZ_MESSAGE_WHEN_SYS_ERROR_OCUURS];

    SystemErrorInfo();                  // コンストラクタ
    void Clear();                       // 初期化（フォーマット時に呼ぶ）
};


/*!------------------------------------------------
@brief データFlashに格納するシステムエラー情報
@note
@author
-------------------------------------------------*/
class SystemErrorStoredInDataFlash : public BaseInDataFlash
{
protected:
    virtual uint32_t GetVeeRecId() { return 2; } // レコードID = システムエラー情報
    virtual uint32_t GetRecSize() { return sizeof(info); }
    virtual void * GetBufferPointer() { return info; }

public:
    SystemErrorInfo info[3];

    SystemErrorStoredInDataFlash();     // コンストラクタ
    virtual ~SystemErrorStoredInDataFlash() {}  // デストラクタ
    void Clear()                        // 初期化（フォーマット時に呼ぶ）
    {
        for(int i=0; i<3; i++)
        {
            info[i].Clear();
        }
    }
    TYPE_ERROR SetInfoSysErrOccurs(const char* pFileName, uint32_t line, const char* pTaskName, const char* pMessage, uint32_t timeTick);   // システムエラー発生箇所の情報セット
};


/*!------------------------------------------------
@brief データ管理タスククラス
@note
@author
-------------------------------------------------*/
class DataManager : public BaseTask
{
private:   

    //デバッグ用状態文字定義
    //文字列配列_pStateString[]の文字列は必ずenumの状態定義順に合わせること
    const char* _pStateString[ST_DATA_MAX] = {
        "DORMANT",	                //!< 停止
        "WAIT_READER_STARTED",	    //!< リーダ起動待ち（NFC、QR 両方待ち）
        "WAIT_RESETDATA_SENDED",    //!< リセット送信リーダ送信待ち
        "RUNNING",	                //!< 動作中
        "STOP",	                    //!< 停止中（FW更新タスク動作中）
    };

    //デバッグ用イベント文字定義
    //文字列配列_pEventString[]の文字列は必ずenumのイベント定義順に合わせること
    const char* _pEventString[EVT_DATA_MAX] = {
        "IDLE",			    //!< イベント無し(必須)
        "START",			//!< 開始

        "QR_STAETED",       //!< QRリーダ 起動完了通知
        "NFC_STAETED",      //!< NFCリーダ 起動完了通知
        "RESETDATA_SENDED", //!< リセットデータ送信完了通知
        "LED",              //!< LED更新指示
        "SET_INITIAL",		//!< イニシャル設定 
        "CLS_CUMULATIVE",   //!< 累積回数クリア

        "TMOUT_WAIT_READER_STARTED", //!< リーダ起動待ちタイムアウト（TIMER）
        "TMOUT_WAIT_BOOT",  //!< 装置起動待ちタイムアウト（TIMER）
        "TM_SHORTEST_TIME_TO_START", //!< 最短起動時間経過通知（TIMER）
        "TM_INTERVAL_ROM_UPDATE",    //!< ROM更新インターバル通知（TIMER）
    };

    const uint32_t _adrs_Region1_Begin = 0x00003800;    // CodeFlash 領域1 先頭アドレス
    const uint32_t _adrs_Region1_End = 0x000217FF;      // CodeFlash 領域1 後尾アドレス

    InitialInfo _InitialInfo;           //!< イニシャル情報

    DataStoredInDataFlash _DataStoredInDataFlash;               //!< データFlash格納データ
    SystemErrorStoredInDataFlash _SystemErrorStoredInDataFlash;  //!< システムエラーFlash格納データ

    uint8_t _DipSw1Info = 0;            //!< DIPSW1(機能選択）設定情報
    uint8_t _DipSw2Info = 0;            //!< DIPSW2(リーダアドレス）設定情報
    uint16_t _ChecksumRunningFw = 0;    //! 起動中FWのチェックサム（起動時に毎回再計算をしてここに登録）

    // リーダ状態
    uint8_t _IsNfcStarted = 0;          //!< NFCリーダ起動完了？  1:正常（起動完了 or 未使用） 0:異常（初期化完了待ち or エラー発生）
    uint8_t _IsQrStarted = 0;           //!< QRリーダ起動完了？   1:正常（起動完了 or 未使用） 0:異常（初期化完了待ち or エラー発生）
    uint8_t _IsDoorNormality = 0;       //!< 自動扉接点出力正常？ 1:正常（未使用含む）  0:異常
    uint8_t _IsEepromNormality = 0;     //!< EEPROM正常?       1:正常（ロード完了） 0:異常

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

    // 停止 状態
    static void Dormant_Start(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen); // 開始

    // リーダ起動待ち（NFC、QR 両方待ち）状態
    static void WaitReaderStarted_Enter(BaseTask* pOwnTask);  // Enter
    static void WaitReaderStarted_Exit(BaseTask* pOwnTask);  // Exit
    static void WaitReaderStarted_NfcOrQrReaderStarted(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen); // リーダ起動完了
    static void WaitReaderStarted_TmoutWaitReaderStarted(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen); // リーダ起動待ちタイムアウト

    // リセットデータ送信待ち状態
    static void WaitResetdataSended_Enter(BaseTask* pOwnTask);  // Enter
    static void WaitResetdataSended_Exit(BaseTask* pOwnTask);  // Exit
    static void WaitResetdataSended_TmShortestTimeToStart(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen);
    void __WaitResetdataSended_IfDoneShortestTimeToStartCommandReceiver(); // コマンド受信タスクを開始
    static void WaitResetdataSended_ResetDataSended(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen);
    static void WaitResetdataSended_TmoutWaitBoot(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen);

    // 動作中 状態
    static void Running_Enter(BaseTask* pOwnTask);  // Enter
    static void Running_Exit(BaseTask* pOwnTask);  // Exit
    static void Running_Led(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen); // LED更新指示
    static void Running_SetInitial(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen); // イニシャル設定
    static void Running_ClsCumulative(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen); // 累積回数クリア
    static void Running_TmIntervalRomUpdate(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen); // ROM更新インターバル通知

    // 停止中（FW更新中）状態
    //（イベント処理は無し： この状態では、FW更新ロジックから直接 SystemSaveAndReboot() が呼ばれ再起動となる ）

    /*----------------------------
    //コンストラクタ
    -----------------------------*/
    DataManager();
    virtual ~DataManager() {}

    /*!------------------------------------------------
    @brief タスク名
    -------------------------------------------------*/
    const char* GetTaskName() { return "DataManager"; }

    /*----------------------------
    //外部公開関数（他タスクよりアクセス）
    -----------------------------*/
    ENUM_SEL_FUNC_FELICA GetSelFuncFelica(); // 機能選択 Felica情報を取得 
    ENUM_SEL_FUNC_MIFARE GetSelFuncMifare(); // 機能選択 Mifare情報を取得
    int8_t IsSelFuncBuzzer();       // 機能選択 ブザー有無
    int8_t IsSelFuncQrReader();     // 機能選択 QRリーダー 有無
    uint8_t GetReaderAddress();     // リーダーアドレスを取得
    uint8_t GetReaderHwStatus();    // リーダーハードウエア状態を取得
    void SetErrorFlag(ERRORFLAG_OFFSET pos, ERRORFLAG val); // イベントフラグをセットする
    void AddEventCount(EVENT_COUNT pos); // 累積回数を＋１する
    const uint8_t* GetRomDataBuf(); // 現在のメモリデータ（イベントフラグ・累積回数）を取得する
    uint8_t GetInfoDipSw1();        // DIP SW1 の情報を返す
    uint8_t GetInfoDipSw2();        // DIP SW2 の情報を返す
    const uint8_t* GetLedStatusForInit(); // LED点灯状態を返す
    uint16_t GetCheckSum();         // 現在実行中のチェックサム値を取得する
    const InitialInfo* GetInitialInfo(); // イニシャル設定情報を取得する

    void SetForcedStop();           // FW更新開始により強制停止
    void SetSystemErrorInfoAndReboot(const char* pFileName, uint32_t line, const char* pTaskName, const char* pMessage);
                                    // システムエラー情報をROMに書き込みリブート
    void SystemSaveAndReboot();     // ROM書込後リブート（書くかどうかは、DataManagerの状態に依存する）
    void DataFlashFormat();         // データFlashのフォーマット（初期１レコード有）
    void DataFlashErase();          // データFlashの消去（全てFF）－ あくまで評価用（基板部品実装直後の状態）
    void DataFlashUpdateForce();    // データFlashを強制的に更新

    static const char *GetSerialNo() { return "        "; } // シリアルNoは装置に保存しないこととなったので、空を返すようにする

    static uint8_t SetSerialNo(const char* serno)
    {
        (void) serno;
        // シリアルNoは装置に保存しないこととなった為、実装中止。
        // （シリアルNoはデータFlashの空き領域に保存する予定だった。）
        return 1;
    }

    void PrintInfo(void (*funcPrint)(const char*)); // タスクの情報をデバッグ出力する
    void PrintInfoDataFlashBuffer(void (*funcPrint)(const char*));

    void PrintInfo_InitialInfo(void (*funcPrint)(const char*));
    void PrintInfo_DataStoredInDataFlash(void (*funcPrint)(const char*));
    void PrintInfo_SystemErrorStoredInDataFlash(void (*funcPrint)(const char*));
    void PrintInfo_Board(void (*funcPrint)(const char*));
    void PrintInfo_HwStatus(void (*funcPrint)(const char*));
    void PrintInfo_ForDebug(void (*funcPrint)(const char*));

    uint16_t GetCount_EVENT_ERROR_HOST_PRI_FER()
    {
        uint8_t* pVal_H = (uint8_t*)(&_DataStoredInDataFlash.info.MemoryBuf[EEPROM_MAP_COUNTER_OFFSET + (EVENT_ERROR_HOST_PRI_FER * 2)]);
        uint8_t* pVal_L = pVal_H + 1;
        uint16_t val = (uint16_t)(*pVal_H << 8) + *pVal_L;  //値に変換する
        return val;
    }


    /*----------------------------
    //タスク内部ロジック
    -----------------------------*/
    void CalculateCheckSum();
    void LoadDipSw();
    void NoticeInitLedInfoToOutputController();
    static void DummyDelay(uint32_t num);
};

#endif
