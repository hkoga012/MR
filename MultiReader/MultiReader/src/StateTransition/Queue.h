/*!------------------------------------------------
@brief キュークラスヘッダ
@note 
@author
-------------------------------------------------*/
#ifndef QUEUE_H
#define QUEUE_H
#include "common.h"

template <typename T>
class Queue
{
	//-------------------------------------------
	// キュー構造体定義
	//-------------------------------------------
	struct Node {
		struct Node* pNext;	//!< 次位置
		T Data;					//!< データ格納先頭ポインタ
	};

	//-------------------------------------------
	// キュー管理構造体定義
	//-------------------------------------------
	struct QueueControl {
		Node* pUsedHead;		//!< 使用中先頭位置
		Node* pUsedTail;		//!< 使用中最後位置
		Node* pFreeHead;		//!< 未使用先頭位置
		Node* pFreeTail;		//!< 未使用最後位置
	};

	Node* _pNode;               //!< ノードバッファ先頭ポインタ(1次元配列)
	uint32_t _max;				//!< 最大キュー数
	uint32_t _count;			//!< 現在格納キュー数
	QueueControl _QueueControl;	//!< キュー制御クラス

public:

	/*!------------------------------------------------
	@brief コンストラクタ
	@note
	@param Max　　最大キューバッファ数
	@return
	@author
	-------------------------------------------------*/
	Queue(uint32_t Max) : _pNode(new Node[Max]), _max(Max), _count(0)
	{
		T defaultData;
		memset((uint8_t*)&defaultData, 0, sizeof(T));
		Node* pNode = &_pNode[0];
		//先頭位置格納
		_QueueControl.pFreeHead = pNode;

		//先頭を初期化
		pNode->pNext = NULL;
		pNode->Data = defaultData;

		//リンクの初期化
		if (Max >= 2)
		{
			pNode->pNext = (pNode + 1);
			pNode++;
			for (uint32_t i = 1; i < (Max - 1); i++)
			{
				pNode->pNext = (pNode + 1);
				pNode->Data = defaultData;
				pNode++;
			}
			//最後
			pNode->pNext = NULL;
			pNode->Data = defaultData;
		}
		else {
			//キューが1つのみ
		}

		_QueueControl.pFreeTail = pNode;
		_QueueControl.pUsedHead = NULL;
		_QueueControl.pUsedTail = NULL;

	}

	/*!------------------------------------------------
	@brief キュー満状態確認
	@note
	@param なし
	@return　true:満、false:空きあり
	@author
	-------------------------------------------------*/
	bool IsFull(void)
	{
		return _max == _count ? true : false;
	}

	/*!------------------------------------------------
	@brief キューデータ存在確認
	@note
	@param なし
	@return　true:データあり、false:データなし
	@author
	-------------------------------------------------*/
	bool IsExist(void)
	{
		bool flag = false;
		DEFINE_INT();
		DISABLE_INT();		//割り込み禁止
		if (NULL != _QueueControl.pUsedHead)
		{
			flag = true;
		}
		ENABLE_INT();		//割り込み許可

		return flag;
	}

	/*!------------------------------------------------
	@brief 現在のキュー格納数を返す
	@note
	@param なし
	@return　現在のキュー格納数
	@author
	-------------------------------------------------*/
	uint32_t Count(void)
	{
		return _count;
	}

	/*!------------------------------------------------
	@brief キューにデータ追加
	@note
	@param Data: 追加するデータ(クラスや構造体も指定可)
	@return　エラーコード
	@author
	-------------------------------------------------*/
	TYPE_ERROR Add(T Data)
	{
		if (NULL == _QueueControl.pFreeHead)
		{
			//キュー満
			return E_FULL;
		}
		DEFINE_INT();
		DISABLE_INT();		//割り込み禁止
		//------------
		//未使用→使用へ移動
		//------------
		//未使用の先頭を取得し、使用データの最後尾に追加(データポインタ設定)
		if (NULL == _QueueControl.pUsedHead)
		{
			//使用データが無い場合は先頭に追加
			_QueueControl.pUsedHead = _QueueControl.pFreeHead;
			_QueueControl.pUsedTail = _QueueControl.pFreeHead;
		}
		else {
			//使用データが存在する場合は最後に追加
			_QueueControl.pUsedTail->pNext = _QueueControl.pFreeHead;
			_QueueControl.pUsedTail = _QueueControl.pUsedTail->pNext;
		}

		//データ格納ポインタ設定
		_QueueControl.pUsedTail->Data = Data;

		//------------
		//未使用格納位置を移動
		//------------
		if (NULL == _QueueControl.pFreeHead->pNext)
		{
			//未使用が無い場合はNULL設定
			_QueueControl.pFreeHead = NULL;
			_QueueControl.pFreeTail = NULL;
		}
		else {
			//未使用が存在する場合は次を先頭に移動する
			_QueueControl.pFreeHead = _QueueControl.pFreeHead->pNext;
		}
		_QueueControl.pUsedTail->pNext = NULL;

		//キュー数インクリメント
		if (_count < _max)
		{
			_count++;
		}

		ENABLE_INT();		//割り込み許可

		return E_OK;
	}

	/*!------------------------------------------------
	@brief キュー格納データポインタ取得
	@note
	@param pData: 取得するデータの先頭ポインタ(クラスや構造体も指定可)
	@return　エラーコード
	@author
	-------------------------------------------------*/
	TYPE_ERROR Get(T* pData)
	{
		//カレントがNULLならデータ無し
		if (NULL == _QueueControl.pUsedHead)
		{
			return E_EMP;
		}
		DEFINE_INT();
		DISABLE_INT();		//割り込み禁止

		//データ格納データセット
		*pData = _QueueControl.pUsedHead->Data;

		//------------
		//使用→未使用へ移動
		//------------
		if (NULL == _QueueControl.pFreeHead)
		{
			//未使用が存在しない場合は先頭にセット
			_QueueControl.pFreeHead = _QueueControl.pUsedHead;
			_QueueControl.pFreeTail = _QueueControl.pUsedHead;

		}
		else {
			//未使用の最後尾に追加
			_QueueControl.pFreeTail->pNext = _QueueControl.pUsedHead;
			_QueueControl.pFreeTail = _QueueControl.pFreeTail->pNext;
		}

		//------------
		//使用データ位置移動
		//------------
		//現在の先頭データをリンクリストから外す
		if (NULL == _QueueControl.pUsedHead->pNext)
		{
			//最後なら現在位置のポインタをNULL設定
			_QueueControl.pUsedHead = NULL;
			_QueueControl.pUsedTail = NULL;
		}
		else {
			//次がある場合は次の位置を先頭にセットする
			_QueueControl.pUsedHead = _QueueControl.pUsedHead->pNext;
		}

		_QueueControl.pFreeTail->pNext = NULL;
		memset((uint8_t*)&_QueueControl.pFreeTail->Data, 0, sizeof(T));

		//キュー数デクリメント
		if (_count > 0)
		{
			_count--;
		}

		ENABLE_INT();		//割り込み許可

		return E_OK;
	}
};


#endif
