/***********************************************************************/
/*  作成日      :2017.07.31	                                   	       */
/*  処理        :共通型定義ヘッダ                	  				   */
/*  備考		:		                                          	   */
/***********************************************************************/

#ifndef TYPEDEF_H
#define TYPEDEF_H
#ifdef __cplusplus
#include <cstdint>

class BaseTask;
class State;

extern "C" {
#endif /*__cplusplus*/


//!型定義
//typedef char			int8_t;
//typedef signed char		int8_t;
//typedef signed short	int16_t;
//typedef signed long		int32_t;
//typedef unsigned char	uint8_t;
//typedef unsigned short	uint16_t;
//typedef unsigned long	uint32_t;
//typedef void			(*FP)();

//#define NULL nullptr

typedef enum {
	OFF = 0,		//!< OFF
	ON				//!< ON
}TYPE_ENUM_ONOFF;

typedef enum {
	E_OK = 0,		//!< OK
	E_ELSE,			//!< その他エラー
	E_PAR,			//!< パラメータエラー(パラメータが範囲外等)
	E_SUM,			//!< チェックサムエラー
	E_BSY,			//!< ビジーエラー
	E_TOUT,			//!< タイムアウトエラー
	E_EMP,			//!< 空エラー(キューにデータ無し等)
	E_STOP,			//!< 停止エラー
	E_NOMEM,		//!< メモリ不足(共通メモリ確保の失敗)
	E_FULL,			//!< キュー等が満状態
	E_NOEXIST,		//!< 該当無し
	E_NOT_RELEASE,	//!< バス未開放
	E_RETRY,		//!< リトライエラー
	E_UNKNOWN,		//!< 不明(コマンドが一致しない等)
	E_SYS,			//!< システムエラー
	E_ID,			//!< ID不正
	E_UNKNOWN_STE,	//!< 不明状態検出
	E_UNKNOWN_EVT,	//!< 不明イベント検出
	E_COLLISION,	//!< 衝突検知
	E_NOT_MY_ADRS,	//!< 自アドレスでない
	E_BCC,			//!< BCCエラー
	E_MAX,			//!< 登録可能数を超過
	E_REGISTERED,	//!< 登録済み
	E_NO_INIT,		//!< 未初期化
}TYPE_ERROR;

#ifdef __cplusplus
}

/*!------------------------------------------------
@brief イベント実行
@note
@param pOwnTask 自タスクオブジェクトポインタ
@param EventId イベントID
@param pData データ格納先頭ポインタ
@param DataLen データサイズ
@return なし
@author
-------------------------------------------------*/
typedef void (*PF_DO_EVENT)(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen);

/*!------------------------------------------------
@brief 状態入口処理実行
@note
@param pOwnTask 自タスクオブジェクトのポインタ
@return なし
@author
-------------------------------------------------*/
typedef void (*PF_DO_ENTER)(BaseTask* pOwnTask);

/*!------------------------------------------------
@brief 状態出口処理実行
@note
@param pOwnTask 自タスクオブジェクトのポインタ
@return なし
@author
-------------------------------------------------*/
typedef void (*PF_DO_EXIT)(BaseTask* pOwnTask);

#endif /*__cplusplus*/

#endif	/* _TYPEDEF_H */
