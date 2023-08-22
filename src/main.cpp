#include <Arduino.h>
#include <ArduinoOTA.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <SimpleTimer.h>
#include "MQTTManager.h"
#include "WiFiManager.h"
#include "Ticker.h"

// PIN Definition
#define ONE_WIRE_BUS 13  // Temp sensors
#define relayMasterPin 4 // Master relay
#define relayMotorPin 5  // Direction Relay
#define switchPin 14

// Definitions
#define SENSORPOOL "pool"
#define SENSORSOLAR "solar"
#define MOTORDIRECTION "mode"
#define AUTOMATIC "automatic"

// Temp Sensors
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
DeviceAddress poolSensorAddress;
DeviceAddress solarSensorAddress;

// Thresholds
const float tempTreshold = 2.0;
const float minTemp = 24.0;
const float maxTemp = 40.0;

bool masterSwitchOn = false;
bool isHeating = false;
bool initalRun = true;

// Deklaration der Funktionen
void flowTimerCallback();
void mainLoopCallback();
void masterSwitchCallback();
void delayedPumpControlCallback();
void compareTemperatures(float poolTemperature, float solarTemperature);
void switchRelay();
void sendMQTT(float poolTemperature, float solarTemperature);

// Timer
Ticker mainTimer(mainLoopCallback, 10000);
Ticker masterSwitchTimer(masterSwitchCallback, 25000, 1);
Ticker delayedPumpControlTimer(delayedPumpControlCallback, 35000, 1);
Ticker flowTimer(flowTimerCallback, 15000, 1);

// Topics
//const String sensorTopic = "homeassistant/sensor/debug/";
//const String switchTopic = "homeassistant/switch/debug/";
const String sensorTopic = "homeassistant/sensor/poolmcu/";
const String switchTopic = "homeassistant/switch/poolmcu/";
const String modeTopic = switchTopic + "mode";

WiFiManager wifiManager;
MQTTManager mqttManager(sensorTopic, switchTopic, modeTopic);

void log()
{
    Serial.printf("masterSwitch: %d\n", masterSwitchOn);
    Serial.printf("isAutomatic: %d\n", mqttManager.isAutomatic());
    Serial.printf("isSolar: %d\n", mqttManager.isSolar());
    Serial.printf("mainTimerState: %d\n", mainTimer.state());
    Serial.printf("masterSwitchTimerState: %d\n", masterSwitchTimer.state());
    Serial.printf("delayedPumpControlTimerState: %d\n", delayedPumpControlTimer.state());
}

void flowTimerCallback()
{
    // Nothing to do here
    // Wait until warm water is flown tp pool
}

void delayedPumpControlCallback()
{
    mqttManager.switchOutlet("Pool", "0");
    Serial.println("Pump off");
}

void masterSwitchCallback()
{
    masterSwitchOn = false;
    digitalWrite(relayMasterPin, masterSwitchOn); // Turn off the relay

    if (mqttManager.isAutomatic())
    {
        Serial.println("Pump off delay started");
        delayedPumpControlTimer.start();
    }
}

void mainLoopCallback()
{
    // Request temperature readings from both sensors
    sensors.requestTemperaturesByAddress(poolSensorAddress);
    sensors.requestTemperaturesByAddress(solarSensorAddress);

    // Get the temperature readings
    float poolTemperature = sensors.getTempC(poolSensorAddress);
    float solarTemperature = sensors.getTempC(solarSensorAddress);

    // Print the temperature readings
    Serial.printf("\nTemperatures pool: %.2f °C solar: %.2f °C\n\n", poolTemperature, solarTemperature);
    sendMQTT(poolTemperature, solarTemperature);

    if (mqttManager.isAutomatic())
    {
        Serial.println("Automatic Mode:");
        if (!masterSwitchOn && flowTimer.state() == STOPPED)
        {
            compareTemperatures(poolTemperature, solarTemperature);
        }
    }
    else
    {
        Serial.println("Manual Mode:");
        if (isHeating != mqttManager.isSolar())
        {
            isHeating = mqttManager.isSolar();
            masterSwitchOn = true;
        }
    }

    switchRelay();

    log();
}

void sendMQTT(float poolTemperature, float solarTemperature)
{
    mqttManager.sendTempDiscovery(SENSORPOOL, poolTemperature);
    mqttManager.sendTemp(SENSORPOOL, poolTemperature);
    mqttManager.sendTempDiscovery(SENSORSOLAR, solarTemperature);
    mqttManager.sendTemp(SENSORSOLAR, solarTemperature);
    mqttManager.sendMotorDiscovery(MOTORDIRECTION);
    mqttManager.sendMotorDirection(MOTORDIRECTION, isHeating);
}

void compareTemperatures(float poolTemperature, float solarTemperature)
{
    if (solarTemperature >= minTemp && poolTemperature <= maxTemp)
    {
        if (!isHeating && solarTemperature - poolTemperature > tempTreshold)
        {
            isHeating = true;
            masterSwitchOn = true;
            flowTimer.start();
        }
        else if (isHeating && solarTemperature - poolTemperature < tempTreshold)
        {
            isHeating = false;
            masterSwitchOn = true;
        }
    }
}

void switchRelay()
{
    if (masterSwitchOn && masterSwitchTimer.state() == STOPPED)
    {
        digitalWrite(relayMotorPin, isHeating);
        digitalWrite(relayMasterPin, masterSwitchOn); // Turn on the relay

        if (mqttManager.isAutomatic())
        {
            mqttManager.switchOutlet("Pool", "1");
        }

        masterSwitchTimer.start();
    }
}

void setup(void)
{
    Serial.begin(9600);

    sensors.begin();
    pinMode(relayMasterPin, OUTPUT);
    pinMode(relayMotorPin, OUTPUT);
    //pinMode(switchPin, INPUT);

    while (!sensors.getAddress(poolSensorAddress, 0) || !sensors.getAddress(solarSensorAddress, 1))
    {
        Serial.println("No temp sensor found");
        delay(5000);
    }

    // Set the resolution of the sensors to 12 bits (0.0625°C)
    sensors.setResolution(poolSensorAddress, 12);
    sensors.setResolution(solarSensorAddress, 12);

    mqttManager.setup();

    ArduinoOTA.setHostname("PoolController-OTA");
    ArduinoOTA.begin();
}

void loop(void)
{
    ArduinoOTA.handle();

    //int switched = digitalRead(switchPin);
    //Serial.printf("Switch pressed: %d\n", switched);
    if (initalRun)
    {
        Serial.printf("\nInitial: isHeating: %d\n", isHeating);
        isHeating = false;
        masterSwitchOn = true;
        initalRun = false;
        switchRelay();
        mainTimer.start();
    }

    mainTimer.update();
    masterSwitchTimer.update();
    delayedPumpControlTimer.update();
    flowTimer.update();

    if (wifiManager.isConnected())
    {
        if (mqttManager.connect())
        {
            mqttManager.loop();
        }
    }
}