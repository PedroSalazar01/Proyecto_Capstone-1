//Librerias Necesarias
#include <ESP8266WiFi.h>
#include <PubSubClient.h>//libreria para publicar en el Broker de mosquitto
#include <OneWire.h>//Libreria para la comunicación del sensor de temperatura con resistencia pull - up
#include <DallasTemperature.h>//Libreria para la comunicación del sensor de temperatura

//Asociación de pines a cada sensor y actiador
const byte intNivel = 2; //el pin 2 digital, recibe la lectura del sensor de nivel
volatile int b=0;
const int pH=A0; //el pin analogico A0 recibe la lectura del sensor de pH
//const int cala=A1; //el pin analogico A1 recibe la lectura del sensor de calidad de agua
OneWire onewire(D3); //Se declara D3 como Pin para la comunicación onewire
DallasTemperature sensors (&onewire); //indica que ese pin va a trabajar con la librería Dallastemp y recibe la lectura del sensor de temperatura

//Variables de activación del interruptor
volatile boolean isrNivelCentinela = false; //Detecta interrupción
volatile boolean isrNivelCentinelaOff =false; //Para la bomba
unsigned long previousMillis=0; //Almacena el ultimo momento en el que se obtuvieron 
unsigned long currentMillis; //Almacena el tiempo transcurrido desde que se encendi la placa
const long timer=5000; //Es el tiempo maximo de espera para obtener un dato
float ph=0; //Establece el valor de pH en 0
char phVal[4]; //Almacena el valor obtenido del sensor como un arreglo de caracteres
unsigned long previousMillisTemp=0; //Almacena el ultimo momento en que se recibieron datos 
unsigned long currentMillisTemp; //Mide el tiempro transcurrido desde que se encendio la tarjeta
const long timerTemp=2500;
float tempValue=0; //Almacena el valor que envia sensor de temperatura
char tempBuf[5];//Se utiliza para almacenar el valor de la temperatuea en un arreglo de caracteres

//configuración de la red inalambrica

char* ssid = "Diplomado IoT";//Nombre de la red WiFi
char* pass = "ITICsITSOEH@";//contraseña de la red WiFi
IPAddress ip(172,10,20,123);     
IPAddress gateway(172,10,20,3);   
IPAddress subnet(255,255,0,0);  
WiFiClient clientewifi;

//configuracón MQTT
PubSubClient clientMqtt(clientewifi);
const char* servidorMqtt = "200.79.180.228";//Determina la dirección del broker
const char* topicNivel = "/estanque/trucha/nivel"; //Topico al que va a publicar el sensor de nivel
const char* topicPote = "/estanque/trucha/pote"; //Topico al que va a publicar el sentos de pH
const char* topicTemp = "/estanque/trucha/temp"; //Topico al que va a publuca el sensor de temperatura
const char* topicCal= "/estanque/trucha/Cal"; //Topico al que va a publicar el sensor de calidad de agua

void setup()
{
  Serial.begin(115200);//Comunicación seria establecida a 115200 baudios 
  delay(500); //Hace una pausa de 1/2 segundo

  //Conexión con la red WiFi
  int intentosWiFi = 0; //Variable que almacena el numero de intentos de conexion a WiFi (se detendra despues de 30 intentos
  WiFi.begin(ssid, pass); //Lanza la conexión a la red wifi seleccionada (ssid) 
  Serial.print("[WiFi]Intentando conectar a: ");//Imprime el mensaje [WiFi]Intentando conectar a: 
  Serial.print(ssid); //Nombre de la red WiFi
  while (WiFi.status() != WL_CONNECTED || intentosWiFi > 30)//Mientras WiFi.status devuelva un valor diferente a WL_CONNECTED o menor a 30 intentos imprime un punto en pantalla
  {
   Serial.print("."); //imprime un punto por cada intento de conexión fallido
   delay(500); // Hace una pausa de 1/2 segundo
   intentosWiFi++; //incrementa el numero de intentos en 1
  }
  Serial.println(".");

  //Si no ha conectado se muestra error
  if (WiFi.status() != WL_CONNECTED)
  {
    while (1){
      digitalWrite(LED_BUILTIN, HIGH);
      delay(500);
      digitalWrite(LED_BUILTIN, LOW);
      delay(500);
    }
  }
  Serial.println("[WiFi]Conectado a WiFi");

  //Configuración MQTT
  clientMqtt.setServer(servidorMqtt, 1883); //Conecta al servidor MQTT establecidoe en la dirección IP (servidor MQTT) y el puerto 1883
  Serial.println("[MQTT] Conectado al servidor MQTT"); //Imprime el resultado de la conexión

  //Configuración de interrupciones
  pinMode(intNivel, INPUT_PULLUP);//Interrupción magnetica
  attachInterrupt(intNivel, isrNivel, CHANGE);//Causa una interrupción cuando intNivel cambia de estado

  digitalWrite(LED_BUILTIN, HIGH); //Apagando el LED de prueba
  sensors.begin(); //Inicia comunicación con el sensor de temperatura
}

void loop()
{
 
 if (!clientMqtt.connected())//Si se interrumpe la conexión 
 {
  reconnectMqtt(); //Inicia una reconexión
 } 
 
 clientMqtt.loop(); //Comprueba el estado de la conexión de la red una vez cada 10 segundos
 currentMillis=millis(); //Almacena el tiempo transcurrido desde que se encendio la tarjeta
 
 if(currentMillis-previousMillis>=timer)//Hace una resta entre el tiempo transcurrido desde que se encendio la tarjeta menos el tiempo de la ultima toma de datos, si el tiempo transcurrido desde que se encendo la tarjeta es menor a 5 segundos, inicia la comunicación con el sensor
 {
  previousMillis=currentMillis; //Almacena el tiempo de la ultima toma de datos
  ph=fmap(analogRead(A0),0,1023,0.0,14.0); //fmap permite realizar una ponderación del valor medido en analogRead(A0), que va de 0-1023 hasta 0-14, obteniendo a la salida el valor correspondiente en pH, devuelve un valor entre 0 y 14
  dtostrf(ph,2,1,phVal); // dtostrf modifica el tipo de variable de ph, de un valor float a cadena de caracteres
  clientMqtt.publish(topicPote,phVal); //publica el valor de phVal en el tema /casa/pecera/pote
  Serial.print("pH="); //Muestra en pantalla el texto pH
  Serial.println(ph);//Muestra en pantalla el valor del pH
 }

  currentMillis=millis(); //Almacena el tiempo transcurrido desde que se encendio la tarjeta
 
 /*if(currentMillis-previousMillis>=timer)//Hace una resta entre el tiempo transcurrido desde que se encendio la tarjeta menos el tiempo de la ultima toma de datos, si el tiempo transcurrido desde que se encendo la tarjeta es menor a 5 segundos, inicia la comunicación con el sensor
 {
  previousMillis=currentMillis; //Almacena el tiempo de la ultima toma de datos
  cal=fmap(analogRead(A1),0,1023,0.0,14.0); //fmap permite realizar una ponderación del valor medido en analogRead(A0), que va de 0-1023 hasta 0-14, obteniendo a la salida el valor correspondiente en pH, devuelve un valor entre 0 y 14
  dtostrf(cal,2,1,CalVal); // dtostrf modifica el tipo de variable de ph, de un valor float a cadena de caracteres
  clientMqtt.publish(topicCal,CalVal); //publica el valor de phVal en el tema /casa/pecera/pote
  Serial.print("Calidad de agua="); //Muestra en pantalla el texto pH
  Serial.println(cal);//Muestra en pantalla el valor del pH
 }

 currentMillisTemp=millis(); //Mide nuevamente el tiempro transcurrido desde que se encendio la tarjeta*/
 
 if(currentMillisTemp-previousMillisTemp>=timerTemp)//Hace una resta entre el tiempo transcurrido desde que se encendio la tarjeta menos el tiempo de la ultima toma de datos, si el tiempo transcurrido desde que se encendo la tarjeta es menor a 5 segundos, inicia la comunicación con el sensor
  {
    previousMillisTemp=currentMillisTemp; //Almacena el tiempo de la ultima toma de datos 
    sensors.requestTemperatures();//Solicita la temperatura al sensor
    tempValue=sensors.getTempCByIndex(0);//Guarda la temperatura en 
    dtostrf(tempValue,2,1,tempBuf); //dtostrf modifica el tipo de variable de temperarura, de un valor float a cadena de caracteres
    clientMqtt.publish(topicTemp,tempBuf); //Publica el valor de tempBuf en el tema /casa/pecera/temp
    Serial.print("Temp="); //Muestra en pantalla el texto Temp
    Serial.println(tempBuf); //Muestra en oantalla el valor de la variable tempBuf
  }

  // Si detecta interrupción nivel
  if (isrNivelCentinela) 
   {
     isrNivelCentinela = false;
     b=1;
     clientMqtt.publish(topicNivel, "1");
     Serial.println("1");
   }
  if(isrNivelCentinelaOff)
   { //Si se "suelta" el pulsador
     isrNivelCentinelaOff=false;
     b=0;
     clientMqtt.publish(topicNivel,"0");
     Serial.println("0");
   }
}
ICACHE_RAM_ATTR void isrNivel() 
{
 // Marca ha ejecutado interrupción
 b==0?isrNivelCentinela=true:isrNivelCentinelaOff=true;
}
float fmap(float x, float in_min, float in_max, float out_min, float out_max)
{
 return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void reconnectMqtt() {
 // Repetimos hasta conectar
 while (!clientMqtt.connected()) 
  {
    Serial.println("[MQTT]Esperando conexión con MQTT...");
    // Intentamos conectar
    if (clientMqtt.connect("")) {
    Serial.println("[MQTT]Conectado");
    } 
    else 
    {
      Serial.print("[MQTT]Fallo, rc=");
      Serial.print(clientMqtt.state());
      Serial.println(" se intentará o travez tras 5 segundos");
      // Esperamos 5 segundos
      delay(5000);
    }
  }
}
