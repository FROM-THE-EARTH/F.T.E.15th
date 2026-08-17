//gemini版と相違点
//servo attach,writeをセットアップに書かない
// if ((count == 10) || (Time > 3000)) {
//        Serial.println("OPEN");
//        myservo.setPeriodHertz(50);
//        myservo.attach(19, 500, 2400);
//        myservo.write(0);
//      }

#include <ESP32Servo.h>
#include <Adafruit_BMP085.h>

#define SAMPLENUM 20
#define settime 500

// UART通信用の設定 (UART2を使用)
HardwareSerial SerialUART(2);
const int RX_PIN = 16;
const int TX_PIN = 17;

int firetime = 9420;
int count = 0;
int fpstate = 0;
int Fpstate = 1;
int fpStateArray[SAMPLENUM];
int state = 0;
int alt_flag = 0;
bool status;
unsigned long starttime, nowtime;
unsigned long Time;
int mode = 0;

Servo myservo;
int pos = 180;

Adafruit_BMP085 bmp;

float current_alt = 0;
float maxalt = -100;
float dalt = 0;
float prealt = 0;

float calcMedian(void *array, int n, int type) {
  if (type == 0) {  // If data type is int

    int *intArray = (int *)array;

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

      return (float)(intArray[n / 2] + intArray[n / 2 - 1]) / 2;

    } else {

      return (float)intArray[n / 2];
    }

  } else if (type == 1) {  // If data type is float

    float *floatArray = (float *)array;

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

    // Error or unknown data type

    return 0.0;
  }
}



uint8_t fp() {

  int pinmode = 0;

  int i = digitalRead(13);

  if (i == 1) {

    pinmode = 0;

  } else if (i == 0) {

    pinmode = 1;
  }

  return (uint8_t)pinmode;
}



int isLaunched(int FlighPinState) {

  fpStateArray[0] = FlighPinState;

  for (int i = (SAMPLENUM - 1); i > 0; i--) {

    fpStateArray[i] = fpStateArray[i - 1];
  }

  fpStateArray[0] = FlighPinState;

  if (calcMedian(fpStateArray, SAMPLENUM, 0) == 1) {  //launched

    return 0;

  } else {

    return 1;
  }
}

void setup() {
  Serial.begin(115200);                                  // PCとのシリアルモニタ用
  SerialUART.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN);  // データロガーとのUART通信用

  pinMode(13, INPUT);

  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);

  myservo.write(90);
  Serial.println("Controller System Ready");

  if (!bmp.begin()) {
    Serial.println("Could not find a valid BMP085 sensor, check wiring!");
    //while (1) { delay(100); }
  }
}

void calc_d_alt() {
  current_alt = bmp.readAltitude();
  if (current_alt > 1000) {
    current_alt = prealt;
  }
  prealt = current_alt;
  if (current_alt > maxalt) {
    maxalt = current_alt;
  }
  dalt = maxalt - current_alt;
}

void loop() {
  // 高度の計算とUART送信 (常にデータロガーへ送信し続ける)
  calc_d_alt();
  // 送信フォーマット: "絶対高度,相対高度"
  nowtime=millis();
  Serial.print(nowtime);
  Serial.print(",");
  Serial.print(current_alt);
  Serial.print(",");
  Serial.println(mode);
  SerialUART.print(current_alt);
  SerialUART.print(",");
  SerialUART.println(mode);


  fpstate = fp();
  Fpstate = isLaunched(fpstate);


  switch (mode) {
    case 0:
      if (Fpstate == 0) {
        Time = 0;
        starttime = millis();
        mode = 1;
        //Serial.println("Launched! Mode 1");
      }
      break;

    case 1:
      if (Fpstate == 1) {
        mode = 0;
        break;
      }
      nowtime = millis();
      Time = nowtime - starttime;

      if (Time > 9420) {
        mode = 2;
        break;
      }
      break;

    case 2:
      if (dalt > 1.7) {
        count++;
      } else {
        count = 0;
      }
      nowtime = millis();
      Time = nowtime - starttime;
      //Time = 0;
      //count = 0;
      if ((count == 10) || (Time > 10000)) {
        //Serial.println("OPEN");
        myservo.attach(19);
        myservo.write(0);
        //delay(1000);
        myservo.write(90);
        mode = 3;
      }
      break;
  }

  delay(100);
}