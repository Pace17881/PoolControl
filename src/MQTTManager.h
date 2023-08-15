#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H
#include <ArduinoJson.h>

class MQTTManager
{
private:
    bool automatic;
public:
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
    void switchOutlet(String entity, const char* state);
    void subscribe();
    bool getAutomaticState();
    bool connect();
};

#endif // MQTT_MANAGER_H
