#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

class WiFiManager
{
private:
    bool connected;
public:
    bool isConnected();
    void disconnect();
};

#endif // WIFI_MANAGER_H
