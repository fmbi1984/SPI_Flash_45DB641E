#include "AT45DB641E.h"
#include "hardware.h"

void setup() {
    Serial.begin(9600);
    dataflash_begin();
}

void loop() {
  dataflash_readID();
  delay(1000);
  char data[13];
  Serial.println();
  dataflash_writeBytes(0, "Hello World!", 12);
  dataflash_readBytes(0, data, 12);
  for(int i=0; i<12; i++)
  {
    Serial.print(data[i]);
  }
  Serial.println();
}


