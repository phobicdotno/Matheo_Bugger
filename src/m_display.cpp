// src/m_display.cpp
#include "m_display.h"
#include <WiFi.h>

MD_Parola display=MD_Parola(HARDWARE_TYPE,CS_PIN,MAX_DEVICES);
bool displayOn=true;
String currentText;
bool messageConfirmed=false;
bool scrolledOnce=false;

void setupDisplay(){
  display.begin();
  display.setZone(0,0,MAX_DEVICES-1);
  display.setZoneEffect(0,true,PA_FLIP_UD);
  display.setZoneEffect(0,true,PA_FLIP_LR);
  display.setIntensity(5);
  display.displayClear();
}

void handleDisplayLoop(){
  if(!displayOn)return;
  if(display.displayAnimate()){
    if(!scrolledOnce){
      display.displayScroll(currentText.c_str(),PA_RIGHT,PA_SCROLL_RIGHT,75);
      scrolledOnce=true;
    }
  }
}

void displayBlinkText(const char *txt){
  display.displayClear();
  display.displayText(txt,PA_CENTER,0,0,PA_PRINT,PA_NO_EFFECT);
  display.displayAnimate();
  for(int i=0;i<3;i++){
    for(int d=15;d>=2;d--){display.setIntensity(d);delay(30);}
    for(int d=2;d<=15;d++){display.setIntensity(d);delay(30);}
  }
  display.displayClear();display.setIntensity(5);
}

void handleButton(){
  static unsigned long start=0;
  static bool prev=false,longDone=false;
  bool p=digitalRead(BUTTON_PIN)==LOW;
  if(p&&!prev){start=millis(); longDone=false;}
  if(p&&!longDone && millis()-start>=5000){
    IPAddress ip=WiFi.localIP();
    currentText=ip.toString();
    displayOn=true; scrolledOnce=false;
    display.displayClear();
    display.displayScroll(currentText.c_str(),PA_RIGHT,PA_SCROLL_RIGHT,75);
    longDone=true;
  }
  if(!p&&prev&&!longDone){
    messageConfirmed=true;
    displayBlinkText("OK");
    display.displayClear();
    displayOn=false; scrolledOnce=true;
  }
  prev=p;
}
