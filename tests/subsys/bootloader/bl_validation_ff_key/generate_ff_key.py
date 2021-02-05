#
# Copyright (c) 2020 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

from ecdsa import SigningKey
from ecdsa.curves import NIST256p
import hashlib
import sys

for i in range(100000):
    sk = SigningKey.generate(curve=NIST256p)
    vk = sk.get_verifying_key()
    vk_hash = hashlib.sha256(vk.to_string()).digest()[:16]

    # Search for aligned 0xFFFF.
    for u16 in zip(vk_hash[::2], vk_hash[1::2]):
        if u16 == (0xff, 0xff):
            print(vk_hash)
            with open('ff.pem', 'wb') as f:
                f.write(sk.to_pem())
            with open('ff_pub.pem', 'wb') as f:
                f.write(vk.to_pem())
            sys.exit()
