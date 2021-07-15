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
#include <init.h>
#include <zephyr.h>

#include "rf_proc.h"
#include "timer_proc.h"
#include "comm_proc.h"
#include "periph_proc.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(phy_tt);

/* size of stack area used by each thread */
#define RF_THREAD_STACKSIZE     (1024u)
#define COMM_THREAD_STACKSIZE   (1024u)

/* scheduling priority used by each thread */
#define RF_THREAD_PRIORITY      (7u)
#define COMM_THREAD_PRIORITY    (7u)

void ptt_do_reset_ext(void)
{
    NVIC_SystemReset();
}

static int setup(const struct device *dev)
{
    LOG_INF("Setup started");

    ARG_UNUSED(dev);

    periph_init();

    rf_init();

    comm_init();

    /* initialize ptt library */
    ptt_init(launch_ptt_process_timer, ptt_get_max_time());

    return 0;
}

SYS_INIT(setup, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

K_THREAD_DEFINE(rf_id, RF_THREAD_STACKSIZE, rf_thread, NULL, NULL, NULL,
                RF_THREAD_PRIORITY, 0, 0);

K_THREAD_DEFINE(comm_id, COMM_THREAD_STACKSIZE, comm_proc, NULL, NULL, NULL,
                COMM_THREAD_PRIORITY, 0, 0);
