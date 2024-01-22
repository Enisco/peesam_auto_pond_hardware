#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <SoftwareSerial.h>

// WiFi
const char *ssid = "Hello_";         // Enter your WiFi name
const char *password = "987654321";  // Enter WiFi password

SoftwareSerial mySerial(D5, D6);  // RX, TX

// MQTT Broker
const char *mqtt_broker = "broker.emqx.io";
const char *pubTopic = "peesam/ayomega/pond/app";
const char *subTopic = "peesam/ayomega/app/pond";
const char *mqtt_username = "emqx";
const char *mqtt_password = "public";
const int mqtt_port = 1883;

WiFiClient espClient;
PubSubClient client(espClient);

unsigned long previousMillis = 0;
const long interval = 5000;

void callback(char *topic, byte *payload, unsigned int length) {
  Serial.print("Message arrived in topic: ");
  Serial.println(topic);
  String messageReceived;
  Serial.print("Message:");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    messageReceived += String((char)payload[i]);
  }
  Serial.print("\n\tmessageReceived: ");
  Serial.println(messageReceived);
  // Send data to Nano
  mySerial.println(messageReceived);
  Serial.println("-----------------------");
}

void setup() {
  // Set software serial baud to 115200;
  Serial.begin(115200);
  mySerial.begin(9600);
  // connecting to a WiFi network
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi..");
  }
  Serial.println("Connected to the WiFi network");
  //connecting to a mqtt broker
  client.setServer(mqtt_broker, mqtt_port);
  client.setCallback(callback);

  while (!client.connected()) {
    String client_id = "esp8266-client-dft4rfbihxcvbitdvc98dfgh";
    client_id += String(WiFi.macAddress());
    Serial.printf("The client %s connects to the public mqtt broker\n", client_id.c_str());
    if (client.connect(client_id.c_str(), mqtt_username, mqtt_password)) {
      Serial.println("Public emqx mqtt broker connected");
    } else {
      Serial.print("failed with state ");
      Serial.print(client.state());
      delay(2000);
    }
  }
  client.subscribe(subTopic);
}

void loop() {
  client.loop();
  unsigned long currentMillis = millis();

  if (mySerial.available() > 0) {
    String data = mySerial.readStringUntil('\n');  // Read the data until newline character '\n'

    Serial.print("Received data: ");
    Serial.println(data);

    // Publish an MQTT message
    Serial.printf("Message: %d \n", data);
    client.publish(pubTopic, data.c_str());
  }
}
