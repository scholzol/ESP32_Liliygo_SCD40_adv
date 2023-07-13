// #include section begin -------------------------------------------------------------------------------------------

#include <Arduino.h>

#include <DFRobot_SCD4X.h>
#include "SPI.h"
#include "TFT_eSPI.h"
#include "Free_Fonts.h"
// #include "esp_adc_cal.h"
#include <credentials.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "Button2.h"
#include "bmp.h"
#include <esp_task_wdt.h>

// #include section end ============================================================================================

// define global variables begin -----------------------------------------------------------------------------------
#define WDT_TIMEOUT 30      // wdt 30s, if something goes wrong, the ESP32 will be restarted after 30s

#define ADC_EN              14  //ADC_EN is the ADC detection enable port
#define ADC_PIN             34
#define BUTTON_1            35
#define BUTTON_2            0

Button2 btn1(BUTTON_1);
Button2 btn2(BUTTON_2);

DFRobot_SCD4X SCD4X(&Wire, /*i2cAddr = */SCD4X_I2C_ADDR);
DFRobot_SCD4X::sSensorMeasurement_t data;

TFT_eSPI tft = TFT_eSPI();

int btnCick = true;
int vref = 1130;
float battery_voltage;
unsigned long previousMillis = 0;
unsigned long interval = 60000;

String HOST_NAME = "http://192.168.178.49"; // change to your MySQL server's IP address
String PATH_NAME   = "/insert_cond.php";

// define global variables end ======================================================================================

// helper functions begin ------------------------------------------------------------------------------------------------------

//! Long time delay, it is recommended to use shallow sleep, which can effectively reduce the current consumption
void espDelay(int ms)
{
    esp_sleep_enable_timer_wakeup(ms * 1000);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
    esp_light_sleep_start();
}

void showData()
{
  tft.setTextDatum(ML_DATUM);
  tft.fillScreen(TFT_BLACK);
  
  Serial.print("Carbon dioxide concentration : ");
  Serial.print(data.CO2ppm);
  Serial.print(" ppm");
  tft.drawString("CO2 (ppm):", 0, 10, GFXFF);
  tft.drawNumber(data.CO2ppm, 180, 10, GFXFF);

  Serial.print("  Environment temperature : ");
  Serial.print(data.temp);
  Serial.print(" C");
  tft.drawString("Temp (°C):", 0, 40, GFXFF);
  tft.drawFloat(data.temp, 2, 180, 40, GFXFF);

  Serial.print("  Relative humidity : ");
  Serial.print(data.humidity);
  Serial.println(" RH");
  tft.drawString("RH (%):", 0, 70, GFXFF);
  tft.drawFloat(data.humidity, 2, 180, 70, GFXFF);

  String voltage = "Voltage :" + String(battery_voltage) + "V";
  tft.drawString(voltage, 0, 100, GFXFF);
}

void button_init()
{
    btn2.setLongClickHandler([](Button2 & b) {
        btnCick = false;
        int r = digitalRead(TFT_BL);
        tft.fillScreen(TFT_BLACK);
        tft.setTextColor(TFT_GREEN, TFT_BLACK);
        tft.setTextDatum(MC_DATUM);
        tft.drawString("Double Press again to wake up",  tft.width() / 2, tft.height() / 2 );
        // espDelay(6000);
        digitalWrite(TFT_BL, !r);

        tft.writecommand(TFT_DISPOFF);
        tft.writecommand(TFT_SLPIN);
        //After using light sleep, you need to disable timer wake, because here use external IO port to wake up
        // esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_TIMER);
        // esp_sleep_enable_ext1_wakeup(GPIO_SEL_35, ESP_EXT1_WAKEUP_ALL_LOW);
        // esp_sleep_enable_ext0_wakeup(GPIO_NUM_35, 0);
        delay(200);
        // esp_deep_sleep_start();
    });
    btn2.setDoubleClickHandler([](Button2 & b) {
        tft.writecommand(TFT_SLPOUT);
        tft.writecommand(TFT_DISPON);
        tft.fillScreen(TFT_BLACK);
        tft.setTextColor(TFT_GREEN, TFT_BLACK);
        tft.setTextDatum(MC_DATUM);
        tft.drawString("TFT On",  tft.width() / 2, tft.height() / 2 );
        Serial.println("TFT On");
        btnCick = true;

    });
}

void button_loop()
{
    btn1.loop();
    btn2.loop();
}

// helper functions end ======================================================================================

// setup begin ------------------------------------------------------------------------------------------------------

void setup(void)
{
  Serial.begin(115200);

// Init the sensor SCD40

  while( !SCD4X.begin() ){
    Serial.println("Communication with device failed, please check connection");
    delay(3000);
  }

  SCD4X.enablePeriodMeasure(SCD4X_STOP_PERIODIC_MEASURE);
  SCD4X.setTempComp(4.0);
  SCD4X.setSensorAltitude(270);
  SCD4X.enablePeriodMeasure(SCD4X_START_PERIODIC_MEASURE);

  Serial.println("Begin ok!");

// init the TFT display

  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_GREEN);
  tft.setTextSize(1);
  tft.setFreeFont(FF6);

// init the buttons

  button_init();

// init the Webserver and the HTTPClient

  WiFi.begin(ssid, password);

// init the task watch dog timer
  esp_task_wdt_init(WDT_TIMEOUT, true); //enable panic so ESP32 restarts
  esp_task_wdt_add(NULL); //add current thread to WDT watch
}

// setup end =============================================================================================

// loop begin --------------------------------------------------------------------------------------------

void loop()
{
  unsigned long currentMillis = millis();
  uint16_t v = analogRead(ADC_PIN);
  battery_voltage = ((float)v / 4095.0) * 2.0 * 3.3 * (vref / 1000.0);

  esp_task_wdt_reset(); // reset the watch dog timer

  button_loop();
  // read the data from the sensor and show on TFT and serial monitor
  if (currentMillis - previousMillis >= interval) {
    if(SCD4X.getDataReadyStatus()) {
      SCD4X.readMeasurement(&data);
      if (btnCick) {
          showData();
      }
      else
      {
        while( !SCD4X.begin() ){
          delay(3000);
        }
        SCD4X.enablePeriodMeasure(SCD4X_STOP_PERIODIC_MEASURE);
        SCD4X.setTempComp(4.0);
        SCD4X.setSensorAltitude(270);
        SCD4X.enablePeriodMeasure(SCD4X_START_PERIODIC_MEASURE);
      }
      

      // Serial.println();
    }

    // transmit the data to the webserver

    String queryString = "?T=" + String(data.temp) + "&RH=" + String(data.humidity) + "&CO2=" + String(data.CO2ppm) + "&Voltage=" + String(battery_voltage);
    String URL = HOST_NAME + PATH_NAME + queryString;
    Serial.println(URL);

    HTTPClient http;
    http.begin(URL); //HTTP
    int httpCode = http.GET();

    // httpCode will be negative on error
    if(httpCode > 0) {
      // file found at server
      if(httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        Serial.println(payload);
      } else {
        // HTTP header has been send and Server response header has been handled
        Serial.printf("[HTTP] GET... code: %d\n", httpCode);
      }
    } else {
      Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }
    http.end();
    previousMillis = currentMillis;
  }
}
// loop end =============================================================================================
