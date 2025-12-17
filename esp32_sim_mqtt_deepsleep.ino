// Beispielcode für ESP32 Walter Board mit WalterModem-Library und MQTT
// Voraussetzungen: WalterModem (liefert GSM/LTE-Treiber + Client), PubSubClient
// Verbindet sich über Mobilfunk (APN), sendet eine MQTT-Nachricht und geht anschließend für 10 Minuten in den Deep Sleep.

#include <Arduino.h>
#include <WalterModem.h>
#include <PubSubClient.h>

// --- Benutzerkonfiguration ---
static const char APN[]           = "internet";       // APN des Mobilfunkanbieters
static const char GPRS_USER[]     = "";               // Nutzername (falls erforderlich)
static const char GPRS_PASS[]     = "";               // Passwort (falls erforderlich)
static const char MQTT_BROKER[]   = "test.mosquitto.org";
static const uint16_t MQTT_PORT   = 1883;
static const char MQTT_TOPIC[]    = "esp32/walter/status";
static const char MQTT_PAYLOAD[]  = "Hallo vom ESP32 über SIM!";

// GPIO für das Walter-Modem (an Board/Modul anpassen)
static const int MODEM_PWR_PIN    = 23;  // Power-Key oder Enable-Pin des Modems
static const int MODEM_RST_PIN    = 5;   // Reset-Pin des Modems
static const int MODEM_TX_PIN     = 27;  // TX vom ESP32 zum Modem
static const int MODEM_RX_PIN     = 26;  // RX zum ESP32 vom Modem

// 10 Minuten Deep Sleep (in Mikrosekunden)
static const uint64_t SLEEP_TIME_US = 10ULL * 60ULL * 1000000ULL;

// WalterModem übernimmt das Ansteuern des Modems und stellt einen Client für MQTT bereit
HardwareSerial SerialAT(1);
WalterModem walterModem(&SerialAT, MODEM_PWR_PIN, MODEM_RST_PIN);
Client &netClient = walterModem.getClient();
PubSubClient mqttClient(netClient);

void setupModemGPIO()
{
    pinMode(MODEM_PWR_PIN, OUTPUT);
    pinMode(MODEM_RST_PIN, OUTPUT);
    digitalWrite(MODEM_PWR_PIN, HIGH);
    digitalWrite(MODEM_RST_PIN, HIGH);
}

bool connectModem()
{
    Serial.println("Modem wird gestartet (WalterModem)...");
    SerialAT.begin(115200, SERIAL_8N1, MODEM_RX_PIN, MODEM_TX_PIN);

    if (!walterModem.begin())
    {
        Serial.println("WalterModem konnte nicht initialisiert werden");
        return false;
    }

    walterModem.configureAPN(APN, GPRS_USER, GPRS_PASS);

    Serial.println("Netzwerkanmeldung...");
    if (!walterModem.connectNetwork())
    {
        Serial.println("GPRS/LTE-Verbindung fehlgeschlagen");
        return false;
    }

    Serial.print("Verbunden, IP: ");
    Serial.println(walterModem.localIP());
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
    walterModem.powerOff();
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
    walterModem.disconnectNetwork();
    goToDeepSleep();
}

void loop()
{
    // Wird aufgrund Deep Sleep nicht erreicht
}
