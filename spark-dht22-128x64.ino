#include "PietteTech_DHT.h"
#include "Adafruit_BMP085.h"
#include "Adafruit_SSD1306.h"

#define OLED_RESET D4
Adafruit_SSD1306 display(OLED_RESET);

#define DHTTYPE    DHT22
#define DHTPIN     D2

void dht_wrapper();
char buf[64];
unsigned long co2;
double t;
double h;
float pressure;
float temp2;
float alti;

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

int setupMillis;
bool boolHasSlept=0;
bool boolWifiReconnected=0;

void dht_wrapper() {
  DHT.isrCallback();
}

void setup()   {
  pinMode(D7, INPUT_PULLDOWN);

  Serial.begin(9600);
  Serial1.begin(9600);

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.setRotation(2);
  display.clearDisplay();
  display.setFont(COMICS_8);
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.println("temp");
  display.setCursor(64,0);
  display.println("RH%");
  display.setCursor(0,37);
  display.println("CO2");
  display.setCursor(64,37);
  display.println("hPa");
  display.display();

  bmp.begin();

  setupMillis = millis();

  delay(2000);
}

void showStatus(char* str) {
  display.setTextSize(1);
  display.setTextColor(BLACK);
  display.setCursor(120,56);
  display.fillRect(118,54,10,10,WHITE);
  display.setCursor(119,55);
  display.println(str);
  display.display();
}

void loop() {

  showStatus("s");

  co2 = co2val(co2run(co2cmd));
  pressure = bmp.readPressure() / 100.0;
  temp2 = bmp.readTemperature();
  alti = bmp.readAltitude();

  int result = DHT.acquireAndWait(0);

  if (result == DHTLIB_OK) {
    t = DHT.getCelsius();
    h = DHT.getHumidity();
  }

  t = (1 - 0.1) * t;

  showStatus("d");

  display.fillRect(0,11,128,16,BLACK);
  display.fillRect(0,48,118,16,BLACK);

  display.setTextColor(WHITE);
  display.setTextSize(2);
  display.setCursor(0,9);
  display.println(String::format("%.0f", t));
  display.setCursor(64,9);
  display.println(String::format("%.0f", h));
  display.setCursor(0,46);
  display.println(String::format("%d",   co2));
  display.setCursor(64,46);
  display.println(String::format("%.0f", pressure));
  display.display();

  if (digitalRead(D7) == HIGH) {
    showStatus("A");
    delay(2000);
  } else {
    showStatus("i");

    if (waitFor(Particle.connected, 30000)) {
      showStatus("I");

      bool stat = Particle.publish("pandora/data", String::format("{\"t1\":\"%.1f\",\"t2\":\"%.1f\",\"h\":\"%.1f\",\"co2\":\"%d\",\"hpa\":\"%.1f\"}", t, temp2, h, co2, pressure));

      Particle.process();

      if (stat) {
        showStatus("S");
        System.sleep(SLEEP_MODE_DEEP, 60);
      } else {
        showStatus("E");
        Particle.publish("pandora/error", "unable to publish all");
        delay(30000);
      }
    } else {
      showStatus("R");
      System.reset();
    }
  }
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
