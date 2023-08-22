#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include "MQTTManager.h"
#include <cstring>

// MQTT broker details
const char *mqttServer = "server01";
const int mqttPort = 1883;

WiFiClient espClient;
PubSubClient mqttClient(espClient);

const String mSensorTopic;
const String mSwitchTopic;
const String mModeTopic;

bool automatic = false;
bool direction = false;

bool mqttConnected = false;

void MQTTManager::subscribe()
{
    mqttClient.subscribe(mModeTopic.c_str());
}

bool MQTTManager::isAutomatic()
{
    return automatic;
}

bool MQTTManager::isSolar()
{
    return direction;
}

bool MQTTManager::connect()
{
    if (!mqttClient.connected())
    {
        // Serial.println("Connecting to MQTT server...");
        mqttClient.setServer(mqttServer, mqttPort);
        if (mqttClient.connect("PoolController"))
        {
            // Serial.println("Connected to MQTT server");
            subscribe();
            mqttConnected = true;
        }
        else
        {
            Serial.println("Failed to connect to MQTT server");
            mqttConnected = false;
        }
    }
    return mqttConnected;
}

void MQTTManager::disconnect()
{
    mqttClient.disconnect();
}

void MQTTManager::sendTempDiscovery(String entity, float temperature)
{
    // This is the discovery topic for this specific sensor
    String stateTopic = mSensorTopic + entity + "/state";
    String configTopic = mSensorTopic + entity + "/config";

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
    String stateTopic = mSensorTopic + entity + "/state";
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
    Serial.println(topic);
    String message = "";

    for (unsigned int i = 0; i < length; i++)
    {
        message += (char)payload[i];
    }
    Serial.println(message);

    switch (message.toInt())
    {
    case 0: // Manual cleaning
        direction = false;
        automatic = false;
        break;
    case 1: // Manual heating
        direction = true;
        automatic = false;
        break;
    case 2: // Automatic
        automatic = true;
        break;
    default:
        Serial.printf("%s is not a valid mode, Try 0,1,2", message.c_str());
        break;
    }
}

void MQTTManager::sendMotorDiscovery(String entity)
{
    String configTopic = mSensorTopic + entity + "/config";
    String stateTopic = mSensorTopic + entity + "/state";
    String commandTopic = mSensorTopic + entity + "/set";

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

void MQTTManager::sendMotorDirection(String entity, bool isHeating)
{
    String stateTopic = mSensorTopic + entity + "/state";
    if (isHeating)
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
    String configTopic = mSwitchTopic + entity + "/config";
    String stateTopic = mSwitchTopic + entity + "/state";
    String commandTopic = mSwitchTopic + entity + "/set";
    String availTopic = mSwitchTopic + entity + "/availability";

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
    String stateTopic = mSwitchTopic + entity + "/set";
    String payload = mode ? "ON" : "OFF";
    mqttClient.publish(stateTopic.c_str(), payload.c_str(), true);
}

void MQTTManager::setup()
{
    mqttClient.setCallback([this](char *topic, byte *payload, unsigned int length)
                           { this->callback(topic, payload, length); });
}

void MQTTManager::loop()
{
    mqttClient.loop();
}