// WiFi credentials
const char* ssid = "Internet";
const char* password = "(/DDD)HD)988793hdHhd39hd987887";

class WiFiManager {
public:
    void connect()
    {
        WiFi.begin(ssid, password);

        while (WiFi.status() != WL_CONNECTED) {
            delay(500);
        }

        Serial.println("");
        Serial.println("WiFi connected");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
    }
};
