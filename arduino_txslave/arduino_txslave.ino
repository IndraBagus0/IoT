#include <SoftwareSerial.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <GravityTDS.h>

#define ONE_WIRE_BUS 12    // DS18B20 pada pin D12
#define TDS_SENSOR_PIN A1  // Sensor TDS pada pin A1
#define PH_SENSOR_PIN A2   // Sensor pH pada pin A2
#define TD_SENSOR_PIN A3   // Sensor Turbidity pada pin A3

#define RX_PIN 2
#define TX_PIN 3

SoftwareSerial mySerial(RX_PIN, TX_PIN);  // RX, TX

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
GravityTDS gravityTds;

float voltNTU = 0;
float ntuNTU = 0;

float square(float x) {
  return x * x;
}

float round_to_dp(float value, int decimal_places) {
  float factor = pow(10, decimal_places);
  return round(value * factor) / factor;
}

// === Fuzzy Membership Functions ===

float func_ph_asam(float ph) {
  if (ph <= 6.5) return 1;
  else if (ph <= 7) return (7 - ph) / 0.5;
  return 0;
}

float func_ph_netral(float ph) {
  if (ph <= 6.5) return 0;
  else if (ph <= 7) return (ph - 6.5) / 0.5;
  else if (ph <= 8) return 1;
  else if (ph <= 8.5) return (8.5 - ph) / 0.5;
  return 0;
}

float func_ph_basa(float ph) {
  if (ph <= 8) return 0;
  else if (ph <= 8.5) return (ph - 8) / 0.5;
  return 1;
}

float func_tds_baik(float tds) {
  if (tds <= 300) return 1;
  else if (tds <= 500) return (500 - tds) / 200.0;
  return 0;
}

float func_tds_cukup(float tds) {
  if (tds <= 300 || tds >= 1100) return 0;
  else if (tds <= 500) return (tds - 300) / 200.0;
  else if (tds <= 900) return 1;
  else if (tds < 1100) return (1100 - tds) / 200.0;
  return 0;
}

float func_tds_tidak_baik(float tds) {
  if (tds <= 900) return 0;
  else if (tds <= 1000) return (tds - 900) / 100.0;
  return 1;
}

float func_turb_jernih(float turb) {
  if (turb <= 4) return 1;
  else if (turb <= 5) return (5 - turb) / 1.0;
  return 0;
}

float func_turb_cukup_jernih(float turb) {
  if (turb <= 4 || turb >= 25) return 0;
  else if (turb <= 5) return (turb - 4) / 1.0;
  else if (turb <= 20) return 1;
  else if (turb < 25) return (25 - turb) / 5.0;
  return 0;
}

float func_turb_keruh(float turb) {
  if (turb <= 24) return 0;
  else if (turb <= 25) return (turb - 24) / 1.0;
  return 1;
}

float fuzzy_and_min(float a, float b, float c) {
  return min(a, min(b, c));
}

float rules_sugeno(float ph, float tds, float turb) {
  float phSet[3] = {
    func_ph_asam(ph),
    func_ph_netral(ph),
    func_ph_basa(ph)
  };

  float tdsSet[3] = {
    func_tds_baik(tds),
    func_tds_cukup(tds),
    func_tds_tidak_baik(tds)
  };

  float turbSet[3] = {
    func_turb_jernih(turb),
    func_turb_cukup_jernih(turb),
    func_turb_keruh(turb)
  };

  float out_layak = 100;
  float out_tidak_layak = 50;
  float numerator = 0;
  float denominator = 0;

  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      for (int k = 0; k < 3; k++) {
        float alpha = fuzzy_and_min(phSet[i], tdsSet[j], turbSet[k]);
        float z = out_tidak_layak;

        if (i == 1) {
          if ((j == 0 && k == 1) || (j == 0 && k == 0) || (j == 1 && k == 0) || (j == 1 && k == 1)) {
            z = out_layak;
          }
        }

        numerator += alpha * z;
        denominator += alpha;
      }
    }
  }

  if (denominator == 0) return 0;
  return numerator / denominator;
}

// === Setup & Loop ===

void setup() {
  Serial.begin(9600);
  mySerial.begin(9600);
  sensors.begin();

  gravityTds.setPin(TDS_SENSOR_PIN);
  gravityTds.setAref(5.0);
  gravityTds.setAdcRange(1024);
  gravityTds.begin();
}

void loop() {
  sensors.requestTemperatures();
  float tempC = sensors.getTempCByIndex(0);

  // pH Kalibration
  int rawPH = analogRead(PH_SENSOR_PIN);
  float voltPH = rawPH * (5.0 / 1023.0);
  // float phVal = -4.76 * voltPH + 20.86;
  // Serial.println(voltPH);
  float phVal = -6.822 * voltPH + 27.934;

  
  gravityTds.setTemperature(tempC);
  gravityTds.update();
  float tdsVal = gravityTds.getTdsValue();

  // int rawTurb = analogRead(TD_SENSOR_PIN);
  // int turbNTU = map(rawTurb, 0, 640, 100, 0);
  // float turbNTU = reverseMapFloat(rawTurb, 300.0, 850.0, 4500.0, 1400.0);
  // if (turbNTU < 0) turbNTU = 1;
  float voltNTU = 0;
  for (int i = 0; i < 800; i++) {
    int raw = analogRead(TD_SENSOR_PIN);
    float v = (float)raw / 1023.0 * 5.0;  // Konversi ADC ke voltase 0-5V
    voltNTU += v;
  }
  voltNTU = voltNTU / 800.0;
  voltNTU = round_to_dp(voltNTU, 2);

  if (voltNTU < 2.5) {
    voltNTU = 3000;
  } else {
    voltNTU = -1120.4 * square(voltNTU) + 5742.3 * voltNTU - 4353.8;
  }

  if (ntu < 0) ntu = 0;
  // float fuzzy
  float fuzzy_result = rules_sugeno(phVal, tdsVal, turbNTU);
  // fuzzy logic sugeno
  Serial.print("Suhu (°C): ");
  Serial.print(tempC);
  Serial.print(" | pH: ");
  Serial.print(phVal);
  Serial.print(" | TDS (ppm): ");
  Serial.print(tdsVal);
  Serial.print(" | Turbidity: ");
  Serial.print(turbNTU);
  Serial.print("NTU");
  Serial.print(" | Kelayakan Air: ");
  Serial.println(fuzzy_result);

  // **Kirim data melalui Software Serial (TX)**
  mySerial.print(tempC);
  mySerial.print(",");
  mySerial.print(phVal);
  mySerial.print(",");
  mySerial.print(tdsVal);
  mySerial.print(",");
  mySerial.println(turbNTU);
  mySerial.print(",");
  mySerial.println(fuzzy_result);

  delay(1000);  // delay 5 detik membaca ulang
}
