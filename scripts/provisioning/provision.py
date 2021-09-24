#!/usr/bin/env python3
#
# Copyright (c) 2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

import argparse
import os
import sys
import logging
import base64
import os.path
from enum import Enum
from subprocess import Popen, PIPE
from MoSH import MoSH
from pathlib import Path
import binascii
from cryptography.hazmat.primitives.asymmetric import ec
from cryptography.hazmat.primitives.serialization import load_der_public_key
from Cryptodome.PublicKey import ECC

# Used AT commands
cmng = "AT%CMNG="
keygen = "AT%KEYGEN="
xpmng = "AT%XPMNG="

class TargetCredentials(Enum):
    srv = "Server"
    clt = "Client"
    root = "Root"

    def __str__(self):
        return self.value

def provision_server_public_key(modem, sec_tag, ipemfile):

    # Check and remove existing credentials
    if modem.at_cmd(",".join([xpmng + "2", "", sec_tag])):
        modem.at_cmd(",".join([cmng + "3", sec_tag, "5"]))
        logging.info(
            "Removed existing public key with security tag " + sec_tag)

    # Write the PEM formatted PBK to modem
    with open(ipemfile) as f:
        pbk_pem = f.read()
        # Reference manual explicitly states that delete is only allowed opcode
        # but still, this works...
        if modem.at_cmd(",".join([cmng + "0", sec_tag, "5", "\"" + pbk_pem + "\""])):
            logging.error("Unable to commission new credentials")
            exit

    # You can't list public keys with CMNG, at least not yet
    if modem.at_cmd(",".join([cmng + "2", sec_tag, "5"])):
        logging.debug("Provisioning - SUCCESS")
    else:
        logging.error("Provisioning - FAILURE")
        exit(1)

def provision_root_ca(modem, sec_tag, ipemfile):

    # Check and remove existing credentials
    modem.at_cmd(",".join([cmng + "3", sec_tag, "0"]))

    # Write the PEM formatted CA to modem
    with open(ipemfile) as f:
        pbk_pem = f.read()
        if modem.at_cmd(",".join([cmng + "0", sec_tag, "0", "\"" + pbk_pem + "\""])):
            logging.error("Unable to commission root CA")
            exit

    if modem.at_cmd(",".join([cmng + "2", sec_tag, "0"])):
        logging.debug("Provisioning - SUCCESS")
    else:
        logging.error("Provisioning - FAILURE")
        exit(1)


def provision_client_key_pair(modem, sec_tag, opemfile):

    # Check and remove existing client private key
    if modem.at_cmd(",".join([cmng + "1", sec_tag])):
        modem.at_cmd(",".join([cmng + "3", sec_tag, "2"]))
        logging.info(
            "Removed existing private key with security tag " + sec_tag)

    # Generate client private key to given sec tag, public key as output
    resp = modem.at_cmd(",".join([keygen + sec_tag, "2", "1"]))

    if not resp:
        logging.error("Unable to generate client key pair")
        exit

    pbk_b64 = resp.split('"')[1].split('.')[0]
    pbk_der = base64.urlsafe_b64decode(pbk_b64 + '===')

    # Write the DER formatted PBK to a file
    with open(os.path.splitext(opemfile)[0] + ".der", 'wb') as df:
        df.write(pbk_der)
        logging.info("Wrote DER file %s", df.name)

    # Write the same PBK in HEX format to a file
    with open(os.path.splitext(opemfile)[0] + ".der", 'rb') as df:
        bkey = df.read()
        with open(os.path.splitext(opemfile)[0] + ".hex", 'wb') as hf:
            hf.write(binascii.hexlify(bkey))
            logging.info("Wrote HEX file %s", hf.name)

    # Write the same PBK in PEM format to a file
    with open(os.path.splitext(opemfile)[0] + ".der", 'rb') as df:
        key = ECC.import_key(df.read())

        with open(opemfile, 'w') as pf:
            pf.write(key.export_key(format='PEM'))
            logging.info("Exported generated public key to file %s", pf.name)

    # You can't read private keys with CMNG, only list
    if modem.at_cmd(",".join([cmng + "1", sec_tag])):
        logging.debug("Provisioning - SUCCESS")
    else:
        logging.error("Provisioning - FAILURE")
        exit(1)


def main():

    logging.basicConfig(level=logging.DEBUG)

    # Create the parser
    parser = argparse.ArgumentParser(description='Provision credentials')

    # Add the arguments
    parser.add_argument('target', type=TargetCredentials, choices=list(TargetCredentials),
                        help='Write Server\'s public key or generate client key pair')
    parser.add_argument('-t', '--tty',      metavar='tty',
                        type=str, help='serial port path')
    parser.add_argument('-b', '--baudrate', metavar='baudrate',
                        type=str, help='serial port speed')
    parser.add_argument('-k', '--key_file', metavar='key_file', type=str,
                        help='PEM formatted public key file')
    parser.add_argument('-s', '--sec_tag',  metavar='sec_tag',  type=str,
                        help='Security tag number - [0–9] for server PBK; [0–2147483647] for client private key')

    args = parser.parse_args()

    if not args.tty:
        parser.error('No tty')
    if not args.baudrate:
        parser.error('No baudrate')
    if not args.key_file:
        parser.error('No key file')
    else:
        kfile = os.path.expanduser(args.key_file)
    if str(args.target) == "Server":
        if not os.path.isfile(kfile):
            parser.error("No PEM formatted file found with given name")
    if str(args.target) == "Client":
        if os.path.isfile(kfile):
            parser.error("Can't overwrite an existing key file")

    if not args.sec_tag:
        parser.error('No security tag')

    # Open AT shell connection
    modem = MoSH(args.tty, args.baudrate)
    if not modem.is_ready():
        logging.error("Modem is not ready")
        exit(1)

    # Power off the modem
    if modem.at_cmd('AT+CFUN=0'):
        logging.error("Modem can't be powered off")
        exit(1)

    if str(args.target) == "Server":
        provision_server_public_key(modem, args.sec_tag, kfile)

    if str(args.target) == "Client":
        provision_client_key_pair(modem, args.sec_tag, kfile)

    if str(args.target) == "Root":
        provision_root_ca(modem, args.sec_tag, kfile)


if __name__ == "__main__":
    main()
