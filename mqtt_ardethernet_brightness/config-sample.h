/*
 * This is a sample configuration file for the "mqtt_esp8266_brightness" light.
 *
 * Change the settings below and save the file as "config.h"
 * You can then upload the code using the Arduino IDE.
 */

 
/* I2C Pins for different boards
 *
 *    Board;	I2C / TWI pins
 *    Uno, Ethernet;	A4 (SDA), A5 (SCL)
 *    Mega2560;	20 (SDA), 21 (SCL)
 *    Leonardo;	2 (SDA), 3 (SCL)
 *    Due;	20 (SDA), 21 (SCL), SDA1, SCL1
 */ 
 
// Pins
#define CONFIG_PIN_LIGHT 0

// WiFi
#define CONFIG_WIFI_SSID "{WIFI-SSID}"
#define CONFIG_WIFI_PASS "{WIFI-PASSWORD}"

// MQTT
#define CONFIG_MQTT_HOST "{MQTT-SERVER}"
#define CONFIG_MQTT_USER "{MQTT-USERNAME}"
#define CONFIG_MQTT_PASS "{MQTT-PASSWORD}"

#define CONFIG_MQTT_CLIENT_ID "ESPBrightnessLED" // Must be unique on the MQTT network

// MQTT Topics
#define CONFIG_MQTT_TOPIC_STATE "home/brightness1"
#define CONFIG_MQTT_TOPIC_SET "home/brightness1/set"

#define CONFIG_MQTT_PAYLOAD_ON "ON"
#define CONFIG_MQTT_PAYLOAD_OFF "OFF"

// Miscellaneous
// Default number of flashes if no value was given
#define CONFIG_DEFAULT_FLASH_LENGTH 2

// Reverse the LED logic
// false: 0 (off) - 255 (bright)
// true: 255 (off) - 0 (bright)
#define CONFIG_INVERT_LED_LOGIC false

// Enables Serial and print statements
#define CONFIG_DEBUG false

// i2c settings
#define I2C_IN_ADDR   16 >> 1 // I2C-INPUT-Addresse als 7 Bit
#define I2C_OUT_ADDR 176 >> 1 // I2C-OUTPUT-Addresse als 7 Bit (all jumpers to off position)

