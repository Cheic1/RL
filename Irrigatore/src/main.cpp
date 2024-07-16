#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include <ArduinoJson.h>
#include <FastBot.h>
#include <EEPROM.h>
#include <OneButton.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <LittleFS.h>

#define APP_VERSION "0.0.31"

// void loadConfig();
// void saveConfig();

time_t now = time(nullptr);
struct tm *currentTime = localtime(&now);
// Variabile di debug
int debugMode = 3; // 0: nessun debug, 1: seriale, 2: telegram, 3: entrambi
int Stato = 1;
const int pump_pin = D3;

// Credenziali Wi-Fi
const char *ssid = "Telecom-15744621";
const char *password = "fZET6ouLwc3wYG6WyfDy4fUL";
#define CHAT_ID "217950359"

// Array di chat ID
const String chatIds[] = {"217950359", "851696190"};
const int numChats = sizeof(chatIds) / sizeof(chatIds[0]);
const String chatIds1 = "217950359,851696190";
unsigned long irrigationStartTime = 0; // Variabile per memorizzare l'ora di inizio dell'irrigazione
bool isIrrigating = false;             // Stato dell'irrigazione

unsigned long irrigationDurationConfig = 30 * 1000; // Durata di irrigazione configurata (in millisecondi)
unsigned long irrigationStartHour = 20;             // Ora di avvio dell'irrigazione (0-23)
unsigned long irrigationStartMinute = 0;            // Minuto di avvio dell'irrigazione (0-59)
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
// const char *TELEGRAM_BOT_TOKEN = "391032347:AAFBVponQ6ck0vd6W930dPzf6Ygj_yi5D9g"; // CheicBot
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
    debugMode = 3; // Visualizza il debug su seriale, telegram e entrambi
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

void readConfig()
{
    if (!LittleFS.begin())
    {
        Serial.println("An Error has occurred while mounting LittleFS");
        return;
    }

    File file = LittleFS.open("/config.json", "r");
    if (!file)
    {
        Serial.println("Failed to open file for reading");
        return;
    }

    Serial.println("File Content:");
    while (file.available())
    {
        Serial.write(file.read());
    }
    file.close();
}
void saveConfig()
{
    StaticJsonDocument<512> doc;

    // for (int i = 0; i <= pinConfigCount; i++)
    // {
    //     JsonObject pin = doc.createNestedObject();
    //     pin["pin"] = pinConfigs[i].pin;
    //     pin["name"] = pinConfigs[i].name;
    //     pin["type"] = pinConfigs[i].type;
    // }
    doc["debugMode"] = debugMode;
    doc["irrigationDurationConfig"] = irrigationDurationConfig;
    doc["irrigationStartHour"] = irrigationStartHour;
    doc["irrigationStartMinute"] = irrigationStartMinute;
    doc["scheduledIrrigation"] = scheduledIrrigation;

    // Verifica se il file esiste, se non esiste lo crea
    if (!LittleFS.exists("/config.json"))
    {
        debug("Config file does not exist, creating it");
        File configFile = LittleFS.open("/config.json", "w");
        if (!configFile)
        {
            debug("Failed to create config file");
            return;
        }
        configFile.close();
    }

    // Apre il file in modalità scrittura
    File configFile = LittleFS.open("/config.json", "w");
    if (!configFile)
    {
        debug("Failed to open config file for writing");
        return;
    }

    serializeJson(doc, configFile);
    configFile.close();
    debug("Configuration saved successfully");
    // loadConfig();
}
void loadConfig()
{

    if (!LittleFS.begin())
    {
        debug("Failed to mount file system");
        return;
        // }else {
        // delay(100);
        // debug("Mounting file system...");
    }

    if (!LittleFS.exists("/config.json"))
    {
        debug("Config file not found");
        return;
    }
    else
    {
        // debug("Config file found");
        // delay(100);
    }

    File configFile = LittleFS.open("/config.json", "r");
    if (!configFile)
    {
        debug("Failed to open config file");
        return;
    }
    else
    {
        // debug("Reading config file");
    }

    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, configFile);
    if (error)
    {
        debug("Failed to parse config file");
        return;
    }

    // pinConfigCount = doc.size() - 5; // Sottraiamo 5 per le altre configurazioni
    // for (int i = 0; i < pinConfigCount; i++)
    // {
    //     JsonObject pin = doc[i];
    //     pinConfigs[i].pin = pin["pin"];
    //     pinConfigs[i].name = pin["name"].as<String>();
    //     pinConfigs[i].type = pin["type"].as<String>();
    // }

    debugMode = doc["debugMode"];
    // debug("debug mode: " + String(debugMode));
    irrigationDurationConfig = doc["irrigationDurationConfig"];
    if (irrigationDurationConfig == 0) irrigationDurationConfig = 30 * 1000;
    // debug("irrigation duration: " + String(irrigationDurationConfig) + "ms");
    irrigationStartHour = doc["irrigationStartHour"];
    irrigationStartMinute = doc["irrigationStartMinute"];
    scheduledIrrigation = doc["scheduledIrrigation"];

    configFile.close();
    debug("Configuration loaded successfully");
    // debug("debug mode: " + String(debugMode));

    debug("debug mode: " +      String(debugMode) + "\n" +
          "irrigation duration: " +     String(irrigationDurationConfig) + "ms\n" +
          "irrigation start hour: " +   String(irrigationStartHour) + "\n" +
          "irrigation start minute: " + String(irrigationStartMinute) + "\n" +
          "scheduledIrrigation: " + String(scheduledIrrigation));
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
    // ESPhttpUpdate.onProgress(updateProgress);

    // Crea un messaggio che verrà aggiornato con il progresso
    menuID = bot.sendMessage("Aggiornamento in corso: 0%");

    t_httpUpdate_return ret = ESPhttpUpdate.update(client, rawUrl);

    // Rimuovi la funzione di callback
    // ESPhttpUpdate.onProgress(nullptr);

    // Elimina il messaggio di progresso
    // bot.deleteMessage(menuID);

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
        bot.sendMessage("Irrigazione Finita dopo " + String((millis() - irrigationStartTime) / 1000) + " s");
        isIrrigating = false;
        digitalWrite(pump_pin, LOW);
    }

    if (scheduledIrrigation)
    {
        // time_t now = time(nullptr);
        // struct tm *currentTime = localtime(&now);
        FB_Time t(bot.getUnix(), 2);

        if (currentTime->tm_hour == irrigationStartHour && currentTime->tm_min == irrigationStartMinute && currentTime->tm_sec == 00 && !isIrrigating)
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
        for (int i = 0; i < numChats; i++)
        {
            bot.inlineMenuCallback("Configurazione Irrigazione",
                                   "Menu-config\tReset\nAttiva/Disattiva Programmazione",
                                   "/menu,/reset,toggle_schedule", chatIds[i]);
        }
    }
    else if (page == 1)
    {
        for (int i = 0; i < numChats; i++)
        {
            bot.inlineMenuCallback("Configurazione Irrigazione",
                                   "Imposta Durata\tImposta Ora\tON/OFF manuale\n ON/OFF Programmazione",
                                   "set_duration,set_time,toggle_manual,toggle_schedule", chatIds[i]);
        }
    }
}

void handleConfigCallback(FB_msg &msg)
{
    if (msg.data == "set_duration")
    {
        debug("Inserisci la durata dell'irrigazione in secondi:");
        currentPinStep = CONFIG_PIN; // Usa questo enum per gestire la prossima risposta
    }
    else if (msg.data == "set_time")
    {
        debug("Inserisci l'ora di avvio dell'irrigazione nel formato HH:MM");
        currentPinStep = CONFIG_NAME; // Usa questo enum per gestire la prossima risposta
    }
    else if (msg.data == "toggle_schedule")
    {
        scheduledIrrigation = !scheduledIrrigation;
        debug(scheduledIrrigation ? "Irrigazione programmata attivata" : "Irrigazione programmata disattivata");
        saveConfig();
    }
    else if (msg.data == "toggle_manual")
    {
        if (!isIrrigating)
        {
            bot.sendMessage("Irrigazione Iniziata tramite toggle_manuale");
            irrigationStartTime = millis();
            isIrrigating = true;
            digitalWrite(pump_pin, HIGH);
        }
    }
}

uint32_t startUnix; // храним время

void newMsg(FB_msg &msg)
{
    FB_Time t(msg.unix, 2);
    if (msg.unix < startUnix)
    {
        debug("messaggio bloccato da :" + String(startUnix));
        return; // Blocca per i messaggi precedenti a quelli che sono stati inviati
    }

    debug(msg.toString());
    // bot.sendMessage(msg.toString());

    delay(100);
    if (msg.OTA)
        bot.update();

    if (msg.text == "/menu" || msg.text == "/config")
    {
        showConfigMenu(msg);
    }
    else if (msg.text == "/time")
    {
        debug("Ora attuale: " + String(currentTime->tm_hour) + ":" + String(currentTime->tm_min) + ":" + String(currentTime->tm_sec));
    }
    else if (msg.text == "/save")
    {
        debug("Salvataggio configurazione... /save");
        saveConfig();
    }
    else if (msg.text == "/attiva")
    {
        if (!isIrrigating)
        {
            debug("Irrigazione Iniziata tramite /attiva");
            debug("Durata attesa: " + String(irrigationDurationConfig / 1000) + " secondi");
            irrigationStartTime = millis();
            isIrrigating = true;
            digitalWrite(pump_pin, HIGH);
        }
    }
    else if (msg.text == "/disattiva")
    {
        if (isIrrigating)
        {
            bot.sendMessage("Irrigazione Finita tramite /disattiva");
            isIrrigating = false;
            digitalWrite(pump_pin, LOW);
        }
    }

    else if (msg.text == "/load")
    {
        debug("load.. /load");
        loadConfig();
    }
    else if (msg.text == "/readConfig")
    {
        debug("readConfig.. /readConfig");
        readConfig();
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
            debug("Durata irrigazione impostata a " + String(irrigationDurationConfig / 1000) + " secondi");
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
                debug("Ora di avvio irrigazione impostata alle " + String(hour) + ":" + String(minute));
                saveConfig();
            }
            else
            {
                debug("Formato ora non valido. Usa HH:MM");
            }
            currentPinStep = NONE;
        }
        break;
        default:
            debug("Comando non riconosciuto. Usa /config per configurare l'irrigazione.");
            break;
        }
    }
}

void handleTime()
{
    if (millis() - now > 10000)
    {

        // debug("Ora attuale: " + String(currentTime->tm_hour) + ":" + String(currentTime->tm_min) + ":" + String(currentTime->tm_sec));
    }
}
void setup()
{
    // Inizializzazione della seriale
    Serial.begin(9600);

    // Configura i PIN di ingresso e uscita
    pinMode(pump_pin, OUTPUT);

    // Configura la modalità Wi-Fi come stazione
    WiFi.mode(WIFI_STA);

    // Configura WiFiManager
    // wm.setConfigPortalBlocking(false);
    wm.setConfigPortalTimeout(60);
    if (wm.autoConnect("Irrigatore"))
    {
        Serial.println("Connesso...yeey :)");
    }
    else
    {
        Serial.println("Portale di configurazione in esecuzione");
    }

    // Connetti alla rete Wi-Fi
    // wm.autoConnect(ssid, password);

    // Configura il bot di Telegram
    // Imposta i comandi del bot
    bot.setChatID(chatIds1);
    bot.attach(newMsg);
    startUnix = bot.getUnix(); // запомнили
    // Invia un messaggio per indicare l'inizializzazione dell'irrigatore
    debug("Irrigatore inizializzato \n /menu : apri il menu setup\n");
    debug("Version : " + String(APP_VERSION));
    // bot.answer("Sicuro?");
    FB_Time t(bot.getUnix(), 2);
    configTime(2 * 3600, 0, "pool.ntp.org", "time.nist.gov");
    // Attendiamo un po' per assicurarci che il tempo sia stato sincronizzato
    while (time(nullptr) < 1000000000)
    {
        delay(100);
    }

    time_t now;
    time(&now);
    currentTime = localtime(&now);
    debug("Ora attuale: t " + t.timeString() + "\nOra attuale: currentTime " + String(currentTime->tm_hour) + ":" + String(currentTime->tm_min) + ":" + String(currentTime->tm_sec));

    // Carica la configurazione dei PIN dalla EEPROM
    // loadConfig();
    // saveConfig();
    // configurePins();

    // Configura i pin dei pulsanti
    butt1.attachClick(handleButton1);
    butt2.attachClick(handleButton2);
    humidity_sens.attachLongPressStart([]()
                                       { bot.sendMessage("Pulsante 3 premuto!"); });

    // Carica la configurazione dalla EEPROM
    loadConfig();

    // Configura i PIN in base alla configurazione caricata
    // configurePins();

    // Debug
    debug("Sistema avviato.");
    debug("debug mode: " + String(debugMode) + "\n " +
          "irrigation duration: " + String(irrigationDurationConfig) + "ms\n" +
          "irrigation start hour: " + String(irrigationStartHour) + "\n"
                                                                    "irrigation start minute: " +
          String(irrigationStartMinute) + "\n" +
          "scheduledIrrigation: " + String(scheduledIrrigation));
}

void loop()
{
    // Verifica nuovi messaggi per il bot di Telegram
    bot.tick();
    // Controllo dei pulsanti
    butt1.tick();
    butt2.tick();
    humidity_sens.tick();

    // handle irrigazione
    handleIrrigazione();

    // handleTime();
}
