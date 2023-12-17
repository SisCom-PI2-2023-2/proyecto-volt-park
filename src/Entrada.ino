#include <SPI.h>
#include <MFRC522.h>
#include "Servo.h"
#include <ESP8266WiFi.h>  // Biblioteca para generar la conexión a internet a través de WiFi
#include <PubSubClient.h> // Biblioteca para generar la conexión MQTT con un servidor (Ej.: ThingsBoard)
#include <ArduinoJson.h>  // Biblioteca para manejar Json en Arduino

//


Servo myservo; // Inicializar objeto Servo para controlar el servomotor
int angle = 0; // Variable para almacenar el ángulo del servomotor

// Credenciales de la red WiFi
const char* ssid = "HUAWEI-IoT";
const char* password = "ORTWiFiIoT";



// Host de ThingsBoard
const char* mqtt_server = "demo.thingsboard.io";
const int mqtt_port = 1883;



// Token del dispositivo en ThingsBoard
const char* token = "it1Y2t9BcaVLXOJK3SzY";


// Objetos de conexión
WiFiClient espClient;             // Objeto de conexión WiFi
PubSubClient client(espClient);   // Objeto de conexión MQTT


// Declaración de variables para los datos a manipular
unsigned long lastMsg = 0;  // Control de tiempo de reporte
int msgPeriod = 2000;       // Actualizar los datos cada 2 segundos
boolean led_state = false;


// Mensajes y buffers
#define MSG_BUFFER_SIZE  (50)
char msg[MSG_BUFFER_SIZE];
char msg2[MSG_BUFFER_SIZE];



int servo_pin = D4;  // for ESP8266 microcontroller

int L0 = 5; // pines de la ESP8266 para salida de cantidad de autos disponibles en BCD
int L1 = 4;
int L2 = 0;
int L3 = 10;


// Objeto Json para recibir mensajes desde el servidor
DynamicJsonDocument incoming_message(256);



// Inicializar la conexión WiFi
void setup_wifi() {
  delay(10);


  WiFi.mode(WIFI_STA); // Declarar la ESP como STATION
  WiFi.begin(ssid, password);



  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }



  randomSeed(micros());


  /*
    Serial.println("");
    Serial.println("¡Conectado!");
    Serial.print("Dirección IP asignada: ");
    Serial.println(WiFi.localIP());*/
}


// Función de callback para recepción de mensajes MQTT (Tópicos a los que está suscrita la placa)
// Se llama cada vez que arriba un mensaje entrante (En este ejemplo la placa se suscribirá al tópico: v1/devices/me/rpc/request/+)
void callback(char* topic, byte* payload, unsigned int length) {

  // Log en Monitor Serie
  /*Serial.print("Mensaje recibido [");
    Serial.print(topic);
    Serial.print("]: ");*/
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();



  // En el nombre del tópico agrega un identificador del mensaje que queremos extraer para responder solicitudes
  String _topic = String(topic);



  // Detectar de qué tópico viene el "mensaje"
  if (_topic.startsWith("v1/devices/me/rpc/request/")) { // El servidor "me pide que haga algo" (RPC)
    // Obtener el número de solicitud (request number)
    String _request_id = _topic.substring(26);



    // Leer el objeto JSON (Utilizando ArduinoJson)
    deserializeJson(incoming_message, payload); // Interpretar el cuerpo del mensaje como Json
    String metodo = incoming_message["method"]; // Obtener del objeto Json, el método RPC solicitado



    // Ejecutar una acción de acuerdo al método solicitado
    if (metodo == "dejarEntrar") {
      Serial.print("dejo pasar al auto");
      myservo.write(180);
      delay(3000);
      myservo.write(0);

    } else if (metodo == "actualizar7Seg") { // Establecer el estado del led y reflejar en el atributo relacionado
      
      int params = int(incoming_message["params"]);

      Serial.print("El numero magico es:");
      Serial.println(params);
      int bcd[4];

      /*// Convierte el número entero a BCD
      for (int i = 3; i >= 0; i--) {
        bcd[i] = params % 10;
        params /= 10;
      }*/
      bcd[0]=bitRead(params,0);
      
      bcd[1]=bitRead(params,1);
      
      bcd[2]=bitRead(params,2);
      
      bcd[3]=bitRead(params,3);

      digitalWrite(L0, bcd[0]);
      digitalWrite(L1, bcd[1]);
      digitalWrite(L2, bcd[2]);
      digitalWrite(L3, bcd[3]);

      Serial.print("El L0 esta en:");
      Serial.println(bcd[0]);
      
      Serial.print("El L1 esta en:");
      Serial.println(bcd[1]);
      
      Serial.print("El L2 esta en:");
      Serial.println(bcd[2]);
      
      Serial.print("El L3 esta en:");
      Serial.println(bcd[3]);
    }
  }
}



// Establecer y mantener la conexión con el servidor MQTT (En este caso de ThingsBoard)
void reconnect() {
  // Bucle hasta lograr la conexión
  while (!client.connected()) {
    Serial.print("Intentando conectar MQTT...");
    if (client.connect("ESP8266", token, token)) {  //Nombre del Device y Token para conectarse
      Serial.println("¡Conectado!");

      // Una vez conectado, suscribirse al tópico para recibir solicitudes RPC
      client.subscribe("v1/devices/me/rpc/request/+");

    } else {

      Serial.print("Error, rc = ");
      Serial.print(client.state());
      Serial.println("Reintenar en 5 segundos...");
      // Esperar 5 segundos antes de reintentar
      delay(5000);

    }
  }
}




#define SS_PIN D8
#define RST_PIN D0

MFRC522 rfid(SS_PIN, RST_PIN); // Instance of the class

MFRC522::MIFARE_Key key;

// Init array that will store new NUID
byte nuidPICC[4];

void setup() {
  myservo.attach(servo_pin);
  Serial.begin(115200);
  SPI.begin(); // Init SPI bus
  rfid.PCD_Init(); // Init MFRC522
  Serial.println();
  Serial.print(F("Reader :"));
  rfid.PCD_DumpVersionToSerial();

  setup_wifi();                           // Establecer la conexión WiFi
  client.setServer(mqtt_server, mqtt_port);// Establecer los datos para la conexión MQTT
  client.setCallback(callback);           // Establecer la función del callback para la llegada de mensajes en tópicos suscriptos

  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }
  Serial.println();
  Serial.println(F("This code scan the MIFARE Classic NUID."));
  Serial.print(F("Using the following key:"));
  printHex(key.keyByte, MFRC522::MF_KEY_SIZE);

  pinMode(L0, OUTPUT);
  pinMode(L1, OUTPUT);
  pinMode(L2, OUTPUT);
  pinMode(L3, OUTPUT);
}

void loop() {



  // === Conexión e intercambio de mensajes MQTT ===
  if (!client.connected()) {  // Controlar en cada ciclo la conexión con el servidor
    reconnect();              // Y recuperarla en caso de desconexión
  }

  client.loop();              // Controlar si hay mensajes entrantes o para enviar al servidor

  // Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle.
  if ( ! rfid.PICC_IsNewCardPresent()) {
    myservo.write(0);
    return;
  }
  // Verify if the NUID has been readed
  if ( ! rfid.PICC_ReadCardSerial())
    return;

  Serial.print(F("PICC type: "));
  MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
  Serial.println(rfid.PICC_GetTypeName(piccType));

  // Check is the PICC of Classic MIFARE type
  if (piccType != MFRC522::PICC_TYPE_MIFARE_MINI &&
      piccType != MFRC522::PICC_TYPE_MIFARE_1K &&
      piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
    Serial.println(F("Your tag is not of type MIFARE Classic."));
    return;
  }

  if (rfid.uid.uidByte[0] != nuidPICC[0] ||
      rfid.uid.uidByte[1] != nuidPICC[1] ||
      rfid.uid.uidByte[2] != nuidPICC[2] ||
      rfid.uid.uidByte[3] != nuidPICC[3] ) {
    Serial.println(F("A new card has been detected."));

    // Store NUID into nuidPICC array
    for (byte i = 0; i < 4; i++) {
      nuidPICC[i] = rfid.uid.uidByte[i];
    }

    Serial.println(F("The NUID tag is:"));
    Serial.print(F("In hex: "));
    printHex(rfid.uid.uidByte, rfid.uid.size);
    Serial.println();
    Serial.print(F("In dec: "));
    printDec(rfid.uid.uidByte, rfid.uid.size);
    Serial.println();
    String aux = String(rfid.uid.uidByte[0]) + " " + String(rfid.uid.uidByte[1]) + " " + String(rfid.uid.uidByte[2]) + " " + String(rfid.uid.uidByte[3]);
    DynamicJsonDocument resp(256);
    resp["Tarjeta_Rfid"] = aux;
    //Agrega el dato al Json
    char buffer[256];
    serializeJson(resp, buffer);
    client.publish("v1/devices/me/telemetry", buffer);  // Publica el mensaje de telemetría

  }
  else Serial.println(F("Card read previously."));

  // Halt PICC
  rfid.PICC_HaltA();

  // Stop encryption on PCD
  rfid.PCD_StopCrypto1();

  // Store NUID into nuidPICC array
  for (byte i = 0; i < 4; i++) {
    nuidPICC[i] = rfid.uid.uidByte[i];
  }
}


/**
   Helper routine to dump a byte array as hex values to Serial.
*/
void printHex(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);
  }
}

/**
   Helper routine to dump a byte array as dec values to Serial.
*/
void printDec(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], DEC);
  }
}
