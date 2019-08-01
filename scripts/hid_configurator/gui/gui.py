#
# Copyright (c) 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic

import threading

from kivy.app import App
from kivy.clock import mainthread
from kivy.config import Config
from kivy.properties import ObjectProperty
from kivy.uix.button import Button
from kivy.uix.floatlayout import FloatLayout
from kivy.uix.gridlayout import GridLayout
from kivy.uix.popup import Popup
from plyer import notification

from gui_backend import Device


class MouseOptions(GridLayout):
    pass


class DfuButtons(GridLayout):
    def start_dfu_thread(self):
        app_ref = App.get_running_app()
        threading.Thread(target=app_ref.dfu, args=(self.update_progressbar,)).start()

    @mainthread
    def update_progressbar(self, permil):
        self.ids.pb.value = permil / 10 + 0.1


class DropDownButton(Button):
    pass


class Gui(App):
    def add_mouse_settings(self):
        mouse_options = MouseOptions()
        self.root.ids.mouse_options = mouse_options
        self.root.ids.possible_settings_place.add_widget(mouse_options)

    def add_dfu_buttons(self):
        dfu_buttons = DfuButtons()
        self.root.ids.dfu_buttons = dfu_buttons
        self.root.ids.dfu_buttons_place.add_widget(dfu_buttons)

    def clear_possible_settings(self):
        print("Clear possible settings")
        self.root.ids.possible_settings_place.clear_widgets()
        self.root.ids.dfu_buttons_place.clear_widgets()

    def setcpi_callback(self, value):
        self.device.setcpi(value)

    def fetchcpi_callback(self):
        returned = self.device.fetchcpi()
        if returned:
            return returned
        else:
            return 0

    def initialize_dropdown(self):
        device_list = Device.list_devices()
        for device in device_list:
            self.add_dropdown_button(device)

    def initialize_device(self, device_type):
        self.device = Device(device_type)
        print("Initialize {}".format(device_type))

    def show_possible_settings(self):
        label = self.root.ids.settings_label
        if self.device.type == 'keyboard':
            label.text = 'there are no available keyboard options for now'
            self.add_dfu_buttons()
        elif self.device.type == 'gaming_mouse':
            label.text = 'mouse options'
            self.add_mouse_settings()
            self.add_dfu_buttons()
        else:
            label.text = 'Unrecognised device'

    def add_dropdown_button(self, device_type):
        dropdown = self.root.ids.dropdown
        button = DropDownButton()
        button.text = device_type
        button.bind(on_release=lambda btn: dropdown.select(btn.text))
        button.bind(on_release=lambda btn: self.initialize_device(device_type))
        dropdown.add_widget(button)

    def clear_dropdown(self):
        dropdown = self.root.ids.dropdown
        dropdown.clear_widgets()
        grid_layout = GridLayout()
        dropdown.add_widget(grid_layout)

    def update_device_list(self):
        self.clear_dropdown()
        self.initialize_dropdown()

    def show_fwinfo(self):
        info = self.device.perform_fwinfo()
        if info:
            info = info.__str__()
        else:
            info = 'FW info request failed'

        settings_info = self.root.ids.settings_info_label
        settings_info.text = info
        settings_info.halign = 'left'
        dfu_info = self.root.ids.dfu_info_label
        dfu_info.text = info
        dfu_info.halign = 'left'

    def show_load_list(self):
        content = LoadDialog(load=self.load_list, cancel=self.dismiss_popup)
        self._popup = Popup(title="Load a file list", content=content, size_hint=(0.9, 0.9))
        self._popup.open()

    def load_list(self, files_path):
        print(files_path)
        self.filepath = files_path[0]
        self.root.ids.dfu_buttons.ids.choose_file_button.disabled = True
        self.root.ids.dfu_buttons.ids.start_uploading_button.disabled = False
        self.dismiss_popup()

    def dismiss_popup(self):
        self._popup.dismiss()

    def dfu(self, update_progressbar):
        dfu_label = self.root.ids.dfu_label
        dfu_label.text = 'Transfer in progress'
        print(self.filepath)
        success = self.device.perform_dfu(self.filepath, update_progressbar)
        if success:
            info = 'DFU transfer completed'
            message = 'Firmware will be switched on next reboot'
            self.root.ids.dfu_buttons.ids.reboot_firmware_button.disabled = False
        else:
            info = 'DFU transfer failed'
            message = 'Restart application and try again'
        print(info)
        dfu_label.text = info
        notification.notify(app_name='GUI Configurator', app_icon='nordic.ico', title=info, message=message)

    def perform_fwreboot(self):
        print('Performing fwreboot, please wait')
        self.device.perform_fwreboot()


class LoadDialog(FloatLayout):
    load = ObjectProperty(None)
    cancel = ObjectProperty(None)


if __name__ == '__main__':
    Config.set('input', 'mouse', 'mouse,multitouch_on_demand')  # turn off right click red dots
    Gui().run()
