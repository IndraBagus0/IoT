#include <SoftwareSerial.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <GravityTDS.h>

#define ONE_WIRE_BUS 12    // DS18B20 pada pin D12
#define TDS_SENSOR_PIN A1  // Sensor TDS pada pin A1
#define PH_SENSOR_PIN A2   // Sensor pH pada pin A2
#define TD_SENSOR_PIN A3   // Sensor Turbidity pada pin A3

#define RX_PIN 2  // Software Serial RX (Tidak dipakai untuk TX-only)
#define TX_PIN 3  // Software Serial TX (Untuk mengirim data)

SoftwareSerial mySerial(RX_PIN, TX_PIN);  // RX, TX

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
GravityTDS gravityTds;

float temperatureC = 25.0;
int maxvisibl = 600, minvisibl = 0;

void setup() {
  Serial.begin(9600);
  mySerial.begin(9600);  // Software Serial untuk komunikasi eksternal
  sensors.begin();

  // Setup TDS Sensor
  gravityTds.setPin(TDS_SENSOR_PIN);
  gravityTds.setAref(5.0);       // Referensi tegangan (5V untuk Arduino Nano)
  gravityTds.setAdcRange(1024);  // ADC 10-bit (0 - 1023)
  gravityTds.begin();
}

void loop() {
  // **Baca Suhu dari DS18B20**
  sensors.requestTemperatures();
  temperatureC = sensors.getTempCByIndex(0);

  // **Baca pH**
  int rawPHValue = analogRead(PH_SENSOR_PIN);
  float voltagePH = rawPHValue * (5.0 / 1023.0);   // Konversi ADC ke Volt
  float pHValue = (voltagePH - 2.89) * 3.5 + 7.0;  // Kalibrasi sederhana

  // **Baca TDS**
  gravityTds.setTemperature(temperatureC);  // Kompensasi suhu
  gravityTds.update();
  float tdsValue = gravityTds.getTdsValue();  // ppm

  // **Baca Turbidity**
  int rawTDValue = analogRead(TD_SENSOR_PIN);
  if (rawTDValue > maxvisibl) rawTDValue = maxvisibl;
  int percenTD = map(rawTDValue, maxvisibl, minvisibl, 0, 100);

  // **Tampilkan data ke Serial Monitor**
  Serial.print("Suhu (°C): ");
  Serial.print(temperatureC);
  Serial.print(" | pH: ");
  Serial.print(pHValue);
  Serial.print(" | TDS (ppm): ");
  Serial.print(tdsValue);
  Serial.print(" | Turbidity: ");
  Serial.print(percenTD);
  Serial.println("%");

  // **Kirim data melalui Software Serial (TX)**
  mySerial.print(temperatureC);
  mySerial.print(",");
  mySerial.print(pHValue);
  mySerial.print(",");
  mySerial.print(tdsValue);
  mySerial.print(",");
  mySerial.println(percenTD);

  delay(1000);  // delay 1 detik membaca ulang
}
