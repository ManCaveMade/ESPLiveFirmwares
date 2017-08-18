// Based on https://github.com/tzapu/SonoffBoilerplate

#define   BUTTON                    0
#define   RELAY_AUX                 12
#define   RELAY_HEATER              13
#define   TEMP_PIN                  5 //pin connected to temp sensor (for control)
//#define   TEMP_PIN_AUX              4 //pin for auxilliary temp sensor, comment if not used
//#define   FLOW_PIN                  4 //pin connected to flow sensor, comment if not used
#define   ADC_MUX_PIN            16 //LOW is CT, high is direct

#define INCLUDE_POWER_MEASUREMENT

#define HOSTNAME "esplive-thermostat" //will be appended with ESP serial
#define WIFISSID "Ubernet"
#define WIFIPASSWORD "BDD5A42640"

//comment out to completly disable respective technology
#define INCLUDE_MQTT_SUPPORT


/********************************************
   Should not need to edit below this line *
 * *****************************************/
#include <ESP8266WiFi.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#ifdef INCLUDE_MQTT_SUPPORT
#include <PubSubClient.h>        //https://github.com/Imroy/pubsubclient

WiFiClient wclient;
PubSubClient mqttClient(wclient);

static bool MQTT_ENABLED              = true;
int         lastMQTTConnectionAttempt = 0;
#endif

#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager

#ifdef INCLUDE_POWER_MEASUREMENT
// https://bitbucket.org/xoseperez/emonliteesp
#include "EmonLiteESP.h"
EmonLiteESP power;

// Aanalog GPIO on the ESP8266
#define CURRENT_PIN             0

// If you are using a nude ESP8266 board it will be 1.0V, if using a NodeMCU there
// is a voltage divider in place, so use 3.3V instead.
#define REFERENCE_VOLTAGE       1.0

// Precision of the ADC measure in bits. Arduinos and ESP8266 use 10bits ADCs, but the
// ADS1115 is a 16bits ADC
#define ADC_BITS                10

// This is basically the volts per amper ratio of your current measurement sensor.
// If your sensor has a voltage output it will be written in the sensor enclosure,
// something like "30V 1A", otherwise it will depend on the burden resistor you are
// using.
#define CURRENT_RATIO           0.1

// Number of samples each time you measure
#define SAMPLES_X_MEASUREMENT   1000

// Time between readings, this is not specific of the library but on this sketch
#define POWER_MEASUREMENT_INTERVAL    5000 //10s
#endif

#define TEMPERATURE_MEASUREMENT_INTERVAL    1000 //10s

#include <EEPROM.h>

#define EEPROM_SALT 12661
typedef struct {
  char  bootState[4]      = "on";
  char  mqttHostname[33]  = "esp-thermostat";
  char  mqttPort[6]       = "1883";
  char  mqttClientID[24]  = "esplive-thermostat";
  char  mqttTopic[33]     = HOSTNAME;
  char  mqttUsername[24]  = "hass";
  char  mqttPassword[24]  = "HAss7412369";
  int   temperatureSP     = 25; //set point degrees C
  float mainsRMSVoltage   = 230.0; //measure with multimeter
  int   salt              = EEPROM_SALT;
} WMSettings;

WMSettings settings;

#include <ArduinoOTA.h>


//for LED status
#include <Ticker.h>
Ticker ticker;

const int CMD_WAIT = 0;
const int CMD_BUTTON_CHANGE = 1;

int cmd = CMD_WAIT;
//int relayState = HIGH;

//inverted button state
int buttonState = HIGH;

static long startPress = 0;

OneWire oneWireTemp(TEMP_PIN);
DallasTemperature ds(&oneWireTemp);
bool dsIsConnected = false;
double dsTemperature = 0;
char dsTemperatureStr[6];

const char* getDSTemperatureStr() {
  if (!dsIsConnected)
    return "NOT CONNECTED";

  return dsTemperatureStr;
}

#ifdef TEMP_PIN_AUX
OneWire oneWireTempAux(TEMP_PIN_AUX);
DallasTemperature dsAux(&oneWireTempAux);
bool dsAuxIsConnected = false;
float dsAuxTemperature = 0;
char dsAuxTemperatureStr[6];

const char* getDSAuxTemperatureStr() {
  if (!dsAuxIsConnected)
    return "NOT CONNECTED";

  return dsAuxTemperatureStr;
}
#endif

void temperatureLoop() {
  static unsigned long last_check = 0;
  static bool requested = false;

  if ((millis() - last_check) > TEMPERATURE_MEASUREMENT_INTERVAL) { //must be 750ms or more
    if (requested == true) {
      dsTemperature = ds.getTempCByIndex(0);
    }
    ds.requestTemperatures();
    requested = true; //just for the first time
    last_check = millis();

    if (isnan(dsTemperature) || dsTemperature < -50) {
      Serial.println(F("Error reading ds sensor"));
    } else {
      dsIsConnected = true;
      dtostrf(dsTemperature, 5, 1, dsTemperatureStr);

      Serial.print(F("Temperature now: "));
      Serial.print(dsTemperatureStr);
      Serial.println(F("C"));
    }
  }
}

#ifdef FLOW_PIN
float lastFlow;
#endif


#ifdef INCLUDE_POWER_MEASUREMENT
unsigned int currentCallback() {
  return analogRead(CURRENT_PIN);
}

void powerMonitorSetup() {
  power.initCurrent(currentCallback, ADC_BITS, REFERENCE_VOLTAGE, CURRENT_RATIO);
}

void powerMonitorLoop() {
  static unsigned long last_check = 0;

  if ((millis() - last_check) > POWER_MEASUREMENT_INTERVAL) {
    #ifdef ADC_MUX_PIN
    digitalWrite(ADC_MUX_PIN, LOW);
    delayMicroseconds(100); //delay 100us for settle
    #endif
  
    float current = power.getCurrent(SAMPLES_X_MEASUREMENT);

    Serial.print(F("[ENERGY] Power now: "));
    Serial.print(int(current * settings.mainsRMSVoltage));
    Serial.println(F("W"));

    last_check = millis();
  }
}
#endif

//http://stackoverflow.com/questions/9072320/split-string-into-string-array
String getValue(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++) {
    if (data.charAt(i) == separator || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }

  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

void tick()
{
  
}

//gets called when WiFiManager enters configuration mode
void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());
  //entered config mode, make led toggle faster
  ticker.attach(0.2, tick);
}

void updateMQTT(int channel) {
#ifdef INCLUDE_MQTT_SUPPORT
  char topic[50];
  sprintf(topic, "%s/channel-%d/status", settings.mqttTopic, channel);
  mqttClient.publish(topic, "lol");
#endif
}




//flag for saving data
bool shouldSaveConfig = false;

//callback notifying us of the need to save config
void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}


void restart() {
  //TODO turn off relays before restarting
  ESP.reset();
  delay(1000);
}

void reset() {
  //reset settings to defaults
  //TODO turn off relays before restarting
  /*
    WMSettings defaults;
    settings = defaults;
    EEPROM.begin(512);
    EEPROM.put(0, settings);
    EEPROM.end();
  */
  //reset wifi credentials
  WiFi.disconnect();
  delay(1000);
  ESP.reset();
  delay(1000);
}

#ifdef INCLUDE_MQTT_SUPPORT
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print(topic);
  Serial.print(" => ");
  
  
  String stopic = String(topic);
  String spayload = String((char*)payload);
  Serial.println(spayload);

  if (stopic.startsWith(settings.mqttTopic)) {
    Serial.println("for this device");
    stopic = stopic.substring(strlen(settings.mqttTopic) + 1);
    String channelString = getValue(stopic, '/', 0);
    if (!channelString.startsWith("channel-")) {
      Serial.println("no channel");
      return;
    }
    channelString.replace("channel-", "");
    int channel = channelString.toInt();
    Serial.println(channel);
    if (spayload == "on") {
      
    }
    if (spayload == "off") {
      
    }
    if (spayload == "toggle") {
      
    }
    if (spayload == "") {
      updateMQTT(channel);
    }
  }
}

#endif


void setup()
{
  Serial.begin(115200);

  // start ticker with 0.5 because we start in AP mode and try to connect
  ticker.attach(0.6, tick);

  char hostname[32];
  sprintf(hostname, "%s_%06X", HOSTNAME, ESP.getChipId());
  Serial.println(hostname);

  WiFiManager wifiManager;
  //set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
  wifiManager.setAPCallback(configModeCallback);

  //timeout - this will quit WiFiManager if it's not configured in 3 minutes, causing a restart
  wifiManager.setConfigPortalTimeout(180);

  //custom params
  EEPROM.begin(512);
  EEPROM.get(0, settings);
  EEPROM.end();

  if (settings.salt != EEPROM_SALT) {
    Serial.println("Invalid settings in EEPROM, trying with defaults");
    WMSettings defaults;
    settings = defaults;
  }


  WiFiManagerParameter custom_boot_state("boot-state", "on/off on boot", settings.bootState, 33);
  wifiManager.addParameter(&custom_boot_state);


  Serial.println(settings.bootState);

#ifdef INCLUDE_MQTT_SUPPORT
  Serial.println(settings.mqttHostname);
  Serial.println(settings.mqttPort);
  Serial.println(settings.mqttClientID);
  Serial.println(settings.mqttTopic);
  Serial.println(settings.mqttUsername);
  Serial.println(settings.mqttPassword);

  WiFiManagerParameter custom_mqtt_text("<br/>MQTT config. <br/> No url to disable.<br/>");
  wifiManager.addParameter(&custom_mqtt_text);

  WiFiManagerParameter custom_mqtt_hostname("mqtt-hostname", "Hostname", settings.mqttHostname, 33);
  wifiManager.addParameter(&custom_mqtt_hostname);

  WiFiManagerParameter custom_mqtt_port("mqtt-port", "port", settings.mqttPort, 6);
  wifiManager.addParameter(&custom_mqtt_port);

  WiFiManagerParameter custom_mqtt_client_id("mqtt-client-id", "Client ID", settings.mqttClientID, 24);
  wifiManager.addParameter(&custom_mqtt_client_id);

  WiFiManagerParameter custom_mqtt_topic("mqtt-topic", "Topic", settings.mqttTopic, 33);
  wifiManager.addParameter(&custom_mqtt_topic);

  WiFiManagerParameter custom_mqtt_username("mqtt-username", "Username", settings.mqttUsername, 24);
  wifiManager.addParameter(&custom_mqtt_username);

  WiFiManagerParameter custom_mqtt_password("mqtt-password", "Password", settings.mqttPassword, 24);
  wifiManager.addParameter(&custom_mqtt_password);
#endif

  //set config save notify callback
  wifiManager.setSaveConfigCallback(saveConfigCallback);

  if (!wifiManager.autoConnect(hostname)) {
    Serial.println("failed to connect and hit timeout");
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(1000);
  }

  //save the custom parameters to FS
  if (shouldSaveConfig) {
    Serial.println("Saving config");

    strcpy(settings.bootState, custom_boot_state.getValue());

#ifdef INCLUDE_MQTT_SUPPORT
    strcpy(settings.mqttHostname, custom_mqtt_hostname.getValue());
    strcpy(settings.mqttPort, custom_mqtt_port.getValue());
    strcpy(settings.mqttClientID, custom_mqtt_client_id.getValue());
    strcpy(settings.mqttTopic, custom_mqtt_topic.getValue());
    strcpy(settings.mqttUsername, custom_mqtt_username.getValue());
    strcpy(settings.mqttPassword, custom_mqtt_password.getValue());
#endif

    Serial.println(settings.bootState);

    EEPROM.begin(512);
    EEPROM.put(0, settings);
    EEPROM.end();
  }

#ifdef INCLUDE_MQTT_SUPPORT
  //config mqtt
  if (strlen(settings.mqttHostname) == 0) {
    MQTT_ENABLED = false;
  }
  if (MQTT_ENABLED) {
    mqttClient.setServer(settings.mqttHostname, atoi(settings.mqttPort));
  }
#endif

  //OTA
  ArduinoOTA.onStart([]() {
    Serial.println("Start OTA");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.setHostname(hostname);
  ArduinoOTA.begin();

  //if you get here you have connected to the WiFi
  Serial.println("connected...yeey :)");
  ticker.detach();

  #ifdef ADC_MUX_PIN
  pinMode(ADC_MUX_PIN, OUTPUT);
  digitalWrite(ADC_MUX_PIN, LOW);
  #endif

  //setup button
  pinMode(BUTTON, INPUT);

  //setup relays
  pinMode(RELAY_AUX, OUTPUT);
  pinMode(RELAY_HEATER, OUTPUT);

  if (strcmp(settings.bootState, "on") == 0) {
    
  } else {
    
  }

  //setup power
#ifdef INCLUDE_POWER_MEASUREMENT
  powerMonitorSetup();
#endif

  //setup temperature
  ds.begin();
  ds.setWaitForConversion(false);

#ifdef TEMP_PIN_AUX
  dsAux.begin();
  dsAux.setWaitForConversion(false);
#endif

  //setup flow
#ifdef FLOW_PIN

#endif

  Serial.println("done setup");
}


void loop()
{

  //ota loop
  ArduinoOTA.handle();

#ifdef INCLUDE_MQTT_SUPPORT
  //mqtt loop
  if (MQTT_ENABLED) {
    if (!mqttClient.connected()) {
      if (lastMQTTConnectionAttempt == 0 || millis() > lastMQTTConnectionAttempt + 3 * 60 * 1000) {
        lastMQTTConnectionAttempt = millis();
        Serial.println(millis());
        Serial.println("Trying to connect to mqtt");
        if (mqttClient.connect(settings.mqttClientID)) {
          mqttClient.setCallback(mqttCallback);
          char topic[50];
          //sprintf(topic, "%s/+/+", settings.mqttTopic);
          //mqttClient.subscribe(topic);
          sprintf(topic, "%s/+", settings.mqttTopic);
          mqttClient.subscribe(topic);

          //TODO multiple relays
          updateMQTT(0);
        } else {
          Serial.println("failed");
        }
      }
    } else {
      mqttClient.loop();
    }
  }
#endif

#ifdef INCLUDE_POWER_MEASUREMENT
  powerMonitorLoop();
#endif

}




