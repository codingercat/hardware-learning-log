#include <DHT.h>

#define DHTPIN 8
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);

int buttonPin = 9;
int playPin = 10;

void setup() {
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(playPin, OUTPUT);

  digitalWrite(playPin, LOW);

  Serial.begin(9600);
  dht.begin();
}

void loop() {

  if (digitalRead(buttonPin) == LOW) {

    Serial.println("Button Pressed!");

    delay(1000);  // allow stable reading

    float temp = dht.readTemperature();

    if (isnan(temp)) {
      Serial.println("Sensor error");
      return;
    }

    Serial.print("Temperature: ");
    Serial.println(temp);

    // Trigger audio
    playAudio();

    delay(3000); // avoid repeat
  }
}

void playAudio() {
  delay(500);  // small pause for accessibility

  digitalWrite(playPin, HIGH);
  delay(400);   // trigger pulse
  digitalWrite(playPin, LOW);
}