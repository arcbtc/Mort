
//x2 serveo ssh tunnel need to be made
//For connecting ESP32 webserver I used "ssh -R CHOOSE-RANDOM-NAME.serveo.net:ESP32-IP-ADDRESS:443 serveo.net" Replace ESP32-IP-ADDRESS with the ip in Serial Monitor, once uploaded
//For connecting Zap Desktop Wallet I used "ssh -R CHOOSE-RANDOM-NAME.serveo.net:3010:localhost:8180 serveo.net"
//CHOOSE-RANDOM-NAME needs to be changed to match line 22,23

#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Conversion.h>
#include "qrcode.h"
#define MAX_BECH32_SIZE 1000
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
 
const char *ssid = "YOUR-WIFI"; //change
const char *password = "WIFI-PASS"; //change

WebServer server(443);

char data[] = "https://CHOOSE-RANDOM-NAME.serveo.net"; //ESP32 serveo tunnel
const char* lndhost = "CHOOSE-RANDOM-NAME.serveo.net"; //zap serveo tunnel
uint8_t data5[100];
size_t data5_len = 0;
int persecret = 1;
char output[100];
String walletstr;
String lninvoice;
String header;
String rannumber = String(random(100000, 1000000));
unsigned long timex;
unsigned long timez;
String giftinvoice;
String letters = "abcdefghijklmnopqrstuvwxyz0123456789";
String secret;
 
String adminmacaroon = "YOUR-ADMIN-MACAROON"; //How to get from Zap Desktop Wallet https://www.youtube.com/watch?v=yxaEO4hXIZk
const int lndport = 3010;

void setup(void) {
  Serial.begin(115200);
  keymaker();
  convert_bits(data5, &data5_len, 5, (const uint8_t*)data, strlen(data), 8, 1);
  int res = bech32_encode(output, "lnurl", (uint8_t *)data5, data5_len);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");
 
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
 
  if (MDNS.begin("esp32")) {
    Serial.println("MDNS responder started");
  }
  
  server.on("/", []() {
    String callback = String(data);
    server.send(200, "application/json", "{\"status\":\"OK\",\"callback\":\"" + String(data) + "/withdraw/" + secret + "/\", \"k1\":\"" + secret + "\", \"maxWithdrawable\":100000, \"defaultDescription\": \"arcbtc\", \"minWithdrawable\":100000,\"tag\": \"withdrawRequest\"}");
  });
    server.on("/withdraw/{}/", []() {
    timez = millis();
    String secre = server.pathArg(0); 
    if (timez - timex >= 30000 && secre == secret){
      server.send(200, "application/json", "{\"status\":\"OK\"}");
      Serial.println("passed");
      
       for(int i = 0;i <= 3;i++){
      if (server.argName(i) == "pr"){
       giftinvoice = server.arg(i);
      }
      }
      
      Serial.println(giftinvoice);
      makepayment();
      }
     else{
      server.send(200, "application/json", "{\"status\":\"ERROR\", \"reason\":\"MORE SATS IN "+ String(30 - ((timez/1000) - (timex/1000))) +" SECS\"}");
      Serial.println("fail");
      }
    });
 
  server.on("/users/{}/devices/{}", []() {
    String user = server.pathArg(0);
    String device = server.pathArg(1);
    server.send(200, "text/plain", "User: '" + user + "' and Device: '" + device + "'");
  });
 
  server.begin();
  Serial.println("HTTP server started");
 
  // Create the QR code
    QRCode qrcode;
    uint8_t qrcodeData[qrcode_getBufferSize(3)];
    qrcode_initText(&qrcode, qrcodeData, 3, 0, output);
 
    // Top quiet zone
    Serial.print("\n\n\n\n");
 
    for (uint8_t y = 0; y < qrcode.size; y++) {
 
        // Left quiet zone
        Serial.print("        ");
 
        // Each horizontal module
        for (uint8_t x = 0; x < qrcode.size; x++) {
 
            // Print each module (UTF-8 \u2588 is a solid block)
            Serial.print(qrcode_getModule(&qrcode, x, y) ? "\u2588\u2588": "  ");
 
        }
 
        Serial.print("\n");
    }
 
    // Bottom quiet zone
    Serial.print("\n\n\n\n");
 
   
}
 
void loop(void) {
  server.handleClient();
}
 
void makepayment(){
  String memo = "Memo-";
  WiFiClientSecure client;
  if (!client.connect(lndhost, lndport)){
    return;
  }
  String topost = "{\"payment_request\": \""+ giftinvoice +"\"}";
  client.print(String("POST ")+ "https://" + lndhost +":"+ String(lndport) +"/v1/channels/transactions HTTP/1.1\r\n" +
               "Host: "  + lndhost +":"+ String(lndport) +"\r\n" +
               "User-Agent: ESP322\r\n" +
               "Grpc-Metadata-macaroon:" + adminmacaroon + "\r\n" +
               "Content-Type: application/json\r\n" +
               "Connection: close\r\n" +
               "Content-Length: " + topost.length() + "\r\n" +
               "\r\n" +
               topost + "\n");
 
  String line = client.readStringUntil('\n');
  Serial.println(line);
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {  
      break;
    }
  }  
  String content = client.readStringUntil('\n');
  client.stop();
 
  timex = millis();
 
  keymaker();
   
}
 
void keymaker(){
  secret = "";
    for(int i = 0;i <= 30; i++){
      int tempnum = random(0, 30);
      secret += letters[tempnum];
    }
}
