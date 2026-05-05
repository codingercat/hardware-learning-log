int tiltPin =7;
int tiltVal;
int redLED = 5;
int greenLED = 6;

void setup() {
  // put your setup code here, to run once:
pinMode(tiltPin, INPUT);
pinMode(redLED, OUTPUT);
pinMode(greenLED, OUTPUT);
digitalWrite(tiltPin, HIGH);
Serial.begin(9600);

}

void loop() {
  // put your main code here, to run repeatedly:
tiltVal=digitalRead(tiltPin);
Serial.println(tiltVal);
if (tiltVal == 0){
  digitalWrite(greenLED, LOW);
  digitalWrite(redLED, HIGH);
}
else{
  digitalWrite(redLED, LOW);
  digitalWrite(greenLED, HIGH);
}
}
