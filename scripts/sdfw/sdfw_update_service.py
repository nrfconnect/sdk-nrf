#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

import argparse
import enum
from pathlib import Path

import cbor2
import zcbor
from ctrlap import Ctrlap
from sdfw_adac import Adac
from ssf_ctrlap import Ssf

SSF_SDFW_UPDATE_SERVICE_ID = 0x69
SSF_SDFW_UPDATE_SERVICE_VERSION = 1
CDDL_PATH = str(
    Path(__file__).absolute().parents[2]
    / "subsys/sdfw_services/services/sdfw_update/sdfw_update_service.cddl"
)

try:
    sdfw_update_service_cddl = zcbor.DataTranslator.from_cddl(
        open(CDDL_PATH, "r").read(), 16
    ).my_types["sdfw_update_req"]
except Exception as e:
    raise RuntimeError(f"Unable to load CDDL from {CDDL_PATH}") from e


class SdfwUpdateStatus(enum.IntEnum):
    """Enum for SDFW Update service status"""

    SUCCESS = 0
    UPDATE_FAILED = 0x3dfc0001


def get_arguments() -> argparse.Namespace:
    """
    Parse command line options.

    :return: argparse arguments.
    """

    parser = argparse.ArgumentParser(
        allow_abbrev=False,
        description="sdfw_update_service invokes the SDFW update service over ADAC.",
    )

    Ctrlap.add_arguments(parser)

    for param in sdfw_update_service_cddl.value:
        parser.add_argument(
            f"--{param.label.replace('_','-')}",
            type=lambda number_string: int(number_string, 0),
            required=True,
            help=f"Value to pass to {param}.",
        )

    parsed_args = parser.parse_args()

    return parsed_args


def create_update_req(**kwargs) -> bytes:
    """
    Create an SDFW update service request

    :param blob_addr:
    :return: The CBOR formatted request
    """

    write_req_raw = (
        kwargs["tbs_addr"],
        kwargs["dl_max"],
        kwargs["dl_addr_fw"],
        kwargs["dl_addr_pk"],
        kwargs["dl_addr_signature"],
    )

    write_req_cbor = cbor2.dumps(write_req_raw)

    sdfw_update_service_cddl.validate_str(write_req_cbor)

    # Verify that the request was populated as expected
    decoded = sdfw_update_service_cddl.decode_str(write_req_cbor)
    assert decoded.tbs_addr == kwargs["tbs_addr"]
    assert decoded.dl_max == kwargs["dl_max"]
    assert decoded.dl_addr_fw == kwargs["dl_addr_fw"]
    assert decoded.dl_addr_pk == kwargs["dl_addr_pk"]
    assert decoded.dl_addr_signature == kwargs["dl_addr_signature"]

    return write_req_cbor


def main() -> None:
    args = get_arguments()

    cbor_request = create_update_req(**vars(args))

    adac = Adac(Ctrlap(**vars(args)))

    adac.ctrlap.connector.connect()

    ssf = Ssf(
        adac=adac,
        service_id=SSF_SDFW_UPDATE_SERVICE_ID,
        service_version=SSF_SDFW_UPDATE_SERVICE_VERSION,
    )

    update_retval = ssf.request(service_request=cbor_request)

    print(
        f"sdfw_update_service: Response status code: {SdfwUpdateStatus(update_retval[0]).name}"
    )

    adac.ctrlap.connector.disconnect()


if __name__ == "__main__":
    main()
