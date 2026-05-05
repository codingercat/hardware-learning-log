int redPin = 8;
int greenPin = 9;
int bluePin = 10;
String myColor;
String msg = "What color LED do you want?";


void setup() {
  // put your setup code here, to run once:
Serial.begin(9600);
pinMode(redPin, OUTPUT);
pinMode(greenPin, OUTPUT);
pinMode(bluePin, OUTPUT);

}

void loop() {
  // put your main code here, to run repeatedly:
Serial.println(msg);
while(Serial.available() == 0){

}
myColor = Serial.readString();
myColor.trim(); 

if (myColor == "red"){
  setColor(255,0,0);
}

else if (myColor == "green"){
  setColor(0,255,0);
}

else if (myColor == "blue"){
  setColor(0,0,255);
}

else if (myColor == "cyan"){
  setColor(0,255,255);
}

else if (myColor == "magenta"){
  setColor(255,0,255);
}

else if (myColor == "yellow"){
  setColor(255,255,0);
}

else if (myColor == "orange"){
  setColor(255,25,0);
}

else if (myColor == "purple"){
  setColor(150,0,255);
}

else if (myColor == "white"){
  setColor(255,255,255);
}

}

void setColor(int r, int g, int b){
  analogWrite(redPin, r);
  analogWrite(greenPin, g);
  analogWrite(bluePin, b);
}
