/*!------------------------------------------------
@brief デバッグオプション定義ヘッダ
@note
@author
-------------------------------------------------*/
#ifndef DEBUG_OPTION_H
#define DEBUG_OPTION_H

//*******************************************************************
//////////////////////////////// 注意 ///////////////////////////////
// リリース時は必ず全てコメントになっていることを確認後ビルドすること
//*******************************************************************

//#define DEBUG_NO_NFC  // NFC無しでデバッグ
//#define DEBUG_NO_COLLISION //衝突検知なしでデバッグ

//#define DEBUG_NFC_POWER_DIPSW //　DIPSWでアンテナを調整する ... 一緒に評価用にカード検出時にビープ音を鳴らす（通常のビープ音出力機能は止める ）
//#define DEBUG_NFC_RF_CONTINUOUS_OUTPUT // NFC RF連続出力(カード読取り不可) ... 【注意】直下ifdef 4行を含め有効にすること
//#ifdef DEBUG_NFC_RF_CONTINUOUS_OUTPUT
//#define DEBUG_DUMMY_DIPSW /* RF出力時はDIPSWの設定を電圧、電流設定とするためダミー読取り値を設定する */
//#define SET_DUMMY_DIPSW() { _DipSw1Info = 0x16; _DipSw2Info = 0x01; }   //Felica FTSic + Idm 　/ MIFARE 2ブロック、QRなし
//#endif /* DEBUG_NFC_RF_CONTINUOUS_OUTPUT */

//#define DEBUG_DUMMY_DIPSW   // // DIPSW1,2のHWが実装されていない環境(Windows上含む)で、以下のSET_DUMMY_DIPSWマクロ関数を使ってダミー設定する
//#define SET_DUMMY_DIPSW() { _DipSw1Info = 0x3d; _DipSw2Info = 0x01; } // DIPSW2 アドレス1、DIPSW1(機能選択）Felica FTSic　/ MIFARE 2ブロック
//#define SET_DUMMY_DIPSW() { _DipSw1Info = 0x30; _DipSw2Info = 0x01; } // DIPSW2 アドレス1、DIPSW1(機能選択）NFC 読み取り無し、QRなし
//#define SET_DUMMY_DIPSW() { _DipSw1Info = 0x31; _DipSw2Info = 0x01; } // DIPSW2 アドレス1、DIPSW1(機能選択）NFC 読み取り無し、QRあり
//#define SET_DUMMY_DIPSW() { _DipSw1Info = 0x00; _DipSw2Info = 0x01; } // DIPSW2 アドレス1、DIPSW1(機能選択）NFC イニシャル指定、QRなし
//#define SET_DUMMY_DIPSW() { _DipSw1Info = 0x01; _DipSw2Info = 0x01; } // DIPSW2 アドレス1、DIPSW1(機能選択）NFC イニシャル指定、QRあり


//#define NFC_DEBUG_PRINT_ENABLE //[NfcReader]NFC読込データをデバッグポートに出力
//#define DEBUG_DFLASH_WRITE_INTERVAL_SHORTEN  // [DataManager]データFlash更新 加速評価
//#define DEBUG_NOT_USE_OTHER_THAN_LED4   // LED4以外使わない

//#define USE_PRINT_DEBUG_TIME  // 時間計測結果をデバッグコンソールに出力する
//#define USE_PRINT_DEBUG_FER  // フレーミングエラー累積発生回数を送信の度にデバッグコンソールに出力する
//#define USE_PRINT_DEBUG_DATAMANAGER   // デバックモニタから DataManager::PrintInfo() を出力可能にする
//#define NFC_SWITCHING_TARGET_SYNC_P609  //カード検出切替とP609出力を同期(P609:HIGH→Felica,LOW→Mifare)、MifareORFelica単体読取の場合はP609:Switching
//#define NFC_SWITCHING_RF_SYNC_P303  //RF無効/有効をP303に同期させる。(P303:HIGH→有効LOW→無効)


///////////////////////
// エラー強制発生定義
// 有効にする場合は1つずつ有効にすること(同時に複数有効禁止)
///////////////////////
//メモリデータ　累積回数情報
//#define NFC_ERROR_DEBUG_RW_INIT			//初期設定中
//#define NFC_ERROR_DEBUG_RW_POL			//ポーリング中:カード捕捉
//#define NFC_ERROR_DEBUG_RW_SELECT			//カードセレクト中
//#define NFC_ERROR_DEBUG_RW_POL2			//ポーリング中:共通領域
//#define NFC_ERROR_DEBUG_RW_DATA			//FeliCa読出し5回エラー
//#define NFC_ERROR_DEBUG_RW_POL3			//ポーリング中:カードリリース
//#define NFC_ERROR_DEBUG_RW_RELEASE		//カードリリース

//#define NFC_ERROR_DEBUG_RW_M_AUTH1		//MIFARE 1ブロック目認証5回エラー
//#define NFC_ERROR_DEBUG_RW_M_AUTH2		//MIFARE 2ブロック目認証5回エラー
//#define NFC_ERROR_DEBUG_RW_M_READ1		//MIFARE 1ブロック目リード5回エラー
//#define NFC_ERROR_DEBUG_RW_M_READ2		//MIFARE 2ブロック目リード5回エラー

//#define NFC_ERROR_DEBUG_RW_RESET			//リセットポート出力異常

//#define NFC_DEBUG_REPLACE_CHAR			//NFCリーダー不正文字置換確認

//[QrReader]
//#define QR_ERROR_DEBUG_QR_INIT			//QRリーダー初期化異常検出(タイムアウト)
//#define QR_ERROR_DEBUG_QR_OVER			//QRリーダー通信オーバーラン検出回数
//#define QR_ERROR_DEBUG_QR_FER				//QRリーダー通信フレーミングエラー検出回数
//#define QR_DEBUG_REPLACE_CHAR				//QRリーダー不正文字置換確認

//[OutputControl]
//#define OUTPUT_ERROR_DEBUG_RELAY1			//リレー出力１ポート異常

//[RS485]
//#define RS485_ERROR_DEBUG_PORT_485_DE_A		//RS485 Primary DEポート出力異常
//#define RS485_ERROR_DEBUG_PORT_485_RE_A		//RS485 Primary REポート出力異常
//#define RS485_ERROR_DEBUG_PORT_485_FET		//RS485 FETポート出力異常
//#define RS485_ERROR_DEBUG_HOST_OVER			//RS485 上位通信受信オーバーランエラー
//#define RS485_ERROR_DEBUG_HOST_FERR			//RS485 上位通信受信フレーミングエラー
//#define RS485_ERROR_DEBUG_HOST_COLLISION		//RS485 上位通信衝突リトライオーバーエラー
//#define RS485_ERROR_DEBUG_RECOVERY_FROM_COLLISION		//RS485 上位通信衝突復旧

//[EEPROM]
//#define EEPROM_ERROR_DEBUG_EEPROM_NORMALITY   // 仮想EEPROM(Data Flash)異常
//#define DFLASH_DRV_DEBUG_FORCED_STOP          // 書き込み中の電源断評価用（ドライバの書込処理途中で強制リセット）

#endif
