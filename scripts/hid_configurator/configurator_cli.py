#
# Copyright (c) 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic

import argparse
import logging

from devices import DEVICE, get_device_pid, get_device_vid
from NrfHidDevice import NrfHidDevice

from modules.config import change_config, fetch_config
from modules.dfu import fwinfo, fwreboot, dfu_transfer, get_dfu_image_version
from modules.led_stream import send_continuous_led_stream
from modules.music_led_stream import send_music_led_stream


def progress_bar(permil):
    length = 40
    done_len = length * permil // 1000
    progress_line = '[' + '*' * done_len + '-' * (length - done_len) + ']'
    percent = permil / 10.0
    print('\r{} {}%'.format(progress_line, percent), end='')


def perform_dfu(dev, args):
    dfu_image = args.dfu_image

    img_ver_file = get_dfu_image_version(dfu_image)
    if img_ver_file is None:
        print('Cannot read image version from file')
        return

    info = fwinfo(dev)
    if info is None:
        print('Cannot get FW info from device')
        return
    img_ver_dev = info.get_fw_version()

    print('Current FW version from device: ' +
          '.'.join([str(i) for i in img_ver_dev]))
    print('Current FW version from file: ' +
          '.'.join([str(i) for i in img_ver_file]))
    print('Perform update? [y/n]')

    if not args.autoconfirm:
        rsp = input().lower()
        if rsp == 'y':
            pass

        elif rsp == 'n':
            print('DFU rejected by user')
            return

        else:
            print('Improper user input. Operation terminated.')
            return

    success = dfu_transfer(dev, dfu_image, progress_bar)

    if success:
        success = fwreboot(dev)

    if success:
        print('DFU transfer completed')
    else:
        print('DFU transfer failed')


def perform_config(dev, args):
    module_name = args.module
    option_name = args.option

    module_config = DEVICE[args.device_type]['config'][module_name]
    option_config = module_config['options'][option_name]

    value_type = option_config.type

    if value_type is not None and args.value is None:
        success, val = fetch_config(dev, module_name, option_name, option_config)

        if success:
            print('Fetched {}/{}: {}'.format(module_name, option_name, val))
        else:
            print('Failed to fetch {}/{}'.format(module_name, option_name))
    else:
        if value_type is None:
            value = None
        else:
            try:
                value = value_type(args.value)
            except ValueError:
                print('Invalid type for {}/{}. Expected {}'.format(module_name,
                                                                   option_name,
                                                                   value_type))
                return

        success = change_config(dev, module_name, option_name, value, option_config)

        if success:
            if value_type is None:
                print('{}/{} set'.format(module_name, option_name))
            else:
                print('{}/{} set to {}'.format(module_name, option_name, value))
        else:
            print('Failed to set {}/{}'.format(module_name, option_name))


def perform_fwinfo(dev, args):
    info = fwinfo(dev)

    if info:
        print(info)
    else:
        print('FW info request failed')


def perform_fwreboot(dev, args):
    success, rebooted = fwreboot(dev)

    if success and rebooted:
        print('Firmware rebooted')
    else:
        print('FW reboot request failed')


def perform_led_stream(dev, args):
    if args.file is not None:
        send_music_led_stream(dev, DEVICE[args.device_type], args.led_id,
                              args.freq, args.file)
    else:
        send_continuous_led_stream(dev, DEVICE[args.device_type], args.led_id,
                                   args.freq)


def parse_arguments():
    parser = argparse.ArgumentParser()
    sp_devices = parser.add_subparsers(dest='device_type')
    sp_devices.required = True

    for device_name in DEVICE:
        device_parser = sp_devices.add_parser(device_name)

        sp_commands = device_parser.add_subparsers(dest='command')
        sp_commands.required = True

        parser_dfu = sp_commands.add_parser('dfu', help='Run DFU')
        parser_dfu.add_argument('dfu_image', type=str, help='Path to a DFU image')
        parser_dfu.add_argument('--autoconfirm',
                                help='Automatically confirm user input',
                                action='store_true')

        sp_commands.add_parser('fwinfo', help='Obtain information about FW image')
        sp_commands.add_parser('fwreboot', help='Request FW reboot')

        parser_stream = sp_commands.add_parser('led_stream',
                                    help='Send continuous LED effects stream')
        parser_stream.add_argument('led_id', type=int, help='Stream LED ID')
        parser_stream.add_argument('freq', type=int, help='Color change frequency (in Hz)')
        parser_stream.add_argument('--file', type=str, help='Selected audio file (*.wav)')

        device_config = DEVICE[device_name]['config']

        if device_config is not None:
            assert isinstance(device_config, dict)
            parser_config = sp_commands.add_parser('config', help='Configuration option get/set')

            sp_config = parser_config.add_subparsers(dest='module')
            sp_config.required = True

            for module_name in device_config:
                module_config = device_config[module_name]
                assert isinstance(module_config, dict)
                module_opts = module_config['options']
                assert isinstance(module_opts, dict)

                parser_config_module = sp_config.add_parser(module_name, help='{} module options'.format(module_name))
                sp_config_module = parser_config_module.add_subparsers(dest='option')
                sp_config_module.required = True

                for opt_name in module_opts:
                    parser_config_module_opt = sp_config_module.add_parser(opt_name, help=module_opts[opt_name].help)
                    if module_opts[opt_name].type == int:
                        parser_config_module_opt.add_argument(
                            'value', type=int, default=None, nargs='?',
                            help='int from range {}'.format(module_opts[opt_name].range))
                    elif module_opts[opt_name].type == str:
                        parser_config_module_opt.add_argument(
                            'value', type=str, default=None, nargs='?',
                            help='str from range {}'.format(module_opts[opt_name].range))

    return parser.parse_args()


def configurator():
    logging.basicConfig(level=logging.ERROR)
    logging.info('Configuration channel for nRF52 Desktop')

    args = parse_arguments()

    dev = NrfHidDevice(args.device_type,
                       get_device_vid(args.device_type),
                       get_device_pid(args.device_type),
                       get_device_pid('dongle'))

    if not dev.initialized():
        print('Cannot find selected device')
        return

    configurator.ALLOWED_COMMANDS[args.command](dev, args)

    dev.close_device()

configurator.ALLOWED_COMMANDS = {
    'dfu' : perform_dfu,
    'fwinfo' : perform_fwinfo,
    'fwreboot' : perform_fwreboot,
    'config' : perform_config,
    'led_stream' : perform_led_stream
}


if __name__ == '__main__':
    try:
        configurator()
    except KeyboardInterrupt:
        pass
