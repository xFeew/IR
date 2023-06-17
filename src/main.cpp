#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <SPI.h>

#include "SparkFunCCS811.h"

#include <BH1750.h>

#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

// Dane do połączenia z siecią Wi-fi
const char *ssid = "Wifi dom";
const char *password = "warszawa";

// MQTT broker login/hasło
const char *MQTT_username = NULL;
const char *MQTT_password = NULL;

// Adres IP server'a MQTT
const char *MQTT_SERVER= "szymonlach.ddns.net";
const uint16_t MQTT_PORT = 9004;

// LED
#define LED_PIN 14 // d5

// Przycisk
#define BUTTON_PIN 12 // d6
int lastButtonState;

// DHT22
#define DHT22_PIN 13 // D7
#define DHTTYPE DHT22
DHT_Unified dht(DHT22_PIN, DHTTYPE);

// CJMCU-811
#define CCS811_ADDR 0x5A
CCS811 ccs811(CCS811_ADDR);

// BH1750
#define BH1750_ADDR 0x23
BH1750 bh1750(BH1750_ADDR);

// Wifi & MQTT & HTTP
WiFiClient espClient;
PubSubClient client(espClient);

long now = millis();
long lastMeasure = 0;
unsigned long previousMillis = 0;
unsigned long previousSensorMillis = 0;

void setupWifi()
{
	delay(10);
	Serial.println();
	Serial.print("Laczenie z siecia ");
	Serial.println(ssid);
	WiFi.begin(ssid, password);
	while (WiFi.status() != WL_CONNECTED)
	{
		delay(500);
		Serial.print(".");
	}
	Serial.println("");
	Serial.print("Polaczono!, adres IP urzadzenia: ");
	Serial.println(WiFi.localIP());
}

void callback(String topic, byte *message, unsigned int length)
{
	Serial.print("Uzyskano odpowiedz na temat ");
	Serial.print(topic);
	Serial.print(". Wiadomosc: ");
	String messageTemp;

	for (int i = 0; i < length; i++)
	{
		Serial.print((char)message[i]);
		messageTemp += (char)message[i];
	}
	Serial.println();

	if (topic == "led")
	{
		if (messageTemp == "on")
		{
			digitalWrite(LED_PIN, HIGH);
		}
		else if (messageTemp == "off")
		{
			digitalWrite(LED_PIN, LOW);
		}
	}

	Serial.println();
}

void reconnect()
{
	while (!client.connected())
	{
		Serial.print("Proba polaczenia MQTT...");
		if (client.connect("ESP8266_2", MQTT_username, MQTT_password))
		{
			Serial.println("Polaczono!");
			client.subscribe("led");
		}
		else
		{
			Serial.print("blad, rc=");
			Serial.print(client.state());
			delay(5000);
		}
	}
}

// Funkcje obsługi czujników

void handleDHTSensor()
{
	sensors_event_t event;
	dht.temperature().getEvent(&event);
	if (!isnan(event.temperature))
	{
		client.publish("DHT22/TEMP", String(event.temperature).c_str());
	}

	dht.humidity().getEvent(&event);
	if (!isnan(event.relative_humidity))
	{
		client.publish("DHT22/HUMIDITY", String(event.relative_humidity).c_str());
	}
}

void handleBH1750Sensor()
{
	float lux = bh1750.readLightLevel();
	client.publish("BHT1750", String(lux).c_str());
}

void handleCCS811Sensor()
{
	if (ccs811.dataAvailable())
	{
		ccs811.readAlgorithmResults();
		int tempCO2 = ccs811.getCO2();
		int tempVOC = ccs811.getTVOC();
		client.publish("CCS811/CO2", String(tempCO2).c_str());
		client.publish("CCS811/TVOC", String(tempVOC).c_str());
	}
}

void setup()
{
	Wire.begin(); // I2C
	Serial.begin(115200);
	// LED
	pinMode(LED_PIN, OUTPUT);
	// PRZYCISK
	pinMode(BUTTON_PIN, INPUT_PULLUP);
	lastButtonState = digitalRead(BUTTON_PIN);

	//Wifi & MQTT
	setupWifi();
	client.setServer(MQTT_SERVER, MQTT_PORT);
	client.setCallback(callback);

	dht.begin();
	ccs811.begin();
	bh1750.begin();
}

void loop()
{
	if (!client.connected())
	{
		reconnect();
	}
	if (!client.loop())
	{
		client.connect("ESP8266Client", MQTT_username, MQTT_password);
	}

	// "Asynchronicznie" żeby nie przeciążać serwera MQTT ilością danych.
	unsigned long currentMillis = millis();
	if (currentMillis - previousMillis >= 100)
	{
		previousMillis = currentMillis;
		client.publish("przycisk", String(digitalRead(BUTTON_PIN)).c_str());
	}

	if (currentMillis - previousSensorMillis >= 1000)
	{
		previousSensorMillis = currentMillis;
		handleDHTSensor();
		handleBH1750Sensor();
		handleCCS811Sensor();
	}
}
