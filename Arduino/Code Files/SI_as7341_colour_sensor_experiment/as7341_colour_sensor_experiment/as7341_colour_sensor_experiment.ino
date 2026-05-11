#include <Adafruit_Sensor.h>

// ═══════════════════════════════════════════════════
// TALKING SPECTRAL COLOUR LAB
// AS7341 + ESP32 TTS Kit + Arduino Mega 2560
// ═══════════════════════════════════════════════════

#include <Wire.h>
#include <Adafruit_AS7341.h>

const int BTN_A  = 2;   // Mode A: object colour
const int BTN_B  = 3;   // Mode B: liquid monitor
const int BTN_C  = 4;   // Mode C: filter absorption
const int BUZZER = 5;   // Optional buzzer

// Change 9600 to match your ESP32 TTS Kit baud rate
#define TTS_BAUD 9600

Adafruit_AS7341 as7341;
uint16_t baseline[8];
bool baselineTaken = false;

void speak(const char* text);
void beep();

// ════════════════════════════════════════════════════
void setup() {
  Serial.begin(115200);
  Serial1.begin(TTS_BAUD);  // TX1 (pin 18) → TTS board RX

  pinMode(BTN_A,  INPUT_PULLUP);
  pinMode(BTN_B,  INPUT_PULLUP);
  pinMode(BTN_C,  INPUT_PULLUP);
  pinMode(BUZZER, OUTPUT);

  if (!as7341.begin()) {
    Serial.println("AS7341 not found! Check 3.3V and pins 20/21.");
    while(1) { digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN)); delay(300); }
  }

  // GAIN_256X = good for typical indoor light
  // Reduce to GAIN_64X if all channels show 65535 (saturated)
  as7341.setATIME(100);
  as7341.setASTEP(999);
  as7341.setGain(AS7341_GAIN_16X);

  speak("Colour lab ready. Button one for object. Button two for liquid. Button three for filter.");
  Serial.println("Ready.");
}

void loop() {
  if (digitalRead(BTN_A) == LOW) { delay(50); if(digitalRead(BTN_A)==LOW) modeA(); }
  if (digitalRead(BTN_B) == LOW) { delay(50); if(digitalRead(BTN_B)==LOW) modeB(); }
  if (digitalRead(BTN_C) == LOW) { delay(50); if(digitalRead(BTN_C)==LOW) modeC(); }
  delay(100);
}

// ════════════════════════════════════════════════════
// MODE A: identify dominant colour of object
// ════════════════════════════════════════════════════
void modeA() {
  beep();
  speak("Hold sensor above object. Reading in 2 seconds.");
  delay(2000);

  // Take 5 readings and average them — reduces noise
  uint32_t avg[8] = {0};
  for (int s = 0; s < 5; s++) {
    uint16_t r[12];
    if (!as7341.readAllChannels(r)) { speak("Sensor error."); return; }
    for (int i = 0; i < 8; i++) avg[i] += r[i];
    delay(100);
  }
  for (int i = 0; i < 8; i++) avg[i] /= 5;

  // Print raw averaged values for calibration
  Serial.print("Averaged channels: ");
  for (int i = 0; i < 8; i++) { Serial.print(avg[i]); Serial.print(" "); }
  Serial.println();

  // Find total light level
  uint32_t total = 0;
  for (int i = 0; i < 8; i++) total += avg[i];

  if (total < 2000) {
    speak("Too dark. Move sensor closer or improve lighting.");
    return;
  }

  // Calculate percentage contribution of each channel
  float pct[8];
  for (int i = 0; i < 8; i++) pct[i] = (float)avg[i] / total * 100.0;

  // Check white/grey — all channels roughly equal
  float maxPct = 0, minPct = 100;
  int peakCh = 0;
  for (int i = 0; i < 8; i++) {
    if (pct[i] > maxPct) { maxPct = pct[i]; peakCh = i; }
    if (pct[i] < minPct) minPct = pct[i];
  }

  char msg[80];

  if ((maxPct - minPct) < 4.0) {
    speak("This object appears white or grey.");
    return;
  }

  // Group channels into colour families using ratios
  float violet_blue = pct[0] + pct[1] + pct[2];  // F1+F2+F3
  float green_cyan  = pct[3] + pct[4];            // F4+F5
  float yellow_red  = pct[5] + pct[6] + pct[7];  // F6+F7+F8

  const char* colour;

  if (yellow_red > 45 && pct[7] > pct[5]) colour = "red";
  else if (yellow_red > 45 && pct[5] > pct[7]) colour = "yellow or orange";
  else if (yellow_red > 40 && pct[6] > 15)     colour = "orange";
  else if (green_cyan > 40 && pct[4] > pct[3]) colour = "green";
  else if (green_cyan > 35 && pct[3] > pct[4]) colour = "cyan";
  else if (violet_blue > 40 && pct[2] > 15)    colour = "blue";
  else if (violet_blue > 35 && pct[1] > pct[0]) colour = "indigo";
  else if (violet_blue > 30 && pct[0] > pct[1]) colour = "violet";
  else colour = "mixed or uncertain";

  sprintf(msg, "Dominant colour is %s.", colour);
  speak(msg);
}

// ════════════════════════════════════════════════════
// MODE B: continuous liquid colour change monitor
// Press button 2 again to stop
// ════════════════════════════════════════════════════
void modeB() {
  beep();
  speak("Liquid monitor on. I will announce colour changes. Press button two to stop.");
  delay(1500);

  int lastPeak = -1;
  while (digitalRead(BTN_B) == HIGH) {
    uint16_t r[12];
    if (!as7341.readAllChannels(r)) { delay(500); continue; }

    int peak=0; uint16_t pv=0;
    for(int i=0;i<8;i++) if(r[i]>pv){pv=r[i];peak=i;}

    if (peak != lastPeak && pv > 500) {
      const char* cols[] = {"violet","indigo","blue","cyan",
                             "green","yellow","orange","red"};
      char msg[60];
      sprintf(msg, "Colour shifting to %s.", cols[peak]);
      speak(msg);
      lastPeak = peak;
    }
    delay(800);  // Check every 800ms — increase to 2000 if too fast
  }
  speak("Liquid monitor stopped.");
  delay(500);
}

// ════════════════════════════════════════════════════
// MODE C: compare coloured filter vs white light baseline
// First press = take baseline; subsequent presses = test filter
// ════════════════════════════════════════════════════
void modeC() {
  beep();

  if (!baselineTaken) {
    speak("Remove all filters. Point sensor at white light. Press button three again.");
    while(digitalRead(BTN_C)==HIGH) delay(10);
    delay(100);
    uint16_t raw[12];
    as7341.readAllChannels(raw);
    for(int i=0;i<8;i++) baseline[i]=raw[i];
    baselineTaken = true;
    speak("Baseline saved. Place colour filter over sensor and press button three.");
    return;
  }

  speak("Reading filter. Hold steady.");
  delay(1500);

  uint16_t f[12];
  as7341.readAllChannels(f);

  int absCh=0, passCh=0;
  float maxDrop=0, maxPass=0;
  for(int i=0;i<8;i++) {
    if(baseline[i]==0) continue;
    float drop = (float)(baseline[i]-f[i])/baseline[i];
    float pass = (float)f[i]/baseline[i];
    if(drop>maxDrop){maxDrop=drop;absCh=i;}
    if(pass>maxPass){maxPass=pass;passCh=i;}
  }

  const char* nm[] = {"violet","indigo","blue","cyan","green","yellow","orange","red"};
  char msg[140];
  sprintf(msg,
    "Filter absorbs %s most, by %d percent. Transmits %s most, at %d percent of baseline.",
    nm[absCh], (int)(maxDrop*100), nm[passCh], (int)(maxPass*100));
  speak(msg);
  speak("Press button three to test another filter. Restart to reset baseline.");
}

// ════════════════════════════════════════════════════
// SPEAK: sends text to ESP32 TTS Kit over Serial1
// ════════════════════════════════════════════════════
void speak(const char* text) {
  Serial.print("[TTS] "); Serial.println(text);  // Echo to Serial Monitor

  // Option 1 — plain text + newline (try this first):
  Serial1.println(text);

  // Option 2 — AT command format (uncomment if Option 1 is silent):
  // Serial1.print("AT+TTS="); Serial1.println(text);

  delay(strlen(text) * 80);  // Wait for speech to finish
}

void beep() { tone(BUZZER, 1000, 120); delay(150); }