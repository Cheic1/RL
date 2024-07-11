#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include <ArduinoJson.h>
#include <FastBot.h>
#include <EEPROM.h>
#include <OneButton.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
// Variabile di debug
int debugMode = 3; // 0: nessun debug, 1: seriale, 2: telegram, 3: entrambi
int Stato = 1;
const int pump_pin = D3; 

// Credenziali Wi-Fi
const char *ssid = "Telecom-15744621";
const char *password = "fZET6ouLwc3wYG6WyfDy4fUL";
#define CHAT_ID "217950359"

unsigned long irrigationStartTime = 0;       // Variabile per memorizzare l'ora di inizio dell'irrigazione
bool isIrrigating = false;                   // Stato dell'irrigazione
unsigned long irrigationDuration = 5 * 1000; // Durata dell'irrigazione

unsigned long irrigationDurationConfig = 5000; // Durata di irrigazione configurata (in millisecondi)
unsigned long irrigationStartHour = 12;        // Ora di avvio dell'irrigazione (0-23)
unsigned long irrigationStartMinute = 0;       // Minuto di avvio dell'irrigazione (0-59)
bool scheduledIrrigation = false;
// Definizione dei PIN e altre variabili globali
const int maxPins = 10; // Numero massimo di PIN configurabili

struct PinConfig
{
    int pin;
    String name;
    String type; // "o= output", "i=input", "p = input_pullup"
};

PinConfig pinConfigs[maxPins];
int pinConfigCount = 0;

OneButton butt1(D5, 2);
OneButton butt2(D6, 2);
OneButton humidity_sens(D4, 2);

int depth = 0;

int32_t menuID = 0;
String currentPinType = "";
int currentPinIndex = 0;
String lastMenu = "";

// Token del bot di Telegram
const char *TELEGRAM_BOT_TOKEN = "7422920725:AAG9RiNmdzPwYlXkMtKuv5j7FQx8aOY-jXs"; // Emmisbot

enum PinConfigStep
{
    NONE,
    CONFIG_PIN,
    CONFIG_NAME,
    CONFIG_TYPE,
    CONFIG_CONFIRM, 
    Menu1
};

PinConfigStep currentPinStep = NONE;

// WiFiManager per gestire la connessione Wi-Fi
WiFiManager wm;

// FastBot per gestire il bot di Telegram
FastBot bot(TELEGRAM_BOT_TOKEN);

void debug(String message)
{
    if (debugMode == 1 || debugMode == 3)
    {
        Serial.println(message);
    }
    if (debugMode == 2 || debugMode == 3)
    {
        bot.sendMessage(message);
    }
}

void handleError(String errorMessage, bool isCritical = false)
{
    String fullMessage = "ERRORE: " + errorMessage;
    debug(fullMessage);

    if (isCritical)
    {
        bot.sendMessage("ERRORE CRITICO: " + errorMessage + ". Il sistema verrà riavviato.");
        delay(5000); // Attendi 5 secondi prima del riavvio
        ESP.restart();
    }
    else
    {
        bot.sendMessage("ERRORE: " + errorMessage);
    }
}

void resetEEPROM()
{
    // Pulisci la EEPROM scrivendo zeri in tutte le posizioni di memoria
    for (int i = 0; i < 512; i++)
    {
        EEPROM.write(i, 0);
    }
    EEPROM.commit();
    debug("EEPROM resettata con successo.");
}

void saveConfig()
{
    EEPROM.begin(512);

    // Leggi la configurazione attuale dalla EEPROM
    String currentConfig;
    for (int i = 0; i < 512; i++)
    {
        currentConfig += char(EEPROM.read(i));
    }

    // Deserializza la configurazione attuale in un documento JSON
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, currentConfig);
    if (error)
    {
        debug("Errore nella deserializzazione della configurazione attuale: " + String(error.c_str()));
        return;
    }

    // Aggiungi la nuova configurazione al documento JSON esistente
    for (int i = 0; i <= pinConfigCount; i++)
    {
        JsonObject pin = doc.createNestedObject();
        pin["pin"] = pinConfigs[i].pin;
        pin["name"] = pinConfigs[i].name;
        pin["type"] = pinConfigs[i].type;
    }
    doc["debugMode"] = debugMode;

    // Serializza il documento JSON in una stringa JSON
    String json;
    serializeJson(doc, json);

    // Controlla la lunghezza della stringa JSON
    if (json.length() > 512)
    {
        debug("Errore: Configurazione supera la capacità di EEPROM");
        return;
    }

    // Salva la nuova configurazione nella EEPROM
    for (int i = 0; i < json.length(); i++)
    {
        EEPROM.write(i, json[i]);
    }
    EEPROM.commit();
}

void loadConfig()
{
    EEPROM.begin(512);
    String json;
    for (int i = 0; i < 512; i++)
    {
        json += char(EEPROM.read(i));
    }
    debug(json);
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, json);
    if (error)
    {
        debug("Errore nella deserializzazione: " + String(error.c_str()));
        return;
    }
    pinConfigCount = doc.size() - 1; // Assumendo che l'ultima voce sia debugMode
    debug("Numero pin configurati : " + String(pinConfigCount));
    for (int i = 0; i <= pinConfigCount; i++)
    {
        JsonObject pin = doc[i];
        pinConfigs[i].pin = pin["pin"];
        pinConfigs[i].name = pin["name"].as<String>();
        pinConfigs[i].type = pin["type"].as<String>();
        debug("Pin configurato come : " + String(pin));
    }
    debugMode = doc["debugMode"];
    debug("Debug mode : " + String(debugMode));
}

void configurePins()
{
    for (int i = 0; i <= pinConfigCount; i++)
    {
        String msg = "";
        msg += "Numero : " + String(pinConfigCount);
        msg += "\nName    : " + String(pinConfigs[i].name);
        msg += "\nPin    : " + String(pinConfigs[i].pin);
        msg += "\nType    : " + String(pinConfigs[i].type);
        debug(msg);
        if (pinConfigs[i].type == "o")
        {
            pinMode(pinConfigs[i].pin, OUTPUT);
        }
        else if (pinConfigs[i].type == "i")
        {
            pinMode(pinConfigs[i].pin, INPUT);
        }
        else if (pinConfigs[i].type == "p")
        {
            pinMode(pinConfigs[i].pin, INPUT_PULLUP);
        }
    }
}

void updateProgress(int progress, int total)
{
    static int lastProgress = -1;
    int percent = (progress * 100) / total;

    if (percent != lastProgress)
    {
        lastProgress = percent;
        String progressBar = "[";
        for (int i = 0; i < 20; i++)
        {
            if (i < percent / 5)
            {
                progressBar += "█";
            }
            else
            {
                progressBar += "░";
            }
        }
        progressBar += "]";

        bot.editMessage(menuID, String(percent) + String("% ") + String(progressBar));
    }
}

void handleGitHubUpdate(FB_msg &msg)
{
    String gitHubUrl = msg.text;
    int32_t msg_id = msg.ID;
    bot.sendMessage("Inizio processo di aggiornamento da GitHub...");

    if (!gitHubUrl.startsWith("https://github.com/"))
    {
        handleError("URL non valido. Deve essere un link GitHub.");
        return;
    }

    String rawUrl = gitHubUrl;
    rawUrl.replace("https://github.com/", "https://raw.githubusercontent.com/");
    rawUrl.replace("/blob/", "/");

    bot.sendMessage("Scaricamento del firmware da: " + rawUrl);
    bot.tickManual(); // Чтобы отметить сообщение прочитанным
    WiFiClientSecure client;
    client.setInsecure(); // Nota: questo disabilita la verifica SSL. Usare con cautela.

    ESPhttpUpdate.rebootOnUpdate(false);
    // Imposta la funzione di callback per il progresso
    //ESPhttpUpdate.onProgress(updateProgress);

    // Crea un messaggio che verrà aggiornato con il progresso
    menuID = bot.sendMessage("Aggiornamento in corso: 0%");

    t_httpUpdate_return ret = ESPhttpUpdate.update(client, rawUrl);

    // Rimuovi la funzione di callback
    //ESPhttpUpdate.onProgress(nullptr);

    // Elimina il messaggio di progresso
    //bot.deleteMessage(menuID);

    switch (ret)
    {
    case HTTP_UPDATE_FAILED:
        handleError("Aggiornamento fallito: " + String(ESPhttpUpdate.getLastErrorString().c_str()));
        break;
    case HTTP_UPDATE_NO_UPDATES:
        bot.sendMessage("Nessun aggiornamento disponibile.");
        break;
    case HTTP_UPDATE_OK:
        bot.sendMessage("Aggiornamento completato con successo.");
        break;
    }
}

void handleButton1()
{
    if (!isIrrigating)
    {
        bot.sendMessage("Irrigazione Iniziata tramite pulsante 1");
        irrigationStartTime = millis();
        isIrrigating = true;
        digitalWrite(pump_pin, HIGH);
    }
}

void handleButton2()
{
    bot.sendMessage("Pulsante 2 premuto!");
}

void handleIrrigazione()
{
    if (isIrrigating && (millis() - irrigationStartTime >= irrigationDurationConfig))
    {
        bot.sendMessage("Irrigazione Finita");
        isIrrigating = false;
        digitalWrite(pump_pin, LOW);
    }

    if (scheduledIrrigation)
    {
        // time_t now = time(nullptr);
        // struct tm *currentTime = localtime(&now);
        FB_Time t(bot.getUnix(), 2);
        if (t.hour == irrigationStartHour && t.minute == irrigationStartMinute && t.second == 00 && !isIrrigating)
        {
            bot.sendMessage("Irrigazione programmata Iniziata");
            irrigationStartTime = millis();
            isIrrigating = true;
            digitalWrite(pump_pin, HIGH);
        }
    }
}
// Aggiungi queste funzioni per gestire il menu inline
void showConfigMenu(FB_msg &msg, int page = 1)
{
    if (page == 0)
    {
        bot.inlineMenuCallback("Configurazione Irrigazione",
                               "Menu-config\tReset\nAttiva/Disattiva Programmazione",
                               "/menu,/reset,toggle_schedule");
    }
    else if (page == 1)
    {
        bot.inlineMenuCallback("Configurazione Irrigazione",
                               "Imposta Durata\tImposta Ora\nAttiva/Disattiva Programmazione",
                               "set_duration,set_time,toggle_schedule");
    }

}

void handleConfigCallback(FB_msg &msg)
{
    if (msg.data == "set_duration")
    {
        bot.sendMessage("Inserisci la durata dell'irrigazione in secondi:");
        currentPinStep = CONFIG_PIN; // Usa questo enum per gestire la prossima risposta
    }
    else if (msg.data == "set_time")
    {
        bot.sendMessage("Inserisci l'ora di avvio dell'irrigazione nel formato HH:MM");
        currentPinStep = CONFIG_NAME; // Usa questo enum per gestire la prossima risposta
    }
    else if (msg.data == "toggle_schedule")
    {
        scheduledIrrigation = !scheduledIrrigation;
        bot.sendMessage(scheduledIrrigation ? "Irrigazione programmata attivata" : "Irrigazione programmata disattivata");
        saveConfig();
    }
}



uint32_t startUnix; // храним время

void newMsg(FB_msg &msg)
{
    FB_Time t(msg.unix, 2);
    if (msg.unix < startUnix)
        return; // игнорировать сообщения
    debug("Nuovo messaggio ricevuto: " + msg.toString());

    if (msg.OTA)
        bot.update();
    if (msg.text == "/menu" || msg.text == "/config")
    {
        showConfigMenu(msg);
    }
    else if (msg.text == "/reset")
    {
        resetEEPROM();
    }
    else if (msg.text.startsWith("https://github.com/"))
    {
        handleGitHubUpdate(msg);
    }
    else if (msg.data != "")
    {
        handleConfigCallback(msg);
    }
    else
    {
        // Gestisci le risposte dell'utente per la configurazione
        switch (currentPinStep)
        {
        case CONFIG_PIN:                                        // Imposta durata
            irrigationDurationConfig = msg.text.toInt() * 1000; // Converti in millisecondi
            bot.sendMessage("Durata irrigazione impostata a " + String(irrigationDurationConfig / 1000) + " secondi");
            currentPinStep = NONE;
            saveConfig();
            break;
        case CONFIG_NAME: // Imposta ora
        {
            int hour, minute;
            sscanf(msg.text.c_str(), "%d:%d", &hour, &minute);
            if (hour >= 0 && hour < 24 && minute >= 0 && minute < 60)
            {
                irrigationStartHour = hour;
                irrigationStartMinute = minute;
                bot.sendMessage("Ora di avvio irrigazione impostata alle " + String(hour) + ":" + String(minute));
                saveConfig();
            }
            else
            {
                bot.sendMessage("Formato ora non valido. Usa HH:MM");
            }
            currentPinStep = NONE;
        }
        break;
        default:
            bot.sendMessage("Comando non riconosciuto. Usa /config per configurare l'irrigazione.");
            break;
        }
    }
}

void setup()
{
    // Inizializzazione della seriale
    Serial.begin(9600);

    // Configura i PIN di ingresso e uscita
    pinMode(pump_pin, OUTPUT);

    // Connetti alla rete Wi-Fi
    wm.autoConnect(ssid, password);

    // Configura il bot di Telegram
    // Imposta i comandi del bot
    bot.setChatID(CHAT_ID);
    bot.attach(newMsg);
    startUnix = bot.getUnix(); // запомнили
    // Invia un messaggio per indicare l'inizializzazione dell'irrigatore
    bot.sendMessage("Irrigatore inizializzato \n /menu : apri il menu setup\n");
    // bot.answer("Sicuro?");
    FB_Time t(bot.getUnix(), 2);
    debug("Ora attuale: " + t.timeString());
    // Carica la configurazione dei PIN dalla EEPROM
    // loadConfig();
    // configurePins();

    // Configura i pin dei pulsanti
    butt1.attachClick(handleButton1);
    butt2.attachClick(handleButton2);
    humidity_sens.attachClick([]()
                              { bot.sendMessage("Pulsante 3 premuto!"); });

    // Carica la configurazione dalla EEPROM
    // loadConfig();

    // Configura i PIN in base alla configurazione caricata
    // configurePins();

    // Debug
    debug("Sistema avviato.");
}

void loop()
{
    // Verifica nuovi messaggi per il bot di Telegram
    bot.tick();
    // Controllo dei pulsanti
    butt1.tick();
    butt2.tick();
    humidity_sens.tick();

    //handle irrigazione
    handleIrrigazione();
}
