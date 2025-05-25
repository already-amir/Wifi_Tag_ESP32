#include <WiFi.h>
#include <WebServer.h>

// ——— تنظیمات شبکه مودم ———
const char* ssid     = "n";
const char* password = "123456789a";

// رمز عبور برای صفحه کنترل
String pagePassword = "1234";

// پایه LED
const int ledPin = 2;

// وب‌سرور روی پورت 80
WebServer server(80);

// مودهای چشمک
enum LedMode { IDLE, BLINKING, TURN_OFF };
LedMode mode = IDLE;

// تایمرها
unsigned long blinkEnd      = 0;
unsigned long lastToggle    = 0;
unsigned long wifiLastCheck = 0;
bool ledState               = LOW;

// HTTPSession tracking
String sessionIP       = "";
unsigned long loginTime = 0;
const unsigned long SESSION_TIMEOUT = 600000; // 10 دقیقه

// تایم‌اوت اتصال وای‌فای
const unsigned long WIFI_TIMEOUT = 10000;

// صفحه HTML کنترل LED با طراحی بهبود یافته و امکانات جدید
const char* htmlPage = R"rawliteral(
<!DOCTYPE html>
<html lang="fa">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>کنترل LED با امنیت</title>
  <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
  <style>
    :root {
      --main-bg: #f0f4f8;
      --card-bg: #ffffff;
      --primary: #007BFF;
      --danger: #dc3545;
      --text-dark: #333;
      --shadow: 0 4px 6px rgba(0, 0, 0, 0.1);
    }
    [data-theme="dark"] {
      --main-bg: #1e1e1e;
      --card-bg: #2e2e2e;
      --text-dark: #eee;
      --primary: #66B2FF;
    }
    * { box-sizing: border-box; margin: 0; padding: 0; }
    body {
      font-family: sans-serif;
      background: var(--main-bg);
      color: var(--text-dark);
      display: flex;
      align-items: center;
      justify-content: center;
      min-height: 100vh;
      padding: 1rem;
      transition: background 0.3s, color 0.3s;
    }
    .container { width: 100%; max-width: 360px; }
    .card {
      background: var(--card-bg);
      border-radius: 12px;
      box-shadow: var(--shadow);
      padding: 1.5rem;
      margin-bottom: 1rem;
      text-align: center;
      transition: background 0.3s;
    }
    h2 { margin-bottom: 1rem; font-size: 1.4rem; }
    input, button {
      width: 100%; padding: 0.75rem; margin-top: 0.5rem;
      border-radius: 8px; border: 1px solid #ccc; font-size: 1rem;
      transition: border-color 0.3s, box-shadow 0.3s;
    }
    input:focus, button:focus {
      outline: none; border-color: var(--primary);
      box-shadow: 0 0 0 2px rgba(0,123,255,0.25);
    }
    button {
      background: var(--primary); color: #fff; border: none;
      cursor: pointer; transition: background 0.3s;
    }
    button:hover { background: #0056b3; }
    .btn-danger { background: var(--danger); margin-top: 0.5rem; }
    .btn-danger:hover { background: #c82333; }
    .hidden { display: none; }
    #status { margin-top: 1rem; font-weight: bold; }
    .login-msg { color: var(--danger); margin-top: 0.5rem; font-size: 0.9rem; }
    .theme-switch { position: absolute; top: 1rem; right: 1rem; cursor: pointer; }
    canvas { margin-top: 1rem; }
  </style>
</head>
<body>
  <div class="theme-switch">🌙/☀️</div>
  <div class="container">
    <div class="card" id="login">
      <h2>ورود به سیستم</h2>
      <input type="password" id="pwd" placeholder="رمز عبور">
      <button onclick="doLogin()">ورود</button>
      <p class="login-msg" id="login-msg"></p>
    </div>

    <div class="card hidden" id="control">
      <h2>کنترل LED</h2>
      <button onclick="toggleLED()">تغییر وضعیت LED</button>
      <button class="btn-danger" onclick="doLogout()">خروج</button>
      <p id="status"></p>
      <canvas id="ledChart" width="300" height="200"></canvas>
    
      <h3>تغییر رمز</h3>
        <input type="password" id="oldPwd" placeholder="رمز فعلی">
        <input type="password" id="newPwd" placeholder="رمز جدید">
        <button onclick="changePwd()">اعمال</button>
        <p id="pwdMsg"></p>

    </div>

  </div>

  <script>
    let chart = null;
    function initChart() {
      if (chart) { chart.destroy(); }
      const ctx = document.getElementById('ledChart').getContext('2d');
      chart = new Chart(ctx, {
        type: 'line',
        data: { labels: [], datasets: [{ label: 'LED State', data: [], fill: false, tension: 0.1 }] },
        options: { scales: { y: { min: 0, max: 1, ticks: { stepSize: 1 } } } }
      });
    }
    function addData(state) {
      if (!chart) return;
      const time = new Date().toLocaleTimeString();
      const value = (state === 'ON' || state === 'BLINKING') ? 1 : 0;
      chart.data.labels.push(time);
      chart.data.datasets[0].data.push(value);
      if (chart.data.labels.length > 10) {
        chart.data.labels.shift();
        chart.data.datasets[0].data.shift();
      }
      chart.update();
    }
    function notify(message) {
      if (Notification.permission === 'granted') new Notification(message);
    }
    document.querySelector('.theme-switch').onclick = () => {
      document.documentElement.dataset.theme = document.documentElement.dataset.theme === 'dark' ? '' : 'dark';
    };
    if (Notification.permission !== 'granted') Notification.requestPermission();

    function doLogin() {
      const pwd = document.getElementById('pwd').value;
      fetch(`/login?pwd=${pwd}`)
        .then(r => r.text())
        .then(res => {
          const msg = document.getElementById('login-msg');
          if (res === 'OK') {
            document.getElementById('login').classList.add('hidden');
            document.getElementById('control').classList.remove('hidden');
            msg.textContent = '';
            initChart();
          } else if (res === 'BUSY') msg.textContent = 'کاربر دیگری در حال استفاده است';
          else msg.textContent = 'رمز اشتباه است';
        });
    }
    function doLogout() {
      fetch('/logout').then(() => {
        document.getElementById('control').classList.add('hidden');
        document.getElementById('login').classList.remove('hidden');
        if (chart) { chart.destroy(); chart = null; }
      });
    }
    function toggleLED() {
      fetch('/toggle')
        .then(r => r.text())
        .then(s => {
          document.getElementById('status').textContent = 'وضعیت LED: ' + s;
          addData(s);
          notify('LED ' + s);
        });
    }
    function updateState() {
      fetch('/state')
        .then(r => r.text())
        .then(s => {
          document.getElementById('status').textContent = 'وضعیت LED: ' + s;
          addData(s);
        });
    }
    setInterval(() => {
      if (!document.getElementById('control').classList.contains('hidden')) updateState();
    }, 1000);

    function changePwd() {
      const oldVal = document.getElementById('oldPwd').value;
      const newVal = document.getElementById('newPwd').value;
    fetch('/changePwd', {
      method: 'POST',
      headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
      body: `old=${encodeURIComponent(oldVal)}&new=${encodeURIComponent(newVal)}`
    })
    .then(r => r.text())
    .then(res => {
      const msg = document.getElementById('pwdMsg');
      if (res === 'OK') msg.textContent = 'رمز با موفقیت تغییر کرد';
      else if (res === 'BAD_OLD') msg.textContent = 'رمز فعلی اشتباه است';
      else if (res === 'NEW_TOO_SHORT') msg.textContent = 'رمز جدید کوتاه است';
      else msg.textContent = 'خطا در تغییر رمز';
    });
}

  </script>
</body>
</html>
)rawliteral";

String clientIP() { return server.client().remoteIP().toString(); }

// هندلرها
void handleRoot()        { server.send(200, "text/html; charset=utf-8", htmlPage); }
void handleLogin();
void handleLogout();
void handleToggle();
void handleState();

void setup() {
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);
  prefs.begin("led_ctrl", false);
  pagePassword = prefs.getString("admin_pwd", pagePassword);
  digitalWrite(ledPin, LOW);


  unsigned long start = millis();
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED && millis() - start < WIFI_TIMEOUT) delay(500);
  if (WiFi.status() == WL_CONNECTED) Serial.println(WiFi.localIP());
  else { WiFi.softAP("ESP32-Setup"); Serial.println(WiFi.softAPIP()); }

  server.on("/",       handleRoot);
  server.on("/login",  handleLogin);
  server.on("/logout", handleLogout);
  server.on("/toggle", handleToggle);
  server.on("/state",  handleState);
    server.on("/changePwd", HTTP_POST, handleChangePwd);
  server.begin();
  
}

void loop() {
  server.handleClient();
  unsigned long now = millis();
  switch (mode) {
    case BLINKING:
      if (now >= blinkEnd) { digitalWrite(ledPin, LOW); mode = IDLE; }
      else if (now - lastToggle >= 500) { ledState = !ledState; digitalWrite(ledPin, ledState); lastToggle = now; }
      break;
    case TURN_OFF: digitalWrite(ledPin, LOW); mode = IDLE; break;
    default: break;
  }
  if (WiFi.status() != WL_CONNECTED && now - wifiLastCheck >= 5000) { WiFi.reconnect(); wifiLastCheck = now; }
}

// توابع هندلرها
void handleLogin() { String pwd = server.arg("pwd"); unsigned long now = millis(); if (sessionIP != "" && now - loginTime > SESSION_TIMEOUT) sessionIP = ""; if (pwd != pagePassword) server.send(200, "text/plain; charset=utf-8", "FAIL"); else if (sessionIP == "" || sessionIP == clientIP()) { sessionIP = clientIP(); loginTime = now; server.send(200, "text/plain; charset=utf-8", "OK"); } else server.send(200, "text/plain; charset=utf-8", "BUSY"); }
void handleLogout() { if (clientIP() == sessionIP) sessionIP = ""; server.send(200, "text/plain; charset=utf-8", "BYE"); }
void handleToggle() { if (clientIP() != sessionIP) { server.send(403, "text/plain; charset=utf-8", "DENIED"); return; } if (mode != BLINKING) { mode = BLINKING; blinkEnd = millis() + 10000; lastToggle = millis(); ledState = LOW; server.send(200, "text/plain; charset=utf-8", "BLINKING"); } else { mode = TURN_OFF; server.send(200, "text/plain; charset=utf-8", "OFF"); } }









void handleState() { if (clientIP() != sessionIP) { server.send(403, "text/plain; charset=utf-8", "DENIED"); return; } if (mode == BLINKING) server.send(200, "text/plain; charset=utf-8", "BLINKING"); else server.send(200, "text/plain; charset=utf-8", digitalRead(ledPin) ? "ON" : "OFF"); }


void handleChangePwd() {
  if (clientIP() != sessionIP) {
    server.send(403, "text/plain; charset=utf-8", "DENIED");
    return;
  }
  String oldPwd = server.arg("old");
  String newPwd = server.arg("new");
  if (oldPwd != pagePassword) {
    server.send(400, "text/plain; charset=utf-8", "BAD_OLD");
    return;
  }
  if (newPwd.length() < 4) {
    server.send(400, "text/plain; charset=utf-8", "NEW_TOO_SHORT");
    return;
  }
  pagePassword = newPwd;
  prefs.putString("admin_pwd", pagePassword);
  server.send(200, "text/plain; charset=utf-8", "OK");
}




















