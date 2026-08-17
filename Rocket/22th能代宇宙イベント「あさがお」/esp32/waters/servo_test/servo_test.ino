#include <ESP32Servo.h>
Servo myservo;
void setup() {
        Serial.begin(115200);
        myservo.attach(19);
        myservo.write(0);
        //delay(1000);
        myservo.write(90);
        Serial.print("open");
}

void loop() {

}
