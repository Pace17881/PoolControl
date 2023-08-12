#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

class MQTTManager
{
public:
    MQTTManager();
    bool isConnected();
    void sendTempDiscovery(String entity, float temperature);
    void sendTemp(String entity, float temperature);
    void sendMotorDiscovery(String entity);
    void sendMotorDirection(String entity, bool motorDirectionSwitch);
    void setup();
    void disconnect();

private:
    static void callback(char *topic, byte *payload, unsigned int length);
};

#endif // MQTT_MANAGER_H
