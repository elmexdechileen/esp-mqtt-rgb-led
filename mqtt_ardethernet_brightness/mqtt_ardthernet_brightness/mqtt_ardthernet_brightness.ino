/*
 * ESP8266 MQTT Lights for Home Assistant.
 *
 * This file is for single-color lights.
 *
 * See https://github.com/corbanmailloux/esp-mqtt-rgb-led
 */

// Set configuration options for pins, WiFi, and MQTT in the following file:
#include "config.h"

// https://github.com/bblanchon/ArduinoJson
#include <ArduinoJson.h>

#include <Ethernet.h>

// http://pubsubclient.knolleary.net/
#include <PubSubClient.h>

// DHT temp sensor support
#include <DHT.h>

const bool debug_mode = CONFIG_DEBUG;
const bool led_invert = CONFIG_INVERT_LED_LOGIC;

//const char* mqtt_server = CONFIG_MQTT_HOST; 
// works only in this format, otherwise -2 error 
// when connecting to MQTT broker.
IPAddress mqtt_server(192, 168, 1, 145);
const char* mqtt_username = CONFIG_MQTT_USER;
const char* mqtt_password = CONFIG_MQTT_PASS;
const char* client_id = CONFIG_MQTT_CLIENT_ID;

// Topics
const char* light_state_topic = CONFIG_MQTT_TOPIC_STATE;
const char* light_set_topic = CONFIG_MQTT_TOPIC_SET;
const char* dht_topic = CONFIG_DHT_TOPIC;

const char* on_cmd = CONFIG_MQTT_PAYLOAD_ON;
const char* off_cmd = CONFIG_MQTT_PAYLOAD_OFF;

const int BUFFER_SIZE = JSON_OBJECT_SIZE(8);

// Maintained state for reporting to HA
byte red = 255;

byte brightness = 255;

// Real values to write to the LEDs (ex. including brightness and state)
byte realRed = 0;

bool stateOn = false;

// Globals for fade/transitions
bool startFade = false;
unsigned long lastLoop = 0;
int transitionTime = 0;
bool inFade = false;
int loopCount = 0;
int stepR;
int redVal; 

// Pin of interest
int pin;

// Get possible pins for output channels PWM
int iopins[] = CONFIG_PWM_PINS;
const int nInputs = sizeof(iopins)/sizeof(int);

// DHT settings
DHT dht(CONFIG_DHT_DELAY, CONFIG_DHT_TYPE); //// Initialize DHT sensor for normal 16mhz Arduino
float hum;  //Stores humidity value
float temp; //Stores temperature value
long DhtTimer;

// Globals for flash
bool flash = false;
bool startFlash = false;
int flashLength = 0;
unsigned long flashStartTime = 0;
byte flashRed = red;
byte flashBrightness = brightness;

byte mac[] = {
  0x00, 0xAA, 0xBB, 0xCC, 0xDE, 0x02
};

// Set up Ethernet client
EthernetClient ethernetClient;

// And MQTT client
PubSubClient client(ethernetClient);

void setup() {
  if (debug_mode) {
    Serial.begin(9600);
    Serial.println("DEBUG MODE");
  }
  
  
  // start the Ethernet connection:
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    // no point in carrying on, so do nothing forevermore:
    for (;;)
      ;
  }
  
  // Set pins to listen on and fill label string
  for (int i = 0; i < nInputs; i = i + 1) {
    pinMode(iopins[i], OUTPUT);
  }

  // For debug purposes only, just to stop the
  // coil whine of my psu...
  analogWrite(3, 255);
  
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  // Setup dht
  dht.begin();
}

 /*
  SAMPLE PAYLOAD:
    {
      "brightness": 120,
      "flash": 2,
      "transition": 5,
      "state": "ON"
    }
  */
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");

  char message[length + 1];
  for (int i = 0; i < length; i++) {
    message[i] = (char)payload[i];
  }
  message[length] = '\0';
  Serial.println(message);

  if (!processJson(message)) {
    return;
  }

  if (stateOn) {
    // Update lights
    realRed = map(red, 0, 255, 0, brightness);
  }
  else {
    realRed = 0;
  }

  startFade = true;
  inFade = false; // Kill the current fade

  sendState();
}

bool processJson(char* message) {
  StaticJsonBuffer<BUFFER_SIZE> jsonBuffer;

  JsonObject& root = jsonBuffer.parseObject(message);

  if (!root.success()) {
    Serial.println("parseObject() failed");
    return false;
  }

  if (root.containsKey("channel")) {
    pin = (int)root["channel"];
  } else {
    // Error when no channel is specified
    Serial.println("No channel specified in payload!");
    return false;
  }

  if (root.containsKey("state")) {
    if (strcmp(root["state"], on_cmd) == 0) {
      stateOn = true;
    }
    else if (strcmp(root["state"], off_cmd) == 0) {
      stateOn = false;
    }
  }

  if (root.containsKey("flash") ||
       (root.containsKey("effect") && strcmp(root["effect"], "flash") == 0)) {

    if (root.containsKey("flash")) {
      flashLength = (int)root["flash"] * 1000;
    } else {
      flashLength = CONFIG_DEFAULT_FLASH_LENGTH * 1000;
    }

    if (root.containsKey("brightness")) {
      flashBrightness = root["brightness"];
    }
    else {
      flashBrightness = brightness;
    }

    flashRed = map(flashRed, 0, 255, 0, flashBrightness);
   
    flash = true;
    startFlash = true;
  }
  else { // Not flashing
    flash = false;

    if (root.containsKey("brightness")) {
      brightness = root["brightness"];
    }

    if (root.containsKey("transition")) {
      transitionTime = root["transition"];
    }
    else {
      transitionTime = 0;
    }
  }

  return true;
}

void sendState() {
  StaticJsonBuffer<BUFFER_SIZE> jsonBuffer;

  JsonObject& root = jsonBuffer.createObject();

  root["state"] = (stateOn) ? on_cmd : off_cmd;

  root["brightness"] = brightness;

  root["channel"] = pin;

  char buffer[root.measureLength() + 1];
  root.printTo(buffer, sizeof(buffer));

  // Loads of operations just to get it to a char array
  String topic = (String)light_state_topic + "/" + (String)pin;
  Serial.println(topic);
  int str_len = topic.length() + 1; 
  char char_array[str_len];

  topic.toCharArray(char_array, str_len);

  client.publish(char_array, buffer, true);
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect (no password test)
    if (client.connect(client_id, mqtt_username, mqtt_password)) {
    //if (client.connect(client_id)) {
      Serial.println("connected");
      client.subscribe(light_set_topic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setColor(int col) {
  if (led_invert) {
    col = (255 - col);
  }
  
  analogWrite(pin, col);
  Serial.println("Setting light:");
  Serial.println(col);

}

void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  if (flash) {
    if (startFlash) {
      startFlash = false;
      flashStartTime = millis();
    }

    if ((millis() - flashStartTime) <= flashLength) {
      if ((millis() - flashStartTime) % 1000 <= 500) {
        setColor(flashRed);
      }
      else {
        setColor(0); 
      }
    }
    else {
      flash = false;
      setColor(realRed);
    }
  }

  if (startFade) {
    Serial.println(transitionTime);
    // If we don't want to fade, skip it.
    if (transitionTime == 0) {
      setColor(realRed);

      redVal = realRed;

      startFade = false;
    }
    else {
      loopCount = 0;
      stepR = calculateStep(redVal, realRed);

      inFade = true;
    }
  }

  if (inFade) {
    startFade = false;
    unsigned long now = millis();
    if (now - lastLoop > transitionTime) {
      if (loopCount <= 1020) {
        lastLoop = now;

        redVal = calculateVal(stepR, redVal, loopCount);

        setColor(redVal); // Write current values to LED pins
        //setColor(brightness);
        
        Serial.print("Loop count: ");
        Serial.println(loopCount);
        loopCount++;
      }
      else {
        inFade = false;
      }
    }
  }

  // DHT Routine
  if ( DhtTimer < millis()) {
    // If timer has expired get temp and humidity and send mqtt message
    hum = dht.readHumidity();
    temp = dht.readTemperature();


    // Send the message
    StaticJsonBuffer<BUFFER_SIZE> jsonBuffer;

    JsonObject& root = jsonBuffer.createObject();
  
    root["temperature"] = temp;
  
    root["humidity"] = hum;
 
    char buffer[root.measureLength() + 1];
    root.printTo(buffer, sizeof(buffer));

    // Publish to topic
    client.publish(dht_topic, buffer, true);

    // Reset timer
    DhtTimer = millis() + CONFIG_DHT_DELAY;
  } 
}

// From https://www.arduino.cc/en/Tutorial/ColorCrossfader

int calculateStep(int prevValue, int endValue) {
    int step = endValue - prevValue; // What's the overall gap?
    if (step) {                      // If its non-zero,
        step = 1020/step;            //   divide by 1020
    }

    return step;
}

/* The next function is calculateVal. When the loop value, i,
*  reaches the step size appropriate for one of the
*  colors, it increases or decreases the value of that color by 1.
*  (R, G, and B are each calculated separately.)
*/
int calculateVal(int step, int val, int i) {
    if ((step) && i % step == 0) { // If step is non-zero and its time to change a value,
        if (step > 0) {              //   increment the value if step is positive...
            val += 1;
        }
        else if (step < 0) {         //   ...or decrement it if step is negative
            val -= 1;
        }
    }

    // Defensive driving: make sure val stays in the range 0-255
    if (val > 255) {
        val = 255;
    }
    else if (val < 0) {
        val = 0;
    }

    return val;
}
