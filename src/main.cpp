#include <Arduino.h>
#include <WiFi.h>
#include "ArduinoJson.h"
#include "PubSubClient.h"
#include <WiFiClientSecure.h>
#include <FS.h>
#include <SPIFFS.h>

#define RGB_PIO 38
#define FORMAT_SPIFFS_IF_FAILED true
static uint8_t s_led_state = 0;

struct Config {
    char ssid[32];
    char password[32];
    char mqttUser[32];
    char mqttPassword[32];
    char host[32];
    int port;
    char topic[32];
};

const char *filename = "/config.txt";
Config config;
// load DigiCert Global Root CA ca_cert
const char *ca_cert = \
"-----BEGIN CERTIFICATE-----\n" \
"MIIDrzCCApegAwIBAgIQCDvgVpBCRrGhdWrJWZHHSjANBgkqhkiG9w0BAQUFADBh\n" \
"MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\n" \
"d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBD\n" \
"QTAeFw0wNjExMTAwMDAwMDBaFw0zMTExMTAwMDAwMDBaMGExCzAJBgNVBAYTAlVT\n" \
"MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j\n" \
"b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IENBMIIBIjANBgkqhkiG\n" \
"9w0BAQEFAAOCAQ8AMIIBCgKCAQEA4jvhEXLeqKTTo1eqUKKPC3eQyaKl7hLOllsB\n" \
"CSDMAZOnTjC3U/dDxGkAV53ijSLdhwZAAIEJzs4bg7/fzTtxRuLWZscFs3YnFo97\n" \
"nh6Vfe63SKMI2tavegw5BmV/Sl0fvBf4q77uKNd0f3p4mVmFaG5cIzJLv07A6Fpt\n" \
"43C/dxC//AH2hdmoRBBYMql1GNXRor5H4idq9Joz+EkIYIvUX7Q6hL+hqkpMfT7P\n" \
"T19sdl6gSzeRntwi5m3OFBqOasv+zbMUZBfHWymeMr/y7vrTC0LUq7dBMtoM1O/4\n" \
"gdW7jVg/tRvoSSiicNoxBN33shbyTApOB6jtSj1etX+jkMOvJwIDAQABo2MwYTAO\n" \
"BgNVHQ8BAf8EBAMCAYYwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4EFgQUA95QNVbR\n" \
"TLtm8KPiGxvDl7I90VUwHwYDVR0jBBgwFoAUA95QNVbRTLtm8KPiGxvDl7I90VUw\n" \
"DQYJKoZIhvcNAQEFBQADggEBAMucN6pIExIK+t1EnE9SsPTfrgT1eXkIoyQY/Esr\n" \
"hMAtudXH/vTBH1jLuG2cenTnmCmrEbXjcKChzUyImZOMkXDiqw8cvpOp/2PV5Adg\n" \
"06O/nVsJ8dWO41P0jmP6P6fbtGbfYmbW0W5BjfIttep3Sp+dWOIrWcBAI+0tKIJF\n" \
"PnlUkiaY4IBIqDfv8NZ5YBberOgOzW6sRBc4L0na4UU+Krk2U886UAb3LujEV0ls\n" \
"YSEY1QSteDwsOoBrp+uvFRTp2InBuThs4pFsiv9kuXclVzDAGySj4dzp30d8tbQk\n" \
"CAUw7C29C79Fv1C5qfPrmAESrciIxpg0X40KPMbp1ZWVbd4=" \
"-----END CERTIFICATE-----\n";

WiFiClientSecure espClient;

PubSubClient client(espClient);

void writeFile(fs::FS &fs, const char *path, const char *message);

void appendFile(fs::FS &fs, const char *path, const char *message);

bool loadConfig();

void setup_wifi() {
    delay(10);
    //Init WiFi as Station, start SmartConfig
    WiFi.mode(WIFI_AP_STA);
    WiFi.beginSmartConfig();
    //Wait for SmartConfig packet from mobile
    Serial.println("Waiting for SmartConfig.");
    while (!WiFi.smartConfigDone()) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.println("SmartConfig received.");

    //Wait for WiFi to connect to AP
    Serial.println("Waiting for WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    randomSeed(micros());
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    // 将 WiFi 配置信息写入文件
    writeFile(SPIFFS, filename, "{\"ssid\":\"");
    appendFile(SPIFFS, filename, WiFi.SSID().c_str());
    appendFile(SPIFFS, filename, "\",\"password\":\"");
    appendFile(SPIFFS, filename, WiFi.psk().c_str());
    appendFile(SPIFFS, filename, "\",\n");
    appendFile(SPIFFS, filename, "\"mqtt\": {\n");
    appendFile(SPIFFS, filename, "\"host\": \"mqtt.rymcu.com\",\n");
    appendFile(SPIFFS, filename, "\"port\": 8883,\n");
    appendFile(SPIFFS, filename, "\"user\": \"esp32s3\",\n");
    appendFile(SPIFFS, filename, "\"password\": \"esp32s3..\",\n");
    appendFile(SPIFFS, filename, "\"topic\": \"rymcu\"\n");
    appendFile(SPIFFS, filename, "}}\n");
}

void setup_wifi(const char *ssid, const char *password) {
    delay(10);
    // We start by connecting to a WiFi network
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.println("Connecting to WiFi..");
    }
    randomSeed(micros());
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
}

static void blink_led() {
    /* If the addressable LED is enabled */
    if (s_led_state) {
        /* Set the LED pixel using RGB from 0 (0%) to 255 (100%) for each color */
        neopixelWrite(RGB_PIO, 0, 255, 255);
//        if (s_wifi_state) {
//        } else {
//            neopixelWrite(RGB_PIO, 255, 128, 0);
//        }
    } else {
        /* Set all LED off to clear all pixels */
        neopixelWrite(RGB_PIO, 0, 0, 0);
    }
}

static void blink_led(int r, int g, int b) {
    /* Set the LED pixel using RGB from 0 (0%) to 255 (100%) for each color */
    neopixelWrite(RGB_PIO, r, g, b);
}

void callback(char *topic, byte *payload, unsigned int length) {
    Serial.print("Message arrived in topic: ");
    Serial.println(topic);
    Serial.print("Message:");
    if (length > 1024) {
        Serial.print("Message too long!");
        return;
    }
    DynamicJsonDocument doc(length + 1024);
    DeserializationError error = deserializeJson(doc, payload, length);

    if (error) {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.c_str());
        return;
    }
    // 配置 rgb
    if (doc["rgb"]) {
        int rgb_state = doc["rgb"]["state"];
        // Switch on the LED if an 1 was received as first character
        if (rgb_state) {
            int r = doc["rgb"]["r"];
            int g = doc["rgb"]["g"];
            int b = doc["rgb"]["b"];
            blink_led(r, g, b);
            Serial.print("LED State is ON!\n");
        } else {
            s_led_state = rgb_state;
            blink_led();
            Serial.print("LED State is OFF!\n");
        }
    }
}

void reconnect() {
    // Loop until we're reconnected
    while (!client.connected()) {
        Serial.print("Attempting MQTT connection...");
        // Attempt to connect
        // Create a random client ID
        String clientId = "ESP32-";
        clientId += String(random(0xffff), HEX);
        if (client.connect(clientId.c_str(), config.mqttUser, config.mqttPassword)) {
            Serial.println("connected");
            client.publish("outTopic", ("hello world, i am " + clientId).c_str());
            client.subscribe(config.topic);
        } else {
            Serial.print("failed, rc=");
            Serial.print(client.state());
            Serial.println(" try again in 5 seconds");
            // Wait 5 seconds before retrying
            delay(5000);
        }
    }
}

void writeFile(fs::FS &fs, const char *path, const char *message) {
    Serial.printf("Writing file: %s\r\n", path);

    File file = fs.open(path, FILE_WRITE);
    if (!file) {
        Serial.println("- failed to open file for writing");
        return;
    }
    if (file.print(message)) {
        Serial.println("- file written");
    } else {
        Serial.println("- write failed");
    }
    file.close();
}

void appendFile(fs::FS &fs, const char *path, const char *message) {
    Serial.printf("Appending to file: %s\r\n", path);
    File file = fs.open(path, FILE_APPEND);
    if (!file) {
        Serial.println("- failed to open file for appending");
        return;
    }
    if (file.print(message)) {
        Serial.println("- message appended");
    } else {
        Serial.println("- append failed");
    }
    file.close();
}

bool loadConfig() {
    Serial.printf("Reading file: %s\r\n", filename);
    File configFile = SPIFFS.open(filename);
    if (!configFile || configFile.isDirectory()) {
        configFile.close();
        Serial.println("Failed to open config file");
        return false;
    }

    Serial.println("- read from file:");

    size_t size = configFile.size();
    if (size > 1024) {
        Serial.println("Config file size is too large");
        configFile.close();
    }

    std::unique_ptr<char[]> buf(new char[size]);

    configFile.readBytes(buf.get(), size);
    DynamicJsonDocument doc(size - 1);
    DeserializationError error = deserializeJson(doc, buf.get(), size);

    if (error) {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.c_str());
        configFile.close();
        return false;
    }
    strcpy(config.ssid, doc["ssid"]);
    strcpy(config.password, doc["password"]);
    strcpy(config.host, doc["mqtt"]["host"]);
    config.port = doc["mqtt"]["port"];
    strcpy(config.mqttUser, doc["mqtt"]["user"]);
    strcpy(config.mqttPassword, doc["mqtt"]["password"]);
    strcpy(config.topic, doc["mqtt"]["topic"]);
    Serial.println(config.password);
    Serial.println(config.topic);
    configFile.close();
    return true;
}

void deleteFile(fs::FS &fs, const char * path){
    Serial.printf("Deleting file: %s\r\n", path);
    if(fs.remove(path)){
        Serial.println("- file deleted");
    } else {
        Serial.println("- delete failed");
    }
}

void setup() {
    Serial.begin(115200);
    while (!Serial) { delay(100); }
    // We start by connecting to a WiFi network
    if (!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED)) {
        Serial.println("SPIFFS Mount Failed");
        return;
    } else {
        Serial.println("SPIFFS Mount Success");
        bool isConfig = loadConfig();
        if (isConfig) {
            Serial.println("Config loaded");
            setup_wifi(config.ssid, config.password);
        } else {
            setup_wifi();
            loadConfig();
        }
        espClient.setCACert(ca_cert);
        client.setServer(config.host, config.port);
        client.setCallback(callback);
    }
}

void loop() {
    if (!client.connected()) {
        reconnect();
    }
    client.loop();
}