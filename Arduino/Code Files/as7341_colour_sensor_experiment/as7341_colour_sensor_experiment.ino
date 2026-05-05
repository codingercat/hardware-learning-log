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
  as7341.setGain(AS7341_GAIN_256X);

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
  speak("Hold sensor 1 to 3 centimetres above object. Reading in 2 seconds.");
  delay(2000);

  uint16_t r[12];
  if (!as7341.readAllChannels(r)) { speak("Sensor error. Try again."); return; }

  int peak = 0; uint16_t pv = 0;
  for (int i=0;i<8;i++) if(r[i]>pv){pv=r[i];peak=i;}

  if (pv < 500) { speak("Reading too low. Move sensor closer."); return; }

  uint16_t minv = 65535;
  for(int i=0;i<8;i++) if(r[i]<minv) minv=r[i];
  float ratio = (float)minv/pv;

  char msg[80];
  if      (ratio > 0.8) speak("This object appears white or light grey.");
  else if (pv < 1000 && ratio < 0.3) speak("This object appears very dark or black.");
  else {
    const char* cols[] = {"violet","indigo blue","blue","cyan",
                           "green","yellow","orange","red"};
    sprintf(msg, "Dominant colour is %s.", cols[peak]);
    speak(msg);
  }

  // Print raw channel values to Serial Monitor for calibration
  Serial.print("Channels: ");
  for(int i=0;i<8;i++){Serial.print(r[i]);Serial.print(" ");}
  Serial.println();
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