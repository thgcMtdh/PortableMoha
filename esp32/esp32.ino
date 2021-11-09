#include <Arduino.h>
#include "src/JointSoundClass.h"
#include "src/SDutil.h"

void setup() {
  Serial.begin(115200);
  SDutil.begin();

  size_t size = 0;
  char* buf = nullptr;
  SDutil.readFile("/hello.txt", &buf, &size);

  Serial.print("size = ");
  Serial.println(size);
  for (int i=0; i<size; i++) {
    Serial.printf("%c", buf[i]);
  }
  
  delete[] buf;
}

void loop() {
  // debug_loop();
  delay(1);
}