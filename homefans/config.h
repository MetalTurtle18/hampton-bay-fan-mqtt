// ===================================================================
// Configure WiFi and MQTT
// ===================================================================
#define WIFI_SSID "ENTER_WIFI_SSID"
#define WIFI_PASS "ENTER_WIFI_SSID_PASS"
#define MQTT_HOST "192.168.0.60"
#define MQTT_PORT 1883
#define MQTT_USER ""
#define MQTT_PASS ""
#define MQTT_CLIENT_NAME "HAMPTONBAY"
#define BASE_TOPIC "home/fans"


// ===================================================================
// MQTT Topics
// ===================================================================
#define LOGGING_TOPIC BASE_TOPIC "/log"
#define STATUS_TOPIC BASE_TOPIC "/status"
#define SUBSCRIBE_TOPIC_SPEED_SET BASE_TOPIC "/+/speed/set"
#define SUBSCRIBE_TOPIC_SPEED_STATE BASE_TOPIC "/+/speed/state"
#define SUBSCRIBE_TOPIC_LIGHT_SET BASE_TOPIC "/+/light/set"
#define SUBSCRIBE_TOPIC_LIGHT_STATE BASE_TOPIC "/+/light/state"


// ===================================================================
// Configure CC1101
// ===================================================================
// 303.631 - (original) determined from FAN-9T remote tramsmissions
// 303.875 - (personal - rniemand) this seems to work for my fans
// 303.848 - (personal - metalturtle) this seems to work for my fans
#define RF_FREQUENCY    303.848
#define RF_PROTOCOL     11
#define RF_REPEATS      8
#define RF_PULSE_LENGTH 320


// ===================================================================
// Optional configuration
// ===================================================================
#define DEBUG_MODE true
