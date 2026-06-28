#include <Arduino.h>
#include <esp_now.h>
#include <Wifi.h>
#include <esp_wifi.h>


int myFunction(int, int);

void setup() {

  int result = myFunction(2, 3);
}



void loop() {

}



// put function definitions here:
int myFunction(int x, int y) {
  return x + y;
}