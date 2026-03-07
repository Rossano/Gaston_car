#!/usr/bin/env python
#
# A simple terminal application with wxPython.
#
# (C) 2001-2020 Chris Liechti <cliechti@gmx.net>
#
# SPDX-License-Identifier:    BSD-3-Clause

# Modulo per la gestione e visualizzazione di un terminale seriale/BLE con interfaccia wxPython
# Permette di inviare/ricevere dati da una porta seriale o da un dispositivo BLE (Bluetooth Low Energy)
# oltre a fornire controlli per la velocità di un motore via SPP (Serial Port Profile)

import codecs                           # Per la gestione di codifiche di caratteri
import time                             # Per delay temporali

from serial.tools.miniterm import unichr  # Import di unichr dal modulo miniterm
import serial                           # Per la comunicazione seriale
import threading                        # Per l'esecuzione di codice in thread separati (non-blocking)
import wx                               # wxPython: framework per GUI desktop
import wx.lib.newevent                  # Per creare event personalizzati
import wxSerialConfigDialog             # Dialog personalizzato per configurazione seriale
from wxasync import AsyncBind, WxAsyncApp, StartCoroutine  # Binding asincroni per wxPython

import asyncio                          # Gestione asincrona: COM Port Control e BLE scanning
import sys                              # Accesso a funzioni di sistema
import queue                            # Code thread-safe per comunicazione tra thread

from device_control import DeviceControl  # Classe custom per gestire la connessione BLE

try:
    unichr  # Verifica se unichr esiste
except NameError:
    unichr = chr  # In Python 3, unichr è stato rinominato in chr

# ===== DEFINIZIONE CUSTOM EVENTS =====
# Creazione di un tipo di evento personalizzato per la ricezione di dati seriali
# Necessario perché solo il thread principale può aggiornare la GUI in wxPython
# Alternative: wxMutexGuiEnter/wxMutexGuiLeave, ma gli event sono più eleganti

SerialRxEvent, EVT_SERIALRX = wx.lib.newevent.NewEvent()  # Evento per dati ricevuti dalla porta seriale
SERIALRX = wx.NewEventType()  # Type di evento

# ===== COSTANTI PER I MENU ID =====
# ID univoci per i controlli della GUI (usati negli event handler)

ID_CLEAR = wx.Window.NewControlId()      # Pulire il log
ID_SAVEAS = wx.Window.NewControlId()     # Salva testo come file
ID_SETTINGS = wx.Window.NewControlId()   # Impostazioni porta seriale
ID_TERM = wx.Window.NewControlId()       # Impostazioni terminale
ID_EXIT = wx.Window.NewControlId()       # Esci dall'applicazione
ID_RTS = wx.Window.NewControlId()        # Request To Send (segnale seriale)
ID_DTR = wx.Window.NewControlId()        # Data Terminal Ready (segnale seriale)
ID_HELP = wx.Window.NewControlId()       # Mostra help/comandi
ID_ABOUT = wx.Window.NewControlId()      # Info dell'app

# ===== COSTANTI PER LA GESTIONE NEWLINE =====
# Definiscono i diversi modi di gestire i newline in trasmissione

NEWLINE_CR = 0      # Carriage Return (\r) solamente
NEWLINE_LF = 1      # Line Feed (\n) solamente
NEWLINE_CRLF = 2    # Carriage Return + Line Feed (\r\n)

# ===== CLASS MyDialog =====
# Dialog semplice per scegliere Se connessione BLE o Seriale all'avvio

class MyDialog(wx.Dialog):
    """Dialog per scegliere il tipo di connessione: BLE o Seriale"""
    
    def __init__(self, *args, **kwds):
        # Configurazione di base del dialog
        kwds["style"] = kwds.get("style", 0) | wx.DEFAULT_DIALOG_STYLE
        wx.Dialog.__init__(self, *args, **kwds)
        self.SetTitle("Bluetooth Or Serial Connection")

        # Layout verticale principale
        sizer_1 = wx.BoxSizer(wx.VERTICAL)

        # RadioBox con opzioni Bluetooth/Serial
        self.ble_uart_radio_box = wx.RadioBox(
            self, wx.ID_ANY, "",
            choices=["Bluetooth", "Serial Port"],
            majorDimension=2,  # 2 colonne
            style=wx.RA_SPECIFY_ROWS
        )
        self.ble_uart_radio_box.SetSelection(0)  # Default: Bluetooth
        sizer_1.Add(self.ble_uart_radio_box, 1, wx.ALL, 1)

        # Pulsanti OK e Cancel (standard wxPython)
        sizer_2 = wx.StdDialogButtonSizer()
        sizer_1.Add(sizer_2, 0, wx.ALIGN_RIGHT | wx.ALL, 4)

        self.button_OK = wx.Button(self, wx.ID_OK, "")
        self.button_OK.SetDefault()  # Pulsante di default
        sizer_2.AddButton(self.button_OK)

        self.button_CANCEL = wx.Button(self, wx.ID_CANCEL, "")
        sizer_2.AddButton(self.button_CANCEL)

        sizer_2.Realize()

        self.SetSizer(sizer_1)
        sizer_1.Fit(self)

        self.SetAffirmativeId(self.button_OK.GetId())      # Enter = OK
        self.SetEscapeId(self.button_CANCEL.GetId())        # Esc = Cancel

        self.Layout()

        # Binding degli event handler
        self.Bind(wx.EVT_RADIOBOX, self.on_ble_selection, self.ble_uart_radio_box)
        self.Bind(wx.EVT_BUTTON, self.on_bleuart_OK_evt, self.button_OK)
        self.Bind(wx.EVT_BUTTON, self.on_bleuart_Cancel_evt, self.button_CANCEL)

    def on_ble_selection(self, event):
        """Callback quando l'utente cambia selezione BLE/Seriale"""
        print("Event handler 'on_ble_selection' not implemented!")
        event.Skip()

    def on_bleuart_OK_evt(self, event):
        """Callback quando l'utente clicca OK"""
        print("Event handler 'on_bleuart_OK_evt' not implemented!")
        event.Skip()

    def on_bleuart_Cancel_evt(self, event):
        """Callback quando l'utente clicca Cancel"""
        print("Event handler 'on_bleuart_Cancel_evt' not implemented!")
        event.Skip()

# ===== CLASS TerminalSetup =====
# Placeholder/contenitore per le impostazioni del terminale

class TerminalSetup:
    """
    Placeholder per varie impostazioni del terminale.
    Usato per passare le opzioni al TerminalSettingsDialog.
    """
    def __init__(self):
        self.echo = False              # Echo locale (ripetere i caratteri digitati)
        self.unprintable = False       # Mostrare caratteri non stampabili
        self.newline = NEWLINE_CRLF    # Modalità newline di default

# ===== CLASS TerminalSettingsDialog =====
# Dialog per impostare i parametri del terminale e di connessione BLE/Seriale

class TerminalSettingsDialog(wx.Dialog):
    """Dialog per configurare le impostazioni del terminale (echo, newline) e BLE"""

    settings = None
    
    def __init__(self, *args, **kwds):
        # Estrarre parametri speciali da kwds
        self.dev = kwds.pop('dev', None)        # DeviceControl per BLE
        self.settings = kwds.pop('settings', None)  # TerminalSetup con impostazioni
        
        # Configurazione di base del dialog
        kwds["style"] = kwds.get("style", 0) | wx.DEFAULT_DIALOG_STYLE
        wx.Dialog.__init__(self, *args, **kwds)
        self.SetTitle("Terminal Settings")

        # Layout verticale principale
        sizer_2 = wx.BoxSizer(wx.VERTICAL)

        # Notebook (tab) per organizzare BLE e Impostazioni Seriali
        self.ble_tab = wx.Notebook(self, wx.ID_ANY)
        sizer_2.Add(self.ble_tab, 1, wx.EXPAND, 0)

        # ===== TAB 1: BLE SETTINGS =====
        self.BLE_Setting = wx.Panel(self.ble_tab, wx.ID_ANY)
        self.ble_tab.AddPage(self.BLE_Setting, "BLE Settings")

        # Grid per i controlli BLE (3 righe x 3 colonne)
        grid_sizer_1 = wx.GridSizer(3, 3, 0, 0)

        # Pulsante Scan per cercare dispositivi BLE
        self.scan_btn = wx.Button(self.BLE_Setting, wx.ID_ANY, "Scan")
        grid_sizer_1.Add(self.scan_btn, 0, 0, 0)

        # Label "Lista Device BLE"
        label_1 = wx.StaticText(self.BLE_Setting, wx.ID_ANY, "Lista Device BLE")
        grid_sizer_1.Add(label_1, 0, 0, 0)

        # Choice (dropdown) per selezionare device BLE trovati
        self.device_choice = wx.Choice(self.BLE_Setting, wx.ID_ANY, choices=["choice 1"])
        self.device_choice.SetSelection(0)
        grid_sizer_1.Add(self.device_choice, 0, 0, 0)

        # Pannelli placeholder (spazi vuoti)
        self.panel_1 = wx.Panel(self.BLE_Setting, wx.ID_ANY)
        grid_sizer_1.Add(self.panel_1, 1, wx.EXPAND, 0)

        self.panel_2 = wx.Panel(self.BLE_Setting, wx.ID_ANY)
        grid_sizer_1.Add(self.panel_2, 1, wx.EXPAND, 0)

        # Pulsante Connect per connettersi al dispositivo selezionato
        self.connect_btn = wx.Button(self.BLE_Setting, wx.ID_ANY, "Connect")
        grid_sizer_1.Add(self.connect_btn, 0, 0, 0)

        # Text log per visualizzare messaggi di scan/connect
        self.log = wx.TextCtrl(self.BLE_Setting, wx.ID_ANY, "")
        grid_sizer_1.Add(self.log, 0, 0, 0)

        # ===== TAB 2: SERIAL SETTINGS =====
        self.Serial_Settings = wx.Panel(self.ble_tab, wx.ID_ANY)
        self.ble_tab.AddPage(self.Serial_Settings, "Serial Settings")

        # Box sizer con border per "Input/Output"
        sizer_4 = wx.StaticBoxSizer(
            wx.StaticBox(self.Serial_Settings, wx.ID_ANY, "Input/Output"),
            wx.VERTICAL
        )

        # Checkbox per Echo locale
        self.checkbox_echo = wx.CheckBox(self.Serial_Settings, wx.ID_ANY, "Local Echo")
        sizer_4.Add(self.checkbox_echo, 0, wx.ALL, 4)

        # Checkbox per mostrare caratteri non stampabili
        self.checkbox_unprintable = wx.CheckBox(
            self.Serial_Settings, wx.ID_ANY,
            "Show unprintable characters"
        )
        sizer_4.Add(self.checkbox_unprintable, 0, wx.ALL, 4)

        # RadioBox per modalità newline (CR, LF, CR+LF)
        self.radio_box_newline = wx.RadioBox(
            self.Serial_Settings, wx.ID_ANY,
            "Newline Handling",
            choices=["CR only", "LF only", "CR+LF"],
            majorDimension=0,  # Verticale
            style=wx.RA_SPECIFY_ROWS
        )
        self.radio_box_newline.SetSelection(0)
        sizer_4.Add(self.radio_box_newline, 0, 0, 0)

        # ===== PULSANTI OK/CANCEL =====
        sizer_3 = wx.BoxSizer(wx.HORIZONTAL)
        sizer_2.Add(sizer_3, 0, wx.ALIGN_RIGHT | wx.ALL, 4)

        self.button_ok = wx.Button(self, wx.ID_OK, "")
        self.button_ok.SetDefault()
        sizer_3.Add(self.button_ok, 0, 0, 0)

        self.button_cancel = wx.Button(self, wx.ID_CANCEL, "")
        sizer_3.Add(self.button_cancel, 0, 0, 0)

        # ===== APLICARE LAYOUT =====
        self.Serial_Settings.SetSizer(sizer_4)
        self.BLE_Setting.SetSizer(grid_sizer_1)
        self.SetSizer(sizer_2)
        sizer_2.Fit(self)

        self.Layout()

        # ===== BINDING EVENTI =====
        self.Bind(wx.EVT_BUTTON, self.on_scan, self.scan_btn)
        self.Bind(wx.EVT_BUTTON, self.on_connect, self.connect_btn)
        
        # Attaccamento degli altri eventi
        self.__attach_events()
        
        # Caricare i valori correnti delle impostazioni nei checkbox/radio
        self.checkbox_echo.SetValue(self.settings.echo)
        self.checkbox_unprintable.SetValue(self.settings.unprintable)
        self.radio_box_newline.SetSelection(self.settings.newline)

    def __attach_events(self):
        """Attaccare gli event handler ai pulsanti OK/Cancel"""
        self.Bind(wx.EVT_BUTTON, self.OnOK, id=self.button_ok.GetId())
        self.Bind(wx.EVT_BUTTON, self.OnCancel, id=self.button_cancel.GetId())
        # Event handler per Scan e Connect già allegati nel __init__
        self.Bind(wx.EVT_BUTTON, self.on_scan, self.scan_btn)
        self.Bind(wx.EVT_BUTTON, self.on_connect, self.connect_btn)

    def OnOK(self, events):
        """Aggiornare i dati con i nuovi valori e chiudere il dialog"""
        self.settings.echo = self.checkbox_echo.GetValue()
        self.settings.unprintable = self.checkbox_unprintable.GetValue()
        self.settings.newline = self.radio_box_newline.GetSelection()
        self.EndModal(wx.ID_OK)  # Chiudi e ritorna OK

    def OnCancel(self, events):
        """Non aggiornare i dati e chiudere il dialog"""
        self.EndModal(wx.ID_CANCEL)  # Chiudi e ritorna CANCEL

    def on_scan(self, _):
        """Callback pulsante Scan: cercare device BLE e popolare lista"""
        self.log.AppendText("Scanning for devices...\n")
        self.scan_btn.Disable()  # Disabilita pulsante durante scan
        # Eseguire scan in un thread separato (non-blocking)
        threading.Thread(target=self.do_scan).start()

    def do_scan(self):
        """Eseguire la scansione BLE in un thread"""
        def on_devices_found(devices):
            # Eseguire l'aggiornamento della GUI nel thread principale
            wx.CallAfter(self.update_device_list, devices)
        
        # Lanciare scan BLE in un thread daemon
        threading.Thread(
            target=self._scan_ble,
            args=(on_devices_found,),
            daemon=True
        ).start()
    
    def _scan_ble(self, callback):
        """Scansione BLE: crea loop asyncio e esegue scan"""
        loop = asyncio.new_event_loop()  # Nuovo event loop per il thread
        asyncio.set_event_loop(loop)
        try:
            # Eseguire scan asincrono con timeout 4 secondi
            devices = loop.run_until_complete(self.dev.scan(timeout=4.0))
            # Callback con i dispositivi trovati
            callback(devices)
        finally:
            loop.close()  # Chiudere il loop

    def update_device_list(self, devices):
        """Aggiornare la lista di choice con i device trovati"""
        self.devices = devices  # Salvare lista per accesso successivo
        self.device_choice.Clear()  # Pulire scelte precedenti
        
        # Popolare choice con nome e indirizzo MAC di ogni device
        for d in devices:
            name = d.name if d.name else "Unknown"
            self.device_choice.Append(f"{name} ({d.address})", d)
        
        self.scan_btn.Enable()  # Riabilitare pulsante scan
        self.log.AppendText(f"Trovati {len(devices)} dispositivi BLE\n")

    def on_connect(self, _):
        """Callback pulsante Connect: connettersi al device selezionato"""
        # Ottenere indice dell'elemento selezionato
        index = self.device_choice.GetSelection()
        if index == wx.NOT_FOUND:
            wx.MessageBox(
                "Please select a device to connect",
                "Error",
                wx.OK | wx.ICON_ERROR
            )
            return
        
        # Recuperare il device object dall'indice
        device = self.devices[index]
        self.log.AppendText(
            f"Connecting to {device.name} ({device.address})...\n"
        )
        
        # Disabilitare controlli durante connessione
        self.connect_btn.Disable()
        self.scan_btn.Disable()
        self.device_choice.Disable()
        
        # Salvare indirizzo del device connesso
        self.address = device.address

    def get_address(self):
        """Ritornare l'indirizzo MAC del device connesso"""
        return self.address
    
# === FINE CLASS TerminalSettingsDialog ===

# ===== CLASS TerminalFrame =====
# Finestra principale: interfaccia grafica per il terminale BLE/Seriale + controlli motore

class TerminalFrame(wx.Frame):
    """Frame principale del terminale seriale/BLE con controlli motore"""
    
    # Attributi di classe
    speed = 0               # Velocità corrente del motore
    rx_queue = None         # Coda per dati ricevuti (BLE/Seriale)
    tx_queue = None         # Coda per dati da trasmettere
    dev = None              # DeviceControl object per gestione BLE
    ble_connection = True   # Flag: True = BLE, False = Seriale

    def Move(self, val):
        """Muove il motore alla velocità specificata
        
        Args:
            val: velocità (-100 a +100)
                Positivo = avanti, Negativo = indietro, 0 = stop
        """
        val = int(val)
        if val > 0:
            # Movimento in avanti
            self.Send(f"for {val}")
            self.log.AppendText(f"Moving Forward speed {val}\n")
        elif val < 0:
            # Movimento indietro
            self.Send(f"back {-val}")
            self.log.AppendText(f"Moving Backward speed {-val}\n")
        else:
            # Stop
            self.Send("stop")
            self.log.AppendText("Stopping motor\n")
        
        # Aggiornare slider con la velocità corrente
        self.speed_slider.SetValue(val)

    def Move_Left(self, val):
        """Muove il motore a sinistra alla velocità specificata
        
        Args:
            val: velocità (-100 a +100)
        """
        speed = int(val)
        if speed == 0:
            # Motore fermo lo faccio muovere
            speed = 10
        
        self.Send(f"left {val}")
        self.log.AppendText(f"Moving Left speed {val}\n")
        
        # Aggiornare slider con la velocità corrente
        self.speed_slider.SetValue(val)

    def Move_Right(self, val):
        """Muove il motore a destra alla velocità specificata
        
        Args:
            val: velocità (-100 a +100)
        """
        speed = int(val)
        if speed == 0:
            # Motore fermo lo faccio muovere
            speed = 10
        
        self.Send(f"right {val}")
        self.log.AppendText(f"Moving Right speed {val}\n")
        
        # Aggiornare slider con la velocità corrente
        self.speed_slider.SetValue(val)

    def __init__(self, *args, **kwds):
        """Inizializzazione della finestra principale"""
        self.ble_connection = True  # Default: connessione BLE
        
        # Inizializzare code thread-safe
        self.address = None
        self.rx_queue = queue.Queue()       # Coda ricezione
        self.tx_queue = queue.Queue()       # Coda trasmissione
        self.dev = DeviceControl(self.address, self.rx_queue, self.tx_queue)

        # Configurare porta seriale
        self.serial = serial.Serial()
        self.serial.timeout = 0.5   # Timeout per lettura non-blocking
        self.settings = TerminalSetup()  # Impostazioni terminale
        self.serial.baudrate = 115200  # Baudrate di default
        self.settings.newline = NEWLINE_CR

        # Configurazione della finestra
        kwds["style"] = kwds.get("style", 0) | wx.DEFAULT_FRAME_STYLE
        wx.Frame.__init__(self, *args, **kwds)
        self.SetSize((640, 480))
        self.SetTitle("Serial Terminal")

        # ===== MENU BAR =====
        self.frame_terminal_menubar = wx.MenuBar()
        
        # Menu File
        wxglade_tmp_menu = wx.Menu()
        wxglade_tmp_menu.Append(ID_SAVEAS, "&Save Text As...", "")
        self.Bind(wx.EVT_MENU, self.OnSaveAs, id=ID_SAVEAS)
        wxglade_tmp_menu.AppendSeparator()
        wxglade_tmp_menu.Append(ID_TERM, "&Terminal Settings...", "")
        self.Bind(wx.EVT_MENU, self.OnTermSettings, id=ID_TERM)
        wxglade_tmp_menu.AppendSeparator()
        wxglade_tmp_menu.Append(ID_EXIT, "&Exit", "")
        self.Bind(wx.EVT_MENU, self.OnExit, id=ID_EXIT)
        self.frame_terminal_menubar.Append(wxglade_tmp_menu, "&File")
        
        # Menu Clear
        wxglade_tmp_menu = wx.Menu()
        self.frame_terminal_menubar.Append(wxglade_tmp_menu, "&Clear")
        
        # Menu Serial Port (con opzioni RTS/DTR)
        wxglade_tmp_menu = wx.Menu()
        wxglade_tmp_menu.Append(ID_RTS, "RTS", "", wx.ITEM_CHECK)
        self.Bind(wx.EVT_MENU, self.OnRTS, id=ID_RTS)
        wxglade_tmp_menu.Append(ID_DTR, "&DTR", "", wx.ITEM_CHECK)
        self.Bind(wx.EVT_MENU, self.OnDTR, id=ID_DTR)
        wxglade_tmp_menu.Append(ID_SETTINGS, "&Port Settings...", "")
        self.Bind(wx.EVT_MENU, self.OnPortSettings, id=ID_SETTINGS)
        self.frame_terminal_menubar.Append(wxglade_tmp_menu, "Serial Port")
        
        # Menu Help
        wxglade_tmp_menu = wx.Menu()
        wxglade_tmp_menu.Append(ID_HELP, "List &Commands", "")
        self.Bind(wx.EVT_MENU, self.OnHelp, id=ID_HELP)
        wxglade_tmp_menu.AppendSeparator()
        wxglade_tmp_menu.Append(ID_ABOUT, "About", "")
        self.Bind(wx.EVT_MENU, self.OnAbout, id=ID_ABOUT)
        self.frame_terminal_menubar.Append(wxglade_tmp_menu, "&Help")
        
        self.SetMenuBar(self.frame_terminal_menubar)

        # ===== CONTROLLI GUI =====
        sizer_1 = wx.BoxSizer(wx.VERTICAL)

        # Grid di controlli motore (3 righe x 4 colonne)
        sizer_5 = wx.GridSizer(3, 4, 0, 0)
        sizer_1.Add(sizer_5, 0, 0, 0)

        # Pulsante sinistro (placeholder)
        self.Left_button = wx.Button(self, wx.ID_ANY, "<-")
        sizer_5.Add(self.Left_button, 0, wx.ALIGN_CENTER, 0)

        # Spin button per velocità
        self.spin_move = wx.SpinButton(self, wx.ID_ANY)
        self.spin_move.SetMinSize((127, 50))
        sizer_5.Add(self.spin_move, 1, wx.ALIGN_CENTER, 0)

        # Pulsante destro (placeholder)
        self.Right_button = wx.Button(self, wx.ID_ANY, "->")
        sizer_5.Add(self.Right_button, 0, wx.ALIGN_CENTER, 0)

        # Pulsante Stop
        self.Stop_button = wx.Button(self, wx.ID_ANY, "STOP")
        sizer_5.Add(self.Stop_button, 0, wx.ALIGN_CENTER, 0)

        # Placeholder: panel vuoto
        self.panel_1 = wx.Panel(self, wx.ID_ANY)
        sizer_5.Add(self.panel_1, 1, wx.EXPAND, 0)

        # Label "Speed"
        speed_label = wx.StaticText(self, wx.ID_ANY, "Speed: ")
        speed_label.SetMinSize((100, 30))
        speed_label.SetFont(wx.Font(15, wx.FONTFAMILY_DEFAULT, wx.FONTSTYLE_NORMAL, wx.FONTWEIGHT_NORMAL, 0, ""))
        sizer_5.Add(speed_label, 0, wx.ALIGN_CENTER, 0)

        # TextCtrl read-only per mostrare velocità corrente
        self.speed_text = wx.TextCtrl(self, wx.ID_ANY, "", style=wx.TE_READONLY)
        sizer_5.Add(self.speed_text, 0, wx.ALIGN_CENTER | wx.SHAPED, 0)

        # Slider verticale per controllo velocità (-100 a +100)
        self.speed_slider = wx.Slider(
            self, wx.ID_ANY, 0, -100, 100,
            style=wx.SL_INVERSE | wx.SL_LABELS | wx.SL_VERTICAL
        )
        sizer_5.Add(self.speed_slider, 0, wx.ALIGN_CENTER, 0)

        # Label "Tx Command"
        label_tx_command = wx.StaticText(self, wx.ID_ANY, "Tx Command", style=wx.ALIGN_CENTER_HORIZONTAL)
        sizer_5.Add(label_tx_command, 0, wx.ALIGN_CENTER, 0)

        # TextCtrl per inserire comando da trasmettere
        self.tx_txt_ctrl = wx.TextCtrl(self, wx.ID_ANY, "")
        self.tx_txt_ctrl.SetMinSize((226, 23))
        sizer_5.Add(self.tx_txt_ctrl, 0, wx.ALIGN_CENTER | wx.FIXED_MINSIZE, 0)

        # Pulsante Send
        self.send_btn = wx.Button(self, wx.ID_ANY, "button_1")
        sizer_5.Add(self.send_btn, 0, wx.ALIGN_CENTER, 0)

        # Placeholder: cella vuota
        sizer_5.Add((0, 0), 0, 0, 0)

        # ===== LOG TEXT CONTROL =====
        # Area principale per visualizzare dati ricevuti/inviati
        self.log = wx.TextCtrl(
            self, wx.ID_ANY, "",
            style=wx.TE_MULTILINE | wx.TE_READONLY  # Read-only
        )
        self.log.SetMinSize((530, 175))
        self.log.SetFont(wx.Font(9, wx.FONTFAMILY_MODERN, wx.FONTSTYLE_NORMAL, wx.FONTWEIGHT_NORMAL, 0, ""))
        sizer_1.Add(self.log, 0, wx.ALL | wx.EXPAND, 0)

        # Applicare layout
        self.SetSizer(sizer_1)
        self.Layout()

        # ===== BINDING EVENTI PRINCIPALI =====
        self.Bind(wx.EVT_BUTTON, self.onLeft, self.Left_button)
        self.Bind(wx.EVT_SPIN_DOWN, self.OnBackward, self.spin_move)      # Giù = indietro
        self.Bind(wx.EVT_SPIN_UP, self.OnForward, self.spin_move)          # Su = avanti
        self.Bind(wx.EVT_BUTTON, self.OnRight, self.Right_button)
        self.Bind(wx.EVT_BUTTON, self.OnStop, self.Stop_button)
        
        # Binding per slider (movimento, cambio valore)
        self.Bind(wx.EVT_COMMAND_SCROLL_BOTTOM, self.onSpeedSlider, self.speed_slider)
        self.Bind(wx.EVT_COMMAND_SCROLL_CHANGED, self.onSpeedSlider, self.speed_slider)
        self.Bind(wx.EVT_COMMAND_SCROLL_PAGEDOWN, self.onSpeedSlider, self.speed_slider)
        self.Bind(wx.EVT_COMMAND_SCROLL_PAGEUP, self.onSpeedSlider, self.speed_slider)
        self.Bind(wx.EVT_COMMAND_SCROLL_TOP, self.onSpeedSlider, self.speed_slider)
        self.Bind(wx.EVT_SLIDER, self.onSpeedSlider, self.speed_slider)
        
        # Binding per pulsante Send
        self.Bind(wx.EVT_BUTTON, self.on_send, self.send_btn)

        # Impostare limiti spin button
        self.spin_move.Min = -10
        self.spin_move.Max = 10
        self.spin_move.Value = 0
        
        # Attaccare altri eventi e avviare thread COM
        self.__attach_events()
        self.OnPortSettings(None)  # Aprire dialog impostazioni porta all'avvio


    def Send(self, data):
        """Invia dati via BLE o porta seriale
        
        Args:
            data: stringa o bytes da inviare
        """
        # Convertire bytes in stringa se necessario
        if isinstance(data, bytes):
            data = data.decode('utf-8', 'replace')
        
        print(f"Sending: {data}")
        
        if self.ble_connection:
            # Inviare via BLE: mettere in coda di trasmissione
            self.tx_queue.put(data)  # self.tx_queue condiviso con self.dev
            self.log.AppendText(f'Sending via BLE: {data}\n')
        else:
            # Inviare via seriale: scrivere byte per byte con delay
            for c in data:
                self.serial.write(c.encode())
                time.sleep(0.01)  # Delay 10ms tra byte
            self.serial.write(b'\r')  # Aggiungere CR finale
            self.log.AppendText(f'Sending via Serial: {data}\n')
        
        self.log.AppendText(f'Sent: {data}\n')

    def __attach_events(self):
        """Attaccare gli event handler ai controlli GUI"""
        # Event handler menu
        self.Bind(wx.EVT_MENU, self.OnClear, id=ID_CLEAR)
        self.Bind(wx.EVT_MENU, self.OnSaveAs, id=ID_SAVEAS)
        self.Bind(wx.EVT_MENU, self.OnExit, id=ID_EXIT)
        self.Bind(wx.EVT_MENU, self.OnPortSettings, id=ID_SETTINGS)
        self.Bind(wx.EVT_MENU, self.OnTermSettings, id=ID_TERM)
        self.Bind(wx.EVT_MENU, self.OnHelp, id=ID_HELP)
        
        # Binding asincroni per input tastiera
        AsyncBind(wx.EVT_CHAR, self.OnKey, self.log)
        AsyncBind(wx.EVT_CHAR_HOOK, self.OnKey, self.log)
        
        # Avviare coroutine asincrona per lettura da porta
        StartCoroutine(self.ComPortThread, self)
        
        # Event handler per dati ricevuti da porta seriale
        AsyncBind(EVT_SERIALRX, self.OnSerialRead, self.log)
        
        # Event handler chiusura applicazione
        self.Bind(wx.EVT_CLOSE, self.OnClose)

    def OnExit(self, event):
        """Callback menu File > Exit"""
        self.Close()

    def OnClose(self, event):
        """Callback chiusura applicazione: pulizia risorse"""
        self.serial.close()  # Chiudere porta seriale
        self.Destroy()       # Chiudere finestra ed uscire

    def OnSaveAs(self, event):
        """Callback menu File > Save Text As: salvare il log su file"""
        with wx.FileDialog(
                None,
                "Save Text As...",
                ".",
                "",
                "Text File|*.txt|All Files|*",
                wx.SAVE) as dlg:
            if dlg.ShowModal() == wx.ID_OK:
                filename = dlg.GetPath()
                # Scrivere il contenuto del log su file
                with codecs.open(filename, 'w', encoding='utf-8') as f:
                    text = self.log.GetValue().encode("utf-8")
                    f.write(text)

    def OnClear(self, event):
        """Callback menu Clear: pulire il log"""
        self.log.Clear()

    def OnPortSettings(self, event):
        """Callback menu Serial Port > Port Settings: mostrare dialog impostazioni porta
        
        Presenta due opzioni:
        1. BLE: accedi all'indirizzo del device BLE tramite dialog
        2. Seriale: configura porta seriale (baudrate, data bits, parity, etc)
        """
        if self.ble_connection:
            # CONNESSIONE BLE
            dlg = TerminalSettingsDialog(self, -1, "", settings=self.settings, dev=self.dev)
            if dlg.ShowModal() == wx.ID_OK:
                self.ble_connection = True
                self.address = dlg.get_address()
                dlg.Destroy()
            
            # Creare/ricrecare DeviceControl se non esiste
            if self.dev is None:
                self.dev = DeviceControl(self.address, self.rx_queue, self.tx_queue)
            
            # Connettersi al device BLE (in un thread daemon)
            threading.Thread(
                target=lambda: asyncio.run(
                    self.dev.connect(self.address)
                ),
                daemon=True
            ).start()
        else:
            # CONNESSIONE SERIALE
            if event is not None:  # Saltare se chiamato all'avvio (event è None)
                self.StopThread()
                self.serial.close()
            
            ok = False
            while not ok:
                dlg = TerminalSettingsDialog(self, -1, "", settings=self.settings)
                
                # Dialog configurazione seriale (baudrate, format, flow control)
                with wxSerialConfigDialog.SerialConfigDialog(
                        self,
                        -1,
                        "",
                        show=wxSerialConfigDialog.SHOW_BAUDRATE | 
                             wxSerialConfigDialog.SHOW_FORMAT | 
                             wxSerialConfigDialog.SHOW_FLOW,
                        serial=self.serial) as dialog_serial_cfg:
                    dialog_serial_cfg.CenterOnParent()
                    result = dialog_serial_cfg.ShowModal()
                
                # Aprire porta seriale se OK oppure all'avvio
                if result == wx.ID_OK or event is not None:
                    try:
                        self.serial.open()
                    except serial.SerialException as e:
                        # Mostrare errore se porta non disponibile
                        with wx.MessageDialog(
                            self, str(e), "Serial Port Error",
                            wx.OK | wx.ICON_ERROR) as dlg:
                            dlg.ShowModal()
                    else:
                        # Porta aperta con successo: aggiornare titolo
                        self.SetTitle("Serial Terminal on {} [{},{},{},{}{}{}]".format(
                            self.serial.portstr,
                            self.serial.baudrate,
                            self.serial.bytesize,
                            self.serial.parity,
                            self.serial.stopbits,
                            ' RTS/CTS' if self.serial.rtscts else '',
                            ' Xon/Xoff' if self.serial.xonxoff else '',
                        ))
                        ok = True
                else:
                    ok = True

    def OnTermSettings(self, event):
        """Callback menu File > Terminal Settings: mostrare dialog impostazioni terminale
        
        Consente di configurare:
        - Echo locale (ripetere caratteri digitati)
        - Visualizzazione caratteri non stampabili
        - Modalità newline (CR, LF, CR+LF)
        """
        with TerminalSettingsDialog(self, -1, "", settings=self.settings) as dialog:
            dialog.CenterOnParent()
            dialog.ShowModal()

    async def OnKey(self, event):
        """Callback evento tastiera: inviare caratteri digitati alla porta
        
        Gestisce:
        - Caratteri ASCII: inviarli alla porta/BLE
        - Enter/Return (CR): applicare newline handling
        - Echo locale: se abilitato, mostrare i caratteri digitati nel log
        """
        code = event.GetUnicodeKey()
        
        if code == 13:  # CR (Return/Enter)
            if self.settings.echo:
                self.log.AppendText('\n')
            
            # Applicare la modalità newline corrente
            if self.settings.newline == NEWLINE_CR:
                self.Send(b'\r')     # Inviare CR
            elif self.settings.newline == NEWLINE_LF:
                self.Send(b'\n')     # Inviare LF
            elif self.settings.newline == NEWLINE_CRLF:
                self.Send(b'\r\n')   # Inviare CR+LF
        else:
            # Carattere ASCII normale
            char = unichr(code)
            if self.settings.echo:
                self.WriteText(char)  # Echo locale
            self.Send(char.lower())   # Inviare minuscolo
        
        event.StopPropagation()  # Non propagare l'evento a handler precedenti

    def WriteText(self, text):
        """Scrivere testo nel log, gestendo caratteri non stampabili
        
        Se unprintable è True, mostrare i caratteri di controllo in formato Unicode (U+24xx)
        """
        if self.settings.unprintable:
            # Convertire caratteri non stampabili in loro notazione Unicode
            text = ''.join([
                c if (c >= ' ' and c != '\x7f')  # Caratteri stampabili: maggiore di spazio e non DEL
                else unichr(0x2400 + ord(c))     # Non stampabili: mostrare come ␀, ␁, etc
                for c in text
            ])
        self.log.AppendText(text)

    async def OnSerialRead(self, event):
        """Callback evento SerialRxEvent: scrivere dati ricevuti nel log"""
        self.WriteText(event.data.decode('UTF-8', 'replace'))

    async def ComPortThread(self):
        """
        Coroutine asincrona che legge continuamente da porta seriale o BLE
        e pubblica i dati nel log tramite evento.
        
        Gestisce sia:
        - Ricezione da porta seriale (self.serial)
        - Ricezione da coda BLE (self.rx_queue)
        
        Applica anche newline transformation (CR -> LF, CRLF -> LF)
        """
        await asyncio.sleep(0.5)  # Delay iniziale
        
        while True:
            try:
                if not self.ble_connection and self.serial.is_open:
                    # LETTURA DA PORTA SERIALE
                    b = self.serial.read(self.serial.in_waiting or 1)
                    if b:
                        b = b.lower()  # Conversione a minuscolo
                        
                        # Trasformazione newline
                        if self.settings.newline == NEWLINE_CR:
                            b = b.replace(b'\r', b'\n')
                        elif self.settings.newline == NEWLINE_CRLF:
                            b = b.replace(b'\r\n', b'\n')
                        
                        # Pubblicare evento con i dati ricevuti
                        wx.PostEvent(self, SerialRxEvent(data=b))
                else:
                    # LETTURA DA CODA BLE (ricezione)
                    try:
                        msg = self.rx_queue.get_nowait()  # Non-blocking
                        # Pubblicare evento con i dati ricevuti
                        wx.PostEvent(self, SerialRxEvent(data=msg.encode()))
                    except queue.Empty:
                        pass  # Nessun dato disponibile
            except Exception as e:
                self.log.AppendText(f"[ERROR] ComPortThread: {e}\n")
            
            # Sleep per evitare di bloccare la GUI
            await asyncio.sleep(0.1)

    def OnRTS(self, event):
        """Callback menu Serial Port > RTS: attivare/disattivare segnale RTS"""
        self.serial.rts = event.IsChecked()

    def OnDTR(self, event):
        """Callback menu Serial Port > DTR: attivare/disattivare segnale DTR"""
        self.serial.dtr = event.IsChecked()

    def onLeft(self, event):
        """Callback pulsante Left (placeholder)"""
#        print("Event handler 'onLeft' not implemented!")
#        self.log.AppendText("Event Handler 'OnLeft' not implemented\n")
#        event.Skip()
        self.Move_Left(self.speed)

    def OnBackward(self, event):
        """Callback spin button DOWN: decrementa velocità (movimento indietro)"""
        if self.speed >= -90:
            self.speed -= 10  # Decrementa di 10
            self.Move(self.speed)
        else:
            self.log.AppendText("Maximum Speed reached\n")
        self.spin_move.Value = int(self.speed // 10)

    def OnForward(self, event):
        """Callback spin button UP: incrementa velocità (movimento avanti)"""
        if self.speed <= 90:
            self.speed += 10  # Incrementa di 10
            self.Move(self.speed)
        else:
            self.log.AppendText("Maximum speed reached!\n")
        self.spin_move.Value = int(self.speed // 10)

    def OnRight(self, event):
        """Callback pulsante Right (placeholder)"""
        # print("Event handler 'OnRight' not implemented!")
        # self.log.AppendText("Event handler 'OnRight' not implemented!\n")
        # event.Skip()
        self.Move_Right(self.speed)

    def OnStop(self, event):
        """Callback pulsante STOP: fermare il motore"""
        self.log.AppendText("Event handler 'OnStop' triggerred!\n")
        print("Sending Stop")
        self.Send("stop")  # Inviare comando stop
        self.speed = 0     # Resettare velocita
        self.speed_slider.SetValue(self.speed)
        self.spin_move.Value = self.speed

    def OnHelp(self, event):
        """Callback menu Help > List Commands: inviare comando 'help'"""
        print("Sending help")
        self.Send("help")

    def onSpeedSlider(self, event):
        """Callback slider velocità: inviare comando motore quando slider cambia"""
        foo = self.speed_slider.GetValue()
        
        if foo > 0:
            # Movimento avanti
            self.Move(foo)
            self.log.AppendText(f"Move forward at speed {foo}\n")
            print(f"Speed Slider: Move forward at speed {foo}")
            self.spin_move.SetValue(int(foo) // 10)
            self.speed = foo
        elif foo < 0:
            # Movimento indietro
            self.Move(foo)
            self.log.AppendText(f"Move backward at speed {-foo}\n")
            print(f"Speed Slider: Move backward at speed {foo}")
            self.spin_move.Value = int(foo) // 10
            self.speed = foo
        else:
            # Stop
            self.log.AppendText("Stopping motor\n")
            print("Sending Stop")
            self.Send("stop")
            self.speed = 0
            self.speed_slider.SetValue(self.speed)
            self.spin_move.Value = self.speed
        
        print(f'Spin Value -> {self.spin_move.Value}')

    def OnAbout(self, event):
        """Callback menu Help > About (placeholder)"""
        print("Event handler 'OnAbout' not implemented!")
        self.log.AppendText("Event handler 'OnAbout' not implemented!\n")
        event.Skip()

    def on_send(self, _):
        """Callback pulsante Send: inviare il comando inserito nel tx_txt_ctrl via BLE UART"""
        txt = self.tx_txt_ctrl.GetValue().strip()
        if txt:
            print(f"Sending via BLE to UART: {txt}")
            self.log.AppendText(f"Sending via BLE to UART: {txt}\n")
            self.tx_queue.put(txt)  # Mettere in coda di trasmissione
            self.log.AppendText(f"Tx: {txt}\n")
            self.tx_txt_ctrl.Clear()      # Pulire il campo di input

    def ont_timer(self, _):
        """Timer callback (unused): verificare ricezione dati da UART to BLE"""
        # Leggere messaggi dalla coda received (rx_queue)
        while not self.rx_queue.empty():
            msg = self.rx_queue.get()
            print(f"Received via UART to BLE: {msg}")
            self.log.AppendText(f"Received via UART to BLE: {msg}\n")
            self.log.AppendText(f"Rx: {msg}\n")
    
    def StopThread(self):
        """Placeholder: fermare il thread di lettura porta (non implementato)"""
        pass
    
# === FINE CLASS TerminalFrame ===


# ===== CONFIGURAZIONE MODALITÀ ASINCRONA =====
# Flag per abilitare/disabilitare modalità asincrona con wxasync

ASYNC = True

# ===== CLASS MyApp =====
# Applicazione wxPython standard (senza asincronia)

class MyApp(wx.App):
    """Applicazione wxPython di base (non asincrona)"""
    
    def OnInit(self):
        """Inizializzazione dell'applicazione"""
        # Creare la finestra principale TerminalFrame
        frame_terminal = TerminalFrame(None, -1, "")
        self.SetTopWindow(frame_terminal)
        frame_terminal.Show(True)
        return 1

# === FINE CLASS MyApp ===

# ===== MAIN ASINCRONO =====
# Funzione di entry point per modalità asincrona con wxasync

async def main_async():
    """Entry point asincrono: crea app wxPython asincrona e finestra principale"""
    # Creare app asincrona
    app = WxAsyncApp()
    
    # Creare finestra principale
    frame_terminal = TerminalFrame(None, -1, "")
    frame_terminal.Show(True)
    app.SetTopWindow(frame_terminal)
    
    # Avviare il main loop asincrono
    await app.MainLoop()

# ===== ENTRY POINT =====
# Punto di ingresso dell'applicazione

if __name__ == "__main__":
    if ASYNC:
        # Modalità asincrona: usare wxasync con asyncio
        asyncio.run(main_async(), debug=True)
    else:
        # Modalità standard: usare wxPython normale
        app = MyApp(0)
        app.MainLoop()