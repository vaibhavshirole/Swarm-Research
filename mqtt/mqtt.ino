#include <WiFi.h>                     //Allows ESP32 to connect to WiFi
#include <PubSubClient.h>             //Adds publish and subscribe for MQTT
#include <Wire.h>                     //For I2C (not using it here)
#include <WiFiManager.h>              //https://github.com/tzapu/WiFiManager

/*
 * Networking things
 */
//const char* ssid = "";              //Name of WiFi network
//const char* password = "";    //Password of WiFi network

char* mqtt_ip = "empty";                     //YOUR_MQTT_BROKER_IP_ADDRESS
int mqtt_port;                               //Port my MQTT broker is on

 
WiFiClient espClient;                 //Declare object espClient
PubSubClient client(espClient);       
long lastMsg = 0;
char msg[50];
int value = 0;

/*
 * Ultrasonic sensor stuff
 */
const int trigPin = 5;
const int echoPin = 18;

//define sound speed in cm/uS
#define SOUND_SPEED 0.034
#define CM_TO_INCH 0.393701

long duration;
float distanceCm;
float distanceInch;

/*
 * LED pin for debugging
 */
const int ledPin = 4;

/*
 * Button for manually activating the config portal
 * Along with the time the config portal is open for
 */
const int portalPin = 15;
const int timeout = 120;    //120 -> 2 minutes 


void setup() {
  Serial.begin(115200);
  
  pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
  pinMode(echoPin, INPUT);  // Sets the echoPin as an Input
  pinMode(ledPin, OUTPUT);
  pinMode(portalPin, INPUT_PULLUP);           //Wired to ground, activates on press

  setup_connection();       //Connects to WiFi and sets up MQTT
}


void setup_connection() {
  delay(10);
  WiFi.disconnect();        //Disconnect from any network
  WiFi.mode(WIFI_STA);      //Explicitly set WiFi mode (default is STA+AP)
  Serial.println();
  Serial.println();

  Serial.println("Press button (GPIO15) to setup WiFi. Portal will open for 2 minutes");
  while(WiFi.status() != WL_CONNECTED){
    if(digitalRead(portalPin) == LOW){
      Serial.print("Opening AP, connect to it like any other WiFi network. ");
      Serial.println("Attempting to connect to WiFi...");
      WiFiManager wm;
      //wm.setDebugOutput(false);              //Less verbose serial out
      wm.resetSettings();                      //Reset settings for testing
      wm.setConfigPortalTimeout(timeout);      //Portal timeout set for 2 minutes
      
                                          //html object, displayed name, default text, max length
      WiFiManagerParameter ip_box("mqtt_ip", "Enter the MQTT broker's IP", "192.168.4.233", 40);
      wm.addParameter(&ip_box);

      WiFiManagerParameter port_box("mqtt_port", "Enter the MQTT broker's port", "1883", 6);
      wm.addParameter(&port_box);
  
      if (!wm.startConfigPortal("Join-to-setup")) {
        Serial.println("Failed to connect and hit timeout");
        delay(3000);
        //reset and try again, or maybe put it to deep sleep
        ESP.restart();
        delay(5000);
      }
      Serial.print("Connected as: ");  //If here, you've connected to WiFi
      Serial.println(WiFi.localIP());

      mqtt_ip = (char*)ip_box.getValue();             //Gets the user input for IP
      Serial.print("MQTT server ip: ");               //Important: avoid kernel panic by moving string pointer
      Serial.println(mqtt_ip);

      mqtt_port = (int)atol(port_box.getValue());     //Gets the user input for port
      Serial.print("MQTT server port: ");
      Serial.println(mqtt_port);
    }
    
  }
  
  client.setServer(mqtt_ip, mqtt_port);       //Finish by connecting to the MQTT broker
  client.setCallback(callback);               //Create a handler for when MQTT message received  
}


/*
 * When something is published on the topic: esp32/output
 */
void callback(char* topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageLED;
  
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageLED += (char)message[i];
  }
  Serial.println();

  // Feel free to add more if statements to control more GPIOs with MQTT

  // If a message is received on the topic esp32/output, you check if the message is either "on" or "off". 
  // Changes the output state according to the message
  if (String(topic) == "esp32/output") {
    Serial.print("Changing output to ");
    if(messageLED == "on"){
      Serial.println("on");
      digitalWrite(ledPin, HIGH);
    }
    else if(messageLED == "off"){
      Serial.println("off");
      digitalWrite(ledPin, LOW);
    }
  }
}


void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("espClient")) {
      Serial.println("connected");
      // Subscribe
      client.subscribe("esp32/output");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}


void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  if(digitalRead(portalPin) == LOW){
    setup_connection();                         //On button press, disconnect WiFi and allow for setup
  }
  
  long now = millis();
  if (now - lastMsg > 250) {
    lastMsg = now;

    digitalWrite(trigPin, LOW);                 // Clears the trigPin
    delayMicroseconds(2);
    
    digitalWrite(trigPin, HIGH);                // Sets the trigPin on HIGH state for 10 micro seconds
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);
    
    duration = pulseIn(echoPin, HIGH);          // Reads the echoPin, returns the sound wave travel time in microseconds
    distanceCm = duration * SOUND_SPEED/2;      // Calculate the distance
    distanceInch = distanceCm * CM_TO_INCH;     // Convert to inches

    // Convert the value to a char array
    char inchString[8];
    dtostrf(distanceInch, 1, 2, inchString);            //Turns float into string
    Serial.print("Distance (in): ");
    Serial.println(inchString);
    client.publish("esp32/temperature", inchString);    //Publish on topic
    
    // Convert the value to a char array
    char cmString[8];
    dtostrf(distanceCm, 1, 2, cmString);                //Turns float into string
    Serial.print("Distance (cm): ");
    Serial.println(cmString);
    client.publish("esp32/humidity", cmString);         //Publish on topic
  }
}
