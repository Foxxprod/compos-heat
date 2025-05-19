#include <WiFiNINA.h>
#include <ArduinoHttpClient.h>

const char* ssid = "WIFI-FAMILLE";
const char* password = "Tit0u@n2";

// Remplace l'IP par ton domaine
const char* serverAddress = "composheat.cloud"; 
const int serverPort = 80;

WiFiClient wifi;
HttpClient client = HttpClient(wifi, serverAddress, serverPort);

// Fonction pour générer une valeur aléatoire flottante
float randomFloat(float minVal, float maxVal) {
  return minVal + ((float)random(0, 10000) / 10000.0) * (maxVal - minVal);
}

void setup() {
  Serial.begin(9600);
  while (!Serial) {}

  Serial.print("Connexion au WiFi...");
  while (WiFi.begin(ssid, password) != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println(" connecté !");
  Serial.print("IP locale : ");
  Serial.println(WiFi.localIP());

  // Attendre un peu avant d'envoyer
  delay(2000);

  // Génère des données aléatoires simulées
  float watertemp = randomFloat(15.0, 30.0);
  float rate = randomFloat(1.0, 5.0);
  float composttemp = randomFloat(25.0, 50.0);
  float composthumid = randomFloat(30.0, 80.0);
  float batterylevel = randomFloat(50.0, 100.0);

  // Créer le corps JSON
  String postData = "{";
  postData += "\"watertemp\":" + String(watertemp, 2) + ",";
  postData += "\"rate\":" + String(rate, 2) + ",";
  postData += "\"composttemp\":" + String(composttemp, 2) + ",";
  postData += "\"composthumid\":" + String(composthumid, 2) + ",";
  postData += "\"batterylevel\":" + String(batterylevel, 2);
  postData += "}";

  Serial.println("Envoi des données via POST à /api/update_data :");
  Serial.println(postData);

  client.beginRequest();
  client.post("/api/update_data");
  client.sendHeader("Content-Type", "application/json");
  client.sendHeader("Content-Length", postData.length());
  client.beginBody();
  client.print(postData);
  client.endRequest();

  int statusCode = client.responseStatusCode();
  Serial.print("Code réponse : ");
  Serial.println(statusCode);

  String response = client.responseBody();
  Serial.print("Réponse : ");
  Serial.println(response);
}

void loop() {
  // Pas besoin de boucler
}
