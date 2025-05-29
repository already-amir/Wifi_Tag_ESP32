#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>

sting ssid = "";
string password = "";

WebServer server(80);
const int ledpin1 = 2;
const int ledpin2 = 4;
const int ledpin3 = 5;

Preferences pr;








const char MAIN_page[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <title>IOT Control Panel</title>
  <style>
    body {
      font-family: sans-serif;
      text-align: center;
      padding: 30px;
    }
    button {
      font-size: 20px;
      margin: 5px;
      padding: 10px 20px;
    }
    .led-container {
      display: flex;
      justify-content: center;
      gap: 50px;
      margin-top: 30px;
    }
    .led-box {
      text-align: center;
    }
  </style>
  <script>
    function toggleLED(target, state) {
      fetch("/led?target=" + target + "&state=" + state)
        .then(response => response.text())
        .then(data => {
          document.getElementById("status").textContent = "LED " + target + " state: " + data;
        });
    }
  </script>
</head>
<body>
  <h1>IOT LED Controller</h1>

  <div class="led-container">
    <div class="led-box">
      <h2>LED 1</h2>
      <button onclick="toggleLED(1, 'ON')">ON</button>
      <button onclick="toggleLED(1, 'OFF')">OFF</button>
    </div>
    <div class="led-box">
      <h2>LED 2</h2>
      <button onclick="toggleLED(2, 'ON')">ON</button>
      <button onclick="toggleLED(2, 'OFF')">OFF</button>
    </div>
    <div class="led-box">
      <h2>LED 3</h2>
      <button onclick="toggleLED(3, 'ON')">ON</button>
      <button onclick="toggleLED(3, 'OFF')">OFF</button>
    </div>
  </div>

  <div id="status" style="margin-top: 30px;">LED state: Unknown</div>
</body>
</html>

)rawliteral";

void handleLED() {
  String state = server.arg("state");
  String target = server.arg("target");

  int ledpin;
    if (target == "1") ledpin = ledpin1;
    else if (target == "2") ledpin = ledpin2;
    else if (target=="3") ledpin=ledpin3;
    else {
    server.send(400, "text/plain", "LED undefined");
      return;
  }

  if (state == "ON") {
    digitalWrite(ledpin, HIGH);
    server.send(200, "text/plain", "ON");
  } else if (state == "OFF") {
    digitalWrite(ledpin, LOW);
    server.send(200, "text/plain", "OFF");
  } else {
    server.send(400, "text/plain", "ERROR");
  }
}


void connectwf(string ssid,string password){

    wifi.begin(ssid,password);
    Serial.println("connecting...");
    int tries=1;
    while (wifi.status()!=WL_CONNECTED && tries<=20){
      delay(500);
      Serial.println("trying : "+string(tries)+" ...");

    }

    if (wifi.status()==WL_CONNECTED){

      Serial.println("connected");
     

    }
    else{
      serial.println("faild to connect");
    

    }


}


void setup() {

  Serial.begin(115200);

  pinMode(ledpin1, OUTPUT);
  pinMode(ledpin2, OUTPUT);
  pinMode(ledpin3, OUTPUT);

  pr.begin("wifi",false);
  ssid=pr.getString("ssid","");
  password=pr.getString("pass","");

  if (ssid==""||password==""){

  }
  else{
    
    connectwf(ssid,password);
    if (wifi.status()!=WL_CONNECTED){

    }
  }






  if (ssid == "" || password == "") {
    startSetupMode(); // وارد حالت تنظیمات شود
  } else {
    WiFi.begin(ssid.c_str(), password.c_str());
    Serial.print("Connecting");
    int retries = 0;
    while (WiFi.status() != WL_CONNECTED && retries < 20) {
      delay(500);
      Serial.print(".");
      retries++;

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\nConnected! IP:");
      Serial.println(WiFi.localIP());
  } else {
      Serial.println("\nFailed to connect. Entering setup mode.");
      startSetupMode();
  }


  }

  WiFi.begin(ssid, password);
  Serial.print("CONNECTING ...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("WiFi CONNECTED . IP:");
  Serial.println(WiFi.localIP());

  server.on("/", HTTP_GET, []() {
    server.send_P(200, "text/html", MAIN_page);
  });

  server.on("/led", handleLED);

  server.begin();
  Serial.println("WEB SERVER ON.");
}

void loop() {
  server.handleClient();
}
