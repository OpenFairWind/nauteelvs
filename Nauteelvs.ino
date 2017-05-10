#include <RTClib.h>
#include "arduinoFFT.h"
#include <Wire.h>
#include <Timer32.h>
#include <string.h>
#include <SD.h>
#include <SPI.h>
#include "Fishino.h"
#include <WebSocket.h>
#include <Arduino.h>


//variabili websocket
#ifndef MYNET_H
// SSID della rete WiFi
#define MY_SSID  "Fishino32"

// PASSWORD of network.
#define MY_PASS ""

// 'IP desiderato per il fishino
#define IPADDR  192, 168, 1, 251
#endif

#ifndef _FISHINO32_
const int SD_CS = 4;
#else
const int SD_CS = SDCS;
#endif

#ifdef IPADDR
IPAddress ip(IPADDR);
#endif

// Enabe debug tracing to Serial port.
#define DEBUG


arduinoFFT FFT = arduinoFFT();
const int samples = 64; //campioni
const double sampling = 200;  //frequenza di campionamento
uint8_t exponent;
double vRealx[samples];
double vImagx[samples];
double vRealy[samples];
double vImagy[samples];
double vRealz[samples];
double vImagz[samples];
double vReala[samples];
double vImaga[samples];
int i = 0;
int j = 0;
double jr[samples];
double ji[samples];
double freq[200/2];

long int ti;
volatile bool intFlag=false;


#define SIZE 8 //numero di FFT calcolate prima di scrivere su SD
const int chipSelect_SD_default = SDCS;
const int chipSelect_SD = chipSelect_SD_default;
String data= "";
#if defined(ARDUINO_ARCH_SAMD)
   #define Serial SerialUSB
#endif
RTC_Millis rtc;
char nome_file[17];
char **data_base;


WebSocketServer wsServer;
uint8_t mac[WL_MAC_ADDR_LENGTH]={0x11,0x22,0x33,0x44,0x55,0x66};


#define SCL_INDEX 0x00
#define SCL_TIME 0x01
#define SCL_FREQUENCY 0x02


#define    MPU9250_ADDRESS            0x68

#define    ACC_FULL_SCALE_2_G        0x00  
#define    ACC_FULL_SCALE_4_G        0x08
#define    ACC_FULL_SCALE_8_G        0x10
#define    ACC_FULL_SCALE_16_G       0x18

#define   a   0
#define   x   1
#define   y   2
#define   z   3


// This function read Nbytes bytes from I2C device at address Address. 
// Put read bytes starting at register Register in the Data array. 
void I2Cread(uint8_t Address, uint8_t Register, uint8_t Nbytes, uint8_t* Data)
{
  // Set register address
  Wire.beginTransmission(Address);
  Wire.write(Register);
  Wire.endTransmission();
  
  // Read Nbytes
  Wire.requestFrom(Address, Nbytes); 
  uint8_t index=0;
  while (Wire.available())
    Data[index++]=Wire.read();
}

// Write a byte (Data) in device (Address) at register (Register)
void I2CwriteByte(uint8_t Address, uint8_t Register, uint8_t Data)
{
  // Set register address
  Wire.beginTransmission(Address);
  Wire.write(Register);
  Wire.write(Data);
  Wire.endTransmission();
}

void callback()
{ 
  intFlag=true;
}


void onConnect(WebSocket &socket) 
{
  Serial.println("onConnect called");
}

// It will be called by the server when a data frame is received.
void onData(WebSocket &socket, char* dataString, byte frameLength) 
{
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


void getFileName(char *name)
{
  DateTime now = rtc.now();
  sprintf(name, "%d", now.year());
  int temp= now.month();
  if(temp > 9)
  {
    sprintf(name+4, "%d", temp);
  }
  else
  {
    name[4]= '0';
    sprintf(name+5, "%d", temp);
  }
  temp= now.day();
  if(temp > 9)
  {
    sprintf(name+6, "%d", temp);
  }
  else
  {
    name[6]= '0';
    sprintf(name+7, "%d", temp);
  }    
  temp= now.hour();
  if(temp > 9)
  {
    sprintf(name+8, "%d", temp);
  }
  else
  {
    name[8]= '0';
    sprintf(name+9, "%d", temp);
  }
  temp= now.minute();
  if(temp > 9)
  {
    sprintf(name+10, "%d", temp);
  }
  else
  {
    name[10]= '0';
    sprintf(name+11, "%d", temp);
  }
  temp= now.second();
  if(temp > 9)
  {
    sprintf(name+12, "%d", temp);
  }
  else
  {
    name[12]= '0';
    sprintf(name+13, "%d", temp);
  }
  name[14]= '.';
  name[15]= 't';
  name[16]= 'x';
  name[17]= 't';
  Serial.println(name);
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


void setup()
{
  Wire.begin();
  #ifdef DEBUG  
    Serial.begin(115200);
  #endif
  //while(!Serial);

  rtc.begin(DateTime(F(__DATE__), F(__TIME__)));  

  Fishino.setApMAC(mac);

  // initialize SPI:
  SPI.begin();
  SPI.setClockDivider(SPI_CLOCK_DIV2);

  // reset and test wifi module
  Serial << F("Resetting Fishino...");
  while(!Fishino.reset())
  {
    Serial << ".";
    delay(500);
  }
  Serial << F("OK\r\n");
  
  // set PHY mode 11G
  Fishino.setPhyMode(PHY_MODE_11G);

  // setup SOFT AP mode
  // imposta la modalitè SOFTAP
  Serial << F("Setting mode SOFTAP_MODE\r\n");
  Fishino.setMode(SOFTAP_MODE);

  // stop AP DHCP server
  Serial << F("Stopping DHCP server\r\n");
  Fishino.softApStopDHCPServer();
  
  // setup access point parameters
  Serial << F("Setting AP IP info\r\n");
  Fishino.setApIPInfo(ip, ip, IPAddress(255, 255, 255, 0));

  Serial << F("Setting AP WiFi parameters\r\n");
  Fishino.softApConfig(F(MY_SSID), F(MY_PASS), 1, false);
  
  // restart DHCP server
  Serial << F("Starting DHCP server\r\n");
  Fishino.softApStartDHCPServer();
  
  // print current IP address
  Serial << F("IP Address :") << ip << "\r\n";

  wsServer.registerConnectCallback(&onConnect);
  wsServer.registerDataCallback(&onData);
  wsServer.registerDisconnectCallback(&onDisconnect);  
  wsServer.begin();

  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect_SD)) {
    Serial.println("Card failed, or not present");
    return;
  }
  Serial.println("card initialized.");
  
  exponent = FFT.Exponent(samples);
  
  // Set accelerometers low pass filter at 60Hz
  I2CwriteByte(MPU9250_ADDRESS,29,0x32);

  // Configure accelerometers range
  I2CwriteByte(MPU9250_ADDRESS,28,ACC_FULL_SCALE_4_G);

  Timer23.setPeriodMs(5);             //Periodo di campionamento => frequenza di campionamento= 200Hz
  Timer23.attachInterrupt(callback);  // attaches callback() as a timer overflow interrupt
  Timer23.start();
  
  // Store initial time
  ti=millis();

  data_base= (char**)malloc(10*sizeof(char*));
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void loop() 
{
  //server in ascolto
  wsServer.listen();
  
  //attesa dati accelerometro
  while (!intFlag);
  
  intFlag=false;

  // Read accelerometer
  uint8_t Buf[14];
  I2Cread(MPU9250_ADDRESS,0x3B,14,Buf);
  
  
  // Accelerometer
  vRealx[i] = -(Buf[0]<<8 | Buf[1]);
  vImagx[i] = 0;
  vRealy[i] = -(Buf[2]<<8 | Buf[3]);
  vImagy[i] = 0;
  vRealz[i] = Buf[4]<<8 | Buf[5];
  vImagz[i] = 0;


  if(i == samples-1)
  {
    for(int j=0; j<samples; j++)
    {
      vReala[j] = sqrt(sq(vRealx[j]) + sq(vRealy[j]) + sq(vRealz[j]));
      vImaga[j] = 0;
    }

    //data x
    PrintVectorjs(vRealx, vImagx, samples, SCL_TIME, x, ti);
    FFT.Windowing(vRealx, samples, FFT_WIN_TYP_HAMMING, FFT_FORWARD);
    FFT.Compute(vRealx, vImagx, samples, exponent, FFT_FORWARD);
    FFT.ComplexToMagnitude(vRealx, vImagx, samples);
    PrintVectorjs(vRealx, vImagx, (samples/2), SCL_FREQUENCY, x, ti);  
      //double x = FFT.MajorPeak(vRealx, samples, sampling);
  
    //data y
    PrintVectorjs(vRealy, vImagy, samples, SCL_TIME, y, ti);
    FFT.Windowing(vRealy, samples, FFT_WIN_TYP_HAMMING, FFT_FORWARD);
    FFT.Compute(vRealy, vImagy, samples, exponent, FFT_FORWARD);
    FFT.ComplexToMagnitude(vRealy, vImagy, samples); 
    PrintVectorjs(vRealy, vImagy, (samples/2), SCL_FREQUENCY, y, ti);  
      //double y = FFT.MajorPeak(vRealy, samples, sampling);

    //data z
    PrintVectorjs(vRealz, vImagz, samples, SCL_TIME, z, ti);
    FFT.Windowing(vRealz, samples, FFT_WIN_TYP_HAMMING, FFT_FORWARD);
    FFT.Compute(vRealz, vImagz, samples, exponent, FFT_FORWARD);  
    FFT.ComplexToMagnitude(vRealz, vImagz, samples);
    PrintVectorjs(vRealz, vImagz, (samples/2), SCL_FREQUENCY, z, ti);      
      //double z = FFT.MajorPeak(vRealz, samples, sampling);

    //data all
    PrintVectorjs(vReala, vImaga, samples, SCL_TIME, a, ti);
    FFT.Windowing(vReala, samples, FFT_WIN_TYP_HAMMING, FFT_FORWARD);
    FFT.Compute(vReala, vImaga, samples, exponent, FFT_FORWARD); 
    FFT.ComplexToMagnitude(vReala, vImaga, samples);
    PrintVectorjs(vReala, vImaga, (samples/2), SCL_FREQUENCY, a, ti);    
      //double a = FFT.MajorPeak(vReala, samples, sampling);

    Serial.println(data);
    char data_server[data.length()+1];
    data.toCharArray(data_server, data.length()+1); 
    data="";
      
    //verifico se ci sono client connessi
    if (wsServer.connectionCount() > 0) 
    {
      wsServer.send(data_server, sizeof(data_server));
      Serial.println("dati inviati");
    }

    data_base[j]= (char*)malloc(sizeof(data_server));
    memmove (data_base[j], data_server, sizeof(data_server));

    if( j == SIZE-1 )
    {
      getFileName(nome_file);
      // open the file on SD
      File dataFile = SD.open(nome_file+6, FILE_WRITE);

      // if the file is available, write to it:
      if (dataFile) 
      {
        for(int i=0; i<SIZE; i++)
        {
          dataFile.println(data_base[i]);
          free(data_base[i]);
        }
      
        dataFile.close();
        // print to the serial port too:
        Serial.println("scrittura ok");
      }
      // if the file isn't open, pop up an error:
      else 
      {
        Serial.println("error opening file");
      } 
      j = 0;
      free(data_base);
      data_base= (char**)malloc(10 * sizeof(char*));
    }
    else j++;
    i = 0;
  }
  else i++;
}

void PrintVectorjs(double *vDataR, double *vDataI, uint8_t bufferSize, uint8_t scaleType, int asseType, long int tempo) 
{  
    double abscissa;
    char buffer[30];
 
    switch (asseType)
    {
      case x:
        if(scaleType == SCL_TIME){
          data+= "{\"millis\":";
          data+= itoa(millis()-tempo, buffer, 10);
          memset(buffer, 0, 30);
          data+= ",\"x\":[";   
        }    
        else  data+= ","; 
      break;
      case y:
        if(scaleType == SCL_TIME){   
          data+= "],\"y\":[";
        }    
        else  data+= ",";
      break;
      case z:
        if(scaleType == SCL_TIME){  
          data+= "],\"z\":[";
        }    
        else  data+= ",";
      break;
      case a:
        if(scaleType == SCL_TIME){  
          data+= "],\"all\":[";
        }    
        else  data+= ","; 
      break;
    }

    switch (scaleType) 
    {
      case SCL_INDEX:
        data+= "{\"components\":[{\"real\": [";
        for (int i = 0; i < bufferSize-1; i++)
        {
              sprintf(buffer, "%f", vDataR[i]);
              data+= buffer;
              memset(buffer, 0, 30);
              data+= ",";
        }
        sprintf(buffer, "%f", vDataR[bufferSize-1]);
        data+= buffer;
        memset(buffer, 0, 30);
        data+= "]},{\"imm\": [";
        for (int i = 0; i < bufferSize-1; i++)
        {
              sprintf(buffer, "%f", vDataI[i]);
              data+= buffer;
              memset(buffer, 0, 30);
              data+= ",";
        }
        sprintf(buffer, "%f", vDataI[bufferSize-1]);
        data+= buffer;       
        memset(buffer, 0, 30);
        data+= "]}]}";
      break;
      case SCL_TIME:
        data+= "{\"signal\":[{\"time\": [";
        for (int i = 0; i < bufferSize-1; i++)
        {
              abscissa = (i / sampling);
              sprintf(buffer, "%f", abscissa);
              data+= buffer;
              memset(buffer, 0, 30);
              data+= ",";
        }
        abscissa = (i / sampling);
        sprintf(buffer, "%f", abscissa);
        data+= buffer;
        memset(buffer, 0, 30);
        data+= "]},{\"real\": [";       
        for (int i = 0; i < bufferSize-1; i++)
        {
              sprintf(buffer, "%f", vDataR[i]);
              data+= buffer;
              memset(buffer, 0, 30);
              data+= ",";
        }
        sprintf(buffer, "%f", vDataR[bufferSize-1]);
        data+= buffer;
        memset(buffer, 0, 30);
        data+= "]}]}";
      break;
      case SCL_FREQUENCY:
        data+= "{\"spectrum\":[{\"f\": [";
        for (int i = 0; i < bufferSize-1; i++)
        {
              abscissa = ((i * sampling) / samples);
              vDataI[i]= abscissa;
              sprintf(buffer, "%f", abscissa);
              data+= buffer; 
              memset(buffer, 0, 30);
              data+= ",";
        }   
        abscissa = ((i * sampling) / samples);               //STAMPA UN 1 IN PIU
        vDataI[i]= abscissa;
        sprintf(buffer, "%f", abscissa);
        data+= buffer;
        memset(buffer, 0, 30); 
        data+= "]},{\"e\": [";                                 
        for (int i = 0; i < bufferSize-1; i++)
        {
              sprintf(buffer, "%f", vDataR[i]);
              data+= buffer;
              memset(buffer, 0, 30);
              data+= ",";
        }
        sprintf(buffer, "%f", vDataR[bufferSize-1]);
        data+= buffer;
        memset(buffer, 0, 30);
        data+= "]}]}";
        if(asseType == a){
          data+= "]}";
        }
      break;
    }
}

