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
* :kconfig:option:`CONFIG_MQTT_HELPER_PROVISION_CERTIFICATES`
* :kconfig:option:`CONFIG_MQTT_HELPER_CERTIFICATES_FOLDER`

API documentation
*****************

| Header file: :file:`include/net/mqtt_helper.h`
| Source file: :file:`subsys/net/lib/mqtt_helper/mqtt_helper.c`

.. doxygengroup:: mqtt_helper
   :project: nrf
   :members:
   :inner:
