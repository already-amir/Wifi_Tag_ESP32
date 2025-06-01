
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>

#define led_pin 2


#define ws WebServer



Preferences pr;
ws server(80);

String e_ssid= "esp32";
String e_pass= "123456789";
String admnpass= "1234";



String ip="";
long long logintime=0;
long long session_timeout=600000;

enum mod{idle,on,off};

mod led1=idle;








void setup() {

  

  Serial.begin(115200);

  pinMode(led_pin, OUTPUT);
  digitalWrite(led_pin,0);


  



  



}

void loop() {
  // put your main code here, to run repeatedly:

}
