#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include <SparkFun_u-blox_GNSS_Arduino_Library.h>

#define SD_CS_PIN 5
#define BNO055_I2C_ADDR 0x28 // BNO055のI2Cアドレス (環境によって0x29)

Adafruit_BNO055 bno = Adafruit_BNO055(55, BNO055_I2C_ADDR, &Wire);
SFE_UBLOX_GNSS myGNSS;

// --- BNO055の加速度レンジを16Gに変更する独自関数 ---
// --- BNO055の加速度レンジを16Gに変更する完全手動版関数 ---
void setBNO055Range16G() {
  // 1. CONFIGモード(0x00)に切り替え（設定変更に必須）
  Wire.beginTransmission(BNO055_I2C_ADDR);
  Wire.write(0x3D); // OPR_MODE レジスタ
  Wire.write(0x00); // CONFIG_MODE
  Wire.endTransmission();
  delay(25); // モード移行待機

  // 2. Page 1 に切り替え
  Wire.beginTransmission(BNO055_I2C_ADDR);
  Wire.write(0x07); // PAGE_ID レジスタ
  Wire.write(0x01); // Page 1
  Wire.endTransmission();
  delay(10);

  // 3. ACC_CONFIGレジスタ(0x08)に16G設定を書き込み
  // 0x0F = 0b00001111 (Normal Power, 62.5Hz Bandwidth, 16G Range)
  Wire.beginTransmission(BNO055_I2C_ADDR);
  Wire.write(0x08);
  Wire.write(0x0F); 
  Wire.endTransmission();
  delay(10);

  // 4. Page 0 に戻す
  Wire.beginTransmission(BNO055_I2C_ADDR);
  Wire.write(0x07);
  Wire.write(0x00); // Page 0
  Wire.endTransmission();
  delay(10);
  
  // 5. AMGモード(0x07)に切り替え（フュージョン無効・生データモード）
  Wire.beginTransmission(BNO055_I2C_ADDR);
  Wire.write(0x3D); // OPR_MODE レジスタ
  Wire.write(0x07); // AMG_MODE
  Wire.endTransmission();
  delay(25); // モード移行待機

  Serial.println("BNO055 Accelerometer range strictly forced to +/- 16G.");
}

void setup() {
  Serial.begin(115200);
  while (!Serial);
  
  Serial.println("System Initialization starting...");

  Wire.begin(21, 22);

  // --- BNO055 初期化 ---
  if (!bno.begin()) {
    Serial.println("Error: BNO055 not detected!");
    while (1);
  }
  
  // 外部クリスタルを使用 (AMGモード設定前に呼び出します)
  //bno.setExtCrystalUse(true); 
  
  // 16GレンジとAMGモードを設定
  setBNO055Range16G(); 

  // --- SAM-M8Q (GPS) 初期化 ---
  if (myGNSS.begin() == false) {
    Serial.println("Error: SAM-M8Q not detected!");
    while (1);
  }
  myGNSS.setI2COutput(COM_TYPE_UBX);
  Serial.println("SAM-M8Q initialized.");

  // --- SDカード 初期化 ---
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("Error: SD Card initialization failed!");
    while (1);
  }
  Serial.println("SD Card initialized.");

  // CSVヘッダー書き込み
  File dataFile = SD.open("/sensor_data.csv", FILE_APPEND);
  if (dataFile) {
    dataFile.println("Latitude,Longitude,AccelX(m/s^2),AccelY(m/s^2),AccelZ(m/s^2)");
    dataFile.close();
  }
  
  Serial.println("Initialization complete. Starting logging...");
}

void loop() {
  // --- GPS座標の取得 ---
  long latitude_raw = myGNSS.getLatitude();
  long longitude_raw = myGNSS.getLongitude();
  double latitude = latitude_raw / 10000000.0;
  double longitude = longitude_raw / 10000000.0;

  // --- BNO055 加速度の取得 ---
  // AMGモードのため、VECTOR_ACCELEROMETERで16Gスケールの生データが取得可能
  imu::Vector<3> accel = bno.getVector(Adafruit_BNO055::VECTOR_ACCELEROMETER);

  // --- 文字列の整形 ---
  String dataString = String(latitude, 7) + "," + 
                      String(longitude, 7) + "," + 
                      String(accel.x()) + "," + 
                      String(accel.y()) + "," + 
                      String(accel.z());

  // --- SDカードへの書き込み ---
  File dataFile = SD.open("/sensor_data.csv", FILE_APPEND);
  if (dataFile) {
    dataFile.println(dataString);
    dataFile.close();
    Serial.println(dataString);
  } else {
    Serial.println("Error: Writing to SD card failed.");
  }

  delay(100); // 必要に応じて待機時間を調整
}