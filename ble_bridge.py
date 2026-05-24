import asyncio
import sys
from bleak import BleakClient, BleakScanner

NUS_TX_UUID = "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"  # ESP32 -> PC (notify)
NUS_RX_UUID = "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"  # PC -> ESP32 (write)

TCP_HOST = "127.0.0.1"
TCP_PORT = 12345

tcp_writer = None
ble_client = None


def on_ble_data(sender, data: bytearray):
    """ESP32 sent a BLE notification — forward to Qt over TCP."""
    global tcp_writer
    if tcp_writer and not tcp_writer.is_closing():
        tcp_writer.write(bytes(data))


async def handle_qt_client(reader: asyncio.StreamReader, writer: asyncio.StreamWriter):
    """One Qt app TCP connection."""
    global tcp_writer, ble_client
    tcp_writer = writer
    print(f"[TCP] Qt connected from {writer.get_extra_info('peername')}")
    try:
        while True:
            data = await reader.read(256)
            if not data:
                break
            if ble_client and ble_client.is_connected:
                await ble_client.write_gatt_char(NUS_RX_UUID, data, response=False)
    except Exception as e:
        print(f"[TCP] error: {e}")
    finally:
        tcp_writer = None
        writer.close()
        print("[TCP] Qt disconnected")


async def main():
    global ble_client

    print("[BLE] Scanning for AFE4404 (up to 10 s)...")
    device = await BleakScanner.find_device_by_name("AFE4404", timeout=10.0)
    if device is None:
        print("[BLE] ERROR: AFE4404 not found.")
        print("      Make sure the ESP32 is powered and the sketch is running.")
        sys.exit(1)

    print(f"[BLE] Found: {device.name}  [{device.address}]")

    server = await asyncio.start_server(handle_qt_client, TCP_HOST, TCP_PORT)
    print(f"[TCP] Listening on {TCP_HOST}:{TCP_PORT}")
    print()
    print("  --> Open Qt app, select 'localhost' in the port list, click Connect")
    print()

    async with BleakClient(device) as client:
        ble_client = client
        print("[BLE] Connected to ESP32!")
        await client.start_notify(NUS_TX_UUID, on_ble_data)
        async with server:
            await server.serve_forever()


if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("\nStopped.")
