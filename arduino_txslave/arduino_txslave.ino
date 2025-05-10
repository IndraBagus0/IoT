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

float numerator = 0, denominator = 0;
float output = 0;

struct FuzzySet {
  float a, b, c, d;
  FuzzySet(float _a, float _b, float _c, float _d)
    : a(_a), b(_b), c(_c), d(_d) {}
  float membership(float x) {
    if (x <= a || x >= d)
      return 0;
    else if (x >= b && x <= c)
      return 1;
    else if (x > a && x < b)
      return (x - a) / (b - a);
    else
      return (d - x) / (d - c);
  }
};

FuzzySet asam(0, 0, 5, 7);
FuzzySet netral(6.5, 7, 8, 8.5);
FuzzySet basa(8, 9, 14, 14);

FuzzySet baik(0, 0, 200, 500);
FuzzySet cukup(300, 500, 700, 1000);
FuzzySet tidak_baik(900, 1000, 1000, 1000);

FuzzySet jernih(0, 0, 2, 5);
FuzzySet cukup_jernih(4, 10, 15, 25);
FuzzySet keruh(24, 25, 2000, 2000);

float defuzzifySugeno(float numerator, float denominator) {
  return (denominator == 0) ? 0 : numerator / denominator;
}

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
  float pHValue = (voltagePH - 2.89) * 3.5 + 7.0;  // Kalibrasi Sensor

  // **Baca TDS**
  gravityTds.setTemperature(temperatureC);  // Kompensasi suhu
  gravityTds.update();
  float tdsValue = gravityTds.getTdsValue();  // ppm

  // **Baca Turbidity**
  int rawTDValue = analogRead(TD_SENSOR_PIN);
  int turbidityNTU = map(rawTDValue, 0, 640, 100, 0);

  if (turbidityNTU < 0)
    turbidityNTU = 1;

  float data_pH = pHValue;
  float data_TDS = tdsValue;
  float data_kekeruhan = turbidityNTU;

  // fuzzy logic sugeno
  float input_pH = pHValue;
  float input_TDS = tdsValue;
  float input_Kekeruhan = turbidityNTU;

  float rules[27][4] = {
    { asam.membership(input_pH), baik.membership(input_TDS), jernih.membership(input_Kekeruhan), 50 },
    { asam.membership(input_pH), baik.membership(input_TDS), cukup_jernih.membership(input_Kekeruhan), 50 },
    { asam.membership(input_pH), baik.membership(input_TDS), keruh.membership(input_Kekeruhan), 50 },
    { asam.membership(input_pH), cukup.membership(input_TDS), jernih.membership(input_Kekeruhan), 50 },
    { asam.membership(input_pH), cukup.membership(input_TDS), cukup_jernih.membership(input_Kekeruhan), 50 },
    { asam.membership(input_pH), cukup.membership(input_TDS), keruh.membership(input_Kekeruhan), 50 },
    { asam.membership(input_pH), tidak_baik.membership(input_TDS), jernih.membership(input_Kekeruhan), 50 },
    { asam.membership(input_pH), tidak_baik.membership(input_TDS), cukup_jernih.membership(input_Kekeruhan), 50 },
    { asam.membership(input_pH), tidak_baik.membership(input_TDS), keruh.membership(input_Kekeruhan), 50 },

    { netral.membership(input_pH), baik.membership(input_TDS), jernih.membership(input_Kekeruhan), 100 },
    { netral.membership(input_pH), baik.membership(input_TDS), cukup_jernih.membership(input_Kekeruhan), 100 },
    { netral.membership(input_pH), baik.membership(input_TDS), keruh.membership(input_Kekeruhan), 50 },
    { netral.membership(input_pH), cukup.membership(input_TDS), jernih.membership(input_Kekeruhan), 100 },
    { netral.membership(input_pH), cukup.membership(input_TDS), cukup_jernih.membership(input_Kekeruhan), 100 },
    { netral.membership(input_pH), cukup.membership(input_TDS), keruh.membership(input_Kekeruhan), 50 },
    { netral.membership(input_pH), tidak_baik.membership(input_TDS), jernih.membership(input_Kekeruhan), 50 },
    { netral.membership(input_pH), tidak_baik.membership(input_TDS), cukup_jernih.membership(input_Kekeruhan), 50 },
    { netral.membership(input_pH), tidak_baik.membership(input_TDS), keruh.membership(input_Kekeruhan), 50 },

    { basa.membership(input_pH), baik.membership(input_TDS), jernih.membership(input_Kekeruhan), 50 },
    { basa.membership(input_pH), baik.membership(input_TDS), cukup_jernih.membership(input_Kekeruhan), 50 },
    { basa.membership(input_pH), baik.membership(input_TDS), keruh.membership(input_Kekeruhan), 50 },
    { basa.membership(input_pH), cukup.membership(input_TDS), jernih.membership(input_Kekeruhan), 50 },
    { basa.membership(input_pH), cukup.membership(input_TDS), cukup_jernih.membership(input_Kekeruhan), 50 },
    { basa.membership(input_pH), cukup.membership(input_TDS), keruh.membership(input_Kekeruhan), 50 },
    { basa.membership(input_pH), tidak_baik.membership(input_TDS), jernih.membership(input_Kekeruhan), 50 },
    { basa.membership(input_pH), tidak_baik.membership(input_TDS), cukup_jernih.membership(input_Kekeruhan), 50 },
    { basa.membership(input_pH), tidak_baik.membership(input_TDS), keruh.membership(input_Kekeruhan), 50 }
  };

  float numerator = 0, denominator = 0;

  for (int i = 0; i < 27; i++) {
    float weight = rules[i][0] * rules[i][1] * rules[i][2];
    numerator += weight * rules[i][3];
    denominator += weight;
  }

  float output = defuzzifySugeno(numerator, denominator);
  String statusKelayakan;
  if (output >= 50) {
    statusKelayakan = "Layak";
  } else {
    statusKelayakan = "Tidak Layak";
  }
  // **Tampilkan data ke Serial Monitor**
  Serial.print("Suhu (°C): ");
  Serial.print(temperatureC);
  Serial.print(" | pH: ");
  Serial.print(pHValue);
  Serial.print(" | TDS (ppm): ");
  Serial.print(tdsValue);
  Serial.print(" | Turbidity: ");
  Serial.print(turbidityNTU);
  Serial.print("NTU");
  Serial.print(" | Kelayakan Air: ");
  Serial.println(statusKelayakan);

  // **Kirim data melalui Software Serial (TX)**
  mySerial.print(temperatureC);
  mySerial.print(",");
  mySerial.print(pHValue);
  mySerial.print(",");
  mySerial.print(tdsValue);
  mySerial.print(",");
  mySerial.println(turbidityNTU);
  mySerial.print(",");
  mySerial.println(statusKelayakan);

  delay(5000);  // delay 5 detik membaca ulang
}
