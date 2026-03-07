import wx
import asyncio
import threading
import queue
from bleak import BleakClient, BleakScanner

# UUID Nordic UART Service
# SPP_SERVICE_UUID = '4880c12c-fdcb-4077-8920-a450d7f9b907'
# SPP_RX_CHAR_UUID = 'fec26ec4-6d71-4442-9f81-55bc21d658d6'
# SPP_TX_CHAR_UUID = 'fec26ec4-6d71-4442-9f81-55bc21d658d6'

# UUID My SPP UART Service
SPP_SERVICE_UUID = "238caa21-7ef5-4be6-9c29-c430b459a66a"
SPP_TX_CHAR_UUID = "5a223b57-b290-447a-b772-547839d797ee" #"d5c57492-85e3-4729-8e1c-0c683b7f7b4e"
SPP_RX_CHAR_UUID = "5a223b57-b290-447a-b772-547839d797ee"
   # "d5c57492-85e3-4729-8e1c-0c683b7f7b4e"


# --- Async routine BLE UART ---
async def ble_uart_loop(address, rx_queue, tx_queue):
    try:
        async with BleakClient(address) as client:
            rx_queue.put(f"[INFO] Connesso a {address}")

            # 🔍 Esplora tutti i servizi
            # services = await client.get_services()
            services = client.services.services
            for service in services:
                rx_queue.put(f"[SERVICE] {services[service].uuid} | {services[service].description}")
                for char in services[service].characteristics:
                    props = ",".join(char.properties)
                    rx_queue.put(f"  [CHAR] {char.uuid} | {char.description} | [{props}]")
                    for desc in char.descriptors:
                        rx_queue.put(f"    [DESC] {desc.uuid}")

            # ▶️ Avvia la comunicazione UART se disponibile
            if SPP_TX_CHAR_UUID in [c.uuid for c in client.services.characteristics.values()]:
                await client.start_notify(
                    SPP_TX_CHAR_UUID,
                    lambda _, data: rx_queue.put(f"[UART RX] {data.decode(errors='ignore')}")
                )
            else:
                rx_queue.put("[WARN] Caratteristica UART_TX non trovata")

            while True:
                try:
                    msg = tx_queue.get_nowait() + '\n'
                    await client.write_gatt_char(SPP_RX_CHAR_UUID, msg.encode())
                except queue.Empty:
                    pass
                await asyncio.sleep(0.1)

    except Exception as e:
        rx_queue.put(f"[BLE ERROR] {e}")


def start_ble_thread(address, rx_queue, tx_queue):
    loop = asyncio.new_event_loop()
    asyncio.set_event_loop(loop)
    loop.run_until_complete(ble_uart_loop(address, rx_queue, tx_queue))

# --- Scansione BLE in thread separato ---
def scan_ble_devices(callback):
    async def scan():
        devices = await BleakScanner.discover(timeout=4.0)
        callback(devices)
    loop = asyncio.new_event_loop()
    asyncio.set_event_loop(loop)
    loop.run_until_complete(scan())

# --- GUI Frame ---
class BLEFrame(wx.Frame):
    def __init__(self):
        super().__init__(None, title="UART via BLE", size=(600, 450))
        panel = wx.Panel(self)

        self.log = wx.TextCtrl(panel, style=wx.TE_MULTILINE | wx.TE_READONLY,
                               size=(560, 200), pos=(10, 10))

        self.input = wx.TextCtrl(panel, size=(450, 30), pos=(10, 220))
        self.send_btn = wx.Button(panel, label="Invia", pos=(470, 220))
        self.send_btn.Bind(wx.EVT_BUTTON, self.on_send)
        self.send_btn.Disable()

        self.scan_btn = wx.Button(panel, label="Scansiona BLE", pos=(10, 270))
        self.scan_btn.Bind(wx.EVT_BUTTON, self.on_scan)

        self.device_choice = wx.Choice(panel, pos=(150, 270), size=(300, -1))
        self.connect_btn = wx.Button(panel, label="Connetti", pos=(470, 270))
        self.connect_btn.Bind(wx.EVT_BUTTON, self.on_connect)

        self.rx_queue = queue.Queue()
        self.tx_queue = queue.Queue()

        self.timer = wx.Timer(self)
        self.Bind(wx.EVT_TIMER, self.on_timer, self.timer)
        self.timer.Start(200)

        self.devices = []

    def on_send(self, _):
        txt = self.input.GetValue().strip()
        if txt:
            self.tx_queue.put(txt)
            self.log.AppendText(f"TX: {txt}\n")
            self.input.Clear()

    def on_timer(self, _):
        while not self.rx_queue.empty():
            msg = self.rx_queue.get()
            self.log.AppendText(f"{msg}\n")

    def on_scan(self, _):
        self.log.AppendText("Scansione in corso...\n")
        self.scan_btn.Disable()
        threading.Thread(target=self.do_scan).start()

    def do_scan(self):
        def on_devices_found(devices):
            wx.CallAfter(self.update_device_list, devices)
        scan_ble_devices(on_devices_found)

    def update_device_list(self, devices):
        self.devices = devices
        self.device_choice.Clear()
        for d in devices:
            name = d.name or "Sconosciuto"
            self.device_choice.Append(f"{name} ({d.address})")
        self.log.AppendText(f"Trovati {len(devices)} dispositivi BLE\n")
        self.scan_btn.Enable()

    def on_connect(self, _):
        index = self.device_choice.GetSelection()
        if index == wx.NOT_FOUND:
            wx.MessageBox("Seleziona un dispositivo prima di connetterti.", "Errore")
            return
        device = self.devices[index]
        self.log.AppendText(f"Connessione a {device.name or 'Sconosciuto'}...\n")
        self.send_btn.Enable()
        self.connect_btn.Disable()
        self.scan_btn.Disable()
        self.device_choice.Disable()

        # Avvia il BLE UART
        threading.Thread(target=start_ble_thread,
                         args=(device.address, self.rx_queue, self.tx_queue),
                         daemon=True).start()

class MyApp(wx.App):
    def OnInit(self):
        frame = BLEFrame()
        frame.Show()
        return True

if __name__ == "__main__":
    MyApp(False).MainLoop()
