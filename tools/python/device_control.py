"""
Modulo di controllo dispositivi Bluetooth Low Energy (BLE).

Fornisce funzionalità per scansionare, connettersi e comunicare con dispositivi BLE
utilizzando il protocollo SPP (Serial Port Profile) tramite la libreria Bleak.

Classi:
    DeviceControl: Gestisce la connessione e comunicazione con un dispositivo BLE singolo.

Funzioni:
    ble_uart_loop(): Ciclo asincrono principale di comunicazione BLE.
    start_ble_thread(): Avvia il ciclo BLE in un thread separato.
    scan_ble_devices(): Esegue una scansione BLE asincrona con callback.
"""

import asyncio
import threading
from bleak import BleakScanner, BleakClient
import queue

from spp_uart import spp_uart

# UUIDs del servizio SPP UART (Serial Port Profile)
SPP_SERVICE_UUID = '4880c12c-fdcb-4077-8920-a450d7f9b907'  # UUID del servizio SPP
SPP_RX_CHAR_UUID = 'fec26ec4-6d71-4442-9f81-55bc21d658d6'  # Caratteristica RX (ricezione)
SPP_TX_CHAR_UUID = 'fec26ec4-6d71-4442-9f81-55bc21d658d6'  # Caratteristica TX (trasmissione)

class DeviceControl:
    """
    Gestisce la connessione e comunicazione con un dispositivo BLE.
    
    Questa classe fornisce metodi per scansionare, connettersi, e comunicare
    con dispositivi Bluetooth Low Energy utilizzando il protocollo SPP.
    
    Attributi:
        address (str): Indirizzo MAC del dispositivo BLE.
        rx_queue (queue.Queue): Coda thread-safe per i dati ricevuti.
        tx_queue (queue.Queue): Coda thread-safe per i dati da trasmettere.
        running (bool): Flag che indica se la comunicazione è attiva.
        client (BleakClient): Client BLE Bleak per la comunicazione.
    """

    def __init__(self, address, rx_queue, tx_queue):
        """
        Inizializza il controller del dispositivo.
        
        Args:
            address (str): Indirizzo MAC del dispositivo BLE target.
            rx_queue (queue.Queue): Coda per i messaggi ricevuti.
            tx_queue (queue.Queue): Coda per i messaggi da inviare.
        """
        self.address = address
        self.rx_queue = rx_queue
        self.tx_queue = tx_queue
        self.running = False
        # self.thread = threading.Thread(target=self.run_ble_loop, daemon=True)

    async def scan(self, timeout=4.0):
        """
        Esegue una scansione BLE per trovare dispositivi.
        
        Args:
            timeout (float): Durata della scansione in secondi (default: 4.0).
            
        Returns:
            list: Lista dei dispositivi BLE trovati.
        """
        devices = await BleakScanner.discover(timeout)
        return devices

    async def connect(self, address):
        """
        Connette al dispositivo BLE specificato.
        
        Stabilisce la connessione, scopre i servizi GATT, avvia la notifica
        UART e inizia il ciclo di trasmissione.
        
        Args:
            address (str): Indirizzo MAC del dispositivo a cui connettersi.
        """
        try:
            self.client = BleakClient(address)
            await self.client.connect()
            self.address = address
            self.running = True

            self.rx_queue.put(f"[INFO] Connesso a {address}")

            await self._print_services()
            await self._start_uart()

            # Esegui il ciclo di trasmissione finché il client è connesso
            await self._tx_loop()

        except Exception as e:
            self.rx_queue.put(f"[BLE ERROR] {e}")
        finally:
            self.running = False
            if self.client and self.client.is_connected:
                await self.client.disconnect()

    async def _print_services(self):
        """
        Scopre e registra tutti i servizi e caratteristiche GATT disponibili.
        
        Stampa nella rx_queue le informazioni su tutti i servizi, caratteristiche
        e le loro proprietà per scopi di debug e verifica.
        """
        for service in self.client.services.services.values():
            self.rx_queue.put(f"[SERVICE] {service.uuid} | {service.description}")
            for char in service.characteristics:
                props = ",".join(char.properties)
                self.rx_queue.put(
                    f"  [CHAR] {char.uuid} | {char.description} | [{props}]"
                )

    async def _start_uart(self):
        """
        Avvia la notifica della caratteristica TX per ricevere dati UART.
        
        Abilita le notifiche sulla caratteristica TX del servizio SPP e registra
        il callback _on_rx per elaborare i dati ricevuti.
        """
        if SPP_TX_CHAR_UUID not in self.client.services.characteristics:
            self.rx_queue.put("[WARN] UART_TX non trovata")
            return

        await self.client.start_notify(
            SPP_TX_CHAR_UUID,
            self._on_rx
        )

    def _on_rx(self, _, data: bytearray):
        """
        Callback per elaborare i dati ricevuti dal dispositivo BLE.
        
        Decodifica i dati ricevuti e li inserisce nella coda rx_queue
        per il consumo dell'interfaccia utente.
        
        Args:
            _ : Identificatore della caratteristica (non utilizzato).
            data (bytearray): Dati ricevuti in formato byte.
        """
        self.rx_queue.put(
            f"[UART RX] {data.decode(errors='ignore')}"
        )

    async def _tx_loop(self):
        """
        Ciclo asincrono di trasmissione dati al dispositivo.
        
        Controlla continuamente la coda tx_queue e invia i messaggi al dispositivo
        tramite la caratteristica RX. Si esegue fino a quando running è False
        o la connessione si interrompe.
        """
        while self.running and self.client.is_connected:
            try:
                msg = self.tx_queue.get_nowait() + "\n"
                print(f"TX: {msg.strip()}")
                await self.client.write_gatt_char(
                    SPP_RX_CHAR_UUID,
                    msg.encode()
                )
            except queue.Empty:
                pass
            except Exception as e:
                self.rx_queue.put(f"[TX ERROR] {e}")
            await asyncio.sleep(0.1)

    async def disconnect(self):
        """
        Disconnette dal dispositivo BLE.
        
        Interrompe il ciclo di trasmissione e chiude la connessione BLE,
        registrando il messaggio di disconnessione nella rx_queue.
        """
        self.running = False
        if self.client and self.client.is_connected:
            await self.client.disconnect()
            self.rx_queue.put("[INFO] Disconnesso")

    # def start(self):
    #     self.thread.start()

    # def run_ble_loop(self):
    #     loop = asyncio.new_event_loop()
    #     asyncio.set_event_loop(loop)
    #     loop.run_until_complete(ble_uart_loop(self.address, self.rx_queue, self.tx_queue))


# --- Funzioni di supporto per il ciclo BLE ---

async def ble_uart_loop(address, rx_queue, tx_queue):
    """
    Ciclo asincrono principale per la comunicazione BLE UART.
    
    Stabilisce una connessione BLE al dispositivo, scopre i servizi GATT,
    abilita le notifiche UART e gestisce l'invio/ricezione di dati in un ciclo
    continuo fino a un'eccezione o disconnessione.
    
    Args:
        address (str): Indirizzo MAC del dispositivo BLE.
        rx_queue (queue.Queue): Coda per i dati ricevuti dal dispositivo.
        tx_queue (queue.Queue): Coda per i dati da trasmettere al dispositivo.
    """
    try:
        async with BleakClient(address) as client:
            rx_queue.put(f"[INFO] Connesso a {address}")

            # 🔍 Esplora tutti i servizi e caratteristiche GATT disponibili
            services = client.services.services
            for service in services:
                rx_queue.put(f"[SERVICE] {services[service].uuid} | {services[service].description}")
                for char in services[service].characteristics:
                    props = ",".join(char.properties)
                    rx_queue.put(f"  [CHAR] {char.uuid} | {char.description} | [{props}]")
                    for desc in char.descriptors:
                        rx_queue.put(f"    [DESC] {desc.uuid}")

            # ▶️ Avvia la comunicazione UART se la caratteristica TX è disponibile
            if SPP_TX_CHAR_UUID in [c.uuid for c in client.services.characteristics.values()]:
                await client.start_notify(
                    SPP_TX_CHAR_UUID,
                    lambda _, data: rx_queue.put(f"[UART RX] {data.decode(errors='ignore')}")
                )
            else:
                rx_queue.put("[WARN] Caratteristica UART_TX non trovata")

            # Ciclo continuo di trasmissione dati
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
    """
    Avvia il ciclo BLE in un thread separato.
    
    Crea un nuovo event loop asyncio in un thread dedicato e esegue il ciclo
    ble_uart_loop, permettendo la comunicazione non-bloccante con il dispositivo BLE.
    
    Args:
        address (str): Indirizzo MAC del dispositivo BLE.
        rx_queue (queue.Queue): Coda per i dati ricevuti.
        tx_queue (queue.Queue): Coda per i dati da trasmettere.
    """
    loop = asyncio.new_event_loop()
    asyncio.set_event_loop(loop)
    loop.run_until_complete(ble_uart_loop(address, rx_queue, tx_queue))


def scan_ble_devices(callback=None):
    """
    Esegue una scansione BLE asincrona in un thread separato.
    
    Scopre i dispositivi BLE disponibili nel raggio d'azione e invoca il callback
    con la lista dei dispositivi trovati.
    
    Args:
        callback (callable): Funzione di callback che riceve la lista dei dispositivi.
                           Signature: callback(devices: list[BleakAdvertisement])
    """
    async def scan():
        """Funzione interna asincrona di scansione."""
        devices = await BleakScanner.discover(4.0)
        callback(devices)
    
    loop = asyncio.new_event_loop()
    asyncio.set_event_loop(loop)
    loop.run_until_complete(scan())



