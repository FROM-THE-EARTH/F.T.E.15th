#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include <utility/imumaths.h>
#include <SparkFun_u-blox_GNSS_Arduino_Library.h>

// ==========================================
// ピン割り当て
// ==========================================
// SPI / I2C ピン
#define SD_CS 10
#define I2C_SDA 18
#define I2C_SCL 19

// ==========================================
// センサー・モジュールのオブジェクト生成
// ==========================================
Adafruit_BNO055 bno = Adafruit_BNO055(-1, 0x28, &Wire);
SFE_UBLOX_GNSS myGNSS;

// 実行間隔（ミリ秒）
const unsigned long INTERVAL = 300;
unsigned long lastTime = 0;
unsigned long elapsedTime;
void setup() {
  // 1. PCとのシリアル通信を開始（デバッグ用）
  // シリアルモニタのボーレートを「115200」に合わせてください
  Serial.begin(115200);
  while (!Serial) {
    ;  // シリアルポートの接続を待つ（一部のマイコン用）
  }
  Serial.println("System Starting...");

// 2. I2Cの初期化
Wire.setClock(50000); 
#if defined(ESP32)
  Wire.begin(I2C_SDA, I2C_SCL);
#else
  Wire.begin();
#endif

  if (!bno.begin()) {
    Serial.println("Error: BNO055 not detected!");
    while (1) { delay(10); }
  }
  //bno.setExtCrystalUse(true);
  Serial.println("BNO055 initialized.");

  // 4. SAM-M8Q (GPS) 初期化
  //if (!myGNSS.begin()) {
  //  Serial.println("Error: SAM-M8Q not detected!");
  //  while (1) { delay(10); }
  //}
  //myGNSS.setI2COutput(COM_TYPE_UBX);
  //Serial.println("SAM-M8Q initialized.");

  // 5. SDカード 初期化
  if (!SD.begin(SD_CS)) {
    Serial.println("Error: SD card initialization failed!");
    while (1) { delay(10); }
  }
  Serial.println("SD card initialized. Ready to log data.");
}

void loop() {
  if (millis() - lastTime >= INTERVAL) {
    lastTime = millis();

    // --- [1] BNO055から3軸加速度取得 ---
    sensors_event_t accelerometerData;
    bno.getEvent(&accelerometerData, Adafruit_BNO055::VECTOR_ACCELEROMETER);
    float accX = accelerometerData.acceleration.x;
    float accY = accelerometerData.acceleration.y;
    float accZ = accelerometerData.acceleration.z;

    if (accX == 0.0 && accY == 0.0 && accZ == 0.0) {
      //Serial.println("Warning: BNO055 returned all zeros. Re-initializing...");
      initBNO();
      return;  // 今回のループはスキップして次回再取得する
    }
    // --- [2] SAM-M8QからGPS座標取得 (十進法) ---
    //float latitude = myGNSS.getLatitude() / 10000000.0;
    //float longitude = myGNSS.getLongitude() / 10000000.0;

    // --- [3] データ文字列のフォーマット ---
    elapsedTime = millis();
    String dataString = String(elapsedTime) +
                        "," + String(accX, 2) + ", " + String(accY, 2) + ", " + String(accZ, 2);
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
void initBNO() {
  if (!bno.begin()) {
    Serial.println("Error: BNO055 initialization failed!");
  } else {
    //bno.setExtCrystalUse(true);
    //Serial.println("BNO055 initialized successfully.");
  }
}