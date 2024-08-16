// SPDX-License-Identifier: 0BSD

///////////////////////////////////////////////////////////////////////////////
//
/// \file       armthumb.c
/// \brief      Filter for ARM-Thumb binaries
///
//  Authors:    Igor Pavlov
//              Lasse Collin
//
///////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include "armthumb.h"

void arm_thumb_filter(uint8_t *buf, uint32_t buf_size, uint32_t pos, bool compress)
{
	uint32_t i = 0;

	while ((i + 4) <= buf_size) {
		if ((buf[i + 1] & 0xF8) == 0xF0 && (buf[i + 3] & 0xF8) == 0xF8) {
			uint32_t dest;
			uint32_t src = (((uint32_t)(buf[i + 1]) & 7) << 19)
			| ((uint32_t)(buf[i + 0]) << 11)
			| (((uint32_t)(buf[i + 3]) & 7) << 8)
			| (uint32_t)(buf[i + 2]);

			src <<= 1;

			if (compress) {
				dest = pos + (uint32_t)(i) + 4 + src;
			} else {
				dest = src - (pos + (uint32_t)(i) + 4);
			}

			dest >>= 1;
			buf[i + 1] = 0xF0 | ((dest >> 19) & 0x7);
			buf[i + 0] = (dest >> 11);
			buf[i + 3] = 0xF8 | ((dest >> 8) & 0x7);
			buf[i + 2] = (dest);
			i += 2;
		}

		i += 2;
	}
}
