#include "esp32_e220900t22s_jp_lib.h"

// LoRaクラスのインスタンスとコンフィギュレーション構造体を準備
CLoRa lora;
struct LoRaConfigItem_t config;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("--- LoRa Transmitter Start ---");

  // ① 既定のコンフィギュレーション値を取得・セット
  lora.SetDefaultConfigValue(config);
  
  // ② E220-900T22S(JP) モジュールの初期化
  if (lora.InitLoRaModule(config)) {
    Serial.println("LoRa init error!");
    while (1) { delay(1000); } // エラー時は停止
  }
  
  // ③ 通信可能なノーマルモードへ移行
  lora.SwitchToNormalMode();
  Serial.println("Switch to Normal Mode.");
}

void loop() {
  // 送信するメッセージを準備
  char msg[] = "Hello E220!";
  
  Serial.printf("Sending Data: %s\n", msg);

  // ④ データの送信 (uint8_t* へのキャストが必要)
  // 戻り値が 0 なら送信成功
  if (lora.SendFrame(config, (uint8_t *)msg, sizeof(msg)) == 0) {
    Serial.println("Send succeeded.");
  } else {
    Serial.println("Send failed.");
  }

  // 5秒おきに送信
  delay(5000);
}