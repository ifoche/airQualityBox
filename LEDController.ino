
void ledInitialConfig(){
  pinMode(LED_BUILTIN, OUTPUT);  
}

void blink(){
  digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(500);                       // wait for a second
  digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
  delay(200);
}

void blink(int times) {
  for (int i=0; i<times; i++){
    blink();  
  }
}
