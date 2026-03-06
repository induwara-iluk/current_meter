#include <Arduino.h>
#include <bluefruit.h>
#include "EmonLib.h"   // For CT current sensor

#define CT_PIN A0
#define LED_PIN 30
#define ADV_INTERVAL 20000   // 20 seconds

EnergyMonitor emon1;

double voltage = 230.0;        // mains voltage
double powerFactor = 0.9;      // assumed PF
double energy_kwh = 0;

uint32_t lastAdv = 0;
unsigned long lastTime = 0;

void startAdv(double Irms, double power)
{
  Serial.println("---- BLE Advertisement ----");

  // LED ON to indicate sending
  digitalWrite(LED_PIN, HIGH);

  Bluefruit.Advertising.stop();
  Bluefruit.Advertising.clearData();

  // Add the device name
  Bluefruit.Advertising.addName();

  // Pack data (Manufacturer Data)
  // Format: 2 bytes Company ID + 2 bytes Irms (scaled) + 2 bytes Power (scaled)
  uint8_t data[6];

  data[0] = 0xFF;  // Company ID
  data[1] = 0xFF;

  // scale Irms to 0-65535
  uint16_t Irms_scaled = Irms * 100; // 0.01A resolution
  data[2] = Irms_scaled & 0xFF;
  data[3] = Irms_scaled >> 8;

  // scale Power to 0-65535 W
  uint16_t power_scaled = power;  // 1W resolution
  data[4] = power_scaled & 0xFF;
  data[5] = power_scaled >> 8;

  Bluefruit.Advertising.addManufacturerData(data, sizeof(data));

  Serial.print("Irms (A): ");
  Serial.println(Irms);
  Serial.print("Power (W): ");
  Serial.println(power);

  Serial.print("Packet: ");
  for (int i = 0; i < 6; i++) {
    Serial.print(data[i], HEX);
    Serial.print(" ");
  }
  Serial.println();

  Bluefruit.Advertising.setInterval(160, 160); // ~100ms
  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.start(0);

  Serial.println("Advertising started");
  Serial.println();

  delay(200);
  digitalWrite(LED_PIN, LOW);
}

void setup()
{
  pinMode(LED_PIN, OUTPUT);

  //Serial.begin(115200);
  //while (!Serial) delay(10);

  //Serial.println("Starting CT Sensor Beacon");

  Bluefruit.begin();
  Bluefruit.setName("ADC_SENSOR");  // device name

  // CT sensor setup: CT100A/100mA ratio
  emon1.current(CT_PIN, 117); // ratio = 100A/100mA, 1000 is scaling factor

  lastTime = millis();

  //Serial.println("BLE Ready");
}

void loop()
{
  // calculate current RMS
  double Irms = emon1.calcIrms(1480);

  // calculate instantaneous power
  double power = voltage * Irms * powerFactor;

  // calculate energy increment
  unsigned long now = millis();
  double hours = (now - lastTime) / 3600000.0; // ms -> hours
  energy_kwh += (power * hours) / 1000.0;
  lastTime = now;

  // debug print
  //Serial.print("Irms: ");
  //Serial.println(Irms);
  //Serial.print("Power W: ");
  //Serial.println(power);
  //Serial.print("Energy kWh: ");
  //Serial.println(energy_kwh);
  //Serial.println();
  //Serial.println(analogRead(A0));

  // send BLE advertisement every 20s
  if (millis() - lastAdv > ADV_INTERVAL)
  {
    lastAdv = millis();
    startAdv(Irms, power);
  }

  delay(1000);
}