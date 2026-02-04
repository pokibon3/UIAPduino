# UIAPduino Sample Programs

UIAP Pro Microで動作するサンプルプログラムです。  
ビルド環境はVSCode + ch32v003funが必要です。  
書き込は、WCH-LinkEまたは、UIAPのUSB端子から行います。

## UIAP Side Tone DDS
モールス信号のON/OFF信号を入力し、700Hzのサイドトーンを出力します。
歪の少ないサイン波を出力します。

■ 変更履歴
- V1.0  
・ 新規リリース

## UIAP CWTX
SI5351aをI2C接続することにより、7Mhz帯のCW送信機として動作します。

■ 変更履歴
- V0.1  
・ テストリリース

## ch32v_morse2_arduino_ide
CW固定メッセージを連続送信します。
ビルド環境はArduino IDEです。


## UIAP_Arduino_I2C_Scanner  
UIAPduinoのI2Cに接続されたデバイスをシリアルコンソールに表示します。


# ライセンス
This project is licensed under the MIT License, see the LICENSE.txt file for details
