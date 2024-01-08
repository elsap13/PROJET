// Bibliothèques utilisées
#include "Arduino.h"
#include "heltec.h"
#include "PubSubClient.h"
#include "WiFi.h"
#include "esp_wpa2.h"
#include <Adafruit_BME280.h> //Spécifique au capteur BME280
#include "BLEDevice.h" // Spécifique au BLE
#include "BLEServer.h" // Spécifique au BLE
#include "DFRobot_CCS811.h" //Spécifique au capteur CCS811V1

#define SCL 33   // Branche SCL des capteur BME280 / CCS811
#define SDA 34  // BrancHE SDA des capteurs BME280 / CCS811

const int soundSensorPin = 1; // GPIO 1 de la carte ESP32 associé au capteur GT1146

//Déclaration des capteurs 
Adafruit_BME280 bme;
DFRobot_CCS811 CCS811(&Wire1,0x5A);
unsigned long delayTime;


// Paramètres MQTT Broker 
const char *mqtt_broker = "192.168.1.15"; // Identifiant du broker (Adresse IP)
const char *topic1 = "ESP32/BME280/TEMP"; // Nom du topic sur lequel les données seront envoyés. 
const char *topic2 = "ESP32/BME280/HUM";
const char *topic3 = "ESP32/BME280/PRE";
const char *topic4 = "ESP32/CCS811/CO2";
const char *topic5 = "ESP32/CCS811/TVOC";
const char *topic6 = "ESP32/GT1146/SON";
const char *mqtt_username = "elsa.puvenel@orange.fr"; // Identifiant dans le cas d'une liaison sécurisée
const char *mqtt_password = "Aslemarseille13."; // Mdp dans le cas d'une liaison sécurisée

// Identifiants numériques au sein de l'organisation
#define EAP_IDENTITY "elsa.puvenel@univ-amu.fr"
#define EAP_PASSWORD "Aslemarseille13." //mot de passe EDUROAM
#define EAP_USERNAME "elsa.puvenel@univ-amu.fr" 
const char* ssid = "eduroam"; 

const int mqtt_port = 1883; // Port : 1883 dans le cas d'une liaison non sécurisée et 8883 dans le cas d'une liaison sécurisée

WiFiClient espClient; 
PubSubClient client(espClient); 
#define CONNECTION_TIMEOUT 10
int timeout_counter = 0;


// Les UUID ont été générés via le site : https://www.uuidgenerator.net/
#define SERVICE_UUID "18bce71b-9719-469a-8619-cf49f7441963"

#define CHARACTERISTIC_UUID_TEMP "b7d02667-fa62-4074-ad27-7e8d37eaa756"
#define CHARACTERISTIC_UUID_HUM "cf84a9f4-969f-4851-8b0f-915af00c92c8"
#define CHARACTERISTIC_UUID_PRE "b046cfa3-1fb4-4606-9e4d-c0b06182c3b9"
#define CHARACTERISTIC_UUID_DIOX "9d4abf96-3245-4235-a3c0-24c0dbaa4814"
#define CHARACTERISTIC_UUID_VOLA "fac9a118-1561-4a50-8e3d-d48962b8f6c2"
#define CHARACTERISTIC_UUID_SON "b129e52e-3617-4a8b-827b-ef4f20d34553"

#define DESCRIPTOR_UUID_TEMP "1e5bca92-d753-4295-8dc4-cb47ee2b8e0e"
#define DESCRIPTOR_UUID_HUM "8464e8c2-9ee1-46ef-a430-113028b7e49d"
#define DESCRIPTOR_UUID_PRE "72fb39c1-de96-4c80-a75e-357e192a6374"
#define DESCRIPTOR_UUID_DIOX "a8157bf6-5b98-4127-a4d8-c816a07c37ab"
#define DESCRIPTOR_UUID_VOLA "37c6fcef-ae89-45ed-928a-afcbb990bc89"
#define DESCRIPTOR_UUID_SON "277c8094-8b43-4c16-b8db-790fe30a09f1"

// Attribution des UUID à la caractéristique et à la description.
BLECharacteristic MyCharacteristic_TEMP(CHARACTERISTIC_UUID_TEMP,
                  BLECharacteristic::PROPERTY_READ);
BLEDescriptor MyDescriptor_TEMP(DESCRIPTOR_UUID_TEMP);

BLECharacteristic MyCharacteristic_HUM(CHARACTERISTIC_UUID_HUM,
                  BLECharacteristic::PROPERTY_READ);
BLEDescriptor MyDescriptor_HUM(DESCRIPTOR_UUID_HUM);

BLECharacteristic MyCharacteristic_PRE(CHARACTERISTIC_UUID_PRE,
                  BLECharacteristic::PROPERTY_READ);
BLEDescriptor MyDescriptor_PRE(DESCRIPTOR_UUID_PRE);

BLECharacteristic MyCharacteristic_DIOX(CHARACTERISTIC_UUID_DIOX,
                  BLECharacteristic::PROPERTY_READ);
BLEDescriptor MyDescriptor_DIOX(DESCRIPTOR_UUID_DIOX);

BLECharacteristic MyCharacteristic_VOLA(CHARACTERISTIC_UUID_VOLA,
                  BLECharacteristic::PROPERTY_READ);
BLEDescriptor MyDescriptor_VOLA(DESCRIPTOR_UUID_VOLA);

BLECharacteristic MyCharacteristic_SON(CHARACTERISTIC_UUID_SON,
                  BLECharacteristic::PROPERTY_READ);
BLEDescriptor MyDescriptor_SON(DESCRIPTOR_UUID_SON);



/*
Affichage sur le moniteur série de l'état de la connexion entre le serveur BLE (ESP32) et le client (RPI3B).
Lorsque le serveur est connecté au client, vous verrez s'afficher sur le moniteur série "BLE connecté !". 
*/ 

class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *MyServer)
  {
    Serial.println("BLE connecté ! ");
  }
  void onDisconnect(BLEServer *MyServer)
  {
    Serial.println("BLE Déconnecté ! ");
  }
};


// Déclaration des variables nécessaires à la mesure de la température, de l'humidité, de la pression , du dioxyde de carbone, des composés organiques volatils totaux et du son
float temp = 0;
float hum = 0;
float pre = 0; 
float diox = 0;
float vola = 0;
float son = 0;

// La variable dans laquelle on stockera les données s'appellera "valeur_affichage_DONNES"
// Les valeurs seront stockées dans des tableaux de 5 cases
char valeur_affichage_TEMP[5] ;  
char valeur_affichage_HUM[5] ;
char valeur_affichage_PRE[5] ;
char valeur_affichage_DIOX[5] ;
char valeur_affichage_VOLA[5] ;
char valeur_affichage_SON[5] ;

// Fonction réception du message MQTT 
void callback(char *topic, byte *payload, unsigned int length) { 
  Serial.print("Le message a été envoyé sur le topic : "); 
  Serial.println(topic); 
  Serial.print("Message:"); 
  for (int i = 0; i < length; i++) { 
    Serial.print((char) payload[i]); 
  } 
  Serial.println(); 
  Serial.println("-----------------------"); 
}

void get_network_info(){
    if(WiFi.status() == WL_CONNECTED) {
        Serial.print("[*] Informations - SSID : ");
        Serial.println(ssid);
    }
}





void setup() { 
  Serial.begin(115200); 
  pinMode(soundSensorPin, INPUT); //configurer le pin du capteur GT1146 pour fonctionner en tant qu'entrée
  // Initialisation du capteur 
  while(!Serial);    // time to get serial running
  Serial.println(F("BME280 test"));
  Wire1.begin(SDA,SCL);
  unsigned status;
    
  // default settings
  status = bme.begin(0x76,&Wire1) ; 


   if (!status) {
        Serial.println("Could not find a valid BME280 sensor, check wiring, address, sensor ID!");
        Serial.print("SensorID was: 0x"); Serial.println(bme.sensorID(),16);
        Serial.print("        ID of 0xFF probably means a bad address, a BMP 180 or BMP 085\n");
        Serial.print("   ID of 0x56-0x58 represents a BMP 280,\n");
        Serial.print("        ID of 0x60 represents a BME 280.\n");
        Serial.print("        ID of 0x61 represents a BME 680.\n");
        while (1) delay(10);
    }
    
    Serial.println("-- Default Test --");
    delayTime = 1000;

    Serial.println();

   CCS811.begin();

       while(CCS811.begin() != 0){
        Serial.println("failed to init chip, please check if the chip connection is fine");
        delay(1000);
    }

// Connexion au réseau EDUROAM

  WiFi.disconnect(true);
  WiFi.begin(ssid, WPA2_AUTH_PEAP, EAP_IDENTITY, EAP_USERNAME, EAP_PASSWORD); 
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(F("."));
  }

  Serial.println("");
  Serial.println(F("L'ESP32 est connecté au WiFi !"));
  

// Connection au BLE
  // Initialisation de la chaîne de mesure : MYESP32BLE-NOMDEFAMILLE
  BLEDevice::init("MYESP32BLE-PUVENELMERCIER");

  // Creation du serveur BLE : MyServer
  BLEServer *Myserver = BLEDevice::createServer();
  Myserver->setCallbacks(new MyServerCallbacks());

  // Creation du service 
  BLEService *Myservice = Myserver->createService(SERVICE_UUID);

  // Association de la caractéristique au service et de la description à la caractéristique
  Myservice->addCharacteristic(&MyCharacteristic_TEMP);
  MyCharacteristic_TEMP.addDescriptor(&MyDescriptor_TEMP);
  MyDescriptor_TEMP.setValue(" Température : ");

  Myservice->addCharacteristic(&MyCharacteristic_HUM);
  MyCharacteristic_HUM.addDescriptor(&MyDescriptor_HUM);
  MyDescriptor_HUM.setValue(" Humidité : ");

  Myservice->addCharacteristic(&MyCharacteristic_PRE);
  MyCharacteristic_PRE.addDescriptor(&MyDescriptor_PRE);
  MyDescriptor_PRE.setValue(" Pression : ");

  Myservice->addCharacteristic(&MyCharacteristic_DIOX);
  MyCharacteristic_DIOX.addDescriptor(&MyDescriptor_DIOX);
  MyDescriptor_DIOX.setValue(" CO2 : ");

  Myservice->addCharacteristic(&MyCharacteristic_VOLA);
  MyCharacteristic_VOLA.addDescriptor(&MyDescriptor_VOLA);
  MyDescriptor_VOLA.setValue(" TVOC : ");

  Myservice->addCharacteristic(&MyCharacteristic_SON);
  MyCharacteristic_SON.addDescriptor(&MyDescriptor_SON);
  MyDescriptor_SON.setValue(" Son : ");



 // Démarrage du service
  Serial.println("Lancement du service");
  Myservice->start();

  // Démarrage de la communication. Avant cette ligne, la chaîne de mesure est invisible.
  Myserver->getAdvertising()->start(); // La chaîne de mesure est visible lorsque l'on scanne les périphériques BLE.
  Serial.println("Lancement du serveur");

// Connexion au broker MQTT  
 client.setServer(mqtt_broker, mqtt_port); 
  client.setCallback(callback); 

  while (!client.connected()) { 
    String client_id = "esp32-client-"; 
    client_id += String(WiFi.macAddress()); 
    Serial.printf("The client %s connects to the public MQTT brokern", client_id.c_str()); 
 
    if (client.connect(client_id.c_str(), mqtt_username, mqtt_password)) { 
      Serial.println("La chaîne de mesure est connectée au broker."); 
    } else { 
      Serial.print("La chaîne de mesure n'a pas réussi à se connecter ... "); 
      Serial.print(client.state()); 
      delay(2000); 
    } 
  } 
} 

void loop(){

  // Lecture de la température, de l'humidité , de la pression , du dioxyde de carbone, des composés organiques volatils totaux et du son
   temp = bme.readTemperature();
   hum = bme.readHumidity();
   pre = bme.readPressure() / 100.0F;
   diox = CCS811.getCO2PPM();
   vola = CCS811.getTVOCPPB();
   int sensorValue = analogRead(soundSensorPin);
   float voltage = sensorValue * (3.3 / 4095.0);
   float tension = 1/voltage; 
   float son = 20*log(tension / 0.03); //0.03 correspond à la sensibilité du capteur en mV

  // Publication sur le topic
  client.publish(topic1, String(temp).c_str());
  client.publish(topic2, String(hum).c_str());
  client.publish(topic3, String(pre).c_str());
  client.publish(topic4, String(diox).c_str());
  client.publish(topic5, String(vola).c_str());
  client.publish(topic6, String(son).c_str());

  //  client.subscribe(topic);
  client.loop(); // permet de lire les messages publiés sur le topic
  delay(5000);

  // Conversion de la valeur de la température, de l'humidité , de la pression , du dioxyde de carbone , des composés orgganiques volatils totaux et du son en chaîne de caractères. 
  dtostrf(temp,1,2,valeur_affichage_TEMP);
  dtostrf(hum,1,0,valeur_affichage_HUM);
  dtostrf(pre,1,2,valeur_affichage_PRE);
  dtostrf(diox,1,0,valeur_affichage_DIOX);
  dtostrf(vola,1,0,valeur_affichage_VOLA);
  dtostrf(son,1,2,valeur_affichage_SON);
 

  // Attribution de la chaîne de caractère à la description de la caractérisque associée.
  MyCharacteristic_TEMP.setValue(valeur_affichage_TEMP);  
  MyCharacteristic_HUM.setValue(valeur_affichage_HUM); 
  MyCharacteristic_PRE.setValue(valeur_affichage_PRE); 
  MyCharacteristic_DIOX.setValue(valeur_affichage_DIOX); 
  MyCharacteristic_VOLA.setValue(valeur_affichage_VOLA); 
  MyCharacteristic_SON.setValue(valeur_affichage_SON); 

  delay(1500);


}
