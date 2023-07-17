#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <MQTTManager.h>
#include <WiFiManager.h>


// Data wire is plugged into pin 2 on the Arduino
#define ONE_WIRE_BUS 2

// Pins for the relays
const int relayMasterPin = 12;
const int relayMotorPin = 13;

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

WiFiManager wifiManager;
MQTTManager mqttManager;

void measureDiffTemp(float poolTemperature, float solarTemperature);
void manageRelay();

void setup(void)
{
    Serial.begin(9600);
    sensors.begin(); // Start up the library
    pinMode(relayMasterPin, OUTPUT);
    pinMode(relayMotorPin, OUTPUT);

    // Search for the addresses of the DS18B20 sensors
    if (!sensors.getAddress(poolSensorAddress, 0)) {
        Serial.println("Unable to find address for Pool sensor.");
        while (1);
    }

    if (!sensors.getAddress(solarSensorAddress, 1)) {
        Serial.println("Unable to find address for Heat source sensor.");
        while (1);
    }

    // Set the resolution of the sensors to 12 bits (0.0625°C)
    sensors.setResolution(poolSensorAddress, 12);
    sensors.setResolution(solarSensorAddress, 12);

    digitalWrite(relayMotorPin, LOW);
    digitalWrite(relayMasterPin, HIGH);

    delay(20000);
    digitalWrite(relayMasterPin, LOW);
    wasMoved = true;

    wifiManager.connect();
    mqttManager.setup();

}

void loop(void)
{
    mqttManager.loop();


    // Request temperature readings from both sensors
    sensors.requestTemperaturesByAddress(poolSensorAddress);
    sensors.requestTemperaturesByAddress(solarSensorAddress);

    // Get the temperature readings
    float poolTemperature = sensors.getTempC(poolSensorAddress);
    float solarTemperature = sensors.getTempC(solarSensorAddress);

    mqttManager.publish("poolControl/pool/temperature", poolTemperature);
    mqttManager.publish("poolControl/solar/temperature", solarTemperature);


    measureDiffTemp(poolTemperature, solarTemperature);
    manageRelay();

    delay(5000);
}

void measureDiffTemp(float poolTemperature, float solarTemperature) {

    // Print the temperature readings
    Serial.print("Pool Temperature: ");
    Serial.print(poolTemperature);
    Serial.print("°C, Solar Temperature: ");
    Serial.print(solarTemperature);
    Serial.println("°C");

    // Check the temperature difference and control the relay
    if (poolTemperature > minTemp && !solarOn && solarTemperature - poolTemperature > tempTreshold) {
        digitalWrite(relayMotorPin, HIGH); // Turn on the relay
        solarOn = true;
        wasMoved = false;


    } else if (poolTemperature > minTemp && solarOn && solarTemperature - poolTemperature < tempTreshold) {
        digitalWrite(relayMotorPin, LOW); // Turn off the relay
        solarOn = false;
        wasMoved = false;
    }
}

void manageRelay() {
    if (!wasMoved) {
        relayStartTime = millis(); // Record the start time
        wasMoved = true;
        digitalWrite(relayMasterPin, HIGH); // Turn on the relay
    }

    // Check if the relay has been on for the specified duration
    if (wasMoved && millis() - relayStartTime >= relayDuration) {
        digitalWrite(relayMasterPin, LOW); // Turn off the relay
        relayStartTime = 0;
    }
}