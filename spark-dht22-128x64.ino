/*********************************************************************
This is an example for our Monochrome OLEDs based on SSD1306 drivers

  Pick one up today in the adafruit shop!
  ------> http://www.adafruit.com/category/63_98

This example is for a 128x64 size display using I2C to communicate
3 pins are required to interface (2 I2C and one reset)

Adafruit invests time and resources providing this open source code,
please support Adafruit and open-source hardware by purchasing
products from Adafruit!

Written by Limor Fried/Ladyada  for Adafruit Industries.
BSD license, check license.txt for more information
All text above, and the splash screen must be included in any redistribution
*********************************************************************/

#include "PietteTech_DHT.h"
#include "Adafruit_BMP085.h"
#include "Adafruit_GFX.h"
#include "Adafruit_SSD1306.h"
#include "U8glib.h"


#define OLED_RESET D4
Adafruit_SSD1306 display(OLED_RESET);

#define DHTTYPE    DHT22
#define DHTPIN     D2

void dht_wrapper();
char cmd[64], buf[64];
double t;
double h;
unsigned long co2;

Adafruit_BMP085 bmp;

byte co2cmd[] = {
  0xFE,
  0X44,
  0X00,
  0X08,
  0X02,
  0X9F,
  0X25
};

PietteTech_DHT DHT(DHTPIN, DHTTYPE, dht_wrapper);

void dht_wrapper() {
  DHT.isrCallback();
}

void setup()   {
  Particle.variable("t", t);
  Particle.variable("h", h);
  Particle.variable("co2", co2);

  Serial.begin(9600);
  Serial1.begin(9600);

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.setRotation(2);
  display.clearDisplay();
  display.display();

  bmp.begin();

  delay(2000);
}

void loop() {

  int result = DHT.acquireAndWait(0);
  float pressure = bmp.readPressure() / 100.0;
  float temp2 = bmp.readTemperature();
  float alti = bmp.readAltitude();

  if (result == DHTLIB_OK) {
    t = DHT.getCelsius();
    h = DHT.getHumidity();
    co2 = co2val(co2run(co2cmd));

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0,0);
    sprintf(buf, "temp: %.1f °C", t);
    display.println(buf);
    sprintf(buf, "humid: %.1f Rh", h);
    display.println(buf);
    sprintf(buf, "CO2: %d ppm", co2);
    display.println(buf);

    sprintf(buf, "T: %.1f *C", temp2);
    display.println(buf);
    sprintf(buf, "P: %.1f hPa", pressure);
    display.println(buf);
    sprintf(buf, "A: %.1f m", alti);
    display.println(buf);

    display.display();
  } else {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0,0);
    display.println("loading...");
    display.display();
    /*display.setTextColor(BLACK, WHITE);*/
  }

  delay(2000);
}

byte* co2run(byte packet[]) {
  byte co2res[] = { 0, 0, 0, 0, 0, 0, 0 };

  while( !Serial1.available() ) {
    Serial1.write(co2cmd, 7);
    delay(50);
  }

  int timeout=0;
  while( Serial1.available() < 7 ) {
    timeout++;

    if(timeout > 10) {
      while(Serial1.available()) Serial1.read();
      break;
    }

    delay(50);
  }

  for (int i = 0; i < 7; i++) co2res[i] = Serial1.read();

  return co2res;
}

unsigned long co2val(byte packet[]) {
  int high          = packet[3];
  int low           = packet[4];

  return high * 256 + low;
}
