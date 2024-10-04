#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <ArduinoJson.h>
 
const char* ssid = "ESP32-AP";
const char* password = "password123";
 
IPAddress local_ip(192,168,1,1);
IPAddress gateway(192,168,1,1);
IPAddress subnet(255,255,255,0);
 
WebServer server(80);
Preferences preferences;
 
void setup() {
  Serial.begin(115200);
 
  WiFi.softAP(ssid, password);
  WiFi.softAPConfig(local_ip, gateway, subnet);
 
  server.on("/save", HTTP_POST, handleSave);
  server.on("/saveHuella", HTTP_POST, handleSaveHuella);
  server.on("/users", HTTP_GET, handleListUsers);
  server.on("/validar", HTTP_POST, handleValidar);
 
  server.begin();
 
  preferences.begin("userdata", false);
 
  Serial.println("HTTP server started");
  Serial.print("IP address: ");
  Serial.println(WiFi.softAPIP());
}
 
void loop() {
  server.handleClient();
}
 
void handleSave() {
  if (server.hasArg("plain") == false) {
    server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Body not received\"}");
    return;
  }
 
  String body = server.arg("plain");
  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, body);
 
  if (error) {
    server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Failed to parse JSON\"}");
    return;
  }
 
  String usuario = doc["usuario"];
  String contrasena = doc["contrasena"];
  String huella = doc["huella"];  // Esto será una cadena vacía inicialmente
 
  if (usuario != "" && contrasena != "") {
    int userCount = preferences.getInt("userCount", 0);
    String userKey = "user" + String(userCount);
    preferences.putString(userKey.c_str(), usuario);
    preferences.putString((userKey + "_pass").c_str(), contrasena);
    preferences.putString((userKey + "_huella").c_str(), huella);  // Guardamos la huella vacía
    preferences.putInt("userCount", userCount + 1);
    server.send(200, "application/json", "{\"status\":\"success\",\"message\":\"Usuario guardado correctamente.\"}");
  } else {
    server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Falta usuario o contraseña\"}");
  }
}
 
void handleSaveHuella() {
  if (server.hasArg("plain") == false) {
    server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Body not received\"}");
    return;
  }
 
  String body = server.arg("plain");
  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, body);
 
  if (error) {
    server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Failed to parse JSON\"}");
    return;
  }
 
  String usuario = doc["usuario"];
  String huella = doc["huella"];
 
  if (usuario != "" && huella != "") {
    int userCount = preferences.getInt("userCount", 0);
    bool userFound = false;
    for (int i = 0; i < userCount; i++) {
      String userKey = "user" + String(i);
      if (preferences.getString(userKey.c_str(), "") == usuario) {
        preferences.putString((userKey + "_huella").c_str(), huella);
        userFound = true;
        break;
      }
    }
    if (userFound) {
      server.send(200, "application/json", "{\"status\":\"success\",\"message\":\"Huella guardada correctamente\"}");
    } else {
      server.send(404, "application/json", "{\"status\":\"error\",\"message\":\"Usuario no encontrado\"}");
    }
  } else {
    server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Falta usuario o huella\"}");
  }
}
 
void handleListUsers() {
  int userCount = preferences.getInt("userCount", 0);
  DynamicJsonDocument doc(1024);
  JsonArray users = doc.createNestedArray("users");
 
  for (int i = 0; i < userCount; i++) {
    String userKey = "user" + String(i);
    String userName = preferences.getString(userKey.c_str(), "");
    String userContrasena = preferences.getString((userKey + "_pass").c_str(), "");
    String userHuella = preferences.getString((userKey + "_huella").c_str(), "No registrada");
    JsonObject user = users.createNestedObject();
    user["nombre"] = userName;
    user["contrasena"] = userContrasena;
    user["huella"] = userHuella;
  }
 
  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}
 
void handleValidar() {
  if (server.hasArg("plain") == false) {
    server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Body not received\"}");
    return;
  }
 
  String body = server.arg("plain");
  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, body);
 
  if (error) {
    server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Failed to parse JSON\"}");
    return;
  }
 
  String usuario = doc["usuario"];
  String contrasena = doc["contrasena"];
 
  if (usuario != "" && contrasena != "") {
    int userCount = preferences.getInt("userCount", 0);
    bool userFound = false;
    for (int i = 0; i < userCount; i++) {
      String userKey = "user" + String(i);
      if (preferences.getString(userKey.c_str(), "") == usuario) {
        if (preferences.getString((userKey + "_pass").c_str(), "") == contrasena) {
          userFound = true;
          break;
        }
      }
    }
    if (userFound) {
      server.send(200, "application/json", "{\"status\":\"success\",\"message\":\"Usuario autenticado correctamente\"}");
    } else {
      server.send(401, "application/json", "{\"status\":\"not_registered\",\"message\":\"Usuario no registrado o contraseña incorrecta\"}");
    }
  } else {
    server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Falta usuario o contraseña\"}");
  }
}