# Copyright (c) 2019 Nordic Semiconductor ASA
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

import os
import sys
import warnings
import re

import gi
gi.require_version('Gtk', '3.0')
from gi.repository import Gtk, GObject, Gdk, GLib

gi.require_version('Vte', '2.91')
from gi.repository import Vte

COLOR_TERMINAL_ACTIVE = Gdk.RGBA(0.0/256, 0.0/256, 0.0/256, 0.8)
COLOR_TERMINAL_INACTIVE = Gdk.RGBA(200.0/256, 200.0/256, 200.0/256, 0.8)
MARKUP_REMOVE = re.compile(r'<[^>]+>')

class TerminalNotebook(object):

    def __init__(self, write_device, widget, unconnect, message):

        # Save images used in terminals

        if getattr(sys, 'frozen', False):
            # frozen (released)
            dir_name = os.path.dirname(sys.executable)
        else:
            # unfrozen
            dir_name = os.path.dirname(os.path.realpath(__file__))

        self.device_terminal_dict = {}
        self.device_string_dict   = {}
        self.labels = {}

        # Methods to communicate with sending protocol and gui
        self.send_data_to_device = write_device
        self.unconnect_menu = unconnect
        self.display_message = message

        # Create a new notebook, place the position of the tabs
        self.notebook = Gtk.Notebook()
        self.notebook.set_tab_pos(Gtk.PositionType.TOP)
        widget.pack_start(self.notebook, True, True, 0)
        self.notebook.show()
        self.show_tabs   = True
        self.show_border = True

        # Create empty terminal
        terminal = Vte.Terminal()
        terminal.set_color_background(COLOR_TERMINAL_INACTIVE)
        terminal.set_size_request(1, 1)
        terminal.show()

        self.notebook.append_page(terminal)
        self.notebook.set_show_tabs(False)
        self.notebook.set_scrollable(True)
        self.notebook.set_current_page(0)

## EVENT HANDLERS ##
    def read_terminal_signal(self, terminal, text, size):
        for device in self.device_terminal_dict:
            if self.device_terminal_dict[device] == terminal:
                self.device_string_dict[device] += text

    def receive_data_from_device(self, device, string):
        self.device_terminal_dict[device].feed(string)

    def get_data_from_terminal(self):
        return self.device_string_dict
## END OF EVENT HANDLERS ##

    def update_terminal_devices(self, disconnected, connected):
        for device in disconnected:
            for dev in self.device_terminal_dict:
                if dev == device:
                    self.activate_terminal(self.device_terminal_dict[dev], False)

        for device in connected:
            for dev in self.device_terminal_dict:
                if dev == device:
                    self.activate_terminal(self.device_terminal_dict[dev], True)

        for device in self.device_terminal_dict:
            if (device not in disconnected) and (device not in connected):
                self.activate_terminal(self.device_terminal_dict[device], False)

    def activate_terminal(self, terminal, state):
        for device in self.device_terminal_dict:
            if self.device_terminal_dict[device] == terminal:
                label_text = self.labels[device].get_text()

                if state == False :
                    terminal.set_color_background(COLOR_TERMINAL_INACTIVE)
                    self.display_message("Lost connection with " + device)
                    # Strikethrough name of inactive device
                    self.labels[device].set_markup('<s>' + label_text + '</s>')
                else:
                    terminal.set_color_background(COLOR_TERMINAL_ACTIVE)
                    # Clear strikethrough
                    self.labels[device].set_markup(MARKUP_REMOVE.sub('', label_text))

    def get_opened_terminals(self):
        opened_terminals = []
        for device in self.device_terminal_dict:
            opened_terminals.append(device)
        return opened_terminals

    def remove_terminal(self, device):
        self.device_terminal_dict[device].feed('\033[2J')
        if self.notebook.get_n_pages() != 1:
	        self.notebook.remove_page(self.notebook.page_num(self.device_terminal_dict[device]))
	        self.notebook.queue_draw_area(0,0,-1,-1)
        else:
            self.device_terminal_dict[device].set_color_background(COLOR_TERMINAL_INACTIVE)
        del self.device_sending_dict[device]
        del self.device_terminal_dict[device]
        del self.device_string_dict[device]
        self.notebook.set_show_tabs(len(self.device_terminal_dict) != 0)

    def remove_terminal_button_sig(self, sender, device):
        self.unconnect_menu(device)

    def add_terminal(self, device, name):
        if len(self.device_terminal_dict) == 0:
            page = self.notebook.get_current_page()
            self.notebook.remove_page(page)

        terminal = Vte.Terminal()
        terminal.connect('commit', self.read_terminal_signal)
        terminal.connect('key_press_event', self.copy_or_paste)
        terminal.set_color_background(COLOR_TERMINAL_ACTIVE)
        terminal.set_size_request(1, 1)
        terminal.show()

        self.device_terminal_dict[device] = terminal
        self.device_sending_dict[device] = 0
        self.device_string_dict[device] = ''

        self.labels[device] = Gtk.Label(name + ' (' + device + ')')

        hbox = Gtk.HBox(False, 0)
        hbox.pack_start(self.labels[device], expand=True, fill=True, padding=0)

        # Make the close button
        btn = Gtk.Button()
        btn.set_relief(Gtk.ReliefStyle.NONE)
        btn.set_focus_on_click(False)
        close_image = Gtk.Image.new_from_icon_name("window-close", Gtk.IconSize.MENU)
        btn.add(close_image)
        btn.connect('clicked', self.remove_terminal_button_sig, device)
        hbox.pack_start(btn, False, False, 0)

        hbox.show_all()

        self.notebook.append_page(terminal, hbox)
        self.notebook.set_current_page(self.notebook.get_n_pages() - 1)
        self.notebook.set_show_tabs(True)
        # Clear terminal
        self.send_data_to_device(device, '\n')

    def copy_or_paste(self, widget, event):
        """Decides if the Ctrl+Shift is pressed, in which case returns True.
        If Ctrl+Shift+C or Ctrl+Shift+V are pressed, copies or pastes,
        acordingly. Return necessary so it doesn't perform other action,
        like killing the process, on Ctrl+C."""
        control_key = Gdk.ModifierType.CONTROL_MASK
        shift_key   = Gdk.ModifierType.SHIFT_MASK
        if event.type == Gdk.EventMask.KEY_PRESS_MASK:
            if event.state & (shift_key | control_key) == shift_key | control_key: # shift & control
                if event.keyval == 67: # that's the C key
                    widget.copy_clipboard()
                elif event.keyval == 86: # and that's the V key
                    widget.paste_clipboard()
                return True

    def delete(self, widget, event=None):
        Gtk.main_quit()
        return False
