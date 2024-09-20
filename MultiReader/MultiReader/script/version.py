#バージョンファイル更新スクリプト
#
#※python 3がPCにインストールされていること
#※TortoiseSVNがPCにインストールされていること
#2024.01.15 AutoUpdateRev.batの処理を直接呼び出すよう変更(AutoUpdateRev.batを不要にした)

import subprocess
import sys

#バージョン反映バッチパス
AUTOUPDATEREV = R'SubWCRev ..\..\..\ ..\src\version.template ..\src\version.h'

#バージョンファイルを作成前に削除
#バージョンファイル作成失敗時にビルドエラーを発生させてユーザーに知らせる為
subprocess.run(R'del /Q ..\src\version.h', stdout=subprocess.PIPE, shell=True)

#最新バージョンを反映
res = subprocess.run(AUTOUPDATEREV, stdout=subprocess.PIPE, shell=True)
if 0!=res.returncode:
     print("#### ERROR #### Failed to update version.h file")
else:
     print("Successfully updated version.h file ")
