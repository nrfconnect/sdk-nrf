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

* :kconfig:option:`CONFIG_MQTT_HELPER_NATIVE_TLS` - Configures the socket to be native for TLS instead of offloading TLS operations to the modem.
* :kconfig:option:`CONFIG_MQTT_HELPER_PORT` - Sets the TCP port number to connect to.
* :kconfig:option:`CONFIG_MQTT_HELPER_SEC_TAG` - Sets the security tag where the certificates are stored.
* :kconfig:option:`CONFIG_MQTT_HELPER_SEND_TIMEOUT` - Enables timeout when sending data.
* :kconfig:option:`CONFIG_MQTT_HELPER_SEND_TIMEOUT_SEC` - Sets the send timeout value (in seconds) to use when sending data.
* :kconfig:option:`CONFIG_MQTT_HELPER_STATIC_IP_ADDRESS`
* :kconfig:option:`CONFIG_MQTT_HELPER_SECONDARY_SEC_TAG` - Sets the secondary security tag that can be used for a second CA root certificate.
* :kconfig:option:`CONFIG_MQTT_HELPER_STACK_SIZE` - Sets the stack size for the internal thread in the library.
* :kconfig:option:`CONFIG_MQTT_HELPER_RX_TX_BUFFER_SIZE` - Sets the size of the MQTT RX and TX buffer that limits the message size, excluding the payload size.
* :kconfig:option:`CONFIG_MQTT_HELPER_PAYLOAD_BUFFER_LEN` - Sets the MQTT payload buffer size.
* :kconfig:option:`CONFIG_MQTT_HELPER_PROVISION_CERTIFICATES`
* :kconfig:option:`CONFIG_MQTT_HELPER_CERTIFICATES_FILE`
* :kconfig:option:`CONFIG_MQTT_HELPER_IPV4_ONLY` - Force using legacy IPv4 for even when running dual stack.


API documentation
*****************

| Header file: :file:`include/net/mqtt_helper.h`
| Source file: :file:`subsys/net/lib/mqtt_helper/mqtt_helper.c`

.. doxygengroup:: mqtt_helper
   :project: nrf
   :members:
   :inner:
