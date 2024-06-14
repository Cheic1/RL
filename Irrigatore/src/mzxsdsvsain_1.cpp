// #include <ESP8266WiFi.h>
// #include <ESP8266WebServer.h>
// #include <ArduinoJson.h>

// // Credenziali Wi-Fi
// const char* ssid = "Telecom-15744621";
// const char* password = "fZET6ouLwc3wYG6WyfDy4fUL";

// // Crea un server web sulla porta 80
// ESP8266WebServer server(80);

// // Definisci la dimensione massima del buffer JSON
// const size_t bufferSize = JSON_OBJECT_SIZE(5) + 100;
// DynamicJsonDocument doc(bufferSize);

// // Variabile String per contenere il codice HTML
// String htmlCode = 
//   "<!DOCTYPE html>"
//   "<html>"
//   "<head>"
//   "<title>Gestione Piante</title>"
//   "<style>"
//   "body {"
//   "  font-family: Arial, sans-serif;"
//   "  text-align: center;"
//   "  background-color: #f7f7f7;"
//   "  margin: 0;"
//   "  padding: 20px;"
//   "}"
//   "h1 {"
//   "  color: #5D4037;"
//   "}"
//   ".container {"
//   "  max-width: 600px;"
//   "  margin: auto;"
//   "  background: white;"
//   "  padding: 20px;"
//   "  border-radius: 10px;"
//   "  box-shadow: 0 0 10px rgba(0, 0, 0, 0.1);"
//   "}"
//   "label {"
//   "  display: block;"
//   "  margin: 15px 0 5px;"
//   "}"
//   "input[type=\"text\"], input[type=\"time\"], input[type=\"number\"] {"
//   "  width: calc(100% - 22px);"
//   "  padding: 10px;"
//   "  margin: 5px 0 10px;"
//   "  border: 1px solid #ccc;"
//   "  border-radius: 5px;"
//   "}"
//   "button {"
//   "  background-color: #5D4037;"
//   "  color: white;"
//   "  padding: 10px 20px;"
//   "  border: none;"
//   "  border-radius: 5px;"
//   "  cursor: pointer;"
//   "}"
//   "button:hover {"
//   "  background-color: #795548;"
//   "}"
//   ".status {"
//   "  margin-top: 20px;"
//   "  padding: 10px;"
//   "  background-color: #E0E0E0;"
//   "  border-radius: 5px;"
//   "}"
//   "</style>"
//   "</head>"
//   "<body>"
//   "<div class=\"container\">"
//   "  <h1>Gestione Piante sul Balcone</h1>"
//   "  <form id=\"plantForm\">"
//   "    <label for=\"plantName\">Nome della Pianta:</label>"
//   "    <input type=\"text\" id=\"plantName\" name=\"plantName\" required>"
//   "    <label for=\"waterTime\">Orario di Irrigazione:</label>"
//   "    <input type=\"time\" id=\"waterTime\" name=\"waterTime\" required>"
//   "    <label for=\"waterAmount\">Quantità di Acqua (ml):</label>"
//   "    <input type=\"number\" id=\"waterAmount\" name=\"waterAmount\" min=\"1\" required>"
//   "    <button type=\"submit\">Imposta Programma</button>"
//   "  </form>"
//   "  <div class=\"status\" id=\"status\">"
//   "    <h2>Stato Attuale</h2>"
//   "    <p id=\"currentStatus\">Nessun programma impostato</p>"
//   "  </div>"
//   "</div>"
//   "<script>"
//   "  document.getElementById('plantForm').addEventListener('submit', function(event) {"
//   "    event.preventDefault();"
//   "    const plantName = document.getElementById('plantName').value;"
//   "    const waterTime = document.getElementById('waterTime').value;"
//   "    const waterAmount = document.getElementById('waterAmount').value;"
//   "    const data = {"
//   "      plantName: plantName,"
//   "      waterTime: waterTime,"
//   "      waterAmount: waterAmount"
//   "    };"
//   "    fetch('/setSchedule', {"
//   "      method: 'POST',"
//   "      headers: {"
//   "        'Content-Type': 'application/json'"
//   "      },"
//   "      body: JSON.stringify(data)"
//   "    })"
//   "    .then(response => response.json())"
//   "    .then(data => {"
//   "      document.getElementById('currentStatus').innerText = `Pianta: ${data.plantName}, Orario: ${data.waterTime}, Acqua: ${data.waterAmount} ml`;"
//   "    })"
//   "    .catch(error => console.error('Errore:', error));"
//   "  });"

//   "  function fetchCurrentStatus() {"
//   "    fetch('/getCurrentStatus')"
//   "    .then(response => response.json())"
//   "    .then(data => {"
//   "      if (data.plantName) {"
//   "        document.getElementById('currentStatus').innerText = `Pianta: ${data.plantName}, Orario: ${data.waterTime}, Acqua: ${data.waterAmount} ml`;"
//   "      }"
//   "    })"
//   "    .catch(error => console.error('Errore:', error));"
//   "  }"
//   "  fetchCurrentStatus();"
//   "</script>"
//   "</body>"
//   "</html>";

// // Variabili per memorizzare le informazioni della pianta
// String plantName = "";
// String waterTime = "";
// int waterAmount = 0;

// // Dichiarazione delle funzioni di callback
// void handleRoot();
// void handleSetSchedule();
// void handleGetCurrentStatus();

// void setup() {
//   Serial.begin(9600);

//   // Connetti al Wi-Fi
//   WiFi.begin(ssid, password);
//   Serial.print("Connessione a ");
//   Serial.println(ssid);

//   // Attendi la connessione
//   while (WiFi.status() != WL_CONNECTED) {
//     delay(500);
//     Serial.print(".");
//   }

//   Serial.println("");
//   Serial.println("Connesso! Indirizzo IP: ");
//   Serial.println(WiFi.localIP());

//   // Imposta le funzioni di callback
//   server.on("/", handleRoot);
//   server.on("/setSchedule", HTTP_POST, handleSetSchedule);
//   server.on("/getCurrentStatus", HTTP_GET, handleGetCurrentStatus);

//   // Inizia il server web
//   server.begin();
// }

// void loop() {
//   server.handleClient();
// }

// // Funzioni di callback per le richieste HTTP

// void handleRoot() {
//   server.send(200, "text/html", htmlCode);
// }

// void handleSetSchedule() {
//   // Ottieni i dati JSON dalla richiesta
//   if (server.hasArg("plain")) {
//     String json = server.arg("plain");
//     deserializeJson(doc, json);
//     plantName = doc["plantName"].as<String>();
//     waterTime = doc["waterTime"].as<String>();
//     waterAmount = doc["waterAmount"].as<int>();

//     // Invia la risposta JSON al client
//     doc.clear();
//     doc["plantName"] = plantName;
//     doc["waterTime"] = waterTime;
//     doc["waterAmount"] = waterAmount;
//     String response;
//     serializeJson(doc, response);
//     server.send(200, "application/json", response);
//   } else {
//     server.send(400, "text/plain", "Richiesta non valida");
//   }
// }

// void handleGetCurrentStatus() {
//   // Invia la risposta JSON al client
//   doc.clear();
//   doc["plantName"] = plantName;
//   doc["waterTime"] = waterTime;
//   doc["waterAmount"] = waterAmount;
//   String response;
//   serializeJson(doc, response);
//   server.send(200, "application/json", response);
// }
