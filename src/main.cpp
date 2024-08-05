//
// Created by Robin Derenty on 21/05/2024.
//
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>

// WiFi settings
const char *ssid = "SSID";             // Replace with your WiFi name
const char *password = "Password";   // Replace with your WiFi password

// MQTT Broker settings
const char *mqtt_broker = "coloc.derenty.net";// EMQX broker endpoint
const char *mqtt_topic = "cocktailMachine"; // MQTT topic
const char *mqtt_username = "ha-mqtt"; // MQTT username for authentication
const char *mqtt_password = "ha-mqtt"; // MQTT password for authentication
const unsigned mqtt_port = 62810; // MQTT port (TCP)

WiFiClient espClient;
PubSubClient mqtt_client(espClient);

byte gate1 = 4;
byte gate2 = 5;
byte gate3 = 6;

byte allGates[] = {gate1, gate2, gate3};

struct alcohol
{
    byte gateNumber;
    String alcoholName;
};

alcohol alcohol1;
alcohol alcohol2;

alcohol allAlcoholList[] = {alcohol1, alcohol2};

struct soft
{
    byte gateNumber;
    String softName;
};

soft soft1;

soft allSoftsList[] = {soft1};

void connectToWiFi();

void connectToMQTTBroker();

void mqttCallback(char *topic, byte *payload, unsigned int length);

byte checkSoft(String soft);

byte checkAlcohol(String alcohol);

void make(int taille, int pourcentage, String alcool, String soft);


void setup() {
    Serial.begin(9600);
    pinMode(v1, OUTPUT);
    pinMode(v2, OUTPUT);
    pinMode(v3, OUTPUT);
    connectToWiFi();
    mqtt_client.setServer(mqtt_broker, mqtt_port);
    mqtt_client.setCallback(mqttCallback);
    connectToMQTTBroker();

}

void loop() {
    if (!mqtt_client.connected()) {
        connectToMQTTBroker();
    }
    mqtt_client.loop();
}


void connectToWiFi() {
    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nConnected to the WiFi network");
}

void connectToMQTTBroker() {
    while (!mqtt_client.connected()) {
        String client_id = "esp8266-client-" + String(WiFi.macAddress());
        Serial.printf("Connecting to MQTT Broker as %s.....\n", client_id.c_str());
        if (mqtt_client.connect(client_id.c_str(), mqtt_username, mqtt_password)) {
            Serial.println("Connected to MQTT broker");
            mqtt_client.subscribe(mqtt_topic);
            // Publish message upon successful connection
            mqtt_client.publish(mqtt_topic, "Cocktail Machine Ready !");
        } else {
            Serial.print("Failed to connect to MQTT broker, rc=");
            Serial.print(mqtt_client.state());
            Serial.println(" try again in 5 seconds");
            delay(5000);
        }
    }
}

void mqttCallback(char *topic, byte *payload, unsigned int length) {
    Serial.print("Message received on topic: ");
    Serial.println(topic);
    char message[length];
    for (unsigned int i = 0; i < length; i++) {
        message[i]=(char) payload[i];
    }

    JsonDocument doc;
    deserializeJson(doc, message);
    String sendType = doc["sendType"];
    Serial.println(sendType);
    if(sendType == "Make")
    {
        String alcohol = doc["alcohol"];
        String soft = doc["soft"];
        int percentage = doc["percentage"];
        int size = doc["size"];
        make(size, percentage, alcohol, soft);
    }
    else if(sendType == "Update")
    {
        String alcoholPresented = doc["alcoholPresented"];
        JsonDocument alcohols;
        deserializeJson(alcohols, alcoholPresented);
        byte alcoholLength;
        int i=0;
        while(alcohols[i])
        {
            i++;
        }
        alcoholLength = i;
        for(int j =0; j<alcoholLength; j++)
        {
            String firstAlcohol = alcohols[j]["alcoholName"];
            allAlcoholList[j] = {allGates[j], firstAlcohol};
        }

        String softPresented = doc["softPresented"];
        JsonDocument softs;
        deserializeJson(softs, softPresented);
        byte softLength;
        i=0;
        while(alcohols[i])
        {
            i++;
        }
        softLength = i;
        for(int j =0; j<softLength; j++)
        {
            String firstSoft = softs[j]["softName"];
            allSoftsList[j] = {allGates[j], firstSoft};
        }

    }


    Serial.println();
    Serial.println("-----------------------");

}


void make(int taille, int pourcentage, String alcool, String soft){


    byte VA =checkAlcohol(alcool);
    byte VS =checkSoft(soft);



    if(VA == 0 || VS == 0)
    {
        String response = VA==0 ? "Pas cette alcool en stock" : "Pas ce soft en stock";
        Serial.println(response);
    }
    else
    {
        digitalWrite(VA, HIGH);
        Serial.println("Ouverture Alcool");
        delay(1000*(float(pourcentage)/100)*taille);
        Serial.println("Fermetture Alcool");
        Serial.println("Ouverture Soft");
        digitalWrite(VA, LOW);
        digitalWrite(VS, HIGH);
        delay(1000*(1-(float(pourcentage)/100))*taille);
        Serial.println("Fermetture Soft");
        digitalWrite(VS, LOW);
    }
};

byte checkAlcohol(String alcohol)
{
    for(int k=0; k<2; k++)
    {
        Serial.println(allAlcoholList[k].alcoholName);
        if(alcohol == allAlcoholList[k].alcoholName) return allAlcoholList[k].gateNumber;
    }
    return 0;
}

byte checkSoft(String soft)
{
    for(int k=0; k<1; k++)
    {
        Serial.println(allSoftsList[k].softName);
        if(soft == allSoftsList[k].softName) return allSoftsList[k].gateNumber;
    }
    return 0;
}
