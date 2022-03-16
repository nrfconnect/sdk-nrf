/*$$$LICENCE_NORDIC_STANDARD<2018>$$$*/

#include <zboss_api.h>
#include <zigbee/zigbee_error_handler.h>
#include "timer_api.h"

#define UNUSED_RETURN_VALUE(val) (void)val
#define UNUSED_PARAMETER(val) (void)val


static timer_cb_t m_timer_cb;
static uint32_t   m_timeout_ms;
static uint32_t   m_timeout_beacon_int;
static uint32_t   m_delta_beacon_int;
static uint32_t   m_stop_beacon_int;
static zb_bool_t  m_enabled = ZB_FALSE;

/**
 * @brief A callback called on timer timeout.
 *
 * @param[in] bufid a reference number to ZBOSS memory buffer (unused).
 */
static void timer_timeout_zboss_cb(zb_bufid_t bufid)
{
    (void)bufid;

    if ((m_enabled == ZB_TRUE) && (m_timer_cb))
    {
        m_timer_cb();
    }
}

uint32_t timer_capture(void)
{
    if (m_enabled)
    {
        return (ZB_TIMER_GET() - m_delta_beacon_int);
    }
    else
    {
        return (m_stop_beacon_int - m_delta_beacon_int);
    }
}

void timer_init(void)
{
    m_timer_cb           = NULL;
    m_timeout_ms         = 0;
    m_delta_beacon_int   = 0;
    m_timeout_beacon_int = 0;
    m_enabled            = ZB_FALSE;
    m_stop_beacon_int    = ZB_TIMER_GET();
}

void timer_set(uint32_t timeout_ms, timer_cb_t p_callback)
{
    zb_ret_t zb_err_code;

    m_timer_cb   = p_callback;
    m_timeout_ms = timeout_ms;

    if (m_enabled == ZB_TRUE)
    {
        m_timeout_beacon_int = ZB_TIMER_GET() + ZB_MILLISECONDS_TO_BEACON_INTERVAL(m_timeout_ms);
        zb_err_code = ZB_SCHEDULE_APP_ALARM(timer_timeout_zboss_cb, 0, ZB_MILLISECONDS_TO_BEACON_INTERVAL(m_timeout_ms));
        // ZB_ERROR_CHECK(zb_err_code);
        m_timeout_ms = 0;
    }
}

void timer_start(void)
{
    zb_ret_t zb_err_code;

    if (m_enabled == ZB_FALSE)
    {
        m_delta_beacon_int += ZB_TIMER_GET() - m_stop_beacon_int;
        m_enabled          = ZB_TRUE;

        if (m_timeout_ms != 0)
        {
            m_timeout_beacon_int = ZB_TIMER_GET() + ZB_MILLISECONDS_TO_BEACON_INTERVAL(m_timeout_ms);
            zb_err_code = ZB_SCHEDULE_APP_ALARM(timer_timeout_zboss_cb, 0, ZB_MILLISECONDS_TO_BEACON_INTERVAL(m_timeout_ms));
            // ZB_ERROR_CHECK(zb_err_code);
            m_timeout_ms = 0;
        }
    }
}

void timer_stop(void)
{
    if (m_enabled == ZB_TRUE)
    {
        m_stop_beacon_int = ZB_TIMER_GET();
        m_enabled = ZB_FALSE;
        if (m_timeout_beacon_int > ZB_TIMER_GET())
        {
            m_timeout_ms = ZB_TIME_BEACON_INTERVAL_TO_MSEC(m_timeout_beacon_int - ZB_TIMER_GET());
        }
        else
        {
            m_timeout_ms = 0;
        }
        UNUSED_RETURN_VALUE(ZB_SCHEDULE_APP_ALARM_CANCEL(timer_timeout_zboss_cb, ZB_ALARM_ANY_PARAM));
    }
}

uint32_t timer_ticks_to_ms(uint32_t ticks)
{
    return ZB_TIME_BEACON_INTERVAL_TO_MSEC(ticks);
}


uint32_t timer_ticks_from_uptime(void)
{
    return (uint32_t)k_uptime_get();
}