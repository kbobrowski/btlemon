#include <stdio.h>
#include "btlemon.h"

void custom_callback(const uint8_t addr[6], const int8_t *rssi) {
  printf("rssi: %d\n", *rssi);
}

int main() {
  btlemon_set_callback(custom_callback);
  return btlemon_run();
}