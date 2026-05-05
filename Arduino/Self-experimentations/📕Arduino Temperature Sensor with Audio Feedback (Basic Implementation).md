
### 🎯 Objective

To build a simple interactive system where a user presses a button to measure temperature and receives audio feedback, designed with accessibility in mind for visually impaired students.

---

### 🧩 Components Used

- Arduino Mega 2560
- DHT11 / Grove Temperature & Humidity Sensor
- ISD1820 Voice Recorder Module with speaker
- Push Button
- Breadboard and jumper wires

---

### 🔌 Circuit Connections

#### 🌡️ Temperature Sensor

- VCC → 5V
- GND → GND
- DATA / SIG → Pin 8

#### 🔘 Push Button

- One leg → Pin 9
- Other leg → GND
- Uses `INPUT_PULLUP` (no external resistor needed)

#### 🔊 Voice Module

- VCC → 5V
- GND → GND
- PLAYE → Pin 10
- Speaker → SP+ and SP-

---

### 🎙️ Audio Setup

- Record a short message using the onboard **REC button** on the module
- Example message:  
    “Temperature measurement complete.”
- Playback is triggered via Arduino using the PLAYE pin

---

### 💻 Working Logic

1. System waits for button press
2. On press:
    - Temperature is read from sensor
    - Value is displayed on Serial Monitor
    - Audio message is played via voice module

 
```
 Arduino Code: 
 Projects/Arduino/Code Files/SI_temperature-sensor-with-audio-feedback 
```


![[Temperature Sensor + ISD1820 Voice Recorder Module (diagram).png|678]]



