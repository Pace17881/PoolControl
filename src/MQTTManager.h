#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

class MQTTManager
{
public:
    bool isConnected();
    void sendDiscoveryTemp(String sensorId, float temperature);
    void publishTemperature(String sensorId, float temperature);
    void callback(char *topic, byte *payload, unsigned int length);
};

#endif // MQTT_MANAGER_H
