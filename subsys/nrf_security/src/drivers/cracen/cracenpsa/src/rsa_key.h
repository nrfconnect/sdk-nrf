/*
 *  Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef RSA_KEY_H
#define RSA_KEY_H

#include <silexpk/iomem.h>
#include <cracen_psa_primitives.h>
#include <silexpk/sxbuf/sxbufop.h>

/** Write the sizes of the elements of a RSA key. */
static inline void cracen_ffkey_write_sz(const struct cracen_rsa_key *key, int *sizes)
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
static inline void cracen_ffkey_write(const struct cracen_rsa_key *key, struct sx_pk_slot *inputs)
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

#define CRACEN_FFKEY_REFER_INPUT(key, array) (array)[key->dataidx]

static inline unsigned int cracen_op_bitsz(const struct sx_buf *op)
{
	unsigned int bitsz;
	size_t i;

	for (i = 0; i < op->sz && op->bytes[i] == 0; i++) {
	}
	bitsz = 8 * (op->sz - i);
	if (!bitsz) {
		return 0;
	}

	uint8_t v = op->bytes[i];

	while ((v & 0x80) == 0) {
		v = v << 1;
		bitsz--;
	}

	return bitsz;
}

static inline unsigned int cracen_ffkey_bitsz(const struct cracen_rsa_key *key)
{
	unsigned int bitsz = 0;
	int szelements = (key->slotmask >> 3) ? 2 : 1;

	for (int i = 0; i < szelements; i++) {
		bitsz += cracen_op_bitsz(key->elements[i]);
	}

	return bitsz;
}

#define CRACEN_RSA_KEY_OPSZ(key)                                                                   \
	((key)->elements[0]->sz + (((key)->slotmask >> 3) ? (key)->elements[1]->sz : 0))

#endif /* RSA_KEY_H */
