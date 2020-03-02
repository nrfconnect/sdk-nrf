#
# Copyright (c) 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic

import argparse
import logging

from configurator_core import DEVICE
from configurator_core import get_device_pid, open_device
from configurator_core import change_config, fetch_config
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
    recipient = get_device_pid(args.device_type)

    img_ver_file = get_dfu_image_version(dfu_image)
    if img_ver_file is None:
        print('Cannot read image version from file')
        return

    info = fwinfo(dev, recipient)
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

    success = dfu_transfer(dev, recipient, dfu_image, progress_bar)

    if success:
        success = fwreboot(dev, recipient)

    if success:
        print('DFU transfer completed')
    else:
        print('DFU transfer failed')


def perform_config(dev, args):
    module_config = DEVICE[args.device_type]['config'][args.module]
    module_id = module_config['id']
    device_options = module_config['options']

    config_name = args.option
    value_type = device_options[config_name].type
    recipient = get_device_pid(args.device_type)

    if value_type is not None and args.value is None:
        success, val = fetch_config(dev, recipient, config_name, device_options, module_id)
        if success:
            print('Fetched {}: {}'.format(config_name, val))
        else:
            print('Failed to fetch {}'.format(config_name))
    else:
        if value_type is None:
            config_value = None
        else:
            try:
                config_value = value_type(args.value)
            except ValueError:
                print('Invalid type for {}. Expected {}'.format(config_name, value_type))
                return
        success = change_config(dev, recipient, config_name, config_value, device_options, module_id)

        if success:
            if value_type is None:
                print('{} set')
            else:
                print('{} set to {}'.format(config_name, config_value))
        else:
            print('Failed to set {}'.format(config_name))


def perform_fwinfo(dev, args):
    recipient = get_device_pid(args.device_type)

    info = fwinfo(dev, recipient)

    if info:
        print(info)
    else:
        print('FW info request failed')


def perform_fwreboot(dev, args):
    recipient = get_device_pid(args.device_type)

    success = fwreboot(dev, recipient)

    if success:
        print('Firmware rebooted')
    else:
        print('FW reboot request failed')


def perform_led_stream(dev, args):
    recipient = get_device_pid(args.device_type)

    if args.file is not None:
        send_music_led_stream(dev, recipient, DEVICE[args.device_type],
                              args.led_id, args.freq, args.file)
    else:
        send_continuous_led_stream(dev, recipient, DEVICE[args.device_type],
                                   args.led_id, args.freq)


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
            assert type(device_config) == dict
            parser_config = sp_commands.add_parser('config', help='Configuration option get/set')

            sp_config = parser_config.add_subparsers(dest='module')
            sp_config.required = True

            for module_name in device_config:
                module_config = device_config[module_name]
                assert type(module_config) == dict
                module_opts = module_config['options']
                assert type(module_opts) == dict

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

    dev = open_device(args.device_type)
    if not dev:
        return

    configurator.ALLOWED_COMMANDS[args.command](dev, args)

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
