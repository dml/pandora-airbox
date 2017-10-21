#include "Adafruit_SSD1306/Adafruit_SSD1306.h"
#include "Adafruit_HTU21DF/Adafruit_HTU21DF.h"

Adafruit_SSD1306 display(D4);
Adafruit_HTU21DF htu = Adafruit_HTU21DF();

unsigned long s8_co2;

float humidity,
      temperature;

byte co2cmd[] = {
  0xFE,
  0X44,
  0X00,
  0X08,
  0X02,
  0X9F,
  0X25
};

void setup()   {
  pinMode(D7, INPUT_PULLDOWN);

  Serial.begin(9600);
  Serial1.begin(9600);

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.setRotation(0);
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
  display.display();

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

  temperature = htu.readTemperature();
  humidity = htu.readHumidity();
  s8_co2 = co2val(co2run(co2cmd));

  showStatus("d");

  display.fillRect(0,11,128,16,BLACK);
  display.fillRect(0,48,118,16,BLACK);

  display.setTextColor(WHITE);
  display.setTextSize(2);
  display.setCursor(0,9);
  display.println(String::format("%.0f", temperature));
  display.setCursor(64,9);
  display.println(String::format("%.0f", humidity));
  display.setCursor(0,46);
  display.println(String::format("%d",   s8_co2));
  display.display();

  if (digitalRead(D7) == HIGH) {
    showStatus("A");
    delay(2000);
  } else {
    showStatus("i");

    if (waitFor(Particle.connected, 30000)) {
      showStatus("I");

      bool stat = Particle.publish("pandora/data",
        String::format("{\
          \"t\":\"%.1f\",\
          \"h\":\"%.1f\",\
          \"co2\":\"%d\"\
        }",
        temperature,
        humidity,
        s8_co2
      ));

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
