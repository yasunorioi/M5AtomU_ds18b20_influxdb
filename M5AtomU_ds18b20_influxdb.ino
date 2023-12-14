#include <OneWire.h>
#include <M5Stack.h>
#include <WiFi.h>
#include <WiFiUdp.h>

// OneWire DS18S20, DS18B20, DS1822 Temperature Example
//
// http://www.pjrc.com/teensy/td_libs_OneWire.html
//
// The DallasTemperature library can do all this work for you!
// http://milesburton.com/Dallas_Temperature_Control_Library
 
OneWire ds(25); // on pin 10 (a 4.7K resistor is necessary)

char ssid[] = "SSID";     //  your network SSID (name) 
char pass[] = "pass";    // your network password
// the IP address of your InfluxDB host
byte host[] = {192,168,1,23};

// the port that the InfluxDB UDP plugin is listening on
int port = 8089;
WiFiUDP udp;

void setup(void) {
    Serial.begin(115200);
    Serial.println("start");
    WiFi.begin(ssid, pass);
    while(WiFi.status() != WL_CONNECTED){
      Serial.println(".");
    delay(500);
  }
}
 
void loop(void) {
    byte i;
    byte present = 0;
    byte type_s;
    byte data[12];
    byte addr[8];
    float celsius;


    if ( !ds.search(addr)) {
   //      Serial.println(" ----- ");
         ds.reset_search();
         delay(250);
         return;
    }
 
   if (OneWire::crc8(addr, 7) != addr[7]) {
        Serial.println("CRC is not valid!");
        return;
    }
 
    ds.reset();
    ds.select(addr);
    ds.write(0x44, 1); // start conversion, with parasite power on at the end
    delay(1000); // maybe 750ms is enough, maybe not
 // we might do a ds.depower() here, but the reset will take care of it.
  
    present = ds.reset();
    ds.select(addr); 
    ds.write(0xBE); // Read Scratchpad
        String address=String(addr[0],HEX)+String(addr[1],HEX)+String(addr[2],HEX)+String(addr[3],HEX)+String(addr[4],HEX)+String(addr[5],HEX);
        
    for ( i = 0; i < 9; i++) { // we need 9 bytes
         data[i] = ds.read();
    }
    int16_t raw = (data[1] << 8) | data[0];
     if (type_s) {
        raw = raw << 3; // 9 bit resolution default
        if (data[7] == 0x10) {
            // "count remain" gives full 12 bit resolution
           raw = (raw & 0xFFF0) + 12 - data[6];
        }
     } else {
         byte cfg = (data[4] & 0x60);
         // at lower res, the low bits are undefined, so let's zero them
         if (cfg == 0x00) raw = raw & ~7; // 9 bit resolution, 93.75 ms
         else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
         else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
         //// default is 12 bit resolution, 750 ms conversion time
     }
     celsius = (float)raw / 16.0;

  String cel;
  cel = String(celsius);
  String line;
  line = String("temp,device=" + address + " value=" + cel);
  Serial.println(line);
  udp.beginPacket(host, port);
  udp.print(line);
  udp.endPacket();
  delay(10000);
}
