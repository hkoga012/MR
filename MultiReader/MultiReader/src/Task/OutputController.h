/*!------------------------------------------------
@brief 出力ポート制御タスクヘッダ
@note
@author
-------------------------------------------------*/
#ifndef OUTPUT_CONTROLLER_H
#define OUTPUT_CONTROLLER_H
#include "../StateTransition/BaseTask.h"

#ifdef WINDOWS_DEBUG
#include "../../../WinMultiReader/SimPanel/SimPanelClient.h"
#endif

//******************************************
// 状態は最低一つ登録する。起動直後は先頭に登録された状態(値=0)に遷移する
//*****************************************
//!< 状態定義
typedef enum {
    ST_OUTPUT_DORMANT = 0,		//!< 停止（必須）
    ST_OUTPUT_RUNNING,			//!< 動作中
    ST_OUTPUT_MAX,				//!< イベント種別の最大数(削除禁止)
}ENUM_STATE_OUTPUT;

//******************************************
// イベントを定義する際は必ずIdle(値=0)を定義すること
//*****************************************
//!< イベント定義
constexpr uint32_t OUTPUT_EVENT_QUE_SIZE = 10; //!< イベントバッファサイズ
typedef enum {
    EVT_OUTPUT_IDLE = 0,      	//!< イベント無し(必須)
    EVT_OUTPUT_START,			//!< 開始

//《タスク外から通知されるイベント》
    EVT_OUTPUT_LED,             	//!< LED更新指示	パラメータ4バイト([0:Left、1:Center、2:Right、3:Frame]:点灯指示文字 '0'-'X','Z'
    EVT_OUTPUT_BUZZER,          	//!< BUZZER更新指示 パラメータ1バイト([0]:鳴動指示文字 '0'-'9','Z'
    EVT_OUTPUT_AUTODOOR,        	//!< AUTODOOR更新指示 パラメータ1バイト([0]:出力指示文字 '0'-'9','Z'
	EVT_OUTPUT_FW_UPDATING,	        //!< FW更新中通知

//《タスク内部イベント》
    EVT_OUTPUT_NOTICE_TIMER,    //!< タイマー通知(50msec周期)

    EVT_OUTPUT_MAX,             //!< タスクイベント種別最大数(削除禁止)
}ENUM_EVT_OUTPUT;

//!< LED定義
enum ENUM_LED{
    LED_IDX_LEFT = 0,       //!< LED Left
    LED_IDX_CENTER,         //!< LED Center
    LED_IDX_RIGHT,          //!< LED Right
    LED_IDX_FRAME,          //!< LED Frame
    LED_IDX_MAX,            //!< LED MAX
};

constexpr uint32_t TIMER_OUTPUT_MAX = 1;	    //!< タイマー登録数
constexpr uint32_t LED_COMMAND_MAX = 36;        //!< LEDコマンド最大数
constexpr uint32_t LED_PATTERN_MAX = 13;        //!< LEDパターン最大数
constexpr uint32_t BUZZER_COMMAND_MAX = 11;     //!< BUZZERコマンド最大数
constexpr uint32_t BUZZER_PATTERN_MAX = 13;     //!< BUZZERパターン最大数
constexpr uint32_t AUTODOOR_COMMAND_MAX = 10;   //!< AUTODOORコマンド最大数
constexpr uint32_t AUTODOOR_PATTERN_MAX = 3;    //!< AUTODOORパターン最大数
constexpr uint32_t DETECT_CARD_TIMEOUT = 20;    //!< カード検出処理のタイムアウト時間
/*!------------------------------------------------
@brief 出力ポート制御タスククラス
@note
@author
-------------------------------------------------*/
class OutputController : public BaseTask
{
public:
    //!< デバッグ用状態文字定義
    //文字列配列_pStateString[]の文字列は必ずenumの状態定義順に合わせること
    const char* _pStateString[ST_OUTPUT_MAX] = {
        "DORMANT",	        //!< 停止
        "RUNNUNG",	        //!< 動作中
    };

    //!< デバッグ用イベント文字定義
    //文字列配列_pEventString[]の文字列は必ずenumのイベント定義順に合わせること
    const char* _pEventString[EVT_OUTPUT_MAX] = {
        "IDLE",			        //!< イベント無し(必須)
        "START",			    //!< 開始
        "LED",                  //!< LED更新指示
        "BUZZER",               //!< BUZZER更新指示
        "AUTODOOR",             //!< AUTODOOR更新指示
        "FW_UPDATING",          //!< ＦＷ更新中通知
        "NOTICE_TIMER",         //!< タイマー通知(50msec周期)
    };


    //////////////////////////////////
    //              LED
    //////////////////////////////////
    //!< LED色指定定義
    enum ENUM_LED_CBIT {
        RBIT = 0x04,  //!< 赤
        GBIT = 0x02,  //!< 緑
        BBIT = 0x01,  //!< 青
    };

    //////////////////////////////////
    //            共通
    // ///////////////////////////////
    //! 共通制御データ定義
    enum ENUM_CTRL {
        IO_OFF = 0,  //!< RGB消灯/鳴動停止/リレーOFF
        IO_ON,       //!< 鳴動/リレーON
        IO_RED,      //!< 赤点灯
        IO_GRN,      //!< 緑点灯
        IO_BLE,      //!< 青点灯
        IO_ORG,      //!< 橙(赤、緑点灯)
        IO_WHE,      //!< 白(赤、緑、青点灯)
        IO_END,      //!< 終了(パターンデータの終端を示す)
        IO_KEP,      //!< 現状維持(現在の状態を維持[Keep]する)
        IO_TOP,      //!< 先頭に戻る(無限ループ)
    };

    //!<共通パターンデータ定義
    typedef struct {
        ENUM_CTRL Ctrl;  //!< 制御(IO_OFF～IO_TOP)
        uint8_t Count;   //!< 指定制御継続回数(50msec単位 例:2を指定すると指定制御を100msec継続)
    }DATA;

	//LEDの50msec毎動作定義テーブル
    const DATA _ledPattern[LED_COMMAND_MAX][LED_PATTERN_MAX] = {
        {{IO_OFF, 1}, {IO_KEP, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}},  //!< '0':全消灯
        {{IO_GRN,40}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}},  //!< '1':2秒間 緑点灯
        {{IO_RED,40}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}},  //!< '2':2秒間 赤点灯
        {{IO_BLE,40}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}},  //!< '3':2秒間 青点灯
        {{IO_ORG,40}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}},  //!< '4':2秒間 橙点灯
        {{IO_WHE,40}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}},  //!< '5':2秒間 白点灯
        {{IO_GRN,60}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}},  //!< '6':3秒間 緑点灯
        {{IO_RED,60}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}},  //!< '7':3秒間 赤点灯
        {{IO_BLE,60}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}},  //!< '8':3秒間 青点灯
        {{IO_ORG,60}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}},  //!< '9':3秒間 橙点灯
        {{IO_WHE,60}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}},  //!< 'A':3秒間 白点灯
        {{IO_GRN, 5}, {IO_OFF, 5}, {IO_GRN, 5}, {IO_OFF, 5}, {IO_GRN, 5}, {IO_OFF, 5}, {IO_GRN, 5}, {IO_OFF, 5}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}},  //!< 'B':緑0.25秒おき2秒間点滅
        {{IO_RED, 5}, {IO_OFF, 5}, {IO_RED, 5}, {IO_OFF, 5}, {IO_RED, 5}, {IO_OFF, 5}, {IO_RED, 5}, {IO_OFF, 5}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}},  //!< 'C':赤0.25秒おき2秒間点滅
        {{IO_BLE, 5}, {IO_OFF, 5}, {IO_BLE, 5}, {IO_OFF, 5}, {IO_BLE, 5}, {IO_OFF, 5}, {IO_BLE, 5}, {IO_OFF, 5}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}},  //!< 'D':青0.25秒おき2秒間点滅
        {{IO_ORG, 5}, {IO_OFF, 5}, {IO_ORG, 5}, {IO_OFF, 5}, {IO_ORG, 5}, {IO_OFF, 5}, {IO_ORG, 5}, {IO_OFF, 5}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}},  //!< 'E':橙0.25秒おき2秒間点滅
        {{IO_WHE, 5}, {IO_OFF, 5}, {IO_WHE, 5}, {IO_OFF, 5}, {IO_WHE, 5}, {IO_OFF, 5}, {IO_WHE, 5}, {IO_OFF, 5}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}},  //!< 'F':白0.25秒おき2秒間点滅
        {{IO_GRN,10}, {IO_OFF,10}, {IO_GRN,10}, {IO_OFF,10}, {IO_GRN,10}, {IO_OFF,10}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}},  //!< 'G':緑0.5秒おき3秒間点滅
        {{IO_RED,10}, {IO_OFF,10}, {IO_RED,10}, {IO_OFF,10}, {IO_RED,10}, {IO_OFF,10}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}},  //!< 'H':赤0.5秒おき3秒間点滅
        {{IO_BLE,10}, {IO_OFF,10}, {IO_BLE,10}, {IO_OFF,10}, {IO_BLE,10}, {IO_OFF,10}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}},  //!< 'I':青0.5秒おき3秒間点滅
        {{IO_ORG,10}, {IO_OFF,10}, {IO_ORG,10}, {IO_OFF,10}, {IO_ORG,10}, {IO_OFF,10}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}},  //!< 'J':橙0.5秒おき3秒間点滅
        {{IO_WHE,10}, {IO_OFF,10}, {IO_WHE,10}, {IO_OFF,10}, {IO_WHE,10}, {IO_OFF,10}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}},  //!< 'K':白0.5秒おき3秒間点滅
        {{IO_ORG, 5}, {IO_GRN, 5}, {IO_ORG, 5}, {IO_GRN, 5}, {IO_ORG, 5}, {IO_GRN, 5}, {IO_ORG, 5}, {IO_GRN, 5}, {IO_ORG, 5}, {IO_GRN, 5}, {IO_ORG, 5}, {IO_GRN, 5}, {IO_END, 0}},  //!< 'L':橙→緑→橙0.25秒おき3秒間点滅
        {{IO_ORG, 5}, {IO_RED, 5}, {IO_ORG, 5}, {IO_RED, 5}, {IO_ORG, 5}, {IO_RED, 5}, {IO_ORG, 5}, {IO_RED, 5}, {IO_ORG, 5}, {IO_RED, 5}, {IO_ORG, 5}, {IO_RED, 5}, {IO_END, 0}},  //!< 'M':橙→赤→橙0.25秒おき3秒間点滅
        {{IO_ORG, 5}, {IO_GRN, 5}, {IO_RED, 5}, {IO_ORG, 5}, {IO_GRN, 5}, {IO_RED, 5}, {IO_ORG, 5}, {IO_GRN, 5}, {IO_RED, 5}, {IO_ORG, 5}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}},  //!< 'N':橙→緑→赤→橙0.25秒おき3秒間点滅
        {{IO_ORG,10}, {IO_GRN,10}, {IO_ORG,10}, {IO_GRN,10}, {IO_ORG,10}, {IO_GRN,10}, {IO_ORG,10}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}},  //!< 'O':橙→緑→橙0.5秒おき3秒間点滅
        {{IO_ORG,10}, {IO_RED,10}, {IO_ORG,10}, {IO_RED,10}, {IO_ORG,10}, {IO_RED,10}, {IO_ORG,10}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}},  //!< 'P':橙→赤→橙0.5秒おき3秒間点滅
        {{IO_ORG,10}, {IO_GRN,10}, {IO_RED,10}, {IO_ORG,10}, {IO_GRN,10}, {IO_RED,10}, {IO_ORG,10}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}},  //!< 'Q':橙→緑→赤→橙0.5秒おき3秒間点滅
        {{IO_GRN,10}, {IO_OFF, 5}, {IO_GRN,10}, {IO_OFF, 5}, {IO_GRN,10}, {IO_OFF, 5}, {IO_GRN,10}, {IO_OFF, 5}, {IO_GRN,10}, {IO_OFF, 5}, {IO_GRN,10}, {IO_OFF, 5}, {IO_END, 0}},  //!< 'R':緑(0.5S)→消灯(0.25S)→緑(0.5S)→消灯(0.25S)→緑(0.5S)→消灯(0.25S)→緑(0.5S)
        {{IO_GRN,25}, {IO_WHE,25}, {IO_ORG,25}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}},  //!< 'S':緑(1.25S)→白(1.25S)→橙(1.25S)
        {{IO_GRN, 1}, {IO_KEP, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}},  //!< 'T':常時 緑点灯
        {{IO_RED, 1}, {IO_KEP, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}},  //!< 'U':常時 赤点灯
        {{IO_BLE, 1}, {IO_KEP, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}},  //!< 'V':常時 青点灯
        {{IO_ORG, 1}, {IO_KEP, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}},  //!< 'W':常時 橙点灯
        {{IO_WHE, 1}, {IO_KEP, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}},  //!< 'X':常時 白点灯
        {{IO_ORG, 5}, {IO_OFF, 5}, {IO_TOP, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}},  //!< 'a':橙0.25秒おき無限点滅(起動時)
        {{IO_RED,25}, {IO_OFF,25}, {IO_TOP, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}},  //!< 'b':赤1.25秒おき無限点滅(FW更新時)
        //!< 'Z':何もしない
    };
    typedef struct {
        DATA*   pCurrent;	    //!< 現在のパターンポインタ（_LedPattern）
        uint8_t Command;        //!< 現在のコマンド指示文字
	    uint8_t PatternIndex;   //!< 現在のパターン実行位置(0-19)
        DATA    WorkData;       //!< 一時的に使用するワーク変数(動作定義テーブルのデータ１つ分を格納する)
        bool PrevIsDetectedCard;    //!< カード検出フラグ(更新前)
        DATA    LastData;       //!< 最後に設定したパターンデータ(L_END,L_KEPは除く)
        uint8_t EndCommand;     //!< パターン終了時実施コマンド(常時点灯の'T'～'X',常時消灯の'0'のみ格納)
	}TYPE_LED_CTRL;

	TYPE_LED_CTRL _ledControl[LED_IDX_MAX];	//!< LED制御変数
	bool _isDetectedCard;                   //!< カード検出フラグ
	uint32_t _DetectedCardTimeout;          //!< カード検出フラグタイムアウト時間


    //!< BUZZER/自動ドア共通制御データ定義
    typedef struct {
        DATA* pCurrent;	        //!< 現在のパターンポインタ（_buzzerPattern/_autoDoorPattern）
        uint8_t Command;        //!< 現在のコマンド指示文字(指示文字)
        uint8_t PatternIndex;   //!< 現在のパターン実行位置
        DATA    WorkData;       //!< 一時的に使用するワーク変数(ブザーまたは自動ドア動作定義テーブルのデータ１つ分を格納する)
        bool Enable;            //!< 出力可能フラグ
    }TYPE_CTRL;

    //////////////////////////////////
    //          BUZZER
    // ///////////////////////////////
    //ブザー動作定義テーブル
    //各データのカウントは50msec単位とする(10を指定した場合は10*50msec=500msec)
    //動作例）{IO_ON, 2}, {IO_OFF, 2}, {IO_ON,  2}, {IO_END, 0}を指定した場合
    //        ブザーを100msec鳴動→ブザーOFF100msec→ブザーを100msec鳴動→終了(IO_END)
    const DATA _buzzerPattern[BUZZER_COMMAND_MAX][BUZZER_PATTERN_MAX] = {
        {{IO_OFF,1}, {IO_END, 0}, {IO_END, 0}, {IO_END, 0}, {IO_END,0}, {IO_END,0}, {IO_END,0}, {IO_END,0}, {IO_END,0}, {IO_END,0}, {IO_END,0}, {IO_END,0}, {IO_END,0}},   //!< '0':ブザー停止
        {{IO_ON, 5}, {IO_OFF, 1}, {IO_END, 0}, {IO_END, 0}, {IO_END,0}, {IO_END,0}, {IO_END,0}, {IO_END,0}, {IO_END,0}, {IO_END,0}, {IO_END,0}, {IO_END,0}, {IO_END,0}},   //!< '1':ピッ0.5秒間（0.25秒のピッを一つだけ）
        {{IO_ON, 2}, {IO_OFF, 2}, {IO_ON,  2}, {IO_OFF, 1}, {IO_END,0}, {IO_END,0}, {IO_END,0}, {IO_END,0}, {IO_END,0}, {IO_END,0}, {IO_END,0}, {IO_END,0}, {IO_END,0}},   //!< '2':ピッピッ0.3秒間
        {{IO_ON, 2}, {IO_OFF, 2}, {IO_ON,  2}, {IO_OFF, 2}, {IO_ON, 2}, {IO_OFF,1}, {IO_END,0}, {IO_END,0}, {IO_END,0}, {IO_END,0}, {IO_END,0}, {IO_END,0}, {IO_END,0}},   //!< '3':ピッピッピッ0.5秒間
        {{IO_ON,10}, {IO_OFF, 1}, {IO_END, 0}, {IO_END, 0}, {IO_END,0}, {IO_END,0}, {IO_END,0}, {IO_END,0}, {IO_END,0}, {IO_END,0}, {IO_END,0}, {IO_END,0}, {IO_END,0}},   //!< '4':ピー0.5秒間
        {{IO_ON,10}, {IO_OFF,10}, {IO_ON, 10}, {IO_OFF, 1}, {IO_END,0}, {IO_END,0}, {IO_END,0}, {IO_END,0}, {IO_END,0}, {IO_END,0}, {IO_END,0}, {IO_END,0}, {IO_END,0}},   //!< '5':ピーピー1.5秒間
        {{IO_ON,10}, {IO_OFF,10}, {IO_ON, 10}, {IO_OFF,10}, {IO_ON,10}, {IO_OFF,1}, {IO_END,0}, {IO_END,0}, {IO_END,0}, {IO_END,0}, {IO_END,0}, {IO_END,0}, {IO_END,0}},   //!< '6':ピーピーピー2.5秒間
        {{IO_ON,60}, {IO_OFF, 1}, {IO_END, 0}, {IO_END, 0}, {IO_END,0}, {IO_END,0}, {IO_END,0}, {IO_END,0}, {IO_END,0}, {IO_END,0}, {IO_END,0}, {IO_END,0}, {IO_END,0}},   //!< '7':ピー連続3秒間
        {{IO_ON,10}, {IO_OFF, 5}, {IO_ON,  5}, {IO_OFF, 1}, {IO_END,0}, {IO_END,0}, {IO_END,0}, {IO_END,0}, {IO_END,0}, {IO_END,0}, {IO_END,0}, {IO_END,0}, {IO_END,0}},   //!< '8':ピーピッ1秒間
        {{IO_ON,10}, {IO_OFF, 5}, {IO_ON,  5}, {IO_OFF, 5}, {IO_ON,10}, {IO_OFF,5}, {IO_ON, 5}, {IO_OFF,5} ,{IO_ON,10}, {IO_OFF,5}, {IO_ON, 5}, {IO_OFF,5}, {IO_END,0}},   //!< '9':ピーピッピーピッピーピッ3.5秒間
        {{IO_ON, 1}, {IO_OFF, 1}, {IO_END, 0}, {IO_END, 0}, {IO_END,0}, {IO_END,0}, {IO_END,0}, {IO_END,0}, {IO_END,0}, {IO_END,0}, {IO_END,0}, {IO_END,0}, {IO_END,0}},   //!< 'A':ピッ0.05秒間（0.05秒のピッを一つだけ）(起動時)
    };
    
    TYPE_CTRL _buzzerControl;	    //!< BUZZER制御変数

    //////////////////////////////////
    //          AUTODOOR
    // ///////////////////////////////
    //自動ドア動作定義テーブル
    //各データのカウントは50msec単位とする(200を指定した場合は200*50msec=10sec)
    const DATA _autoDoorPattern[AUTODOOR_COMMAND_MAX][AUTODOOR_PATTERN_MAX] = {
        {{IO_OFF, 1}, {IO_END, 0}, {IO_END, 0} },   //!< '0':ブレイク信号を出します
        {{IO_ON,  1}, {IO_END, 0}, {IO_END, 0} },   //!< '1':メイク信号を出します
        {{IO_ON, 20}, {IO_OFF, 1}, {IO_END, 0} },   //!< '2':1秒間、メイク信号を出します
        {{IO_ON, 40}, {IO_OFF, 1}, {IO_END, 0} },   //!< '3':2秒間、メイク信号を出します
        {{IO_ON, 60}, {IO_OFF, 1}, {IO_END, 0} },   //!< '4':3 秒間、メイク信号を出します
        {{IO_ON,100}, {IO_OFF, 1}, {IO_END, 0} },   //!< '5':5 秒間、メイク信号を出します
        {{IO_ON, 10}, {IO_OFF, 1}, {IO_END, 0} },   //!< '6':0.5 秒間、メイク信号を出します
        {{IO_ON,140}, {IO_OFF, 1}, {IO_END, 0} },   //!< '7':7 秒間、メイク信号を出します
        {{IO_ON,180}, {IO_OFF, 1}, {IO_END, 0} },   //!< '8':9 秒間、メイク信号を出します
        {{IO_ON,200}, {IO_OFF, 1}, {IO_END, 0} },   //!< '9':10 秒間、メイク信号を出します
    };

    TYPE_CTRL _autoDoorControl;	    //!< AUTODOOR制御変数

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

    // 動作中 状態
    static void Running_Idle(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen);  // Idle
    static void Running_NoticeTimer(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen);  // タイマー通知
	static void Running_Led(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen);  // LED動作指示通知
	static void Running_Buzzer(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen);  // ブザー動作指示通知
	static void Running_AutoDoor(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen);  // ドア動作指示通知
    static void Running_FwUpdating(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen);  // FW更新中通知
    static void Running_Enter(BaseTask* pOwnTask); // Enter
    static void Running_Exit(BaseTask* pOwnTask); // Exit

    void SendBreakForInitAutoDoor(); // 初期状態として自動ドアブレイク信号を出力

    /*----------------------------
    //コマンド文字からLEDパターンインデックス取得
    -----------------------------*/
    int8_t GetLedPatternIndex(char Id);
    /*----------------------------
    //LED制御
    -----------------------------*/
	TYPE_ERROR LEDControl(void);
	/*----------------------------
    //LED出力判定
    -----------------------------*/
	TYPE_ERROR LEDOutput(uint8_t LedIndex, ENUM_CTRL Data, bool IsDetectedCard);
	/*----------------------------
    //LED出力
    -----------------------------*/
	TYPE_ERROR LEDRGB(uint8_t LedIndex, uint8_t DutyR, uint8_t DutyG, uint8_t DutyB, bool IsDetectedCard);

	/*----------------------------
    //コマンド文字からブザーパターンインデックス取得
    -----------------------------*/
    int8_t GetBuzzerPatternIndex(char Id);
    /*----------------------------
    //ブザー制御
    -----------------------------*/
    TYPE_ERROR BuzzerControl(void);
	
    /*----------------------------
    //コマンド文字から自動ドアパターンインデックス取得
    -----------------------------*/
	int8_t GetAutoDoorPatternIndex(char Id);
	/*----------------------------
    //自動ドア制御
    -----------------------------*/
	TYPE_ERROR AutoDoorControl(void);

	/*----------------------------
    // カード検出通知
    -----------------------------*/
	void DetectedCard(bool IsDetect);

	void PrintInfo(void (*funcPrint)(const char*));  // タスクの情報をデバッグ出力する

    /*----------------------------
    //コンストラクタ
    -----------------------------*/
    OutputController();
    virtual ~OutputController() {}

    /*!------------------------------------------------
    @brief タスク名
    -------------------------------------------------*/
    const char* GetTaskName() { return "OutputController"; }


private:

#ifdef WINDOWS_DEBUG
    SimPanelClient* _pSimPanel;
#endif

};

#endif
