 #include <Servo.h>
int servoPin = 9;
int servoPos=0;
int lightPin = A0;
int lightVal;
int delayT;
Servo myServo;

void setup() {
  // put your setup code here, to run once:
Serial.begin(9600);
myServo.attach(servoPin);
pinMode(A0, INPUT);
}

void loop() {
  // put your main code here, to run repeatedly:
  lightVal = analogRead(A0);
  delayT = map(lightVal, 0, 1023, 50, 1000);

  Serial.println(lightVal);
  Serial.println(delayT);
  if (delayT > 900) {
    servoPos = 180;
  }
  else if (delayT > 700) {
    servoPos = 100;
  }
  else {
    servoPos = 0;
  }

  myServo.write(servoPos);
  delay(delayT);
}
