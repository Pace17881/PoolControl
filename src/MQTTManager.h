#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

class MQTTManager
{
private:
    bool automatic;
public:
    bool isConnected();
    void sendTempDiscovery(String entity, float temperature);
    void sendTemp(String entity, float temperature);
    void sendMotorDiscovery(String entity);
    void sendMotorDirection(String entity, bool motorDirectionSwitch);
    void sendAutomaticStateDiscovery(String entity);
    void sendAutomaticStateValue(String entity, bool mode);
    void setup();
    void loop();
    void disconnect();
    void switchOutlet(String topic, const char* state);
    void subscribe();
    bool getAutomaticState();
};

#endif // MQTT_MANAGER_H
