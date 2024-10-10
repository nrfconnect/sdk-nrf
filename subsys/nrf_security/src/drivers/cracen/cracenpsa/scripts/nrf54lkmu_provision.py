#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

import binascii
import sys
from pynrfjprog import LowLevel
from ctypes import *
import enum
import argparse
import itertools

PushArea = 0x20000000

SrcAddress = 0x50045504
ProvisionTaskAddress = 0x50045000
RevokeTaskAddress = 0x50045008
ReadMetadataTaskAddress = 0x5004500C
ProvisionedEventAddress = 0x50045100
RevokedEventAddress = 0x50045108
MetadataReadEventAddress = 0x50045110
ErrorEventAddress = 0x5004510c
KeySlotAddress = 0x50045500
MetadataAddress = 0x50045508

class KeyUsageScheme(enum.IntEnum):
    PROTECTED = 0
    SEED = 1
    ENCRYPTED = 2
    RAW = 3

class Rpolicy(enum.IntEnum):
    ROTATING = 1
    LOCKED = 2
    REVOKED = 3

class KeyBits(enum.IntEnum):
    BITS_128 = 1
    BITS_192 = 2
    BITS_255 = 3
    BITS_256 = 4
    BITS_384_SEED = 5
    RESERVED1 = 6
    RESERVED2 = 7

class Algorithm(enum.IntEnum):
    CHACHA20 = 1
    CHACHA20_POLY1305 = 2
    AES_GCM = 3
    AES_CCM = 4
    AES_ECB = 5
    AES_CTR = 6
    AES_CBC = 7
    SP800_108_COUNTER_CMAC = 8
    CMAC = 9
    ED25519 = 10
    ECDSA = 11
    RESERVED2 = 12
    RESERVED3 = 13
    RESERVED4 = 14
    RESERVED5 = 15

class UsageFlags(enum.IntFlag):
    ENCRYPT = enum.auto()
    DECRYPT = enum.auto()
    SIGN = enum.auto()
    VERIFY = enum.auto()
    DERIVE = enum.auto()
    EXPORT = enum.auto()
    COPY = enum.auto()

class Metadata(Structure):
    _fields_ = [('version', c_uint, 4),
                ('key_usage_scheme', c_uint, 2),
                ('reserved', c_uint, 10),
                ('algorithm', c_uint, 4),
                ('size', c_uint, 3),
                ('rpolicy', c_uint, 2),
                ('usage_flags', c_uint, 7)]
    def __str__(self):
        s = ""
        s += str(KeyUsageScheme(self.key_usage_scheme))
        s += " "
        s += str(Algorithm(self.algorithm))
        s += " "
        s += str(KeyBits(self.size))
        s += " "
        s += str(Rpolicy(self.rpolicy))
        s += " "
        s += str(UsageFlags(self.usage_flags))

        return s

    def is_secondary_slot(self):
        return bytes(self) == b'\xff\xff\xff\xff'
SecondarySlot = Metadata.from_buffer_copy(b'\xff\xff\xff\xff')

class KMUDescriptor(Structure):
    _fields_ = [("value", c_ubyte*16),
                ("rpolicy", c_uint),
                ("dest", c_uint),
                ("metadata", Metadata)]

def clear_events(api):
    api.write_u32(ProvisionedEventAddress, 0, False)
    api.write_u32(ErrorEventAddress, 0, False)
    api.write_u32(RevokedEventAddress, 0, False)
    api.write_u32(MetadataReadEventAddress, 0, False)

def provision_ed25519_key(api, args):
    keyslot = args.keyslot
    keydata = args.key

    print(f"Provisioning keyslot {keyslot}: ", end='')

    if len(keydata) != 32:
        print("Unexpected size of key data", len(keydata))
        sys.exit(-1)

    metadata = Metadata()
    metadata.rpolicy = Rpolicy.REVOKED
    metadata.key_usage_scheme = KeyUsageScheme.RAW
    metadata.size = KeyBits.BITS_255
    metadata.algorithm = Algorithm.ED25519
    metadata.usage_flags = UsageFlags.VERIFY

    push_address = PushArea

    iterator = iter(keydata)
    while keychunk := tuple(itertools.islice(iterator, 16)):
        value = (c_ubyte*16)(*keychunk)
        desc = KMUDescriptor(value, Rpolicy.REVOKED, push_address, metadata)

        clear_events(api)
        api.write_u32(KeySlotAddress, keyslot, False)
        api.write_u32(SrcAddress, PushArea, False)
        api.write(PushArea, bytearray(desc), False)
        api.write_u32(ProvisionTaskAddress, 1, True)
        check_event = api.read_u32(ProvisionedEventAddress)
        error_event = api.read_u32(ErrorEventAddress)

        if check_event == 0 or error_event == 1:
            print("KMU reported error.")
            exit(-1)

        # advance to secondary slot
        metadata = SecondarySlot
        keyslot += 1
        push_address += 16
    print("Success.")


def revoke_key(api, args):
    keyslot = args.keyslot
    print(f"Revoking keyslot {keyslot}: ", end='')

    for i in range(2):
        clear_events(api)
        api.write_u32(KeySlotAddress, keyslot + i, False)
        api.write_u32(RevokeTaskAddress, 1, True)

        check_event = api.read_u32(RevokedEventAddress)
        error_event = api.read_u32(ErrorEventAddress)
        if check_event == 0 or error_event == 1:
            print("KMU reported error.")
            exit(-1)

    print("Success.")

def dump_content(api, args):
    clear_events(api)
    last_slot = -1
    for i in range(256):
        api.write_u32(KeySlotAddress, i, False)
        api.write_u32(ReadMetadataTaskAddress, 1, True)

        error_event = api.read_u32(ErrorEventAddress)
        revoked_event = api.read_u32(RevokedEventAddress)
        metadata_event = api.read_u32(MetadataReadEventAddress)

        if metadata_event:
            metadata = Metadata.from_buffer_copy(bytes(api.read(MetadataAddress, 4)))
        clear_events(api)

        if error_event:
            print(f"{i}\tUnprovisioned")

        if revoked_event:
            print(f"{i}\tRevoked")

        if metadata_event:
            if not metadata.is_secondary_slot():
                try:
                    print(f"{i}\t{metadata}")
                except ValueError:
                    print(f"{i}\tInvalid/unknown metadata")
                last_slot = i
            else:
                print(f"{i}\tcontinuation of {last_slot}")

def keyslot_int(value):
    ivalue = int(value)
    if ivalue < 0:
        raise argparse.ArgumentTypeError("Key slot index must be positive")
    if ivalue > 250:
        raise argparse.ArgumentTypeError(f"Key slot index {ivalue} is invalid")
    return ivalue

if __name__ == '__main__':
    parser = argparse.ArgumentParser(prog='nRF54L KMU provisioning script', allow_abbrev=False)
    parser.add_argument('--snr', help='Segger serial number', type=int)

    subparser = parser.add_subparsers(required=True)
    parse_dump = subparser.add_parser('dump')
    parse_dump.set_defaults(func=dump_content)

    parse_key = subparser.add_parser('provision')
    parse_key.add_argument('--keyslot', required=True, type=keyslot_int,
            help="Index of the key slot to provision")
    parse_key.add_argument('key', help='Hex formatted ed25519 public key', type=binascii.unhexlify)
    parse_key.set_defaults(func=provision_ed25519_key)

    parse_revoke = subparser.add_parser('revoke')
    parse_revoke.add_argument('--keyslot', required=True, type=keyslot_int,
            help="Index of the key slot to provision")
    parse_revoke.set_defaults(func=revoke_key)

    args = parser.parse_args()

    with LowLevel.API('NRF54L') as api:
        if args.snr:
            api.connect_to_emu_with_snr(args.snr, jlink_speed_khz=1000)
        else:
            api.connect_to_emu_without_snr(jlink_speed_khz=1000)
        args.func(api, args)
