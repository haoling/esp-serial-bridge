
/*
 * ESP8266 UART <-> WiFi(UDP) <-> UART Bridge
 * ESP8266 TX of Serial1 is GPIO2
 */

#if ESP8266
#include <ESP8266WiFi.h>
#else
#include <WiFi.h>
#endif
#include <WiFiUdp.h>

#include "credentials.h"

const char* ssid = WIFI_SSID;
const char* password = WIFI_PW;

WiFiUDP Udp;
unsigned int localUdpPort = PORT;  // local port to listen on
#define bufferSize 1024
char incomingPacket[bufferSize + 1];  // buffer for incoming packets

#if IS_RECEIVER == 1

#define MY_IPADDR RECEIVER_IPADDR
#define REMOTE_IPADDR SENDER_IPADDR

#else

#define MY_IPADDR SENDER_IPADDR
#define REMOTE_IPADDR RECEIVER_IPADDR

#endif

uint8_t iofs=0;
IPAddress remoteIp(REMOTE_IPADDR);
unsigned int remotePort = PORT;

IPAddress local_IP(MY_IPADDR);
IPAddress gateway(GATEWAY_IPADDR);
IPAddress subnet(SUBNET);

void setup()
{
  delay(1000);
  #if ESP8266
  system_update_cpu_freq(160);
  WiFi.setOutputPower(20.5f);
  #endif
  Serial.begin(230400);
  #if SWAP_SERIAL
  Serial.swap();
  #endif
  Serial1.begin(921600);
  Serial1.println("uni-station-proxy START");

  if (!WiFi.config(local_IP, gateway, subnet)) {
    Serial.println("STA Failed to configure");
  }

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial1.print(".");
  }
  Serial1.println("Wifi connected");
  Serial1.println(WiFi.localIP().toString());

  Udp.begin(localUdpPort);
  Udp.beginPacket(remoteIp, remotePort); // remote IP and port
}

unsigned long lastLog = 0;
unsigned int sent = 0;
unsigned int recv = 0;
void loop()
{
  int packetSize;
  while (1) {
    packetSize = Udp.parsePacket();
    if (! packetSize) {
      break;
    }
    // receive incoming UDP packets
    int len = Udp.read(incomingPacket, bufferSize);
    Serial.write(incomingPacket, len);
    recv += len;
  }

  while (1) {
    int c = Serial.read(); // read char from UART
    if (c == -1) {
      break;
    }
    Udp.write((char)c);
    iofs++;
    if ((char)c == 0x0a || (char)c == 0x00) {
      Udp.endPacket();
      sent += iofs;
      iofs = 0;
    }
  }
  if (lastLog + 1000 <= millis()) {
    if (iofs > 0) {
      Udp.endPacket();
      sent += iofs;
      iofs = 0;
    }
    Serial1.printf(">%u\n", sent);
    Serial1.printf("<%u\n", recv);
    sent = 0;
    recv = 0;
    lastLog = millis();
  }
}