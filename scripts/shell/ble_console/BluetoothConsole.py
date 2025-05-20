#!/usr/bin/env python
# Copyright (c) 2019 Nordic Semiconductor ASA
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
"""Desktop application - Bluetooth console for Linux to communicate with any NUS device"""

import time
import sys
import os

import gi
gi.require_version('Gtk', '3.0')
from gi.repository import Gtk, GObject, Gdk, GLib

gi.require_version('Vte', '2.91')
from gi.repository import Vte

from dbus import DBusException
from TerminalNotebook import TerminalNotebook
from BlueZ_communication import BluetoothConnection
from BlueZ_communication import GATT_CHAR_PATH
READ_CHARACTERISTIC = GATT_CHAR_PATH
MAX_SEND_LENGTH     = 20

class BluetoothConsole( TerminalNotebook, BluetoothConnection):

    def __init__(self):
        self.device_sending_dict = {}
        self.disconnected_devs   = []
        self.connected_devs      = []

        self.window = Gtk.Window()
        self.window.set_title('Bluetooth Console')
        self.window.set_default_size(730,410)
        self.window.set_resizable(True)
        #self.window.set_position(Gtk.WIN_POS_CENTER)

        if getattr(sys, 'frozen', False):
            # frozen (released)
            dir_name = os.path.dirname(sys.executable)
        else:
            # unfrozen
            dir_name = os.path.dirname(os.path.realpath(__file__))

        logo_path = os.path.join(dir_name, 'docs', 'nordic_semi_logo.png')

        self.window.set_icon_from_file(logo_path)
        self.window.connect('delete_event', self.delete)
        self.window.set_border_width(0)
        vbox = Gtk.VBox()
        self.window.add(vbox)
        self.text_window = Gtk.TextView(buffer = None)
        self.text_window.set_editable(False)
        self.text_window.set_cursor_visible(False)
        self.text_message = self.text_window.get_buffer()

        try:
            BluetoothConnection.__init__(self, self.bt_read_event, self.display_message)
        except DBusException:
            self.popup_warning("Bluetooth stack not reachable on D-Bus. Ensure Bluetooth device is connected and restart program.")

        TerminalNotebook.__init__(self, self.write_bluetooth, vbox, self.unconnect_from_menu, self.display_message)

        GLib.timeout_add(1000, self.update_device_list)

        menubar = Gtk.MenuBar()

        filemenu = Gtk.Menu()
        filem = Gtk.MenuItem("File")
        filem.set_submenu(filemenu)

        exit = Gtk.MenuItem("Exit")
        exit.connect("activate", Gtk.main_quit)
        filemenu.append(exit)

        self.connect_menu = Gtk.Menu()
        connect_item = Gtk.MenuItem("Select device")
        connect_item.set_submenu(self.connect_menu)

        menubar.append(filem)
        menubar.append(connect_item)

        vbox.pack_start(menubar, False, False, 0)
        vbox.pack_start(self.text_window, False, False, 0)
        self.text_window.set_size_request(-1,0)
        self.window.show_all()
        self.display_message("Welcome to Bluetooth Console by Nordic Semiconductor\r\nSelect device from context menu to connect", 8000)

    """Signal from BlueZ"""
    def bt_read_event(self, *args, **kwargs):
        if len(args) >= 2:
            if (args[0] == READ_CHARACTERISTIC) and ('Value' in args[1]):
                array = args[1]["Value"]
                temp_array = ''.join([chr(byte) for byte in array])
                # Show data received from BLE in terminal
                self.receive_data_from_device(self.export_name_from_rec_arg(kwargs['char_path']).split('dev_',1)[1], temp_array)

    def export_name_from_rec_arg(self, argument):
        return argument.split('/service',1)[0]

    def display_message(self, message, timeout = 3000):
        self.text_window.set_size_request(-1,-1)
        self.text_message.set_text(message)
        GLib.timeout_add(timeout, self.clr_display_message)

    def clr_display_message(self):
        self.text_message.set_text('')
        self.text_window.set_size_request(-1,0)
        return False

    def send_data(self):
        device_text_dict = self.get_data_from_terminal()
        for device in device_text_dict:
            if device_text_dict[device] != '':
                if len(device_text_dict[device]) > MAX_SEND_LENGTH:
                    self.write_bluetooth(device, device_text_dict[device][:MAX_SEND_LENGTH])
                    device_text_dict[device] = device_text_dict[device][MAX_SEND_LENGTH:]
                else:
                    self.write_bluetooth(device, device_text_dict[device])
                    device_text_dict[device] = ''
        time.sleep(0.01)
        return len(device_text_dict) > 0

    def unconnect_from_menu(self, device):
        for item in self.connect_menu:
            if device in item.get_label():
                item.set_active(False)
                return
        self.remove_terminal(device)

    def update_device_list(self):
        disconnected_devs, connected_devs = self.update_bluetooth_devices()

        if self.connected_devs != connected_devs:
            opened_devices = self.get_opened_terminals()

            for dev in self.connect_menu:
                self.connect_menu.remove(dev)

            for dev in connected_devs:
                dev_menu_item = Gtk.CheckMenuItem(self.get_device_name(dev) + ' (' + dev + ')')
                if dev in opened_devices:
                    dev_menu_item.set_active(True)
                    self.connect_to_device(dev)
                dev_menu_item.connect("activate", self.connect_disconnect)
                self.connect_menu.append(dev_menu_item)
                self.connect_menu.show_all()

            self.update_terminal_devices(disconnected_devs, connected_devs)

        # When there are no connected NUS devices, show info in menu
        items = [item for item in self.connect_menu]
        if not items:
            no_bonded_menu_item = Gtk.CheckMenuItem('No compatible devices connected')
            self.connect_menu.append(no_bonded_menu_item)
            self.connect_menu.show_all()

        self.disconnected_devs = disconnected_devs
        self.connected_devs = connected_devs

        time.sleep(0.01)
        return True

    def connect_disconnect(self, widget):
        if widget.get_active():
            self.connect(self.address_from_menu_label(widget.get_label()))
        else:
            self.disconnect(self.address_from_menu_label(widget.get_label()))

    def address_from_menu_label(self, label):
        return label.split('(',1)[1].split(')',1)[0]

    def connect(self, device):
        # every device increase send/receive frequency
        GLib.idle_add(self.send_data)
        self.connect_to_device(device)
        self.add_terminal(device, self.get_device_name(device))
        self.display_message("Connected to " + device)

    def disconnect(self, device):
        self.disconnect_from_device(device)
        self.remove_terminal(device)
        self.display_message("Disonnected from " + device)

    def popup_warning(self, text):
        popup = Gtk.MessageDialog(self.window, 0, Gtk.MessageType.WARNING,
                Gtk.ButtonsType.OK, text)
        popup.run()
        popup.destroy()

if __name__=='__main__':

    app = BluetoothConsole()
    Gtk.main()
