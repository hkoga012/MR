/*!------------------------------------------------
@brief 致命的Exception(システムエラー)クラス
@note 各タスクに実装するイベントハンドラで THROW_FATAL_ERROR マクロ関数を実行することで 
      FatalExeption を throw します。（ROMに記録し再起動）
@param
@return
@author
-------------------------------------------------*/
#ifndef FATAL_EXCEPTION_H
#define FATAL_EXCEPTION_H

// ### 2023/12/01 RA環境のRAM消費抑制の為、throw-catch を使わない形で実装することにした。###

// #include <exception>
#include <string.h>
#include <stdint.h>

extern "C" void Throw_FatalExceltionEvent(const char* pFilePath, uint32_t line, const char* pTaskName, const char* pMessage);
// hal_entry.cpp で実装する

////!< 関数マクロ 致命的ExceptionをThrowする
#define THROW_FATAL_ERROR(taskName, message) Throw_FatalExceltionEvent(__FILE__, __LINE__, taskName, message)
#define THROW_FATAL_ERROR_ID(taskName) Throw_FatalExceltionEvent(__FILE__, __LINE__, taskName, "Id is invalid.")
#define THROW_FATAL_ERROR_STATUS(taskName) Throw_FatalExceltionEvent(__FILE__, __LINE__, taskName, "Status is invalid.")
#define THROW_FATAL_ERROR_PAR(taskName) Throw_FatalExceltionEvent(__FILE__, __LINE__, taskName, "Parameter error.")
#define THROW_FATAL_ERROR_QUE_FULL(taskName) Throw_FatalExceltionEvent(__FILE__, __LINE__, taskName, "Queue is full.")
#define THROW_FATAL_ERROR_PROGRAM_LOGIC(taskName) Throw_FatalExceltionEvent(__FILE__, __LINE__, taskName, "Program logic error.")


// ### これまでの FatalException 実装を止める ###
//
// 
//
////!< 関数マクロ 致命的ExceptionをThrowする
//#define THROW_FATAL_ERROR(taskName, message) throw FatalException(__FILE__, __LINE__, taskName, message)
//#define THROW_FATAL_ERROR_ID(taskName) throw FatalException(__FILE__, __LINE__, taskName, "Id is invalid.")
//#define THROW_FATAL_ERROR_STATUS(taskName) throw FatalException(__FILE__, __LINE__, taskName, "Status is invalid.")
//#define THROW_FATAL_ERROR_PAR(taskName) throw FatalException(__FILE__, __LINE__, taskName, "Parameter error.")
//#define THROW_FATAL_ERROR_QUE_FULL(taskName) throw FatalException(__FILE__, __LINE__, taskName, "Queue is full.")
//#define THROW_FATAL_ERROR_PROGRAM_LOGIC(taskName) throw FatalException(__FILE__, __LINE__, taskName, "Program logic error.")
//
//
//class FatalException : public std::exception
//{
//public:
//	/*!------------------------------------------------
//	@brief コンストラクタ
//	@note
//	@param pFilePath 発生ファイルパス (__FILE__ を指定)
//	@param line 発生ライン（__LINE__ を指定）
//	@param pTaskName タスク名（固定文字列を定義要）
//	@param pMessage 発生エラー説明（固定文字列を定義要）
//	@author
//	-------------------------------------------------*/
//	FatalException(const char* pFilePath, uint32_t line, const char *pTaskName, const char* pMessage)
//		: _pFilePath(pFilePath), _Line(line), _pTaskName(pTaskName), _pMessage(pMessage)
//	{
//	}
//
//	/*!------------------------------------------------
//	@brief 発生ファイル名を取得する
//	@note
//	@return 発生ファイル名
//	@author
//	-------------------------------------------------*/
//	const char* GetFileName()
//	{
//		const char* pLastSeparator = strrchr(_pFilePath, '\\');
//		return (pLastSeparator == NULL) ? _pFilePath : pLastSeparator + 1;
//	}
//
//	/*!------------------------------------------------
//	@brief 発生行数を取得する
//	@note
//	@return 発生行数
//	@author
//	-------------------------------------------------*/
//	uint32_t GetLine()
//	{
//		return _Line;
//	}
//
//	/*!------------------------------------------------
//	@brief 発生エラーの説明を取得する
//	@note
//	@return 発生エラー説明文字列
//	@author
//	-------------------------------------------------*/
//	const char* GetMessage()
//	{
//		return _pMessage;
//	}
//
//private:
//	const char* _pFilePath;		//!> 発生ファイルパス
//	const uint32_t _Line;		//!> 発生行数
//	const char* _pTaskName;		//!> 対象タスク名
//	const char* _pMessage;		//!> 発生エラーの説明
//};

#endif
