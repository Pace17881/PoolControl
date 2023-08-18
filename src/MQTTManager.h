#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H
#include <ArduinoJson.h>

class MQTTManager
{
private:
    bool automatic;

public:
    MQTTManager(String sensorTopic, String switchTopic) : baseSensorTopic(sensorTopic), baseSwitchTopic(switchTopic) {};
    void sendDataToTopic(String &topic, DynamicJsonDocument &data);
    void sendTempDiscovery(String entity, float temperature);
    void sendTemp(String entity, float temperature);
    void sendMotorDiscovery(String entity);
    void sendMotorDirection(String entity, bool motorDirectionSwitch);
    void sendAutomaticStateDiscovery(String entity);
    void sendAutomaticStateValue(String entity, bool mode);
    void setup();
    void loop();
    void disconnect();
    void switchOutlet(String entity, const char *state);
    void subscribe();
    bool getAutomaticState();
    bool connect();
    void callback(char *topic, byte *payload, unsigned int length);

private:
    const String baseSensorTopic;
    const String baseSwitchTopic;
};

#endif // MQTT_MANAGER_H
