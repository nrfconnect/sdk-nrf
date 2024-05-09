#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

"""
Module communicating with the SSF (Secure Domain Service Framework)
using the ADAC protocol over the CTRLAP interface
"""

from enum import IntEnum
from typing import Any

import cbor2
import sdfw_adac_cmd
from sdfw_adac import Adac

ADAC_SSF_REMOTE_ID = 165


class SsfError(IntEnum):
    """Enum for SSF error messages"""

    SSF_SUCCESS = 0
    SSF_EPERM = 1
    SSF_EIO = 5
    SSF_ENXIO = 6
    SSF_EAGAIN = 11
    SSF_ENOMEM = 12
    SSF_EFAULT = 14
    SSF_EBUSY = 16
    SSF_EINVAL = 22
    SSF_ENODATA = 61
    SSF_EPROTO = 71
    SSF_EBADMSG = 77
    SSF_ENOBUFS = 105
    SSF_EADDRINUSE = 112
    SSF_ETIMEDOUT = 116
    SSF_EALREADY = 120
    SSF_EMSGSIZE = 122
    SSF_EPROTONOSUPPORT = 123
    SSF_ENOTSUP = 134


class Ssf:
    """Execute SSF service requests"""

    def __init__(
        self,
        adac: Adac,
        service_id: int,
        service_version: int,
    ) -> None:
        """
        :param adac: Used for performing ADAC operations
        :param service_id: ID of service
        :param service_version: Version of service.
        """
        self.adac = adac
        self.service_id = service_id
        self.service_version = service_version

    def request(self, service_request: bytes) -> Any:
        """
        Issue an SSF service request and read its response.
        The generic SSF header is added by this function.

        :param service_request: Service specific CBOR serialized data to write.
        """
        ssf_header = cbor2.dumps(
            (ADAC_SSF_REMOTE_ID, self.service_id, self.service_version)
        )
        full_request = ssf_header + service_request

        req = sdfw_adac_cmd.Ssf(full_request).to_request()

        rsp = self.adac.request(req)

        if rsp.status != sdfw_adac_cmd.AdacStatus.ADAC_SUCCESS:
            raise RuntimeError(f"Got ADAC failure: {rsp.status.name}")

        decoded = cbor2.loads(rsp.data)
        ssf_response = decoded[0]
        if ssf_response != SsfError.SSF_SUCCESS:
            raise RuntimeError(f"SSF operation failed: {SsfError(ssf_response)}")
        return decoded
