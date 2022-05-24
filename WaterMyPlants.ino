#include <BluetoothSerial.h>
#include <BTAddress.h>
#include <BTAdvertisedDevice.h>
#include <BTScan.h>

/***************************************************************************
  UPDATED: Tweaked to work with BME280 and OLED screen: @martywassmer
  
  This is a library for the BME280 humidity, temperature & pressure sensor

  Designed specifically to work with the Adafruit BME280 Breakout
  ----> http://www.adafruit.com/products/2650

  These sensors use I2C or SPI to communicate, 2 or 4 pins are required
  to interface.

  Adafruit invests time and resources providing this open source code,
  please support Adafruit andopen-source hardware by purchasing products
  from Adafruit!

  Written by Limor Fried & Kevin Townsend for Adafruit Industries.
  BSD license, all text above must be included in any redistribution
 ***************************************************************************/

#include <Wire.h>
#include <SPI.h>
#include <WiFi.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME680.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <Adafruit_MotorShield.h>
#include <math.h>
#include "Adafruit_seesaw.h"

#define BME_SCK 13
#define BME_MISO 12
#define BME_MOSI 11
#define BME_CS 10
#define SEALEVELPRESSURE_HPA (1013.25)

Adafruit_seesaw ss;
Adafruit_SH1107 display = Adafruit_SH1107(64, 128, &Wire);
Adafruit_BME680 bme; // I2C

Adafruit_MotorShield AFMS = Adafruit_MotorShield();
Adafruit_DCMotor *myMotor = AFMS.getMotor(1);

#if defined(ESP8266)
  #define BUTTON_A  0
  #define BUTTON_B 16
  #define BUTTON_C  2
#elif defined(ESP32)
  #define BUTTON_A 15
  #define BUTTON_B 32
  #define BUTTON_C 14
#elif defined(ARDUINO_STM32_FEATHER)
  #define BUTTON_A PA15
  #define BUTTON_B PC7
  #define BUTTON_C PC5
#elif defined(TEENSYDUINO)
  #define BUTTON_A  4
  #define BUTTON_B  3
  #define BUTTON_C  8
#elif defined(ARDUINO_FEATHER52832)
  #define BUTTON_A 31
  #define BUTTON_B 30
  #define BUTTON_C 27
#else // 32u4, M0, M4, nrf52840 and 328p
  #define BUTTON_A  9
  #define BUTTON_B  6
  #define BUTTON_C  5
#endif

int sel;
char ip[50] = "None";
int ledState = 0;
int pumpState = 0;

// WiFi network name and password:
const char * networkName = "**";
const char * networkPswd = "**";

// Internet domain to request from:
const char * hostDomain = "google.com";
const int hostPort = 80;

const int LED_PIN = 13;

void setup() {
  
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);

  if(Serial)Serial.println("BME280, OLED, and WIFI test");

  // Show image buffer on the display hardware.
  // Since the buffer is intialized with an Adafruit splashscreen
  // internally, this will display the splashscreen.
  if(Serial)Serial.println("Setting up display");
  delay(250); // wait for the OLED to power up
  display.begin(0x3C, true); // Address 0x3C default
  display.display();
  display.setRotation(1);
  delay(500);
  
  // Clear the buffer.
  display.clearDisplay();
  display.display();

  if(Serial)Serial.println("Setting up buttons");
  pinMode(BUTTON_A, INPUT_PULLUP);
  pinMode(BUTTON_B, INPUT_PULLUP);
  pinMode(BUTTON_C, INPUT_PULLUP);

  //check for temp module
  if (!bme.begin()) {
    Serial.println("ERROR! Could not find BME680 sensor");
    //while (1);
  } else {
    Serial.println("Found BME680");
  }

  //check for moisture sensor
  if (!ss.begin(0x36)) {
    Serial.println("ERROR! Seesaw not found");
    //while(1) delay(1);
  } else {
    Serial.print("Seesaw started! version: ");
    Serial.println(ss.getVersion(), HEX);
  }

  //check for dc motor
  if (!AFMS.begin()) {         // create with the default frequency 1.6KHz
  // if (!AFMS.begin(1000)) {  // OR with a different frequency, say 1KHz
    Serial.println("Could not find Motor Shield");
    //while (1);
  } else {
    Serial.print("Motor shield started!");
  }

  // Connect to the WiFi network (see function below loop)
  // connectToWiFi(networkName, networkPswd);

}

void sendToDisplay(String message, int delayTime, bool clearDisplay){
  if(clearDisplay)display.clearDisplay();
  display.setTextSize(1.5);
  display.setTextColor(SH110X_WHITE);
  if(clearDisplay)display.setCursor(0,0);
  display.print(message);
  display.display(); // actually display all of the above
  delay(delayTime);
}

void loop() {

  // Blink led
  digitalWrite(LED_PIN, ledState);
  ledState = (ledState + 1) % 2; // Flip ledState
   
  //get data from sensor
  float temp = ((bme.temperature*180)/100)+32;
  float alt = bme.readAltitude(SEALEVELPRESSURE_HPA);
  float pres = bme.pressure / 1000.0;
  float hum = bme.humidity;
  float gas = bme.gas_resistance / 1000.0;
 
  if(!sel)sel=0;

  if(Serial)Serial.print("Temperature = ");
  if(Serial)Serial.print(temp);
  if(Serial)Serial.println(" *F");
  if(Serial)Serial.print("Pressure = ");
  if(Serial)Serial.print(pres);
  if(Serial)Serial.println(" kPa");
  if(Serial)Serial.print("Approx. Altitude = ");
  if(Serial)Serial.print(alt);
  if(Serial)Serial.println(" m");
  if(Serial)Serial.print("Humidity = ");
  if(Serial)Serial.print(hum);
  if(Serial)Serial.println(" %");
  if(Serial)Serial.print("Gas = ");
  if(Serial)Serial.print(gas);
  if(Serial)Serial.println(" KOhms");
  if(Serial)Serial.print("IP = ");
  if(Serial)Serial.println(ip);    

  //data from soil sensor
  float tempC = ((ss.getTemp()*180)/100)+32;
  uint16_t capread = ss.touchRead(0);
  if(Serial)Serial.print("Temperature: "); Serial.print(tempC); Serial.println("*C");
  if(Serial)Serial.print("Capacitive: "); Serial.println(capread);

  if(Serial)Serial.println("-----------------");

  if (!digitalRead(BUTTON_A)) sel=1;
  if (!digitalRead(BUTTON_B)) sel=2;
  if (!digitalRead(BUTTON_C)) sel=3;

  //run motor when button pressed
  if(sel==1){
    display.clearDisplay();
    display.setTextSize(3);
    display.setCursor(0,0);
    display.print("Running");
    display.print("Pump");
    if(Serial)Serial.print("Running Pump");
    display.display();
    myMotor->setSpeed(150);
    myMotor->run(FORWARD);
    delay(2000);
    myMotor->run(RELEASE);
    sel=0;
  }

  display.clearDisplay();
  display.setTextSize(2.2);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(0,0);
  
  display.setCursor(0,0);
  display.print("pres:");
  display.print(pres);
  display.setCursor(0,15);
  display.print("hum:");
  display.print(hum);
  display.setCursor(0,30);
  display.print("tmp:");
  display.print(temp);
  display.setCursor(0,45);
  display.print("cap:");
  display.print(capread);
  display.display(); // actually display all of the above

  delay(2000);
}

void connectToWiFi(const char * ssid, const char * pwd)
{
  int ledState = 0;

  //printLine();
  if(Serial)Serial.println("Connecting to WiFi network: " + String(ssid));

  WiFi.begin(ssid, pwd);

  while (WiFi.status() != WL_CONNECTED) 
  {
    // Blink LED while we're connecting:
    digitalWrite(LED_PIN, ledState);
    ledState = (ledState + 1) % 2; // Flip ledState
    delay(500);
    Serial.print(".");
  }

  if(Serial)Serial.println();
  if(Serial)Serial.println("WiFi connected!");
  if(Serial)Serial.print("IP address: ");
  if(Serial)Serial.println(WiFi.localIP());
  sprintf(ip, "%s", WiFi.localIP());
  
}

void requestURL(const char * host, uint8_t port)
{
  //printLine();
  if(Serial)Serial.println("Connecting to domain: " + String(host));

  // Use WiFiClient class to create TCP connections
  WiFiClient client;
  if (!client.connect(host, port))
  {
    Serial.println("connection failed");
    return;
  }
  if(Serial)Serial.println("Connected!");
  //printLine();

  // This will send the request to the server
  client.print((String)"GET / HTTP/1.1\r\n" +
               "Host: " + String(host) + "\r\n" +
               "Connection: close\r\n\r\n");
  unsigned long timeout = millis();
  while (client.available() == 0) 
  {
    if (millis() - timeout > 5000) 
    {
      Serial.println(">>> Client Timeout !");
      client.stop();
      return;
    }
  }

  // Read all the lines of the reply from server and print them to Serial
  while (client.available()) 
  {
    String line = client.readStringUntil('\r');
    Serial.print(line);
  }

  Serial.println();
  Serial.println("closing connection");
  client.stop();
}
