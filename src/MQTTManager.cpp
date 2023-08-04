#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include "MQTTManager.h"

// MQTT broker details
const char *mqttServer = "server01";
const int mqttPort = 1883;
const char *mqttUser = "your_mqtt_user";
const char *mqttPassword = "your_mqtt_password";
WiFiClient espClient;
PubSubClient mqttClient(espClient);
const String baseSensorTopic = "homeassistant/sensor/PoolCtrl";

bool MQTTManager::isConnected()
{
    bool connected = true;
    int retries = 0;
    Serial.println("Checking MQTT connection...");
    while (retries <= 3 && !mqttClient.connected())
    {
        Serial.println("Attempting MQTT connection...");
        mqttClient.setServer(mqttServer, mqttPort);
        if (mqttClient.connect("PoolController"))
        {
            Serial.println("Connection to mqtt server: " + String(mqttServer) + " established");
            connected = true;
        }
        else
        {
            Serial.println("MQTT connection failed with state rc=: " + mqttClient.state());
            Serial.println("Retries " + retries++);
            Serial.println("Retrying in one second");
            connected = false;
            delay(2000);
        }
    }
    return connected;
}

void MQTTManager::disconnect()
{
    mqttClient.disconnect();
}

void MQTTManager::sendDiscovery(String sensorId, float temperature)
{
    // This is the discovery topic for this specific sensor
    String stateTopic = baseSensorTopic + sensorId + "/state";
    String configTopic = baseSensorTopic + sensorId + "/config";

    DynamicJsonDocument doc(1024);
    char buffer[256];

    doc["dev_cla"] = "temperature";
    doc["name"] = sensorId;
    doc["stat_t"] = stateTopic;
    doc["unit_of_meas"] = "°C";
    doc["frc_upd"] = true;
    doc["temperature"] = temperature;
    doc["val_tpl"] = "{{ value_json.temperature|default(0) }}";

    size_t n = serializeJson(doc, buffer);

    mqttClient.publish(configTopic.c_str(), buffer, n);
}

void MQTTManager::sendTemp(String sensorId, float temperature)
{
    String stateTopic = baseSensorTopic + sensorId + "/state";
    DynamicJsonDocument doc(1024);
    char buffer[256];

    doc["temperature"] = temperature;

    size_t n = serializeJson(doc, buffer);

    mqttClient.publish(stateTopic.c_str(), buffer, n);
}

void MQTTManager::callback(char *topic, byte *payload, unsigned int length)
{
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    for (unsigned int i = 0; i < length; i++)
    {
        Serial.print((char)payload[i]);
    }
    Serial.println();
}
