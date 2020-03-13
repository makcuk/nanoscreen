
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
#include <Button2.h>
#include <Shell.h>

#define FORMAT_SPIFFS_IF_FAILED false
#define BUTTON_1        35
#define BUTTON_2        0

/* globals */

enum pages{page0, page1, page2, page3, page4, page5, page6, page7, page8, page9};

int current_page = page0;

Button2 btn1(BUTTON_1);
Button2 btn2(BUTTON_2);
int rightClick = false;
int leftClick = false;

unsigned int check_file_size = 0;

TFT_eSPI tft = TFT_eSPI();           // TFT object
TFT_eSprite spr = TFT_eSprite(&tft); // Sprite object

const char* host = "esp32-display";
char *ssid = "test";
char *password = "test";

bool new_upload = true;
bool new_upload_displayed = false;

WebServer server(80);
const char* serverIndex = "<form method='POST' action='/update' enctype='multipart/form-data'><input type='file' name='update'><input type='submit' value='Update'></form>";

void setup() {
  Serial.begin(115200);
  

  tft.begin();
  tft.setRotation(0);  // 0 & 2 Portrait. 1 & 3 landscape
  tft.fillScreen(TFT_BLACK);

  button_init();
  Serial.println();
  Serial.println("Booting Sketch...");
  if(!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED)){
        Serial.println("SPIFFS Mount Failed");
        return;
    }
  ssid = load_file("/ssid");
  password = load_file("/password");
  
  WiFi.mode(WIFI_AP_STA);
  if (!ssid) {
    ssid = "test";
    password = "test";
    Serial.println("Wi-fi settings not found, using default test/test");
  } else {
    Serial.println("Wi-Fi settings found, connecting");
  }

  delay(1000);
  shell_init(shell_reader, shell_writer, 0);
  shell_register(set_wifi, PSTR("set_wifi"));
  shell_register(show_storage, PSTR("show_storage"));
  shell_register(show_wifi, PSTR("show_wifi"));

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
    }, []() {

      HTTPUpload& upload = server.upload();
      String new_file_name = upload.filename.c_str();
      if (new_upload) {
        SPIFFS.remove("/"+new_file_name);
        new_upload = false;
      }
      File file = SPIFFS.open("/"+new_file_name, FILE_APPEND);
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


/* shell commands begins */
int set_wifi(int argc, char** argv)
{
  shell_println("Setting wifi");
  
  if (argc != 3 ) {
    shell_println("Syntax: set_wifi <ssid> <password>");
  } else {
    shell_println(argv[1]);
    shell_println(argv[2]);
    File ssid = SPIFFS.open("/ssid", "wb");
    File password = SPIFFS.open("/password", "wb");
    ssid.print(argv[1]);
    password.print(argv[2]);
    ssid.close();
    password.close();
    ESP.restart();
  }
  return SHELL_RET_SUCCESS;
}

int show_storage(int argc, char **argv)
{
  shell_println("SPIFFS drive contents: ");
  File root = SPIFFS.open("/");
 
  File file = root.openNextFile();
 
  while(file){
      shell_println(file.name());
      file = root.openNextFile();
  }
}

int show_wifi(int argc, char **argv)
{
  shell_println("Wi-Fi settings: ");
  char *ssid = load_file("/ssid");
  char *password = load_file("/password");

  if(ssid && password) {
    Serial.printf("SSID: %s, password: %s\n", ssid, password);
  } else {
    shell_println("No Wi-Fi configuration, use set_wifi <ssid> <password> to populate");
  }

}
void loop(void) {
  //
  button_loop();
  shell_task();
  server.handleClient();
  if (rightClick) {
      current_page +=1;
      if (current_page > 9) {
        current_page = page0;
      }
      String page = "/page"+String(current_page)+".bmp";
      if(!SPIFFS.exists(page)) {
        current_page -= 1;
      }
      rightClick = false;
      new_upload_displayed = false;
  }
  if (leftClick) {
      current_page -= 1;
     
      String page = "/page"+String(current_page)+".bmp";
      if(!SPIFFS.exists(page)) {
        current_page += 1;
      }
      if (current_page < 0) {
        current_page = page9;
      }
      leftClick = false;
      new_upload_displayed = false;
  }
  
  if (!new_upload_displayed) {
    size_t needed = snprintf(NULL, 0, "/page%d.bmp", current_page) + 1;
    char *page = (char *) malloc(needed);
    sprintf(page,"/page%d.bmp", current_page);
   // tft.fillScreen(TFT_BLACK);
    //Serial.println(page);
    drawBmp(page, 0, 0);
    new_upload_displayed = true;
  }
  delay(1);
}


void button_init()
{
    btn1.setPressedHandler([](Button2 & b) {
        //Serial.println("Right");
        rightClick = true;
    });

    btn2.setPressedHandler([](Button2 & b) {
        //Serial.println("Left");
        leftClick = true;
    });
}

void button_loop()
{
    btn1.loop();
    btn2.loop();
}

int shell_reader(char * data)
{
  // Wrapper for Serial.read() method
  if (Serial.available()) {
    *data = Serial.read();
    return 1;
  }
  return 0;
}

void shell_writer(char data)
{
  // Wrapper for Serial.write() method
  Serial.write(data);
}

char *load_file(char *fname) 
{
char * buffer = 0;
unsigned long length;
fs::File f = SPIFFS.open(fname, "rb");

if (f.available())
{
  length = f.size();
  buffer = (char *) malloc (length+1);
  if (buffer)
  {
    f.read((unsigned char *)buffer, length);
  }
  buffer[length] = '\0';
  f.close();
}

if (buffer)
{
  return buffer;
}
return NULL;
}
