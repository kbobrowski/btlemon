#ifndef BTLEMON_BTLEMON_H
#define BTLEMON_BTLEMON_H

#include <stdint.h>

typedef void (*btlemon_callback)(const uint8_t addr[6], const int8_t *rssi);
void btlemon_set_callback(btlemon_callback callback);
void btlemon_stop();
int btlemon_run();

#endif //BTLEMON_BTLEMON_H
