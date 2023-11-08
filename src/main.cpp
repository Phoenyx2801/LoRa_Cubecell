#include "LoRaWanMinimal_APP.h"
#include "Arduino.h"
#include "DHT.h"
/*
 * set LoraWan_RGB to Active,the RGB active in loraWan
 * RGB red means sending;
 * RGB purple means joined done;
 * RGB blue means RxWindow1;
 * RGB yellow means RxWindow2;
 * RGB green means received done;
 */

#define DHTPIN GPIO1      // Digital pin connected to the DHT sensor
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);

//Set these OTAA parameters to match your app/node in TTN
static uint8_t devEui[] = {0x70, 0xB3, 0xD5, 0x7E, 0xD0, 0x06, 0x24, 0x58};
static uint8_t appEui[] = { 0x00, 0x20, 0xC0, 0x0F, 0x70, 0x0F, 0x01, 0x00 };
static uint8_t appKey[] = { 0x01, 0x8A, 0x0A, 0x2B, 0xF6, 0x95, 0xA0, 0x45, 0xA7, 0xA2, 0xF8, 0x83, 0xF7, 0xBF, 0x2C, 0xF8};

uint16_t userChannelsMask[6]={ 0x00FF,0x0000,0x0000,0x0000,0x0000,0x0000 }; 

/////////////////////////////////////////////////// 
TimerEvent_t sleepTimer; 
bool sleepTimerExpired;

byte data[8];

static void wakeUp()
{
  sleepTimerExpired=true;
}

static void lowPowerSleep(uint32_t sleeptime)
{
  sleepTimerExpired=false;
  TimerInit( &sleepTimer, &wakeUp );
  TimerSetValue( &sleepTimer, sleeptime );
  TimerStart( &sleepTimer ); 
  while (!sleepTimerExpired) lowPowerHandler();
  TimerStop( &sleepTimer );
}

///////////////////////////////////////////////////
void setup() {
  Serial.begin(115200); 
  dht.begin();
 
  LoRaWAN.begin(LORAWAN_CLASS, ACTIVE_REGION); 
  LoRaWAN.setAdaptiveDR(true); 
  
  while (1) {
    Serial.println("Joining... ");
    LoRaWAN.joinOTAA(appEui, appKey, devEui);
    if (!LoRaWAN.isJoined()) { 
      Serial.println("JOIN FAILED! Sleeping for 30 seconds");
      lowPowerSleep(30000);
    } else {
      Serial.println("JOINED");
      break;
    }
  }
}

///////////////////////////////////////////////////
void loop()
{ 
  int h = (int)dht.readHumidity();
  int t = (int)dht.readTemperature(); 
  Serial.println("Waiting 10 seconds for data"); 
  lowPowerSleep(10000); 

  int2Bytes(data,t);
  int2Bytes(data+4,h);
  
  if (LoRaWAN.send(8, data, 1, true)) {
    Serial.println("Send OK");
  } else {
    Serial.println("Send FAILED");
  }
}

///////////////////////////////////////////////////
//Example of handling downlink data
void downLinkDataHandle(McpsIndication_t *mcpsIndication)
{
  Serial.printf("Received downlink: %s, RXSIZE %d, PORT %d, DATA: ",mcpsIndication->RxSlot?"RXWIN2":"RXWIN1",mcpsIndication->BufferSize,mcpsIndication->Port);
  for(uint8_t i=0;i<mcpsIndication->BufferSize;i++) {
    Serial.printf("%02X",mcpsIndication->Buffer[i]);
  }
  Serial.println();
}

void int2Bytes(byte bytes_temp[4],int data){ 
  memcpy(bytes_temp, (unsigned char*) (&data), 4);
}
