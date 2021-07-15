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

/* Purpose: Production configuration routines */

#include <assert.h>
#include <stdint.h>
#include <stdbool.h>

bool ptt_get_mode_mask_ext(uint32_t * mode_mask)
{
    assert(mode_mask != NULL);

#ifdef CONFIG_DUT_MODE
    *mode_mask = 0x00000001;
#elif CONFIG_CMD_MODE
    *mode_mask = 0x00000002;
#else
    *mode_mask = 0x00000003;
#endif
    return true;
}

bool ptt_get_channel_mask_ext(uint32_t * channel_mask)
{
    assert(channel_mask != NULL);

    *channel_mask = CONFIG_CHANNEL_MASK;
    return true;

}

bool ptt_get_power_ext(int8_t * power)
{
    assert(power != NULL);

    *power = CONFIG_POWER;
    return true;
}

bool ptt_get_antenna_ext(uint8_t * antenna)
{
    assert(antenna != NULL);

#if CONFIG_ANTENNA_DIVERSITY
    *antenna = CONFIG_ANTENNA_NUMBER;
    return true;
#else
    return false;
#endif
}

bool ptt_get_sw_version_ext(uint8_t * sw_version)
{
    assert(sw_version != NULL);

    *sw_version = CONFIG_SW_VERSION;
    return true;
}

bool ptt_get_hw_version_ext(uint8_t * hw_version)
{
    assert(hw_version != NULL);

    *hw_version = CONFIG_HW_VERSION;
    return true;
}

bool ptt_get_ant_mode_ext(uint8_t * ant_mode)
{
    assert(ant_mode != NULL);

#if CONFIG_ANTENNA_DIVERSITY
#if CONFIG_ANT_MODE_AUTO
    /* TODO: Implement when antenna diversity is supported in NCS */
#elif CONFIG_ANT_MODE_MANUAL
    /* TODO: Implement when antenna diversity is supported in NCS */
#else
    /* TODO: Implement when antenna diversity is supported in NCS */
#endif
    return true;
#else
    return false;
#endif
}

bool ptt_get_ant_num_ext(uint8_t * ant_num)
{
    assert(ant_num != NULL);

#if CONFIG_ANTENNA_DIVERSITY
    *ant_num = CONFIG_ANTENNA_NUMBER;
    return true;
#else
    return false;
#endif
}


