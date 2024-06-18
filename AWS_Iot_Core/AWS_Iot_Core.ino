#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <time.h>
#include "secrets.h"

#define TIME_ZONE +5
 

 
int h;
float t;
int heart_rate;
int o2_rate;
int blood_rate;
int pulse_rate;
int temp_rate;
int check_rate;
unsigned long lastMillis = 0;
unsigned long previousMillis = 0;
const long interval = 5000;
unsigned long lastUpdateTime = 0;

 
#define AWS_IOT_PUBLISH_TOPIC   "esp8266/pub"
#define AWS_IOT_SUBSCRIBE_TOPIC "esp8266/sub"
 
WiFiClientSecure net;
 
BearSSL::X509List cert(cacert);
BearSSL::X509List client_crt(client_cert);
BearSSL::PrivateKey key(privkey);
 
PubSubClient client(net);
 
time_t now;
time_t nowish = 1510592825;
 
 
void NTPConnect(void)
{
  Serial.print("Setting time using SNTP");
  configTime(TIME_ZONE * 3600, 0 * 3600, "pool.ntp.org", "time.nist.gov");
  now = time(nullptr);
  while (now < nowish)
  {
    delay(500);
    Serial.print("ntpc erroer ");
    now = time(nullptr);
  }
  Serial.println("done!");
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  Serial.print("Current time: ");
  Serial.print(asctime(&timeinfo));
}
 
 
void messageReceived(char *topic, byte *payload, unsigned int length)
{
  Serial.print("Received [");
  Serial.print(topic);
  Serial.print("]: ");
  for (int i = 0; i < length; i++)
  {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}
 
 
void connectAWS()
{
  delay(3000);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
 
  Serial.println(String("Attempting to connect to SSID: ") + String(WIFI_SSID));
 
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print("wifi not connected ");
    delay(1000);
  }
 
  NTPConnect();
 
  net.setTrustAnchors(&cert);
  net.setClientRSACert(&client_crt, &key);
 
  client.setServer(MQTT_HOST, 8883);
  client.setCallback(messageReceived);
 
 
  Serial.println("Connecting to AWS IOT");
 
  while (!client.connect(THINGNAME))
  {
    Serial.print("thingname error  ");
    delay(1000);
  }
 
  if (!client.connected()) {
    Serial.println("AWS IoT Timeout!");
    return;
  }
  // Subscribe to a topic
  client.subscribe(AWS_IOT_SUBSCRIBE_TOPIC);
 
  Serial.println("AWS IoT Connected!");
}
 
 
void publishMessage()
{
  StaticJsonDocument<200> doc;
  doc["time"] = millis();
  doc["heart_rate"] = heart_rate;
  doc["o2_rate"] = o2_rate;
  doc["blood_rate"] = blood_rate;
  doc["pulse_rate"] = pulse_rate;
  doc["temp_rate"] = temp_rate;
  doc["check_rate"] = check_rate;
  
  
  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer); // print to client
 
  client.publish(AWS_IOT_PUBLISH_TOPIC, jsonBuffer);
}
 
 
void setup()
{
  Serial.begin(115200);
  connectAWS();
  
}
 
 
void loop()
{
  h = 0;
  t = 0;
 
  if (isnan(h) || isnan(t) )  // Check if any reads failed and exit early (to try again).
  {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }
 
  
  delay(2000);
 
  now = time(nullptr);

  unsigned long currentTime = millis();
  
  // Check if one second has passed since the last update
  if (currentTime - lastUpdateTime >= 1000) {
    // Generate a new random value
    heart_rate = random(70,80);
    o2_rate = random(70,80);
    blood_rate = random(70,80);
    pulse_rate = random(70,80);
    temp_rate = random(70,80);
    check_rate = random(70,80);// +1 is to include maxValue in the range
    
    Serial.println(heart_rate);
    Serial.println(o2_rate);
    Serial.println(blood_rate);
    Serial.println(pulse_rate);
    Serial.println(temp_rate);
    Serial.println(check_rate);
    
    // Update the last update time
    lastUpdateTime = currentTime;
  }
 
  if (!client.connected())
  {
    connectAWS();
  }
  else
  {
    client.loop();
    if (millis() - lastMillis > 5000)
    {
      lastMillis = millis();
      publishMessage();
    }
  }
}
