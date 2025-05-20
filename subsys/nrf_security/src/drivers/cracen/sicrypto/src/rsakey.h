/*
 *  Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef RSAKEY_HEADER_FILE
#define RSAKEY_HEADER_FILE

#include <silexpk/iomem.h>
#include "../include/sicrypto/rsa_keys.h"

/** Write the sizes of the elements of a RSA key. */
static void si_ffkey_write_sz(const struct si_rsa_key *key, int *sizes)
{
	int slotidx = 0;
	int i = 0;
	unsigned int mask = key->slotmask;

	while (mask) {
		if (mask & 1) {
			sizes[slotidx] = key->elements[i]->sz;
			i++;
		}
		mask >>= 1;
		slotidx++;
	}
}

/** Write the elements of a RSA key into the input slots. */
static void si_ffkey_write(const struct si_rsa_key *key, struct sx_pk_slot *inputs)
{
	int slotidx = 0;
	int i = 0;
	unsigned int mask = key->slotmask;

	while (mask) {
		if (mask & 1) {
			sx_wrpkmem(inputs[slotidx].addr, key->elements[i]->bytes,
				   key->elements[i]->sz);
			i++;
		}
		mask >>= 1;
		slotidx++;
	}
}

#define SI_FFKEY_REFER_INPUT(key, array) (array)[key->dataidx]

static inline unsigned int si_op_bitsz(const struct sx_buf *op)
{
	unsigned int bitsz;
	size_t i;

	for (i = 0; i < op->sz && op->bytes[i] == 0; i++) {
	}
	bitsz = 8 * (op->sz - i);
	if (!bitsz) {
		return 0;
	}

	unsigned char v = (unsigned char)op->bytes[i];

	while ((v & 0x80) == 0) {
		v = v << 1;
		bitsz--;
	}

	return bitsz;
}

static inline unsigned int si_ffkey_bitsz(const struct si_rsa_key *key)
{
	unsigned int bitsz = 0;
	int szelements = (key->slotmask >> 3) ? 2 : 1;

	for (int i = 0; i < szelements; i++) {
		bitsz += si_op_bitsz(key->elements[i]);
	}

	return bitsz;
}

#define SI_RSA_KEY_OPSZ(key)                                                                       \
	((key)->elements[0]->sz + (((key)->slotmask >> 3) ? (key)->elements[1]->sz : 0))

#endif
