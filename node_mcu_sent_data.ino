#include "painlessMesh.h"
#include <ArduinoJson.h>
#include <SPI.h>
#include <Wire.h>
#include <WiFiClient.h>
#include "DHT.h"
#define DHTPIN 2
#define DHTTYPE DHT22

#define   MESH_PREFIX     "HelloMyMesh"
#define   MESH_PASSWORD   "hellomymeshnetwork"
#define   MESH_PORT       5555
DHT dht(DHTPIN, DHTTYPE);
Scheduler     userScheduler;


painlessMesh  mesh;
size_t logServerId = 0;

void receivedCallback( uint32_t from, String &msg );
// Send message to the logServer every 10 seconds

Task logServerTask(10000, TASK_FOREVER, []() {

  float h = dht.readHumidity();
  float t = dht.readTemperature();

  DynamicJsonDocument jsonBuffer(1024);
  JsonObject msg = jsonBuffer.to<JsonObject>();
  
  msg["nodename"] = "mcu-t1";  //change for identify for the node that send data mcu-t1 to mcu-t3
  msg["NodeID"] = mesh.getNodeId();
  
  msg["Temp"] = String(t) + "C";
  msg["Humidity"] = String(h) + "%";
  String str;
  
  serializeJson(msg, str);
  if (logServerId == 0) // If we don't know the logServer yet
    mesh.sendBroadcast(str);
  else
    mesh.sendSingle(logServerId, str);
  // log to serial
  serializeJson(msg, Serial);
  Serial.printf("\n");
});

void setup() {
  Serial.begin(115200);
  Serial.println("Begin DHT22 Mesh Network test!");
  dht.begin();
  
  mesh.setDebugMsgTypes( ERROR | STARTUP | CONNECTION );  // set before init() so that you can see startup messages
  //mesh.init( MESH_PREFIX, MESH_PASSWORD, MESH_PORT, STA_AP, AUTH_WPA2_PSK, 6 );
  mesh.init( MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT, WIFI_AP_STA, 6 );
  mesh.onReceive(&receivedCallback);
  // Add the task to the mesh scheduler
  userScheduler.addTask(logServerTask);
  logServerTask.enable();
}
void loop() {
  // put your main code here, to run repeatedly:
  mesh.update();
}

void receivedCallback( uint32_t from, String &msg ) {
  Serial.printf("logClient: Received from %u msg=%s\n", from, msg.c_str());
  // Saving logServer
  
  DynamicJsonDocument jsonBuffer(1024 + msg.length());
  DeserializationError error = deserializeJson(jsonBuffer, msg);
  if (error) {
    Serial.printf("DeserializationError\n");
    return;
  }
  JsonObject root = jsonBuffer.as<JsonObject>();
  if (root.containsKey("topic")) {
    if (String("logServer").equals(root["topic"].as<String>())) {
      // check for on: true or false
      logServerId = root["nodeId"];
      Serial.printf("logServer detected!!!\n");
    }
    Serial.printf("Handled from %u msg=%s\n", from, msg.c_str());
  }
}
