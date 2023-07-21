#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <MQTTManager.h>
#include <WiFiManager.h>

// Data wire is plugged into pin 2 on the Arduino
#define ONE_WIRE_BUS 2

// Pins for the relays
#define relayMasterPin 12
#define relayMotorPin 13



// Setup a OneWire instance to communicate with any OneWire devices
OneWire oneWire(ONE_WIRE_BUS);

// Pass OneWire reference to Dallas Temperature
DallasTemperature sensors(&oneWire);

// Address of the first DS18B20 sensor (Pool)
DeviceAddress poolSensorAddress;

// Address of the second DS18B20 sensor (Heat source)
DeviceAddress solarSensorAddress;

// Threshold for temperature difference
const float tempTreshold = 2.0;

const float minTemp = 25.0;
const float maxTemp = 60.0;

unsigned long relayStartTime = 0;
const unsigned long relayDuration = 20000; // 20 seconds

bool wasMoved = false;
bool solarOn = false;
bool isBypassInitiated = false;

WiFiManager wifiManager;
MQTTManager mqttManager;

void measureDiffTemp(float poolTemperature, float solarTemperature);
void manageRelay();
void startDeepSleep();

#define SENSORPOOL "pool"
#define SENSORSOLAR "solar"

void setup(void)
{
    Serial.begin(9600);
    sensors.begin(); // Start up the library
    pinMode(relayMasterPin, OUTPUT);
    pinMode(relayMotorPin, OUTPUT);

    // Search for the addresses of the DS18B20 sensors
    if (!sensors.getAddress(poolSensorAddress, 0))
    {
        Serial.println("Unable to find address for Pool sensor.");
        while (1)
            ;
    }

    if (!sensors.getAddress(solarSensorAddress, 1))
    {
        Serial.println("Unable to find address for Heat source sensor.");
        while (1)
            ;
    }

    // Set the resolution of the sensors to 12 bits (0.0625°C)
    sensors.setResolution(poolSensorAddress, 12);
    sensors.setResolution(solarSensorAddress, 12);
}

void loop(void)
{
    // Request temperature readings from both sensors
    sensors.requestTemperaturesByAddress(poolSensorAddress);
    sensors.requestTemperaturesByAddress(solarSensorAddress);

    // Get the temperature readings
    float poolTemperature = sensors.getTempC(poolSensorAddress);
    float solarTemperature = sensors.getTempC(solarSensorAddress);

    if (wifiManager.isConnected() && mqttManager.isConnected())
    {
        mqttManager.sendDiscoveryTemp(SENSORPOOL, poolTemperature);
        mqttManager.sendDiscoveryTemp(SENSORSOLAR, solarTemperature);
    }

    measureDiffTemp(poolTemperature, solarTemperature);
    manageRelay();

    delay(5000);
    //startDeepSleep();

}

void measureDiffTemp(float poolTemperature, float solarTemperature)
{

    // Print the temperature readings

    Serial.printf("\nTemperatures pool: %.2f °C solar: %.2f °C\n\n", poolTemperature, solarTemperature);

    // Check the temperature difference and control the relay
    if (poolTemperature > minTemp && !solarOn && solarTemperature - poolTemperature > tempTreshold)
    {
        digitalWrite(relayMotorPin, HIGH); // Turn on the relay
        solarOn = true;
        wasMoved = false;
    }
    else if (poolTemperature > minTemp && solarOn && solarTemperature - poolTemperature < tempTreshold)
    {
        digitalWrite(relayMotorPin, LOW); // Turn off the relay
        solarOn = false;
        wasMoved = false;
    }
}

void manageRelay()
{
    if (!wasMoved)
    {
        relayStartTime = millis(); // Record the start time
        wasMoved = true;
        digitalWrite(relayMasterPin, HIGH); // Turn on the relay
    }

    // Check if the relay has been on for the specified duration
    if (wasMoved && millis() - relayStartTime >= relayDuration)
    {
        digitalWrite(relayMasterPin, LOW); // Turn off the relay
        relayStartTime = 0;
    }
}

void startDeepSleep(){
  Serial.println("Going to deep sleep...");
  ESP.deepSleep(1 * 60 * 1e6);
  yield();
}
