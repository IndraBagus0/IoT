#include <HardwareSerial.h>

HardwareSerial mySerial(1);  // Gunakan Serial1 pada ESP32 (RX=16, TX=17)

float temperature = 0.0;
float ph = 0.0;
float ppm = 0.0;
int turbidity = 0;

void setup() {
  Serial.begin(115200);                      // Serial Monitor ESP32
  mySerial.begin(9600, SERIAL_8N1, 16, 17);  // RX=16, TX=17

  Serial.println("Serial data dimulai");
}

void loop() {
  if (mySerial.available()) {
    String receivedData = mySerial.readString();  // Baca seluruh data hingga timeout (default 1s)
    receivedData.trim();                          // Hapus spasi atau newline yang tidak diinginkan
    processReceivedData(receivedData);
  }
}

void processReceivedData(String data) {
  Serial.print("Received Data: ");
  Serial.println(data);

  // Pisahkan data berdasarkan koma
  int firstComma = data.indexOf(',');
  int secondComma = data.indexOf(',', firstComma + 1);
  int thirdComma = data.indexOf(',', secondComma + 1);

  if (firstComma != -1 && secondComma != -1 && thirdComma != -1) {
    temperature = data.substring(0, firstComma).toFloat();
    ph = data.substring(firstComma + 1, secondComma).toFloat();
    ppm = data.substring(secondComma + 1, thirdComma).toFloat();
    turbidity = data.substring(thirdComma + 1).toInt();

    Serial.print("Temperature (°C): ");
    Serial.println(temperature);
    Serial.print("pH: ");
    Serial.println(ph);
    Serial.print("TDS (ppm): ");
    Serial.println(ppm);
    Serial.print("Turbidity (%): ");
    Serial.println(turbidity);
    Serial.println("--------------------------------");
  } else {
    Serial.println("Error: Data format incorrect!");
  }
}
