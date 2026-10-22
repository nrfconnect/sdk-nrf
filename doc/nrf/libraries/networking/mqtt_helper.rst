.. _lib_mqtt_helper:

MQTT helper
###########

.. contents::
   :local:
   :depth: 2

This library simplifies Zephyr MQTT API and socket handling.

Configuration
*************

To use the MQTT helper library, enable the :kconfig:option:`CONFIG_MQTT_HELPER` Kconfig option.

Additionally, configure the following options as per the needs of your application:

* :kconfig:option:`CONFIG_MQTT_HELPER_NATIVE_TLS`
* :kconfig:option:`CONFIG_MQTT_HELPER_PORT`
* :kconfig:option:`CONFIG_MQTT_HELPER_SEC_TAG`
* :kconfig:option:`CONFIG_MQTT_HELPER_SEND_TIMEOUT`
* :kconfig:option:`CONFIG_MQTT_HELPER_SEND_TIMEOUT_SEC`
* :kconfig:option:`CONFIG_MQTT_HELPER_STATIC_IP_ADDRESS`
* :kconfig:option:`CONFIG_MQTT_HELPER_SECONDARY_SEC_TAG`
* :kconfig:option:`CONFIG_MQTT_HELPER_STACK_SIZE`
* :kconfig:option:`CONFIG_MQTT_HELPER_RX_TX_BUFFER_SIZE`
* :kconfig:option:`CONFIG_MQTT_HELPER_PAYLOAD_BUFFER_LEN`
* :kconfig:option:`CONFIG_MQTT_HELPER_PROVISION_CERTIFICATES` or :kconfig:option:`CONFIG_MQTT_HELPER_CREDENTIALS_KEYGEN` - Select one of them, as they exclude each other.
* :kconfig:option:`CONFIG_MQTT_HELPER_CERTIFICATES_FOLDER`
* :kconfig:option:`CONFIG_TLS_CREDENTIALS_SHELL_CRED_BUF_SIZE` - Set it to fit the certificate chain when you use :kconfig:option:`CONFIG_MQTT_HELPER_CREDENTIALS_KEYGEN`.

On-device private key
=====================

When the :kconfig:option:`CONFIG_MQTT_HELPER_CREDENTIALS_KEYGEN` Kconfig option is enabled, the library connects with a device private key that was generated on the device.
The key is generated with the ``cred keygen`` command of the TLS credentials shell or with the TLS credentials key generation API of Zephyr (:kconfig:option:`CONFIG_TLS_CREDENTIALS_KEYGEN`), and it never leaves the device.
The key is registered as a TLS credential of its security tag when it is generated, and the TLS credentials protected storage backend keeps the registration after a reboot.
The security tag must be the one that the library uses for the connection, see :kconfig:option:`CONFIG_MQTT_HELPER_SEC_TAG`.
The option requires the TLS credentials protected storage backend (:kconfig:option:`CONFIG_TLS_CREDENTIALS_BACKEND_PROTECTED_STORAGE`).
It enables the TLS credentials key generation, the TLS credentials shell, and PEM parsing.
The certificates are then installed with the shell, for example with the ``nrfcredstore`` tool.
See :ref:`lib_aws_iot` and :ref:`lib_azure_iot_hub` for an example of how to generate the key and obtain the device certificate.

API documentation
*****************

| Header file: :file:`include/net/mqtt_helper.h`
| Source file: :file:`subsys/net/lib/mqtt_helper/mqtt_helper.c`

.. doxygengroup:: mqtt_helper
