#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include "hdc1080.h"
#include <WiFiUdp.h>
#include <EEPROM.h>


/* Define for Debug Infomation, 1 for use, 0 for not use */
#define   USE_DEBUG_IN_PROGRAM  0

/* Define Device ID */
#define   DEVICE_ID             "ESP1"


/* Define MQTT connection information */
const char* mqtt_server = "m13.cloudmqtt.com";
const int   mqtt_port =   15571;
const char* mqtt_user = "thiepnx";
const char* mqtt_password = "thiepnx";


/* Define Sleep Time (in Seconds) */
const unsigned char SLEEP_IN_SECONDS = 30;

/* Define for Status Led */
#define     LED_STATUS_OK               14
#define     LED_STATUS_ERROR            12

/* Declaring Global Variables */
WiFiClient espClient;
PubSubClient client(espClient);
hdc1080 myhdc1080;

/* Variables store Wifi information write to EEPROM */
char *ssid;
char *pass;
unsigned char ssid_length,pass_length,count_connect=0;

/* Variables store Wifi information read from EEPROM */
char  ssid_eeprom[32];
char  pass_eeprom[64];
unsigned char ssid_eeprom_length, pass_eeprom_length;



float temp;               //Store temperature read from HDC1080
unsigned char humi;       //Store humidity read from HDC1080
char msg[50];             //Store message to publish to broker
char* float_buf;
IPAddress ip;             //Class store ip address
char wifi_ip[16];         //Array store ip address
char MQTT_Client_ID[30];  //Array store MQTT client id

/* Declaring Function Prototype */

void setup_wifi(void);        /* Set up wifi function */
char* f2s(float f, int p);   /* Convert float number to ASCII array with precision */

#if(USE_DEBUG_IN_PROGRAM==1)
/* Call back function, when receive message from broker */
void callback(char* topic, byte* payload, unsigned int length); 
#endif

void reconnect(void); /* Reconnect function, when this client is disconnect to broker */


void setup(void)
{
  /* Config GPIO for LED Status */
  digitalWrite(LED_STATUS_OK,LOW);
  digitalWrite(LED_STATUS_ERROR,LOW);
  
  pinMode(LED_STATUS_OK,OUTPUT);
  pinMode(LED_STATUS_ERROR,OUTPUT);

  
  #if(USE_DEBUG_IN_PROGRAM==1)
  Serial.begin(115200);
  #endif

  setup_wifi();
  
  client.setServer(mqtt_server, mqtt_port);
  #if(USE_DEBUG_IN_PROGRAM==1)
  client.setCallback(callback);
  #endif
  myhdc1080.Init(Temperature_Resolution_14_bit,Humidity_Resolution_14_bit);
}

void loop()
{

  if (!client.connected())
  {
    reconnect();
  }
  client.loop();

  myhdc1080.Start_measurement(&temp,&humi);

  #if(USE_DEBUG_IN_PROGRAM==1)
  Serial.print("[Debug]Temp = ");
  Serial.print(temp);
  Serial.print("\tHumi = ");
  Serial.println(humi);
  #endif

  float_buf = f2s(temp,2);
  sprintf(msg,"%s,%s,%d",DEVICE_ID,float_buf,humi);
  
  /* Publish message with retain flag = true */
  client.publish("outTopic", msg,true);
  
  #if(USE_DEBUG_IN_PROGRAM==1)
  Serial.print("[Debug]Send Message: ");
  Serial.println(msg);
  #endif

 /*  @Brief:  Put chip enter deep sleep mode to reduce power consumption
  *   
  *  @Para :  time deep sleep in microsecond(uS). After that chip auto wake up    
  *  
  *  @Note :  +   Require GPIO16 pin CONNECT to RST for auto wake up.
  *   
  *           +   But on the time programming for chip require GPIO16 pin is DISCONNECT
  *               from RST pin.
  *            
  *           +   If power on when GPIO16 pin connected to RST pin, require pulse on RST pin.
  *               Normaly in this case user must be push RST button.
  */
  ESP.deepSleep(SLEEP_IN_SECONDS*1000000);

}



void setup_wifi(void)
{
  EEPROM.begin(96);

  /**** Read wifi information from EEPROM 
   **** Wifi Information Frame is:
   **** 1 byte ssid length + n bytes ssid + 1 byte pass length + m bytes pass 
   ****/
  
  ssid_eeprom_length = EEPROM.read(0); 
  for(unsigned char i = 1; i <= ssid_eeprom_length; i++)
  {
   *(ssid_eeprom + i -1)= EEPROM.read(i); 
  }
  
  pass_eeprom_length = EEPROM.read(ssid_eeprom_length + 1);
  for(unsigned char i = ssid_eeprom_length + 2; i <= ssid_eeprom_length + 2 + pass_eeprom_length; i++)
  {
   *(pass_eeprom + i - ssid_eeprom_length - 2)= EEPROM.read(i); 
  }

  #if(USE_DEBUG_IN_PROGRAM==1)
  Serial.write("");
  Serial.write("SSID EEPROM: ");
  Serial.write(ssid_eeprom);

  Serial.println("");
  Serial.print("SSID EEPROM LENGTH: ");
  Serial.print(strlen(ssid_eeprom));
  
  Serial.println("");
  Serial.print("PASS EEPROM: ");
  Serial.write(pass_eeprom);
  
  Serial.println("");
  Serial.print("PASS EEPROM LENGTH: ");
  Serial.println(strlen(pass_eeprom));

  Serial.println("Connecting");
  #endif
  
  /* Try connect to WIFI with information read from EEPROM */
  WiFi.begin(ssid,pass);
  while(WiFi.status() != WL_CONNECTED)
  {
    count_connect++;
    
    #if(USE_DEBUG_IN_PROGRAM==1)
    Serial.print(".");
    #endif
    
    delay(500);
    if(count_connect>20)
    {
      break;
    }
  }
  if(WiFi.status() == WL_CONNECTED)
  {
    #if(USE_DEBUG_IN_PROGRAM==1)
    Serial.write("Connected to Wifi\r\n");
    #endif
  
    /* Turn on Green Led, indicate ESP8266 is connected */
    digitalWrite(LED_STATUS_OK,HIGH);
  }
  else
  {
    /* Turn on Red Led, indicate ESP8266 is not connected */
    digitalWrite(LED_STATUS_ERROR,HIGH);
    
    /* User must be use smart phone to config wifi information */
    
    WiFi.disconnect();
    /* Set ESP8266 in STA mode */
    WiFi.mode(WIFI_STA);

    #if(USE_DEBUG_IN_PROGRAM==1)
    Serial.println("");
    Serial.println("Start SmartConfig");
    #endif

    WiFi.beginSmartConfig();
    while(1)
    {
       delay(500);

       #if(USE_DEBUG_IN_PROGRAM==1)
       printf(".");
       #endif 
       
       if(WiFi.smartConfigDone())
       {
          #if(USE_DEBUG_IN_PROGRAM==1)
          Serial.println("SmartConfig Success");
          #endif 
          
          /* Save Wifi information to EEPROM */
          
          /* Note: WiFi.Get_SSID() and WiFi.Get_PassWord() are custom function 
           * please implement two function before compile this sketch.
           * ESP8266WiFi.h and ESP8266WiFi.cpp must be modified. 
           * Normaly two files are locate in
           * C:\Users\"User_Name"\AppData\Local\Arduino15\packages\esp8266\hardware\esp8266\2.3.0\libraries\ESP8266WiFi\src
           * 
           * 
           * 
           * In ESP8266WiFi.h:
           * +  Implement two code lines after "public:" keyword 
           *                                                 " 
           *                                                    const char* Get_PassWord(void); 
           *                                                    const char* Get_SSID(void); 
           *                                                                                   "                                                                                
           * +  Save  "ESP8266WiFi.h"
           *                                                 
           *                                                 
           * In ESP8266WiFi.cpp:                                               
           * +  Implement two method after void ESP8266WiFiClass::printDiag(Print& p) method 
           *                                            "   
           *                                               const char* ESP8266WiFiClass::Get_PassWord(void)
           *                                               {
           *                                                   struct station_config conf;
           *                                                   wifi_station_get_config(&conf);
           *                                               
           *                                                   return reinterpret_cast<const char*>(conf.password);
           *                                               }
           *                                               
           *                                               const char* ESP8266WiFiClass::Get_SSID(void)
           *                                               {
           *                                                   struct station_config conf;
           *                                                   wifi_station_get_config(&conf);
           *                                                   
           *                                                   return reinterpret_cast<const char*>(conf.ssid);
           *                                               }
           *                                                                                                    "
           * +  Save "ESP8266WiFi.cpp"                                                                                                   
           */
           
          ssid = (char*)WiFi.Get_SSID();
          pass = (char*)WiFi.Get_PassWord();
          
          ssid_length = strlen(ssid);
          pass_length = strlen(pass);
          
          EEPROM.write(0,ssid_length);
          for(unsigned char i = 1; i <= ssid_length; i++)
          {
            EEPROM.write(i,*(ssid + i- 1));
          }
        
          EEPROM.write(ssid_length + 1,pass_length);
          for(unsigned char i = ssid_length + 2; i <= ssid_length + pass_length + 2 ; i++)
          {
            EEPROM.write(i,*(pass + i - ssid_length - 2));
          }
          EEPROM.commit();
  
          #if(USE_DEBUG_IN_PROGRAM==1)
          Serial.write("\r\n");
          Serial.write("Smart Config Pass:");
          Serial.write(pass);
          
          Serial.write("\r\n");
          Serial.write("Smart Config SSID:");
          Serial.write(ssid);
          #endif
          digitalWrite(LED_STATUS_ERROR,LOW);
          digitalWrite(LED_STATUS_OK,HIGH);
          break;
       }
    }
  }

  ip = WiFi.localIP();
  sprintf(wifi_ip,"%u.%u.%u.%u",ip[0],ip[1],ip[2],ip[3]);
  
  #if(USE_DEBUG_IN_PROGRAM==1) 
  Serial.print("IP address: ");
  Serial.println(ip);
  #endif
}

char* f2s(float f, int p)
{
  char * pBuff;                         // use to remember which part of the buffer to use for dtostrf
  const int iSize = 10;                 // number of bufffers, one for each float before wrapping around
  static char sBuff[iSize][20];         // space for 20 characters including NULL terminator for each float
  static int iCount = 0;                // keep a tab of next place in sBuff to use
  pBuff = sBuff[iCount];                // use this buffer
  if(iCount >= iSize -1)
  {               // check for wrap
    iCount = 0;                         // if wrapping start again and reset
  }
  else{
    iCount++;                           // advance the counter
  }
  return dtostrf(f, 0, p, pBuff);       // call the library function
}

#if(USE_DEBUG_IN_PROGRAM==1)
void callback(char* topic, byte* payload, unsigned int length)
{
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++)
  {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}
#endif

void reconnect(void)
{
  // Loop until we're reconnected
  while (!client.connected()) 
  {
    sprintf(MQTT_Client_ID,"ESP%s",wifi_ip);

    #if(USE_DEBUG_IN_PROGRAM==1)
    Serial.println("Attempting MQTT connection...");
    Serial.print("Own Client ID =");
    Serial.println(MQTT_Client_ID);
    #endif

    // Attempt to connect
    if (client.connect(MQTT_Client_ID,mqtt_user,mqtt_password))
    {
      #if(USE_DEBUG_IN_PROGRAM==1)
      
      Serial.println("connected");      
      // Once connected, publish an announcement...
      client.publish("outTopic", "hello world");
      // ... and resubscribe
      client.subscribe("outTopic");
      
      #endif
    } else 
    {
      #if(USE_DEBUG_IN_PROGRAM==1)
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 2 seconds");
      #endif
      // Wait 2 seconds before retrying
      delay(2000);
    }
  }
}
