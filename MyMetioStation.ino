
#include <Arduino.h>
#include <Arduino_JSON.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

#define DHTPIN      2
#define DHTTYPE DHT11


extern char* rootCACertificate;
extern String BearerToken;

extern void setClock(); //Set a time for ssl session
extern void dhtInit(sensor_t *sensor, uint32_t *dlayMS);
extern String getSensorData(DHT_Unified dht);
String baseUri = "https://evo-bot.online/api";

WiFiMulti WiFiMulti;
DHT_Unified dht(DHTPIN, DHTTYPE);

uint32_t dlayMS;
String SN = "003201";
float lastTemperature = 0;
float lastHumidity = 0;
int ctr = 31;
bool auth = true;
bool dataChange = false;

void setup() {
  uint32_t chipId = 0;

  Serial.begin(115200);
  // Serial.setDebugOutput(true);

  Serial.println();
  Serial.println();
  Serial.println();

  WiFi.mode(WIFI_STA);
  WiFiMulti.addAP("", "");//WiFi SSID , WiFi Password here

  for (int i = 0; i < 17; i = i + 8) {
    chipId |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
  }
  SN = SN + String(chipId);

  Serial.printf("ESP32 Chip model = %s Rev %d\n", ESP.getChipModel(), ESP.getChipRevision());
  Serial.printf("This chip has %d cores\n", ESP.getChipCores());
  Serial.print("Chip ID: "); Serial.println(SN);
  Serial.print("Waiting for WiFi to connect...");
  while ((WiFiMulti.run() != WL_CONNECTED)) {
    Serial.print(".");
  }

  sensor_t sensor;
  dhtInit(&sensor, &dlayMS);
  Serial.println(" connected");

  setClock();
}

void loop() {
  String jsonString = getSensorData(dht);
  String apiMethod = "/sensor/meteo-data";
  String httpMethod = "POST";
  if (auth == false) {
    apiMethod = "/sensor/meteo/new-sensor";
    httpMethod = "POST";
    auth = true;
  }
  Serial.println(dataChange);
  if (ctr >= 30 or dataChange == true) {
    httpMethod = "POST";
    ctr = 0;
    dataChange = false;
  } else {
    httpMethod = "PATCH";
  }

  WiFiClientSecure *client = new WiFiClientSecure;
  if (client) {
    client -> setCACert(rootCACertificate);

    {
      // Add a scoping block for HTTPClient https to make sure it is destroyed before WiFiClientSecure *client is
      HTTPClient https;

      //Serial.print(rootCACertificate);
      Serial.print("[HTTPS] begin...\n");

      if (https.begin(*client, baseUri + apiMethod)) {  // HTTPS

        Serial.print("[HTTPS] POST...\n");

        https.addHeader("Content-Type", "application/json");
        https.addHeader("Accept", "application/json");
        https.addHeader("X-serial-number", SN);
        https.addHeader("X-chip-model", ESP.getChipModel());
        https.addHeader("X-chip-rev", String(ESP.getChipRevision()));
        https.addHeader("Authorization", BearerToken);
        // start connection and send HTTP header
        char buffer[100];
        jsonString.toCharArray(buffer, 100);
        int httpCode = 0;
        if (httpMethod == "POST") {
          httpCode = https.POST(buffer);
        } else if (httpMethod == "PATCH") {
          httpCode = https.PATCH(buffer);
        }
        Serial.println(httpMethod);


        // httpCode will be negative on error
        if (httpCode > 0) {
          // HTTP header has been send and Server response header has been handled
          Serial.printf("[HTTPS] POST... code: %d\n", httpCode);

          // file found at server
          if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
            String payload = https.getString();
            Serial.println(payload);
            ctr++;
          } else if (httpCode == 422) {
            auth = false;
            Serial.printf("Device not added, try add device");
          }
        } else {

          Serial.printf("[HTTPS] POST... failed, error: %s\n", https.errorToString(httpCode).c_str());
        }

        https.end();
      } else {
        Serial.printf("[HTTPS] Unable to connect\n");
      }

      // End extra scoping block
    }

    delete client;
  } else {
    Serial.println("Unable to create client");
  }

  Serial.println();
  Serial.println("Waiting 10s before the next round...");
  delay(10000);
}
