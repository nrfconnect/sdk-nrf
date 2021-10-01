/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#if defined(CONFIG_CLOUD_CODEC_NRF_CLOUD)
#include "nrf_cloud/json_protocol_names_nrf_cloud.h"
#elif defined(CONFIG_CLOUD_CODEC_AWS_IOT)
#include "aws_iot/json_protocol_names_aws_iot.h"
#elif defined(CONFIG_CLOUD_CODEC_AZURE_IOT_HUB)
#include "azure_iot_hub/json_protocol_names_azure_iot_hub.h"
#endif
