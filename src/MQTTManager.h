#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H
#include <ArduinoJson.h>

class MQTTManager
{
private:
    bool automatic;
    bool direction;

public:
    MQTTManager(String sensorTopic, String switchTopic, String modeTopic) : mSensorTopic(sensorTopic), mSwitchTopic(switchTopic), mModeTopic(modeTopic) {};
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
    bool isAutomatic();
    bool isSolar();
    bool connect();
    void callback(char *topic, byte *payload, unsigned int length);

private:
    const String mSensorTopic;
    const String mSwitchTopic;
    const String mModeTopic;
};

#endif // MQTT_MANAGER_H
