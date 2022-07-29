/*
 * Author: tol2@nordicsemi.no
 * Date: 07/28/2022
 * NRFCloud rest message sample app replicate
 */

#include <zephyr/kernel.h>
#include <modem/nrf_modem_lib.h>
#include <nrf_modem_at.h>
#include <modem/modem_info.h>
#include <zephyr/settings/settings.h>
#include <net/nrf_cloud.h>
#include <net/nrf_cloud_rest.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/logging/log.h>
#include <dk_buttons_and_leds.h>

