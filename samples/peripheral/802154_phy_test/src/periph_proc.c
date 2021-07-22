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

/* Purpose: Periphery configuring and processing */

#include <assert.h>
#include <stdint.h>

#include <random/rand32.h>

#include "periph_proc.h"

#include <nrfx_timer.h>
#include <helpers/nrfx_gppi.h>
#include <nrfx.h>
#include <nrfx_nvmc.h>
#include <nrfx_power.h>
#include <hal/nrf_gpio.h>
#include <nrfx_gpiote.h>
#include <platform/nrf_802154_temperature.h>

#include <devicetree.h>
#include <drivers/gpio.h>
#include <drivers/clock_control.h>
#include <drivers/clock_control/nrf_clock_control.h>

#if defined(DPPI_PRESENT)
#include <nrfx_dppi.h>
#else
#include <nrfx_ppi.h>
#endif

#include <logging/log.h>
LOG_MODULE_REGISTER(periph);

/* Timer instance used for HFCLK output */
#define PTT_CLK_TIMER 2

static struct onoff_client hfclk_cli;
static nrfx_timer_t        m_clk_timer = NRFX_TIMER_INSTANCE(PTT_CLK_TIMER);

/* The devicetree node identifier for the "led0" alias. */
#define LED0_NODE DT_ALIAS(led0)

#if DT_NODE_HAS_STATUS(LED0_NODE, okay)
#define LED0      DT_GPIO_LABEL(LED0_NODE, gpios)
#define PIN       DT_GPIO_PIN(LED0_NODE, gpios)
#define FLAGS     DT_GPIO_FLAGS(LED0_NODE, gpios)
#else
/* A build error here means your board isn't set up to blink an LED. */
#error "Unsupported board: led0 devicetree alias is not defined"
#define LED0       ""
#define PIN        0
#define FLAGS      0
#endif

#define CLOCK_NODE DT_INST(0, nordic_nrf_clock)

static const struct device * indication_led_dev;
static const struct device * gpio_port0_dev;

/* Check if the system has GPIO port 1 */
#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpio1), okay)
static const struct device * gpio_port1_dev;

#endif

#if defined(DPPI_PRESENT)
uint8_t ppi_channel;

#else
nrf_ppi_channel_t ppi_channel;

#endif

static void hfclk_on_callback(struct onoff_manager * mgr,
                              struct onoff_client  * cli,
                              uint32_t               state,
                              int                    res)
{
    /* do nothing */
}

static void clk_timer_handler(nrf_timer_event_t event_type, void * p_context)
{
    /* do nothing */
}

void periph_init(void)
{
    nrfx_err_t err_code;
    int        ret;

    /* Enable HFCLK */
    struct onoff_manager * mgr =
        z_nrf_clock_control_get_onoff(CLOCK_CONTROL_NRF_SUBSYS_HF);

    __ASSERT_NO_MSG(mgr != NULL);

    sys_notify_init_callback(&hfclk_cli.notify, hfclk_on_callback);

    ret = onoff_request(mgr, &hfclk_cli);
    __ASSERT_NO_MSG(ret >= 0);

    nrfx_timer_config_t clk_timer_cfg = NRFX_TIMER_DEFAULT_CONFIG;

    err_code = nrfx_timer_init(&m_clk_timer, &clk_timer_cfg, clk_timer_handler);
    NRFX_ASSERT(err_code);

    err_code = nrfx_gpiote_init(0);
    NRFX_ASSERT(err_code);

    indication_led_dev = device_get_binding(LED0);
    assert(indication_led_dev);

    ret = gpio_pin_configure(indication_led_dev, PIN, GPIO_OUTPUT_ACTIVE | FLAGS);

    assert(ret == 0);
    gpio_pin_set(indication_led_dev, PIN, false);

    gpio_port0_dev = device_get_binding(DT_LABEL(DT_NODELABEL(gpio0)));
    assert(gpio_port0_dev);

#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpio1), okay)
    gpio_port1_dev = device_get_binding(DT_LABEL(DT_NODELABEL(gpio1)));
    assert(gpio_port1_dev);
#endif
}

uint32_t ptt_random_get_ext(void)
{
    return sys_rand32_get();
}

bool ptt_clk_out_ext(uint8_t pin, bool mode)
{
    uint32_t   compare_evt_addr;
    nrfx_err_t err;

    if (!nrf_gpio_pin_present_check(pin))
    {
        return false;
    }

    if (mode)
    {
        nrfx_timer_extended_compare(&m_clk_timer,
                                    (nrf_timer_cc_channel_t)0,
                                    1,
                                    NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK,
                                    false);

        nrfx_gpiote_out_config_t const out_config = {
            .action     = NRF_GPIOTE_POLARITY_TOGGLE,
            .init_state = 0,
            .task_pin   = true,
        };

        /* Initialize output pin. SET task will turn the LED on,
         * CLR will turn it off and OUT will toggle it.
         */
        err = nrfx_gpiote_out_init(pin, &out_config);
        if (err != NRFX_SUCCESS)
        {
            LOG_ERR("nrfx_gpiote_out_init error: %08x", err);
            return false;
        }

        compare_evt_addr = nrfx_timer_event_address_get(&m_clk_timer, NRF_TIMER_EVENT_COMPARE0);

        nrfx_gpiote_out_task_enable(pin);

        /* Allocate a (D)PPI channel. */
#if defined(DPPI_PRESENT)
        err = nrfx_dppi_channel_alloc(&ppi_channel);
#else
        err = nrfx_ppi_channel_alloc(&ppi_channel);
#endif
        if (err != NRFX_SUCCESS)
        {
            LOG_ERR("(D)PPI channel allocation error: %08x", err);
            return false;
        }

        nrfx_gppi_channel_endpoints_setup(ppi_channel, compare_evt_addr,
                                          nrf_gpiote_task_address_get(NRF_GPIOTE,
                                                                      nrfx_gpiote_in_event_get(pin)));

        /* Enable (D)PPI channel. */
#if defined(DPPI_PRESENT)
        err = nrfx_dppi_channel_enable(ppi_channel);
#else
        err = nrfx_ppi_channel_enable(ppi_channel);
#endif
        if (err != NRFX_SUCCESS)
        {
            LOG_ERR("Failed to enable (D)PPI channel, error: %08x", err);
            return false;
        }

        nrfx_timer_enable(&m_clk_timer);
    }
    else
    {
        nrfx_timer_disable(&m_clk_timer);
        nrfx_gpiote_out_task_disable(pin);
#if defined(DPPI_PRESENT)
        err = nrfx_dppi_channel_free(ppi_channel);
#else
        err = nrfx_ppi_channel_free(ppi_channel);
#endif
        if (err != NRFX_SUCCESS)
        {
            LOG_ERR("Failed to disable (D)PPI channel, error: %08x", err);
            return false;
        }
        nrfx_gpiote_out_uninit(pin);
    }

    return true;
}

static const struct device * get_pin_port(uint32_t * pin)
{
    switch (nrf_gpio_pin_port_number_extract(pin))
    {
        case 0:
            return gpio_port0_dev;

#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpio1), okay)
        case 1:
            return gpio_port1_dev;

#endif
        default:
            return NULL;
    }
}

bool ptt_set_gpio_ext(uint8_t pin, uint8_t value)
{
    int                   ret;
    const struct device * port;
    uint32_t              pin_nr = pin;

    if (nrf_gpio_pin_present_check(pin_nr))
    {
        port = get_pin_port(&pin_nr);
        if (port == NULL)
        {
            return false;
        }

        ret = gpio_pin_configure(port, pin_nr, GPIO_OUTPUT);

        if (ret == 0)
        {
            ret = gpio_pin_set(port, pin_nr, value);
            if (ret == 0)
            {
                return true;
            }
        }
    }

    return false;
}

bool ptt_get_gpio_ext(uint8_t pin, uint8_t * value)
{
    int                   ret;
    const struct device * port;
    uint32_t              pin_nr = pin;

    if (nrf_gpio_pin_present_check(pin_nr))
    {
        port = get_pin_port(&pin_nr);
        if (port == NULL)
        {
            return false;
        }

        ret = gpio_pin_configure(port, pin_nr, GPIO_INPUT);

        if (ret == 0)
        {
            *value = gpio_pin_get(port, pin_nr);
            return true;
        }
    }

    return false;
}

void ptt_set_dcdc_ext(bool enable)
{
#if NRF_POWER_HAS_DCDCEN
    nrf_power_dcdcen_set(NRF_POWER, enable);
#endif

#if NRF_POWER_HAS_DCDCEN_VDDH
    nrf_power_dcdcen_vddh_set(NRF_POWER, enable);
#endif

#if !NRF_POWER_HAS_DCDCEN && !NRF_POWER_HAS_DCDCEN_VDDH
#warning "DC-DC related commands have no effect!"
#endif
}

bool ptt_get_dcdc_ext(void)
{
#if NRF_POWER_HAS_DCDCEN && NRF_POWER_HAS_DCDCEN_VDDH
    return (nrf_power_dcdcen_get(NRF_POWER) || nrf_power_dcdcen_vddh_get(NRF_POWER));
#elif NRF_POWER_HAS_DCDCEN
    return nrf_power_dcdcen_get(NRF_POWER);
#elif NRF_POWER_HAS_DCDCEN_VDDH
    return nrf_power_dcdcen_vddh_get(NRF_POWER);
#else
    return false;
#endif
}

void ptt_set_icache_ext(bool enable)
{
#if defined(NVMC_FEATURE_CACHE_PRESENT)
    if (enable)
    {
        nrfx_nvmc_icache_enable();
    }
    else
    {
        nrfx_nvmc_icache_disable();
    }
#else
#warning "ICACHE related command have no effect"
#endif
}

bool ptt_get_icache_ext(void)
{
#if defined(NVMC_FEATURE_CACHE_PRESENT)
    return nrf_nvmc_icache_enable_check(NRF_NVMC);
#else
    return false;
#endif
}

bool ptt_get_temp_ext(int32_t * p_temp)
{
    if (NULL == p_temp)
    {
        return false;
    }

    *p_temp = nrf_802154_temperature_get();

    return true;
}

void ptt_ctrl_led_indication_on_ext(void)
{
    gpio_pin_set(indication_led_dev, PIN, true);
}

void ptt_ctrl_led_indication_off_ext(void)
{
    gpio_pin_set(indication_led_dev, PIN, false);
}