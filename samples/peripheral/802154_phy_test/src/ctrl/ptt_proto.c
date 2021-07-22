/* Copyright (c) 2019, Nordic Semiconductor ASA
 * All rights reserved.
 *
 * Use in source and binary forms, redistribution in binary form only, with
 * or without modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 2. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * 3. This software, with or without modification, must only be used with a Nordic
 *    Semiconductor ASA integrated circuit.
 *
 * 4. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/* Purpose: Protocol processing */

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "ctrl/ptt_trace.h"

#include "ptt_proto.h"

#include "ptt_types.h"

ptt_bool_t ptt_proto_check_packet(const uint8_t * p_pkt, ptt_pkt_len_t len)
{
    ptt_bool_t ret = false;

    if ((p_pkt != NULL) && (len > PTT_PREAMBLE_LEN))
    {
        if ((PTT_PREAMBLE_1ST == p_pkt[0])
            && (PTT_PREAMBLE_2ND == p_pkt[1])
            && (PTT_PREAMBLE_3D == p_pkt[2]))
        {
            ret = true;
        }
    }

    return ret;
}

ptt_pkt_len_t ptt_proto_construct_header(uint8_t * p_pkt, ptt_cmd_t cmd, ptt_pkt_len_t pkt_max_size)
{
    ptt_pkt_len_t len = 0;

    assert(pkt_max_size >= (PTT_PREAMBLE_LEN + PTT_CMD_CODE_LEN));

    p_pkt[len] = PTT_PREAMBLE_1ST;
    len++;
    p_pkt[len] = PTT_PREAMBLE_2ND;
    len++;
    p_pkt[len] = PTT_PREAMBLE_3D;
    len++;
    p_pkt[len] = cmd;
    len++;

    assert((PTT_PREAMBLE_LEN + PTT_CMD_CODE_LEN) == len);

    return len;
}

void ptt_htobe16(uint8_t * p_src, uint8_t * p_dst)
{
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    p_dst[0] = p_src[1];
    p_dst[1] = p_src[0];
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    p_dst[0] = p_src[0];
    p_dst[1] = p_src[1];
#else
#error "Unsupported endianness"
#endif
}

void ptt_htole16(uint8_t * p_src, uint8_t * p_dst)
{
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    p_dst[0] = p_src[0];
    p_dst[1] = p_src[1];
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    p_dst[0] = p_src[1];
    p_dst[1] = p_src[0];
#else
#error "Unsupported endianness"
#endif
}

void ptt_htobe32(uint8_t * p_src, uint8_t * p_dst)
{
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    p_dst[0] = p_src[3];
    p_dst[1] = p_src[2];
    p_dst[2] = p_src[1];
    p_dst[3] = p_src[0];
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    p_dst[0] = p_src[0];
    p_dst[1] = p_src[1];
    p_dst[2] = p_src[2];
    p_dst[3] = p_src[3];
#else
#error "Unsupported endianness"
#endif
}

void ptt_betoh16(uint8_t * p_src, uint8_t * p_dst)
{
    ptt_htobe16(p_src, p_dst);
}

void ptt_betoh32(uint8_t * p_src, uint8_t * p_dst)
{
    ptt_htobe32(p_src, p_dst);
}

uint16_t ptt_htobe16_val(uint8_t * p_src)
{
    uint16_t val = 0;

    ptt_htobe16(p_src, (uint8_t *)(&val));

    return val;
}

uint32_t ptt_htobe32_val(uint8_t * p_src)
{
    uint32_t val = 0;

    ptt_htobe32(p_src, (uint8_t *)(&val));

    return val;
}

uint16_t ptt_betoh16_val(uint8_t * p_src)
{
    uint16_t val = 0;

    ptt_betoh16(p_src, (uint8_t *)(&val));

    return val;
}

uint32_t ptt_betoh32_val(uint8_t * p_src)
{
    uint32_t val = 0;

    ptt_betoh32(p_src, (uint8_t *)(&val));

    return val;
}