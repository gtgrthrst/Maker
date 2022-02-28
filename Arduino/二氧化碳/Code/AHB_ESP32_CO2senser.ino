
#include <esp32-hal-timer.h>

static hw_timer_t *timer = NULL;
#include <esp_task_wdt.h>
//3 seconds WDT
#define WDT_TIMEOUT 60
int i = 0;
int last = millis();

//---------------------WS2812B----------------

#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
 #include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif

// Which pin on the Arduino is connected to the NeoPixels?
#define PIN        15 // On Trinket or Gemma, suggest changing this to 1

// How many NeoPixels are attached to the Arduino?
#define NUMPIXELS 1 // Popular NeoPixel ring size
Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

#define DELAYVAL 500 // Time (in milliseconds) to pause between pixels


//---------------------SHT30------------------
//#include <Wire.h>
//#include "SHT31.h"
//#define SHT31_ADDRESS   0x44
//
//uint32_t start;
//uint32_t stop;
//
//SHT31 sht;


/*---------------------------------------ESP32 WIFI-----------------------------------------*/

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
WiFiClient espClient;

String DeviceName = "";
/*---------------------------------------WiFiManager-----------------------------------------*/
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
WiFiManager wm;
/*---------------------------------------ArduinoJson-----------------------------------------*/
// https://www.itread01.com/content/1550251984.html
#include <ArduinoJson.h> // ArduinoJson library v6.15.1
DynamicJsonDocument JSONbuffer(1024);
char json_output[200];
// Standard Particles, CF=1

//-----------------------SSD1306OLED螢幕-------
#include <Adafruit_SSD1306.h>
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

//---------------------------------------MQTT-----------------------------------------
#include "PubSubClient.h"
const char* mqttServer = "broker.emqx.io"; // mqtt server address
const int mqttPort = 1883; // mqtt port
const char* mqttUser = ""; // your mqtt user
const char* mqttPassword = ""; // your mqtt password
String clientId = "ESP32Client-"; // Create a random client ID
// #include <WiFi.h>
PubSubClient client(espClient);

#define MQTT_PUB_AirBox   "huan-yu/AirBox/DSC-20-CO2"  //資料傳送
void MQTTreconnect()
{
  while (!client.connected())
  {
    Serial.println("Connecting to MQTT...");
    clientId += String(random(0xffff), HEX);
    if (client.connect(clientId.c_str(), mqttUser, mqttPassword))
    {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish(MQTT_PUB_HandShake, "I'm In~");
      // ... and resubscribe
      Serial.println(client.subscribe(MQTT_SUB_ACK));
    }
    else
    {
      Serial.print("failed with state ");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
      display.clearDisplay();
      display.setCursor(0, 0);
      display.println("MQTT");
      display.setCursor(0, 25);
      display.println(client.state());
      Serial.print("failed with state ");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      display.display();
      wm.process();
      // delay(5000); // Wait 5 seconds before retrying
      /*
        client.state() Returns
        -4 : MQTT_CONNECTION_TIMEOUT - the server didn't respond within the keepalive time
        -3 : MQTT_CONNECTION_LOST - the network connection was broken
        -2 : MQTT_CONNECT_FAILED - the network connection failed
        -1 : MQTT_DISCONNECTED - the client is disconnected cleanly
        0 : MQTT_CONNECTED - the client is connected
        1 : MQTT_CONNECT_BAD_PROTOCOL - the server doesn't support the requested version of MQTT
        2 : MQTT_CONNECT_BAD_CLIENT_ID - the server rejected the client identifier
        3 : MQTT_CONNECT_UNAVAILABLE - the server was unable to accept the connection
        4 : MQTT_CONNECT_BAD_CREDENTIALS - the username/password were rejected
        5 : MQTT_CONNECT_UNAUTHORIZED - the client was not authorized to connect
      */
    }
  }
}



// --------------------DS-CO2-20 高精度二氧化碳---------------------------
HardwareSerial myHardwareSerial(1); //ESP32可宣告需要一個硬體序列，軟體序列會出錯
#include <SoftwareSerial.h>
//SoftwareSerial Serial1 =  SoftwareSerial(25, 26);  //RX=25,TX=26

//myHardwareSerial.begin(9600, SERIAL_8N1, 12, 13); // Serial的TX,RX
static unsigned int co2 = 0;
static unsigned int ucRxBuffer[10];



//=====================setup====================================
void setup() {

  Serial2.begin(9600);
  delay(500);
  Serial.begin(9600);
 


  Serial.println(F("Trying to connect."));

  // Initialize communication port
  // FOR ESP32 SERIAL1 CHANGE THIS TO : E.G. RX-PIN 25, TX-PIN 26
  // SP30_COMMS.begin(115200, SERIAL_8N1, 25, 26);


  //-------------------SSD1306OLED螢幕-------------------
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }
  delay(2000);
  display.clearDisplay();

  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 10);
  // Display static text
  display.println("Hello, world!");
  display.display();
  //------------------------WiFiManager----------------------------------------
  WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP
  // it is a good practice to make sure your code sets wifi mode how you want it.
  //WiFiManager, Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wm;

  // reset settings - wipe stored credentials for testing
  // these are stored by the esp library
  //wm.resetSettings();

  bool res;
  // res = wm.autoConnect(); // auto generated AP name from chipid
  // res = wm.autoConnect("AutoConnectAP"); // anonymous ap
  res = wm.autoConnect("二氧化碳偵測器偵測器_001", "12345678"); // password protected ap

  if (!res) {
    Serial.println("Failed to connect");
    // ESP.restart();
  }
  else {
    //if you get here you have connected to the WiFi
    Serial.println("connected...yeey :)");
  }
  // ---------------------------MQTT------------------
  client.setBufferSize(512); // option or with need
  client.setServer(mqttServer, mqttPort); // need
  //client.setCallback(callback); // need
  delay(2000);
  display.clearDisplay();
  display.setCursor(0, 10);
  display.println("- MQTT connect to ");
  display.setCursor(0, 25);
  display.println(mqttServer);
  MQTTreconnect(); // need
 

  

  //---------------------WDT----------------------------------
  esp_task_wdt_init(WDT_TIMEOUT, true); //enable panic so ESP32 restarts
  esp_task_wdt_add(NULL); //add current thread to WDT watch
  Serial.println("setup OK");

  //----------------------WS2812B------------------------
#if defined(__AVR_ATtiny85__) && (F_CPU == 16000000)
  clock_prescale_set(clock_div_1);
#endif
  // END of Trinket-specific code.

  pixels.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
}
//=========================loop()===========================
void loop() {
  read_all();
  delay(3000);
}

/**
   @brief : read and display device info
*/

/**
   @brief : read and display all values
*/
bool read_all()
{
  static bool header = true;
  uint8_t ret, error_cnt = 0;
  //----------------------------二氧化碳感測器 CO2 sensor------------------------
  //Serial1.listen();
  byte request[] = {0x42, 0x4d, 0xe3, 0x00, 0x00, 0x01, 0x72};
  Serial2.write(request, 7);
  delay(500);

  while (Serial2.available()) {
    for (int i = 0; i < 12; i = i + 1) {
      ucRxBuffer[i] = Serial2.read();
    }
    co2 = ucRxBuffer[4] * 256 + ucRxBuffer[5];
    Serial.print("CO2 PPM:");
    Serial.println(co2);

    display.clearDisplay();
    display.setTextSize(2,2);
    display.setCursor(0, 10);
    display.println("CO : " );
    display.setCursor(40, 30);
    display.println(co2, DEC);
    display.setCursor(80, 45);
    display.println("ppm");
    display.setTextSize(2,1);
    display.setCursor(28, 15);
    display.println("2 " );
    //--------------------SHT30-----------------
      start = micros();
      sht.read();         // default = true/fast       slow = false
      stop = micros();

    display.setCursor(0, 50);
    display.println("Temp:" );
    display.setCursor(30, 50);
    display.println(sht.getTemperature(),1);
    display.setCursor(70, 50);
    display.println("Humi:" );
    display.setCursor(100, 50);
    display.println(sht.getHumidity(),1);
    //display.println(now.tostr(buf));
    display.display();
    //---------------json--------------
 
    JSONbuffer["CO2"] = int(co2);
//    JSONbuffer["Temp"] = sht.getTemperature();
//    JSONbuffer["Humi"] = sht.getHumidity();

    serializeJson(JSONbuffer, json_output);
    Serial.print("string to json:");
    Serial.println(json_output);


    //-----------------------MQTT--------------
    if (client.publish(MQTT_PUB_AirBox, json_output) == true)
    {
//          display.setTextSize(2,1);
      Serial.println("Success sending message");
//      display.setCursor(70, 0);
//      display.print("PubOK" );
      display.display();
    }
    else
    {
      Serial.println("Error sending message");
      display.setCursor(65, 0);
      display.print("PubErr" );
      display.display();
      MQTTreconnect(); 
      
    
    }



    //-------------------WifiManager-------------
    //wm.process(); 
    //----------------------WDT------------------------
      if (millis() - last >= 2000 && i < 300) {
      Serial.println("Resetting WDT...");
      esp_task_wdt_reset();
      last = millis();
      i++;
      if (i == 300) {
        Serial.println("Stopping WDT reset. CPU should reboot in 3s");
      }
    }
        //-----------------------WS2812B-------------------
      pixels.clear(); // Set all pixel colors to 'off'

  // The first NeoPixel in a strand is #0, second is 1, all the way up
  // to the count of pixels minus one.
  for(int i=0; i<NUMPIXELS; i++) { // For each pixel...

    // pixels.Color() takes RGB values, from 0,0,0 up to 255,255,255
    // Here we're using a moderately bright green color:
    
      pixels.setPixelColor(i, pixels.Color(254*co2/5000, 254*(5000-co2)/5000, 0));
        

    pixels.show();   // Send the updated pixel colors to the hardware.

      }
  }
  return (true);
}






void serialTrigger(char * mess)
{
  Serial.println();

  while (!Serial.available()) {
    Serial.println(mess);
    delay(2000);
  }

  while (Serial.available())
    Serial.read();
}
