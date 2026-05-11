
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESP8266SAM_ES.h>
#include <AudioOutputI2S.h>

/* ================= WIFI ================= */
const char* ssid     = "ST";      
const char* password = "11223344";   

/* ================= I2S PINS ================= */
#define I2S_BCLK 15
#define I2S_LRC  2
#define I2S_DOUT 3

/* ================= GLOBALS ================= */
AsyncWebServer server(80);
volatile bool speakNow = false;
String textBuffer;

/* ================= AUDIO ================= */
AudioOutputI2S audio;
ESP8266SAM_ES sam;

/* ================= WEB PAGE ================= */
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>ESP32 TTS</title>
<style>
body{font-family:Arial;text-align:center;max-width:420px;margin:auto;}
textarea{width:90%;height:80px;}
button{padding:14px 18px;font-size:16px;}
</style>
</head>
<body>
<h2>ESP32 Text to Speech</h2>
<textarea id="txt" placeholder="Type short sentence..."></textarea><br><br>
<button onclick="send()">Speak</button>
<script>
function send(){
  let t=document.getElementById("txt").value;
  fetch("/speak?text="+encodeURIComponent(t));
}
</script>
</body></html>
)rawliteral";

/* ================= SETUP ================= */
void setup() {
  // Serial0 = USB debug + receives text from Mega
  Serial.begin(115200);
  delay(1000);

  /* WiFi */
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected. IP: ");
  Serial.println(WiFi.localIP());

  /* Audio I2S */
  audio.SetPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  audio.SetOutputModeMono(true);
  audio.SetGain(0.35);
  audio.begin();

  /* Web Server */
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html);
  });

  server.on("/speak", HTTP_GET, [](AsyncWebServerRequest *request){
    if (request->hasParam("text")) {
      textBuffer = request->getParam("text")->value();
      textBuffer.replace("\n", " ");
      textBuffer.replace("\r", " ");
      if (textBuffer.length() > 140)
        textBuffer = textBuffer.substring(0, 140);
      speakNow = true;
    }
    request->send(200, "text/plain", "OK");
  });

  server.begin();
  Serial.println("Web server started. Ready for serial input.");
}

/* ================= LOOP ================= */
void loop() {

  // ── NEW: listen for text from Arduino Mega over Serial ──
  if (Serial.available()) {
    String incoming = Serial.readStringUntil('\n');
    incoming.trim();
    if (incoming.length() > 0) {
      // Limit to 140 chars to avoid distortion
      if (incoming.length() > 140)
        incoming = incoming.substring(0, 140);
      textBuffer = incoming;
      speakNow = true;
      Serial.println("Serial trigger: " + textBuffer);
    }
  }

  // ── Speak when triggered (web or serial) ──
  if (speakNow) {
    speakNow = false;
    Serial.println("Speaking: " + textBuffer);
    sam.Say(&audio, textBuffer.c_str());
    delay(400);
  }
}