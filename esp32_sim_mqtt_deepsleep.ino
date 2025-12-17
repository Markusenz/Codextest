// Beispielcode für ESP32 mit SIM-Modem (z.B. SIM800/7000) und MQTT
// Voraussetzungen: TinyGSM, PubSubClient und passende Board-/Modem-Treiber installiert
// Verbindet sich über Mobilfunk (APN), sendet eine MQTT-Nachricht und geht anschließend für 10 Minuten in den Deep Sleep.

#include <Arduino.h>
#include <TinyGsmClient.h>
#include <PubSubClient.h>

// --- Benutzerkonfiguration ---
static const char APN[]           = "internet";       // APN des Mobilfunkanbieters
static const char GPRS_USER[]     = "";               // Nutzername (falls erforderlich)
static const char GPRS_PASS[]     = "";               // Passwort (falls erforderlich)
static const char MQTT_BROKER[]   = "test.mosquitto.org";
static const uint16_t MQTT_PORT   = 1883;
static const char MQTT_TOPIC[]    = "esp32/walter/status";
static const char MQTT_PAYLOAD[]  = "Hallo vom ESP32 über SIM!";

// GPIO für das Modem (an Board/Modul anpassen)
static const int MODEM_PWR_PIN    = 23;  // Power-Key oder Enable-Pin des Modems
static const int MODEM_RST_PIN    = 5;   // Reset-Pin des Modems
static const int MODEM_TX_PIN     = 27;  // TX vom ESP32 zum Modem
static const int MODEM_RX_PIN     = 26;  // RX zum ESP32 vom Modem

#define TINY_GSM_MODEM_SIM800
// #define TINY_GSM_MODEM_SIM7000   // Alternativ aktivieren, falls SIM7000 genutzt wird

TinyGsm modem(Serial1);
TinyGsmClient gsmClient(modem);
PubSubClient mqttClient(gsmClient);

// 10 Minuten Deep Sleep (in Mikrosekunden)
static const uint64_t SLEEP_TIME_US = 10ULL * 60ULL * 1000000ULL;

void setupModemGPIO()
{
    pinMode(MODEM_PWR_PIN, OUTPUT);
    pinMode(MODEM_RST_PIN, OUTPUT);
    digitalWrite(MODEM_PWR_PIN, HIGH);
    digitalWrite(MODEM_RST_PIN, HIGH);
}

bool connectModem()
{
    Serial.println("Modem wird gestartet...");
    Serial1.begin(115200, SERIAL_8N1, MODEM_RX_PIN, MODEM_TX_PIN);

    if (!modem.restart())
    {
        Serial.println("Modem-Start fehlgeschlagen");
        return false;
    }

    Serial.println("Netzwerkanmeldung...");
    if (!modem.gprsConnect(APN, GPRS_USER, GPRS_PASS))
    {
        Serial.println("GPRS-Verbindung fehlgeschlagen");
        return false;
    }

    Serial.print("Verbunden, IP: ");
    Serial.println(modem.localIP());
    return true;
}

bool publishOnce()
{
    mqttClient.setServer(MQTT_BROKER, MQTT_PORT);

    Serial.println("Verbinde zum MQTT-Broker...");
    if (!mqttClient.connect("esp32-walter"))
    {
        Serial.print("MQTT-Verbindung fehlgeschlagen, RC=");
        Serial.println(mqttClient.state());
        return false;
    }

    Serial.print("Sende Nachricht nach ");
    Serial.println(MQTT_TOPIC);
    bool ok = mqttClient.publish(MQTT_TOPIC, MQTT_PAYLOAD, true /*retain*/);
    mqttClient.disconnect();
    return ok;
}

void goToDeepSleep()
{
    Serial.println("Gehe jetzt in Deep Sleep...");
    modem.poweroff();
    esp_sleep_enable_timer_wakeup(SLEEP_TIME_US);
    esp_deep_sleep_start();
}

void setup()
{
    Serial.begin(115200);
    delay(2000);

    setupModemGPIO();

    if (!connectModem())
    {
        Serial.println("Abbruch wegen fehlender Modemverbindung");
        goToDeepSleep();
    }

    if (!publishOnce())
    {
        Serial.println("MQTT-Senden fehlgeschlagen");
    }
    else
    {
        Serial.println("MQTT-Nachricht erfolgreich gesendet");
    }

    // Trennung der Datenverbindung und Sleep
    modem.gprsDisconnect();
    goToDeepSleep();
}

void loop()
{
    // Wird aufgrund Deep Sleep nicht erreicht
}

