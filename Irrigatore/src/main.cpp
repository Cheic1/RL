#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include <ArduinoJson.h>
#include <FastBot.h>
#include <EEPROM.h>
#include <OneButton.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>

// Credenziali Wi-Fi
const char *ssid = "Telecom-15744621";
const char *password = "fZET6ouLwc3wYG6WyfDy4fUL";
#define CHAT_ID "217950359"

unsigned long irrigationStartTime = 0;       // Variabile per memorizzare l'ora di inizio dell'irrigazione
bool isIrrigating = false;                   // Stato dell'irrigazione
unsigned long irrigationDuration = 5 * 1000; // Durata dell'irrigazione

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
const int pump_pin = D3;
OneButton butt1(D5, 2);
OneButton butt2(D6, 2);
OneButton humidity_sens(D4, 2);

int depth = 0;

int32_t menuID = 0;
String currentPinType = "";
int currentPinIndex = 0;
String lastMenu = "";
// Variabile di debug
int debugMode = 1; // 0: nessun debug, 1: seriale, 2: telegram, 3: entrambi

// Token del bot di Telegram
const char *TELEGRAM_BOT_TOKEN = "391032347:AAFBVponQ6ck0vd6W930dPzf6Ygj_yi5D9g";

enum PinConfigStep
{
  NONE,
  CONFIG_PIN,
  CONFIG_NAME,
  CONFIG_TYPE,
  CONFIG_CONFIRM
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

void handleMenu(int menuNum, const char *answers, const char *callbacks)
{
  String menu = "Answer " + String(menuNum) + ".1 \t Answer " + String(menuNum) + ".2 \t Answer " + String(menuNum) + ".3 \n Back";
  bot.editMenuCallback(menuID, menu, callbacks);
  depth = 1;
}
void displayMenu(String menu, int depth)
{
  String menuOptions;
  String menuCallbacks;

  if (menu == "config:d1")
  {
    menuOptions = "PIN IO \t Altre Configurazioni \t EEPROM \t Back";
    menuCallbacks = "pin_io:d2, other_configs:d2, eeprom:d2, back_main:d0";
  }
  else if (menu == "pin:d1")
  {
    menuOptions = "Controlla Output \t Visualizza Input \t Overview PIN \t Back";
    menuCallbacks = "control_output:d2, view_input:d2, overview_pin:d2, back_main:d0";
  }
  else if (menu == "debug:d1")
  {
    menuOptions = "Debug Serial \t Debug Telegram \t Debug Entrambi \t Disabilita Debug \t Back";
    menuCallbacks = "debug_serial:d2, debug_telegram:d2, debug_both:d2, debug_none:d2, back_main:d0";
  }
  else if (menu == "pin_io:d2")
  {
    menuOptions = "Inserisci PIN da configurare \t Inserisci nome del PIN \t Seleziona tipo di PIN \t Conferma configurazione \t Annulla configurazione \t Back";
    menuCallbacks = "pin_config:d3, pin_name:d3, pin_type:d3, confirm_config:d3, cancel_config:d3, back_config:d3";
  }
  else if (menu == "eeprom:d2")
  {
    menuOptions = "Reset \t Visualizza \t Back";
    menuCallbacks = "reset_eeprom:d3, view_eeprom:d3, back_config:d3";
  }
  else if (menu == "control_output:d2")
  {
    menuOptions = "[Lista PIN Output configurati] \t Toggle PIN Output \t Back";
    menuCallbacks = "toggle_pin_output:d3, toggle_pin_output:d3, back_pin_output:d3";
  }
  else if (menu == "view_input:d2")
  {
    menuOptions = "[Stato PIN Input configurati] \t Back";
    menuCallbacks = "view_input_status:d3, back_pin_output:d3";
  }
  else if (menu == "overview_pin:d2")
  {
    menuOptions = "[Lista e stato PIN Input] \t [Lista e stato PIN Output] \t Back";
    menuCallbacks = "overview_pin_input:d3, overview_pin_output:d3, back_pin:d3";
  }
  else
  {
    menuOptions = "Back";
    menuCallbacks = "back_main:d0";
  }

  bot.inlineMenuCallback(menu, menuOptions, menuCallbacks);
}

String currentMenu = "/menu:d0";

void newMsg(FB_msg &msg)
{
  debug("Ricevuto messaggio:\n " + msg.toString());

  if (msg.OTA)
    bot.update();

  if (msg.text == "/menu" || msg.data.startsWith("back"))
  {
    if (depth == 0)
    {
      currentMenu = "/menu:d0";
      bot.inlineMenuCallback("Menu principale", "Configurazione \t Pin \t Debug", "config:d1, pin:d1, debug:d1");
    }
    else
    {
      depth--;
      currentMenu = currentMenu.substring(0, currentMenu.lastIndexOf(':'));
      displayMenu(currentMenu, depth);
    }
  }
  else
  {
    if (msg.data != "")
    {
      depth++;
      currentMenu = msg.data;
      displayMenu(currentMenu, depth);
    }
  }

  
  if (msg.text.startsWith("https://github.com/"))
  {
    bot.sendMessage("Download e aggiornamento del firmware in corso...");
    WiFiClient client;
    String firmwareURL = msg.text;
    t_httpUpdate_return ret = ESPhttpUpdate.update(client, firmwareURL);

    switch (ret)
    {
    case HTTP_UPDATE_FAILED:
      bot.sendMessage("Aggiornamento fallito: " + String(ESPhttpUpdate.getLastErrorString().c_str()));
      break;
    case HTTP_UPDATE_NO_UPDATES:
      bot.sendMessage("Nessun aggiornamento disponibile.");
      break;
    case HTTP_UPDATE_OK:
      bot.sendMessage("Aggiornamento completato con successo!");
      break;
    }
  }
}

void handleButton1()
{
  bot.sendMessage("Pulsante 1 premuto!");
  digitalWrite(pump_pin, !digitalRead(pump_pin));
  isIrrigating = false;
}

void handleButton2()
{
  bot.sendMessage("Pulsante 2 premuto!");
}

void handleIrrigazione()
{

  if (irrigationDuration == 0)
    isIrrigating = false;
  if (isIrrigating)
  { // Avvia l'irrigazione solo se non è già in corso
    bot.sendMessage("Irrigazione Iniziata");
    irrigationStartTime = millis(); // Memorizza il tempo di inizio
    // isIrrigating = true;            // Imposta lo stato di irrigazione a vero
    digitalWrite(pump_pin, HIGH);
  }
  else if (isIrrigating && (millis() - irrigationStartTime >= irrigationDuration))
  {
    bot.sendMessage("Irrigazione Finita");
    isIrrigating = false; // Imposta lo stato di irrigazione a falso
    digitalWrite(pump_pin, LOW);
  }
  else
  {
  }
}

void setup()
{
  // Inizializza la comunicazione seriale
  Serial.begin(9600);

  // Configura la modalità Wi-Fi come stazione
  WiFi.mode(WIFI_STA);

  // Configura WiFiManager
  wm.setConfigPortalBlocking(false);
  wm.setConfigPortalTimeout(60);
  if (wm.autoConnect("Irrigatore"))
  {
    Serial.println("Connesso...yeey :)");
  }
  else
  {
    Serial.println("Portale di configurazione in esecuzione");
  }

  // Imposta i comandi del bot
  bot.setChatID(CHAT_ID);
  bot.attach(newMsg);

  // Invia un messaggio per indicare l'inizializzazione dell'irrigatore
  bot.sendMessage("Irrigatore inizializzato \n /menu : apri il menu setup\n");
  // bot.answer("Sicuro?");

  // Carica la configurazione dei PIN dalla EEPROM
  // loadConfig();
  // configurePins();

  // Configura i pin dei pulsanti
  butt1.attachClick(handleButton1);
  butt2.attachClick(handleButton2);
  humidity_sens.attachClick([]()
                            { bot.sendMessage("Pulsante 3 premuto!"); });

  pinMode(pump_pin, OUTPUT);
  digitalWrite(pump_pin, LOW);
}

void loop()
{
  // Gestisci il WiFiManager
  wm.process();

  // Gestisci i messaggi del bot di Telegram
  bot.tick();

  butt1.tick();
  butt2.tick();
  humidity_sens.tick();

  handleIrrigazione();
}
