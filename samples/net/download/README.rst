.. _download_sample:

.. ncs-sample::
   :title: Download

   The Download sample demonstrates how to download a file from an HTTP or a CoAP server, with optional TLS or DTLS.
   It uses the :ref:`lib_downloader` library.
   The sample connects to either an LTE network using an nRF91 Series device, or to Wi-Fi® using an nRF71 Series or nRF70 Series device combined with a compatible host platform.

   .. |wifi| replace:: Wi-Fi

   .. include:: /includes/net_connection_manager.txt

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

Overview
********

The sample first initializes the device (:ref:`nrfxlib:nrf_modem` and AT communications for cellular devices).
Next, if the :option:`CONFIG_SAMPLE_PROVISION_CERT` is set, it provisions a certificate to the device if the :option:`CONFIG_SAMPLE_SECURE_SOCKET` option is set.
When using an nRF91 Series device, the provisioning of the certificates must be done before connecting to the LTE network since the certificates can only be provisioned when the device is not connected.
The certificate file name and security tag can be configured using the :option:`CONFIG_SAMPLE_SEC_TAG` and the :option:`CONFIG_SAMPLE_CERT_FILE` options, respectively.

The sample then performs the following actions:

1. Establishes a connection to the network
#. Optionally sets up the secure socket options
#. Uses the :ref:`lib_downloader` library to download a file.

Selecting the HTTP(S) or CoAP(S) transport
===========================================

The :ref:`lib_downloader` library supports both HTTP(S) and CoAP(S), and this sample builds in support for both (:kconfig:option:`CONFIG_DOWNLOADER_TRANSPORT_HTTP` and :kconfig:option:`CONFIG_DOWNLOADER_TRANSPORT_COAP`, together with :kconfig:option:`CONFIG_COAP`).
No separate build-time choice is needed to pick between them.
The transport is selected automatically at runtime from the scheme of the URL being downloaded (``http://`` or ``https://`` for HTTP(S), ``coap://`` or ``coaps://`` for CoAP(S)).
Set :option:`CONFIG_SAMPLE_FILE_URL` (with :option:`CONFIG_SAMPLE_FILE_CUSTOM` selected) to a URL with the appropriate scheme to exercise either transport or security level.

Using TLS and DTLS
==================

By default, the :option:`CONFIG_SAMPLE_PROVISION_CERT` option is set, which means that the sample provisions the certificate found in the :file:`samples/net/download/cert` folder.
The :option:`CONFIG_SAMPLE_CERT_FILE` option indicates the certificate file name.
This certificate will work for the default test files.
If you are using a custom download test file, you must provision the correct certificate for the servers from which the certificates will be downloaded.

|hex_format|

See :ref:`cert_dwload` for more information.

.. _download_sample_mtls:

Mutual TLS (client certificate authentication)
----------------------------------------------

The sample can optionally use mutual TLS (client certificate authentication), for both HTTP(S) and CoAP(S).

.. note::
   This functionality is only supported on Wi-Fi boards and not on cellular boards.

Enable the :option:`CONFIG_SAMPLE_PROVISION_CLIENT_CERT` option to provision a client certificate and private key, in addition to the CA certificate, under the same security tag.
Set :option:`CONFIG_SAMPLE_CLIENT_CERT_FILE` and :option:`CONFIG_SAMPLE_CLIENT_KEY_FILE` to the client certificate and private key to provision.
This must match what the server you connect to expects.

The sample includes an example client certificate and private key, together with a matching CA trust store, under :file:`cert/`, for use against the `Eclipse Californium`_ CoAP interop server:

* :file:`cert/cf-ca.pem` - CA trust store (root + intermediate)
* :file:`cert/cf-client.pem` - Client leaf certificate
* :file:`cert/cf-client-key.pem` - Client private key (EC P-256)

The :file:`wifi-mutual-dtls.conf` extra-conf file configures the sample to use these to perform a mutual TLS DTLS download from the Californium interop server.

Wi-Fi
=====

On Wi-Fi boards, networking and TLS/DTLS support are not part of the default configuration and must be added with the :file:`wifi.conf` extra-conf file, using the ``download_EXTRA_CONF_FILE`` sysbuild variable.
To perform a mutual DTLS download from the Californium interop server (see :ref:`Mutual TLS (client certificate authentication) <download_sample_mtls>`), add the :file:`wifi-mutual-dtls.conf` extra-conf file on top of :file:`wifi.conf`.

Configuration
*************

|config|

Configuration options
=====================

The following sample-specific Kconfig options are used in this sample (located in :file:`samples/net/download/Kconfig`):

.. options-from-kconfig::
   :show-type:

.. include:: /includes/wifi_credentials_shell.txt

.. include:: /includes/wifi_credentials_static.txt

.. include:: /libraries/modem/nrf_modem_lib/nrf_modem_lib_trace.rst
   :start-after: modem_lib_sending_traces_UART_start
   :end-before: modem_lib_sending_traces_UART_end

Configuration files
===================

The sample provides predefined configuration files for the following development kits:

* :file:`prj.conf` - General configuration file for all devices.
* :file:`boards/nrf9151dk_nrf9151_ns.conf` - Configuration file for the nRF9151 DK.
* :file:`boards/nrf9161dk_nrf9161_ns.conf` - Configuration file for the nRF9161 DK.
* :file:`boards/nrf9160dk_nrf9160_ns.conf` - Configuration file for the nRF9160 DK.
* :file:`boards/nrf7002dk_nrf5340_cpuapp_ns.conf` - Board-specific configuration for the nRF7002 DK.
* :file:`boards/nrf7120dk_nrf7120_cpuapp_ns.conf` - Board-specific configuration for the nRF7120 DK.
* :file:`boards/native_sim.conf` - Configuration file for the native simulator emulation.
* :file:`wifi.conf` - Wi-Fi networking configuration, common to the nRF71 Series and nRF70 Series.
  It must be added explicitly for Wi-Fi builds.
* :file:`wifi-mutual-dtls.conf` - Additional configuration for mutual DTLS downloads from the Californium interop server.

Board files under :file:`boards/` are merged automatically for the selected target.
The :file:`wifi.conf` and :file:`wifi-mutual-dtls.conf` files are not board-specific and must be passed with ``download_EXTRA_CONF_FILE``.

To add a specific extra configuration file to the build, add the ``-- -Ddownload_EXTRA_CONF_FILE=<extra_conf_file>`` flag to your west build command.

Building and running
********************

.. |sample path| replace:: :file:`samples/net/download`

.. include:: /includes/build_and_run_ns.txt


Testing
=======

After programming the sample to your development kit, test it by performing the following steps:

1. |connect_kit|
#. Power on or reset the kit.
#. |connect_terminal_ANSI|
#. Observe that the sample starts, provisions certificates, and starts to download.
#. Observe that the progress bar fills up as the download progresses.
#. Observe that the sample displays the message "Download completed" on the terminal when the download completes.

Sample output
=============

The following output is logged on the terminal when the sample downloads the default HTTPS test file over Wi-Fi:

.. code-block:: console

   Download client sample started
   Connecting to network
   IP Up
   Network connected
   Downloading https://nrfconnectsdk.s3.eu-central-1.amazonaws.com/sample-img-100kb.png
   Setting up TLS credentials, sec tag count 1

   [ 100% ] (102923/102923 bytes)

   Download completed in 25233 ms @ 4078 bytes per sec, total 102923 bytes
   SHA256: 344577d739dc0e9f9498c13be2fbc7fb947abd291e9e57db9e9ca4a89ccd5f63
   Bye
   IP down
   Disconnected from network

Troubleshooting
===============

If you have issues with connectivity on nRF91 Series devices, see the `Cellular Monitor app`_ documentation to learn how to capture modem traces in order to debug network traffic in Wireshark.
This sample enables modem traces by default.

If you have issues with connectivity on nRF71 Series or nRF70 Series devices, see the :ref:`wifi_monitor_sample` sample documentation to learn how to capture and analyze Wi-Fi traffic in order to debug connectivity issues.
Also verify that the Wi-Fi credentials configured for your device match your access point, as described in the `Configuration options`_ section on this page.

Dependencies
************

This sample uses the following |NCS| libraries when using an nRF91 Series device:

* :ref:`modem_key_mgmt`
* :ref:`nrf_modem_lib_readme`

It uses the following `sdk-nrfxlib`_ library:

* :ref:`nrfxlib:nrf_modem`

In addition, it uses the following secure firmware component:

* :ref:`Trusted Firmware-M <ug_tfm>`

It uses the following Zephyr libraries:

* :ref:`lib_downloader`
* :ref:`Connection Manager <zephyr:conn_mgr_overview>`
