#include <SoftwareSerial.h>
#include <SPI.h>
#include <SD.h>

// ==========================================
// ピン割り当て
// ==========================================
// ソフトウェアシリアルピン（E220通信用）
#define E220_RX_PIN 0  // ArduinoのRX（E220のTXDへ接続）
#define E220_TX_PIN 1  // Arduino의 TX（E220のRXDへ接続）

// E220 制御ピン（ご指定のピン）
#define E220_AUX    5
#define E220_M0     8
#define E220_M1     9

// SPI CSピン（ご指定のピン）
#define SD_CS       10

// ソフトウェアシリアルオブジェクトの生成
SoftwareSerial e220Serial(E220_RX_PIN, E220_TX_PIN);

void setup() {
  // 1. E220モジュールのピン設定
  pinMode(E220_M0, OUTPUT);
  pinMode(E220_M1, OUTPUT);
  pinMode(E220_AUX, INPUT);

  // E220 WOR受信モード設定 (M0=LOW, M1=HIGH)
  digitalWrite(E220_M0, LOW);
  digitalWrite(E220_M1, LOW);

  // 2. PCとのシリアル通信を開始（デバッグ用・ハードウェアシリアル）
  Serial.begin(115200);
  while (!Serial) {
    ; // シリアルポートの接続を待つ
  }
  Serial.println("Receiver System Starting...");

  // 3. E220とのソフトウェアシリアル通信を開始
  // E220のデフォルトボーレート（9600）に合わせます
  e220Serial.begin(9600);

  // AUXがHIGHになるまで待機（モジュールの準備完了待ち）
  waitE220();

  // 4. SDカード 初期化
  if (!SD.begin(SD_CS)) {
    Serial.println("Error: SD card initialization failed!");
    while (1) { delay(10); }
  }
  Serial.println("SD card initialized. Waiting for data...");
}

void loop() {
  // ソフトウェアシリアル（E220）からデータが届いているか確認
  Serial.println(e220Serial.available());
  if (e220Serial.available() > 0) {
    
    // 改行コード（\n）に達するまで文字列を読み込む
    
    String receivedData = e220Serial.readStringUntil('\n');
    
    // 空文字でなければ処理を実行
    if (receivedData.length() > 0) {
      
      // --- [1] PCのシリアルモニタへ出力（リアルタイム確認用） ---
      Serial.print("Received: ");
      Serial.println(receivedData);
      
      // --- [2] SDカードへ保存 (SPI) ---
      // 処理の前にAUXピンの状態を確認（アイドル状態待ち）
      waitE220();
      
      File dataFile = SD.open("rx_data.txt", FILE_WRITE);
      if (dataFile) {
        dataFile.println(receivedData);
        dataFile.close();
        Serial.println("-> Saved to SD card.");
      } else {
        Serial.println("-> Error: Failed to open rx_data.txt");
      }
    }
  }
}

// ==========================================
// E220がアイドル状態（AUXがHIGH）になるまで待機する関数
// ==========================================
void waitE220() {
  while (digitalRead(E220_AUX) == LOW) {
    delay(10);
  }
}