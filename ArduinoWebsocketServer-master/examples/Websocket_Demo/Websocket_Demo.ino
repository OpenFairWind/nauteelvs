#include <Fishino.h>
#include <SPI.h>
#include <WebSocket.h>

///////////////////////////////////////////////////////////////////////
//           CONFIGURATION DATA -- ADAPT TO YOUR NETWORK             //
//     CONFIGURAZIONE SKETCH -- ADATTARE ALLA PROPRIA RETE WiFi      //
#ifndef __MY_NETWORK_H

// OPERATION MODE :
// NORMAL (STATION)  -- NEEDS AN ACCESS POINT/ROUTER
// STANDALONE (AP)  -- BUILDS THE WIFI INFRASTRUCTURE ON FISHINO
// COMMENT OR UNCOMMENT FOLLOWING #define DEPENDING ON MODE YOU WANT
// MODO DI OPERAZIONE :
// NORMAL (STATION) -- HA BISOGNO DI UNA RETE WIFI ESISTENTE A CUI CONNETTERSI
// STANDALONE (AP)  -- REALIZZA UNA RETE WIFI SUL FISHINO
// COMMENTARE O DE-COMMENTARE LA #define SEGUENTE A SECONDA DELLA MODALITÀ RICHIESTA
#define STANDALONE_MODE

// here pur SSID of your network
// inserire qui lo SSID della rete WiFi
#define MY_SSID  "Fishino32"

// here put PASSWORD of your network. Use "" if none
// inserire qui la PASSWORD della rete WiFi -- Usare "" se la rete non ￨ protetta
#define MY_PASS ""

// here put required IP address (and maybe gateway and netmask!) of your Fishino
// comment out this lines if you want AUTO IP (dhcp)
// NOTE : if you use auto IP you must find it somehow !
// inserire qui l'IP desiderato ed eventualmente gateway e netmask per il fishino
// commentare le linee sotto se si vuole l'IP automatico
// nota : se si utilizza l'IP automatico, occorre un metodo per trovarlo !
#define IPADDR  192, 168,   1, 251
#define GATEWAY 192, 168,   1, 1
#define NETMASK 255, 255, 255, 0

#endif
//                    END OF CONFIGURATION DATA                      //
//                       FINE CONFIGURAZIONE                         //
///////////////////////////////////////////////////////////////////////

// define ip address if required
// NOTE : if your network is not of type 255.255.255.0 or your gateway is not xx.xx.xx.1
// you should set also both netmask and gateway
#ifdef IPADDR
  IPAddress ip(IPADDR);
  #ifdef GATEWAY
    IPAddress gw(GATEWAY);
  #else
    IPAddress gw(ip[0], ip[1], ip[2], 1);
  #endif
  #ifdef NETMASK
    IPAddress nm(NETMASK);
  #else
    IPAddress nm(255, 255, 255, 0);
  #endif
#endif



// Enabe debug tracing to Serial port.
#define DEBUG

// Here we define a maximum framelength to 64 bytes. Default is 256.
#define MAX_FRAME_LENGTH 64

uint8_t mac[WL_MAC_ADDR_LENGTH]={0x11,0x22,0x33,0x44,0x55,0x66};


// Create a Websocket server
WebSocketServer wsServer;

void onConnect(WebSocket &socket) {
  Serial.println("onConnect called");
}


// You must have at least one function with the following signature.
// It will be called by the server when a data frame is received.
void onData(WebSocket &socket, char* dataString, byte frameLength) {
  
#ifdef DEBUG
  Serial.print("Got data: ");
  Serial.write((unsigned char*)dataString, frameLength);
  Serial.println();
#endif
  
  // Just echo back data for fun.
  socket.send(dataString, strlen(dataString));
}

void onDisconnect(WebSocket &socket) {
  Serial.println("onDisconnect called");
}

void setup() {
  
  #ifdef DEBUG  
    Serial.begin(115200);
    while(!Serial);
  #endif
  
  Serial << F("Resetting Fishino...");
  while(!Fishino.reset())
  {
    Serial << ".";
    delay(500);
  }
  Serial << F("OK\r\n");

  Serial << F("Fishino WiFi web server\r\n");

  Fishino.setApMAC(mac);
  // set PHY mode to 11G
  Fishino.setPhyMode(PHY_MODE_11G);
  
  // for AP MODE, setup the AP parameters
  #ifdef STANDALONE_MODE
    // setup SOFT AP mode
    // imposta la modalitè SOFTAP
    Serial << F("Setting mode SOFTAP_MODE\r\n");
    Fishino.setMode(SOFTAP_MODE);

    // stop AP DHCP server
    Serial << F("Stopping DHCP server\r\n");
    Fishino.softApStopDHCPServer();
  
    // setup access point parameters
    // imposta i parametri dell'access point
    Serial << F("Setting AP IP info\r\n");
    Fishino.setApIPInfo(ip, ip, IPAddress(255, 255, 255, 0));

    Serial << F("Setting AP WiFi parameters\r\n");
    Fishino.softApConfig(MY_SSID, MY_PASS, 1, false);
  
    // restart DHCP server
    Serial << F("Starting DHCP server\r\n");
    Fishino.softApStartDHCPServer();
  
  #else
    // setup STATION mode
    // imposta la modalitè STATION
    Serial << F("Setting mode STATION_MODE\r\n");
    Fishino.setMode(STATION_MODE);

    // NOTE : INSERT HERE YOUR WIFI CONNECTION PARAMETERS !!!!!!
    Serial << F("Connecting to AP...");
    while(!Fishino.begin(MY_SSID, MY_PASS))
    {
      Serial << ".";
      delay(500);
    } 
    Serial << F("OK\r\n");

    // setup IP or start DHCP server
    #ifdef IPADDR
      Fishino.config(ip, gw, nm);
    #else
      Fishino.staStartDHCP();
    #endif

    // wait for connection completion
    Serial << F("Waiting for IP...");
    while(Fishino.status() != STATION_GOT_IP)
    {
      Serial << ".";
      delay(500);
    }
    Serial << F("OK\r\n");

  #endif
  
  wsServer.registerConnectCallback(&onConnect);
  wsServer.registerDataCallback(&onData);
  wsServer.registerDisconnectCallback(&onDisconnect);  
  wsServer.begin();
  
  delay(100); 
}

void loop() {
  // Should be called for each loop.
  wsServer.listen();
  
  // Do other stuff here, but don't hang or cause long delays.
  delay(100);
  if (wsServer.connectionCount() > 0) {
    wsServer.send("abc123", 6);
  }
}