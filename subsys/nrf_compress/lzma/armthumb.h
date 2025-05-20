// SPDX-License-Identifier: 0BSD

///////////////////////////////////////////////////////////////////////////////
//
/// \file       armthumb.h
/// \brief      Filter for ARM-Thumb binaries
///
//  Authors:    Igor Pavlov
//              Lasse Collin
//
///////////////////////////////////////////////////////////////////////////////

#ifndef LZMA_ARMTHUMB_H
#define LZMA_ARMTHUMB_H

#include <stdint.h>
#include <stdbool.h>

void arm_thumb_filter(uint8_t *buf, uint32_t buf_size, uint32_t pos, bool compress,
		      bool *end_part_match);

#endif
