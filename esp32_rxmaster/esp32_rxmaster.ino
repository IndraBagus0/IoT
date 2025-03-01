#include <WiFi.h>
#include <HardwareSerial.h>
#include <HTTPClient.h>

const char *serverURL = "http://192.168.0.4:5000/data";

const char *ssid = "TOTOLINK_N200RE";
const char *password = "1234 abcd";

HardwareSerial mySerial(1); // Gunakan Serial1 pada ESP32 (RX=16, TX=17)

float temperature = 0.0;
float ph = 0.0;
float ppm = 0.0;
int turbidity = 0;
float kelayakan = 0.0;

void setup()
{
    Serial.begin(115200);                     // Serial Monitor ESP32
    mySerial.begin(9600, SERIAL_8N1, 16, 17); // RX=16, TX=17

    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi");

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(1000);
        Serial.print(".");
    }
    Serial.println("\nConnected to WiFi!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    Serial.println("Serial data dimulai");
}

void loop()
{

    if (mySerial.available())
    {
        String receivedData = mySerial.readString(); // Baca seluruh data hingga timeout (default 1s)
        receivedData.trim();                         // Hapus spasi atau newline yang tidak diinginkan
        processAndSendData(receivedData);
    }
}

void processAndSendData(String data)
{
    // Pisahkan data berdasarkan koma
    int firstComma = data.indexOf(',');
    int secondComma = data.indexOf(',', firstComma + 1);
    int thirdComma = data.indexOf(',', secondComma + 1);
    int fourthComma = data.indexOf(',', thirdComma + 1); 

    if (firstComma != -1 && secondComma != -1 && thirdComma != -1 && fourthComma != -1)
    {
        temperature = data.substring(0, firstComma).toFloat();
        ph = data.substring(firstComma + 1, secondComma).toFloat();
        ppm = data.substring(secondComma + 1, thirdComma).toFloat();
        turbidity = data.substring(thirdComma + 1, fourthComma).toInt();
        kelayakan = data.substring(fourthComma + 1).toFloat();

        Serial.println("===== Sensor Data =====");
        Serial.print("Temperature (°C): ");
        Serial.println(temperature);
        Serial.print("pH: ");
        Serial.println(ph);
        Serial.print("TDS (ppm): ");
        Serial.println(ppm);
        Serial.print("Turbidity (%): ");
        Serial.println(turbidity);
        Serial.print("Kelayakan: ");
        Serial.println(kelayakan);
        Serial.println("-----------------------");

        // Kirim data ke Flask jika terhubung ke WiFi
        if (WiFi.status() == WL_CONNECTED)
        {
            HTTPClient http;
            WiFiClient client;

            http.begin(client, serverURL);
            http.addHeader("Content-Type", "application/json");

            // Format JSON
            String jsonData = "{";
            jsonData += "\"temperature\":" + String(temperature) + ",";
            jsonData += "\"ph\":" + String(ph) + ",";
            jsonData += "\"tds\":" + String(ppm) + ",";
            jsonData += "\"turbidity\":" + String(turbidity) + ",";
            jsonData += "\"kelayakan\":" + String(kelayakan);
            jsonData += "}";

            int httpResponseCode = http.POST(jsonData);

            Serial.print("HTTP Response code: ");
            Serial.println(httpResponseCode);

            http.end();
        }
        else
        {
            Serial.println("Error: Not connected to WiFi");
        }
    }
    else
    {
        Serial.println("Error: Data format incorrect!");
    }
}

