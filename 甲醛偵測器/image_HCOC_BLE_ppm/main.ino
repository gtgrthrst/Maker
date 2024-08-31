#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>
#include "HCOC.h"
#include <Arduino.h>
#include <SensirionI2CSfa3x.h>
#include <Wire.h>
#include <Fonts/FreeSans18pt7b.h>
#include <Fonts/FreeSans9pt7b.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#define TFT_CS        7
#define TFT_RST       3
#define TFT_DC        2

#define SDA_PIN       8
#define SCL_PIN       9

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);
SensirionI2CSfa3x sfa3x;

BLEServer *pServer = NULL;
BLECharacteristic *pCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;
int sendCount = 0;

class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *pServer) {
    deviceConnected = true;
    Serial.println("Device connected");
  };
  void onDisconnect(BLEServer *pServer) {
    deviceConnected = false;
    Serial.println("Device disconnected");
  }
};

void setup() {
  Serial.begin(115200);
  Serial.println("Starting BLE HCHO Sensor...");

  Wire.begin(SDA_PIN, SCL_PIN);

  uint16_t error;
  char errorMessage[256];

  sfa3x.begin(Wire);

  error = sfa3x.startContinuousMeasurement();
  if (error) {
    Serial.print("Error trying to execute startContinuousMeasurement(): ");
    errorToString(error, errorMessage, 256);
    Serial.println(errorMessage);
  } else {
    Serial.println("Continuous measurement started successfully");
  }

  tft.initR(INITR_BLACKTAB);
  tft.setRotation(3);
  tft.fillScreen(ST77XX_BLACK);

  // 繪製背景圖像
  tft.drawRGBBitmap(0, 0, HCOC.data, HCOC.width, HCOC.height);

  // Create the BLE Device
  BLEDevice::init("ESP32 HCHO Sensor");

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ | 
    BLECharacteristic::PROPERTY_WRITE | 
    BLECharacteristic::PROPERTY_NOTIFY | 
    BLECharacteristic::PROPERTY_INDICATE
  );

  // Create a BLE Descriptor
  pCharacteristic->addDescriptor(new BLE2902());

  // Start the service
  pService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);  // set value to 0x00 to not advertise this parameter
  BLEDevice::startAdvertising();
  Serial.println("Waiting for a client connection to notify...");
}

void loop() {
    uint16_t error;
    char errorMessage[256];

    int16_t hcho, humidity, temperature;
    error = sfa3x.readMeasuredValues(hcho, humidity, temperature);

    if (error) {
        Serial.print("Error trying to execute readMeasuredValues(): ");
        errorToString(error, errorMessage, 256);
        Serial.println(errorMessage);
    } else {
        // 計算 HCHO 濃度（ppm）
        float hchoValue = hcho / 5.0 / 1000.0;
        
        // 根據濃度大小決定顯示的小數位數
        String hchoValueStr;
        if (hchoValue < 10) {
            hchoValueStr = String(hchoValue, 5);  // 包括小數點在內的5位
            // 確保顯示4位小數
            if (hchoValueStr.length() < 6) {  // 如果不足6位（包括小數點）
                while (hchoValueStr.length() < 6) {
                    hchoValueStr += "0";  // 在末尾添加0
                }
            }
            hchoValueStr = hchoValueStr.substring(0, 6);  // 取前6位（包括小數點）
        } else if (hchoValue < 100) {
            hchoValueStr = String(hchoValue, 3);
            hchoValueStr = hchoValueStr.substring(0, 5);  // 取前5位（包括小數點）
        } else {
            hchoValueStr = String(hchoValue, 2);
            hchoValueStr = hchoValueStr.substring(0, 5);  // 取前5位（包括小數點）
        }

        String humidityValueStr = String(humidity / 100.0, 1);
        String temperatureValueStr = String(temperature / 200.0, 1);

        // 清除先前的數值
        tft.setFont(&FreeSans18pt7b);
        tft.setTextSize(1);
        tft.fillRect(45, 22, 105, 36, ST77XX_WHITE);
        // 顯示新的HCHO數值
        tft.setCursor(45, 54);
        tft.setTextColor(ST77XX_BLACK);
        tft.print(hchoValueStr);
        //tft.setFont();
        //tft.setTextSize(2);
        tft.setFont(&FreeSans9pt7b);
        tft.fillRect(25,0, 50, 16, ST77XX_WHITE);  //x,y,w,h
        tft.setCursor(35, 14);
        tft.print(temperatureValueStr);
        tft.fillRect(105,0, 50, 16, ST77XX_WHITE);  //x,y,w,h
        tft.setCursor(115, 14);
        tft.print(humidityValueStr);
        // 使用逗號分隔數據
        String dataString = hchoValueStr + "," + humidityValueStr + "," + temperatureValueStr + "," + String(sendCount++);

        // 通過 BLE 發送數據
        if (deviceConnected) {
            pCharacteristic->setValue(dataString.c_str());
            pCharacteristic->notify();
            Serial.println("Notified: " + dataString);
        }

        // 輸出到串口以進行調試
        Serial.print("發送數據: ");
        Serial.println(dataString);
    }

    // 處理 BLE 連接狀態
    if (!deviceConnected && oldDeviceConnected) {
        delay(500); // 給藍牙堆疊一些時間準備
        pServer->startAdvertising(); // 重新開始廣播
        Serial.println("Start advertising");
        oldDeviceConnected = deviceConnected;
    }
    if (deviceConnected && !oldDeviceConnected) {
        oldDeviceConnected = deviceConnected;
    }

    delay(1000);
}
