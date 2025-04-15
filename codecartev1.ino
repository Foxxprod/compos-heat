//PROJET SI, RECUPERATION CHALEURE COMPOST
//CODE ARDUINO, INTERFACEE AVEC L'APPLICATION MOBILE COMPOS'HEAT



//------------------------IMPORT DES MODULES---------------------------------//
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
#define ONE_WIRE_BUS_WATERSENSOR 2 // Broche pour le DS18B20 temperature de l'eau compost
#define ONE_WIRE_BUS_POOLSENSOR 4 // Broche pour le DS18B20 temperature de temperature piscine
#define DHTPIN 5        // Broche numérique D8 où le capteur est connecté temperature et humiditée du compost
#define DHTTYPE DHT11   // Type du capteur de temp / humiditée de l'int compost
//---------------------------------------------------------------------------//



//------------------------DECLARATION DES OBJET------------------------------//
OneWire watersensorbus(ONE_WIRE_BUS_WATERSENSOR);
OneWire poolsensorbus(ONE_WIRE_BUS_POOLSENSOR);
DallasTemperature watersensor(&watersensorbus); //capteur temperature de l'eau
DallasTemperature poolsensor(&poolsensorbus); //capteur temperature de l'eau
SoftwareSerial blueToothSerial(RxBLT, TxBLT); //module bluethoot
DHT temphumid(DHTPIN, DHTTYPE); // Initialisation du capteur DHT
rgb_lcd lcd;
//---------------------------------------------------------------------------//

const int colorR = 150;
const int colorG = 0;
const int colorB = 150;

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


//Construction de la trame a envoyer a l'application
String trametx() {
    float waterTemp = getWaterTemp();
    float PoolTemp = getPoolTemp();
    float CompostTemp = getCompostTemp();
    float CompostHumidity = getComposthumid();
    int BatteryLevel = getBatterylevel();

    String trametxdata = "batterylevel:" + String(BatteryLevel) +";watertemp:" + String(waterTemp) + ";pooltemp:" + String(PoolTemp) + ";composttemp:" + String(CompostTemp) + ";composthumid:" + String(CompostHumidity)+";";
  
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

    pinMode(RxBLT, INPUT);
    pinMode(TxBLT, OUTPUT);

    setupBlueToothConnection();
    testBluetoothConnection();

    lcd.begin(16, 2);
    lcd.setRGB(colorR, colorG, colorB);


    

}


void loop() {

    Serial.println(mesure());
    
    String temperature = "temp eau = "+  String(getWaterTemp());
    lcd.print(temperature);
    
    delay(5000);
    

}
