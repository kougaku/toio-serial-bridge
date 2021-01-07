# toio-serial-bridge

M5StickC経由でのシリアル通信によるtoio制御

<img src="https://i.gyazo.com/f79c130f8f55e9f460e85ba43f415b95.png" alt="connection" width="700"/>

<img src="https://i.gyazo.com/681e062d8b2df67e866eebae0641019e.png" alt="overview" height="250"/> <img src="https://i.gyazo.com/be919231e71ab9c2691b38dd602f1733.png" alt="M5StickC" height="250"/>

- USB側の接続に関係なく、M5StickCが起動していればキューブ（1個）との接続が行われます。
- キューブと接続できている場合は緑色、切断されている場合は赤色のラインが表示されます。
- キューブとの接続が切れた場合、自動的に再接続が行われます。
- 動作テスト用の機能として、Aボタンで前進、Bボタンで回転の操作ができます。
- キューブのx座標、y座標、角度、Standard ID、Standard IDでの角度がコンマ区切りでシリアルで送られてきます。

## シリアルコマンド
シリアルモニタからコマンドを送信してキューブを操作できます。<br>
プログラムから制御する場合は、各コマンドの終端に改行コード（\n）を入れてください。<br>

### モータ制御
```m [left_speed] [right_speed]```<br>
```m [left_speed] [right_speed] [time_x100ms]```<br>
<br>
例：<br>

左輪の速度100、右輪の速度100で移動<br>
```m 100 100```<br>
<br>
左輪の速度100、右輪の速度100で1000ms間移動<br>
```m 100 100 10```<br>
<br>
停止<br>
```m 0 0```<br>

### 目標位置へ移動
```d [x] [y] [max_speed]```<br>
```d [x] [y] [max_speed] [angle] [move_type] [speed_type] [angle_type]```<br>
<br>
```[move_type]```<br>
0:回転しながら移動<br>
1:回転しながら移動（後退なし）<br>
2:回転してから移動<br>

```[speed_type]```<br>
0:速度一定<br>
1:目標地点まで徐々に加速<br>
2:目標地点まで徐々に減速<br>
3:中間地点まで徐々に加速し、そこから目標地点まで減速<br>
<br>
```[angle_type]```<br>
0:絶対角度/回転量が少ない方向<br>
1:絶対角度/正方向<br>
2:絶対角度/負方向<br>
3:相対角度/正方向<br>
4:相対角度/負方向<br>
5:角度指定なし/回転しない<br>
6:書き込み操作時と同じ（動作開始時と同じ角度）/回転量が少ない方向<br>
<br>
例：<br>
目標位置(500, 150)に速度100で移動<br>
```d 500 150 100```<br>
<br>
目標位置(500, 150)に速度100で移動、角度45度、回転しながら移動、加減速、絶対角度/回転量が少ない方向<br>
```d 500 150 100 45 0 3 0```<br>
<br>  
### サウンド再生
```s [id] [volume]```<br>
<br>
例：<br>
音1を音量200で再生<br>
```s 1 200```

## Processingによるアプリケーション例
<img src="https://i.gyazo.com/0b8e72eb142dcd34a668b3a32abdb42c.gif" alt="p5 example" width="500"/></a>

## 参考
こちらのコードをベースにしました。
https://github.com/Katsushun89/M5Stack_BLE_toio_controller
