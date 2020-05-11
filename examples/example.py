import pybtlemon


def callback(addr, rssi, data):
    print(f"addr: {addr}, distance: {10**((-60-rssi)/20):.2f}, data: {data}")


pybtlemon.set_callback(callback)
pybtlemon.run()
