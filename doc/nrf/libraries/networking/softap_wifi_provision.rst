.. _lib_softap_wifi_provision:

SoftAP Wi-Fi provision
######################

.. contents::
   :local:
   :depth: 2

The SoftAP Wi-Fi provision library provides means to provision a Nordic Semiconductor Wi-Fi® device over HTTPS using SoftAP mode.
The device acts as an HTTP server, supporting multiple resources to get available Wi-Fi networks and to provision the device with the credentials to the desired network.

Overview
********

To provision the device, an HTTP client needs to connect to the SoftAP network (default is *nrf-wifiprov*).
The HTTP client then sends Wi-Fi credentials to the server by sending a POST request to ``/prov/configure``.
If the request succeeds, the device parses the content of the request and retrieves the credentials needed to connect to the Wi-Fi network.
When the device connects to the Wi-Fi network, the SoftAP network is disabled, and the device is considered provisioned.
If the client wants to verify that the provisioning was successful, it can connect to the same network and ping the device using the hostname defined by the Kconfig option :kconfig:option:`CONFIG_NET_HOSTNAME`.

To see how this library can be used in an application, refer to the :ref:`softap_wifi_provision_sample` sample.

SSID and SoftAP network
=======================

The default hostname that the library is reachable by is set by the Kconfig option :kconfig:option:`CONFIG_NET_HOSTNAME`.
The default SSID advertised in SoftAP mode is set by the Kconfig option :kconfig:option:`CONFIG_SOFTAP_WIFI_PROVISION_SSID`.

Protobuf
========

The library supports protobuf to encode or decode messages that are exchanged between the HTTP server and client.
The protobuf schema used for HTTP message content encoding is located under :file:`subsys/net/lib/softap_wifi_provision/proto/`.

.. _softap_wifi_provision_http_resources:

HTTP Resources
==============

The SoftAP Wi-Fi provision library provides the following HTTP resources:

* GET /prov/networks:

    * Description- Get available Wi-Fi networks.
    * Response content- Protobuf message that contains a list of available Wi-Fi networks.
    * Protobuf message structure- *ScanResults*

* POST /prov/configure:

    * Description- POST credentials to the device, provisioning it to a Wi-Fi network.
    * Response content- N/A
    * Protobuf message structure- *WifiConfig*

The following is an example of a request using curl:

.. code-block:: bash

      curl --cacert server_certificate.pem -ipv4 GET https://wifiprov.local/prov/networks
      echo -n "<hex payload>" | xxd -r -p | curl --cacert server_certificate.pem -ipv4 -X POST https://wifiprov.local/prov/configure --data-binary @-

Security
========

The library uses TLS to secure the communication between the client and the server.
By default, the library expects a certificate to be present in the folder defined by the Kconfig option :kconfig:option:`CONFIG_SOFTAP_WIFI_PROVISION_SERVER_CERTIFICATES_FOLDER`, relative to the application's root folder.
The certificate is provisioned to a security tag defined by the Kconfig option :kconfig:option:`CONFIG_SOFTAP_WIFI_PROVISION_CERTIFICATE_SEC_TAG`.

In a production scenario, it is recommended to provision credentials independently of the application to avoid hardcoding credentials in non-secure memory.
You can do this by disabling the option :Kconfig:option:`CONFIG_SOFTAP_WIFI_PROVISION_SERVER_CERTIFICATE_REGISTER` and using :ref:`TLS Credentials Shell <zephyr:tls_credentials_shell>` commands to store the credentials in the device's protected storage.
It is important that the credentials are stored in the security tag defined by the Kconfig option :kconfig:option:`CONFIG_SOFTAP_WIFI_PROVISION_CERTIFICATE_SEC_TAG`.
Otherwise, the library will not be able to use the credentials.

It is recommended to store the Wi-Fi credentials in the device's protected storage.
If you are using the TLS credentials API, to enable the API, the :Kconfig:option:`CONFIG_TLS_CREDENTIALS_BACKEND_PROTECTED_STORAGE` Kconfig options must be enabled.


Architecture
************

The library uses an event-driven architecture that combines events and states to handle the provisioning process.
Events are put in a queue and processed by the state machine in a separate thread internal to the library.

The library has the following states:

* ``STATE_INIT``- The initial state of the library.
* ``STATE_UNPROVISIONED``- The device is unprovisioned.
* ``STATE_PROVISIONING``- The provisioning process is ongoing.
* ``STATE_PROVISIONED``- The device is provisioned.
* ``STATE_RESET``- The library has been reset.

The library has the following events:

* ``EVENT_PROVISIONING_START``- :c:func:`softap_wifi_provision_start` called, starting the provisioning process.
* ``EVENT_AP_ENABLE``- SoftAP enabled, advertising the SSID.
* ``EVENT_AP_DISABLE``- SoftAP disabled.
* ``EVENT_SCAN_DONE``- Scan done, available Wi-Fi networks stored.
* ``EVENT_CREDENTIALS_RECEIVED``- Credentials received from the client.
* ``EVENT_RESET``- :c:func:`softap_wifi_provision_start` called, resetting the library.

The following diagram shows the relationship between the different states and events in the SoftAP Wi-Fi  provision library:

.. image:: /images/wifi_provisioning_state_diagram.svg
   :alt: SoftAP Wi-Fi provision state diagram
   :align: center

Configuration
*************

To use the SoftAP Wi-Fi provision library, you must enable the :kconfig:option:`CONFIG_SOFTAP_WIFI_PROVISION` Kconfig option.

The following Kconfig options are available to configure the library:

* :kconfig:option:`CONFIG_SOFTAP_WIFI_PROVISION`
* :kconfig:option:`CONFIG_SOFTAP_WIFI_PROVISION_SERVER_CERTIFICATE_REGISTER`
* :kconfig:option:`CONFIG_SOFTAP_WIFI_PROVISION_SERVER_CERTIFICATES_FOLDER`
* :kconfig:option:`CONFIG_SOFTAP_WIFI_PROVISION_RESPONSE_BUFFER_SIZE`
* :kconfig:option:`CONFIG_SOFTAP_WIFI_PROVISION_TCP_RECV_BUF_SIZE`
* :kconfig:option:`CONFIG_SOFTAP_WIFI_PROVISION_SCAN_RESULT_BUFFER_SIZE`
* :kconfig:option:`CONFIG_SOFTAP_WIFI_PROVISION_THREAD_STACK_SIZE`
* :kconfig:option:`CONFIG_SOFTAP_WIFI_PROVISION_URL_MAX_SIZE`
* :kconfig:option:`CONFIG_SOFTAP_WIFI_PROVISION_BODY_MAX_SIZE`
* :kconfig:option:`CONFIG_SOFTAP_WIFI_PROVISION_SSID`
* :kconfig:option:`CONFIG_SOFTAP_WIFI_PROVISION_SOCKET_CLOSE_ON_COMPLETION`
* :kconfig:option:`CONFIG_SOFTAP_WIFI_PROVISION_CERTIFICATE_SEC_TAG`
* :kconfig:option:`CONFIG_SOFTAP_WIFI_PROVISION_TCP_PORT`
* :kconfig:option:`CONFIG_SOFTAP_WIFI_PROVISION_IPV4_ADDRESS`
* :kconfig:option:`CONFIG_SOFTAP_WIFI_PROVISION_IPV4_NETMASK`
* :kconfig:option:`CONFIG_SOFTAP_WIFI_PROVISION_MESSAGE_QUEUE_ENTRIES`

Usage
*****

In order to use the library, the function :c:func:`softap_wifi_provision_init` must be first called.
This function initializes the library and registers a handler :c:func:`softap_wifi_provision_evt_handler_t` that is used to notify the application during the provisioning process.

To start provisioning, the function :c:func:`softap_wifi_provision_start` must be called.
A prerequisite for this function is that the network interface has been brought up.

At any time during the provisioning process, the function :c:func:`softap_wifi_provision_reset` can be called to reset the library and stop the provisioning process.

The following example demonstrates how to use the SoftAP Wi-Fi provision library:

.. code-block:: c

   #include <net/softap_wifi_provision.h>

   static void softap_wifi_provision_handler(const struct softap_wifi_provision_evt *evt)
   {
      int ret;

      switch (evt->type) {
      case SOFTAP_WIFI_PROVISION_EVT_STARTED:
         LOG_INF("Provisioning started");
         break;
      case SOFTAP_WIFI_PROVISION_EVT_CLIENT_CONNECTED:
         LOG_INF("Client connected");
         break;
      case SOFTAP_WIFI_PROVISION_EVT_CLIENT_DISCONNECTED:
         LOG_INF("Client disconnected");
         break;
      case SOFTAP_WIFI_PROVISION_EVT_CREDENTIALS_RECEIVED:
         LOG_INF("Wi-Fi credentials received");
         break;
      case SOFTAP_WIFI_PROVISION_EVT_COMPLETED:
         LOG_INF("Provisioning completed");
         break;
      case SOFTAP_WIFI_PROVISION_EVT_UNPROVISIONED_REBOOT_NEEDED:
         LOG_INF("Reboot request notified, rebooting...");
         sys_reboot(0);
         break;
      case SOFTAP_WIFI_PROVISION_EVT_FATAL_ERROR:
         LOG_ERR("Provisioning failed, fatal error!");
         break;
      default:
         /* Don't care */
         return;
      }
   }

   int main(void)
   {
         int ret;

         ret = softap_wifi_provision_init(softap_wifi_provision_handler);
         if (ret) {
            LOG_ERR("softap_wifi_provision_init, error: %d", ret);
            return ret;
         }

         ret = conn_mgr_all_if_up(true);
         if (ret) {
            LOG_ERR("conn_mgr_all_if_up, error: %d", ret);
            return ret;
         }

         LOG_INF("Network interface brought up");

         /* Start provisioning, this function blocks. */
         ret = softap_wifi_provision_start();
         if (ret == -EALREADY) {
            LOG_INF("Wi-Fi credentials found, skipping provisioning");
            return ret;
         } else if (ret) {
               LOG_ERR("softap_wifi_provision_start, error: %d", ret);
            return ret;
         }
   }


Troubleshooting
===============

For issues related to the library and |NCS| in general, refer to :ref:`known_issues`.

Multicast Domain Name System Service Discovery (mDNS SD) that is used to resolve the device hostname to an IP address might be unstable when enabling Wi-Fi power save mode.
It is recommended to disable Wi-Fi power save mode during provisioning to ensure that the device is reachable.
This is because multicast messages in general are more unstable when the device is in power save mode.

API documentation
*****************

| Header file: :file:`include/net/softap_wifi_provision.h`
| Source files: :file:`subsys/net/lib/softap_wifi_provision`

.. doxygengroup:: softap_wifi_provision_library
   :project: nrf
   :members:
