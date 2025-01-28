#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <PulseSensorPlayground.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "time.h"


#include "addons/TokenHelper.h"  
#include "addons/RTDBHelper.h"   


#define WIFI_SSID "NAVEEN"
#define WIFI_PASSWORD "Jnm00000"


#define API_KEY "AIzaSyBA89tVhyGDsYjSHMFO_NcabYtTx6VctSs"
#define USER_EMAIL "rsnaveensiva2004@gmail.com"
#define USER_PASSWORD "rsnaveen23"
#define DATABASE_URL "https://heart-rate-and-temperature-default-rtdb.asia-southeast1.firebasedatabase.app/"


FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
String uid;
String databasePath;
String heartRatePath = "/heartRate";
String tempPath = "/temperature";
String timePath = "/timestamp";

const int pulsePin = 34;       
const int oneWireBus = 14;    

PulseSensorPlayground pulseSensor;
OneWire oneWire(oneWireBus);
DallasTemperature sensors(&oneWire);

unsigned long sendDataPrevMillis = 0;
unsigned long timerDelay = 5000; 

const char* ntpServer = "pool.ntp.org";

void initSensors() {
  pulseSensor.analogInput(pulsePin);
  pulseSensor.begin();
  pulseSensor.setThreshold(550);  
}

void initWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println();
  Serial.print("Connected! IP Address: ");
  Serial.println(WiFi.localIP());
}

unsigned long getTime() {
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return 0;
  }
  time(&now);
  return now;
}

void setup() {
  Serial.begin(115200);
  initSensors();
  initWiFi();
  configTime(0, 0, ntpServer);

  config.api_key = API_KEY;
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  config.database_url = DATABASE_URL;

  Firebase.reconnectWiFi(true);
  fbdo.setResponseSize(4096);
  config.token_status_callback = tokenStatusCallback;
  Firebase.begin(&config, &auth);

  Serial.println("Getting User UID...");
  while (auth.token.uid == "") {
    delay(1000);
  }
  uid = auth.token.uid.c_str();
  Serial.print("User UID: ");
  Serial.println(uid);

  databasePath = "/UsersData/" + uid + "/readings";
}

void loop() {

  if (Firebase.ready() && (millis() - sendDataPrevMillis > timerDelay || sendDataPrevMillis == 0)) {
    sendDataPrevMillis = millis();

    unsigned long timestamp = getTime();
    Serial.print("Timestamp: ");
    Serial.println(timestamp);

    String parentPath = databasePath + "/" + String(timestamp);

    int heartRate = pulseSensor.getBeatsPerMinute();

    sensors.requestTemperatures();
    float temperature = sensors.getTempCByIndex(0);

    if (heartRate > 0) {
      Serial.print("Heart Rate: ");
      Serial.print(heartRate);
      Serial.println(" BPM");
    } else {
      Serial.println("No heartbeat detected.");
    }

    if (temperature != DEVICE_DISCONNECTED_C) {
      Serial.print("Temperature: ");
      Serial.print((temperature*(9/5) ));
      Serial.println(" °F");
    } else {
      Serial.println("Temperature sensor disconnected.");
    }

    FirebaseJson json;
    json.set(heartRatePath.c_str(), String(heartRate));
    json.set(tempPath.c_str(), String((temperature*(9/5) + 52)));
    json.set(timePath.c_str(), String(timestamp));

    Serial.printf("Uploading data... %s\n", Firebase.RTDB.setJSON(&fbdo, parentPath.c_str(), &json) ? "Success" : fbdo.errorReason().c_str());
  }
}
