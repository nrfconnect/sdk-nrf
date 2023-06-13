#!/usr/bin/env python3
#
# Copyright (c) 2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

"""Module for interacting with AVSystem Coiote device management REST API"""

import os
import sys
import argparse
import logging
import json
import time
import requests
import fnmatch

class Operationlist():
    def __init__(self):
        self.resource = None
        self.value = ""

def create_attributes_payload(attribute=None, value=""):
    """Cretae payload attributes"""
    attributes = []

    if attribute is not None:
        attributes = [{
                    "name": attribute,
                    "value": value
                    }
                ]

    tmp_attribute = {
            "attributes":
            attributes,
            "createEnsureObserveIfNotExists": False
    }
    return tmp_attribute

class Coiote():
    """Interact with Coiote server"""
    def __init__(self):
        #
        self.base_url = os.environ.get('COIOTE_URL')

        if self.base_url is None:
            self.base_url = 'https://eu.iot.avsystem.cloud'

        self.api_url = f"{self.base_url}/api/coiotedm/v3"

        self.user   = os.environ.get("COIOTE_USER")
        self.passwd = os.environ.get("COIOTE_PASSWD")

        if not self.user or not self.passwd:
            logging.error("COIOTE_USER or COIOTE_PASSWD not set")
            sys.exit(1)

        self.headers = {'accept': 'application/json',
                        'content-type': 'application/json'}
        self.binary_headers = {"accept": "application/json",
                                "Content-Type": "application/octet-stream"}

        self.domain = os.environ.get("COIOTE_DOMAIN")

        token = os.environ.get("COIOTE_TOKEN")

        if token is not None:
            self.headers = {'accept': 'application/json',
                            'content-type': 'application/json',
                            'authorization': f'Bearer {token}'}

        if self.user and self.passwd:
            self.auth = (self.user, self.passwd)
        else:
            self.auth = None

        self.device_id = f"urn:imei"
        self.queueMode = None
        self.callerPath = os.getcwd()

    def set_device_id(self, dev_id):
        """Set device ID"""
        self.device_id = dev_id
        logging.debug('Set device ID: %s', self.device_id)
        device = self.get_device(self.device_id)
        if "properties" in device:
            if device["properties"]["registeredQueueMode"] == "true":
                self.queueMode = True
                logging.debug("Queuemode is Enabled")
            else:
                self.queueMode = False
            return True
        else:
            logging.error(f" Device not Include properties:\n{device}")
            return False



    def authenticate(self):
        """Request authentication token"""
        auth_url = f"{self.base_url}/api/auth/oauth_password"
        headers = {'content-type': 'application/x-www-form-urlencoded'}
        payload = f"grant_type=password&username={self.user}&password={self.passwd}"
        resp = requests.post(auth_url, headers=headers, data=payload)
        if resp.status_code != 201:
            logging.debug(resp.text)
            logging.error("Error in authentication")
            sys.exit(1)

        response = dict(json.loads(resp.text))
        token = response['access_token']
        token_expiry = response['expires_in']
        logging.info("Authenticated, token expires in %s s", str(token_expiry))

        self.headers = {'accept': 'application/json',
                        'content-type': 'application/json',
                        'authorization': f'Bearer {token}'}
        logging.info("Token: \n%s", token)

    def get(self, path, handle=True):
        """Send HTTP GET query"""
        resp = requests.get(f"{self.api_url}{path}",
                            headers=self.headers, auth=self.auth)

        if resp is not None:
            logging.debug("GET Status Code %d", resp.status_code)
        if handle:
            return Coiote.handle_response(resp)
        else:
            return resp

    def put(self, path, data, headers = None):
        """Send HTTP PUT query"""
        if headers is None:
            headers = self.headers
        resp = requests.put(f"{self.api_url}{path}",
                            headers=headers, data=data, auth=self.auth)
        return Coiote.handle_response(resp)


    def post(self, path, data, handle=True):
        """Send HTTP POST query"""
        logging.debug("Post Path %s", path)

        resp = requests.post(f"{self.api_url}{path}",
                             headers=self.headers, data=data, auth=self.auth)
        if resp is None:
            return None

        if handle:
            return Coiote.handle_response(resp)
        else:
            return resp

    def delete(self, path):
        """Send HTTP DELETE query"""
        resp = requests.delete(f"{self.api_url}{path}", headers={
                               'accept': 'application/json'}, auth=self.auth)
        return Coiote.handle_response(resp)

    def fota_object(self):
        resp = self.get(f'/cachedDataModels/{self.device_id}?parameters={"Advanced Firmware Update."}')
        response_length = len(resp)
        if response_length:
            return 33629

        resp = self.get(f'/cachedDataModels/{self.device_id}?parameters={"Firmware Update."}')
        response_length = len(resp)
        if response_length:
            return 5
        return 0

    def fota_resource_allocate(self, name, instance_id):
        """Allocate Resource id for Binary by given binary name and instance ID"""
        temp_id = f'lwm2m_client_fota_instance_{instance_id}'
        logging.info(f"Creating fota resource for binary {name} with id {temp_id}")

        url = self.fota_pull_url(temp_id, "HTTP")
        if url:
            logging.debug("Resource allocated already")
            return temp_id

        logging.info("Allocated new entry")
        payload_s = {
            "id": temp_id,
            "name": name,
            "location": {"InternalLocation": {"fileName": f"{name}"}},
            "category": "FIRMWARE",
            "domain": "/IoT/NordicSemi/Interop/",
            "expirationTime": "ONE_DAY",
            "visibleForSubtenants": False
            }
        logging.debug(payload_s)
        path  = "/resources"

        resp = self.post(path, json.dumps(payload_s))

        if resp and "resourceId" in resp:
            return resp["resourceId"]
        else:
            return None

    def fota_app_binary_get(self, file_name):
        """Discover Binary from root folder or build/zephyr"""
        os.chdir(self.callerPath)
        os.getcwd()
        file_list = os.listdir()
        for filename in file_list:
            if fnmatch.fnmatch(filename, file_name):
                return filename

        os.chdir('build/zephyr')
        os.getcwd()
        file_list = os.listdir()
        for filename in file_list:
            if fnmatch.fnmatch(filename, file_name):
                return filename
        return None

    def upload_fota_file(self, fota_id, file):
        """Upload Binary to allocated resource"""
        logging.debug(f"Uplaoding delta image {file}")
        path = f"/resources/{fota_id}/data"
        return self.put(path, file, self.binary_headers)

    def fota_pull_url(self, id, protocol):
        """Generate Download url by given Resource id and Protocol"""
        objs = {
            "protocol": protocol,
        }

        resp = self.post(f'/resources/{id}/downloadUrl', json.dumps(objs))
        if resp:
            if "address" in resp:
                return resp["address"]
        return None

    def fota_update(self, object, instance, linked_instance=None):
        """Generate Update execution for fiven instace list"""
        operation = Operationlist()

        if object == 33629:
            resource = f'Advanced Firmware Update.{instance}.Update'
            if linked_instance is not None:
                if isinstance(linked_instance, list):
                    linked_list = ""
                    for id in linked_instance:
                        linked_list += f'</{object}/{id}>'
                    operation.value = linked_list
                else:
                    operation.value = f'</{object}/{linked_instance}>'
        elif object == 5:
            resource = f'Firmware Update.{instance}.Update'

        operation.resource = resource
        return self.execute(operation, True)

    def fota_object_read(self, object, instance):
        """Read Firmware object from data cache"""
        resource = None
        state_path = None
        result_path = None
        version_num_path = "Dummy"

        state = None
        state_update = None
        result = None
        version_num = None

        if object == 33629:
            resource = f'Advanced Firmware Update.{instance}.'
            state_path = f'Advanced Firmware Update.{instance}.State'
            result_path = f'Advanced Firmware Update.{instance}.Update Result'
            version_num_path = f'Advanced Firmware Update.{instance}.Current Version'
        else:
            resource = f'Firmware Update.{instance}.'
            state_path = f'Firmware Update.{instance}.State'
            result_path = f'Firmware Update.{instance}.Update Result'

        resp = self.get(f'/cachedDataModels/{self.device_id}?parameters={resource}', False)
        obj = json.loads(resp.text)
        length = len(obj)
        for i in range (0,length):
            name = obj[i].get('name')
            value = obj[i].get('value')
            update_time = obj[i].get('updateTime')
            if name == state_path and value is not None:
                state = value
                state_update = update_time
            elif name == result_path and value is not None:
                result = value
            elif name == version_num_path and value is not None:
                version_num = value
            value = None
        return state, state_update, result, version_num

    def fota_cancel(self, object, instance):
        """Generate Cancel execute command"""
        operation = Operationlist()

        if object == 33629:
            operation.resource = f'Advanced Firmware Update.{instance}.Cancel'
            return self.execute(operation, True)
        elif object == 5:
            operation.resource = "Firmware Update.0.Package URI"
            operation.value = " "
            return self.write(operation)

        return None

    def get_task_result(
            self,
            task_id,
            timeout=800
            ):
        """Poll generated Task Id when it executed"""

        if "error" in task_id:
            raise SystemExit("Incorrect response from server")

        status_code = 404
        status_task = ""
        status_fail = [ "Warning", "Error" ]

        resp        = None

        task_id     = task_id.replace("\"", "")
        path        = f"/taskReports/{task_id}/{self.device_id}"
        t_start     = time.perf_counter()

        if self.queueMode is True:
            logging.info("Device is Queuemode Coiote have to wait next Registration Update")
            logging.debug(f'Timeout for wait is {timeout}')

        while status_code == 404:
            resp = self.get(path)
            if resp is not None:
                status_code = 200

            if time.perf_counter() - t_start > timeout:
                logging.warning("Task report status not success")
                return None
            time.sleep(1)

        # Check the task status before loop to avoid unnecessary API calls
        # if the task has already completed successfully
        if resp is not None:
            if "status" in resp:
                status_task = resp["status"]

        # Check the task status until it indicates success or failure
        while status_task != "Success":
            resp = self.get(path)
            if resp is not None:
                if "status" in resp:
                    status_task = resp["status"]

            # Check if the status indicates task failure
            if status_task in status_fail:
                logging.info(
                    f"Resp failed with status {status_task}"
                    f"and content:\n{resp} "
                )
                return None
            elif time.perf_counter() - t_start > timeout:
                logging.warning("Task report status not success")
                return None

            time.sleep(1)

        logging.debug(f"response: {resp}")

        # Check if the API response is valid before continuing
        if not resp["properties"]:
            raise IndexError(
                "API response does not contain \'properties\' list."
                f" Response:\n{resp}"
            )

        return resp
    def create_payload(self, action, operation):
        """Create Payload for task"""
        operations = []

        if isinstance(operation, list):
            for res in operation:
                if action == "execute" and res.value != "":
                    operation = [{ action: {
                            "key": res.resource,
                            "argumentList": [
                                {
                                    "digit": "0=;",
                                    "argument": res.value
                                }
                            ]
                            }
                        }]
                else:
                    operation = { action: {
                            "key": res.resource,
                            "value": res.value
                            }
                        }
                operations.append(operation)
        else:
            if action == "execute" and operation.value != "":
                operations = [{ action: {
                                "key": operation.resource,
                                "argumentList": [
                                    {
                                        "digit": "0=;",
                                        "argument": operation.value
                                    }
                                ]
                                }
                            }]
            else:
                operations = [{ action: {
                            "key": operation.resource,
                            "value": operation.value
                            }
                        }]

        execute = None
        if self.queueMode is True:
            execute = False
        else:
            execute = True
        tmp_payload = {
            "taskDefinition": {
            "operations": operations,
            "name" : f"{action} object resource",
            "executeImmediately": execute
          }
        }
        return tmp_payload

    def delete_task(self, task_id):
        """Delete finished task"""
        if self.delete('/tasks/' + task_id) is None:
            logging.error('Coiote: Failed to delete task %s', task_id)
        else:
            logging.debug('Coiote: Deleted task %s', task_id)

    def write(self,
            operations,
            validate_task_outside=False
            ):
        """Generate Write operation task"""
        payload = self.create_payload("write", operations)
        resp = self.post(f"/tasks/configure/{self.device_id}", json.dumps(payload))

        if resp is None:
            logging.error("Write fail")
            return None


        task_id = resp
        logging.debug("Write Task id %s", task_id)
        if validate_task_outside is True:
            return task_id


        resp = self.get_task_result(task_id)
        self.delete_task(task_id)

        return resp

    def read(self,
            resource,
            validate_task_outside=False
            ):
        """Generate Read operation task"""
        operation = Operationlist()
        operation.resource = resource
        payload = self.create_payload("read", operation)
        resp = self.post(f"/tasks/configure/{self.device_id}", json.dumps(payload))

        if resp is None:
            logging.error("Read fail")
            return None

        task_id = resp
        if validate_task_outside is True:
            return task_id

        resp = self.get_task_result(task_id)
        self.delete_task(task_id)

        return resp

    def execute(self, operation, validate_task_outside=False):
        """Generate Execute operation task"""
        payload = self.create_payload("execute", operation)
        resp = self.post(f"/tasks/configure/{self.device_id}", json.dumps(payload))

        task_id = resp
        if validate_task_outside is True:
            return task_id

        resp = self.get_task_result(task_id)
        self.delete_task(task_id)

        return resp

    def oberservatio_create(self, resource, attribute=None, value=""):
        payload = create_attributes_payload(attribute, value)
        path = f"/observations/device/{self.device_id}/{resource}"

        resp = self.post(path, json.dumps(payload), False)

        return resp
    def oberservatio_delete(self, resource, attribute=None, value=""):
        resp = self.delete(f"/observations/device/{self.device_id}/{resource}")

        return resp

    @staticmethod
    def handle_response(resp):
        """Generic response handler for all queries"""
        if resp is None:
            return None

        obj = None

        if hasattr(resp,"text"):
            if (resp.text is None or len(resp.text) == 0):
                logging.warning('Response code {%d}', resp.status_code)
                return None

            obj = json.loads(resp.text)

            if resp.status_code >= 300 or resp.status_code < 200:
                if "error" in obj:
                    logging.warning('Coiote: %s', obj["error"])
                else:
                    logging.debug(json.dumps(obj, indent=4))
                    return None

            logging.debug(json.dumps(obj, indent=4))
        return obj

    @staticmethod
    def handle_batch_response(resp):
        if resp is None:
            logging.warning("Empty response received.")
            return None,0,0,0

        status_code = resp.status_code
        succeeded = 0
        failed = 0

        if hasattr(resp,"text") is False:
            logging.warning("No response payload received.")
            return None,status_code,succeeded,failed

        obj = json.loads(resp.text)

        if status_code < 200 or status_code >= 300:
            logging.warning("Coiote returned HTTP error %d",status_code)
            logging.warning('Coiote: %s', obj['error'])

        if obj:
            if "succeeded" in obj:
                succeeded = obj["succeeded"]
            if "failed" in obj:
                failed = obj["failed"]

        return obj,status_code,succeeded,failed


    def get_device(self, dev_id):
        """Get device information"""
        return self.get('/devices/' + dev_id)

    def delete_device(self, dev_id):
        """Delete a device with given id"""
        if self.delete('/devices/' + dev_id) is None:
            logging.error('Coiote: Failed to delete device %s', dev_id)
        else:
            logging.info('Coiote: Deleted device %s', dev_id)

    def batch_create_device(self, dev_file, model, lines, prefix):
        # dev_list is in form:  \"352656100030868\",\"XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\"

        logging.info(f"URL = {self.api_url}")

        line_cnt = 0
        line_tot = 0
        succeeded = 0
        failed = 0

        with open(dev_file,"r") as fh:
            objs = []
            domain = self.get_domain()

            for line in fh:
                tmp = line.split(',')
                dev_id = prefix+tmp[0].strip('"')
                psk = tmp[1].strip('\n"')

                objs.append({
                    "id": dev_id,
                    "connectorType": "bootstrap",
                    "properties": {
                        "endpointName": dev_id,
                    },
                    "domain": domain,
                    "securityMode": "psk",
                    "dtlsIdentity": dev_id,
                    "dtlsPsk": {"HexadecimalPsk": psk}
                })

                if model:
                    objs[-1]["properties"]["genericModelName"] = model

                line_cnt += 1
                line_tot += 1

                if line_cnt == lines:
                    logging.debug("Sending POST:")
                    logging.debug(json.dumps(objs,indent=2))

                    resp = self.post("/devices/batch",json.dumps(objs),False)
                    obj,status_code,tmp_s,tmp_f = Coiote.handle_batch_response(resp)

                    logging.debug("Received response (status %d):",status_code)
                    if obj:
                        logging.debug(json.dumps(obj,indent=2))

                    succeeded += tmp_s
                    failed += tmp_f

                    objs = []
                    line_cnt = 0

            if line_cnt > 0:
                logging.debug("Sending POST:")
                logging.debug(json.dumps(objs,indent=2))

                resp = self.post("/devices/batch",json.dumps(objs),False)
                obj,status_code,tmp_s,tmp_f = Coiote.handle_batch_response(resp)

                logging.debug("Received response (status %d):",status_code)
                if obj:
                    logging.debug(json.dumps(obj,indent=2))

                succeeded += tmp_s
                failed += tmp_f

        logging.info("Coiote: uploaded %d devices into domain '%s', %d succeeded, %d failed.",
                line_tot,self.domain,succeeded,failed)


    def create_device(self, dev_id, psk, model=None, bootstrap=True):
        """Create a device to bootstrap server"""
        domain = self.get_domain()
        obj = {
            "connectorType": "bootstrap",
            "properties": {
                "endpointName": dev_id
            },
            "domain": domain,
            "securityMode": "psk",
            "dtlsIdentity": dev_id,
            "dtlsPsk": {"HexadecimalPsk": psk}
        }

        if bootstrap:
            obj["id"] = f"{dev_id}-bs"
        else:
            obj["id"] = dev_id

        if model:
            obj["properties"]["genericModelName"] = model

        logging.debug(json.dumps(obj,indent=2))


        if self.post('/devices',json.dumps(obj)) is None:
            logging.error('Coiote: Failed to create device %s', dev_id)
        else:
            logging.info(
                'Coiote: Created device %s to domain %s', dev_id, domain)

    def get_domain(self):
        """Get list of domains for current user"""
        if self.domain is None:
            self.domain = self.get('/domains')[0]
            logging.debug('Coiote: Domain is %s', self.domain)
        return self.domain

#
#
if __name__ == "__main__":
    logging.basicConfig(level=logging.INFO, format='%(message)s')

    try:
        from dotenv import load_dotenv
        load_dotenv()
    except ImportError:
        pass

    coiote = Coiote()

    def login(args):
        """Login to Coiote"""
        from getpass import getpass
        coiote.user = args.username
        coiote.passwd = getpass()
        coiote.authenticate()

    def domain(args):
        """Get domain"""
        domain = coiote.get_domain()
        if domain is not None:
            logging.info('Domain: %s', domain)

    def get(args):
        """Get a device"""
        resp = coiote.get_device(args.id)
        print(json.dumps(resp, indent=4))

    def delete(args):
        """Delete a device"""
        coiote.delete_device(args.id)

    def create(args):
        """Create a device"""
        coiote.create_device(args.id, args.psk, args.model,args.bootstrap)

    def batch_create(args):
        """Batch create devices from a file"""
        coiote.batch_create_device(args.file,args.model,args.lines,args.prefix)

    def data(args):
        """Get data from cached datamodel"""
        resp = coiote.get(f'/cachedDataModels/{args.id}?parameters={args.query}')
        print(json.dumps(resp, indent=4))

    def fota_object(args):
        """Read registered Firmware object"""
        coiote.set_device_id(args.id)
        object_num = coiote.fota_object()
        logging.info('Firmware object: %d', object_num)

        return object_num

    def fota_pull_url(args):
        """Generate Download Pull URL"""
        pull_url = coiote.fota_pull_url(args.id, args.protocol)
        logging.info('Firmware image pull url: %s', pull_url)
        return pull_url

    def set_device_id(args):
        """Set Device ID"""
        coiote.set_device_id(args.id)

    def fota_object_read(args):
        """Read Fota object instance data"""
        coiote.set_device_id(args.id)
        logging.info('Firmware State read %d/%d', int(args.object), int(args.instance))
        coiote.fota_object_read(int(args.object), int(args.instance))

    def fota_cancel(args):
        """Fota cancel request"""
        coiote.set_device_id(args.id)
        logging.info('Firmware Cancel %d/%d:', int(args.object), int(args.instance))
        task_id = coiote.fota_cancel(int(args.object), int(args.instance))
        coiote.get_task_result(task_id)
        coiote.delete_task(task_id)

    def fota_update(args):
        """Fota update request"""
        linked_instance = int(args.linked_instance)
        coiote.set_device_id(args.id)
        task_id = coiote.fota_update(int(args.object), int(args.instance), linked_instance)
        coiote.get_task_result(task_id)
        coiote.delete_task(task_id)

    parser = argparse.ArgumentParser(
        description='Coiote device management',
        allow_abbrev=False)
    parser.set_defaults(func=None)
    parser.add_argument("-d", "--debug", action="store_true", default=False,
                        help="Output debug information.")
    subparsers = parser.add_subparsers(title='commands')
    login_pars = subparsers.add_parser(
        'login', help='Log in using username and password')
    login_pars.set_defaults(func=login)
    login_pars.add_argument('username', help='Coiote username')
    login_pars.add_argument(
        '-e', action='store_true', help='Read username and password environment variables')
    login_pars.add_argument(
        '-n', action='store_true', dest='no_store', help='Don\'t store the received token')

    get_pars = subparsers.add_parser('get', help='Get device data')
    get_pars.set_defaults(func=get)
    get_pars.add_argument('id', help='Device ID')
    del_pars = subparsers.add_parser('delete', help='Delete device')
    del_pars.set_defaults(func=delete)
    del_pars.add_argument('id', help='Device ID')
    create_pars = subparsers.add_parser('create', help='Create device')
    create_pars.set_defaults(func=create)
    create_pars.add_argument('id', help='Device ID')
    create_pars.add_argument('psk')
    create_pars.add_argument('-m', '--model', action='store', default=None,
                            help='Generic Model Name')
    create_pars.add_argument('-b', '--bootstrap', default=False, action='store_true',
                            help='Use bootstrap')
    get_domain_pars = subparsers.add_parser('domain', help='Get domain')
    get_domain_pars.set_defaults(func=domain)
    batch_pars = subparsers.add_parser('batch', help='Batch create devices from a file.')
    batch_pars.set_defaults(func=batch_create)
    batch_pars.add_argument('file', type=str, action='store', help='CSV file with IMEI+PSK pairs.')
    batch_pars.add_argument('-m', '--model', action='store', default=None,
                            help='Generic Model Name.')
    batch_pars.add_argument('-p', '--prefix', action='store', default="urn:imei:",
                            help="Prefix string for IDs. Default is 'urn:imei:'")
    batch_pars.add_argument('-l', '--lines', action='store', type=int, default=100,
                            help='Maximum batch size. Default is 100.')

    data_pars = subparsers.add_parser('data', help='Get cached data from DataModel')
    data_pars.set_defaults(func=data)
    data_pars.add_argument('id', help='Device ID')
    data_pars.add_argument('query', help='Data query')

    fota_object_pars = subparsers.add_parser('fota_object', help='Get support FOTA object')
    fota_object_pars.set_defaults(func=fota_object)
    fota_object_pars.add_argument('id', help='File reorce ID')

    pull_url_pars = subparsers.add_parser('fota_pull_url', help='Get generate Download url')
    pull_url_pars.set_defaults(func=fota_pull_url)
    pull_url_pars.add_argument('id', help='File reorce ID')
    pull_url_pars.add_argument('protocol', help='Protocol COAP, HTTP, COAPS, HTTPS')

    set_id_pars = subparsers.add_parser('set_device_id', help='Set device id')
    set_id_pars.set_defaults(func=set_device_id)
    set_id_pars.add_argument('id', help='File reorce ID')

    cancel_pars = subparsers.add_parser('fota_cancel', help='Cancel Fota request')
    cancel_pars.set_defaults(func=fota_cancel)
    cancel_pars.add_argument('id', help='Device id')
    cancel_pars.add_argument('object', help='Object id')
    cancel_pars.add_argument('instance', help='Instance id')

    update_pars = subparsers.add_parser('fota_update', help='Fota Update request')
    update_pars.set_defaults(func=fota_update)
    update_pars.add_argument('id', help='Device id')
    update_pars.add_argument('object', help='Object id')
    update_pars.add_argument('instance', help='Instance id')
    update_pars.add_argument('linked_instance', help='Instance id')


    arg = parser.parse_args()
    if arg.func is None:
        parser.print_help()
        sys.exit(0)
    if arg.debug:
        logging.getLogger().setLevel(level = logging.DEBUG)
    arg.func(arg)
