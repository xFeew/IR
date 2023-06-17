#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <SPI.h>
#include <Adafruit_CCS811.h>

// Dane do połączenia z siecią Wi-fi
const char* ssid = "Wifi dom";
const char* password = "warszawa";

// MQTT broker login/hasło
const char* MQTT_username = NULL; 
const char* MQTT_password = NULL; 

// Adres IP server'a MQTT
const char* mqtt_server = "szymonlach.ddns.net";

//Kontrola LED
#define LED_PIN 14 //d1
//Przycisk
#define BUTTON_PIN 12 //d2
int lastButtonState;

//CJMCU-811
Adafruit_CCS811 ccs;
//CCS811 ccs;


WiFiClient espClient;
PubSubClient client(espClient);

long now = millis();
long lastMeasure = 0;
unsigned long previousMillis = 0; 

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Laczenie z siecia ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Polaczono!, adres IP urzadzenia: ");
  Serial.println(WiFi.localIP());
}

void callback(String topic, byte* message, unsigned int length) {
  Serial.print("Uzyskano odpowiedz na temat ");
  Serial.print(topic);
  Serial.print(". Wiadomosc: ");
  String messageTemp;
  
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();



  if(topic=="led"){
      if(messageTemp == "on"){
        digitalWrite(LED_PIN, HIGH);   

      }
      else if(messageTemp == "off"){
        digitalWrite(LED_PIN, LOW);   

      }
  }


  Serial.println();
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP8266_2", MQTT_username, MQTT_password)) {
      Serial.println("connected");  
      client.subscribe("led");
     // client.subscribe("przycisk"); //nie subskrybuj bo wysyla wiadomosc
      client.subscribe("potencjometr");
      client.subscribe("czujnik_1");
      client.subscribe("czujnik_2");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

// The setup function sets your ESP GPIOs to Outputs, starts the serial communication at a baud rate of 115200
// Sets your mqtt broker and sets the callback function
// The callback function is what receives messages and actually controls the LEDs
void setup() {
  Wire.begin(); //I2C
  Serial.begin(115200);
  //LED
  pinMode(LED_PIN, OUTPUT);
  //PRZYCISK
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  lastButtonState = digitalRead(BUTTON_PIN);

  setup_wifi();
  client.setServer(mqtt_server, 9004);
  client.setCallback(callback);
  //CCS811 | 0x5B
  if(!ccs.begin(0x5B)){
    Serial.println("CJMCU-811 nie dziala");
  }

  while(!ccs.available());
}

// For this project, you don't need to change anything in the loop function. Basically it ensures that you ESP is connected to your broker
void loop() {
  // unsigned long currentMillis = millis();
    

  // if (!client.connected()) {
  //   reconnect();
  // }
  // if(!client.loop())
  //   client.connect("ESP8266Client", MQTT_username, MQTT_password);

  // // Publish z interwałem 200ms
  // if (currentMillis - previousMillis >= 200) {
  //   previousMillis = currentMillis;
  //   client.publish("przycisk",String(digitalRead(BUTTON_PIN)).c_str());



  // } 


      
    if (ccs.available())
    {
      if(!ccs.readData()){
        int tempCO2 = ccs.geteCO2();
        int tempVOC = ccs.getTVOC();
        client.publish("CCS811/CO2",String(tempCO2).c_str());
        client.publish("CCS811/TVOC",String(tempVOC).c_str());
        Serial.print("ppm, TVOC: ");
        Serial.println(tempVOC);
        Serial.print("ppb Temp:");
        Serial.println(tempCO2);
      }else{
        Serial.println("ERROR!");
      }

    }
    delay(500);
    // if(ccs.available()){
    //     float temp = ccs.calculateTemperature();
    //     if(!ccs.readData()){
    //     Serial.print("CO2: ");
    //     Serial.print(ccs.geteCO2());
    //     Serial.print("ppm, TVOC: ");
    //     Serial.print(ccs.getTVOC());
    //     Serial.print("ppb Temp:");
    //     Serial.println(temp);
    //     }
    // }
}

