#!/usr/bin/env python3
# Copyright (c) 2019-2020 Nordic Semiconductor ASA
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
# flake8: noqa

import logging
import queue
import time
import socket
import sys
from _thread import *
import argparse

from pc_ble_driver_py import config
config.__conn_ic_id__ = "nrf52"

from nordicsemi.dfu.dfu_transport_ble import DFUAdapter
from pc_ble_driver_py.exceptions import NordicSemiException
from pc_ble_driver_py.ble_driver import Flasher, BLEDriver, BLEDriverObserver, BLEConfig, BLEConfigConnGatt,\
    BLEUUIDBase, BLEUUID, BLEAdvData, BLEGapConnParams
from pc_ble_driver_py.ble_driver import ATT_MTU_DEFAULT
from pc_ble_driver_py.ble_adapter import BLEAdapter, BLEAdapterObserver, EvtSync


CFG_TAG = 1

parser = argparse.ArgumentParser(description='BLE serial daemon.', allow_abbrev=False)
parser.add_argument('--com', help='COM port name (e.g. COM110).',
                    nargs='*', required=True, default="")
parser.add_argument('--snr', help='segger id.',
                    nargs=1, required=False, default="")
parser.add_argument('--family', help='chip family.',
                    nargs=1, required=False, default="")
parser.add_argument('--name', help='Device name (advertising name).',
                    nargs='*', required=False, default=None)
parser.add_argument('--port', help='Local port to use.',
                    required=False, default=8889)

args = parser.parse_args()

HOST = ''   # Symbolic name meaning all available interfaces
PORT =  int(args.port)

nrf_sd_ble_api_ver = config.sd_api_ver_get()
logger = logging.getLogger(__name__)


class BleSerial(BLEDriverObserver, BLEAdapterObserver):
    BASE_UUID = BLEUUIDBase([0x6E, 0x40, 0x00, 0x00, 0xB5, 0xA3, 0xF3, 0x93,
                               0xE0, 0xA9, 0xE5, 0x0E, 0x24, 0xDC, 0xCA, 0x9E])

    RX_UUID = BLEUUID(0x0003, BASE_UUID)
    TX_UUID = BLEUUID(0x0002, BASE_UUID)
    LOCAL_ATT_MTU = 23

    def __init__(self, com_port, periph_name, s_conn):
        driver = BLEDriver(serial_port=com_port, baud_rate=1000000)
        adapter = BLEAdapter(driver)
        self.evt_sync = EvtSync(['connected', 'disconnected'])
        self.target_device_name = periph_name
        self.target_device_addr = 0
        self.conn_handle = None
        self.adapter = adapter
        self.notifications_q = queue.Queue()
        self.adapter.observer_register(self)
        self.adapter.driver.observer_register(self)
        self.s_conn = s_conn
        self.adapter.driver.open()

        if nrf_sd_ble_api_ver >= 3:
            logger.info("\nBLE: ble_enable with local ATT MTU: {}".format(DFUAdapter.LOCAL_ATT_MTU))

        gatt_cfg = BLEConfigConnGatt()
        gatt_cfg.att_mtu = self.adapter.default_mtu
        gatt_cfg.tag = CFG_TAG
        self.adapter.driver.ble_cfg_set(BLEConfig.conn_gatt, gatt_cfg)

        self.adapter.driver.ble_enable()
        self.adapter.driver.ble_vs_uuid_add(BleSerial.BASE_UUID)

        self.connect()

    def connect(self):
        logger.debug('BLE: connect: target address: 0x{}'.format(self.target_device_addr))
        logger.info('BLE: Scanning...')
        self.adapter.driver.ble_gap_scan_start()
        self.conn_handle = self.evt_sync.wait('connected')
        if self.conn_handle is None:
            raise NordicSemiException('Timeout. Target device not found.')
        logger.info('BLE: Connected to target')
        logger.debug('BLE: Service Discovery')

        if nrf_sd_ble_api_ver >= 3:
            if DFUAdapter.LOCAL_ATT_MTU > ATT_MTU_DEFAULT:
                logger.info('BLE: Enabling longer ATT MTUs')
                self.att_mtu = self.adapter.att_mtu_exchange(self.conn_handle, BleSerial.LOCAL_ATT_MTU)
                logger.info('BLE: ATT MTU: {}'.format(self.att_mtu))
            else:
                logger.info('BLE: Using default ATT MTU')

        self.adapter.service_discovery(conn_handle=self.conn_handle)
        logger.debug('BLE: Enabling Notifications')
        self.adapter.enable_notification(conn_handle=self.conn_handle, uuid=BleSerial.RX_UUID)
        return self.target_device_name, self.target_device_addr

    def on_gap_evt_connected(self, ble_driver, conn_handle, peer_addr, role, conn_params):
        self.evt_sync.notify(evt = 'connected', data = conn_handle)

    def on_gap_evt_disconnected(self, ble_driver, conn_handle, reason):
        self.evt_sync.notify(evt = 'disconnected', data = conn_handle)
        logger.info('BLE: Disconnected...')
        self.connect()

    def on_gap_evt_adv_report(self, ble_driver, conn_handle, peer_addr, rssi, adv_type, adv_data):
        dev_name_list = []
        if BLEAdvData.Types.complete_local_name in adv_data.records:
            dev_name_list = adv_data.records[BLEAdvData.Types.complete_local_name]

        elif BLEAdvData.Types.short_local_name in adv_data.records:
            dev_name_list = adv_data.records[BLEAdvData.Types.short_local_name]

        dev_name        = "".join(chr(e) for e in dev_name_list)
        address_string  = "".join("{0:02X}".format(b) for b in peer_addr.addr)
        logger.debug('Received advertisement report, address: 0x{}, device_name: {}'.format(address_string, dev_name))

        if dev_name == self.target_device_name:
            conn_params = BLEGapConnParams(min_conn_interval_ms = 15,
                                           max_conn_interval_ms = 30,
                                           conn_sup_timeout_ms  = 4000,
                                           slave_latency        = 0)
            logger.info('BLE: Found target advertiser, address: 0x{}, name: {}'.format(address_string, dev_name))
            logger.info('BLE: Connecting to 0x{}'.format(address_string))
            self.adapter.connect(address = peer_addr, conn_params = conn_params)
            # store the name and address for subsequent connections

    def on_notification(self, ble_adapter, conn_handle, uuid, data):
        if self.conn_handle         != conn_handle: return
        if BleSerial.RX_UUID.value != uuid.value:  return

        logger.debug("Received notification {}".format(len(data)))

        # send to the socket
        buf = [chr(x) for x in data]
        buf = ''.join(buf)
        buf = bytes(buf, 'utf-8')
        self.s_conn.sendall(buf)

    def write_data(self, data):
        self.adapter.write_req(self.conn_handle, BleSerial.TX_UUID, data)


# Function for handling connections. This will be used to create threads
def client_thread(conn, ble_serial):
    # Sending message to connected client
    conn.send(b'Welcome to the server. Type something and hit enter\n')

    # infinite loop so that function do not terminate and thread do not end.
    while True:
        # Receiving from client
        data = conn.recv(1024)
        outbuf = [ord(x) for x in data.decode('utf-8')]
        ble_serial.write_data(outbuf)

    # came out of loop
    conn.close()


logging.basicConfig(format='%(message)s', level=logging.INFO)


def flash_connectivity(comport, jlink_snr):
    flasher = Flasher(serial_port=comport, snr=jlink_snr)
    if flasher.fw_check():
        print("Board already flashed with connectivity firmware.")
    else:
        print("Flashing connectivity firmware...")
        flasher.fw_flash()
        print("Connectivity firmware flashed.")
    flasher.reset()
    time.sleep(1)


def run_bt_nus_server(com_port, name="BT_NUS_shell"):
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    print('Socket created')

    # Bind socket to local host and port
    try:
        s.bind((HOST, PORT))
    except OSError:
        print('Bind failed')
        s.close()
        sys.exit()

    print('Socket bind complete')

    # Start listening on socket
    s.listen(10)
    print('Socket now listening')

    # now keep talking with the client
    try:
        while 1:
            # wait to accept a connection - blocking call
            conn, _addr = s.accept()
            try:
                b = BleSerial(com_port, name, conn)
            except NordicSemiException as msg:
                print('Starting BLESerial failed. Error: ' + str(msg))
                break
            print('BLE serial started')

        # start new thread takes 1st argument as a function to be run, second is the tuple of arguments to the function.
            start_new_thread(client_thread, (conn, b))

    finally:
        s.close()
        print("SOCKET CLOSED")

if args.snr:
    flash_connectivity(args.com[0], args.snr[0])

if args.name:
    args.name = ' '.join(args.name)
    run_bt_nus_server(args.com[0], args.name)
else:
    run_bt_nus_server(args.com[0])
