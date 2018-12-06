#!/usr/bin/env python3
#
# Copyright (c) 2018 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic

import ecdsa
import random
import hashlib
from os import linesep as ls
import sys


def c_code_declare_array(name, array):
	return "uint8_t %s[] = {%s};%s" % (name, ", ".join([hex(c) for c in array]), ls+ls)


if __name__ == "__main__":
	firmware = bytearray(random.randint(0, 255) for _ in range(random.randrange(4, 1000)))
	firmware_hash = hashlib.sha256(firmware).digest()
	metadata = b"a%sb" % firmware_hash
	priv = ecdsa.SigningKey.generate(curve=ecdsa.NIST256p)
	pub = priv.get_verifying_key()
	# sig = priv.sign(firmware, hashfunc = hashlib.sha256)
	sig = priv.sign(firmware_hash, hashfunc = hashlib.sha256)
	pub_hash = hashlib.sha256(pub.to_string()).digest()

	with open(sys.argv[1], 'w') as f:
		f.write(c_code_declare_array("pk_hash", pub_hash))
		f.write(c_code_declare_array("pk", pub.to_string()))
		# f.write(c_code_declare_array("metadata", metadata))
		f.write(c_code_declare_array("sig", sig))
		f.write(c_code_declare_array("firmware_hash", firmware_hash))
		f.write(c_code_declare_array("firmware", firmware))
