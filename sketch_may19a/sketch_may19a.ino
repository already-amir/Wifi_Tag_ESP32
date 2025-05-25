
#include <WiFi.h>
#include<WebServer.h>
#include <Preferences.h>

#define  led_pin 2


#define ws WebServer



Preferences pr;
//ws server(80);

String e_ssid= "esp32";
String e_pass= "123456789";
String admnpass= "1234";

string w_ssid=

String ip="";
long long logintime=0;
long long session_timeout=600000;

enum mod{idle,on,off};

mod led1=idle;








void setup() {

  

  Serial.begin(115200);

  pinMode(led_pin, OUTPUT);
  digitalWrite(led_pin,0);

  pr.begin("esp",false);

  e_pass=pr.getString("e_pass",e_pass);


  unsigned long start=millis();










  Serial.println(e_ssid);
  pr.begin("esp_config",false);

  pr.putString("ssid","12345");

  e_ssid=pr.getString("ssid");

  Serial.println(e_ssid);


  pr.end();



  



  



}

void loop() {
  // put your main code here, to run repeatedly:

}
