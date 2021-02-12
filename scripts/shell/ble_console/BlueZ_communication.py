# Copyright (c) 2019 Nordic Semiconductor ASA
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

import dbus
import dbus.service
import sys
import subprocess

from xml.etree import ElementTree
from dbus.mainloop.glib import DBusGMainLoop

NUS_UUID             = '6e400001-b5a3-f393-e0a9-e50e24dcca9e'
GATT_CHAR_PATH       = 'org.bluez.GattCharacteristic1'
DBUS_PROP_PATH       = 'org.freedesktop.DBus.Properties'
GATT_SERV_PATH       = 'org.bluez.GattService1'
DBUS_INTROSPECT_PATH = 'org.freedesktop.DBus.Introspectable'
BLUEZ_DEV_PATH       = 'org.bluez.Device1'
BLUEZ_PATH           = 'org.bluez'

class MyException(Exception):
    def _get_message(self):
        return self._message
    def _set_message(self, message):
        self._message = message
    message = property(_get_message, _set_message)

class BluetoothDevice(object):

    def __init__(self, path):
        self.name = ''
        self.service_path = path
        self.path = ''
        self.tx_interface = ''
        self.rx_interface = ''
        self.connected  = False

class BluetoothConnection(object):

    def __init__(self, read_handler, message):
        DBusGMainLoop(set_as_default=True)
        self.display_message = message
        self.bus = dbus.SystemBus()
        self.object_path_list = []
        self.device_list = []
        self.update_devices()
        self.bus.add_signal_receiver(read_handler, path_keyword = 'char_path')
        (self.bluez_maj, self.bluez_min) = self.get_bluez_version()

    def get_bluez_version(self):
	bashCommand = 'bluetoothd -v'
        process = subprocess.Popen(bashCommand.split(), stdout=subprocess.PIPE)
        output, error = process.communicate()
	output = output.split('.')
	return (int(output[0],10), int(output[1],10))

    def get_device_name(self, device):
        for dev in self.device_list:
            if device in dev.path:
                return dev.name

    def connect_to_device(self, device):
        for dev in self.device_list:
            if device in dev.path:
                self.enable_notification(dev)

    def disconnect_from_device(self, device):
        for dev in self.device_list:
            if device in dev.path:
                self.disable_notification(dev)

    def update_devices(self):
        self.device_list = []
        self.object_path_list = []
        self.find_services()
        for device in self.device_list:
            self.find_communication_interfaces(device)
            device.path = device.service_path.split('/service',1)[0]
            char_proxy = self.bus.get_object(BLUEZ_PATH, device.path)
            iface = dbus.Interface(char_proxy, DBUS_PROP_PATH)
            device.connected = iface.Get(BLUEZ_DEV_PATH, 'Connected')
            device.name = iface.Get(BLUEZ_DEV_PATH, 'Name')

    def update_bluetooth_devices(self):
        self.update_devices()
        disconnected_devs = []
        connected_devs = []
        for device in self.device_list:
            if device.connected:
                connected_devs.append(device.path.split('/dev_',1)[1])
            else:
                disconnected_devs.append(device.path.split('/dev_',1)[1])
        return disconnected_devs, connected_devs

    def find_communication_interfaces(self, device):
        obj = self.bus.get_object(BLUEZ_PATH, device.service_path)
        iface = dbus.Interface(obj, DBUS_INTROSPECT_PATH)
        xml_string = iface.Introspect()
        char_list = []
        for child in ElementTree.fromstring(xml_string):
            if child.tag == 'node':
                char_list.append('/'.join((device.service_path, child.attrib['name'])))
        for char_path in char_list:
            char_proxy = self.bus.get_object(BLUEZ_PATH, char_path)
            iface = dbus.Interface(char_proxy, DBUS_PROP_PATH)
            flags = iface.Get(GATT_CHAR_PATH, 'Flags')
            for flag in flags:
                if flag == 'notify':
                    device.tx_interface = dbus.Interface(char_proxy, GATT_CHAR_PATH)
                if flag == 'write':
                    device.rx_interface = dbus.Interface(char_proxy, GATT_CHAR_PATH)

    def find_services(self):
        self.list_objects()
        service_path_list_buf = []
        service_path_list = []
        for obj in self.object_path_list:
            if 'service' in obj:
                service_path_list_buf.append(obj)
        for serv in service_path_list_buf:
            if 'char'  not in serv:
                service_path_list.append(serv)
        for serv in service_path_list:
            if self.validate_service_path(NUS_UUID, serv):
                self.device_list.append(BluetoothDevice(serv))

    def validate_service_path(self, uuid, path):
        service_proxy = self.bus.get_object(BLUEZ_PATH, path)
        iface = dbus.Interface(service_proxy, DBUS_PROP_PATH)
        uuid_ = iface.Get(GATT_SERV_PATH, 'UUID')
        if uuid_ == uuid:
            return path

    def list_objects(self, service = BLUEZ_PATH, object_path = '/'):
        self.object_path_list.append(object_path)
        if BLUEZ_PATH in self.bus.list_activatable_names():
            try:
                obj = self.bus.get_object(service, object_path)
                iface = dbus.Interface(obj, DBUS_INTROSPECT_PATH)
                xml_string = iface.Introspect()
                for child in ElementTree.fromstring(xml_string):
                    if child.tag == 'node':
                        if object_path == '/':
                            object_path = ''
                        new_path = '/'.join((object_path, child.attrib['name']))
                        self.list_objects(service, new_path)
            except dbus.DBusException as e:
                if "Not connected" in e.args:
                    self.display_message("Not connected to " + device)
                elif "Did not receive a reply" in e.args:
                    self.display_message("Timeout exceeded " + device)
                elif "The name" in e.args[0]:
                    pass
                else:
                    raise

    def write_bluetooth(self, device, text):
        message = bytearray()
        if '\r' in text:
            message.extend(text + '\r\n')
        else:
            message.extend(text)
        for dev in self.device_list:
            if device in dev.path:
                try:
                    if (self.bluez_maj > 5 or (self.bluez_maj == 5
			and self.bluez_min > 39)):
			dev.rx_interface.WriteValue(message, {})
                    else:
                        dev.rx_interface.WriteValue(message)
                except dbus.DBusException as e:
                    if "Not connected" in e.args:
                        self.display_message("Not connected with " + device)
                    elif "Did not receive a reply" in e.args[0]:
                        self.display_message("Timeout exceeded :" + device)
                    else:
                        print e.args

    def enable_notification(self, device):
        try:
            device.tx_interface.StartNotify()
        except dbus.DBusException as e:
            if "Not connected" in e.args:
                self.display_message("Not connected with " + device)
            elif "Did not receive a reply" in e.args:
                self.display_message("Timeout exceeded :" + device)
            elif "Already notifying" in e.args:
                pass
            else:
                raise


    def disable_notification(self, device):
        try:
            device.tx_interface.StopNotify()
        except dbus.DBusException as e:
            if "Not connected" in e.args:
                self.display_message("Not connected with " + device)
            elif "Did not receive a reply" in e.args:
                self.display_message("Timeout exceeded " + device)
            else:
                raise
