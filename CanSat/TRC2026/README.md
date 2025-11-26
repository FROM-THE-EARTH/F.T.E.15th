# TRC2026
## 1. 開発環境の準備

### ハードウェア

* Raspberry Pi Zero 2 W
* microSDカード（16GB以上あれば良い）
* 電源（5V / 2.5A）（PCからのUSB給電でもよいが、不安定になるときがある）
* PC（Windows / macOS / Linux のいずれか）
  * USBハブ
  * Mini HDMI ケーブル（モニター接続用）
  * モニター・キーボード（あると便利）

### ソフトウェア

* Git
* VSCode
* Raspberry Pi Imager
* SSH クライアント（Windows なら標準 PowerShell）

---

## 2. Raspberry Pi Zero 2 W のセットアップ

### 2.1 OS イメージの準備

1. Raspberry Pi Imager をインストールする（https://www.raspberrypi.com/software/）。
2. 起動後、
  * Device → *Raspberry PI Zero 2 W* 
  * OS → *Raspberry Pi OS(other)*から*Raspberry Pi OS Lite（64-bit）* を選択（GUIは要らない）
  * Storage → 該当するMicroSDカードを選択

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
   ssh pi@<ホスト名>.local
   ```
3. パスワードは Imager で設定したものを使用

接続できない場合は以下を確認：

* 同一ネットワークにいるか
* `.local` 解決ができない環境では、ラズパイの IPアドレス を確認して、ホスト名のところを IP に置き換えて接続する（WindowsやAndroid端末では、mDNS（.local）が安定的にサポートされておらず、ホスト名接続は一般に不安定）
* Permission denied (publickey,password).が出る場合
以下で接続を実行（その場しのぎ）
```bash
ssh -o PreferredAuthentications=password -o PubkeyAuthentication=no <ユーザー名>@<IPアドレス>
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
sudo apt install -y python3-venv python3-pip
python3 -m venv venv
source venv/bin/activate
```

### 3.2 依存パッケージのインストール

```bash
sudo apt update
sudo apt install -y python3-smbus i2c-tools python3-gpiozero python3-rpi-lgpio python3-rpi.gpio python3-venv python3-pip git python3-gpiozero python3  python3-pandas screen #ラズパイの新しいOSでは、pigpioのサポートが切れたので、gpiozeroで代替する
pip install -y numpy opencv-python==4.6.0.66 pyserial #sudoは付けない
```

---

## 4. リポジトリのクローン（ https://github.com/YutakaOkutani/TRC2026 ）

```bash
git clone https://github.com/YutakaOkutani/TRC2026
cd TRC2026
```

---

## 5. シリアル通信 / GPIO / I2C の有効化

### 5.1 raspi-config での設定

```bash
sudo raspi-config
```

以下を有効化：

* Interface Options → I2C

* Interface Options → Serial

* Interface Options → SPI（必要に応じて）

* Performance → GPU Memory = （必要に応じて）

完了後、再起動。

```bash
sudo reboot
```

---

## 6. プロジェクト構成

```
.
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
│   ├── LED.py
├── requirements.txt
└── README.md
```

各ディレクトリの目的を以下に示す：

* **main.py**
  制御コード本体。
* **library**
  importするライブラリ
* **__init__.py**
  libraryフォルダを認識できるようにするための空のファイル
* **tests/**
  モジュール単位の動作確認用。
* **README.md**
  本書。

---

## 7. 実行方法
* メインコード
```bash
source venv/bin/activate
python3 main.py
```
* GPSのテスト（気圧、9軸はライブラリコードを実行すればテスト可能）
```bash
sudo screen /dev/ttyAMA0 115200 #ボーレートが違う場合、9600も試す
```
* カメラのテスト
```bash
rpicam-hello -t 0　#bookwormの場合
```

---

## 8. トラブルシューティング

### 8.1 センサが認識されない

* `i2cdetect -y 1` で確認
* 配線の導通を確認

---

## 9. その他

### Raspberry Pi Zero 2 W で IP アドレスを固定する手順


#### 1. ネットワーク情報の確認
```bash
ip a
```
または

```bash
ifconfig
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
ssh <ユーザー名>@<Raspberry Pi のIPアドレス>
```
* 接続後、VS Code 下部のステータスバーが「SSH: Raspberry Pi」表示になる
* Terminal → New Terminal を開くと、Pi のターミナルが利用可能

#### 3. 注意点

* 初回接続時は Pi 側に VS Code サーバが自動インストールされる。
* ターミナルは Pi のユーザ権限で動く（root操作は sudo）。




---
