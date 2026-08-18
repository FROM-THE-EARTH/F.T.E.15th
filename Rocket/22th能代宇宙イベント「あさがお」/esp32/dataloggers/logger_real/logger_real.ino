#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include <SparkFun_u-blox_GNSS_Arduino_Library.h>
#include "esp32_e220900t22s_jp_lib.h" // LoRa用ライブラリ

#define SD_CS_PIN 5
#define BNO055_I2C_ADDR 0x28 // 0x29の場合あり

// UART通信用の設定 (UART2: コントローラーからの高度データ受信用)
HardwareSerial SerialUART(2);
const int RX_PIN = 33;
const int TX_PIN = 32;

// センサーのインスタンス
Adafruit_BNO055 bno = Adafruit_BNO055(55, BNO055_I2C_ADDR, &Wire);
SFE_UBLOX_GNSS myGNSS;

// LoRaのインスタンスと設定
CLoRa lora;
struct LoRaConfigItem_t config;

// コントローラーからの受信データを保持する変数"alt,mode""

// LoRaの送信間隔管理用変数
unsigned long lastLoRaSend = 0;
unsigned long Time=0;
const unsigned long LORA_SEND_INTERVAL = 1000; // 1000ms(1秒)に1回送信

// --- グローバル変数 ---
String remoteData = "0.00,0"; 
unsigned long lastRecvTime = 0;       
const unsigned long TIMEOUT_MS = 1500; 

// --- マルチタスク・排他制御用の変数 ---
SemaphoreHandle_t i2cMutex;  // I2Cバス通信の衝突を防ぐためのロック
SemaphoreHandle_t dataMutex; // GPSデータの読み書き衝突を防ぐためのロック

// GPSタスクで取得し、メインループで読み取る共有変数
double global_latitude = 0.0;
double global_longitude = 0.0;
// BNO055の加速度レンジを16Gに強制変更する関数

int phase = 0;
void setBNO055Range16G() {
  Wire.beginTransmission(BNO055_I2C_ADDR);
  Wire.write(0x3D); // OPR_MODE
  Wire.write(0x00); // CONFIG_MODE
  Wire.endTransmission();
  delay(25);

  Wire.beginTransmission(BNO055_I2C_ADDR);
  Wire.write(0x07); // PAGE_ID
  Wire.write(0x01); // Page 1
  Wire.endTransmission();
  delay(10);

  Wire.beginTransmission(BNO055_I2C_ADDR);
  Wire.write(0x08); // ACC_CONFIG
  Wire.write(0x0F); // 16G Range
  Wire.endTransmission();
  delay(10);

  Wire.beginTransmission(BNO055_I2C_ADDR);
  Wire.write(0x07);
  Wire.write(0x00); // Page 0
  Wire.endTransmission();
  delay(10);
  
  Wire.beginTransmission(BNO055_I2C_ADDR);
  Wire.write(0x3D);
  Wire.write(0x07); // AMG_MODE
  Wire.endTransmission();
  delay(25);
}

void gpsTask(void *pvParameters) {
  for (;;) {
    // 1. I2Cのロックを取得 (他がI2Cを使っていたら待つ)
    xSemaphoreTake(i2cMutex, portMAX_DELAY);
    
    // 2. GPSに新しいデータが来ているかチェック
    // タイムアウトを短め(50ms)にすることで、データ未到着時にI2Cバスを長時間独占するのを防ぐ
    bool hasData = myGNSS.getPVT(50);
    
    long lat_raw = 0;
    long lon_raw = 0;
    if (hasData) {
      lat_raw = myGNSS.getLatitude();
      lon_raw = myGNSS.getLongitude();
    }
    
    // 3. I2Cの通信が終わったので即座にロックを解放 (重要: BNO055に順番を譲る)
    xSemaphoreGive(i2cMutex);

    // 4. データが取れていれば、共有変数に書き込む
    if (hasData) {
      xSemaphoreTake(dataMutex, portMAX_DELAY); // データ書き込みロック
      global_latitude = lat_raw / 10000000.0;
      global_longitude = lon_raw / 10000000.0;
      xSemaphoreGive(dataMutex); // ロック解放
    }

    // 次のGPSチェックまで少し待つ (100ms)
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

void setup() {
  Serial.begin(115200); // PC用
  SerialUART.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN); // コントローラー受信用
    // 排他制御用ミューテックスの作成
  i2cMutex = xSemaphoreCreateMutex();
  dataMutex = xSemaphoreCreateMutex();
  
  // --- LoRa 初期化 ---
  Serial.println("--- LoRa Transmitter Start ---");
  lora.SetDefaultConfigValue(config);
  if (lora.InitLoRaModule(config)) {
    Serial.println("Error: LoRa init failed!");
    while (1) { delay(1000); }
  }
  lora.SwitchToNormalMode();
  Serial.println("LoRa Switch to Normal Mode.");

  // --- I2C 初期化 ---
  Wire.begin(21, 22);

  // --- BNO055 初期化 ---
  if (!bno.begin()) {
    Serial.println("Error: BNO055 not detected!");
    while (1);
  }

  setBNO055Range16G(); // 16Gに設定

  // --- GPS 初期化 ---
  if (myGNSS.begin() == false) {
    Serial.println("Error: SAM-M8Q not detected!");
    while (1);
  }
  myGNSS.setI2COutput(COM_TYPE_UBX);

  // --- SDカード 初期化 ---
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("Error: SD Card initialization failed!");
    while (1);
  }

  // CSVヘッダー書き込み
  File dataFile = SD.open("/sensor_data.csv", FILE_APPEND);
  if (dataFile) {
    dataFile.println("Time,Latitude,Longitude,AccelX(m/s^2),AccelY(m/s^2),AccelZ(m/s^2),Abs_Altitude(m),mode");
    dataFile.close();
  }
  xTaskCreatePinnedToCore(
    gpsTask,        // 実行する関数
    "GPSTask",      // タスク名
    4096,           // スタックサイズ
    NULL,           // パラメータ
    1,              // 優先度 (1が標準)
    NULL,           // タスクハンドル
    0               // 割り当てるコア番号 (Core 0)
  );

  Serial.println("Logger System Ready. Logging started.");
}


void loop() {
  // --- UARTでコントローラーからの最新データを取得 ---
if(phase!=3){
  while (SerialUART.available() > 0) {
      String temp = SerialUART.readStringUntil('\n');
      temp.trim();
    
  if (temp.length() > 0 && temp.indexOf(',') != -1) {
        
        // さらに厳密に、文字化け（数字、カンマ、ドット、マイナス以外のゴミ文字）がないかチェック
        bool isValid = true;
        for (int i = 0; i < temp.length(); i++) {
          char c = temp.charAt(i);
          if (!isdigit(c) && c != '.' && c != ',' && c != '-' && c != '\r' && c != '\n') {
            isValid = false; // 不正な文字（文字化け）が含まれている
            break;
          }
        }
        
        // 正常なデータであれば変数を更新し、受信時刻を記録
        if (isValid) {
          remoteData = temp;
        }else{
          remoteData = "0,0";
        }
        int commaIndex = remoteData.indexOf(',');
        // カンマが見つかった場合
        if (commaIndex != -1) {
        // カンマの次の文字から最後までを切り出し、int型に変換する
        phase = remoteData.substring(commaIndex + 1).toInt();
        }
      }
    }
  }
  
  double latitude, longitude;
  xSemaphoreTake(dataMutex, portMAX_DELAY);
  latitude = global_latitude;
  longitude = global_longitude;
  xSemaphoreGive(dataMutex);

  // --- BNO055 加速度の取得 ---
  xSemaphoreTake(i2cMutex, portMAX_DELAY);
  imu::Vector<3> accel = bno.getVector(Adafruit_BNO055::VECTOR_ACCELEROMETER);
  xSemaphoreGive(i2cMutex);

  Time=millis();

  // --- データの合体 (文字列化) ---
  String dataString = String(Time)+","+
                      String(latitude, 7) + "," + 
                      String(longitude, 7) + "," + 
                      String(accel.x()) + "," + 
                      String(accel.y()) + "," + 
                      String(accel.z()) + "," + 
                      remoteData;

  // --- SDカードへの書き込み (毎ループ実行) ---
  File dataFile = SD.open("/sensor_data.csv", FILE_APPEND);
  if (dataFile) {
    dataFile.println(dataString);
    dataFile.close();
    Serial.println("SD: " + dataString); // 確認用
  } else {
    Serial.println("Error: SD Write failed.");
  }


  // --- LoRaによる無線送信 (指定間隔ごとに実行) ---
 
  if (Time - lastLoRaSend >= LORA_SEND_INTERVAL) {
    // String型をC言語スタイルの文字配列(const char*)に変換して送信
    const char* msg = dataString.c_str();
    size_t msgLen = dataString.length();

    // データの送信 (文字列の長さを指定)
    if (lora.SendFrame(config, (uint8_t *)msg, msgLen) == 0) {
      //Serial.println("LoRa Send succeeded.");
    } else {
      Serial.println("LoRa Send failed.");
    }
    lastLoRaSend = Time; // 送信時刻を更新
  }

  delay(10); 
}