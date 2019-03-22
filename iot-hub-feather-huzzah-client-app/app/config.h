// Physical device information for board and sensor
#define DEVICE_ID "Feather HUZZAH ESP8266 WiFi"
#define DHT_TYPE DHT22

//Sound detector configs
#define PIN_GATE_IN 2
//#define IRQ_GATE_IN  0
//#define PIN_LED_OUT 13
#define PIN_ANALOG_IN A0

// Pin layout configuration
#define LED_PIN 0
//#define DHT_PIN 2

#define TEMPERATURE_ALERT 30

// Interval time(ms) for sending message to IoT Hub
#define INTERVAL 1000

// If don't have a physical DHT sensor, can send simulated data to IoT hub
#define SIMULATED_DATA false

// EEPROM address configuration
#define EEPROM_SIZE 512

// SSID and SSID password's length should < 32 bytes
// http://serverfault.com/a/45509
#define SSID_LEN 32
#define PASS_LEN 32
#define CONNECTION_STRING_LEN 256

#define MESSAGE_MAX_LEN 256
