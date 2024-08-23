#include "homefans.h"

void setup_wifi() {
  delay(10);

  // Connect to WiFi
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  randomSeed(micros());
  
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void transmitState(int fanId, char* attr, char* payload) {

  mySwitch.disableReceive();                // Receiver off
  mySwitch.enableTransmit(TX_PIN);          // Transmit on
  mySwitch.setRepeatTransmit(RF_REPEATS);   // set RF code repeat
  mySwitch.setProtocol(RF_PROTOCOL);        // send Received Protocol
  ELECHOUSE_cc1101.SetTx();                 // set Transmit on
  
  // Generate and send the RF payload to the fan
  int rfCommand = generateCommand(fanId, attr, payload);
  mySwitch.send(rfCommand, 12);
  
  #if DEBUG_MODE
    Serial.print("(RF) OUT [protocol: ");
    Serial.print(RF_PROTOCOL);
    Serial.print("] [repeats: ");
    Serial.print(RF_REPEATS);
    Serial.print("] [frequency: ");
    Serial.print(RF_FREQUENCY);
    Serial.print("] [fan: ");
    Serial.print(fanId);
    Serial.print("] [attr: ");
    Serial.print(attr);
    Serial.print("] [payload: ");
    Serial.print(payload);
    Serial.print("] ");
    Serial.print(rfCommand);
    Serial.println();
  #endif
  
  mySwitch.disableTransmit();             // set Transmit off
  ELECHOUSE_cc1101.SetRx();               // set Receive on
  
  mySwitch.enableReceive(RX_PIN);         // Receiver on
  
  postStateUpdate(fanId);
}

void callback(char* topic, byte* payload, unsigned int length) {
  #if DEBUG_MODE
    Serial.print("(MQTT) IN [");
    Serial.print(topic);
    Serial.print("] ");
    for (int i = 0; i < length; i++) {
      Serial.print((char) payload[i]);
    }
    Serial.println();
  #endif
  
  char payloadChar[length + 1];
  sprintf(payloadChar, "%s", payload);
  payloadChar[length] = '\0';
  
  // Get ID after the base topic + a slash
  char id[5];
  memcpy(id, &topic[sizeof(BASE_TOPIC)], 4);
  id[4] = '\0';
  if (strspn(id, idchars)) {
    uint8_t idint = strtol(id, (char**) NULL, 2);
    char *attr;
    char *action;
    // Split by slash after ID in topic to get attribute and action
    attr = strtok(topic + sizeof(BASE_TOPIC) + 5, "/");
    action = strtok(NULL, "/");
    // Convert payload to lowercase
    for (int i = 0; payloadChar[i]; i++) {
      payloadChar[i] = tolower(payloadChar[i]);
    }
    
    bool lightChange = true;

    // Sync tracked fan states based on the incomming MQTT message
    if (strcmp(attr, "speed") == 0) { // Fan Speed (low, med, high, off)
      if (strcmp(payloadChar, "low") == 0) {
        fans[idint].fanSpeed = FAN_LOW;
      } else if (strcmp(payloadChar, "medium") == 0 || strcmp(payloadChar, "med") == 0) {
        fans[idint].fanSpeed = FAN_MED;
      } else if (strcmp(payloadChar, "high") == 0) {
        fans[idint].fanSpeed = FAN_HI;
      } else if (strcmp(payloadChar, "off") == 0) {
        fans[idint].fanSpeed = FAN_OFF;
      }
    } else if (strcmp(attr, "light") == 0) { // Fan Light State (On/Off)
      if (strcmp(payloadChar, "on") == 0) {
        lightChange = !fans[idint].lightState;
        fans[idint].lightState = true;
      } else if (strcmp(payloadChar, "off") == 0) {
        lightChange = fans[idint].lightState;
        fans[idint].lightState = false;
      }
    }
    
    if (strcmp(action, "set") == 0 && lightChange) {
      transmitState(idint, attr, payloadChar);
    }
  }
  else {
    // Invalid ID
    return;
  }
}

void postStateUpdate(int id) {
  char outTopic[100];
  
  // Publish "Fan Speed" state
  sprintf(outTopic, "%s/%s/speed/state", BASE_TOPIC, idStrings[id]);
  client.publish(outTopic, fanStateTable[fans[id].fanSpeed], true);
  #if DEBUG_MODE
    Serial.print("(MQTT) OUT [");
    Serial.print(outTopic);
    Serial.print("] ");
    Serial.print(fanStateTable[fans[id].fanSpeed]);
    Serial.println();
  #endif
  
  // Publish "Fan Light" state
  sprintf(outTopic, "%s/%s/light/state", BASE_TOPIC, idStrings[id]);
  client.publish(outTopic, fans[id].lightState ? "on" : "off", true);
  #if DEBUG_MODE
    Serial.print("(MQTT) OUT [");
    Serial.print(outTopic);
    Serial.print("] ");
    Serial.print(fans[id].lightState ? "on":"off");
    Serial.println();
  #endif
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(MQTT_CLIENT_NAME, MQTT_USER, MQTT_PASS, STATUS_TOPIC, 0, true, "offline")) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish(STATUS_TOPIC, "online", true);
      // ... and resubscribe
      client.subscribe(SUBSCRIBE_TOPIC_SPEED_SET);
      client.subscribe(SUBSCRIBE_TOPIC_SPEED_STATE);
      client.subscribe(SUBSCRIBE_TOPIC_LIGHT_SET);
      client.subscribe(SUBSCRIBE_TOPIC_LIGHT_STATE);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

int generateCommand(int fanId, char* attr, char* payload) {
  int baseCommand = 0b000000000000;
  int fanIdDips   = (fanId ^ 0b1111) << 7;
  int command     = 0b0000000;

  if (strcmp(attr, "speed") == 0) { // Handle "Fan Speed" commands
    if(strcmp(payload, "low") == 0) {
      command = 0b1110111;
    } else if(strcmp(payload, "medium") == 0 || strcmp(payload, "med") == 0) {
      command = 0b1101111;
    } else if(strcmp(payload, "high") == 0) {
      command = 0b1011111;
    } else if(strcmp(payload, "off") == 0) {
      command = 0b1111101;
    } else {
      Serial.print("Unsupported payload: (attr: ");
      Serial.print(attr);
      Serial.print(") ");
      Serial.println(payload);
    }
  } else if(strcmp(attr, "light") == 0) { // Handle "Fan Light" commands
      command = 0b1111110; // No difference in command between light on and light off; this must be tracked in software
  } else { // Handle all other commands (i.e. unknown commands)
    Serial.print("Unsupported command: (attr: ");
    Serial.print(attr);
    Serial.print(") ");
    Serial.println(payload);
  }

  // Combine all values together to create our final command
  int finalCommand = baseCommand + fanIdDips + command;
  return finalCommand;
}

void setup() {
  Serial.begin(9600);
  cooldown = 0;
  
  // initialize fan struct
  for(int i=0; i<16; i++) {
    fans[i].lightState = false;
    fans[i].fanSpeed = FAN_LOW;
  }
  
  ELECHOUSE_cc1101.Init();
  ELECHOUSE_cc1101.setMHZ(RF_FREQUENCY);
  ELECHOUSE_cc1101.SetRx();
  mySwitch.enableReceive(RX_PIN);
  
  setup_wifi();

  // https://github.com/esp8266/Arduino/blob/master/libraries/ArduinoOTA/ArduinoOTA.h
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_FS
      type = "filesystem";
    }
    Serial.println("Start updating " + type);
  });
  
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });

  ArduinoOTA.setPort(8266);
  ArduinoOTA.begin();
  
  client.setServer(MQTT_HOST, MQTT_PORT).setCallback(callback);

  if (!client.connected()) reconnect();

  postStateUpdate(DEFAULT_FAN_ID);
}

void loop() {
  // Allow for OTA updating
  ArduinoOTA.handle();
  
  // Ensure that the MQTT client is connected
  if (!client.connected()) {
    reconnect();
  }

  client.loop();
  
  // Handle received rf-transmissions
  if (mySwitch.available()) {
    value               = mySwitch.getReceivedValue();        // int value of rf command
    prot                = mySwitch.getReceivedProtocol();     // transmission protocol used (expect 11)
    bits                = mySwitch.getReceivedBitlength();    // transmission bit length (expect 12)
    int id = (value >> 7) ^0b01111;

    #if DEBUG_MODE
      Serial.print("(RF) IN [protocol: ");
      Serial.print(prot);
      Serial.print("] [value: ");
      Serial.print(value);
      Serial.print("] [bits: ");
      Serial.print(bits);
      Serial.println("]");
     #endif
    
    // Ensure that the protocol and bit-length are what we expect to see
    if (prot == 11 && bits == 12 && millis() - cooldown > MIN_COOLDOWN) {
      cooldown = millis();
      // Strip the first 5 bits from the "value" above to be left with the 7-bit command
      //  - 1111110 (126)  Light toggle
      //  - 1111101 (125)  Fan speed 0 (off)
      //  - 1110111 (119)  Fan speed 1 (low)
      //  - 1101111 (111)  Fan speed 2 (medium)
      //  - 1011111 (095)  Fan speed 3 (high)
      int command = value & 0b000001111111;

      // Ensure that the command if for one of our remote IDs (0-15)
      if (id < 16) {
        switch (command) {
          case 126: // Light toggle
            fans[id].lightState = !fans[id].lightState;
            break;
          case 125: // Fan speed 0 (off)
            fans[id].fanSpeed = FAN_OFF;
            break;
          case 119: // Fan speed 1 (low)
            fans[id].fanSpeed = FAN_LOW;
            break;
          case 111: // Fan speed 2 (medium)
            fans[id].fanSpeed = FAN_MED;
            break;
          case 95: // Fan speed 3 (high)
            fans[id].fanSpeed = FAN_HI;
            break;
          default:
            Serial.print("Unknown command received: ");
            Serial.println(value);
        }Serial.println(fans[id].lightState);
        postStateUpdate(id);
      }
    }
    
    mySwitch.resetAvailable();
  }
}

// Fin.
