/*!------------------------------------------------
@brief リングバッファクラスヘッダ
@note 
@author
-------------------------------------------------*/
#ifndef RING_BUFFER_H
#define RING_BUFFER_H
#include "common.h"

//リングバッファ書込み動作
enum ENUM_RING_WRITE {
	RING_W_NORMAL = 0,		//!< データを上書きしない
	RING_W_OVERWRITE,		//!< データを上書き
};
	
template <typename T>
class RingBuffer
{
	//-------------------------------------------
	// 構造体定義
	//-------------------------------------------
	struct Node {
		struct Node* pNext;	//!< 次位置
		T Data;					//!< データ格納先頭ポインタ
	};

	//-------------------------------------------
	// 管理構造体定義
	//-------------------------------------------
	struct RingControl {
		Node* pReadStart;	//!< 読込開始位置
		Node* pRead;		//!< 読出し位置
		Node* pWrite;		//!< 書込み位置
	};
	
	Node* _pNode;               //!< ノードバッファ先頭ポインタ(1次元配列)
	uint32_t _max;				//!< 最大キュー数
	uint32_t _count;			//!< 現在格納キュー数
	ENUM_RING_WRITE _WriteOption;       //!< 書込オプション
	RingControl _RingControl;	//!< リングバッファ制御クラス

public:
	
	/*!------------------------------------------------
	@brief コンストラクタ
	@note
	@param Max　　最大キューバッファ数
	@param WriteOption　　書込み動作
	@return
	@author
	-------------------------------------------------*/
	RingBuffer(uint32_t Max, ENUM_RING_WRITE WriteOption) 
		: _pNode(new Node[Max]), _max(Max), _count(0), _WriteOption(WriteOption)
	{
		T defaultData;
		
		memset((uint8_t*)&defaultData, 0, sizeof(T));
		Node* pNode = &_pNode[0];
		
		//先頭位置格納
		_RingControl.pReadStart = pNode;
		_RingControl.pRead = pNode;
		_RingControl.pWrite = pNode;

		//先頭を初期化
		pNode->pNext = pNode;
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
			//最後の次は先頭ポインタをセット
			pNode->pNext = &_pNode[0];
			pNode->Data = defaultData;
		}
		else {
			//キューが1つのみ
		}

	}
	
	/*!------------------------------------------------
	@brief データ満状態確認
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
	@brief データ存在確認
	@note
	@param なし
	@return　true:データあり、false:データなし
	@author
	-------------------------------------------------*/
	bool IsExist(void)
	{
		return (_count > 0);
	}

	/*!------------------------------------------------
	@brief 現在のデータ格納数を返す
	@note
	@param なし
	@return　現在の格納数
	@author
	-------------------------------------------------*/
	uint32_t Count(void)
	{
		return _count;
	}

	/*!------------------------------------------------
	@brief データ追加
	@note
	@param Data: 追加するデータ(クラスや構造体も指定可)
	@return　エラーコード
	@author
	-------------------------------------------------*/
	TYPE_ERROR Add(T Data)
	{
		//通常モードではデータ満は書き込まない
		if(_WriteOption == RING_W_NORMAL && _count == _max)
		{
			return E_FULL;
		}
		
		DEFINE_INT();
		DISABLE_INT();		//割り込み禁止
		
		//データ格納ポインタ設定
		_RingControl.pWrite->Data = Data;
		
		//上書きモード
		if(_WriteOption == RING_W_OVERWRITE)
		{
			if(_RingControl.pWrite == _RingControl.pReadStart && _count == _max)
			{
				//1周したらRead位置をWrite位置(最も古いデータ)に変更する
				_RingControl.pReadStart = _RingControl.pWrite->pNext;
				_RingControl.pRead = _RingControl.pReadStart;
			}
		}
		
		//次に移動
		_RingControl.pWrite = _RingControl.pWrite->pNext;
		
		if (_count < _max)
		{
			_count++;
		}
		
		ENABLE_INT();		//割り込み許可

		return E_OK;
	}

	/*!------------------------------------------------
	@brief 格納データ取得(データ残す)
	@note  データ消去Getと併用不可
	@param Offset: 読出し位置
	@return　エラーコード
	@author
	-------------------------------------------------*/
	const T Get(uint32_t Offset)
	{
		Node* p = NULL;
		T Data;

		memset((uint8_t*)&Data, 0, sizeof(T));
		//データなし
		if ( 0 == _count )
		{
			return Data;
		}
		//オフセット不正
		if (Offset >= _count)
		{
			return Data;
		}
		DEFINE_INT();
		DISABLE_INT();		//割り込み禁止

		p = _RingControl.pReadStart;
		for (uint32_t i=0; i < Offset; i++)
		{
			p = p->pNext;
		}
		
		ENABLE_INT();		//割り込み許可

		return  p->Data;
	}

	/*!------------------------------------------------
	@brief 格納データ取得(データ消去)
	@note　データ残すGetと併用不可
	@param なし
	@return　エラーコード
	@author
	-------------------------------------------------*/
	const T Get(void)
	{
		T Data;

		memset((uint8_t*)&Data, 0, sizeof(T));

		//データなし
		if (0 == _count)
		{
			return Data;
		}
		DEFINE_INT();
		DISABLE_INT();		//割り込み禁止

		//データ格納データセット
		Data = _RingControl.pRead->Data;

		//次に移動
		_RingControl.pRead = _RingControl.pRead->pNext;

		//デクリメント
		if (_count > 0)
		{
			_count--;
		}
		ENABLE_INT();		//割り込み許可

		return Data;
	}
};


#endif
