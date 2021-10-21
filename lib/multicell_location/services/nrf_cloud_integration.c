/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <modem/modem_jwt.h>
#include <net/nrf_cloud_rest.h>
#include "location_service.h"

#include <logging/log.h>

LOG_MODULE_REGISTER(multicell_location_nrf_cloud, CONFIG_MULTICELL_LOCATION_LOG_LEVEL);

#define NRF_CLOUD_INTEGRATION_JWT_VALID_TIME (5 * 60)

static char jwt_buf[600];

/* TLS certificate:
 *	CN=Starfield Services Root Certificate Authority - G2
 *	O=Starfield Technologies, Inc.
 *	OU=Starfield Class 2 Certification Authority
 *	Serial number=a7:0e:4a:4c:34:82:b7:7f
 *	Valid from=Sep 2 00:00:00 2009 GMT
 *	Valid to=Jun 28 17:39:16 2034 GMT
 *	Download url=https://www.amazontrust.com/repository/SFSRootCAG2.pem
 */
static const char tls_certificate[] =
	"-----BEGIN CERTIFICATE-----\n"
	"MIIEdTCCA12gAwIBAgIJAKcOSkw0grd/MA0GCSqGSIb3DQEBCwUAMGgxCzAJBgNV\n"
	"BAYTAlVTMSUwIwYDVQQKExxTdGFyZmllbGQgVGVjaG5vbG9naWVzLCBJbmMuMTIw\n"
	"MAYDVQQLEylTdGFyZmllbGQgQ2xhc3MgMiBDZXJ0aWZpY2F0aW9uIEF1dGhvcml0\n"
	"eTAeFw0wOTA5MDIwMDAwMDBaFw0zNDA2MjgxNzM5MTZaMIGYMQswCQYDVQQGEwJV\n"
	"UzEQMA4GA1UECBMHQXJpem9uYTETMBEGA1UEBxMKU2NvdHRzZGFsZTElMCMGA1UE\n"
	"ChMcU3RhcmZpZWxkIFRlY2hub2xvZ2llcywgSW5jLjE7MDkGA1UEAxMyU3RhcmZp\n"
	"ZWxkIFNlcnZpY2VzIFJvb3QgQ2VydGlmaWNhdGUgQXV0aG9yaXR5IC0gRzIwggEi\n"
	"MA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQDVDDrEKvlO4vW+GZdfjohTsR8/\n"
	"y8+fIBNtKTrID30892t2OGPZNmCom15cAICyL1l/9of5JUOG52kbUpqQ4XHj2C0N\n"
	"Tm/2yEnZtvMaVq4rtnQU68/7JuMauh2WLmo7WJSJR1b/JaCTcFOD2oR0FMNnngRo\n"
	"Ot+OQFodSk7PQ5E751bWAHDLUu57fa4657wx+UX2wmDPE1kCK4DMNEffud6QZW0C\n"
	"zyyRpqbn3oUYSXxmTqM6bam17jQuug0DuDPfR+uxa40l2ZvOgdFFRjKWcIfeAg5J\n"
	"Q4W2bHO7ZOphQazJ1FTfhy/HIrImzJ9ZVGif/L4qL8RVHHVAYBeFAlU5i38FAgMB\n"
	"AAGjgfAwge0wDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMCAYYwHQYDVR0O\n"
	"BBYEFJxfAN+qAdcwKziIorhtSpzyEZGDMB8GA1UdIwQYMBaAFL9ft9HO3R+G9FtV\n"
	"rNzXEMIOqYjnME8GCCsGAQUFBwEBBEMwQTAcBggrBgEFBQcwAYYQaHR0cDovL28u\n"
	"c3MyLnVzLzAhBggrBgEFBQcwAoYVaHR0cDovL3guc3MyLnVzL3guY2VyMCYGA1Ud\n"
	"HwQfMB0wG6AZoBeGFWh0dHA6Ly9zLnNzMi51cy9yLmNybDARBgNVHSAECjAIMAYG\n"
	"BFUdIAAwDQYJKoZIhvcNAQELBQADggEBACMd44pXyn3pF3lM8R5V/cxTbj5HD9/G\n"
	"VfKyBDbtgB9TxF00KGu+x1X8Z+rLP3+QsjPNG1gQggL4+C/1E2DUBc7xgQjB3ad1\n"
	"l08YuW3e95ORCLp+QCztweq7dp4zBncdDQh/U90bZKuCJ/Fp1U1ervShw3WnWEQt\n"
	"8jxwmKy6abaVd38PMV4s/KCHOkdp8Hlf9BRUpJVeEXgSYCfOn8J3/yNTd126/+pZ\n"
	"59vPr5KW7ySaNRB6nJHGDn2Z9j8Z3/VyVOEVqQdZe4O/Ui5GjLIAZHYcSNPYeehu\n"
	"VsyuLAOQ1xk4meTKCRlb/weWsKh/NEnfVqn3sF/tM+2MR7cwA130A4w=\n"
	"-----END CERTIFICATE-----\n";

const char *location_service_get_certificate(void)
{
	return tls_certificate;
}

int location_service_get_cell_location(const struct lte_lc_cells_info *cell_data,
				       char * const rcv_buf, const size_t rcv_buf_len,
				       struct multicell_location *const location)
{
	int err;
	struct nrf_cloud_cell_pos_result result;
	struct nrf_cloud_rest_context rest_ctx = {
		.auth = jwt_buf,
		.connect_socket = -1,
		.keep_alive = false,
		.timeout_ms = CONFIG_NRF_CLOUD_REST_RECV_TIMEOUT * MSEC_PER_SEC,
		.rx_buf = rcv_buf,
		.rx_buf_len = rcv_buf_len,
		.fragment_size = 0
	};
	const struct nrf_cloud_rest_cell_pos_request loc_req = {
		.net_info = (struct lte_lc_cells_info *)cell_data
	};

	err = nrf_cloud_jwt_generate(
		NRF_CLOUD_INTEGRATION_JWT_VALID_TIME,
		jwt_buf,
		sizeof(jwt_buf));
	if (err) {
		LOG_ERR("Failed to generate JWT, error: %d", err);
		return err;
	}

	err = nrf_cloud_rest_cell_pos_get(&rest_ctx, &loc_req, &result);
	if (!err) {
		location->accuracy = (double)result.unc;
		location->latitude = result.lat;
		location->longitude = result.lon;
	}

	return err;
}
