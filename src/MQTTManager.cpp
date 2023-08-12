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
//const String baseSensorTopic = "homeassistant/sensor/PoolCtrl";
const String baseSensorTopic = "homeassistant/sensor/poolmcu/";

MQTTManager::MQTTManager()
{
    mqttClient.setCallback(callback);
}

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

void MQTTManager::sendTempDiscovery(String entity, float temperature)
{
    // This is the discovery topic for this specific sensor
    String stateTopic = baseSensorTopic + entity + "/state";
    String configTopic = baseSensorTopic + entity + "/config";

    DynamicJsonDocument doc(1024);
    char buffer[256];

    char temp[10];
    dtostrf(temperature, 5, 2, temp);

    doc["dev_cla"] = "temperature";
    doc["name"] = entity;
    doc["stat_t"] = stateTopic;
    doc["unit_of_meas"] = "°C";
    doc["frc_upd"] = true;
    doc["temperature"] = temp;
    doc["val_tpl"] = "{{ value_json.temperature|default(0) }}";

    size_t n = serializeJson(doc, buffer);

    mqttClient.publish(configTopic.c_str(), buffer, n);
}

void MQTTManager::sendTemp(String entity, float temperature)
{
    String stateTopic = baseSensorTopic + entity + "/state";
    DynamicJsonDocument doc(1024);
    char buffer[256];

    char temp[10];
    dtostrf(temperature, 5, 2, temp);

    doc["temperature"] = temp;

    size_t n = serializeJson(doc, buffer);

    mqttClient.publish(stateTopic.c_str(), buffer, n);
}

void MQTTManager::callback(char *topic, byte *payload, unsigned int length)
{
    Serial.print("Nachricht empfangen [");
    Serial.print(topic);
    Serial.print("] ");

    // Konvertiere die empfangene Nachricht in einen String
    String message = "";
    for (unsigned int i = 0; i < length; i++)
    {
        message += (char)payload[i];
    }

    Serial.println(message);
}

void MQTTManager::sendMotorDiscovery(String entity)
{
    String configTopic = baseSensorTopic + entity + "/config";
    String stateTopic = baseSensorTopic + entity + "/state";
    String commandTopic = baseSensorTopic + entity + "/set";

    DynamicJsonDocument doc(1024);
    char buffer[256];

    doc["name"] = entity;
    doc["stat_t"] = stateTopic;
    doc["cmd_t"] = commandTopic;
    doc["state_on"] = "Heating";
    doc["state_off"] = "Cleaning";
    doc["frc_upd"] = true;

    size_t n = serializeJson(doc, buffer);

    mqttClient.publish(configTopic.c_str(), buffer, n);
}

void MQTTManager::sendMotorDirection(String entity, bool motorDirectionSwitch)
{
    String stateTopic = baseSensorTopic + entity + "/state";
    if (motorDirectionSwitch)
    {
        mqttClient.publish(stateTopic.c_str(), "Heating");
    }
    else
    {
        mqttClient.publish(stateTopic.c_str(), "Cleaning");
    }
}