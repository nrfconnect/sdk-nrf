#!/usr/bin/env python3
#
# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

"""Example Firmware update script for nRF91 devices using
   AVSystem Coiote service
"""

import sys
import argparse
import logging
import time

from coiote import Operationlist
from coiote import Coiote

class firmwareObject():
    def __init__(self):
        self.instance_id = None
        self.state = None
        self.ref_time = None
        self.result = None
        self.new_data = False
        self.original_version = ""
        self.current_version = ""
        self.ready_for_update = False
        self.failure = False
        self.ready = False
        self.download_url = None

def fota_state_check(obj_data):
    """Fota State check"""
    if obj_data is None:
        return None
    if obj_data.state == "2":
        logging.info(f'Download ready instance: {obj_data.instance_id}')
        obj_data.ready_for_update = True
    elif obj_data.state == "3":
        logging.info(f'Update started instance: {obj_data.instance_id}')
    elif obj_data.state == "1":
        logging.info(f'Downloading instance: {obj_data.instance_id}')
    elif obj_data.state == "0":
        if obj_data.result == "0":
            logging.info(f'Idle state[{obj_data.instance_id}]')
        elif obj_data.result == "1":
            logging.info(f'Firmware Update Successfully instance: {obj_data.instance_id}')
            logging.info(f'From:{obj_data.original_version} to {obj_data.current_version}')
            obj_data.ready = True
        else:
            logging.info(f'Firmware fail by result(%d) instance {obj_data.instance_id}',
                        int(obj_data.result))
            obj_data.ready = True
            obj_data.failure = True

        return obj_data

def fota_ready(obj_list):
    for obj in obj_list:
        if obj.ready is False:
            return False
    return True

def fota_ready_for_update(obj_list):
    for obj in obj_list:
        if obj.ready_for_update is False and obj.failure is False:
            return False

    return True

def fota_ready_for_test_state(obj_list, test_state):
    if test_state is None:
        return False
    for obj in obj_list:
        if obj.state != test_state and obj.failure is False:
            return False
    return True

class Fota():
    """Interact with Coiote server"""
    def __init__(self):
        #
        self.coiote = Coiote()
        self.fota_object = 5
        self.app_instance_id = 0
        self.modem_instance_id = 0
        self.observation_url = "Firmware Update"
        self.identity = "urn:imei:"
        self.firmware_objects = []
        self.firmware_update_cmd = False
        self.task_timeout = 800

    def set_dev_id(self, dev_id):
        self.identity = dev_id
        logging.info("Client identity: %s", self.identity)
        status = self.coiote.set_device_id(self.identity)
        if status is False:
            return False
        self.fota_object = self.coiote.fota_object()
        self.app_instance_id = 0
        if self.fota_object == 33629:
            self.modem_instance_id = 1
            self.observation_url = "Multicomponent Firmware Update"
        else:
            self.modem_instance_id = 0
            self.observation_url = "Firmware Update"
        return True

    def observation_enable(self):
        logging.info(f'Enable Observation {self.observation_url}')
        return self.coiote.oberservatio_create(self.observation_url, "pmax", "30")

    def observation_disable(self):
        logging.info(f'Delete Observation {self.observation_url}')
        self.coiote.oberservatio_delete(self.observation_url)

    def download_url_generate(self, resource_id, protocol):
        return self.coiote.fota_pull_url(resource_id, protocol)

    def firmware_url_resource_get(self, instance_id):
        app_pack_url = None
        if self.fota_object == 33629:
            app_pack_url = f'Multicomponent Firmware Update.{instance_id}.Package URI'
        else:
            app_pack_url = f'Firmware Update.{instance_id}.Package URI'
        return app_pack_url

    def download_url_write(self, obj_list):
        operations = []

        for obj in obj_list:
            operation = Operationlist()
            operation.resource = self.firmware_url_resource_get(obj.instance_id)
            operation.value = obj.download_url
            operations.append(operation)
            logging.info('Write Fota Download url to %s', operation.resource)

        task_id = self.coiote.write(operations, True)
        return self.task_monitor(task_id)

    def firmware_update_execute(self, obj_list):
        linked_instance = []
        updated_instance = None
        for obj in obj_list:
            obj.ready_for_update = False
            obj.original_version = obj.current_version
            if updated_instance is not None:
                if obj.state == "2":
                    linked_instance.append(obj.instance_id)
            else:
                updated_instance = obj.instance_id

        task_id = self.coiote.fota_update(self.fota_object, updated_instance, linked_instance)
        return self.task_monitor(task_id)

    def firmware_update_cancel(self, obj_list):
        if isinstance(obj_list, list):
            for obj in obj_list:
                task_id = self.coiote.fota_cancel(self.fota_object, obj.instance_id)
                resp = self.task_monitor(task_id)
                if resp is None:
                    logging.error("Cancel Execute fail")
        else:
            task_id = self.coiote.fota_cancel(self.fota_object, obj_list.instance_id)
            resp = self.task_monitor(task_id)
            if resp is None:
                logging.error("Cancel Execute fail")

    def task_monitor(self,task_id):
        resp = self.coiote.get_task_result(task_id, self.task_timeout)
        self.coiote.delete_task(task_id)
        return resp

    def binary_load(self, instance_id, file_name):
        app_binary = self.coiote.fota_app_binary_get(file_name)
        if app_binary is None:
            return None
        with open(app_binary, mode='rb') as f:
            f_payload = f.read()
        logging.info("Binary %s, Size %d (bytes)", app_binary, len(f_payload))
        app_resource_id = self.coiote.fota_resource_allocate(app_binary, instance_id)
        if app_resource_id:
            resp = self.coiote.upload_fota_file(app_resource_id, f_payload)
            if resp is None:
                return None
            if "result" in resp:
                result = resp["result"]
                if result != 'Success':
                    logging.info(f'Resource ID allocate fail {result}')
                    return None
        return app_resource_id

    def object_state_poll(self, obj_data):
        if obj_data is None:
            return None

        obj_data.new_data = False
        state, update_time, result, version_num =  self.coiote.fota_object_read(self.fota_object,
                                                                            obj_data.instance_id)
        if state != obj_data.state and update_time != obj_data.ref_time:
            obj_data.state = state
            obj_data.ref_time = update_time
            obj_data.result = result
            obj_data.current_version = version_num
            obj_data.new_data = True

        return obj_data

    def firmware_update_state_machine(self, obj_list, test_state=None):
        timeout = 1200
        t_start  = time.perf_counter()
        loop = True
        status = False
        while loop:
            time.sleep(1)
            for obj in obj_list:
                obj = self.object_state_poll(obj)
                if obj.new_data:
                    t_start  = time.perf_counter()
                    obj = fota_state_check(obj)

            if fota_ready_for_test_state(obj_list, test_state) is True:
                status = True
                loop = False
            elif fota_ready_for_update(obj_list) is True:
                resp = self.firmware_update_execute(obj_list)
                if resp is None:
                    logging.error("Update Execute fail")
                    loop = False
            elif fota_ready(obj_list) is True:
                status = True
                loop = False
            elif time.perf_counter() - t_start > timeout:
                logging.error("Timeout --> cancel update")
                loop = False

        return status

    def app_binary_setup(self):
        logging.info("Load Client binary to Coiote")
        app_obj = firmwareObject()
        app_obj.instance_id = self.app_instance_id
        resource_id = fota.binary_load(app_obj.instance_id, "app_update.bin")
        if resource_id is None:
            return None
        logging.info(f'Client Binary Resource id {resource_id}')
        app_obj.download_url = self.download_url_generate(resource_id, "HTTP")
        if app_obj.download_url is None:
            return None
        self.firmware_objects.append(app_obj)
        return app_obj

    def modem_binary_setup(self, bin_name):
        logging.info("Init setup for Modem firmware Update :%s", bin_name)
        modem_obj = firmwareObject()
        modem_obj.instance_id = self.modem_instance_id
        r_id = fota.binary_load(modem_obj.instance_id, bin_name)
        if r_id is None:
            return None
        modem_obj.download_url = self.download_url_generate(r_id, "HTTP")
        if modem_obj.download_url is None:
            return None
        self.firmware_objects.append(modem_obj)
        return modem_obj

    def generic_binary_setup(self, r_id, instance_id):
        logging.info("Init setup for instance %d firmware Update resource:%s", instance_id, r_id)
        gen_obj = firmwareObject()
        gen_obj.instance_id = instance_id
        gen_obj.download_url = self.download_url_generate(r_id, "HTTP")
        if gen_obj.download_url is None:
            return None
        self.firmware_objects.append(gen_obj)
        return gen_obj
    def firmware_update_group_cancel(self):
        for obj in self.firmware_objects:
            obj = self.object_state_poll(obj)
            if obj.state != "0" and obj.state is not None:
                self.firmware_update_cancel(obj)

    def firmware_update(self):
        self.observation_disable()
        self.firmware_update_group_cancel()
        resp = self.download_url_write(self.firmware_objects)

        if resp is None:
            logging.error("Path Url Write fail")
            self.firmware_update_cancel(self.firmware_objects)
            sys.exit(0)

        logging.info("Download Url Write done")
        resp = self.observation_enable()
        if resp is None:
            logging.error("Obervation enable for firmware state fail")
            self.firmware_update_cancel(self.firmware_objects)
            self.observation_disable()
            sys.exit(0)

        update_process_ok = self.firmware_update_state_machine(self.firmware_objects)
        if update_process_ok:
            logging.info("Firmware update process finished")
        else:
            logging.info("Firmware update process fail")
            self.firmware_update_group_cancel()

        self.observation_disable()

if __name__ == "__main__":
    try:
        from dotenv import load_dotenv
        load_dotenv()
    except ImportError:
        pass

    fota = Fota()

    def upload(opt, args):
        instance_id = int(args.instance_id)
        file_name = args.bin_name
        logging.info(f'Upload {file_name} with instance {instance_id} to Coiote')
        r_id = fota.binary_load(instance_id, file_name)
        if r_id is None:
            return None
        logging.info(f'Allocated Resource id {r_id} for instance {instance_id}')
    def update(opt, args):
        if opt.id is None:
            logging.error("Update need device ID option")
            sys.exit(0)

        instance_id = int(args.instance_id)
        r_id = None
        if args.value_type == 'file':
            r_id = fota.binary_load(instance_id, args.value)
            if r_id is None:
                sys.exit(1)

        elif args.value_type == 'resource':
            r_id = args.value
        else:
            logging.error(f'Un supported type {args.value_type} for command update')
            sys.exit(1)

        resp = fota.generic_binary_setup(r_id, instance_id)
        if resp is None:
            sys.exit(1)
        fota.firmware_update_cmd = True

    def init_app_binary(args):
        app_obj = fota.app_binary_setup()
        if app_obj is None:
            logging.error("Application binary setup fail")
            sys.exit(0)

    def init_modem_binary(args):
        modem_obj = fota.modem_binary_setup(args.bin_name)
        if modem_obj is None:
            logging.error("Application binary setup fail")
            sys.exit(0)

    def fota_test_cancel_process(args):
        fota.observation_disable()
        fota.firmware_update_group_cancel()
        resp = fota.download_url_write(fota.firmware_objects)

        if resp is None:
            logging.error("Path Url Write fail")
            fota.firmware_update_cancel(fota.firmware_objects)
            return False

        logging.info("Download Url Write done")
        resp = fota.observation_enable()
        if resp is None:
            logging.error("Observation enable for firmware state fail")
            fota.firmware_update_cancel(fota.firmware_objects)
            fota.observation_disable()
            return False

        state_process_ok = fota.firmware_update_state_machine(fota.firmware_objects, "2")
        if state_process_ok is False:
            fota.firmware_update_group_cancel()
            fota.observation_disable()
            logging.info("Cancel test FAIL")
            return False
        logging.info("Cancel downloaded Image")
        fota.firmware_update_group_cancel()

        fota.firmware_update_state_machine(fota.firmware_objects, "0")
        fota.observation_disable()

        logging.info("Verify That State is idle and Result is 10")
        for obj in fota.firmware_objects:
            if obj.state != "0" and obj.result != "10":
                logging.error(f'Not correct state{obj.state} or result{obj.result}')
                logging.info("Cancel test FAIL")
                return False
        logging.info("Cancel test PASS")
        return True

    def fota_test_separate_update_process(args):
        fota.observation_disable()
        fota.firmware_update_group_cancel()
        resp = fota.download_url_write(fota.firmware_objects)

        if resp is None:
            logging.error("Path Url Write fail")
            fota.firmware_update_cancel(fota.firmware_objects)
            return False

        logging.info("Download Url Write done")
        resp = fota.observation_enable()
        if resp is None:
            logging.error("Obervation enable for firmware state fail")
            fota.firmware_update_cancel(fota.firmware_objects)
            fota.observation_disable()
            return False

        state_process_ok = fota.firmware_update_state_machine(fota.firmware_objects, "2")
        if state_process_ok is False:
            fota.firmware_update_group_cancel()
            fota.observation_disable()
            return False

        logging.info("Update Image seprately")
        test_objects = []
        for obj in fota.firmware_objects:
            test_objects = []
            test_objects.append(obj)
            logging.info(f'Update instance {obj.instance_id}')
            state_process_ok = fota.firmware_update_state_machine(test_objects)
            if state_process_ok is False:
                fota.firmware_update_group_cancel()
                fota.observation_disable()
                return False

        fota.firmware_update_state_machine(fota.firmware_objects, "0")
        fota.observation_disable()

        for obj in fota.firmware_objects:
            if obj.state != "0" and obj.result != "1":
                logging.error(f'Not correct state{obj.state} or result{obj.result}')
                logging.info("Separate Update test FAIL")
                return False
        logging.info("Separate Update test PASS")
        return True

    def test_cancel(opt, args):
        status = fota.set_dev_id(arg.device_id)
        if status is False:
            sys.exit(0)
        if fota.fota_object != 33629:
            logging.error("Current application not support Advamced Fota")
            sys.exit(0)
        init_modem_binary(args)
        return fota_test_cancel_process(args)
    def test_separate_update(opt, args):
        status = fota.set_dev_id(arg.device_id)
        if status is False:
            sys.exit(0)
        if fota.fota_object != 33629:
            logging.error("Current application not support Advamced Fota")
            sys.exit(0)
        init_app_binary(args)
        init_modem_binary(args)
        return fota_test_separate_update_process(args)

    def parse_command_and_options(parser, command_list):
        # Divide argv by commands
        options_array = []
        commands_start = False
        commands_array = [[]]
        #Parse Options and command to own array
        for c in sys.argv[1:]:
            if c in command_list.choices:
                commands_array.append([c])
                commands_start = True
            else:
                if commands_start is False:
                    options_array.append(c)
                else:
                    commands_array[-1].append(c)
        return options_array, commands_array

    parser = argparse.ArgumentParser(description='nRF91 firmware update example',
                                    allow_abbrev=False)
    parser.set_defaults(func=None)
    parser.add_argument("-d", "--debug", action="store_true", default=False,
                        help="Output debug information.")
    parser.add_argument('-id', help='Device ID, mandatory for update command')
    parser.add_argument('-to', help='Timeout for waiting task is ready')

    subparsers = parser.add_subparsers(title='command')

    upload_pars = subparsers.add_parser('upload',
                                        help='Upload binary to Coiote for firmware Update')
    upload_pars.set_defaults(func=upload)
    upload_pars.add_argument('instance_id', help='Instance ID')
    upload_pars.add_argument('bin_name', help='Modem Binary name')

    update_pars = subparsers.add_parser('update', help='Download and Update firmware')
    update_pars.set_defaults(func=update)
    update_pars.add_argument('instance_id', help='Instance ID')
    update_pars.add_argument('value_type', help='file or resource')
    update_pars.add_argument('value', help='Binary name or resource id')

    test_cancel_pars = subparsers.add_parser('test_cancel', help='Test Cancel operation')
    test_cancel_pars.set_defaults(func=test_cancel)
    test_cancel_pars.add_argument('device_id', help='Device ID')
    test_cancel_pars.add_argument('bin_name', help='Modem Binary name')

    test_separate_update_pars = subparsers.add_parser('test_series_update',
                                                    help='Test multi instance update in serial')
    test_separate_update_pars.set_defaults(func=test_separate_update)
    test_separate_update_pars.add_argument('device_id', help='Device ID')
    test_separate_update_pars.add_argument('bin_name', help=' Modem Binary name')

    options, command_array = parse_command_and_options(parser, subparsers)
    opt = parser.parse_args(options)

    if opt.debug:
        logging.basicConfig(
            level=logging.DEBUG, format='[%(levelname)s] %(filename)s - %(message)s')
    else:
        logging.basicConfig(
            level=logging.INFO, format='[%(levelname)s] %(filename)s - %(message)s')

    if opt.id is not None:
        status = fota.set_dev_id(opt.id)
        if status is False:
            sys.exit(0)

    if opt.to is not None:
        fota.task_timeout = int(opt.to)

    if len(command_array) < 2:
        print(f'length of commands {len(command_array)}')
        print(f'lCommand array {command_array}')
        parser.print_help()
        sys.exit(0)

    for cmd in command_array[1:]:  # Commands skip blanko
        arg = parser.parse_args(cmd)
        if arg.func is None:
            parser.print_help()
            sys.exit(0)
        arg.func(opt, arg)

    if fota.firmware_update_cmd is True:
        logging.info("Start Firmware Update")
        fota.firmware_update()
