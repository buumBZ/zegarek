#include "Arduino.h"
#include <WiFi.h>


WiFiClient client;

String date;

#define NUM_MAX 4
#define LINE_WIDTH 16
#define ROTATE 270

// for NodeMCU 1.0 
#define DIN_PIN 23  // D8
#define CS_PIN  5  // D7
#define CLK_PIN 18  // D6

const int buttonPin = 15;
const int buttonPin2 = 2;// the number of the pushbutton pin
const int ledPin = 4;

#include "max7219.h"
#include "fonts.h"

// =======================================================================
// CHANGE YOUR CONFIG HERE:
// =======================================================================
#define BLYNK_TEMPLATE_ID "TMPL4ByMcQsI3"
#define BLYNK_TEMPLATE_NAME "esp32"
#define BLYNK_AUTH_TOKEN "fdK_vaIIae2LfX6p05KgkCffyi-TtBcW"
const char* ssid     = "";     // SSID of local network
const char* password = "F";   // Password on network
float utcOffset = 2; // Time Zone setting
// =======================================================================

void setup() 
{
  pinMode(ledPin, OUTPUT);
  // initialize the pushbutton pin as an input:
  pinMode(buttonPin, INPUT);
  pinMode(buttonPin2, INPUT);
  digitalWrite(ledPin, LOW);
  Serial.begin(115200);
  initMAX7219();
  sendCmdAll(CMD_SHUTDOWN,1);
 // sendCmdAll(CMD_INTENSITY,10); // Adjust the brightness between 0 and 15
 
  
  Serial.print("Connecting WiFi ");
  WiFi.begin(ssid, password);
  printStringWithShift("Connecting ",16);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); 
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected: "); Serial.println(WiFi.localIP());
}
// =======================================================================
#define MAX_DIGITS 16
byte dig[MAX_DIGITS]={0};
byte digold[MAX_DIGITS]={0};
byte digtrans[MAX_DIGITS]={0};
int updCnt = 0;
int dots = 0;
long dotTime = 0;
long clkTime = 0;
int dx=0;
int dy=0;
byte del=0;
int h,m,s;
long localEpoc = 0;
long localMillisAtUpdate = 0;
int buttonState = 0;
int buttonState2 = 0;
int start = 1;

// =======================================================================
void loop()
{
  digitalWrite(ledPin, LOW);
  if (buttonState == HIGH && start == 1) {
    // turn LED on:
    digitalWrite(ledPin, HIGH);
    delay(2000);
    start = 2;
  } 
  if (buttonState2 == LOW && start == 2) {
    // turn LED on:
    digitalWrite(ledPin, HIGH);
    delay(2000);
    start = 1;
  } 
  if(updCnt<=0) { // every 10 scrolls, ~450s=7.5m
    updCnt = 10;
    Serial.println("Getting data ...");
    printStringWithShift("  Getting data",15);
   
    getTime();
    Serial.println("Data loaded");
    clkTime = millis();
  }
  if(millis()-dotTime > 500) {
    dotTime = millis();
    dots = !dots;
  }
  updateTime();
  showAnimClock();
    
  // Adjusting LED intensity.
  // 12am to 6am, lowest intensity 0
  if ( (h == 0) || ((h >= 1) && (h <= 6)) ) sendCmdAll(CMD_INTENSITY, 0);
  // 6pm to 11pm, intensity = 2
  else if ( (h >=18) && (h <= 23) ) sendCmdAll(CMD_INTENSITY, 2);
  // max brightness during bright daylight
  else sendCmdAll(CMD_INTENSITY, 10);

  buttonState = digitalRead(buttonPin);
  buttonState2 = digitalRead(buttonPin2);

  // check if the pushbutton is pressed. If it is, the buttonState is HIGH:
}

// =======================================================================

void showSimpleClock()
{

}

// =======================================================================

void showAnimClock()
{
  
  byte digPos[4]={1,8,17,25};
  int digHt = 12;
  int num = 4; 
  int i;
  if(del==0) {
    
    del = digHt;
    for(i=0; i<num; i++) digold[i] = dig[i];
    dig[3] = h/10 ? h/10 : 10;
    dig[2] = h%10;
    dig[1] = m/10;
    dig[0] = m%10; 
      if(h/10==2 && h%10>3){
        sendCmdAll(CMD_SHUTDOWN,12);
      }
      if(h/10==0 && h%10<5){
        sendCmdAll(CMD_SHUTDOWN,12);
      }
      if(h/10==0 && h%10==5 && m%10==0){
          Serial.begin(115200);
          initMAX7219();
          sendCmdAll(CMD_SHUTDOWN,1);
          Serial.print("Connecting WiFi ");
          WiFi.begin(ssid, password);
          printStringWithShift("Connecting ",16);
          while (WiFi.status() != WL_CONNECTED) {
            delay(500);
            Serial.print(".");
          }
          Serial.println("");
          Serial.print("Connected: "); Serial.println(WiFi.localIP());
      }
    for(i=0; i<num; i++)  digtrans[i] = (dig[i]==digold[i]) ? 0 : digHt;
  } else
    del--;
  clr();
  for(i=0; i<num; i++) {
    if(digtrans[i]==0) {
      dy=0;
      showDigit(dig[i], digPos[i], dig6x8);
    } else {
      dy = digHt-digtrans[i];
      showDigit(digold[i], digPos[i], dig6x8);
      dy = -digtrans[i];
      showDigit(dig[i], digPos[i], dig6x8);
      digtrans[i]--;
    }
  }
  dy=0;
  setCol(23,dots ? B00100100 : 0);
  refreshAll();
 delay(30);
}

// =======================================================================

void showDigit(char ch, int col, const uint8_t *data) //sprawdzić
{
  int len = pgm_read_byte(data);
  int w = pgm_read_byte(data + 1 + ch * len);
  for (int i = 0; i < w; i++)
    if(col+i>=0 && col+i<8*NUM_MAX) {
      byte v = pgm_read_byte(data + 1 + ch * len + 1 + i);
      if(!dy) scr[col + i] = v; else scr[col + i] |= dy>0 ? v>>dy : v<<-dy;
    }
}

// =======================================================================

void setCol(int col, byte v)
{
  if(dy<-8 | dy>8) return;
  col += dx;
  if(col>=0 && col<8*NUM_MAX)
    if(!dy) scr[col] = v; else scr[col] |= dy>0 ? v>>dy : v<<-dy;
}

// =======================================================================

int showChar(char ch, const uint8_t *data)
{
  
}

// =======================================================================

void printCharWithShift(unsigned char c, int shiftDelay) {
  
}

// =======================================================================

void printStringWithShift(const char* s, int shiftDelay){

}

// =======================================================================



void getTime()
{
  WiFiClient client;
  if (!client.connect("www.google.com", 80)) {
    Serial.println("connection to google failed");
    return;
  }

  client.print(String("GET / HTTP/1.1\r\n") +
               String("Host: www.google.com\r\n") +
               String("Connection: close\r\n\r\n"));
  int repeatCounter = 0;
  while (!client.available() && repeatCounter < 10) {
    delay(500);
    //Serial.println(".");
    repeatCounter++;
    
  }

  String line;
  client.setNoDelay(false);
  while(client.connected() && client.available()) {
    line = client.readStringUntil('\n');
    line.toUpperCase();
    if (line.startsWith("DATE: ")) {
      date = "     "+line.substring(6, 17);
      h = line.substring(23, 25).toInt();
      m = line.substring(26, 28).toInt();
      s = line.substring(29, 31).toInt();
      localMillisAtUpdate = millis();
      localEpoc = (h * 60 * 60 + m * 60 + s);
      
    }
  }
  client.stop();
}

// =======================================================================

void updateTime()
{
  long curEpoch = localEpoc + ((millis() - localMillisAtUpdate) / 1000);
  long epoch = fmod(round(curEpoch + 3600 * utcOffset + 86400L), 86400L);
  h = ((epoch  % 86400L) / 3600) % 24;
  m = (epoch % 3600) / 60;
  s = epoch % 60;
}

// =======================================================================