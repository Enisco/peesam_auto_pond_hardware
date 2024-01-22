/*
  TdsSensorPin = A4, topWaterLevelSensorPin = A1, bottomWaterLevelSensorPin = A2
  Temp = 4, nodeSerial(5, 6)
  inPump = 8, outPump = 9,
*/

#include <SoftwareSerial.h>

#include <EEPROM.h>  // For Tds Sensor
#include "GravityTDS.h"

#include <OneWire.h>
#include <DallasTemperature.h>

// Data wire is connected to the Arduino digital pin 4
#define ONE_WIRE_BUS 4

// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature sensor
DallasTemperature sensors(&oneWire);
float temperature;

SoftwareSerial nodeSerial(5, 6);

#define TdsSensorPin A4
GravityTDS gravityTds;
float tdsTemperature = 25, tdsValue = 0, toxicTdsVal = 450.0;

unsigned long int avgValue;  //Store the average value of the sensor feedback
int temp, tdsToxicCount = 0, temperatureToxicCount = 0;

const int inPump = 8;
const int outPump = 9;

bool keepPumpingIn = false, keepPumpingOut = false;

int topWaterLevelSensorPin = A1, bottomWaterLevelSensorPin = A2;

void setup() {
  Serial.begin(115200);
  nodeSerial.begin(9600);

  sensors.begin();
  gravityTds.setPin(TdsSensorPin);
  gravityTds.setAref(5.0);       //reference voltage on ADC, default 5.0V on Arduino UNO
  gravityTds.setAdcRange(1024);  //1024 for 10bit ADC;4096 for 12bit ADC
  gravityTds.begin();            //initialization
  delay(2000);

  // locate devices on the bus
  Serial.print(F("Found: "));
  Serial.print(sensors.getDeviceCount(), DEC);
  Serial.println(F(" Devices."));

  pinMode(inPump, OUTPUT);
  pinMode(outPump, OUTPUT);

  digitalWrite(inPump, LOW);
  digitalWrite(outPump, LOW);

  delay(500);
}

void loop() {
  Serial.println(F("Running . . ."));
  checkIfCommandReceived();
  monitorTemperature();
  measureTds();
  sendDataToNode();
  delay(5000);
}

void checkIfCommandReceived() {
  if (nodeSerial.available() > 0) {
    String data = nodeSerial.readStringUntil('\n');  // Read the data until newline character '\n'

    Serial.print("Received data: ");
    Serial.println(data);

    String commandString = "CW";
    const char *commandArray = commandString.c_str();

    if (strstr(data.c_str(), commandArray) != NULL) {
      Serial.println("Change water command received");
      startPumpingProcedure();
    } else {
      Serial.println("Invalid command received");
    }
  }
}

void sendDataToNode() {
  String data = String(keepPumpingIn) + "," + String(keepPumpingOut) + "," + String(temperature, 1) + "," + String(tdsValue);
  Serial.println(data);
  nodeSerial.println(data);
}

void startPumpingProcedure() {
  pumpOut();
}

void measureTds() {
  gravityTds.setTemperature(temperature);  // set the temperature and execute temperature compensation
  gravityTds.update();                     //sample and calculate
  tdsValue = gravityTds.getTdsValue();     // then get the value
  Serial.print(tdsValue, 0);
  Serial.println("ppm");

  if (tdsValue > toxicTdsVal) {
    tdsToxicCount++;
    if (tdsToxicCount >= 5) {
      Serial.println("Too much dissolved solids in the water");
      startPumpingProcedure();
    }
  } else {
    tdsToxicCount = 0;
    Serial.print("-");
  }
}

void monitorTemperature() {
  sensors.requestTemperatures();
  temperature = sensors.getTempFByIndex(0);
  Serial.print(F("Fahrenheit temperature: "));
  Serial.println(temperature);

  if (temperature < 65.0) {
    temperatureToxicCount++;
    if (temperatureToxicCount >= 5) {
      Serial.println(F("Temperature is too low"));
    }
  } else if (temperature > 85.0) {
    temperatureToxicCount++;
    if (temperatureToxicCount >= 5) {
      Serial.println(F("Temperature is too high"));
    }
  } else {
    temperatureToxicCount = 0;
  }
  delay(50);
}

void pumpIn() {
  keepPumpingIn = true;
  sendDataToNode();
  Serial.println(F("Pumping in now"));
  digitalWrite(inPump, HIGH);
  monitorInPumpFlow();
}

void stopPumpIn() {
  keepPumpingIn = false;
  sendDataToNode();
  digitalWrite(inPump, LOW);
  tdsToxicCount = 0;
  Serial.println(F("Pumping in DONE, all values reset"));
}

void pumpOut() {
  keepPumpingOut = true;
  sendDataToNode();
  Serial.println(F("Pumping out"));
  digitalWrite(outPump, HIGH);
  monitorOutPumpFlow();
}

void stopPumpOut() {
  keepPumpingOut = false;
  sendDataToNode();
  digitalWrite(outPump, LOW);
  Serial.println(F("Pumping out DONE"));
}

void monitorInPumpFlow() {
  while (keepPumpingIn == true) {
    digitalWrite(inPump, HIGH);
    int topWaterLevel = 0;
    for (int i = 0; i < 5; i++) {
      int topWaterLevelValue = analogRead(topWaterLevelSensorPin);
      Serial.print(String(topWaterLevelValue) + "\t");
      topWaterLevel += topWaterLevelValue;
      delay(50);
    }
    topWaterLevel = topWaterLevel / 5;
    Serial.println("\topWaterLevel: " + String(topWaterLevel));

    if (topWaterLevel <= 500) {
      Serial.println("\nStop pumping in");
      keepPumpingIn = false;
      break;
    } else {
    }
  }
  stopPumpIn();
}

void monitorOutPumpFlow() {
  while (keepPumpingOut == true) {
    digitalWrite(outPump, HIGH);
    int bottomWaterLevel = 0;
    for (int i = 0; i < 3; i++) {
      int bottomWaterLevelValue = analogRead(bottomWaterLevelSensorPin);
      Serial.print(String(bottomWaterLevelValue) + "\t");
      bottomWaterLevel += bottomWaterLevelValue;
      delay(50);
    }
    bottomWaterLevel = bottomWaterLevel / 3;
    Serial.println("\nbottomWaterLevel: " + String(bottomWaterLevel));

    if (bottomWaterLevel >= 1000) {
      Serial.print("Water at minimum level");
      keepPumpingOut = false;
      break;
    } else {
      Serial.println("- ");
    }
  }
  nodeSerial.flush();
  sendDataToNode();
  stopPumpOut();
  pumpIn();
}
