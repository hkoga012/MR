#マルチリーダー更新用MOTファイル、量産用ROMイメージ作成スクリプト
#
#バージョンファイルよりメジャー、マイナー、SVNリビジョンを取得して出力ファイル名に追記する
#このスクリプトはe2studioよりビルド完了後に呼び出される
#
#※Bootloader.binはmcubootをマイコンに書き込んだ後、00000000 - 0x00003800のデータを
#  E2Liteで読み出して作成し、予めDebugフォルダにコピーしておくこと
#※Bin2MotMr.exeが所定のフォルダに存在していること
#※python 3がPCにインストールされていること
#※Debugフォルダをカレントフォルダとする

import subprocess
import sys

#モトローラ形式ファイル変換ツールパス
#文字列の先頭に'R'を付加しないと\x64の部分がエスケープシーケンスとして判断され、削除される
BIN2MOT = R'..\..\..\Tools\Bin2MotMr\x64\Debug\Bin2MotMr.exe'

#バージョンファイルよりバージョンを取得
f = open('../src/version.h', 'r', encoding='UTF-8')

major = minor = revision = ""
datalist = f.readlines() #1行毎の配列に変換
for data in datalist:    #1行毎に検索し、目的のデータを取得
     data = data.replace(' ','') #スペース削除
     #メジャー番号取得
     if 'MAJOR_VERSION' in data :
          spos = data.find('=')
          epos = data.find(';')
          if spos > 0 :
               major = data[spos+1:epos]
     #マイナー番号取得
     if 'MINER_VERSION' in data :
          spos = data.find('=')
          epos = data.find(';')
          if spos > 0 :
               minor = data[spos+1:epos]
     #SVNリビジョン取得
     if 'SVN_REVISION' in data :
          spos = data.find('"')           #最初の"を検出
          epos = data.find('"',spos+1)    #次の"を検出
          if spos > 0 :
               revision = data[spos+1:epos]
     #リビジョンまで取得できたらループを抜ける
     if major != "" and minor != "" and revision != "" :
          break 
f.close()

#取得バージョン表示
print("Version:" + major + "." + minor + "." + revision)

#作成前に既存ファイル削除
subprocess.run("del /Q *HFS-MR_*.*", stdout=subprocess.PIPE, shell=True)

#--------------------------------------------------------------------
#ファームウェエア更新用モトローラ形式ファームウェアファイル作成
#対象ツール：HFS-MR_CommTool.exe(メンテツール)
#ファイル名：HFS-MR_Ver99.99.9999.mot (9の部分は取得した数値 0サプレス)
#開始アドレス：0xFFFE0000 (従来仕様に合わせるための開始アドレス)
#出力ROMサイズ：0x20000　(128Kbyte)
#--------------------------------------------------------------------
mot_filename = "HFS-MR_Ver" + major.zfill(2) + "." + minor.zfill(2) + "_Rev" + revision.zfill(4) + ".mot"
res = subprocess.run(BIN2MOT + " MultiReader.bin.signed " + mot_filename + " 0xFFFE0000 0x20000")
if 0!=res.returncode:
     print("#### ERROR #### Failed to create Firmware file")
else:
     print("Successfully created Firmware file: " + mot_filename)

#--------------------------------------------------------------------
#量産用ROMイメージ作成
#対象ツール：E2 Lite, Renesas Flash Programmer(https://www.renesas.com/jp/ja/software-tool/renesas-flash-programmer-programming-gui)
#ファイル名：Factory_HFS-MR_Ver99.99.9999.mot (9の部分は取得した数値 0サプレス)
#開始アドレス：0x00000000 (ROM開始アドレス)
#出力ROMサイズ：0x21800　 (134Kbyte = 120Kbyte(MultiReader.bin.signed) + 14Kbyte(Bootloader.bin))
#--------------------------------------------------------------------
#Bootloader.bin の後ろに MultiReader.bin.signedを追加し、ROMイメージ作成
img_filename = "HFS-MR_Image.bin"
res = subprocess.run("copy /y /b Bootloader.bin + MultiReader.bin.signed " + img_filename, stdout=subprocess.PIPE, shell=True)
if 0!=res.returncode:
     print("#### ERROR #### Failed to create ROM Image file(bin)")
else:
     #結合したバイナリをモトローラ形式へ変換
     imgmot_filename = "Factory_HFS-MR_Ver" + major.zfill(2) + "." + minor.zfill(2) + "_Rev" + revision.zfill(4) + ".mot"
     res = subprocess.run(BIN2MOT + " " + img_filename + " " + imgmot_filename + " 0x00000000 0x21800")
     if 0!=res.returncode:
          print("#### ERROR #### Failed to create ROM Image file(mot)")
     else:
          print("Successfully created ROM Image file: " + imgmot_filename)
