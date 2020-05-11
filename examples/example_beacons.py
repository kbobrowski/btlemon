import pybtlemon
import uuid


def callback(addr, rssi, data):
    # check for eddystone UID
    if len(data) == 26 and data[2] == 0xaa and data[3] == 0xfe and data[8] == 0x00:
        tx = data[9] - (1 << 8)
        namespace = "".join('{:02x}'.format(x) for x in data[10:20])
        instance = "".join('{:02x}'.format(x) for x in data[20:])
        print(f"eddystone | addr: {addr}, rssi: {rssi}, tx: {tx}, namespace: {namespace}, instance: {instance}")
    # check for ibeacon
    if len(data) >= 26 and data[2] == 0x4c and data[3] == 0x00 and data[4] == 0x02 and data[5] == 0x15:
        id = uuid.UUID(bytes=bytes(data[6:22]))
        major = int.from_bytes([data[22], data[23]], "big")
        minor = int.from_bytes([data[24], data[25]], "big")
        print(f"ibeacon   | addr: {addr}, rssi: {rssi}, uuid: {id}, major: {major}, minor: {minor}")


pybtlemon.set_callback(callback)
pybtlemon.run()
