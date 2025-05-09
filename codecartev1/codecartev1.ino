//PROJET SI, RECUPERATION CHALEURE COMPOST
//CODE ARDUINO, INTERFACEE AVEC L'APPLICATION MOBILE COMPOS'HEAT



//------------------------IMPORT DES MODULES---------------------------------//
#include <Arduino.h>
#include <OneWire.h> //one wire pour capteures
#include <DallasTemperature.h> //interpreter temperature du capteur
#include <SoftwareSerial.h> //module pour gerer le bluethoot
#include <DHT.h> //module pour gerer la capteur temp air / humiditée
#include <Wire.h> //gestion de l'ecran lcd
#include "rgb_lcd.h" //gestion de l'ecran lcd
//---------------------------------------------------------------------------//


//------------------------DEFINITION DES IO----------------------------------//
#define RxBLT 6 //rx blt
#define TxBLT 7 //tx blt
#define ONE_WIRE_BUS_WATERSENSOR 8// Broche pour le DS18B20 temperature de l'eau compost
#define ONE_WIRE_BUS_POOLSENSOR 4 // Broche pour le DS18B20 temperature de temperature piscine
#define DHTPIN 5        // Broche numérique D8 où le capteur est connecté temperature et humiditée du compost
#define DHTTYPE DHT11   // Type du capteur de temp / humiditée de l'int compost
//---------------------------------------------------------------------------//



OneWire watersensorbus(ONE_WIRE_BUS_WATERSENSOR);
OneWire poolsensorbus(ONE_WIRE_BUS_POOLSENSOR);
DallasTemperature watersensor(&watersensorbus); //capteur temperature de l'eau
DallasTemperature poolsensor(&poolsensorbus); //capteur temperature de l'eau
SoftwareSerial blueToothSerial(RxBLT, TxBLT); //module bluethoot
DHT temphumid(DHTPIN, DHTTYPE); // Initialisation du capteur DHT
rgb_lcd lcd;
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
int newpumpspeed = -1;
int pumpspeed = 255;

//CONSTANTES / VARIABLES POUR DEBITMETRE
volatile unsigned int pulseCount = 0;
float flowRate = 0.0;
unsigned long lastMillis = 0;
const float pulsesPerLiter = 450.0;  // changer pour notre capteur


///////////////////////////////////////////////////////////////////////////////
/////////////////////////FONTIONNEMENT DU CODE/////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

//configuration du bluethoot
void setupBlueToothConnection() {
    blueToothSerial.print("AT");
    delay(400); 

    blueToothSerial.print("AT+DEFAULT");  // Réinitialisation du module Bluetooth
    delay(2000); 

    blueToothSerial.print("AT+NAMECOMPOS'HEAT");  // Nom du module Bluetooth
    delay(400);

    blueToothSerial.print("AT+PIN0000");  // Code PIN
    delay(400);

    blueToothSerial.print("AT+AUTH1");  // Authentification
    delay(400);    

    blueToothSerial.flush();  // Purge du buffer pour préparer la communication
}

// Fonction de test de la connexion Bluetooth
void testBluetoothConnection() {
    blueToothSerial.println("AT");  // Envoie une commande AT au module Bluetooth pour tester la connexion
    delay(500);

    if (blueToothSerial.available()) {
        String response = "";
        while (blueToothSerial.available()) {
            char c = blueToothSerial.read();
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
    
    while (blueToothSerial.available()) {
        char c = blueToothSerial.read();
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

                // Si la clé est "newpumpspeed", extraire la valeur
                if (key.equals("newpumpspeed")) {  // Utiliser equals() pour comparer les String
                    newpumpspeed = value.toInt();  // Convertir la valeur en entier
                }
            }
            
            // Supprimer la partie traitée
            extractedData = extractedData.substring(pos + 1);
        }

        // Si pumpstate est égal à 1, démarrer la pompe, sinon l'arrêter
        if (pumpstate == 1) {
            startPump();  // Démarre la pompe
        } else {
            stopPump();   // Arrête la pompe
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

    String trametxdata = "TX;batterylevel:" + String(BatteryLevel) +";watertemp:" + String(waterTemp) + ";pooltemp:" + String(PoolTemp) + ";composttemp:" + String(CompostTemp) + ";composthumid:" + String(CompostHumidity)+";$TX" ;
  
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

void countPulse() {
  pulseCount++;
}


// Fonction pour convertir les microsecondes en secondes
float microsToSeconds(unsigned long microsValue) {
  return microsValue / 1000000.0;
}

//initialisation de la carte
void setup() {
    Serial.begin(9600);
    blueToothSerial.begin(9600); //communication bluethoot
    watersensor.begin();
    temphumid.begin();

    //definition des pins module bluethoot
    pinMode(RxBLT, INPUT);
    pinMode(TxBLT, OUTPUT);

    //definition des pins shield moteur
    pinMode(directionPin, OUTPUT);
    pinMode(pwmPin, OUTPUT);
    pinMode(brakePin, OUTPUT);

    setupBlueToothConnection();
    testBluetoothConnection();

    lcd.begin(16, 2);
    lcd.setRGB(colorR, colorG, colorB);

    pinMode(2, INPUT_PULLUP);

    attachInterrupt(digitalPinToInterrupt(2), countPulse, RISING);

    

    


    

}


void loop() {

    
    //ENVOYE DE LA TRAME VIA BT ET SUR LE SERIAL
    Serial.println(trametx());
    blueToothSerial.print(trametx());
    receiveDataFromApp();
    
    
    
    //AFFICHAGE DE LA TEMPERATURE DE L'EAU SUR L'ECRAN
    String temperatureair = "temp eau = "+  String(getWaterTemp());
    lcd.setCursor(0, 0);
    lcd.print(temperatureair);

    //AFFICHAGE DE LA TEMPERATURE DU COMPOST SUR L'ECRAN
    String temperaturecomp = "temp comp = "+  String(getCompostTemp());
    lcd.setCursor(0, 1);
    lcd.print(temperaturecomp);
    

    if (millis() - lastMillis >= 1000) {  // Chaque seconde
      detachInterrupt(digitalPinToInterrupt(2));  // Empêcher perturbation pendant calcul

      // Débit en L/s
      flowRate = pulseCount / pulsesPerLiter;

      Serial.print("Débit : ");
      Serial.print(flowRate);
      Serial.println(" L/s");

      pulseCount = 0;  // Réinitialiser pour la prochaine seconde
      lastMillis = millis();

      attachInterrupt(digitalPinToInterrupt(2), countPulse, RISING);  // Réactiver l'interruption

    }

    //DELAI D'AFFICHAGE
    delay(1000);
    
    

}
