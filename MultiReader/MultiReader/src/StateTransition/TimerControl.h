/*!------------------------------------------------
@brief タイマーイベント管理クラスヘッダ
@note　各タスクよりタイマー指示を設定され、
		指定時間が経過した場合にタイマーイベントを
		設定タスクへ送信する
@author
-------------------------------------------------*/

#ifndef TIMER_H
#define TIMER_H

#include "common.h"
constexpr uint32_t TIMER_INTERVAL_MSEC = 1;	//!< タイマーの基本となる割り込み周期(ミリ秒) ※１または１０のみ指定可能

//タイマー動作種別定義
typedef enum {
	TIMER_ONESHOT = 0,		//!< ワンショット通知(指定時間経過→通知後は動作停止 1回のみ通知)
	TIMER_FOREVER,			//!< 無限通知(指定時間経過→通知を繰り返す)
	TIMER_MAX,				//!< タイマー動作種別最大数(削除禁止)
}ENUM_TIMER_MODE;

template <typename T>
class   TimerControl {
private:	
	//!<タイマー管理構造体
	struct TimerData {
		uint32_t		EventId;		//!< イベントID(イベント通知用)
		bool			StopFlag;		//!< 停止フラグ
		ENUM_TIMER_MODE	Mode;			//!< 動作モード
		uint32_t		Msec;			//!< タイマー間隔
		uint32_t		Counter;		//!< 経過カウンタ
		bool			IsUsed;			//!< 使用中フラグ
	};

	T&					_pTask;			//!< 自タスク
	TimerData			*_pTimer;		//!< タイマー管理(1次元配列)
	uint32_t			_maxTimer;		//!< 最大登録可能数
	uint32_t			_indexTimer;	//!< 登録タイマー数
	uint32_t			_tickTimer;		//!< 起動時からの経過時間(TIMER_INTERVAL_MSEC単位)
public:

	/*!------------------------------------------------
	@brief コンストラクタ
	@note
	@param Max 最大登録可能数
	@return なし
	@author
	-------------------------------------------------*/
	TimerControl(T& Task, uint32_t Max)
		:_pTask(Task),_pTimer(new TimerData[Max]), _maxTimer(Max),_indexTimer(0)
	{
		//初期化
		for (uint32_t i = 0; i < _maxTimer; i++)
		{
			_pTimer[i].EventId = 0;
			_pTimer[i].StopFlag = true;
			_pTimer[i].Mode = TIMER_ONESHOT;
			_pTimer[i].Msec = 0;
			_pTimer[i].Counter = 0;
			_pTimer[i].IsUsed = false;
		}
		_tickTimer = 0;
	}

	/*!------------------------------------------------
	@brief タイマ登録
	@note  使用するタイマを登録する
	@param TaskId タスクID
	@param Mode   動作モード
	@param EventId イベントID(時間経過で通知するイベントIDをセット)
	@return エラーコード
	@author
	-------------------------------------------------*/
	TYPE_ERROR Register(const ENUM_TIMER_MODE Mode, const uint32_t EventId)
	{
		//イベントIDが不正(各タスク別に定義されるのでココでは完全に不正チェックができない)
		if (0 == EventId)
		{
			return E_PAR;
		}

		//登録可能数を超えた
		if (_indexTimer >= _maxTimer)
		{
			return E_PAR;
		}

		//登録済み
		for (uint32_t i = 0; i < _indexTimer; i++)
		{
			if (_pTimer[i].EventId == EventId)
			{
				return E_REGISTERED;
			}
		}
		DEFINE_INT();
		DISABLE_INT();		//割り込み禁止
		_pTimer[_indexTimer].EventId = EventId;
		_pTimer[_indexTimer].StopFlag = true;
		_pTimer[_indexTimer].Mode = Mode;
		_pTimer[_indexTimer].Msec = 0;
		_pTimer[_indexTimer].Counter = 0;
		_pTimer[_indexTimer].IsUsed = true;
		_indexTimer++;
		ENABLE_INT();		//割り込み許可

		return E_OK;
	}

	/*!------------------------------------------------
	@brief タイマ開始
	@note  使用するタイマ動作を開始する
	@param TaskId   タスクID
	@param Msec   タイマー間隔(TIMER_INTERVAL_MSECの倍数を指定すること)
	@return なし
	@author
	-------------------------------------------------*/
	void Start(const uint32_t EventId, const uint32_t Msec)
	{
		//登録なし
		if (0 == _indexTimer)
		{
			return;
		}
		//イベントIDが不正(各タスク別に定義されるのでココでは完全に不正チェックができない)
		if (0 == EventId)
		{
			return;
		}
		this->Stop(EventId);
		for (uint32_t i = 0; i < _indexTimer; i++)
		{
			if (_pTimer[i].EventId == EventId)
			{
			    DEFINE_INT();
				DISABLE_INT();		//割り込み禁止

				_pTimer[i].Counter = Msec / TIMER_INTERVAL_MSEC;
				_pTimer[i].Msec = Msec / TIMER_INTERVAL_MSEC;
				_pTimer[i].StopFlag = false;

				ENABLE_INT();		//割り込み許可
				break;
			}
		}
	}

	/*!------------------------------------------------
	@brief タイマ強制停止
	@note  使用するタイマ動作を開始する
	@note  (最大ベースとなるタイマ割り込み周期分の遅れが発生するので注意)
	@param Id   タイマーID
	@return なし
	@author
	-------------------------------------------------*/
	void Stop(const uint32_t EventId)
	{
		//登録なし
		if (0 == _indexTimer)
		{
			return;
		}
		//イベントIDが不正(各タスク別に定義されるのでココでは完全に不正チェックができない)
		if (0 == EventId)
			return;

		for (uint32_t i = 0; i < _indexTimer; i++)
		{
			if (EventId == _pTimer[i].EventId)
			{
			    DEFINE_INT();
				DISABLE_INT();		//割り込み禁止

				_pTimer[i].StopFlag = true;

				ENABLE_INT();		//割り込み許可
				break;
			}
		}

		return;

	}

	/*!------------------------------------------------
	@brief タイマ停止中確認
	@note  
	@param Id   タイマーID
	@return true:動作中 false:停止中
	@author
	-------------------------------------------------*/
	bool IsStopped(const uint32_t EventId)
	{
		bool isRunning = false;

		if ((_indexTimer > 0) && (EventId > 0))
		{
			for (uint32_t i = 0; i < _indexTimer; i++)
			{
				if (EventId == _pTimer[i].EventId)
				{
					DEFINE_INT();
					DISABLE_INT();		//割り込み禁止

					isRunning = _pTimer[i].StopFlag;

					ENABLE_INT();		//割り込み許可
					break;
				}
			}
		}

		return isRunning;
	}

	/*!------------------------------------------------
	@brief タイマカウント処理
	@note  タイマ割り込みハンドラで呼び出すこと
	@param　なし
	@return なし
	@author
	-------------------------------------------------*/
	void Count(void)
	{
		_tickTimer++;
		for (uint32_t i = 0; i < _indexTimer; i++) {
			//有効なタイマーを対象とする
			if (false == _pTimer[i].StopFlag) 
			{

				//カウントダウン
				if (0 < _pTimer[i].Counter)
				{
					_pTimer[i].Counter--;
				}

				//指定時間経過した
				if (0 == _pTimer[i].Counter && 0 != _pTimer[i].Msec)
				{
					
					//時間経過をイベントで通知
					if (0 != _pTimer[i].EventId)
					{
						//エラー無視
						_pTask.SendEvent(_pTimer[i].EventId, NULL, 0);
					}
					
					//カウンタリセット
					_pTimer[i].Counter = _pTimer[i].Msec;

					//１回動作のみの場合は動作停止
					if (TIMER_ONESHOT == _pTimer[i].Mode)
					{
						_pTimer[i].StopFlag = true;
					}
				}
			}
		}
	}

	/*!------------------------------------------------
	@brief カウンタ値取得
	@note  
	@param　なし
	@return システム起動時からのカウンタ値
	@author
	-------------------------------------------------*/
	uint32_t GetTick(void)
	{
		return _tickTimer;
	}
};


#endif	/* TIMER_H */
