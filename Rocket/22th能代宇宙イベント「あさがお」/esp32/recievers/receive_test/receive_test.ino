#include "esp32_e220900t22s_jp_lib.h"

// LoRaクラスのインスタンスとコンフィギュレーション構造体を準備
CLoRa lora;
struct LoRaConfigItem_t config;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("--- LoRa Receiver Start ---");

  // 既定のコンフィギュレーション値を取得・セット
  lora.SetDefaultConfigValue(config);
  
  // E220-900T22S(JP) モジュールの初期化
  if (lora.InitLoRaModule(config)) {
    Serial.println("LoRa init error!");
    while (1) { delay(1000); }
  }
  
  // ノーマルモードへ移行
  lora.SwitchToNormalMode();
  Serial.println("Switch to Normal Mode.");
  Serial.println("Waiting for data...");
}

void loop() {
  // ① 受信データ格納用の専用構造体を準備
  struct RecvFrameE220900T22SJP_t rcv_data;

  // ② 構造体のアドレス(&)を渡してデータを受信（大文字の R から始まります）
  if (lora.receiveFrame(&rcv_data) == 0) {
    
    // 受信したデータ長 (recv_data_len) をチェック
    if (rcv_data.recv_data_len > 0) {
      
      // 文字列として安全に表示するため、末尾に終端文字を追加
      rcv_data.recv_data[rcv_data.recv_data_len] = '\0';
      
      // シリアルモニタに出力
      Serial.println((char*)rcv_data.recv_data);
      
      // 受信したバイト数を出力
    }
  }

  // 待機時間を入れてループ
  delay(10);
}