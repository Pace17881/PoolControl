#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>

// MQTT broker details
const char *mqttServer = "server01";
const int mqttPort = 1883;
const char *mqttUser = "your_mqtt_user";
const char *mqttPassword = "your_mqtt_password";

WiFiClient espClient;
PubSubClient mqttClient(espClient);

const String baseSensorTopic = "homeassistant/sensor/PoolCtrl";
//const String baseSwitchTopic = "homeassistant/switch/PoolCtrl";
void callback(char* topic, byte* payload, unsigned int length);

class MQTTManager
{
public:
    void setup()
    {
        mqttClient.setServer(mqttServer, mqttPort);
        // If your MQTT broker requires authentication, uncomment the following line and provide the credentials
        // mqttClient.setCredentials(mqttUser, mqttPassword);
        //mqttClient.setCallback(callback);
    }

    void reconnect()
    {
        while (!mqttClient.connected())
        {
            Serial.print("Attempting MQTT connection...");
            if (mqttClient.connect("ESP8266Client"))
            {
                Serial.println("connected");
                
            }
            else
            {
                Serial.print("failed, rc=");
                Serial.print(mqttClient.state());
                Serial.println(" retrying in 5 seconds");
                delay(5000);
            }
        }
    }

    void sendDiscoveryTemp(String sensorId, float temperature)
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

    // void sendDiscoverySwitch(String sensorId, String dev_cla)
    // {
    //     // This is the discovery topic for this specific sensor
    //     String configTopic = baseSensorTopic + sensorId + "/config";
    //     String stateTopic = baseSensorTopic + sensorId + "/state";
    //     String commandTopic = baseSensorTopic + sensorId + "/set";

    //     DynamicJsonDocument doc(1024);
    //     char buffer[256];

    //     doc["name"] = sensorId;
    //     doc["command_topic"] = commandTopic;
    //     doc["state_topic"] = stateTopic;
    //     doc["qos"] = 0;
    //     doc["payload_on"] = "ON";
    //     doc["payload_off"] = "OFF";
    //     doc["retain"] = true;

    //     size_t n = serializeJson(doc, buffer);

    //     mqttClient.publish(configTopic.c_str(), buffer, true);
    // }

    void publishTemperature(String sensorId, float temperature)
    {
        String stateTopic = baseSensorTopic + sensorId + "/state";
        DynamicJsonDocument doc(1024);
        char buffer[256];

        doc["temperature"] = temperature;

        size_t n = serializeJson(doc, buffer);

        mqttClient.publish(stateTopic.c_str(), buffer, n);
    }

    void callback(char* topic, byte* payload, unsigned int length)
    {
        Serial.print("Message arrived [");
        Serial.print(topic);
        Serial.print("] ");
        for (int i = 0; i < length; i++)
        {
            Serial.print((char)payload[i]);
        }
        Serial.println();
    }

    void loop()
    {
        if (!mqttClient.connected())
        {
            reconnect();
        }
        mqttClient.loop();
    }
};
