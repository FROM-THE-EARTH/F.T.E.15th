#include <E220.h>
#include <SoftwareSerial.h>
#define M0 2
#define M1 3

SoftwareSerial mySerial(D0, D1); // RX, TX
E220 e220(mySerial,0xFF,0xFF,0x00);/
byte payload[199]="YA";

void setup() {
    Serial.begin(9600);
    mySerial.begin(9600);//e220 conect to 9600bps
    pinMode(M0, OUTPUT);
    pinMode(M1, OUTPUT);
    digitalWrite(M0, LOW);
    digitalWrite(M1, LOW);
}

void loop() {
  e220.TransmissionData(payload)

}
