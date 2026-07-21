#include <SparkFun_u-blox_GNSS_Arduino_Library.h>
SFE_UBLOX_GNSS myGNSS;
void setup() {
    if (myGNSS.begin() == false) {
    Serial.println("Error: SAM-M8Q not detected!");
    while (1);
  }
  myGNSS.setI2COutput(COM_TYPE_UBX);

}

void loop() {
    bool hasData = myGNSS.getPVT(50);
    long lat_raw = 0;
    long lon_raw = 0;
    if (hasData) {
      lat_raw = myGNSS.getLatitude();
      lon_raw = myGNSS.getLongitude();
    }
    Serial.println(lat_raw/10000000.0,long_raw/10000000.0)

}
