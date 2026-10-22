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
* :kconfig:option:`CONFIG_MQTT_HELPER_PROVISION_CERTIFICATES`, :kconfig:option:`CONFIG_MQTT_HELPER_CREDENTIALS_KEYGEN`, or :kconfig:option:`CONFIG_MQTT_HELPER_CREDENTIALS_NONE` - Select one of them, as they exclude each other.
  The :kconfig:option:`CONFIG_MQTT_HELPER_PROVISION_CERTIFICATES` Kconfig option is the default.
  Select the :kconfig:option:`CONFIG_MQTT_HELPER_CREDENTIALS_NONE` Kconfig option if your application installs the TLS credentials itself.
* :kconfig:option:`CONFIG_MQTT_HELPER_CERTIFICATES_FOLDER`

.. _mqtt_helper_credentials_keygen:

On-device private key
=====================

When the :kconfig:option:`CONFIG_MQTT_HELPER_CREDENTIALS_KEYGEN` Kconfig option is enabled, the library connects with a device private key that was generated on the device.
You generate the key with the ``cred keygen`` command of the TLS credentials shell.
You can also generate it with the TLS credentials key generation API of Zephyr (:kconfig:option:`CONFIG_TLS_CREDENTIALS_KEYGEN`).
The device generates the key pair with the PSA Crypto API and stores it in secure storage as a non-exportable key, so the private key never leaves the device.
The key is registered as a TLS credential of its security tag when it is generated.
The TLS credentials protected storage backend keeps the registration after a reboot.
The security tag must be the one that the library uses for the connection.
See :kconfig:option:`CONFIG_MQTT_HELPER_SEC_TAG`.

To use the device-generated private key, set the following Kconfig options:

* :kconfig:option:`CONFIG_MQTT_HELPER_CREDENTIALS_KEYGEN` - Enables the generation of the private key on the device.
  This option enables the :kconfig:option:`CONFIG_TLS_CREDENTIALS_KEYGEN` Kconfig option.
  It also enables the :kconfig:option:`CONFIG_TLS_CREDENTIALS_SHELL` Kconfig option and PEM parsing by default.
* :kconfig:option:`CONFIG_TLS_CREDENTIALS_BACKEND_PROTECTED_STORAGE` - Selects the TLS credentials protected storage backend, which is required by the device-generated private key.
* :kconfig:option:`CONFIG_TLS_CREDENTIALS_SHELL_CRED_BUF_SIZE` - Set to a value that fits the certificate chain that you install, for example ``4096`` for AWS IoT and Azure IoT Hub.

To generate a new key pair, first delete the existing one with the ``cred keydel <sec tag>`` command of the shell.

You can also use the `nrfcredstore`_ tool for these commands.

.. note::
   The ``nrfcredstore`` tool supports the TLS credentials shell from version 3.0.0, with the ``--cmd-type tls-shell`` option.

API documentation
*****************

| Header file: :ncs-file:`/include/net/mqtt_helper.h`
| Source file: :ncs-file:`/subsys/net/lib/mqtt_helper/mqtt_helper.c`

.. doxygengroup:: mqtt_helper
