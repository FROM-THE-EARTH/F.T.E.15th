#include <SoftwareSerial.h>
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
// ソフトウェアシリアルピン（E220通信用）
#define E220_RX_PIN 0  // ArduinoのRX（E220のTXDへ接続）
#define E220_TX_PIN 1  // ArduinoのTX（E220のRXDへ接続）

// E220 制御ピン（ご指定のピン）
#define E220_M0   2
#define E220_M1   3
#define E220_AUX  5

// SPI / I2C ピン（ご指定のピン）
#define SD_CS     10
#define I2C_SDA   18
#define I2C_SCL   19

// ==========================================
// センサー・モジュールのオブジェクト生成
// ==========================================
SoftwareSerial e220Serial(E220_RX_PIN, E220_TX_PIN);
Adafruit_BNO055 bno = Adafruit_BNO055(-1, 0x28, &Wire);
SFE_UBLOX_GNSS myGNSS;

// 実行間隔（ミリ秒）
const unsigned long INTERVAL = 5000; 
unsigned long lastTime = 0;

void setup() {
  // 1. E220モジュールのピン設定
  pinMode(E220_M0, OUTPUT);
  pinMode(E220_M1, OUTPUT);
  pinMode(E220_AUX, INPUT);

  // E220 WOR送信モード設定 (M0=HIGH, M1=LOW)
  digitalWrite(E220_M0, LOW);
  digitalWrite(E220_M1, LOW);

  // 2. PCとのシリアル通信を開始（デバッグ用・ハードウェアシリアル）
  Serial.begin(115200);
  while (!Serial) {
    ; // シリアルポートの接続を待つ
  }
  Serial.println("Transmitter System Starting...");

  // 3. E220とのソフトウェアシリアル通信を開始
  e220Serial.begin(9600);

  // 4. I2Cの初期化
  #if defined(ESP32)
    Wire.begin(I2C_SDA, I2C_SCL);
  #else
    Wire.begin(); 
  #endif

  // AUXがHIGHになるまで待機（モジュールの準備完了待ち）
  waitE220();

  // 5. BNO055 (加速度) 初期化
  if (!bno.begin()) {
    Serial.println("Error: BNO055 not detected!");
    while (1) { delay(10); }
  }
  bno.setExtCrystalUse(true);
  Serial.println("BNO055 initialized.");

  // 6. SAM-M8Q (GPS) 初期化
  if (!myGNSS.begin()) {
    Serial.println("Error: SAM-M8Q not detected!");
    while (1) { delay(10); }
  }
  myGNSS.setI2COutput(COM_TYPE_UBX); 
  Serial.println("SAM-M8Q initialized.");

  // 7. SDカード 初期化
  if (!SD.begin(SD_CS)) {
    Serial.println("Error: SD card initialization failed!");
    while (1) { delay(10); }
  }
  Serial.println("SD card initialized. Ready.");
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

    // --- [2] SAM-M8QからGPS座標取得 (十進法) ---
    float latitude = myGNSS.getLatitude() / 10000000.0;
    float longitude = myGNSS.getLongitude() / 10000000.0;

    // --- [3] データ文字列のフォーマット ---
    String dataString = "Lat:" + String(latitude, 6) + 
                        ", Lon:" + String(longitude, 6) + 
                        ", aX:" + String(accX, 2) + 
                        ", aY:" + String(accY, 2) + 
                        ", aZ:" + String(accZ, 2);

    // --- [4] PCのシリアルモニタへ出力（ローカルデバッグ用） ---
    Serial.print("Sending Data: ");
    Serial.println(dataString);

    // --- [5] SDカードへ保存 (SPI) ---
    File dataFile = SD.open("data.txt", FILE_WRITE);
    if (dataFile) {
      dataFile.println(dataString);
      dataFile.close();
      Serial.println("-> Saved to SD card.");
    } else {
      Serial.println("-> Error: Failed to open data.txt");
    }

    // --- [6] E220-900T22Sで無線送信 (SoftwareSerial) ---
    // 送信前にモジュールがアイドル状態（AUXがHIGH）か確認
    waitE220();
    
    // ソフトウェアシリアルで文字列を送信
    e220Serial.println(dataString);

    // 無線送信が完全に完了するまで待機
    waitE220();
    Serial.println("-> Broadcast finished.");
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