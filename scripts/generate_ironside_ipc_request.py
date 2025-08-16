# Copyright (c) 2025 Nordic Semiconductor ASA
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

"""
Script for generating IPC request binary blobs for IronSide SE IPC requests.
Two segments are created, one for the IPC shared memory, and one for placing the data
that is pointed to by the IPC request.
The resulting data is printed to stdout in intelhex format unless an output file is given.
"""

import argparse
import binascii
import ctypes as c
import io
import json

from intelhex import IntelHex

TFM_CRYPTO_IMPORT_KEY_SID = (
    0x0203  # crypto import key function ID (group ID 2, function index 3)
)
TFM_CRYPTO_GENERATE_RANDOM_SID = (
    0x0100  # crypto generate random function ID (group ID 1, function index 0)
)
IRONSIDE_CALL_ID_PSA_V1 = 0  # Ironside call ID for PSA v1
PSA_IPC_CALL = 0x20000000  # PSA IPC call type
TFM_CRYPTO_HANDLE = 0x40000100  # see include/tfm/ironside/se/ipc_service.h
IPC_SHARED_MEM_ADDR = 0x2F88FB80  # Address used for IPC messages to/from IronSide SE
IRONSIDE_CALL_STATUS_REQ = 6  # Indicates that a client request is in the IPC buffer
KEY_ID_LEN = 4  # Length in bytes of key identifier


class TfmCryptoPackIovec(c.LittleEndianStructure):
    """Represents struct tfm_crypto_pack_iovec"""

    _pack_ = 1
    _fields_ = [
        ("key_id", c.c_uint32),
        ("alg", c.c_uint32),
        ("op_handle", c.c_uint32),
        ("ad_length", c.c_uint32),
        ("plaintext_length", c.c_uint32),
        ("aead_nonce", c.c_uint8 * 16),
        ("aead_nonce_length", c.c_uint32),
        ("function_id", c.c_uint16),
        ("step", c.c_uint16),
        ("_padding1", c.c_uint8 * 4),  # 4 bytes padding to align union
        ("capacity_or_value", c.c_uint64),  # union
        ("role", c.c_uint8),
        ("_padding2", c.c_uint8 * 7),  # 7 bytes padding for 8-byte alignment
    ]


class PsaVec(c.LittleEndianStructure):
    """Represents psa_[in/out]vec structure"""

    _pack_ = 1
    _fields_ = [("base", c.c_uint32), ("length", c.c_uint32)]


class PsaInvec(PsaVec):
    """Represents psa_invec structure"""


class PsaOutvec(PsaVec):
    """Represents psa_outvec structure"""


IronsideCallBufArgs = c.c_uint32 * 7


class IronsideCallBuf(c.LittleEndianStructure):
    """Represents struct ironside_call_buf"""

    _pack_ = 1
    _fields_ = [
        ("status", c.c_uint16),
        ("id", c.c_uint16),
        ("args", IronsideCallBufArgs),
    ]


class MemoryBlob:
    """Manages a virtual memory blob with address tracking"""

    def __init__(self, base_address: int):
        self.base_address = base_address
        self.data = bytearray()
        self.current_address = base_address

    def append(self, data: bytes) -> int:
        """Append data to the blob and return its address"""
        address = self.current_address
        self.data.extend(data)
        self.current_address += len(data)
        return address


def generate_psa_ipc_request(
    function_id: int,
    input_data_list: list[bytes],
    output_specs: list[tuple[int, int]],
    base_address: int,
) -> tuple[bytes, bytes]:
    """
    Generate IPC request data for PSA crypto functions.

    Args:
        function_id: TFM crypto function ID
        input_data_list: List of (address, length) tuples for input buffers (beyond the iovec)
        output_specs: List of (address, length) tuples for output buffers
        base_address: Base address for memory blob

    Returns:
        (ipc_request_data, memory_blob_data)
    """
    memory = MemoryBlob(base_address)

    # All requests has this
    iovec = TfmCryptoPackIovec(function_id=function_id)
    iovec_addr = memory.append(bytes(iovec))
    invecs = [PsaInvec(base=iovec_addr, length=c.sizeof(iovec))]

    for data in input_data_list:
        data_addr = memory.append(data)
        invecs.append(PsaInvec(base=data_addr, length=len(data)))

    in_vec_addr = memory.current_address
    for vec in invecs:
        memory.append(bytes(vec))

    out_vecs = []
    for address, length in output_specs:
        out_vecs.append(PsaOutvec(base=address, length=length))

    out_vec_addr = memory.current_address
    for vec in out_vecs:
        memory.append(bytes(vec))

    call_buf = IronsideCallBuf(
        status=IRONSIDE_CALL_STATUS_REQ,
        id=IRONSIDE_CALL_ID_PSA_V1,
        args=IronsideCallBufArgs(
            TFM_CRYPTO_HANDLE,  # args[0] - TFM_CRYPTO_HANDLE
            in_vec_addr,  # args[1] - address of in_vec
            len(invecs),  # args[2] - number of entries in invecs
            out_vec_addr,  # args[3] - address of out_vec
            len(out_vecs),  # args[4] - number of entries in out_vec
            0,  # args[5] - status, not set by the client
            PSA_IPC_CALL,  # args[6] - PSA_IPC_CALL
        ),
    )

    return bytes(call_buf), bytes(memory.data)


def generate_import_key_request(
    psa_key_attrs_hex: str,
    key_value_hex: str,
    base_address: int,
    key_id_address: int,
    key_id_len: int,
) -> tuple[bytes, bytes]:
    """
    Generate IPC request data for the psa_import_key function.

    Returns:
        (ipc_request_data, memory_blob_data)
    """

    def parse_hex_string(hex_str: str) -> bytes:
        return binascii.unhexlify(hex_str.removeprefix("0x"))

    psa_key_attrs_data = parse_hex_string(psa_key_attrs_hex)
    key_data = parse_hex_string(key_value_hex)

    return generate_psa_ipc_request(
        function_id=TFM_CRYPTO_IMPORT_KEY_SID,
        input_data_list=[psa_key_attrs_data, key_data],
        output_specs=[(key_id_address, key_id_len)],
        base_address=base_address,
    )


def generate_random_request(
    output_address: int,
    output_length: int,
    base_address: int,
) -> tuple[bytes, bytes]:
    """
    Generate IPC request data for the psa_generate_random function.

    Returns:
        (ipc_request_data, memory_blob_data)
    """
    # psa_generate_random only needs the output buffer, no input data beyond iovec
    return generate_psa_ipc_request(
        function_id=TFM_CRYPTO_GENERATE_RANDOM_SID,
        input_data_list=[],  # No additional input data beyond iovec
        output_specs=[(output_address, output_length)],
        base_address=base_address,
    )


def main():
    parser = argparse.ArgumentParser(
        description="Generate IPC request binary blobs for PSA operations",
        allow_abbrev=False,
    )

    parser.add_argument(
        "--base-address",
        help="Base address for memory blob. This acts as the working area for the requests. All "
        "data pointed to by the request is stored here. Any unused RAM available to the core that "
        "is used to execute the IPC request can be given",
        default=0x22000000,
        type=lambda x: int(x, 0),
    )

    parser.add_argument(
        "--output",
        help="Output JSON filename (if not provided, outputs Intel HEX to stdout)",
        type=argparse.FileType("wb"),
        required=False,
    )

    subparsers = parser.add_subparsers(
        dest="operation",
        help="IPC operation to perform",
        required=True,
    )

    import_parser = subparsers.add_parser(
        "psa_import_key",
        help="Invoke psa_import_key through the PSA service",
    )

    import_parser.add_argument(
        "--psa-key-attrs",
        help="PSA key attributes as hex string. This can be found by running the "
        "generate_psa_key_attributes.py script",
        type=str,
        required=True,
    )

    import_parser.add_argument(
        "--key-value",
        help="Key value as hex string",
        type=str,
        required=True,
    )

    import_parser.add_argument(
        "--key-id-address",
        help="The address to store the resulting key_id to. The psa_import_key outputs the key_id "
        " of the imported key. For persistent keys this will be the same value that was given as "
        "input. It is still required to provide an address where this output key_id can be "
        "written.",
        type=lambda x: int(x, 0),
        default=0x22001000,
    )

    random_parser = subparsers.add_parser(
        "psa_generate_random",
        help="Invoke psa_generate_random through the PSA service",
    )

    random_parser.add_argument(
        "--output-address",
        help="Base address for random output buffer (hex or decimal)",
        type=lambda x: int(x, 0),
        required=True,
    )

    random_parser.add_argument(
        "--output-length",
        help="Length of random output buffer (hex or decimal)",
        type=lambda x: int(x, 0),
        required=True,
    )

    args = parser.parse_args()

    if args.operation == "psa_import_key":
        ipc_request, request_data = generate_import_key_request(
            args.psa_key_attrs,
            args.key_value,
            args.base_address,
            args.key_id_address,
            KEY_ID_LEN,
        )
    elif args.operation == "psa_generate_random":
        ipc_request, request_data = generate_random_request(
            args.output_address,
            args.output_length,
            args.base_address,
        )

    if args.output:
        output_data = {
            "ipc_request": binascii.hexlify(ipc_request).decode("utf-8").upper(),
            "request_data": binascii.hexlify(request_data).decode("utf-8").upper(),
            "address": f"0x{args.base_address:08X}",
        }

        with open(args.output, "w", encoding="utf-8") as f:
            json.dump(output_data, f, indent=4)
    else:
        # Output intelhex to stdout
        ih = IntelHex()
        ih.puts(args.base_address, request_data)
        ih.puts(IPC_SHARED_MEM_ADDR, ipc_request)
        output = io.StringIO()
        ih.write_hex_file(output)
        print(output.getvalue().rstrip())


if __name__ == "__main__":
    main()
