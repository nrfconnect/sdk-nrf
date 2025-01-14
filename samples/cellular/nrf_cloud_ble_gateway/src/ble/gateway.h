/* Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef GATEWAY_CLOUD_TRANSPORT__
#define GATEWAY_CLOUD_TRANSPORT__

#include <net/nrf_cloud_codec.h>
#define NRF_CLOUD_CLIENT_ID_LEN  128
extern char gateway_id[NRF_CLOUD_CLIENT_ID_LEN+1];

struct cloud_msg;

void device_shutdown(bool reboot);
void control_cloud_connection(bool enable);
void cli_init(void);
void set_log_panic(void);
void init_gateway(void);
void reset_gateway(void);
int g2c_send(const struct nrf_cloud_data *output);
int gw_shadow_publish(const struct nrf_cloud_data *output);
int gateway_shadow_delta_handler(const struct nrf_cloud_obj_shadow_delta *delta);
int gateway_shadow_accepted_handler(const struct nrf_cloud_obj *desired);
int gateway_shadow_transform_handler(const struct nrf_cloud_obj *desired_conns);
int gateway_data_handler(const struct nrf_cloud_data *const dev_msg);

#if defined(CONFIG_ENTER_52840_MCUBOOT_VIA_BUTTON)
/* functions defined in the board's .c file */
int nrf52840_reset_to_mcuboot(void);
int nrf52840_wait_boot_complete(int timeout_ms);
#endif

#endif
