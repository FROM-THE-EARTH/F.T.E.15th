# TRC2026
## ファイル構成（ https://github.com/YutakaOkutani/TRC2026 ）
```
├── main.py
├── venv
├── library/
│   ├── bno055.py
│   ├── bmp180.py
│   ├── detect_corn.py
│   ├── capture_roi_img.py
│   ├── micropyGPS.py
│   ├── __init__.py
├── tests/
│   ├── motor_test.py
│   ├── led.py
│   ├── gps_test.py
│   ├── landing_impact.py
│   ├── open_parachute.py
└── README.md
```
- 本番用コード：main.py
- センサー用ライブラリ：bno055.py, bmp180.py, mycropyGPS.py
- テストコード：motor_test.py, led.py, gps_test.py, landing_impact.py（着地衝撃試験用）, open_parachute.py（パラシュート投下試験用）
- カメラフェーズ用：detect_corn.py
- ゴール画像撮影用：capture_roi_image.py

## 搭載計器一覧
|分類|型番|購入先|備考|
| ---- | ---- |---|---|
|マイコン |Raspberry Pi Zero 2 W|マルツ（協賛）|OSは、Raspberry Pi OS Lite(64bit)|
|マイクロSD |LMEX1L032GG2/LMEX1L032GG4|秋月|16GBで十分|
|9軸センサ|BNO055|秋月|
|気圧センサ|BMP180|電子工作ステーション|
|モーター|99:1 Metal Gearmotor 25Dx54L mm HP 12V |POLOLU|
|モータードライバ|POLOLU-4038|マルツ|DRV8256E搭載モジュール|
|超音波センサ|HC-SR04|秋月|
|GPS|GT-502MGG-N|秋月|
|カメラ|KEYESTUDIO 5MP |Yahooショッピング|互換品であるが十分|
|バッテリー|KT850/35-3S|Amazon|
|DC-DCコンバータ|AE-OKL-T/6-W12N-C|秋月|変格ではあるが、出力電流が多く（Max6A）、ラズパイの定格電流（2.5A）を出せるので選定|

## 0. 環境構築の準備

### ハードウェア

* Raspberry Pi Zero 2 W
* microSDカード（16GB以上あれば良い）
* 電源（5V / 2.5A）（PCからのUSB給電でもよいが、不安定になるときがある）
* PC
* USBハブ
* Mini HDMI ケーブル（モニター接続用）
* モニター・キーボード（あると便利）

### ソフトウェア

* Raspberry Pi Imager
* VSCode
* Git
* ターミナル

---

## 2. Raspberry Pi Zero 2 W のセットアップ

### 2.1 OS イメージの準備

1. Raspberry Pi Imager をインストールする（ https://www.raspberrypi.com/software/ ）
2. 起動後、
  * Device → *Raspberry PI Zero 2 W* 
  * OS → *Raspberry Pi OS(other)*から*Raspberry Pi OS Lite（64-bit）* （GUIは要らない）
  * Storage → MicroSDカード

3. 以下を設定・有効化する

   * Hostname
   * Localisation（国・キーボード設定）
   * User
   * Wi-Fi
   * Remote access（SSHの有効化、パスワード認証で十分）
   * Raspberry Pi Connect（オフでよい）

4. Writing（書き込み）を実行

---

### 2.2 初回起動と接続
1. microSD を ラズパイ に挿入して電源投入
2. PC から次のコマンドで接続

   ```bash
   ssh ユーザー名@ホスト名.local（or IPアドレス）
   ```
3. パスワードは Imager で設定したものを使用

接続できない場合は以下を確認：

* 同一ネットワークにいるか
* `.local` 解決ができない環境では、ラズパイの IPアドレス を確認して、ホスト名のところを IP に置き換えて接続する（WindowsやAndroid端末では、mDNS（.local）が安定的にサポートされておらず、ホスト名接続は一般に不安定）
* Permission denied (publickey,password).が出る場合、以下のコマンドで接続
```bash
ssh -o PreferredAuthentications=password -o PubkeyAuthentication=no ユーザー名@IPアドレス
```
その後、以下のコマンドでラズパイ上の設定を確認
```bash
sudo nano /etc/ssh/sshd_config
```
その中に、以下の行があれば確認。
```
PasswordAuthentication yes
```

PasswordAuthentication が no になっていたら yes に変更。

"#" でコメントアウトされている場合は、"#" を外して PasswordAuthentication yes にする

設定を変更したら、SSH サーバーを再起動。
```bash
sudo systemctl restart ssh
```

---

### 2.3 初期アップデート

```bash
sudo apt update
sudo apt full-upgrade -y
sudo reboot
```

---

## 3. Python 実行環境の構築

### 3.1 Python 仮想環境の作成

```bash
# sudo apt install -y python3-pip python3.10 python3.10-venv
# python3.10（opencv-python==4.6.0.66 が入るバージョン（最新版のPythonは当該バージョンをサポートしてない）；opencv-python==4.6.0.66 なのは、動作保証が取れているからで、最新版だとバグりがち（最新版のPythonをサポートしてないこともある））がはじかれた場合は以下を実行
# sudo apt install -y git build-essential libssl-dev zlib1g-dev \ libbz2-dev libreadline-dev libsqlite3-dev wget curl llvm \ libncursesw5-dev xz-utils tk-dev libxml2-dev libxmlsec1-dev libffi-dev liblzma-dev
# pyenv をインストール
# curl https://pyenv.run | bash
# 確認
# ls ~/.pyenv
# .bashrc を編集
# nano ~/.bashrc
# ~/.bashrc に以下を追記（自動で追記されている場合あり）
# export PATH="$HOME/.pyenv/bin:$PATH"
# eval "$(pyenv init -)"
# eval "$(pyenv virtualenv-init -)"
# 読み込み
# source ~/.bashrc
# 再度確認
# pyenv --version
# Python 3.10 をインストール
# pyenv install 3.10.13
# pyenv global 3.10.13
# 確認
# python3 --version
# 仮想環境作成
# python3 -m venv --system-site-packages venv
# source venv/bin/activate 
```

### 3.2 依存パッケージのインストール

```bash
sudo apt update
sudo apt install -y python3-smbus i2c-tools python3-gpiozero python3-rpi-lgpio git python3 screen libgl1 

# pynmea2
sudo apt install -y python3-pip
python3 -m pip install --upgrade pip
python3 -m pip install pynmea2

sudo apt install swig python3-dev python3-setuptools build-essential
sudo apt install liblgpio1 liblgpio-de

sudo apt install -y python3-picamera2 python3-libcamera libcamera-apps

sudo apt install python3-opencv python3-numpy python3-smbus python3-serial

sudo apt install python3-opencv

# pip install smbus2 "numpy>=1.23,<1.25" opencv-python==4.6.0.66 pyserial # pip install requirements.txt でもよい
# pip install gpiozero 
# pip install lgpio
# pip install --no-cache-dir lgpio
```
```bash
sudo apt update
sudo apt install -y python3-smbus i2c-tools python3-gpiozero python3-rpi-lgpio git python3 screen libgl1
sudo apt install swig python3-dev python3-setuptools build-essential
sudo apt install liblgpio1 liblgpio-de
sudo apt install -y python3-picamera2 python3-libcamera libcamera-apps
sudo apt install python3-numpy python3-smbus python3-serial
sudo apt install python3-opencv
```

---

## 4. シリアル通信 / GPIO / I2C の有効化

### 4.1 raspi-config での設定

```bash
sudo raspi-config
```

以下を有効化：

* Interface Options → I2C

* Interface Options → Serial

* Performance → GPU Memory（必要に応じて）

完了後、再起動。

```bash
sudo reboot
```

---

## 5. リポジトリのクローン（ https://github.com/YutakaOkutani/TRC2026 ）

```bash
git clone https://github.com/YutakaOkutani/TRC2026
cd TRC2026
```
---


## 6. 実行方法
* メインコード
```bash
source venv/bin/activate # 仮想環境を作らなくても、環境構築ができたら、実行しなくてよい
python3 main.py
```

* GPSからの生データ取得（テストコードのほうが見やすいが一応）
```bash
sudo screen /dev/ttyAMA0 115200 # ボーレートが違う場合、9600も試す
```

* カメラの設定
```bash
# バージョン確認
lsb_release -a 
```
```bash
# 設定ファイル編集
sudo nano /boot/firmware/config.txt
# 以下の内容を追加または修正
dtoverlay=OV5647 # OV5647 カメラを明示指定
camera_auto_detect=0 # カメラ自動検出を無効化
```
```bash
# 編集後、再起動
sudo reboot
```

* カメラ認識の確認
```bash
# 認識デバイスの一覧
libcamera-hello --list-cameras
# dmesg による初期化ログ確認
dmesg | grep -i ov5647
# 初期化時間の確認
time rpicam-hello -t 1
```

* カメラのプロパティ確認
```bash
rpicam-hello --info-text
```

* カメラ映像のテスト
```bash
# ライブプレビュー
rpicam-hello -t 0
# 静止画撮影
rpicam-still -o test.jpg
# 動画撮影
rpicam-vid -t 10000 -o test.h264
# 高解像度での静止画撮影テスト
rpicam-still --width 2592 --height 1944 -o maxres.jpg
```


---

## 7. トラブルシューティング

### センサが認識されない

* `sudo i2cdetect -y 1` で確認
* 配線の導通を確認

---

## 8. その他

### Raspberry Pi Zero 2 W で IP アドレスを固定する手順


#### 1. ネットワーク情報の確認
```bash
ip a
```

#### 2. 設定ファイルの編集

dhcpcd.conf を編集。
```bash
sudo nano /etc/dhcpcd.conf
```

#### 3. 静的アドレス設定の追加

ファイル末尾に以下を追加。IP アドレスやルーター情報は使用中のネットワーク環境に合わせて変更。
```bash
interface wlan0
static ip_address=192.168.1.50/24
static routers=192.168.1.1
static domain_name_servers=192.168.1.1 8.8.8.8
```

* static ip_address: 割り当てたい固定 IP とサブネット
* static routers: デフォルトゲートウェイ（通常はルーターのアドレス）
* static domain_name_servers: DNS サーバー（static routersに入力したものと同じでよい）

#### 4. 再起動と確認

Raspberry Pi を再起動。
```bash
sudo reboot
ip a
```


### VSCodeでSSH接続したラズパイのターミナルを操作する方法


#### 1. VS Codeで拡張機能「Remote - SSH」をインストールする

#### 2. 接続

* 左下の「><」アイコン（リモート接続）からRemote-SSH: Connect to Host… を選択
* 以下を入力
```
ssh ユーザー名@IPアドレス
```
* 接続後、VS Code 下部のステータスバーが「SSH: Raspberry Pi」表示になる
* Terminal → New Terminal を開くと、Pi のターミナルが利用可能

#### 3. 注意点

* 初回接続時は Pi 側に VS Code サーバが自動インストールされる。
* ターミナルは Pi のユーザ権限で動く（root操作は sudo）。



### 基本的なgit操作コマンド


```bash
# ファイルをステージングに追加
git add .
# コミットを作成
git commit -m "Initial commit"
# GitHub へ初回プッシュ
git push -u origin main
# 2回目以降
git push
```
``` bash
# ローカルを GitHub の最新版で完全に上書きするコマンド
git fetch origin
git reset --hard origin/main
```
```bash
# ローカルの変更を残しつつ、GitHub の更新を取り込むコマンド（pull.ver）
git pull origin main
```
```bash
# ローカルの変更を残しつつ、GitHub の更新を取り込むコマンド（rebase.ver）
git pull --rebase origin main
```





---
