/*
 * Copyright (c) 2020, Max Speransky


Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <vfs_api.h>
#include <FS.h>
#include <FSImpl.h>

#include <SPIFFS.h>

#include <TFT_eSPI.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>


#define FORMAT_SPIFFS_IF_FAILED false

unsigned int check_file_size = 0;

TFT_eSPI tft = TFT_eSPI();           // TFT object

TFT_eSprite spr = TFT_eSprite(&tft); // Sprite object

const char* host = "esp32-display";
const char* ssid = "...";
const char* password = "....";

bool new_upload = true;
bool new_upload_displayed = false;

WebServer server(80);
const char* serverIndex = "<form method='POST' action='/update' enctype='multipart/form-data'><input type='file' name='update'><input type='submit' value='Update'></form>";

void setup() {
  Serial.begin(115200);
  
  tft.begin();
  tft.setRotation(0);  // 0 & 2 Portrait. 1 & 3 landscape
  tft.fillScreen(TFT_BLACK);

 
  Serial.println();
  Serial.println("Booting Sketch...");
  if(!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED)){
        Serial.println("SPIFFS Mount Failed");
        return;
    }
  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(ssid, password);
  if (WiFi.waitForConnectResult() == WL_CONNECTED) {
    MDNS.begin(host);
    server.on("/", HTTP_GET, []() {
      server.sendHeader("Connection", "close");
      server.send(200, "text/html", serverIndex);
    });
    server.on("/update", HTTP_POST, []() {
      server.sendHeader("Connection", "close");
      server.send(200, "text/plain", "OK");
   //   ESP.restart();
    }, []() {

      HTTPUpload& upload = server.upload();
      if (new_upload) {
        SPIFFS.remove("/default.bmp");
        new_upload = false;
      }
      File file = SPIFFS.open("/default.bmp", FILE_APPEND);
      if (upload.status == UPLOAD_FILE_START) {
        Serial.setDebugOutput(true);
        Serial.printf("Received: %s\n", upload.filename.c_str());
        
      } else if (upload.status == UPLOAD_FILE_WRITE) {
        file.write(upload.buf, upload.currentSize);
        Serial.printf("Upload size: %d\n", upload.currentSize);
     
      } else if (upload.status == UPLOAD_FILE_END) {
        file.close();
        new_upload = true;
        new_upload_displayed = false;
        
      } else {
        Serial.printf("Upload Failed Unexpectedly (likely broken connection): status=%d\n", upload.status);
      }
      
    });
    server.begin();
    MDNS.addService("http", "tcp", 80);

    Serial.printf("Ready! Open http://%s.local in your browser\n", host);
  } else {
    Serial.println("WiFi Failed");
  }
}


void loop(void) {
  //
  server.handleClient();
  if (!new_upload_displayed) {
   // tft.fillScreen(TFT_BLACK);
    drawBmp("/default.bmp", 0, 0);
    new_upload_displayed = true;
  }
  delay(1);
}
