.. _download_sample:

nRF9160: Download client
########################

.. contents::
   :local:
   :depth: 2

The Download client sample demonstrates how to download a file from an HTTP or a CoAP server, with optional TLS or DTLS.
It uses the :ref:`lib_download_client` library.

Requirements
************

The sample supports the following development kit:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf9160dk_nrf9160ns


.. include:: /includes/spm.txt

Overview
********

The sample first initializes the :ref:`nrfxlib:nrf_modem` and AT communications.
Next, it provisions a certificate to the modem using the :ref:`modem_key_mgmt` library if the :option:`CONFIG_SAMPLE_SECURE_SOCKET` option is set.
The provisioning of the certificates must be done before connecting to the LTE network since the certificates can only be provisioned when the device is not connected.
The certificate file name and security tag can be configured via the :option:`CONFIG_SAMPLE_SEC_TAG` and the :option:`CONFIG_SAMPLE_CERT_FILE` options, respectively.

The sample then performs the following actions:

1. Establishes a connection to the LTE network
#. Optionally sets up the secure socket options
#. Uses the :ref:`lib_download_client` library to download a file from an HTTP server.


Downloading from a CoAP server
==============================

To enable CoAP block-wise transfer, it is necessary to enable :ref:`Zephyr's CoAP stack <zephyr:coap_sock_interface>` via the :option:`CONFIG_COAP` option.

Using TLS and DTLS
==================

When the :option:`CONFIG_SAMPLE_SECURE_SOCKET` option is set, the sample provisions the certificate found in the :file:`samples/nrf9160/download/cert` folder.
The certificate file name is indicated by the :option:`CONFIG_SAMPLE_CERT_FILE` option.
This certificate will work for the default test files.
If you are using a custom download test file, you have to provision the correct certificate for the servers from which the certificates will be downloaded.

See :ref:`cert_dwload` for more information.

Configuration
*************

|config|

Configuration options
=====================

Check and configure the following configuration options for the sample:

.. option:: CONFIG_SAMPLE_SECURE_SOCKET - Secure socket configuration

If enabled, this option provisions the certificate to the modem.

.. option:: CONFIG_SAMPLE_SEC_TAG - Security tag configuration

This option configures the security tag.

.. option:: CONFIG_SAMPLE_CERT_FILE - Certificate file name configuration

This option sets the certificate file name.

.. option:: CONFIG_SAMPLE_COMPUTE_HASH - Hash compute configuration

If enabled, this option computes the SHA256 hash of the downloaded file.

.. option:: CONFIG_SAMPLE_COMPARE_HASH - Hash compare configuration

If enabled, this option compares the hash against the SHA256 hash set by :option:`CONFIG_SAMPLE_SHA256_HASH` for a match.

.. option:: CONFIG_SAMPLE_SHA256_HASH - Hash configuration

This option sets the SHA256 hash to be compared with :option:`CONFIG_SAMPLE_COMPUTE_HASH`.


Building and running
********************

.. |sample path| replace:: :file:`samples/nrf9160/download`

.. include:: /includes/build_and_run_nrf9160.txt


Testing
=======

After programming the sample to your development kit, test it by performing the following steps:

1. Connect the development kit to your PC using a USB cable and power on or reset the kit.
#. Open a terminal emulator and observe that the sample starts, provisions certificates, and starts to download.
#. Observe that the progress bar fills up as the download progresses.
#. Observe that the sample displays the message "Download completed" on the terminal when the download completes.

Sample Output
=============

The following output is logged on the terminal when the sample downloads a file from an HTTPS server:

.. code-block:: console

   Download client sample started
   Provisioning certificate
   Waiting for network.. OK
   Downloading https://file-examples-com.github.io/uploads/2017/10/file_example_JPG_100kB.jpg
   [ 100% ] |==================================================| (102117/102117 bytes)
   Download completed in 13302 ms @ 7676 bytes per sec, total 102117 bytes
   Bye

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`at_cmd_readme`
* :ref:`at_notif_readme`
* :ref:`modem_key_mgmt`
* :ref:`lte_lc_readme`

It uses the following `sdk-nrfxlib`_ library:

* :ref:`nrfxlib:nrf_modem`

In addition, it uses the following sample:

* :ref:`secure_partition_manager`
