#include <stdio.h>
#include <math.h>
#include "btlemon.h"

static void callback(const uint8_t addr[6], const int8_t *rssi, const uint8_t *data, uint8_t data_len) {
  char addr_string[18];
  sprintf(addr_string, "%2.2X:%2.2X:%2.2X:%2.2X:%2.2X:%2.2X",
          addr[5], addr[4], addr[3],
          addr[2], addr[1], addr[0]);
  double distance = pow(10, (-60-*rssi)/20.);
  printf("addr: %s, distance: %.2f\n", addr_string, distance);
}

int main() {
  btlemon_set_callback(callback);
  return btlemon_run();
}