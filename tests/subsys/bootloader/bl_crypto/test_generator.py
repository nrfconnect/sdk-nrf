#!/usr/bin/env python3
#
# Copyright (c) 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

from ecdsa import SigningKey, NIST256p
import random
from hashlib import sha256
import operator
from os import linesep as ls
import sys


def c_code_declare_array(name, array):
	return "uint8_t %s[] = {%s};%s" % (name, ", ".join([hex(c) for c in array]), ls+ls)

def arr_to_hexstr(arr):
	return b''.join([bytes([x]) for x in arr])

def hexstr_to_array(hexstr):
	ret_str = ""
	for byte in map(operator.add, hexstr[::2], hexstr[1::2]):
		ret_str += "0x"+byte+","
	return ret_str[:-1]

if __name__ == "__main__":
	firmware = bytearray(random.randint(0, 255) for _ in range(random.randrange(4, 1000)))
	firmware_hash = sha256(firmware).digest()
	metadata = b"a%sb" % firmware_hash
	priv = SigningKey.generate(curve=NIST256p)
	pub = priv.get_verifying_key()
	sig = priv.sign(firmware_hash, hashfunc = sha256)
	pub_hash = sha256(pub.to_string()).digest()

	with open('fw_data.bin', 'rb') as f:
		fw_hex = f.read()

	fw_sk = SigningKey.generate(curve=NIST256p, hashfunc = sha256)
	fw_vk = fw_sk.get_verifying_key()
	generated_sig = fw_sk.sign(fw_hex, hashfunc = sha256)
	fw_hash = sha256(fw_hex).hexdigest()
	fw_hash = hexstr_to_array(fw_hash) 
	gen_sig = hexstr_to_array(generated_sig.hex())

	fw_x = fw_vk.pubkey.point.x()
	fw_pubkey_x = hexstr_to_array(fw_x.to_bytes(32, "big").hex())
	fw_y = fw_vk.pubkey.point.y()
	fw_pubkey_y = hexstr_to_array(fw_y.to_bytes(32, "big").hex())
	fw_pubkey = fw_pubkey_x +","+ fw_pubkey_y

	assert fw_vk.verify(generated_sig, fw_hex, hashfunc = sha256)

	sk = SigningKey.generate(curve=NIST256p, hashfunc = sha256)
	vk = sk.get_verifying_key()
	my_hash = b"breadcrumb"
	my_hash_array = hexstr_to_array(my_hash.hex())
	breadcrumb = sha256(b"breadcrumb")
	sha256_hash =  hexstr_to_array(breadcrumb.hexdigest())

	signature = sk.sign(my_hash)
	r = signature[:int(len(signature)/2)]
	s = signature[int(len(signature)/2):]
	sig_r = hexstr_to_array(r.hex())
	sig_s = hexstr_to_array(s.hex())
	sig_concat = hexstr_to_array(signature.hex())

	x = vk.pubkey.point.x()
	pubkey_x = hexstr_to_array(x.to_bytes(32, "big").hex())
	y = vk.pubkey.point.y()
	pubkey_y = hexstr_to_array(y.to_bytes(32, "big").hex())

	pubkey_concat = pubkey_x + "," + pubkey_y

	mcuboot_key = [0x30, 0x59, 0x30, 0x13, 0x06, 0x07, 0x2a, 0x86,
		0x48, 0xce, 0x3d, 0x02, 0x01, 0x06, 0x08, 0x2a,
		0x86, 0x48, 0xce, 0x3d, 0x03, 0x01, 0x07, 0x03,
		0x42, 0x00, 0x04, 0x2a, 0xcb, 0x40, 0x3c, 0xe8,
		0xfe, 0xed, 0x5b, 0xa4, 0x49, 0x95, 0xa1, 0xa9,
		0x1d, 0xae, 0xe8, 0xdb, 0xbe, 0x19, 0x37, 0xcd,
		0x14, 0xfb, 0x2f, 0x24, 0x57, 0x37, 0xe5, 0x95,
		0x39, 0x88, 0xd9, 0x94, 0xb9, 0xd6, 0x5a, 0xeb,
		0xd7, 0xcd, 0xd5, 0x30, 0x8a, 0xd6, 0xfe, 0x48, 0xb2, 0x4a, 0x6a, 0x81, 0x0e, 0xe5, 0xf0, 0x7d,
		0x8b, 0x68, 0x34, 0xcc, 0x3a, 0x6a, 0xfc, 0x53,
		0x8e, 0xfa, 0xc1]

	mcuboot_key_hash = sha256(b''.join(bytes([x]) for x in mcuboot_key))
	mcuboot_key_hash = hexstr_to_array(mcuboot_key_hash.hexdigest())

	mcuboot_key = b''.join(bytes([x]) for x in mcuboot_key)
	mcuboot_key = hexstr_to_array(mcuboot_key.hex())

	long_input = b'a' * 100000
	long_input_hash = hexstr_to_array(sha256(long_input).hexdigest())
	long_input = hexstr_to_array(long_input.hex())

	fw_sig = b'ac95651230dee1b857d29971fd5177cf4536ee4a819abaec950cccae27548a3823ff093cc2a64a8dab7f4df73dec98'

	assert vk.verify(signature, my_hash)

	with open(sys.argv[1], 'w') as f:
		f.write(c_code_declare_array("pk_hash", pub_hash))
		f.write(c_code_declare_array("pk", pub.to_string()))
		f.write(c_code_declare_array("sig", sig))
		f.write(c_code_declare_array("firmware_hash", firmware_hash))
		f.write(c_code_declare_array("firmware", firmware))
		f.write(c_code_declare_array("pub_x", pubkey_x))
		f.write(c_code_declare_array("pub_y", pubkey_y))
		f.write(c_code_declare_array("pub_concat", pubkey_concat))
		f.write(c_code_declare_array("const_pub_concat", pubkey_concat))
		f.write(c_code_declare_array("sig_r", sig_r))
		f.write(c_code_declare_array("sig_s", sig_s))
		f.write(c_code_declare_array("sig_concat", sig_concat))
		f.write(c_code_declare_array("const_sig_concat", sig_concat))
		f.write(c_code_declare_array("hash", my_hash_array))
		f.write(c_code_declare_array("const_hash", my_hash_array))
		f.write(c_code_declare_array("hash_sha256", sha256_hash))
		f.write(c_code_declare_array("const_hash_sha256", sha256_hash))
		f.write(c_code_declare_array("mcuboot_key", mcuboot_key))
		f.write(c_code_declare_array("const_mcuboot_key", mcuboot_key))
		f.write(c_code_declare_array("mcuboot_key_hash", mcuboot_key_hash))
		f.write(c_code_declare_array("long_input", long_input))
		f.write(c_code_declare_array("const_long_input", long_input))
		f.write(c_code_declare_array("long_input_hash", long_input_hash))
		f.write(c_code_declare_array("image_fw_data", hexstr_to_array(fw_hex.hex())))
		f.write(c_code_declare_array("image_fw_sig", hexstr_to_array(fw_sig_hex.hex())))
		f.write(c_code_declare_array("image_gen_sig", gen_sig))
		f.write(c_code_declare_array("image_public_key", fw_pubkey))
		f.write(c_code_declare_array("image_fw_hash", fw_hash))
		f.write(c_code_declare_array("const_fw_sig", hexstr_to_array(fw_sig_hex.hex())))
		f.write(c_code_declare_array("const_gen_sig", gen_sig))
		f.write(c_code_declare_array("const_public_key", fw_pubkey))
		f.write(c_code_declare_array("const_fw_hash", fw_hash))
		f.write(c_code_declare_array("const_fw_data", hexstr_to_array(fw_hex.hex())))
