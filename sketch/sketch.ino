
int val = 0;

void setup() {
  Serial.begin(9600);
  pinMode(10, OUTPUT); // laser
  pinMode(13, OUTPUT); // control led
}

void loop() {
  digitalWrite(10, HIGH);
  val = analogRead(A0);
  digitalWrite(13, val > 100);
  Serial.println(val);
  delay(1000);
}