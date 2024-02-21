.. _http_server:

HTTP Server
###########

.. contents::
   :local:
   :depth: 2

The HTTP Server sample demonstrates how to host an HTTP server on a Nordic Semiconductor device that is connected to the Internet through LTE or Wi-Fi®.

.. |wifi| replace:: Wi-Fi

.. include:: /includes/net_connection_manager.txt

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

Overview
********

The sample supports managing the state of **LED 1** and **LED 2** on a compatible development kit.
This feature allows you to both set and retrieve the current state of the LEDs through HTTP calls from a REST client.

Moreover, the sample also offers the option to enable TLS for secure communication.
You can configure TLS with or without server and/or device authentication based on your security requirements.

There are slight differences in how the sample behaves depending on the wireless technology for which the sample is built.
The following table explains some of these differences:

+------------+------------------+---------------------------------------+----------------+-------------------------------+------------------------+
|            | Network access   | LED resource URL                      | Security       | Peer verification             | IP family [3]_         |
+============+==================+=======================================+================+===============================+========================+
| Wi-Fi      | Local            | \http://httpserver.local/led/<led_id> | TLS (optional) | Server and client [2]_        | IPv4 IPv6              |
+------------+------------------+---------------------------------------+----------------+-------------------------------+------------------------+
| Cellular   | Global [1]_      | \http://<ip>/led/<led_id>             | TLS (optional) | Server and client [2]_        | IPv4 IPv6              |
+------------+------------------+---------------------------------------+----------------+-------------------------------+------------------------+

.. [1] When running the server on a cellular board, you are dependent on a SIM card with a fixed/static IP to be reachable from the global Internet.
       These SIM cards can be provided by Mobile Network Operators (MNO) and are often referred to as IoT SIM cards.
.. [2] The sample includes pregenerated server and client certificates with the local hostname *httpserver.local* used as the Common Name (CN).
       These certificates work out of the box when building the sample for Wi-Fi.
       On a cellular network, a static IP is required to reach the server.
       This means that a new set of server and client certificates needs to be generated, with a CN matching the assigned IP.
.. [3] IPv4 and/or IPv6 might not be enabled in your Wi-Fi Access Point (AP) or cellular network.

.. tabs::

   .. group-tab:: Wi-Fi

      When the server is built for Wi-Fi, the server's availability is limited to the local network.
      It can also be globally available by configuring your AP and giving it a static IP, but this is not covered in this documentation.

      The sample supports mDNS with a hostname set by the :kconfig:option:`CONFIG_NET_HOSTNAME` option, which defaults to *httpserver.local*.
      To access the server there are a few considerations.
      Not all access points support IPv6.
      The access points that support IPv6 is required to enable the appropriate IPv6 settings mentioned in the configuration page of the access points.
      To check if the device is assigned an IP, run the following shell command in a serial terminal connected to the server:

      .. code-block:: console

         uart:~$ net ipv6
         ...
         IPv6 addresses for interface 1 (0x200190d0) (WiFi)
         ================================================
         Type            State           Lifetime (sec)  Address
         autoconf        preferred       infinite        fe80::f7ce:37ff:fe00:1971/128
         autoconf        preferred       9250            2001:8c0:5140:895:f7ce:37ff:fe00:1971/64

      If the device has received a valid IPv6 address, it is usually assigned a lifetime and has an address slightly longer than its local IPv6 address, a combination of a local and global address.
      In this case, the address is ``2001:8c0:5140:895:f7ce:37ff:fe00:1971``.

   .. group-tab:: Cellular

      To use a static SIM card, you may need to instruct the modem to use a specific Access Point Name (APN).
      This can be carried out by setting the following options:

      * :kconfig:option:`CONFIG_PDN_DEFAULTS_OVERRIDE` - Used to override the default PDP context configuration.
        Set the option to ``y`` to override the default PDP context configuration.
      * :kconfig:option:`CONFIG_PDN_DEFAULT_APN` - Used for manual configuration of the APN.
        An example is ``apn.example.com``.

      To verify that you have properly set up the AP and given a static IP, perform the following:

      * Enable the :kconfig:option:`CONFIG_AT_HOST_LIBRARY` option.
      * Run the ``AT+CGDCONT?`` AT command on a serial terminal.
        The command returns the available Packet Data Network (PDN) context with the accompanied APNs and given IPs.
        If you reboot the device, you should get the same IP.

      You might also want to address the responsiveness of the server.
      By default, the server disables both :term:`Power Saving Mode (PSM)` and :term:`Extended Discontinuous Reception (eDRX)` to ensure maximum availability, which increases the power consumption of the server.
      So, you can reduce power consumption by putting the device in sleep mode more frequently.

      A lot of networks do not cache downlink data when the device sleeps.
      So, requests can drop if sent when the device is in PSM.
      For more information on power-saving features and how to use them, see :ref:`lte_lc_power_saving`.

Security
========

To prevent attackers from getting access to the server, the sample supports TLS with the option of enabling peer verification.
With peer verification, the server requires that the client is authenticated during the TLS handshake process.
This requires that the client include a client certificate in the REST call that will be used to prove its authenticity.

To enable TLS, the sample includes pregenerated credentials that are located in the sample folder:

* :file:`http_server/credentials/server_certificate` - Self-signed server certificate
* :file:`http_server/credentials/server_private_key` - Server certificate private key
* :file:`http_server/credentials/client.crt` - Client certificate
* :file:`http_server/credentials/client.key` - Client private key

.. note::
   These should only be used for testing purposes.

Peer verification can be used by building the sample with the :ref:`CONFIG_HTTP_SERVER_SAMPLE_PEER_VERIFICATION_REQUIRE <CONFIG_HTTP_SERVER_SAMPLE_PEER_VERIFICATION_REQUIRE>` Kconfig option, and passing the appropriate credentials when making the HTTP calls.
These pregenerated credentials work only when you are building for Wi-Fi and running the server on your local Wi-Fi network.
You need to generate new credentials when building for cellular due to a CN mismatch.
To generate new credentials, run the following commands:

      .. code-block:: console

         # Generate a new RSA key pair (private key and public key) and create a self-signed X.509 certificate for a server.
         openssl req -newkey rsa:2048 -nodes -keyout server_private_key.pem -x509 -days 365 -out server_certificate.pem

         # Generate a new RSA private key for a client.
         openssl genpkey -algorithm RSA -out client.key

         # Create a Certificate Signing Request (CSR) for the client using the generated private key.
         openssl req -new -key client.key -out client.csr

         # Sign the client's CSR with the server's private key and certificate, creating a client certificate.
         openssl x509 -req -in client.csr -CA server_certificate.pem -CAkey server_private_key.pem -CAcreateserial -out client.crt -days 365

To provision the generated credentials to the server's TLS stack, replace the pregenerated certificates with the newly generated one in the :file:`http_server/credentials` folder in PEM format.
Provisioning happens automatically after the firmware boots by the sample.

Configuration
*************

|config|

Configuration options
=====================

The following lists the application-specific configurations used in the sample.
They are located in the :file:`samples/net/http_server/Kconfig` file.

.. _CONFIG_HTTP_SERVER_SAMPLE_PEER_VERIFICATION_REQUIRE:

CONFIG_HTTP_SERVER_SAMPLE_PEER_VERIFICATION_REQUIRE
   This configuration option enables peer verification for the sample.
   Enabling peer verification ensures that the server validates the authenticity of the client during the TLS handshake.

.. _CONFIG_HTTP_SERVER_SAMPLE_PORT:

CONFIG_HTTP_SERVER_SAMPLE_PORT
   This configuration option sets the server port for the sample.

.. _CONFIG_HTTP_SERVER_SAMPLE_CLIENTS_MAX:

CONFIG_HTTP_SERVER_SAMPLE_CLIENTS_MAX
   This configuration option sets the maximum number of concurrent clients for the sample.
   Increasing this option impacts the server's performance and increases the total memory footprint, as each client instance has its own thread with a dedicated stack.
   The option sets the number of maximum concurrent clients per IP family, allowing a total of 4 clients to be connected simultaneously if both IPv4 and IPv6 are enabled and the value is set to ``2``.

.. _CONFIG_HTTP_SERVER_SAMPLE_SERVER_CERTIFICATE_SEC_TAG:

CONFIG_HTTP_SERVER_SAMPLE_SERVER_CERTIFICATE_SEC_TAG
   This configuration option sets the security tag for the credentials used in TLS.

.. _CONFIG_HTTP_SERVER_SAMPLE_STACK_SIZE:

CONFIG_HTTP_SERVER_SAMPLE_STACK_SIZE
   This configuration option sets the stack size for the threads used in the sample.

.. _CONFIG_HTTP_SERVER_SAMPLE_RECEIVE_BUFFER_SIZE:

CONFIG_HTTP_SERVER_SAMPLE_RECEIVE_BUFFER_SIZE
   This configuration option sets the receive buffer size for the sockets used in the sample.

.. include:: /includes/wifi_credentials_shell.txt

.. include:: /includes/wifi_credentials_static.txt

Configuration files
===================

The sample includes pre-configured configuration files for the development kits that are supported:

* :file:`prj.conf` - For all devices.
* :file:`boards/nrf7002dk_nrf5340_cpuapp_ns.conf` - For the nRF7002 DK.
* :file:`boards/nrf9151dk_nrf9151_ns.conf` - For the nRF9151 DK.
* :file:`boards/nrf9161dk_nrf9161_ns.conf` - For the nRF9161 DK.
* :file:`boards/nrf9160dk_nrf9160_ns.conf` - For the nRF9160 DK.
* :file:`boards/thingy91_nrf9160_ns.conf` - For the Thingy:91.

Files that are located under the :file:`/boards` folder are automatically merged with the :file:`prj.conf` file when you build for the corresponding target.

Building and running
********************

.. |sample path| replace:: :file:`samples/net/http_server`

.. include:: /includes/build_and_run_ns.txt

Testing
=======

|test_sample|

1. |connect_kit|
#. |connect_terminal|
#. Reset your board.
#. Observe that the board connects to the network and waits for incoming HTTP connections.
#. Set or get the value of the two supported LEDs by performing REST calls to either of the two resource URLs.
   In the following example, `HTTPie`_ is used:

.. tabs::

   .. group-tab:: Wi-Fi

         Non TLS (unsecure):

         .. code-block:: console

            http PUT http://httpserver.local:80/led/1 --raw="1"
            http GET http://httpserver.local:80/led/1

         TLS with server authentication:

         .. code-block:: console

            https PUT https://httpserver.local:443/led/1 --raw="1" --verify server_certificate.pem
            https GET https://httpserver.local:443/led/1 --verify server_certificate.pem

         TLS with server authentication and client authentication:

         .. code-block:: console

            https PUT https://httpserver.local:443/led/1 --raw="1" --verify server_certificate.pem --cert client.crt --cert-key client.key
            https GET https://httpserver.local:443/led/1 --verify server_certificate.pem --cert client.crt --cert-key client.key

         It might be required to specify the IP address directly in the REST command if it does not work using the hostname:

         .. code-block:: console

            http PUT 'http://[2001:8c0:5140:895:f7ce:37ff:fe00:1971]:81/led/1' --raw="1"

   .. group-tab:: Cellular

         Non TLS (unsecure):

         .. code-block:: console

            http PUT http://<ip>:80/led/1 --raw="1"
            http GET http://<ip>:80/led/1

         TLS with server authentication:

         .. code-block:: console

            https PUT https://<ip>:443/led/1 --raw="1" --verify server_certificate.pem
            https GET https://<ip>:443/led/1 --verify server_certificate.pem

         TLS with server authentication and client authentication:

         .. code-block:: console

            https PUT https://<ip>:443/led/1 --raw="1" --verify server_certificate.pem --cert client.crt --cert-key client.key
            https GET https://<ip>:443/led/1 --verify server_certificate.pem --cert client.crt --cert-key client.key

Sample output
=============

The following serial UART output is displayed in the terminal emulator when running the sample:

.. tabs::

   .. group-tab:: Wi-Fi

      .. code-block:: console

         *** Booting nRF Connect SDK v3.4.99-ncs1-4667-g883c3709f9c8 ***
         [00:00:00.529,632] <inf> http_server: HTTP Server sample started
         [00:00:00.554,504] <inf> http_server: Network interface brought up
         uart:~$ wifi_cred add "cia-asusgold" WPA2-PSK thingy91rocks
         uart:~$ wifi_cred auto_connect
         [00:00:53.984,100] <inf> http_server: Network connected
         [00:00:53.985,778] <inf> http_server: Waiting for IPv6 HTTP connections on port 81, sock 9
         [00:00:54.986,511] <inf> http_server: Waiting for IPv4 HTTP connections on port 80, sock 10
         [00:01:20.705,139] <inf> http_server: [11] Connection from 192.168.50.134 accepted
         [00:01:20.710,998] <inf> http_server: LED 0 state updated to 1
         uart:~$

   .. group-tab:: Cellular

      .. code-block:: console

         [00:00:00.416,839] <inf> http_server: HTTP Server sample started
         [00:00:00.687,652] <inf> http_server: Network interface brought up
         [00:00:03.047,210] <inf> http_server: Network connected
         [00:00:03.048,522] <inf> http_server: Waiting for IPv6 HTTP connections on port 81, sock 0
         [00:00:04.057,067] <inf> http_server: Waiting for IPv4 HTTP connections on port 80, sock 1
         [00:00:19.522,796] <inf> http_server: [2] Connection from 194.19.86.146 accepted
         [00:00:19.557,128] <inf> http_server: LED 0 state updated to 1


The following serial output is from the terminal window that performs the HTTP calls:

.. code-block:: console

   ➜  ~ http PUT http://httpserver.local:80/led/1 --raw="1"
   HTTP/1.1 200 OK

   ➜  ~ http GET http://httpserver.local:80/led/1
   HTTP/1.1 200 OK
   Content-Length: 1

   1

Troubleshooting
***************

If you have issues with connectivity on nRF91 Series devices, see the `Cellular Monitor`_ documentation to learn how to capture modem traces to debug network traffic in Wireshark.
Modem traces can be enabled by providing a snippet with the west build command using the ``-S`` argument as shown in the following example for nRF9161 DK:

.. code-block:: console

   west build -p -b nrf9161dk_nrf9161_ns -S nrf91-modem-trace-uart

Dependencies
************

This sample uses the following Zephyr libraries:

* :ref:`net_if_interface`
* :ref:`net_mgmt_interface`
* :file:`zephyr/net/http/parser.h`
* :ref:`Connection Manager <zephyr:conn_mgr_overview>`
