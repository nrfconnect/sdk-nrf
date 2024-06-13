#
# Copyright (c) 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

import argparse
import logging
import os
import pprint

if os.name == "nt":
    try:
        script_dir = os.path.dirname(os.path.abspath(__file__))
        os.add_dll_directory(script_dir)
    except AttributeError:
        print('Cannot add directory containing configurator_cli.py script to DLL search path. \
               Please update to Python 3.8 or newer to allow placing hidapi.dll in the directory \
               containing configurator_cli.py script')

from NrfHidManager import NrfHidManager

from modules.module_config import MODULE_CONFIG
from modules.config import change_config, fetch_config
from modules.dfu import DfuImage
from modules.dfu import fwinfo, devinfo, fwreboot, dfu_transfer
from modules.led_stream import send_continuous_led_stream
try:
    from modules.music_led_stream import send_music_led_stream
except ImportError as e:
    print()
    print('Exception when importing music LED stream functionality:')
    print(e)
    print('Music LED stream functionality cannot be used')
    print()


def progress_bar(permil):
    length = 40
    done_len = length * permil // 1000
    progress_line = '[' + '*' * done_len + '-' * (length - done_len) + ']'
    percent = permil / 10.0
    print('\r{} {}%'.format(progress_line, percent), end='')


def perform_show(dev, args):
    pp = pprint.PrettyPrinter(indent=4)
    pp.pprint(dev.get_device_config())


def perform_dfu(dev, args):
    info = fwinfo(dev)
    if info is None:
        print('Cannot get FW info from device')
        return
    img_ver_dev = info.get_fw_version()

    complete_dfu_module_name = dev.get_complete_module_name('dfu')

    if complete_dfu_module_name is None:
        print('Dfu module not found')
        return
    if complete_dfu_module_name == 'dfu':
        bootloader_variant = None
    else:
        bootloader_variant = complete_dfu_module_name[len('{}/'.format('dfu')):]
    img_file = DfuImage(args.dfu_image, info, dev.get_board_name(), bootloader_variant)

    img_file_bin = img_file.get_dfu_image_bin_path()
    if img_file_bin is None:
        print('No proper update image in file')
        return

    print('DFU will use file {}'.format(os.path.basename(img_file_bin)))

    img_ver_file = img_file.get_dfu_image_version()
    if img_ver_file is None:
        print('Cannot read image version from file')
        return

    if bootloader_variant:
        if bootloader_variant != img_file.get_dfu_image_bootloader_var():
            print('Bootloader types does not match')
            return

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

    success = dfu_transfer(dev, img_file_bin, progress_bar)

    if success:
        success = fwreboot(dev)

    if success:
        print('DFU transfer completed')
    else:
        print('DFU transfer failed')


def perform_config(dev, args):
    module_name = args.module
    option_name = args.option

    module_config = MODULE_CONFIG[module_name]
    option_config = module_config['options'][option_name]

    value_type = option_config.type

    if value_type is not None and args.value is None:
        success, val = fetch_config(dev, module_name, option_name, option_config)

        if success:
            print('Fetched {} {}: {}'.format(module_name, option_name, val))
        else:
            print('Failed to fetch {} {}'.format(module_name, option_name))
    else:
        if value_type is None:
            value = None
        else:
            try:
                value = value_type(args.value)
            except ValueError:
                print('Invalid type for {} {}. Expected {}'.format(module_name,
                                                                   option_name,
                                                                   value_type))
                return

        success = change_config(dev, module_name, option_name, value, option_config)

        if success:
            if value_type is None:
                print('{} {} set'.format(module_name, option_name))
            else:
                print('{} {} set to {}'.format(module_name, option_name, value))
        else:
            print('Failed to set {} {}'.format(module_name, option_name))


def perform_fwinfo(dev, args):
    info = fwinfo(dev)

    if info:
        print(info)
        complete_dfu_module_name = dev.get_complete_module_name('dfu')
        if complete_dfu_module_name is None:
            print('Dfu module not found')
            return
        if complete_dfu_module_name != 'dfu':
            print('  Bootloader variant: ' + complete_dfu_module_name[len('{}/'.format('dfu')):])
    else:
        print('FW info request failed')


def perform_devinfo(dev, args):
    info = devinfo(dev)

    if info is not None:
        print(info)
    else:
        print('Device information request failed')


def perform_fwreboot(dev, args):
    success, rebooted = fwreboot(dev)

    if success and rebooted:
        print('Firmware rebooted')
    else:
        print('FW reboot request failed')


def perform_led_stream(dev, args):
    if args.file is not None:
        try:
            send_music_led_stream(dev, args.led_id, args.freq, args.file)
        except NameError:
            print('Music LED stream functionality is not available')
    else:
        send_continuous_led_stream(dev, args.led_id, args.freq)


def parse_arguments():
    parser = argparse.ArgumentParser(allow_abbrev=False)

    parser.add_argument(dest='device', default=None, nargs='?',
                        help='Device specified by type, board name or HW ID '
                             '(if not specified, script will show list of available '
                             'devices)')

    sp_commands = parser.add_subparsers(dest='command',
                                        help='Command executed on device')

    sp_commands.add_parser('show', help='Show available modules and options')

    parser_dfu = sp_commands.add_parser('dfu', help='Run DFU')
    parser_dfu.add_argument('dfu_image', type=str, help='Path to a DFU image')
    parser_dfu.add_argument('--autoconfirm',
                            help='Automatically confirm user input',
                            action='store_true')

    sp_commands.add_parser('fwinfo', help='Obtain information about FW image')
    sp_commands.add_parser('devinfo', help='Obtain identification information about device')
    sp_commands.add_parser('fwreboot', help='Request FW reboot')

    parser_stream = sp_commands.add_parser('led_stream',
                                    help='Send continuous LED effects stream')
    parser_stream.add_argument('led_id', type=int, help='Stream LED ID')
    parser_stream.add_argument('freq', type=int, help='Color change frequency (in Hz)')
    parser_stream.add_argument('--file', type=str, help='Selected audio file (*.wav)')

    assert isinstance(MODULE_CONFIG, dict)
    parser_config = sp_commands.add_parser('config',
                                           help='Configuration option set/fetch')

    sp_config = parser_config.add_subparsers(dest='module')
    sp_config.required = True

    for module_name in MODULE_CONFIG:
        module_config = MODULE_CONFIG[module_name]
        assert isinstance(module_config, dict)
        module_opts = module_config['options']
        assert isinstance(module_opts, dict)

        parser_config_module = sp_config.add_parser(module_name, help='{} module options'.format(module_name))
        sp_config_module = parser_config_module.add_subparsers(dest='option')
        sp_config_module.required = True

        for opt_name in module_opts:
            parser_config_module_opt = sp_config_module.add_parser(opt_name,
                                                        help=module_opts[opt_name].help)
            if module_opts[opt_name].type == int:
                parser_config_module_opt.add_argument('value', type=int,
                                                      default=None, nargs='?',
                                                      help='int from range {}'.format(
                                                          module_opts[opt_name].range))
            elif module_opts[opt_name].type == str:
                parser_config_module_opt.add_argument('value', type=str,
                                                      default=None, nargs='?',
                                                      help='str from range {}'.format(
                                                          module_opts[opt_name].range))

    return parser.parse_args()


def configurator():
    logging.basicConfig(level=logging.ERROR)
    logging.info('Configuration channel for nRF52 Desktop')

    args = parse_arguments()
    backend = NrfHidManager()

    if args.device is None:
        if args.command is not None:
            print('Please specify device')
        devlist = backend.list_devices()
        if len(devlist) > 0:
            print('Found following nRF Desktop devices:')
            for d in devlist:
                print("  {}".format(d))
        else:
            print('Found no nRF Desktop devices')
        return

    devs = backend.find_devices(args.device)

    if len(devs) == 0:
        print('Specified device was not found')
    elif len(devs) == 1:
        print('Performing {} operation on {}'.format(args.command,
                                                     devs[0].get_board_name()))
        configurator.ALLOWED_COMMANDS[args.command](devs[0], args)
    else:
        print('More than one device found. Please specify hwid.')

configurator.ALLOWED_COMMANDS = {
    'show' : perform_show,
    'dfu' : perform_dfu,
    'fwinfo' : perform_fwinfo,
    'devinfo' : perform_devinfo,
    'fwreboot' : perform_fwreboot,
    'config' : perform_config,
    'led_stream' : perform_led_stream
}


if __name__ == '__main__':
    try:
        configurator()
    except KeyboardInterrupt:
        pass
