//PROJET SI, RECUPERATION CHALEURE COMPOST
//CODE ARDUINO, INTERFACEE AVEC L'APPLICATION MOBILE COMPOS'HEAT



//------------------------IMPORT DES MODULES---------------------------------//
#include <Arduino.h>
#include <OneWire.h> //one wire pour capteures
#include <DallasTemperature.h> //interpreter temperature du capteur
#include <DHT.h> //module pour gerer la capteur temp air / humiditée
#include <Wire.h> //gestion de l'ecran lcd
#include "rgb_lcd.h" //gestion de l'ecran lcd
#include <SPI.h>
#include <WiFiNINA.h>
//---------------------------------------------------------------------------//


//------------------------DEFINITION DES IO----------------------------------//
#define ONE_WIRE_BUS_WATERSENSOR 8// Broche pour le DS18B20 temperature de l'eau compost
#define ONE_WIRE_BUS_POOLSENSOR 4 // Broche pour le DS18B20 temperature de temperature piscine
#define DHTPIN 5        // Broche numérique D8 où le capteur est connecté temperature et humiditée du compost
#define DHTTYPE DHT11   // Type du capteur de temp / humiditée de l'int compost
//---------------------------------------------------------------------------//



OneWire watersensorbus(ONE_WIRE_BUS_WATERSENSOR);
OneWire poolsensorbus(ONE_WIRE_BUS_POOLSENSOR);
DallasTemperature watersensor(&watersensorbus); //capteur temperature de l'eau
DallasTemperature poolsensor(&poolsensorbus); //capteur temperature de l'eau
DHT temphumid(DHTPIN, DHTTYPE); // Initialisation du capteur DHT
rgb_lcd lcd;
WiFiClient client;
//---------------------------------------------------------------------------//


//-----------------------DEFINITION DES CONSTANTES--------------------------//
//CONSTANTES ECRAN RGB LCD
const int colorR = 0;
const int colorG = 200;
const int colorB = 0;

//CONSTANTES CONTROLE SHIELD MOTEUR POUR POMPE
const int directionPin = 12;
const int pwmPin = 3;
const int brakePin = 9;
bool directionState;

int pumpstate = -1;
int newtemperature = -1;
int pumpspeed = 255;


const int pinDebit = 2; 
volatile int nbImpulsions = 0;
float debit = 0.0;
unsigned long lastTime = 0;

bool pompeEnMarche = false;
int command = 0;

//POTENTIOMMETRE DE COMMANDE 
const int potPin = A0; 
int potValue = 0; 
float commandepot = 0;
bool commandpoton = false;


//WIFI
char ssid[] = "SI-eleve";         // Nom du réseau Wi-Fi
char pass[] = "AccessSI"; // Mot de passe du Wi-Fi

char server[] = "composheat.cloud"; // Serveur
int port = 80;                      // Port HTTP


///////////////////////////////////////////////////////////////////////////////
/////////////////////////FONTIONNEMENT DU CODE/////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

//configuration du bluethoot
void setupBlueToothConnection() {
    Serial1.print("AT");
    delay(400); 

    Serial1.print("AT+DEFAULT");  // Réinitialisation du module Bluetooth
    delay(2000); 

    Serial1.print("AT+NAMECOMPOS'HEAT");  // Nom du module Bluetooth
    delay(400);

    Serial1.print("AT+PIN0000");  // Code PIN
    delay(400);

    Serial1.print("AT+AUTH1");  // Authentification
    delay(400);    

    Serial1.flush();  // Purge du buffer pour préparer la communication
}

// Fonction de test de la connexion Bluetooth
void testBluetoothConnection() {
    Serial1.println("AT");  // Envoie une commande AT au module Bluetooth pour tester la connexion
    delay(500);

    if (Serial1.available()) {
        String response = "";
        while (Serial1.available()) {
            char c = Serial1.read();
            response += c;
        }
        // Si la réponse est OK, la connexion est établie
        if (response.indexOf("OK") != -1) {
            Serial.println("lE MODULE BLUETOOTH EST PRET");
        } else {
            Serial.println("IMPOSSIBLE DE SE CONNECTER AU MODULE BLUETOOTH");
        }
    }
}

String getBluetoothData(){
    String data = "";
    
    while (Serial1.available()) {
        char c = Serial1.read();
        data += c;
        delay(2);
    }

    return data;
}

void receiveDataFromApp(){
    // Récupérer les données Bluetooth
    String data = getBluetoothData();
    
    // Cherche la première occurrence de RX et $RX dans le message
    int startPos = data.indexOf("RX");
    int endPos = data.indexOf("$RX");

    // Si le message contient bien RX et $RX, et que RX est avant $RX
    if (startPos != -1 && endPos != -1 && startPos < endPos) {
        // Extraire les données entre "RX" et "$RX"
        String extractedData = data.substring(startPos + 2, endPos);  // Exclure "RX" et "$RX"

        // Diviser les données extraites par ';' pour obtenir les paires clé-valeur
        int pos = 0;
        while ((pos = extractedData.indexOf(';')) != -1) {
            String entry = extractedData.substring(0, pos);
            int separatorPos = entry.indexOf(':');
            
            if (separatorPos != -1) {
                String key = entry.substring(0, separatorPos);   // Clé
                String value = entry.substring(separatorPos + 1); // Valeur

                // Si la clé est "pumpstate", extraire la valeur
                if (key.equals("pumpstate")) {  // Utiliser equals() pour comparer les String
                    pumpstate = value.toInt();  // Convertir la valeur en entier
                }

                // Si la clé est "newtemperature", extraire la valeur
                if (key.equals("newtemperature")) {  // Utiliser equals() pour comparer les String
                    newtemperature = value.toInt();  // Convertir la valeur en entier
                }

                if (key.equals("command")) {  // Utiliser equals() pour comparer les String
                    command = value.toInt();  // Convertir la valeur en entier
                }
            }
            
            // Supprimer la partie traitée
            extractedData = extractedData.substring(pos + 1);
        }

        // Si pumpstate est égal à 1, démarrer la pompe, sinon l'arrêter
        if (pumpstate == 1 && !pompeEnMarche && command == 0) {
            startPump();  // Démarre la pompe
            pompeEnMarche = true;
        } else if (pumpstate == 0 && pompeEnMarche && command == 0) {
            stopPump();   // Arrête la pompe
            pompeEnMarche = false;
        }

        
    }
}


void reiceiveDataFromWeb() {
    if (Serial.available() > 0) { // Si des données sont disponibles
      String command = Serial.readStringUntil('\n');
      command.trim();
  
      if (command.startsWith("startpump")) {
        startPump();
      }
      else if (command.startsWith("stoppump")) {
        stopPump();
      }
      else if (command.startsWith("updatecommand")) {
        int tempIndex = command.indexOf(":") + 1;
        String tempValue = command.substring(tempIndex);
        newtemperature = tempValue.toFloat(); // Convertir en float
      }
      else {
        Serial.println("Commande non reconnue");
      }
    }
  }


//Recuperer la temperature de capteur de l'eau
float getWaterTemp() {
    watersensor.requestTemperatures(); //demande lecture
    float temperature = watersensor.getTempCByIndex(0); //recupere la donnee de temp
    
    return temperature; //return la temperature mesurée
}

//recuperer la temperature de l'eau de la piscine 
float getPoolTemp() {
    poolsensor.requestTemperatures(); //demande lecture
    float temperature = poolsensor.getTempCByIndex(0); //recupere la donnee de temp
    
    return temperature; //return la temperature mesurée
}

//recuperer temperature dans le compost
float getCompostTemp() {
    float temperature = temphumid.readTemperature();

    return temperature;
}

//recuperer l'humiditée dans le compost
float getComposthumid() {
    float humidity = temphumid.readHumidity();

    return humidity;
}

//mesurer / renvoyer la mesure de la batterie (a faire)
int getBatterylevel() {
    return 100;
}




void startPump(){
    digitalWrite(brakePin, LOW);
    digitalWrite(directionPin, HIGH);

    analogWrite(pwmPin, pumpspeed);
}

void stopPump(){
    digitalWrite(brakePin, HIGH);
    digitalWrite(directionPin, HIGH);

    analogWrite(pwmPin, 0);
}
//Construction de la trame a envoyer a l'application
String trametx() {
    float waterTemp = getWaterTemp();
    float PoolTemp = getPoolTemp();
    float CompostTemp = getCompostTemp();
    float CompostHumidity = getComposthumid();
    int BatteryLevel = getBatterylevel();

    String trametxdata = "TX;batterylevel:" + String(BatteryLevel) +";watertemp:" + String(waterTemp) + ";rate:" + String(debit) + ";composttemp:" + String(CompostTemp) + ";composthumid:" + String(CompostHumidity)+";$TX" ;
  
    return trametxdata;

}

//Construction des données pour mesures
String mesure() {
    float waterTemp = getWaterTemp();
    float PoolTemp = getPoolTemp();
    float CompostTemp = getCompostTemp();
    float CompostHumidity = getComposthumid();

    String mesuredata = String(microsToSeconds(micros())) + "\t" + String(waterTemp) + "\t" + String(CompostTemp) + "\t" + String(CompostHumidity);
  
    return mesuredata;

}


void compterImpulsions() {
  nbImpulsions++;  // Incrémenter le compteur d'impulsions
}


// Fonction pour convertir les microsecondes en secondes
float microsToSeconds(unsigned long microsValue) {
  return microsValue / 1000000.0;
}

void getPotValue() {
    potValue = analogRead(potPin);
    float commandepot = 10 + (potValue / 1023.0) * (40 - 10);
    
}


void changecommandpotmode() {
    if (commandpoton == false) {
        command = 1;
        commandpoton = true;
        
    } else {
        command = 0;
        commandpoton = false;

    }


}

String getCommandState() {
    String message = "";

    if (command == 1) {
        message = "CMD:APP  TEMP=" + String(newtemperature);
    } else if (commandpoton == true) {
        message = "CMD:UC TEMP=" + String(newtemperature) + "º";
    } else if (command == 0 && commandpoton == false) {
        message = "CMD:OFF TEMP=OFF";
    }

    return message;
}
String getTemperatureData(){

    String message = "EAU="+  String(getWaterTemp()) + " CMP="+ String(getCompostTemp());
    return message;
}

void UpdateScreen() {
    
    //afficher les données de temperatures
    lcd.setCursor(0, 0);
    lcd.print(getTemperatureData());

    //afficher les données sur la commande
    lcd.setCursor(0, 1);
    lcd.print(getCommandState());

}

void BluetoothUpdate(){
    Serial1.print(trametx());
    receiveDataFromApp();
}

void connectToWiFi() {
  Serial.print("Connexion au réseau Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    WiFi.begin(ssid, pass);
    delay(2000);
  }
  Serial.println("\nConnecté !");
  Serial.print("Adresse IP : ");
  Serial.println(WiFi.localIP());
}

// Envoi des données JSON au serveur
void sendData() {
  // Reconnexion si le Wi-Fi est perdu
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Wi-Fi perdu. Tentative de reconnexion...");
    connectToWiFi();
  }


    float waterTemp = getWaterTemp();
    float CompostTemp = getCompostTemp();
    float CompostHumidity = getComposthumid();
    int BatteryLevel = getBatterylevel();

  // Construction du JSON
  String postData = "{";
  postData += "\"watertemp\":" + String(waterTemp, 1) + ",";
  postData += "\"rate\":" + String(debit, 1) + ",";
  postData += "\"composttemp\":" + String(CompostTemp, 1) + ",";
  postData += "\"composthumid\":" + String(CompostHumidity, 1) + ",";
  postData += "\"batterylevel\":" + String(BatteryLevel, 2);
  postData += "}";

  // Connexion au serveur
  if (client.connect(server, port)) {
    Serial.println("Connecté au serveur");

    // Envoi de la requête POST
    client.println("POST /api/update_data HTTP/1.1");
    client.print("Host: ");
    client.println(server);
    client.println("Content-Type: application/json");
    client.print("Content-Length: ");
    client.println(postData.length());
    client.println("Connection: close");
    client.println();
    client.println(postData);

    // Lecture de la réponse
    while (client.connected()) {
      while (client.available()) {
        String line = client.readStringUntil('\n');
        Serial.println(line);
      }
    }

    client.stop();
    Serial.println("Déconnecté du serveur");
  } else {
    Serial.println("Erreur de connexion au serveur");
  }
}


//initialisation de la carte
void setup() {
    Serial.begin(9600);
    Serial1.begin(9600); //communication bluethoot
    watersensor.begin();
    temphumid.begin();

    //definition des pins shield moteur
    pinMode(directionPin, OUTPUT);
    pinMode(pwmPin, OUTPUT);
    pinMode(brakePin, OUTPUT);

    setupBlueToothConnection();
    testBluetoothConnection();

    lcd.begin(16, 2);
    lcd.setRGB(colorR, colorG, colorB);

    pinMode(pinDebit, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(pinDebit), compterImpulsions, RISING);

    pinMode(7, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(7), changecommandpotmode, RISING);  

    //connectToWiFi(); 

}


void loop() {

    

    Serial.println(trametx()); //envoie de la trame sur le serie usb
    BluetoothUpdate(); //envoie et reception des données bt
    getPotValue(); //recuperer la valeure du potentiometre
    UpdateScreen(); //mettre a jour l'ecran
    
    

    unsigned long currentTime = millis();
    if (currentTime - lastTime >= 1000) {
      float frequence = nbImpulsions / ((currentTime - lastTime) / 1000.0);
  
      // Calcul du débit en L/min
      debit = frequence / 11.0;  // Conversion de la fréquence en débit (L/min)
      
      
      // Réinitialiser le compteur d'impulsions et mettre à jour le dernier temps
      nbImpulsions = 0;
      lastTime = currentTime;
    }

    float tempEau = getWaterTemp();
   
    if (command == 1 && commandpoton == true) {
        commandpoton = false; // On force commandpoton à false si les deux conditions sont vraies
    }

    // Si command vaut 1, on utilise newtemperature
    if (command == 1) {
        if (tempEau <= newtemperature) {
            if (!pompeEnMarche) {
                pompeEnMarche = true;
                startPump();
            }
        } else {
            if (pompeEnMarche) {
                pompeEnMarche = false;
                stopPump();
            }
        }
    }
    // Sinon, si commandpoton est actif, on utilise commandepot
    else if (commandpoton == true) {
        if (tempEau <= commandepot) {
            if (!pompeEnMarche) {
                pompeEnMarche = true;
                startPump();
            }
        } else {
            if (pompeEnMarche) {
                pompeEnMarche = false;
                stopPump();
            }
        }
    }


        
        
        

}
