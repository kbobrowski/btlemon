### btlemon - Bluetooth Low Energy monitor for Linux

#### About

This tool provides address, signal strength (RSSI), and data of BLE devices scanned with ```hcitool```.

Example usage:

- execute ```sudo btlemonrun```
- in another shell execute ```sudo hcitool lescan --duplicates```
- output from hcitool:
```
40:40:51:17:2B:B1 (unknown)
4E:AF:83:1E:D6:2F (unknown)
4E:AF:83:1E:D6:2F (unknown)
```
- example output from btlemon:
```
1589219961 40:40:51:17:2B:B1 -87 
1589219964 4E:AF:83:1E:D6:2F -54 0303AAFE1516AAFE00BFD7088787A44470A9FBD2000000000000
1589219964 4E:AF:83:1E:D6:2F -54
```

Check [tools](tools) for convenient hcitool commands.

#### C library

Custom callback can be defined:

```c
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
```

#### Python bindings

```pybtlemon.so``` is compiled with CMake. Can be used as follows (remember to execute as root user, together with ```sudo hcitool lescan```):

```python
import pybtlemon


def callback(addr, rssi, data):
    print(f"addr: {addr}, distance: {10**((-60-rssi)/20):.2f}, data: {data}")


pybtlemon.set_callback(callback)
pybtlemon.run()
```

#### Compiling

Requires Bluez, in particular ```bluetooth.h``` and ```hci.h``` headers. On Ubuntu 18.04 these headers are packaged in ```libbluetooth-dev```. Python bindings require Python 3.

```
mkdir -p build
cd build
cmake ..
make
```
