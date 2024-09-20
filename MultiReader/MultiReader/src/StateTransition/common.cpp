/***********************************************************************/
/*  作成日      :2017.07.31	                                   	       */
/*  処理        :共通処理                        				       */
/*  備考		:共通処理を定義する                               	   */
/***********************************************************************/
#include "common.h"

//-------------------------------------------
// 数値をASCII-HEXに変換
// Val:変換対象値
// pHexH:変換後文字格納ポインタ　上位
// pHexL:変換後文字格納ポインタ　下位
//-------------------------------------------
void ClsCommon::I2HexAsciiByte(uint8_t Val, char* pHexH, char* pHexL)
{
	uint8_t n;

	n = (Val >> 4) & 0x0F;
	if (n <= 9) {
		*pHexH = '0' + (char)n;
	}
	else {
		*pHexH = 'A' + (char)(n - 10);
	}

	n = (Val) & 0x0F;
	if (n <= 9) {
		*pHexL = '0' + (char)n;
	}
	else {
		*pHexL = 'A' + (char)(n - 10);
	}

}

//-------------------------------------------
// 16進数の2文字から数値を返す
// HexH :変換対象文字　上位
// HexL :変換対象文字　下位
// pBuff:変換値格納バッファ先頭ポインタ
//-------------------------------------------
bool ClsCommon::AsciiHex2Int(const char HexH, char HexL, uint8_t* pBuff)
{
	uint8_t cnv = 0;

	if ('0' <= HexH && HexH <= '9')
	{
		cnv = (uint8_t)(HexH - '0');
	}
	else if ('A' <= HexH && HexH <= 'F')
	{
		cnv = (10 + (uint8_t)(HexH - 'A'));
	}
	else
	{
		return false;	//異常文字
	}
	cnv = (uint8_t)((cnv << 4) & 0xF0);

	if ('0' <= HexL && HexL <= '9')
	{
		cnv |= (uint8_t)(HexL - '0');
	}
	else if ('A' <= HexL && HexL <= 'F')
	{
		cnv |= (uint8_t)(10 + (HexL - 'A'));
	}
	else
	{
		return false;	//異常文字
	}
	*pBuff = cnv;

	return true;
}

//-------------------------------------------
// ASCII 16進数ををバイナリに変換
// 1バイト分の16進数文字は必ず2桁であること
// pAscii :変換対象16進文字列
// Len    :変換対象文字列数
// pBinary:変換値格納バッファ先頭ポインタ
// Size   :変換値格納バッファサイズ(2147483647以下を指定すること)
//-------------------------------------------
int32_t ClsCommon::AsciiHex2Bin(const char* pAscii, uint32_t Len, uint8_t* pBinary, uint32_t Size)
{
	uint32_t i, cnt, convSize;
	bool ret;

	convSize = 0;
	//文字列数が奇数の場合はエラー
	if (Len % 2 == 1)
	{
		return -1;
	}
	cnt = Len / 2;
	for (i = 0;i < cnt;i++)
	{
		ret = AsciiHex2Int(*pAscii, *(pAscii + 1), pBinary);

		if (false == ret)
		{
			return -2;	//変換できない文字が存在
		}
		pAscii += 2;
		pBinary++;
		convSize++;
		if (convSize > Size)
		{
			//バッファサイズを超えた
			return -3;
		}
	}
	return convSize;
}

/// 2023/11/02 使わない方向で削除 DebugPrintDataHex関数を使う（折角作ったので暫くコメント化）
//	 
//	 /*! ---------------------------------------------------------------------
//	 @brief バイナリ配列を ASCII16進数文字列(\0終端付き)で出力
//	 @note 1バイト分の16進数文字は2桁で出力される
//	 @param pAscii  :変換対象バイナリデータ格納バッファ先頭ポインタ
//	 @param Len     :変換対象バイナリデータサイズ
//	 @param pBinary :変換後に格納する16進文字列バッファポインタ
//	 @param Size    :変換後に格納するバッファ長（\0を含む）
//	 @return 変換した文字列長
//	 @author
//	  --------------------------------------------------------------------- */
//	 int32_t ClsCommon::Bin2AsciiHex(const uint8_t* pBinary, uint32_t Len, char* pAscii, uint32_t Size)
//	 {
//	 	uint32_t i, convSize;
//	 
//	 	convSize = 0;
//	 	for (i = 0; i < Len; i++)
//	 	{
//	 		if (convSize >= Size - 2 - 1) 
//	 		{
//	 			// まだ1Byte分の16進数と\0を入れるサイズが残っているか確認
//	 			// 残っていなければ「バッファサイズを超えた」
//	 			return -3;
//	 		}
//	 
//	 		I2HexAsciiByte(*(pBinary + i), pAscii, pAscii + 1);
//	 		pAscii += 2;
//	 		pBinary++;
//	 		convSize+=2;
//	 	}
//	 
//	 	*pAscii = 0; // \0
//	 
//	 	return convSize;
//	 }

//-------------------------------------------
// 数値を文字列に変換(変換は10桁(4294967295)まで、マイナス対応、10進数のみ)
// n:変換対象の数値
// pStr:変換文字列格納先頭ポインタ
// Size:変換文字列格納バッファのサイズ
//-------------------------------------------
uint32_t ClsCommon::ITOA(int32_t n, char* pStr, uint32_t Size)
{
	uint8_t Digi;
	char temp[12];
	char* p = &temp[0];
	uint32_t val = (uint32_t)(n < 0 ? n * -1 : n);
	uint32_t len, i;

	len = 0;

	do {
		Digi = (uint8_t)(val % 10);
		*p = (char)(Digi + '0');
		val /= 10;
		p++;
		len++;
		if (len > Size)
		{
			return 0;	//バッファサイズを超えた
		}
	} while (val > 0);

	//マイナス対応
	if (n < 0)
	{
		*pStr = '-';
		pStr++;
		if (len > Size)
		{
			return 0;	//バッファサイズを超えた
		}
	}

	//並びを逆にする
	for (i = len;i > 0;i--)
	{
		*pStr = temp[i - 1];
		pStr++;
	}
	//終端
	*pStr = '\0';
	if (n < 0)
	{
		len++;
	}
	return len;
}

//-------------------------------------------
// 文字列を数値に変換(変換は10桁(4294967295)まで、マイナス未対応、10進数のみ)
// マイナス値が帰った場合はエラー
// str pStr:変換対象数字文字列
// len Len:変換対象文字列の長さ
// n   pNum:変換後の数値格納ポインタ
//-------------------------------------------
TYPE_ERROR ClsCommon::ATOI(const char* pStr, uint32_t Len, uint32_t* pNum)
{
	uint32_t val;
	uint32_t strLen, i, j;
	uint32_t pow;
	const char* s = pStr;

	//0-9以外はエラー
	for (i = 0;i < Len;i++)
	{
		if ('0' > *s || '9' < *s)
		{
			//マイナス禁止に変更
			return E_PAR;
		}
		s++;
	}
	//5桁以上は不可
	if (Len > 10)
	{
		return E_PAR;
	}
	val = 0;
	strLen = Len;
	for (i = strLen;i > 0;i--)
	{
		pow = 1;
		for (j = 0;j < (i - 1);j++)
		{
			pow *= 10;
		}
		val += (*pStr - '0') * pow;
		pStr++;
	}
	*pNum = (uint32_t)val;

	return E_OK;
}

//-------------------------------------------
// HEX文字列を数値に変換する
// pAscii:変換するHEX文字列の先頭ポインタ
// Len :変換するHEX文字列の長さ（2, 4, 6, 8の何れかを指定）
// pNum:変換後の数値格納ポインタ
//-------------------------------------------
TYPE_ERROR ClsCommon::AsciiHex2Uint32(const char* pStr, uint32_t Len, uint32_t* pNum)
{
    uint32_t val;
    uint32_t strLen, i, j;
    uint32_t pow;
    const char* s = pStr;
    //0-9&&A-F&&a-f以外はエラー
    for (i = 0;i < Len;i++)
    {
        if (( * s <'A' || *s >'F') && ( * s < 'a' || *s >'f')&&(*s < '0' || *s >'9'))
        {
            return E_PAR;
        }
        s++;
    }
    //5桁以上は不可
    if (Len > 8)
    {
        return E_PAR;
    }
    val = 0;
    strLen = Len;
    for (i = strLen;i > 0;i--)
    {
        pow = 1;
        for (j = 0;j < (i - 1);j++)
        {
            pow *= 16;
        }
        if (*pStr>='A'&&*pStr<='F')
        {
            val += (*pStr - '7') * pow;
        }
        else if(*pStr >= 'a' && *pStr <= 'f')
        {
            val += (*pStr - 'W') * pow;
        }
        else
        {
            val += (*pStr - '0') * pow;
        }
        pStr++;
    }
    *pNum = (uint32_t)val;
    return E_OK;
}


//-------------------------------------------
// 数値をゼロサプレス処理した文字列にして返す
// n:文字列に変換する数値
// digit:変換する桁数(不足分を0で埋める)
// out :出力バッファポインタ
// size:出力バッファのサイズ
//-------------------------------------------
int32_t ClsCommon::ZeroSuppress(uint32_t n, uint32_t digit, char* out, uint32_t size)
{
	uint32_t len, strLen;
	char strTemp[11];		//10桁分 + '\0'
	uint32_t i;

	if (digit > 10)
	{
		return 0;
	}

	strLen = 0;
	len = ITOA((int32_t)n, &strTemp[0], sizeof(strTemp));
	if (len > digit)
	{
		return 0;
	}
	for (i = (digit - len);i > 0;i--)
	{
		*out = '0';
		out++;
		strLen++;
	}
	for (i = 0;i < len;i++)
	{
		*out = strTemp[i];
		out++;
		strLen++;
	}
	*out = '\0';

	if (strLen > size)
	{
		return 0;
	}

	return strLen;
}
//-------------------------------------------
// 数値を指定桁の16進文字列に変換する
// マイナス値が帰った場合はエラー
// n     :変換対象値
// Digit  :変換桁数(2,4,6,8のどれか)
// pStrHex:変換値格納ポインタ
//-------------------------------------------
void ClsCommon::I2HexAscii(uint32_t n, uint8_t Digit, char* pStrHex)
{
	uint16_t i, size;
	uint8_t* p = (uint8_t*)&n;

	size = (Digit / 2);
	p += (size - 1);
	for (i = 0;i < size;i++)
	{
		I2HexAsciiByte(*p, pStrHex, (pStrHex + 1));
		p--;
		pStrHex += 2;
	}
	*pStrHex = '\0';
}


/*!------------------------------------------------
@brief  数値をデバッグ出力へ表示する
@note　もともと DebugCommand タスクに実装されていたものに引数を追加し移動 (2023/11/01)
@param funcPrint デバッグ出力関数ポインタ
@param Num 変換対象数値
@return なし
@author
-------------------------------------------------*/
void ClsCommon::DebugPrintNum(void (*funcPrint)(const char*), int32_t Num)
{
	char data[10];
	memset(data, 0, sizeof(data));
	ClsCommon::ITOA((int32_t)Num, &data[0], sizeof(data));
	funcPrint(&data[0]);
}

/*!------------------------------------------------
@brief  数値をデバッグ出力へ16進数表示する
@note　もともと DebugCommand タスクに実装されていたものに引数を追加し移動 (2023/11/01)
@param funcPrint デバッグ出力関数ポインタ
@param Num 変換対象数値
@return なし
@author
-------------------------------------------------*/
void ClsCommon::DebugPrintHex(void (*funcPrint)(const char*), uint32_t Num)
{
	char data[10];
	memset(data, 0, sizeof(data));
	ClsCommon::I2HexAscii(Num, 8, &data[0]);
	funcPrint(&data[0]);
}

/*!------------------------------------------------
@brief  バイナリ配列データをHEXADECIMAL形式に変換して表示
@note　もともと DebugCommand タスクに実装されていたものに引数を追加し移動 (2023/11/01)
@param funcPrint デバッグ出力関数ポインタ
@param pData	 送信データ配列先頭ポインタ
@param Len		 送信データ長
@return なし
@author
-------------------------------------------------*/
void ClsCommon::DebugPrintDataHex(void (*funcPrint)(const char*), const uint8_t* pData, uint32_t Len)
{
	uint32_t i = 0;
	char PrintData[3];

	memset(PrintData, 0, sizeof(PrintData));
	for (i = 0; i < Len; i++)
	{
		if (i == 0)
		{
		}
		else if ((i % 16) == 0)
		{
			funcPrint("\r\n ");
			ClsCommon::I2HexAsciiByte((uint8_t)((i / 16) % 10), &PrintData[0], &PrintData[1]);
			PrintData[2] = 0;
			funcPrint(PrintData);
			funcPrint(") ");
		}
		else
		{
			funcPrint(" ");
		}

		//1バイト毎にHEXDECIMALに変換
		ClsCommon::I2HexAsciiByte(*pData, &PrintData[0], &PrintData[1]);
		pData++;
		PrintData[2] = 0;
		funcPrint(PrintData);
	}
}

/*!------------------------------------------------
@brief  バイナリ配列データをそのままデバッグ出力
@note　
@param funcPrint デバッグ出力関数ポインタ
@param pData	 送信データ配列先頭ポインタ
@param Len		 送信データ長
@return なし
@author
-------------------------------------------------*/
void ClsCommon::DebugPrintBuf(void (*funcPrint)(const char*), const uint8_t* pData, uint32_t Len)
{
	char PrintData[2];
	memset(PrintData, 0, sizeof(PrintData));
	for (uint32_t i = 0; i < Len; i++)
	{
		PrintData[0] = (*pData);
		funcPrint(PrintData);
		pData++;
	}
}

/*!------------------------------------------------
@brief  文字列値を項目名付きでデバッグ出力
@note　
@param funcPrint デバッグ出力関数ポインタ
@param title	 項目名
@param strVal	 値（文字列）
@return なし
@author
-------------------------------------------------*/
void ClsCommon::PrintInfoLine(void (*funcPrint)(const char*), const char* title, const char* strVal)
{
	funcPrint(title);
	funcPrint(strVal);
	funcPrint("\r\n");
}

/*!------------------------------------------------
@brief  バッファのダンプを項目名付きで１６進数デバッグ出力
@note　
@param funcPrint デバッグ出力関数ポインタ
@param title	 項目名
@param pData	 バッファのポインタ
@param Len		 長さ
@return なし
@author
-------------------------------------------------*/
void ClsCommon::PrintInfoDump(void (*funcPrint)(const char*), const char* title, const uint8_t* pData, uint32_t Len)
{
	funcPrint(title);
	DebugPrintDataHex(funcPrint, pData, Len);
	funcPrint("\r\n");
}

/*!------------------------------------------------
@brief  数値を項目名付きでデバッグ出力
@note　
@param funcPrint デバッグ出力関数ポインタ
@param title	 項目名
@param val		 数値
@return なし
@author
-------------------------------------------------*/
void ClsCommon::PrintInfoNum(void (*funcPrint)(const char*), const char* title, uint32_t val)
{
	funcPrint(title);
	DebugPrintNum(funcPrint, val);
	funcPrint("\r\n");
}

/*!------------------------------------------------
@brief  数値を項目名付きでHEX文字列としてデバッグ出力
@note
@param funcPrint デバッグ出力関数ポインタ
@param title     項目名
@param val       数値
@return なし
@author
-------------------------------------------------*/
void ClsCommon::PrintInfoHex(void (*funcPrint)(const char*), const char* title, uint32_t val)
{
    funcPrint(title);
    DebugPrintHex(funcPrint, val);
    funcPrint("\r\n");
}

/*!------------------------------------------------
@brief 送信データ変換
@note　データに0x20-0x7E以外があれば0x20に置換する
@param pData 対象データ格納先頭ポインタ
@param DataLen 対象データバッファ長
@return なし
@author
-------------------------------------------------*/
void ClsCommon::ConvertSendData(uint8_t *pData, uint8_t DataLen)
{
    for(uint8_t i=0; i<DataLen; i++)
    {
        if( 0x20 > *pData || 0x7E < *pData)
        {
            *pData = 0x20;
        }
        pData++;
    }
}
#if 0
/*!
---------------------------------------------------------------------
 @brief 共通バッファ取得
 @note 
 @param size 確保するメモリサイズ
 @return 取得した共有メモリ先頭ポインタ(空きがない場合はNULLを返す)
 @author 時間があればヒープメモリ使用に変更する
 ---------------------------------------------------------------------
 */
uint8_t* ClsCommon::GetBuffer(uint32_t size)
{
	uint16_t i;
	uint8_t *p = NULL;
	
	if( size > COMMON_BUFF_SIZE_MAX )
	{
		//サイズオーバー
		return p;
	}
	
	for(i=0;i<COMMON_BUFF_INDEX_MAX;i++)
	{
		if( ST_FALSE == s_buffer[i].is_used )
		{
			DISABLE_INT();		//割り込み禁止
			//未使用バッファのポインタを取得
			p = &(s_buffer[i].buffer[0]);
			s_buffer[i].is_used = 1;
			ENABLE_INT();		//割り込み許可
			break;
		}
	}
	
	return p;
}

/*!
---------------------------------------------------------------------
 @brief 共通バッファ解放
 @note  引数に渡されたポインタと一致するメモリを解放する
 @param pBuf  get_common_bufferで取得した共有メモリの先頭ポインタ
 @return なし
 @author 
 ---------------------------------------------------------------------
 */
void ClsCommon::ReleaseBuffer(uint8_t *pBuf)
{
	uint16_t i;
	
	for(i=0;i<COMMON_BUFF_INDEX_MAX;i++)
	{
		if( pBuf == &(s_buffer[i].buffer[0]) )
		{
			//アドレス一致で解放
			memset(&(s_buffer[i].buffer[0]), 0, COMMON_BUFF_SIZE_MAX);
			DISABLE_INT();		//割り込み禁止
			s_buffer[i].is_used = 0;
			ENABLE_INT();		//割り込み許可
			break;
		}
	}
}

/*!
---------------------------------------------------------------------
 @brief チェックサム計算
 @note  指定データ、指定サイズのチェックサムを計算し、返す
 		桁あふれしても無視して計算を行う
 @param pData		計算対象データの先頭ポインタ
 @param Len			計算対象データのサイズ
 @return チェックサム計算結果
 @author 
 ---------------------------------------------------------------------
 */
uint8_t ClsCommon::GetChecksum(uint8_t *pData, uint16_t Len)
{
	uint16_t i;
	uint8_t sum = 0;
	for(i=0;i<Len;i++){
		sum ^= *pData;
		pData++;	
	}
	return sum;
}

/*
---------------------------------------------------------------------
 @brief 文字列の最後に指定文字列を追加
 @note  
 @param pStrPos		格納変数追加位置
 @param pStrAdd		追加文字列先頭ポインタ
 @param Len			生成送信データサイズ
 @return 追加した文字列長(格納変数の空サイズを超える場合は処理しない)
 @author 
 ---------------------------------------------------------------------
 */
int16_t ClsCommon::str_add_tail(int8_t *pStrPos, int8_t *pStrAdd, int16_t Len)
{
	memcpy( pStrPos, pStrAdd, Len);
	*(pStrPos + Len) = '\0';	//終端
	return  Len;
}

/*!
---------------------------------------------------------------------
 @brief ビッグエンディアンをリトルエンディアンに変換
 @note  
 @param pBig	ビッグエンディアン値格納先頭ポインタ
 @param pLtl	変換値格納先頭ポインタ
 @param Len		変換値のサイズ(1,2,4のどれか)
 @return なし
 @author 
 ---------------------------------------------------------------------
 */
void ClsCommon::big_to_little(uint8_t *pBig, uint8_t *pLtl, uint8_t Len)
{
	uint16_t i;
	
	for(i=0;i<Len;i++)
	{
		*pLtl = *(pBig + (Len - 1) - i); 	
		pLtl++;
	}
}

/*!
---------------------------------------------------------------------
 @brief リトルエンディアンをビッグエンディアンに変換
 @note  
 @param pLtl	変換値格納先頭ポインタ
 @param pBig	ビッグエンディアン値格納先頭ポインタ
 @param Len		変換値のサイズ(1,2,4のどれか)
 @return なし
 @author 
 ---------------------------------------------------------------------
 */
void ClsCommon::little_to_big(uint8_t *pLtl, uint8_t *pBig, uint8_t Len)
{
	uint16_t i;
	
	for(i=0;i<Len;i++)
	{
		*pBig = *(pLtl + (Len - 1) - i); 
		pBig++;
	}
}
#endif
