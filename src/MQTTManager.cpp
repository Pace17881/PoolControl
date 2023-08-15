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

// The String must not longer than the below ones.
const String baseSensorTopic = "homeassistant/sensor/mcu/";
const String baseSwitchTopic = "homeassistant/switch/mcu/";
// const String baseSensorTopic = "homeassistant/sensor/poolmcu/";
// const String baseSwitchTopic = "homeassistant/switch/poolmcu/";

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
        mqttClient.setServer(mqttServer, mqttPort);
        if (mqttClient.connect("PoolController"))
        {
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

void MQTTManager::sendDataToTopic(String &topic, DynamicJsonDocument &data)
{
    if (mqttClient.connected())
    {
        size_t jsonSize = measureJson(data);

        Serial.print("JSON size: ");
        Serial.println(jsonSize);

        char buffer[256];
        size_t n = serializeJson(data, buffer);

        Serial.print("Sending discovery data for ");
        Serial.println(topic); // Überprüfe, ob das generierte Thema korrekt ist
        Serial.println(buffer);
        bool state = mqttClient.publish(topic.c_str(), buffer, n);
        Serial.println(state);
    }
    else
    {
        Serial.println("No connection");
        Serial.println(topic);
    }
}

void MQTTManager::sendTempDiscovery(String entity, float temperature)
{
    // This is the discovery topic for this specific sensor
    String stateTopic = baseSensorTopic + entity + "/state";
    String topic = baseSensorTopic + entity + "/config";

    DynamicJsonDocument doc(256);

    char temp[10];
    dtostrf(temperature, 5, 2, temp);

    doc["dev_cla"] = "temperature";
    doc["name"] = entity;
    doc["stat_t"] = stateTopic;
    doc["unit_of_meas"] = "°C";
    doc["frc_upd"] = true;
    doc["temperature"] = temp;
    doc["val_tpl"] = "{{ value_json.temperature|default(0) }}";

    sendDataToTopic(topic, doc);
}

void MQTTManager::sendTemp(String entity, float temperature)
{
    String topic = baseSensorTopic + entity + "/state";
    DynamicJsonDocument doc(256);

    char temp[10];
    dtostrf(temperature, 5, 2, temp);
    doc["temperature"] = temp;

    sendDataToTopic(topic, doc);
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
    String topic = baseSensorTopic + entity + "/config";
    String stateTopic = baseSensorTopic + entity + "/state";
    String commandTopic = baseSensorTopic + entity + "/set";

    DynamicJsonDocument doc(1024);

    doc["name"] = entity;
    doc["stat_t"] = stateTopic;
    doc["cmd_t"] = commandTopic;
    doc["state_open"] = "Heating";
    doc["state_closed"] = "Cleaning";
    doc["frc_upd"] = true;

    sendDataToTopic(topic, doc);
}

void MQTTManager::sendMotorDirection(String entity, bool motorDirectionSwitch)
{
    String topic = baseSensorTopic + entity + "/state";
    if (motorDirectionSwitch)
    {
        mqttClient.publish(topic.c_str(), "Heating");
    }
    else
    {
        mqttClient.publish(topic.c_str(), "Cleaning");
    }
}

void MQTTManager::switchOutlet(String entity, const char *state)
{
    String topic = "cmnd/" + entity + "/POWER";
    Serial.println(topic);
    mqttClient.publish(topic.c_str(), state);
}

void MQTTManager::sendAutomaticStateDiscovery(String entity)
{
    String topic = baseSwitchTopic + entity + "/config";
    String stateTopic = baseSwitchTopic + entity + "/state";
    String commandTopic = baseSwitchTopic + entity + "/set";
    String availTopic = baseSwitchTopic + entity + "/availability";

    DynamicJsonDocument doc(1024);

    doc["name"] = entity;
    doc["stat_t"] = stateTopic;
    doc["cmd_t"] = commandTopic;
    doc["uniq_id"] = "switch_" + entity;

    sendDataToTopic(topic, doc);
}

void MQTTManager::sendAutomaticStateValue(String entity, bool mode)
{
    String topic = baseSwitchTopic + entity + "/set";
    String payload = mode ? "ON" : "OFF";
    mqttClient.publish(topic.c_str(), payload.c_str(), true);
}

void MQTTManager::setup()
{
    mqttClient.setCallback(callback);
}

void MQTTManager::loop()
{
    mqttClient.loop();
}