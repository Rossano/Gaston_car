import asyncio
import sys

from bleak import BleakScanner, BleakClient
from bleak.backends.device import BLEDevice
from bleak.backends.characteristic import BleakGATTCharacteristic
from bleak.backends.scanner import AdvertisementData
from itertools import takewhile, count
from typing import Iterator
import socket

SPP_SERVICE_UUID = '4880c12c-fdcb-4077-8920-a450d7f9b907'
SPP_RX_CHAR_UUID = 'fec26ec4-6d71-4442-9f81-55bc21d658d6'
SPP_TX_CHAR_UUID = 'fec26ec4-6d71-4442-9f81-55bc21d658d6'
HOST = '127.0.0.1'
PORT = 65432

def sliced(buffer: bytes, n: int) -> Iterator[bytes]:
    """
    Slice the buffer in slices of lenght n
    :param buffer: input buffer to slice
    :param n: slice length
    :return: iterator with the different slices
    """
    return takewhile(len, (buffer[i: i + n] for i in count(0, n)))

"""
    Function to implement the SPP UART
"""
async def spp_uart():
    """
    Simple terminal function that uses the SPP service to implement a wireless UART.
    It reads from stdin and sends lines data to the remote interface. Any data read from the
    device is printed on stdin
    :return: None
    """

    def match_device_uuid(device: BLEDevice, adv: AdvertisementData):
        """
        Checks if a devices matches the SPP Service UUID
        :param device: device detected
        :param adv: Advertise data from the device
        :return: True if one device service matches SPP UUID, False otherwise
        """
        if len(adv.service_uuids) > 0:
            if SPP_SERVICE_UUID.lower() == adv.service_uuids[0]:
                return True

        return False

    # Discover the devices matching the SPP Service UUID
    device = await BleakScanner.find_device_by_filter(match_device_uuid)
    if device is None:
        print ("SPP Service not found!")
        sys.exit(1)

    def handle_rx(_: BleakGATTCharacteristic, data:bytearray):
        """
        Function to handle the data received
        :param _: dummy parameter
        :param data: received data
        :return: None
        """
        print("PC > ", data.decode('utf-8'))

    async with BleakClient(device) as client:
        """
        Main code for the terminal
        """
        # Tells that the RX service is handled as notification by handle_rx
        await client.start_notify(SPP_RX_CHAR_UUID, handle_rx)
        # At this point the BLE device is connected
        print("Connected, type and press ENTER ...")
        loop = asyncio.get_running_loop()
        # Tells that the transmission is handled by a service
        spp = client.services.get_service(SPP_SERVICE_UUID)
        rx_char = spp.get_characteristic(SPP_TX_CHAR_UUID)
        # Socket opening
        # sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        # sock.bind((HOST, PORT))
        # sock.listen()
        # conn, addr = sock.accept()
        # Main program loop
        while True:
            # Print the prompt
            # print("STM32> ", end='')
            # Wait the input from the prompt
            data = await loop.run_in_executor(None, sys.stdin.buffer.readline)
            #data = await sock.recv(1024)
            """
            Awaits the next standard input in a non blocking way 
            """
            if not data or (len(data) == 1 and data[0] == 10):
                """
                Exit from the loop just pressing ENTER
                """
                break
            # print() # Just a new line
            # print(data)
            # print(' '.join(f'{x:02x}' for x in data)) # print data in HEX format
            # Send the data to the Remote device
            for s in sliced(data, rx_char.max_write_without_response_size):
                await client.write_gatt_char(rx_char, s, response=False)



"""
    Main Program
"""
if __name__ == "__main__":
    try:
        asyncio.run(spp_uart())
    except asyncio.CancelledError:
        pass
