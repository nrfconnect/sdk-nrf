/** Cryptomaster DMA (descriptors and control) and hardware detection
 *
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "crypmasterregs.h"
#include <security/cracen.h>
#include <sxsymcrypt/internal.h>

#ifndef CMDMA_HEADER_FILE
#define CMDMA_HEADER_FILE

#define DMATAG_BA411	      (1)
#define DMATAG_BA412	      (2)
#define DMATAG_BA413	      (3)
#define DMATAG_BA417	      (4)
#define DMATAG_BA418	      (5)
#define DMATAG_AESGCM	      (6)
#define DMATAG_AESXTS	      (7)
#define DMATAG_BA419	      (0x0B)
#define DMATAG_CONFIG(offset) ((1 << 4) | (offset << 8))

/* can be 0, 1 or 2 */
#define DMATAG_DATATYPE(x)   (x << 6)
#define DMATAG_CFG_BA415_W   (DMATAG_AESGCM | DMATAG_CONFIG(0))
#define DMATAG_CFG_BA415_KEY (DMATAG_AESGCM | DMATAG_CONFIG(0x10))
#define DMATAG_CFG_BA415_IV  (DMATAG_AESGCM | DMATAG_CONFIG(0x30))

#define DMATAG_CFG_BA416_W    (DMATAG_AESXTS | DMATAG_CONFIG(0))
#define DMATAG_CFG_BA416_IV   (DMATAG_AESXTS | DMATAG_CONFIG(0x10))
#define DMATAG_CFG_BA416_KEY1 (DMATAG_AESXTS | DMATAG_CONFIG(0x20))
#define DMATAG_CFG_BA416_KEY2 (DMATAG_AESXTS | DMATAG_CONFIG(0x40))

#define DMATAG_DATATYPE_HEADER	    (1 << 6)
#define DMATAG_DATATYPE_REFERENCE   (3 << 6)
#define DMATAG_LAST		    (1 << 5)
#define DMATAG_INVALID_BYTES_MASK   0x1F
#define DMATAG_INVALID_BYTES_OFFSET 8
#define DMATAG_IGN(sz)		    ((sz) << DMATAG_INVALID_BYTES_OFFSET)

#define DMA_LAST_DESCRIPTOR ((struct sxdesc *)1)
#define DMA_REALIGN	    (1 << 29)
#define DMA_DISCARD	    (1 << 30)

#define DMA_BUS_FETCHER_ERROR_MASK (1 << 2)
#define DMA_BUS_PUSHER_ERROR_MASK  (1 << 5)

#define ADD_CFGDESC(dmactl, baddr, bsz, tag)                                                       \
	do {                                                                                       \
		(dmactl).d->addr = (char *)(baddr);                                                \
		(dmactl).d->sz = (uint32_t)(bsz) | DMA_REALIGN;                                    \
		(dmactl).d->dmatag = tag;                                                          \
		(dmactl).d++;                                                                      \
	} while (0)

#define ADD_INDESC_PRIV(dmactl, offset, bsz, tag)                                                  \
	do {                                                                                       \
		(dmactl).d->addr = (dmactl).mapped + (offset);                                     \
		(dmactl).d->sz = (uint32_t)(bsz) | DMA_REALIGN;                                    \
		(dmactl).d->dmatag = tag;                                                          \
		(dmactl).d++;                                                                      \
	} while (0)

#define ADD_INDESC_PRIV_RAW(dmactl, offset, bsz, tag)                                              \
	do {                                                                                       \
		(dmactl).d->addr = (dmactl).mapped + (offset);                                     \
		(dmactl).d->sz = (uint32_t)(bsz);                                                  \
		(dmactl).d->dmatag = tag;                                                          \
		(dmactl).d++;                                                                      \
	} while (0)

#define ADD_RAW_INDESC(dmactl, baddr, bsz, tag)                                                    \
	do {                                                                                       \
		(dmactl).d->addr = (char *)(baddr);                                                \
		(dmactl).d->sz = (uint32_t)(bsz);                                                  \
		(dmactl).d->dmatag = (tag);                                                        \
		(dmactl).d++;                                                                      \
	} while (0)

#define ADD_INDESC_IGN(dmactl, baddr, bsz, ignsz, tag)                                             \
	do {                                                                                       \
		(dmactl).d->addr = (char *)(baddr);                                                \
		(dmactl).d->sz = (uint32_t)(bsz) | DMA_REALIGN;                                    \
		(dmactl).d->dmatag = tag | DMATAG_IGN(ignsz);                                      \
		(dmactl).d++;                                                                      \
	} while (0)

#define ALIGN_SZA(sz, msk) (((sz) + (msk)) & ~(msk))
/* addr needs to be set to some DMA fetchable address even though the data will not be used */
#define ADD_EMPTY_INDESC(dmactl, ignb, tag)                                                        \
	do {                                                                                       \
		(dmactl).d->addr = (char *)&(dmactl).dmamem;                                       \
		(dmactl).d->sz = ignb | DMA_REALIGN;                                               \
		(dmactl).d->dmatag = tag | DMATAG_IGN(ignb & DMATAG_INVALID_BYTES_MASK);           \
		(dmactl).d++;                                                                      \
	} while (0)

#define ADD_INDESCA(dmactl, baddr, bsz, tag, msk)                                                  \
	do {                                                                                       \
		uint32_t asz = ALIGN_SZA(bsz, msk);                                                \
		(dmactl).d->addr = (char *)(baddr);                                                \
		(dmactl).d->sz = asz | DMA_REALIGN;                                                \
		(dmactl).d->dmatag = tag | DMATAG_IGN(asz - (bsz));                                \
		(dmactl).d++;                                                                      \
	} while (0)

#define SET_LAST_DESC_IGN(d, bsz, msk)                                                             \
	do {                                                                                       \
		size_t ign = ALIGN_SZA(bsz, msk) - bsz;                                            \
		(d)->dmatag |= DMATAG_IGN(ign);                                                    \
		(d)->sz = ((d)->sz + ign) | DMA_REALIGN;                                           \
	} while (0)

#define ADD_DISCARDDESC(d, bsz)                                                                    \
	do {                                                                                       \
		d->addr = 0;                                                                       \
		d->sz = (uint32_t)(bsz) | DMA_DISCARD;                                             \
		d++;                                                                               \
	} while (0)

#define ADD_OUTDESCA(d, baddr, bsz, msk)                                                           \
	do {                                                                                       \
		uint32_t asz = ALIGN_SZA(bsz, msk);                                                \
		d->addr = (char *)(baddr);                                                         \
		d->sz = (uint32_t)bsz;                                                             \
		d++;                                                                               \
		if (asz - (bsz))                                                                   \
			ADD_DISCARDDESC(d, (asz - (bsz)));                                         \
	} while (0)

#define ADD_OUTDESC_PRIV(dmactl, d, offset, bsz, msk)                                              \
	do {                                                                                       \
		uint32_t asz = ALIGN_SZA(bsz, msk);                                                \
		d->addr = dmactl.mapped + offset;                                                  \
		d->sz = (uint32_t)(bsz);                                                           \
		d++;                                                                               \
		if (asz - (bsz))                                                                   \
			ADD_DISCARDDESC(d, (asz - (bsz)));                                         \
	} while (0)

#define ALIGN_SZ(sz)		       ALIGN_SZA(sz, 0xf)
#define ADD_INDESC(d, baddr, bsz, tag) ADD_INDESCA(d, baddr, bsz, tag, 0xf)
#define ADD_OUTDESC(d, baddr, bsz)     ADD_OUTDESCA(d, baddr, bsz, 0xf)

#define DMA_SZ_MASK (0xFFFFFF)

#define DESC_SZ(desc)	 (size_t)((desc)->sz & DMA_SZ_MASK)
#define CFGDESC_SZ(desc) DESC_SZ(desc)
#define INDESC_SZ(desc)	 (DESC_SZ(desc) - ((desc)->dmatag >> 8))
#define OUTDESC_SZ(desc) DESC_SZ(desc)

#define DMA_MAX_SZ (1u << 24)

struct sx_dmactl;

/** Prepare for a new command/operation over DMA */
void sx_cmdma_newcmd(struct sx_dmactl *dma, struct sxdesc *d, uint32_t cmd, uint32_t tag);

/** Start input/fetcher DMA at indescs and output/pusher DMA at outdescs */
void sx_cmdma_start(struct sx_dmactl *dma, size_t privsz, struct sxdesc *indescs);

/** Final preparation of array of descriptors before sx_cmdma_start()
 *
 * The array of descriptors begins at 'start' and ends at 'end'. Each
 * descriptor in the array will be linked to the next one in the array.
 * The last descriptor will have the 'last' dma tag and 'stop' instead
 * of a next descriptor.
 */
void sx_cmdma_finalize_descs(struct sxdesc *start, struct sxdesc *end);

/** Return how the DMA is doing.
 *
 * Possible return values are:
 *  - SX_ERR_HW_PROCESSING: The DMA transfers are going on.
 *  - SX_OK: The DMA transfers finished successfully.
 *  - SX_ERR_DMA_FAILED: The DMA engine failed.
 */
int sx_cmdma_check(void);

/* Struct describing the features of CRACEN. ba411, ba413 and ba419 are the HWCONFIG
 * from the PS.
 */
static const struct {
	uint32_t capabilities;
	uint32_t ba411;
	uint32_t ba413;
	uint32_t ba419;
} cracen_devices[] = {{0x771, 0x070301FF, 0x0003003F, 0x201ff}};

/** Soft-reset of the DMA
 */
void sx_cmdma_reset(void);

#endif /* CMDMA_HEADER_FILE */
