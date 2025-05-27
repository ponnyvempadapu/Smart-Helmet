#include <SoftwareSerial.h> 
SoftwareSerial mySerial(A2, A3); 

#include <LiquidCrystal.h> 
#define alc A1 
#define stb 6 // connect normal push button (it acts as a start button) 
#define motor 7 
#define bz 5 
#define hst A0 

#include <Wire.h> 
#include <Adafruit_Sensor.h> 
#include <Adafruit_ADXL345_U.h> 
Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_U(12345); 

#include <TinyGPS.h> 
TinyGPS gps; 

float flat = 0, flon = 0; 

const int rs = 8, en = 9, d4 = 10, d5 = 11, d6 = 12, d7 = 13; 
LiquidCrystal lcd(rs, en, d4, d5, d6, d7); 

int gval = 0; 
int vhs = 0; 
int aval; 

void read_gps() {
  bool newData = false;
  unsigned long chars;
  unsigned short sentences, failed;

  for (unsigned long start = millis(); millis() - start < 1000;) {
    while (Serial.available()) {
      char c = Serial.read();
      if (gps.encode(c)) {
        newData = true;
      }
    }
  }

  if (newData) {
    unsigned long age;
    gps.f_get_position(&flat, &flon, &age);
  }
}

void setup() {
  mySerial.begin(115200);
  Serial.begin(9600);
  accel.begin();
  lcd.begin(16, 2);

  lcd.print(" WELCOME");
  lcd.setCursor(0, 1);
  lcd.print("INITIALIZING");

  do {
    aval = analogRead(alc);
    lcd.setCursor(13, 1);
    lcd.print(" ");
    lcd.setCursor(13, 1);
    lcd.print(aval);
  } while (aval > 750);

  lcd.clear();
  lcd.print("INITIALIZED");

  Serial.println("AT");
  delay(1500);
  Serial.println("AT+CMGF=1");
  delay(500);

  pinMode(stb, INPUT_PULLUP);
  pinMode(bz, OUTPUT);
  pinMode(motor, OUTPUT);
  pinMode(alc, INPUT);
  pinMode(hst, INPUT_PULLUP);

  digitalWrite(bz, 0);
  digitalWrite(motor, 0);

  wifi_init();
}

void loop() {
  if (vhs == 0) {
    lcd.setCursor(0, 1);
    lcd.print("PLS Start");
  }

  if (digitalRead(stb) == 0) {
    if (digitalRead(hst) == 0) {
      lcd.clear();
      lcd.print("checking alc..");
      delay(3000);

      if (analogRead(alc) < 800) {
        lcd.clear();
        lcd.print("ALC NORMAL ");
        digitalWrite(motor, 1);
        delay(2000);
        vhs = 1;
      } else {
        lcd.clear();
        lcd.print("Driver alcoholic ");
        digitalWrite(bz, 1);
        digitalWrite(motor, 0);
        delay(2000);
        digitalWrite(bz, 0);
      }
    } else {
      lcd.clear();
      lcd.print("No Helmet ");
      digitalWrite(bz, 1);
      digitalWrite(motor, 0);
      delay(2000);
      digitalWrite(bz, 0);
    }
  }

  if (vhs == 1) {
    sensors_event_t event;
    accel.getEvent(&event);

    int xval = event.acceleration.x;
    int yval = event.acceleration.y;
    gval = analogRead(alc);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("A:" + String(gval) + " X:" + String(xval) + " Y:" + String(yval));
    lcd.setCursor(0, 1);
    lcd.print("H:" + String(hst));

    if (xval > 5 || xval < -5 || yval > 5 || yval < -5) {
      digitalWrite(bz, 1);
      digitalWrite(motor, 0);
      lcd.clear();
      lcd.setCursor(0, 1);
      lcd.print("ACCIDENT...");
      upload_iot(gval, hst, xval, yval);
      while (1);
    }

    if (gval > 500) {
      lcd.clear();
      lcd.print("Driver alcoholic ");
      lcd.setCursor(0, 1);
      lcd.print("VEHICLE STOPPED");
      digitalWrite(bz, 1);
      digitalWrite(motor, 0);
      digitalWrite(bz, 0);
      upload_iot(gval, hst, xval, yval);
      while (1);
    }

    if (digitalRead(hst) == 1) {
      lcd.clear();
      lcd.print("Helmet Removed ");
      digitalWrite(bz, 1);
      digitalWrite(motor, 0);
      digitalWrite(bz, 0);
      upload_iot(gval, hst, xval, yval);
      while (1);
    }
  }
}

void wifi_init() {
  mySerial.println("AT+RST");
  delay(2000);
  mySerial.println("AT+CWMODE=1");
  delay(1000);
  mySerial.print("AT+CWJAP=");
  mySerial.write('"');
  mySerial.print("project"); // ssid/user name 
  mySerial.write('"');
  mySerial.write(',');
  mySerial.write('"');
  mySerial.print("12345678"); // password 
  mySerial.write('"');
  mySerial.println();
  delay(1000);
}

void upload_iot(int x, int y, int z, int p) {
  String cmd = "AT+CIPSTART=\"TCP\",\"";
  cmd += "184.106.153.149"; // api.thingspeak.com 
  cmd += "\",80";
  mySerial.println(cmd);
  delay(1500);

  String getStr = "GET /update?api_key=RT86R9FBPBBEKUXT&field1=";
  getStr += String(x);
  getStr += "&field2=";
  getStr += String(y);
  getStr += "&field3=";
  getStr += String(z);
  getStr += "&field5=";
  getStr += String("18.4636");
  getStr += "&field6=";
  getStr += String("73.8682");
  getStr += "\r\n\r\n";

  cmd = "AT+CIPSEND=";
  cmd += String(getStr.length());
  mySerial.println(cmd);
  delay(1500);
  mySerial.println(getStr);
  delay(1500);
}
