/*
 *  Copyright (c) 2023 Nordic Semiconductor ASA
 *
 *  SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#ifndef INTERNAL_HEADER_FILE
#define INTERNAL_HEADER_FILE

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <cracen/statuscodes.h>

#ifndef SX_EXTRA_IN_DESCS
#define SX_EXTRA_IN_DESCS 0
#endif

#define SX_BLKCIPHER_PRIV_SZ (16)
#define SX_AEAD_PRIV_SZ	     (70)

#define SX_MAX(p, q) ((p >= q) ? p : q)

/** Mode Register value for context loading */
#define BA417_MODEID_CTX_LOAD (1u << 5)
/** Mode Register value for context saving */
#define BA417_MODEID_CTX_SAVE (1u << 6)

struct sx3gppalg;
struct sx_blkcipher_cmdma_cfg;
struct sx_aead_cmdma_cfg;

/** A cryptomaster DMA descriptor */
struct sxdesc {
	uint8_t *addr;
	struct sxdesc *next;
	uint32_t sz;
	uint32_t dmatag;
};

/** Input and output descriptors and related state for cmdma */
struct sx_dmaslot {
	uint32_t cfg;
	struct sxdesc outdescs[5];
};

/** DMA controller
 *
 * For internal use only. Don't access directly.
 */
struct sx_dmactl {
	bool hw_acquired;
	struct sxdesc *d;
	struct sxdesc *out;
	uint8_t *mapped;
	struct sx_dmaslot dmamem;
};

/** Key reference
 *
 * Used for making a reference to a key stored in memory or to a key selected
 * by an identifier.
 * Created by sx_keyref_load_material() or sx_keyref_load_by_id(). Used by
 * blkcipher and aead creation functions.
 *
 * All members should be considered INTERNAL and may not be accessed directly.
 */
struct sxkeyref {
	const uint8_t *key;
	size_t sz;
	uint32_t cfg;
	int (*prepare_key)(const uint8_t *arg0);
	int (*clean_key)(const uint8_t *arg0);
	const uint8_t *user_data;
	uint32_t owner_id;
};

/** An AEAD operation
 *
 * To be used with sx_aead_*() functions.
 *
 * All members should be considered INTERNAL and may not be accessed
 * directly.
 */
struct sxaead {
	const struct sx_aead_cmdma_cfg *cfg;
	const uint8_t *expectedtag;
	uint32_t discardaadsz;
	uint32_t datainsz;
	size_t dataintotalsz;
	size_t totalaadsz;
	uint8_t tagsz;
	bool is_in_ctx;
	bool has_countermeasures;
	uint8_t ctxsz;
	const struct sxkeyref *key;
	struct sx_dmactl dma;
	struct sxdesc descs[7];
	uint8_t extramem[SX_AEAD_PRIV_SZ];
};

/** A simple block cipher operation
 *
 * To be used with sx_blkcipher_*() functions.
 *
 * All members should be considered INTERNAL and may not be accessed
 * directly.
 */
struct sxblkcipher {
	const struct sx_blkcipher_cmdma_cfg *cfg;
	const struct sxkeyref *key;
	uint32_t textsz;
	struct sx_dmactl dma;
	struct sxdesc descs[5];
	uint8_t extramem[SX_BLKCIPHER_PRIV_SZ];
};

/** A simple 3gpp operation
 *
 * To be used with sx_3gpp_*() functions.
 *
 * All members should be considered INTERNAL and may not be accessed
 * directly.
 */
struct sx3gpp {
	const struct sx3gppalg *cfg;
	struct sx_dmactl dma;
	struct sxdesc descs[5];
	uint32_t params[2];
	size_t insz;
	size_t outsz;
};

/** A simple MAC(message authentication code) operation
 *
 * To be used with sx_mac_*() functions.
 *
 * All members should be considered INTERNAL and may not be accessed
 * directly.
 */
struct sxmac {
	const struct sx_mac_cmdma_cfg *cfg;
	uint32_t cntindescs;
	uint32_t feedsz;
	int macsz;
	const struct sxkeyref *key;
	struct sx_dmactl dma;
	struct sxdesc descs[7];
	uint8_t extramem[16];
};

struct sxchannel {
	struct sx_dmactl dma;
	struct sxdesc descs[1];
};

/** A operation to load a countermeasures mask into the hardware.
 * This operation is based on channel.
 *
 * To be used with sx_cm_*() functions.
 *
 * All members should be considered INTERNAL and may not be accessed
 * directly.
 */
struct sxcmmask {
	struct sxchannel channel;
};

/**
 * @brief Function to handle CRACEN nested errors in the sxsymcrypt
 *
 * @param[in] nested_err	Nested error occurred while handling an error.
 * @param[in] err		Original error code.
 *
 * @return Return the nested error if it is not SX_OK, otherwise return
 *         the original error code.
 */
static inline int sx_handle_nested_error(int nested_err, int err)
{
	return nested_err ? nested_err != SX_OK : err;
}

#ifdef __cplusplus
}
#endif

#endif
