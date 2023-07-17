#include <PubSubClient.h>
#include <ESP8266WiFi.h>

// MQTT broker details
const char* mqttServer = "server01";
const int mqttPort = 1883;
const char* mqttUser = "your_mqtt_user";
const char* mqttPassword = "your_mqtt_password";

WiFiClient espClient;
PubSubClient mqttClient(espClient);

class MQTTManager {
public:
    void setup()
    {
        mqttClient.setServer(mqttServer, mqttPort);
        // If your MQTT broker requires authentication, uncomment the following line and provide the credentials
        // mqttClient.setCredentials(mqttUser, mqttPassword);
    }

    void reconnect()
    {
        while (!mqttClient.connected()) {
            Serial.print("Attempting MQTT connection...");
            if (mqttClient.connect("ESP8266Client")) {
                Serial.println("connected");
            } else {
                Serial.print("failed, rc=");
                Serial.print(mqttClient.state());
                Serial.println(" retrying in 5 seconds");
                delay(5000);
            }
        }
    }

    void publish(const char* topic, float temperature)
    {
        char message[10];
        dtostrf(temperature, 4, 2, message);
        mqttClient.publish(topic, message);
    }

    void loop()
    {
        if (!mqttClient.connected()) {
            reconnect();
        }
        mqttClient.loop();
    }
};
