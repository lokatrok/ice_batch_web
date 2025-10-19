#include <WiFi.h>
#include <WebServer.h>
#include <SPIFFS.h>

const char* ssid = "ESP32-Debug";

WebServer server(80);

void setup() {
  Serial.begin(115200);
  Serial.println("\n[SETUP] Initializing Debug Mode...");

  // Mount SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("[ERROR] Failed to mount SPIFFS");
    return;
  }
  Serial.println("[INFO] SPIFFS mounted");

  // List files in SPIFFS
  File root = SPIFFS.open("/");
  File file = root.openNextFile();
  Serial.println("[INFO] Files in SPIFFS:");
  while(file){
      Serial.print("  ");
      Serial.println(file.name());
      file = root.openNextFile();
  }
  root.close();

  // Try to read index.html content
  File testFile = SPIFFS.open("/index.html", "r");
  if (testFile) {
    Serial.println("\n[INFO] Content of /index.html:");
    while (testFile.available()) {
      Serial.write(testFile.read());
    }
    Serial.println("\n[INFO] End of /index.html");
    testFile.close();
  } else {
    Serial.println("[ERROR] Could not open /index.html");
  }

  // Start SoftAP
  WiFi.softAP(ssid);
  Serial.println("[INFO] SoftAP started");
  Serial.print("[INFO] IP Address: ");
  Serial.println(WiFi.softAPIP());

  // Serve index.html on root
  server.on("/", HTTP_GET, []() {
    File file = SPIFFS.open("/index.html", "r");
    if (!file) {
      Serial.println("[ERROR] /index.html not found");
      server.send(404, "text/plain", "File not found");
      return;
    }
    Serial.println("[INFO] Serving /index.html");
    server.streamFile(file, "text/html");
    file.close();
  });

  server.begin();
  Serial.println("[INFO] Web server started");
}

void loop() {
  server.handleClient();
}