#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

import logging
import argparse
import time
import requests

from pc_ble_driver_py import config

config.__conn_ic_id__ = "nrf52"

from pc_ble_driver_py.exceptions import NordicSemiException
from pc_ble_driver_py.ble_driver import BLEDriver, BLEDriverObserver, BLEUUIDBase, BLEUUID, Flasher
from pc_ble_driver_py.ble_driver import BLEConfig, BLEConfigConnGatt, BLEAdvData, BLEGapConnParams
from pc_ble_driver_py.ble_driver import BLEGattStatusCode, BLEGapSecStatus, BLEHci
from pc_ble_driver_py.ble_adapter import BLEAdapter, BLEAdapterObserver, EvtSync

logger = logging.getLogger(__name__)

BLE_GAP_PHY_2MBPS = 0x02
MDS_UUID = [0x54, 0x22, 0x00, 0x00, 0xf6, 0xa5, 0x40, 0x07, 0xa3, 0x71, 0x72, 0x2f, 0x4e, 0xbd,
            0x84, 0x36]


def device_flash(comm, snr, erase):
    """ Flash device with the connectivity firmware if needed.
        This function check first if device has already a valid firmware.
    """

    flasher = Flasher(comm, snr)

    if erase:
        logger.info("Erasing...")
        flasher.erase()
        logger.info("Erased")

    if flasher.fw_check():
        logger.info("Device is already flashed with connectivity firmware")
        logger.info("Restarting device...")
        flasher.reset()
        time.sleep(1)
    else:
        logger.info("Flashing firmware...")

        flasher.fw_flash()

        logger.info("Firmware flashed")
        logger.info("Restarting...")

        flasher.reset()
        time.sleep(1)


class BLEMemfault(BLEDriverObserver, BLEAdapterObserver):
    """
        A class used to represent a MDS client instance.
        It is used to perform connection with device running the MDS service.
    """

    ERROR_NOCONN = -1
    DEFAULT_MTU = 247
    CFG_TAG = 1

    BASE_UUID = BLEUUIDBase(MDS_UUID)
    MDS_SVC_UUID = BLEUUID(0x0000, BASE_UUID)
    SUPPORTED_FEATURES_UUID = BLEUUID(0x0001, BASE_UUID)
    DEVICE_IDENTIFIER_UUID = BLEUUID(0x0002, BASE_UUID)
    DATA_URI_UUID = BLEUUID(0x0003, BASE_UUID)
    AUTHORIZATION_UUID = BLEUUID(0x0004, BASE_UUID)
    DATA_EXPORT = BLEUUID(0x0005, BASE_UUID)

    DATA_STREAM_ENABLE = [0x01]
    DATA_STREAM_DISABLE = [0x00]

    def __init__(self, comm, bond, conn_timeout):
        driver = BLEDriver(serial_port=comm, baud_rate=1000000)
        adapter = BLEAdapter(driver)

        self.adapter = adapter
        self.conn_timeout = conn_timeout
        self.bond = bond
        self.evt_sync = EvtSync(['connected', 'disconnected'])

        self.adapter.observer_register(self)
        self.adapter.driver.observer_register(self)
        self.adapter.driver.open()

        self.conn_handle = None
        self.keys = None
        self.att_mtu = self.adapter.default_mtu

        gatt_cfg = BLEConfigConnGatt()
        gatt_cfg.att_mtu = BLEMemfault.DEFAULT_MTU
        gatt_cfg.conn_cfg_tag = BLEMemfault.CFG_TAG

        self.adapter.driver.ble_cfg_set(BLEConfig.conn_gatt, gatt_cfg)
        self.adapter.driver.ble_enable()
        self.adapter.driver.ble_vs_uuid_add(BLEMemfault.BASE_UUID)

    def data_export_received(self, data):
        pass

    def connect(self):
        logger.debug('BLE: Scanning...')
        self.adapter.driver.ble_gap_scan_start()
        self.conn_handle = self.evt_sync.wait('connected', timeout=self.conn_timeout)

        if self.conn_handle is None:
            raise NordicSemiException('Timeout. BLE Memfault target device not found',
                                      BLEMemfault.ERROR_NOCONN)

        logger.info('BLE: Connected to the Memfault target')

        # Try to set longer ATT MTU
        self.att_mtu = self.adapter.att_mtu_exchange(self.conn_handle, BLEMemfault.DEFAULT_MTU)
        logging.info(f'BLE: Current ATT MTU: {self.att_mtu}')

        # Try to use 2Mb PHY
        status = self.adapter.phy_update(self.conn_handle, [BLE_GAP_PHY_2MBPS, BLE_GAP_PHY_2MBPS])
        if status['status'] == BLEHci.success:
            logger.info(f"BLE: Phy updated tx_phy: {status['tx_phy']} rx_phy: {status['rx_phy']}")

        self.adapter.service_discovery(self.conn_handle, BLEMemfault.MDS_SVC_UUID)

        if not self.keys:
            self.adapter.authenticate(conn_handle=self.conn_handle,
                                      _role=self.adapter.db_conns[self.conn_handle].role,
                                      bond=self.bond)

            # Simulate a bonding, bond will be stored during a lifetime of this script.
            if self.bond:
                self.keys = self.adapter.db_conns[self.conn_handle]._keyset
        else:
            enc_key = self.keys.keys_peer.enc_key
            self.adapter.encrypt(self.conn_handle, enc_key.master_id.ediv, enc_key.master_id.rand,
                                 enc_key.enc_info.ltk, enc_key.enc_info.auth)

    def disconnect(self):
        self.adapter.disconnect(self.conn_handle)

    def read_supported_features(self):
        status = self.adapter.read_req(self.conn_handle, BLEMemfault.SUPPORTED_FEATURES_UUID)
        if status[0] != BLEGattStatusCode.success:
            raise NordicSemiException(f'Read supported features returned error status: {status[0]}')

        return status[1]

    def read_device_identifier(self):
        status = self.adapter.read_req(self.conn_handle, BLEMemfault.DEVICE_IDENTIFIER_UUID)
        if status[0] != BLEGattStatusCode.success:
            raise NordicSemiException(f'Read device identifier returned error status: {status[0]}')

        device_identifier = "".join(chr(c) for c in status[1])

        return device_identifier

    def read_data_uri(self):
        status = self.adapter.read_req(self.conn_handle, BLEMemfault.DATA_URI_UUID)
        if status[0] != BLEGattStatusCode.success:
            raise NordicSemiException(f'Read data URI returned error status: {status[0]}')

        data = status[1]

        while status[0] == BLEGattStatusCode.success:
            offset = len(data)

            if len(status[1]) < (self.att_mtu - 3):
                break

            status = self.adapter.read_req(self.conn_handle, BLEMemfault.DATA_URI_UUID,
                                           offset=offset)
            if status[0] == BLEGattStatusCode.success:
                data.extend(status[1])

        uri = "".join(chr(c) for c in data)

        return uri

    def read_authorization(self):
        status = self.adapter.read_req(self.conn_handle, BLEMemfault.AUTHORIZATION_UUID)
        if status[0] != BLEGattStatusCode.success:
            raise NordicSemiException(f'Read authorization returned error status: {status[0]}')

        data = status[1]

        while status[0] == BLEGattStatusCode.success:
            offset = len(data)

            if len(status[1]) < (self.att_mtu - 3):
                break

            status = self.adapter.read_req(self.conn_handle, BLEMemfault.AUTHORIZATION_UUID,
                                           offset=offset)
            if status[0] == BLEGattStatusCode.success:
                data.extend(status[1])

        project_key = "".join(chr(c) for c in data)

        return project_key

    def data_export_notification_enable(self):
        self.adapter.enable_notification(conn_handle=self.conn_handle, uuid=BLEMemfault.DATA_EXPORT)

    def data_export_write(self, data):
        self.adapter.write_req(self.conn_handle, BLEMemfault.DATA_EXPORT, data)

    def close(self):
        self.adapter.close()

    def on_gap_evt_adv_report(self, ble_driver, conn_handle, peer_addr, rssi, adv_type, adv_data):
        uuid = 0

        if BLEAdvData.Types.service_128bit_uuid_complete in adv_data.records:
            uuid = adv_data.records[BLEAdvData.Types.service_128bit_uuid_complete]
            uuid.reverse()

            logger.info('BLE: Received advertising packet with the MDS UUID')

        address = "".join(f'{b:02X}:' for b in peer_addr.addr)
        address = address[:-1]

        logger.debug(f'BLE: Received advertisement report, address: {address}')

        if uuid == MDS_UUID:
            conn_params = BLEGapConnParams(min_conn_interval_ms = 15,
                                           max_conn_interval_ms = 30,
                                           conn_sup_timeout_ms  = 4000,
                                           slave_latency        = 0)

            logger.info(f'BLE: Connecting to {address}')
            self.adapter.connect(address=peer_addr, conn_params=conn_params,
                                 tag=BLEMemfault.CFG_TAG)

    def on_gap_evt_connected(self, ble_driver, conn_handle, peer_addr, role, conn_params):
        self.evt_sync.notify('connected', data=conn_handle)

    def on_gap_evt_disconnected(self, ble_driver, conn_handle, reason):
        self.evt_sync.notify('disconnected', data=conn_handle)
        logger.info(f'BLE: Disconnected (reason: {reason})')

    def on_gap_evt_sec_request(self, ble_driver, conn_handle, bond, mitm, lesc, keypress):
        if conn_handle != self.conn_handle:
            return

        logger.info(f'BLE: Security request, bond: {bond}')

    def on_gap_evt_conn_sec_update(self, ble_driver, conn_handle, conn_sec):
        if conn_handle != self.conn_handle:
            return

        logger.info(f'BLE: Security changed level: {conn_sec.sec_mode.lv}')

    def on_gap_evt_auth_status(self, ble_driver, conn_handle, **kwargs):
        if conn_handle != self.conn_handle:
            return

        status = kwargs.get('auth_status')
        if status == BLEGapSecStatus.success:
            status = 'Success'
        else:
            status = 'Failed'

        logger.info(f'BLE: Authentication status conn_handle: {conn_handle} status: {status}')

    def on_notification(self, ble_adapter, conn_handle, uuid, data):
        if conn_handle != self.conn_handle:
            return

        if uuid != BLEMemfault.DATA_EXPORT:
            return

        logger.debug(f'Received data export notification, data length: {len(data)}')

        self.data_export_received(data)


class Memfault(BLEMemfault):
    """
        Memfault class performs operations on the MDS service to get necessary data to forward
        a diagnostic data to the cloud.
    """

    MAX_CHUNK_NUMBER = 31

    def __init__(self, comm, bond=True, conn_timeout=5):
        super().__init__(comm, bond, conn_timeout)

        self.chunk_counter = 0
        self.expected_chunk_number = 0
        self.reconnection = False

    def connect(self):
        super().connect()

        self.device_identifier = self.read_device_identifier()
        self.uri = self.read_data_uri()
        self.project_key = self.read_authorization()
        self.supported_features = self.read_supported_features()

        logger.info(f'Device identifier {self.device_identifier}, '
                    f'supported features: {self.supported_features}')
        logger.info('MDS Data Export: Enabling data stream')

        self.data_export_notification_enable()
        self.data_export_write(Memfault.DATA_STREAM_ENABLE)

        logger.info('MDS Data Export: Data stream enabled')

        self.reconnection = True

    def wait_for_disconnection(self):
        self.evt_sync.wait('disconnected', timeout=None)
        self.expected_chunk_number = 0

    def data_export_received(self, data):
        if len(data) < 1:
            return

        chunk_cn = data.pop(0) & 0x1F

        logger.debug(f'Received chunk data CN: {chunk_cn}, data: {data}')

        if chunk_cn != self.expected_chunk_number:
            logger.warning('Invalid chunk number, data lost or duplicated packet! '
                    f'Expected chunk number was {self.expected_chunk_number} but got {chunk_cn}')

        self.expected_chunk_number = (self.expected_chunk_number + 1) % \
                                     (Memfault.MAX_CHUNK_NUMBER + 1)
        self._upload_chunk(data)

        self.chunk_counter += 1

        self.print_progress(self.chunk_counter, self.reconnection)

        self.reconnection = False

    def _upload_chunk(self, data):
        headers = {}

        key = self.project_key.split(':')
        auth_key = key[0]
        auth_value = key[1]

        headers[auth_key] = auth_value
        headers['Content-Type'] = 'application/octet-stream'

        try:
            response = requests.post(self.uri, headers=headers, data=bytes(data))
            response.raise_for_status()
        except requests.HTTPError as err:
            logger.info(f'{err}')
            self.adapter.disconnect(self.conn_handle)

    @staticmethod
    def print_progress(chunk_number, reconnection):
        dots_num = (chunk_number % 4)
        dots = ''.join('.' * dots_num)

        if chunk_number == 1 or reconnection:
            print()
        else:
            print('\33[3A')

        print(f'\33[2K\rSending {dots}')
        print(f'\rForwarded \033[92m{chunk_number}\033[39m Memfault Chunks')


def parse_args():
    """ Parse command line arguments. """

    parser = argparse.ArgumentParser(description='Memfault BLE gateway',
                                     allow_abbrev=False)
    parser.add_argument('--snr', type=str, required=True, help='Segger chip ID')
    parser.add_argument('--com', type=str, required=True,
                        help='COM port name. For example COM0 or /dev/ttyACM0')
    parser.add_argument('--erase', '-e', action='store_true',
                        help='Erase target device before flashing the firmware')
    parser.add_argument('--bond-disable', action='store_false', dest='bond',
                        help='Disable bonding simulation')
    parser.add_argument('--reconnections', type=int, help='Number of reconnection attempts',
                        default=0)
    parser.add_argument('--timeout', '-t', type=int, default=5,
                        help='Connection establish timeout in seconds')

    args = parser.parse_args()

    return args


if __name__ == '__main__':
    logging.basicConfig(format='%(message)s', level=logging.INFO)

    print("MDS BLE gateway application started")

    args = parse_args()

    if args.snr:
        device_flash(args.com, args.snr, args.erase)

    memfault = Memfault(args.com, bond=args.bond, conn_timeout=args.timeout)

    reconnection_count = args.reconnections

    try:
        while True:
            try:
                memfault.connect()
            except NordicSemiException as err:
                if err.error_code == BLEMemfault.ERROR_NOCONN:
                    print('Connection establish timeout.')
                    if reconnection_count > 0:
                        print('Trying to reconnect..')
                        reconnection_count -= 1
                        continue
                    else:
                        break
                else:
                    raise err

            memfault.wait_for_disconnection()
            # Disconnection, reset the reconnection count
            reconnection_count = args.reconnections

            time.sleep(5)
    except KeyboardInterrupt:
        pass

    finally:
        memfault.close()
