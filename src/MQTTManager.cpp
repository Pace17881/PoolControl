#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include "MQTTManager.h"
#include <cstring>

// MQTT broker details
const char *mqttServer = "server01";
const int mqttPort = 1883;
const char *mqttUser = "your_mqtt_user";
const char *mqttPassword = "your_mqtt_password";
WiFiClient espClient;
PubSubClient mqttClient(espClient);
const String baseSensorTopic = "homeassistant/sensor/poolmcu_debug/";
const String baseSwitchTopic = "homeassistant/switch/poolmcu_debug/";
//const String baseSensorTopic = "homeassistant/sensor/poolmcu/";
//const String baseSwitchTopic = "homeassistant/switch/poolmcu/";

bool automatic = false;

void MQTTManager::subscribe()
{
    mqttClient.subscribe("poolAutomaticMode");
}

bool MQTTManager::getAutomaticState()
{
    return automatic;
}

bool MQTTManager::connect()
{
    if (!mqttClient.connected())
    {
        Serial.println("Connecting to MQTT server...");
        mqttClient.setServer(mqttServer, mqttPort);
        if (mqttClient.connect("PoolController"))
        {
            Serial.println("Connected to MQTT server");
            mqttClient.subscribe("poolAutomaticMode");
            return true;
        }
        else
        {
            Serial.println("Failed to connect to MQTT server");
            return false;
        }
    }
    return true;
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

void callback(char *topic, byte *payload, unsigned int length)
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

    if (message.equals("0"))
    {
        automatic = false;
    }
    else
    {
        automatic = true;
    }
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
    doc["state_open"] = "Heating";
    doc["state_closed"] = "Cleaning";
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

void MQTTManager::switchOutlet(String topic, const char *state)
{
    String command = "cmnd/" + topic + "/POWER";
    Serial.println(command);
    mqttClient.publish(command.c_str(), state);
}

void MQTTManager::sendAutomaticStateDiscovery(String entity)
{
    String configTopic = baseSwitchTopic + entity + "/config";
    String stateTopic = baseSwitchTopic + entity + "/state";
    String commandTopic = baseSwitchTopic + entity + "/set";
    String availTopic = baseSwitchTopic + entity + "/availability";

    DynamicJsonDocument doc(1024);
    char buffer[256];

    doc["name"] = entity;
    doc["stat_t"] = stateTopic;
    doc["cmd_t"] = commandTopic;
    doc["uniq_id"] = "switch_" + entity;

    size_t n = serializeJson(doc, buffer);

    mqttClient.publish(configTopic.c_str(), buffer, n);
}

void MQTTManager::sendAutomaticStateValue(String entity, bool mode)
{
    String stateTopic = baseSwitchTopic + entity + "/set";
    String payload = mode ? "ON" : "OFF";
    mqttClient.publish(stateTopic.c_str(), payload.c_str(), true);
}

void MQTTManager::setup()
{
    mqttClient.setCallback(callback);
}

void MQTTManager::loop()
{
    mqttClient.loop();
}