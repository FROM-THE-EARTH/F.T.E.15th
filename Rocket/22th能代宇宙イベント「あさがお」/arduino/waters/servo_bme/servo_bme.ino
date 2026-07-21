#include <Servo.h>
#include <Adafruit_BMP085.h>

#define SAMPLENUM 20
#define settime 500

int firetime=9420;

int count=0;
int fpstate=0;
int Fpstate=1;
int fpStateArray[SAMPLENUM];
int state=0;
int alt_flag=0;
bool status;
int starttime,nowtime;
int Time;
int mode=0;

Servo myservo;
int pos = 180;

Adafruit_BMP085 bmp;

float alt=0;
float pres=0;
float maxalt=-100;
float dalt=0;
float prealt=0;


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
    // Error or unknown data type
    return 0.0;
  }
}

uint8_t fp (){
  int pinmode=0;
  int i = digitalRead(14);
  if(i==1){
    pinmode=0;
  }else if(i==0){
    pinmode=1;
  }
  return (uint8_t)pinmode;
}

int isLaunched(int FlighPinState) {
  fpStateArray[0] = FlighPinState;
  for (int i = (SAMPLENUM - 1); i > 0; i--) {
    fpStateArray[i] = fpStateArray[i - 1];
  }
  fpStateArray[0] = FlighPinState;
  if (calcMedian(fpStateArray, SAMPLENUM, 0) == 1) { //launched
    return 0;
  } else {
    return 1;
  }
}


void setup()
{ 
  
  pinMode(14,INPUT);
  myservo.write(90); 
  //myservo.write(0);  
  Serial.begin(9600);
  Serial.print("1");
  pinMode(17,OUTPUT);
  digitalWrite(17,LOW);

  
  if (!bmp.begin()) {
  Serial.println("Could not find a valid BMP085 sensor, check wiring!");
  while (1) {}

   
}
}

float d_alt(){
    float alt;
    alt=bmp.readAltitude();
    //Serial.println(alt);
    if(alt>1000){
      alt=prealt;
    }
    prealt=alt;
    if (alt>maxalt){
      maxalt=alt;

      } 

    dalt=maxalt-alt;
    return dalt;
}

void loop()
{
//myservo.write(0);  
fpstate = fp();
Fpstate = isLaunched(fpstate);
//Serial.print("Fpstate:");
//Serial.println(Fpstate);
switch(mode){
case 0: 
digitalWrite(17,LOW);
if (Fpstate==0){
Time=0;
starttime=millis();
mode=1;
}
break;

case 1:
if (Fpstate==1){
mode = 0;
break;
} 
nowtime=millis();
Time=nowtime-starttime;
digitalWrite(17,HIGH);
dalt=d_alt();
//Serial.print("dalt:");
Serial.println(dalt);
//Serial.print(",");
//Serial.print("Time:");
//Serial.print(Time);
if (dalt>1.7){
  count++;
  //Serial.println(count);
  }else{
  count=0;
  }

//Serial.println(Time);
if((count==10)||(Time>3000)){
  Serial.println("open");
  myservo.attach(3);
  delay(1000);
  myservo.write(90);
  delay(30000000000);
}
break;
}
}