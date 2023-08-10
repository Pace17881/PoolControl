#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

class MQTTManager
{
public:
    MQTTManager();
    bool isConnected();
    void sendDiscovery(String sensorId, float temperature);
    void sendTemp(String sensorId, float temperature);
    void setup();
    void disconnect();

private:
    static void callback(char *topic, byte *payload, unsigned int length);
};

#endif // MQTT_MANAGER_H
