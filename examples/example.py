import pybtlemon


def callback(addr, rssi):
    print(f"addr: {addr}, distance: {10**((-60-rssi)/20):.2f}")


pybtlemon.set_callback(callback)
pybtlemon.run()
