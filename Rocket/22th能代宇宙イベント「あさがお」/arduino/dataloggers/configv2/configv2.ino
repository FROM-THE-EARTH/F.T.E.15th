#include <SoftwareSerial.h>

// ピンの定義
const int RX_PIN = 14; // ArduinoのRX (E220のTXDへ)
const int TX_PIN = 15; // ArduinoのTX (E220のRXDへ)
const int M0_PIN = 2;
const int M1_PIN = 3;

// SoftwareSerialのインスタンス作成
SoftwareSerial loraSerial(RX_PIN, TX_PIN);

void setup() {
  // PCとの通信用
  Serial.begin(9600);
  // E220との通信用 (初期設定のボーレートは通常9600)
  loraSerial.begin(9600);

  // M0, M1ピンの出力設定
  pinMode(M0_PIN, OUTPUT);
  pinMode(M1_PIN, OUTPUT);

  // モード3 (設定/スリープモード) に移行
  digitalWrite(M0_PIN, HIGH);
  digitalWrite(M1_PIN, HIGH);
  
  // モード切り替えが安定するまで少し待つ
  delay(100); 

  Serial.println("--- E220-900T22S(JP) Config Mode ---");

  // 【1】設定の読み出しを実行
  readConfiguration();

  // 【2】設定の書き込みを行いたい場合は、以下のコメントアウトを外して実行してください
  // writeConfiguration();
}

void loop() {
  // E220からデータが返ってきたらPCのシリアルモニタに16進数で表示
  if (loraSerial.available()) {
    byte incomingByte = loraSerial.read();
    
    // 1桁の場合は先頭に0をつける
    if(incomingByte < 0x10) {
      Serial.print("0");
    }
    Serial.print(incomingByte, HEX);
    Serial.print(" ");
  }else{
  Serial.println("can't read");
  }
}

// 設定を読み出す関数
void readConfiguration() {
  Serial.println("Reading configuration...");
  
  byte readCmd[] = {0xC1, 0x00, 0x08}; // C1:読み出し, 00:開始アドレス, 08:8バイト
  loraSerial.write(readCmd, sizeof(readCmd));
  
  delay(100); // 応答を待つ
}

// 設定を書き込む関数（例）
void writeConfiguration() {
  Serial.println("\nWriting configuration...");
  
  // 注意: 以下のデータは一例です。データシートを見て環境に合わせた値を設定してください。
  // C0:書き込み, 00:開始アドレス, 08:8バイト長
  // [ADDH, ADDL, REG0, REG1, REG2, REG3, CRYPT_H, CRYPT_L]
  byte writeCmd[] = {0xC0, 0x00, 0x08, 0x00, 0x00, 0x62, 0x00, 0x12, 0x03, 0x00, 0x00}; 
  
  loraSerial.write(writeCmd, sizeof(writeCmd));
  
  delay(100); // 書き込み完了を待つ
  
  // 書き込み後に再度読み出して確認すると確実です
  // readConfiguration();
}