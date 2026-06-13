#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include <utility/imumaths.h>
#include <SparkFun_u-blox_GNSS_Arduino_Library.h>

#define SD_CS 10
#define I2C_SDA 18
#define I2C_SCL 19

Adafruit_BNO055 bno = Adafruit_BNO055(-1, 0x28, &Wire);

// 実行間隔を少し長めに設定して負荷を下げる（100ms -> 500ms）
const unsigned long INTERVAL = 100;
unsigned long lastTime = 0;

void setup() {
  Serial.begin(115200);
  while (!Serial) { delay(10); }
  Serial.println("System Starting...");

  #if defined(ESP32)
    Wire.begin(I2C_SDA, I2C_SCL);
  #else
    Wire.begin();
  #endif

  // 【対策1】I2Cのクロック速度を極端に下げる（50kHzに設定）
  // これでクロックストレッチングのタイムアウトを防ぎます
  Wire.setClock(50000); 

  if (!bno.begin()) {
    Serial.println("Error: BNO055 not detected!");
    while (1) { delay(10); }
  }

  // 【対策2】外部クリスタル設定を無効化する（内部クロックで動作させる）
  // bno.setExtCrystalUse(true); 
  
  Serial.println("BNO055 initialized.");
}

void loop() {
  if (millis() - lastTime >= INTERVAL) {
    lastTime = millis();

    sensors_event_t accelerometerData;
    bno.getEvent(&accelerometerData, Adafruit_BNO055::VECTOR_ACCELEROMETER);
    float accX = accelerometerData.acceleration.x;
    float accY = accelerometerData.acceleration.y;
    float accZ = accelerometerData.acceleration.z;

    if (accX == 0.0 && accY == 0.0 && accZ == 0.0) {
      Serial.println("Warning: BNO055 returned all zeros. Re-initializing...");
      initBNO();
      return; 
    }

    Serial.print("Acc: ");
    Serial.print(accX); Serial.print(", ");
    Serial.print(accY); Serial.print(", ");
    Serial.println(accZ);
  }
}

void initBNO() {
  if (!bno.begin()) {
    Serial.println("Error: BNO055 initialization failed!");
  } else {
    // ここも無効化
    // bno.setExtCrystalUse(true);
    Serial.println("BNO055 recovered.");
  }
}