#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include <utility/imumaths.h>
#include <SD.h>
// ==========================================
// ピン割り当て (ESP32の場合は標準の21, 22を使用)
// ==========================================
#if defined(ESP32)
  #define I2C_SDA 21
  #define I2C_SCL 22
#endif

#define SD_CS 10
Adafruit_BNO055 bno = Adafruit_BNO055(-1, 0x28, &Wire);

const unsigned long INTERVAL = 100; // 測定間隔(ミリ秒)
unsigned long lastTime = 0;



void setup() {
  Serial.begin(115200);
  while (!Serial) { delay(10); }
  Serial.println("System Starting...");
  if (!SD.begin(SD_CS)) {
    Serial.println("Error: SD card initialization failed!");
    while (1) { delay(10); }
  }
  Serial.println("SD card initialized. Ready to log data.");
  // --- [対策1] I2Cの初期化とクロック速度の低下 ---
  #if defined(ESP32)
    Wire.begin(I2C_SDA, I2C_SCL);
  #else
    Wire.begin();
  #endif
  
  // クロックストレッチングによるタイムアウトを防ぐため 50kHz に下げる
  Wire.setClock(50000);

  // 初回起動時のBNO055初期化
  initBNO();
}

void loop() {
  if (millis() - lastTime >= INTERVAL) {
    lastTime = millis();

    // AMGモード時は getVector() で生の加速度を取得する
    imu::Vector<3> accel = bno.getVector(Adafruit_BNO055::VECTOR_ACCELEROMETER);

    float accX = accel.x();
    float accY = accel.y();
    float accZ = accel.z();

    // --- [対策2] 0検知時の自己修復（再初期化） ---
    if (accX == 0.0 && accY == 0.0 && accZ == 0.0) {
      //Serial.println("Warning: BNO055 returned all zeros. Re-initializing...");
      initBNO(); // 再起動と16G設定をやり直す
      return;    // 今回のループはスキップ
    }

    // 値の出力

    String dataString = 
                        String(accX, 2) + ", " + String(accY, 2) + ", " + String(accZ, 2);
    // --- [4] PCのシリアルモニタへ出力 (デバッグ確認用) ---
    Serial.println(dataString);

    // --- [5] SDカードへ保存 (SPI) ---
    File dataFile = SD.open("data.txt", FILE_WRITE);
    if (dataFile) {
      dataFile.println(dataString);
      dataFile.close();
    } else {
      Serial.println("Error: Failed to open data.txt for writing.");
    }
  }
}

// ==========================================
// BNO055の初期化処理（自己修復時にも呼ばれる）
// ==========================================
void initBNO() {
if (!bno.begin()) {
    Serial.println("Error: BNO055 initialization failed!");
    return;
  }
  
  // 2. 16Gに設定する安全な関数を呼ぶ
  setSafeAccelRange16G();

  //Serial.println("BNO055 ready in AMG Mode (Range: 16G)");
}

// ==========================================
// レジスタを直接操作して加速度レンジを変更する関数
// ==========================================
void setSafeAccelRange16G() {
  uint8_t bno_address = 0x28; // BNO055のI2Cアドレス

  // 1. Adafruitライブラリの機能を使って、確実にCONFIGモード（設定モード）へ移行
  bno.setMode(OPERATION_MODE_CONFIG);
  delay(50); // モード切り替え完了を長めに待つ（これがないとフリーズします）

  // 2. レジスタのページを「Page 1」に変更
  Wire.beginTransmission(bno_address);
  Wire.write(0x07); // PAGE_ID register
  Wire.write(0x01); // Page 1
  Wire.endTransmission();
  delay(10);

  // 3. 現在のACC_CONFIGレジスタ(0x08)の値を読み込む
  Wire.beginTransmission(bno_address);
  Wire.write(0x08);
  Wire.endTransmission(false);
  Wire.requestFrom((int)bno_address, 1);
  uint8_t acc_config = Wire.read();

  // 4. レンジ部分(下位2ビット)を 16G に書き換える
  // 0x00=2G, 0x01=4G, 0x02=8G, 0x03=16G
  acc_config = (acc_config & 0xFC) | 0x03; 

  // 5. 変更した設定を書き戻す
  Wire.beginTransmission(bno_address);
  Wire.write(0x08);
  Wire.write(acc_config);
  Wire.endTransmission();
  delay(10); // 書き込み完了を待つ

  // 6. ページを「Page 0」に戻す
  Wire.beginTransmission(bno_address);
  Wire.write(0x07); 
  Wire.write(0x00); // Page 0
  Wire.endTransmission();
  delay(10);

  // 7. ライブラリの機能を使って、確実にAMGモード（非フュージョン・測定モード）へ復帰
  bno.setMode(OPERATION_MODE_AMG);
  delay(50); // 測定開始の準備ができるまでしっかり待つ
}