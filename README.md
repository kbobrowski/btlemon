### btlemon - Bluetooth Low Energy monitor for Linux

#### About

This tool provides conveniently signal strength (RSSI) and address of BLE devices scanned with ```hcitool```.

Example usage:

- execute ```sudo btlemonrun```
- in another shell execute ```sudo hcitool lescan --duplicates```
- output from hcitool:
```
40:16:3B:EB:7E:0B (unknown)
51:1E:59:4C:46:C6 (unknown)
51:1E:59:4C:46:C6 (unknown)
```
- example output from btlemon:
```
1589153080 40:16:3B:EB:7E:0B -93
1589153080 51:1E:59:4C:46:C6 -66
1589153080 51:1E:59:4C:46:C6 -66
```

Check [tools](tools) for convenient hcitool commands.

#### Using as library

Can be integrated with another program using custom callback, check [examples/example.c](examples/example.c).

#### Python bindings

```pybtlemon.so``` is compiled with CMake. Can be used as follows (remember to execute as root user, together with ```sudo hcitool lescan```):

```python
import pybtlemon


def callback(addr, rssi):
    print(f"addr: {addr}, distance: {10**((-60-rssi)/20):.2f}")


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
