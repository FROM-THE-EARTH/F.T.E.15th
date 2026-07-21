#include <ESP32Servo.h> // ESP32専用のサーボライブラリに変更
#include <Adafruit_BMP085.h>

#define SAMPLENUM 20
#define settime 500

int firetime = 9420;
int count = 0;
int fpstate = 0;
int Fpstate = 1;
int fpStateArray[SAMPLENUM];
int state = 0;
int alt_flag = 0;
bool status;
unsigned long starttime, nowtime; // millis()を扱うためunsigned longに変更
unsigned long Time;
int mode = 0;

Servo myservo;
int pos = 180;

Adafruit_BMP085 bmp;

float alt = 0;
float pres = 0;
float maxalt = -100;
float dalt = 0;
float prealt = 0;

// メジアン計算関数（元の配列を壊さないように呼び出し側でコピーを渡す想定）
float calcMedian(void *array, int n, int type) {
  if (type == 0) { // If data type is int
    int *intArray = (int*) array;
    for (int i = 0; i < n; i++) {
      for (int j = i + 1; j < n; j++) {
        if (intArray[i] > intArray[j]) {
          int changer = intArray[j];
          intArray[j] = intArray[i];
          intArray[i] = changer;
        }
      }
    }
    if (n % 2 == 0) {
      return (float) (intArray[n / 2] + intArray[n / 2 - 1]) / 2;
    } else {
      return (float) intArray[n / 2];
    }
  } else if (type == 1) { // If data type is float
    float *floatArray = (float*) array;
    for (int i = 0; i < n; i++) {
      for (int j = i + 1; j < n; j++) {
        if (floatArray[i] > floatArray[j]) {
          float changer = floatArray[j];
          floatArray[j] = floatArray[i];
          floatArray[i] = changer;
        }
      }
    }
    if (n % 2 == 0) {
      return (floatArray[n / 2] + floatArray[n / 2 - 1]) / 2;
    } else {
      return floatArray[n / 2];
    }
  } else {
    return 0.0;
  }
}

uint8_t fp () {
  int pinmode = 0;
  int i = digitalRead(27);
  if (i == 1) {
    pinmode = 0;
  } else if (i == 0) {
    pinmode = 1;
  }
  return (uint8_t)pinmode;
}

int isLaunched(int FlighPinState) {
  // 履歴をスライド
  for (int i = (SAMPLENUM - 1); i > 0; i--) {
    fpStateArray[i] = fpStateArray[i - 1];
  }
  fpStateArray[0] = FlighPinState;

  // ソートによって元の履歴配列が壊れるのを防ぐため、コピー配列を作成
  int tempArray[SAMPLENUM];
  for (int i = 0; i < SAMPLENUM; i++) {
    tempArray[i] = fpStateArray[i];
  }

  // コピーした配列をcalcMedianに渡す
  if (calcMedian(tempArray, SAMPLENUM, 0) == 1) { // launched
    return 0;
  } else {
    return 1;
  }
}

void setup() {
  Serial.begin(115200); // ESP32では115200bpsが一般的です
  
  pinMode(27, INPUT);
  pinMode(17, OUTPUT); // 追記: 17番ピンの出力設定

  // ESP32Servoの初期化 (タイマーの割り当て)
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);
  
 
  Serial.println("System Ready");

  if (!bmp.begin()) {
    Serial.println("Could not find a valid BMP085 sensor, check wiring!");
    while (1) {
      delay(100);
    }
  }
}

float d_alt() {
  float alt;
  alt = bmp.readAltitude();
  if (alt > 1000) {
    alt = prealt; // 異常値(1000m以上)が出た場合は前回の値を使用
  }
  prealt = alt;
  if (alt > maxalt) {
    maxalt = alt;
  }
  dalt = maxalt - alt;
  return dalt;
}

void loop() {
  fpstate = fp();
  Fpstate = isLaunched(fpstate);

  switch (mode) {
    case 0:
      digitalWrite(17, LOW);
      if (Fpstate == 0) {
        Time = 0;
        starttime = millis();
        mode = 1;
        Serial.println("Launched! Mode changed to 1");
      }
      break;

    case 1:
      if (Fpstate == 1) {
        mode = 0;
        break;
      }
      nowtime = millis();
      Time = nowtime - starttime;
      digitalWrite(17, HIGH);
      dalt = d_alt();
      
      Serial.print("dalt: ");
      Serial.println(dalt);

      if (dalt > 1.7) {
        count++;
      } else {
        count = 0;
      }

      // 条件合致でサーボ動作
      if ((count == 10) || (Time > 3000)) {
        Serial.println("OPEN");
        myservo.setPeriodHertz(50);    // サーボの標準的な周波数(50Hz)
        myservo.attach(19, 500, 2400); // 19番ピンにアタッチ(最小パルス幅と最大パルス幅を環境に合わせて調整)
        myservo.write(0);
        
        // 動作完了後は無限ループで待機（30000000000の代わり）
        while (1) {
          delay(1000); 
        }
      }
      break;
  }
  
  delay(50); // ループが早すぎるとBMP180の読み取りに負荷がかかるため少し待機
}