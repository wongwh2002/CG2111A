void setup() {
  // put your setup code here, to run once:

}

void loop() {
  // put your main code here, to run repeatedly:

  forward(200); // takes values from 0 to 255
  delay(2000);

  ccw(200); // takes values from 0 to 255
  delay(2000);

  backward(200); // takes values from 0 to 255
  delay(2000);

  cw(200); // takes values from 0 to 255
  delay(2000);

  stop();
  delay(4000);
}
