#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

class MQTTManager
{
public:
    bool isConnected();
    void sendDiscovery(String sensorId, float temperature);
    void sendTemp(String sensorId, float temperature);
    void callback(char *topic, byte *payload, unsigned int length);
    void disconnect();
};

#endif // MQTT_MANAGER_H
