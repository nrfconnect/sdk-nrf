/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

static const unsigned char ca_certificate[] = {
#if defined(MQTT_HELPER_CA_CERT)
#include MQTT_HELPER_CA_CERT

/* Null terminate certificate */
(0x00)
#else
""
#endif
};

static const unsigned char private_key[] = {
#if defined(MQTT_HELPER_PRIVATE_KEY)
#include MQTT_HELPER_PRIVATE_KEY
(0x00)
#else
""
#endif
};

static const unsigned char device_certificate[] = {
#if defined(MQTT_HELPER_CLIENT_CERT)
#include MQTT_HELPER_CLIENT_CERT
(0x00)
#else
""
#endif
};

#if CONFIG_MQTT_HELPER_SECONDARY_SEC_TAG != -1

static const unsigned char ca_certificate_2[] = {
#if defined(MQTT_HELPER_CA_CERT_2)
#include MQTT_HELPER_CA_CERT_2
(0x00)
#else
""
#endif
};

static const unsigned char private_key_2[] = {
#if defined(MQTT_HELPER_PRIVATE_KEY_2)
#include MQTT_HELPER_PRIVATE_KEY_2
(0x00)
#else
""
#endif
};

static const unsigned char device_certificate_2[] = {
#if defined(MQTT_HELPER_CLIENT_CERT_2)
#include MQTT_HELPER_CLIENT_CERT_2
(0x00)
#else
""
#endif
};

#endif /* CONFIG_MQTT_HELPER_SECONDARY_SEC_TAG != -1 */
