/* 
 *  Use Akizuki Denshi Micro SD Card DIP Kit AE-MICRO-SD-DIP
 *  Connect the SD card to the following pins:
 *
 *  SD Card | ESP32
 *  1   D2       -
 *  2   D3       SS(5)
 *  3   CMD      MOSI(23)
 *  4   VDD      3.3V
 *  5   CLK      SCK(18)
 *  6   VSS      GND
 *  7   D0       MISO(19)
 *  8   D1       -
 * 
 * 
 */
/* 
 *  参考
 *  - ファイル>スケッチ例>ESP32 Dev Module 用のスケッチ例>SD(esp32)>SD_Test
 *  - ファイル>スケッチ例>ESP32 Dev Module 用のスケッチ例>ESP32>Time>SimpleTime
 *  - https://soar130650.net/2021/02/11/esp32-cmicrosd/
 *  - https://diysmarthome.hatenablog.com/entry/2022/12/03/174133
 *  - https://soar130650.net/2021/02/17/esp32-ntp/
 *  - https://www.mulong.me/tech/electronics/esp32-rtc-ntp/
 */

#include <SPI.h>
#include <SD.h>
#include <DHT20.h>
#include <WiFi.h>
#include "time.h"
#include "esp_sntp.h"

#include "secrets.h"

const char* ssid       = WIFI_SSID;  // defined in secrets.h
const char* password   = WIFI_PASS;  // defined in secrets.h

const char* ntpServer = "ntp.nict.jp";
const long  gmtOffset_sec = 3600 * 9;  // UTC+9
const int   daylightOffset_sec = 0;  // summer time

bool readDone = false;
bool ntpDone = false;

DHT20 dht20 = DHT20();

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  // connect to WiFi
  Serial.printf("Connecting to %s ", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" CONNECTED");
  
  // get time from ntp server
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  
  // wait until RTC is synced
  while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET) {
    Serial.print(".");
    delay(1000);
  }

  // disconnect WiFi as it's no longer needed
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  Serial.println("WiFi DISCONNECTED");

  // initialize SD card
  if (!SD.begin()) {
    Serial.println("Card Mount Failed");
    return;
  }
  
  uint8_t cardType = SD.cardType();

  if(cardType == CARD_NONE){
    Serial.println("No SD card attached");
    return;
  }

  // initialize DHT20 temperature and humidity sensor
  dht20.begin(21, 22);  // ESP32 default pin: SDA(21), SCL(22)
}

void loop() {
  
  // get current time
  struct tm timeinfo;
  getLocalTime(&timeinfo);

  // once every 10sec
  if (timeinfo.tm_sec % 10 == 0) {

    // check read done flag in order not to read twice or more
    if (!readDone) {
      
      // set read done flag
      readDone = true;
      
      // read temperature and humidity. It takes 50ms
      if (dht20.read() == DHT20_OK) {
        float humidity = dht20.getHumidity();
        float temperature = dht20.getTemperature();
        
        char timeString[20] = "2000/01/01 00:00:00";
        snprintf(timeString, 20, "%04d/%02d/%02d %02d:%02d:%02d",
          timeinfo.tm_year + 1900,
          timeinfo.tm_mon + 1,
          timeinfo.tm_mday,
          timeinfo.tm_hour,
          timeinfo.tm_min,
          timeinfo.tm_sec
        );
        
        String dataString;
        dataString += String(timeString) + ",";
        dataString += String(temperature) + ",";
        dataString += String(humidity);
        
        // open the file (must need '/' before file name)
        File dataFile = SD.open("/datalog.txt", FILE_APPEND);
      
        // if the file is available, write to it:
        if (dataFile) {
          dataFile.println(dataString);
          dataFile.close();
          Serial.println(dataString);
        // if the file isn't open, pop up an error:
        } else {
          Serial.println("error opening datalog.txt");
        }
      }
    }
    
  } else {
    readDone = false;
  }

  delay(10);
}
