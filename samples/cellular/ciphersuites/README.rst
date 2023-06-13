.. _ciphersuites:

Cellular: TLS cipher suites
###########################

.. contents::
   :local:
   :depth: 2

The Transport Layer Security (TLS) cipher suites sample demonstrates a minimal implementation of a client application that attempts to connect to a host by trying different TLS cipher suites.
This sample shows the cipher suites and lists them as supported or not supported by the host, and provides a summary of the support.

Requirements
************

The sample supports the following development kit:

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

Overview
********

The sample first initializes the :ref:`nrfxlib:nrf_modem` and AT communications.
Next, it provisions a root CA certificate to the modem using the :ref:`modem_key_mgmt` library.
Provisioning must be done before connecting to the LTE network because the certificates can only be provisioned when the device is not connected.

The sample then iterates through a list of TLS cipher suites, attempting connection to the host with each one of them.
The sample connects successfully to the host (``www.example.com``) with the cipher suites that are supported by the host, while unsupported cipher suites cause a connection failure, setting ``errno`` to ``95``.

Finally, the sample provides a summary of the cipher suites that are supported and not supported by the host, ``example.com``.

Obtaining a certificate
=======================

The sample connects to ``www.example.com``, which requires an X.509 certificate.
This certificate is provided in the :file:`samples/cellular/https_client/cert` folder.

To connect to other servers, you might need to provision a different certificate.
See :ref:`cert_dwload` for more information.

Configuration
**************

|config|

Configuration options
=====================

Check and configure the following Kconfig options:

.. _CONFIG_EXTENDED_CIPHERSUITE_LIST:

CONFIG_EXTENDED_CIPHERSUITE_LIST
   The sample configuration extends the cipher suite list with extra cipher suites that are only supported by modem firmware v1.3.x, where x is greater than or equal to 1 and modem firmware v1.2.x, where x is greater than or equal to 7.

.. include:: /libraries/modem/nrf_modem_lib/nrf_modem_lib_trace.rst
   :start-after: modem_lib_sending_traces_UART_start
   :end-before: modem_lib_sending_traces_UART_end

Building and running
********************

.. |sample path| replace:: :file:`samples/cellular/ciphersuites`

.. include:: /includes/build_and_run_ns.txt

Testing
=======

|test_sample|

1. |connect_kit|
#. |connect_terminal|
#. Observe that the sample starts, provisions certificates, and connects to the LTE network.
#. Observe that the sample iterates through a list of cipher suites, attempting a connection to ``example.com`` with each one of them, showing either a successful or an unsuccessful connection.

Sample output
=============

The sample shows the following output:

.. code-block:: console

   TLS ciphersuites sample started
   certificate match
   waiting for network.. OK
   trying all ciphersuites to find which ones are supported...
   trying ciphersuite: TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA384
   connecting to example.com... connect() failed, err: 95, Operation not supported on socket
   trying ciphersuite: TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA
   connecting to example.com... connect() failed, err: 95, Operation not supported on socket
   trying ciphersuite: TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA256
   connecting to example.com... connect() failed, err: 95, Operation not supported on socket
   trying ciphersuite: TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA
   connecting to example.com... connect() failed, err: 95, Operation not supported on socket
   trying ciphersuite: TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA
   connecting to example.com... Connected.
   trying ciphersuite: TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA256
   connecting to example.com... Connected.
   trying ciphersuite: TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA
   connecting to example.com... Connected.
   trying ciphersuite: TLS_PSK_WITH_AES_256_CBC_SHA
   connecting to example.com... connect() failed, err: 95, Operation not supported on socket
   trying ciphersuite: TLS_PSK_WITH_AES_128_CBC_SHA256
   connecting to example.com... connect() failed, err: 95, Operation not supported on socket
   trying ciphersuite: TLS_PSK_WITH_AES_128_CBC_SHA
   connecting to example.com... connect() failed, err: 95, Operation not supported on socket
   trying ciphersuite: TLS_PSK_WITH_AES_128_CCM_8
   connecting to example.com... connect() failed, err: 95, Operation not supported on socket
   trying ciphersuite: TLS_EMPTY_RENEGOTIATIONINFO_SCSV
   connecting to example.com... connect() failed, err: 95, Operation not supported on socket

   Ciphersuite support summary for host `example.com`:
   TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA384: No
   TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA: No
   TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA256: No
   TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA: No
   TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA: Yes
   TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA256: Yes
   TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA: Yes
   TLS_PSK_WITH_AES_256_CBC_SHA: No
   TLS_PSK_WITH_AES_128_CBC_SHA256: No
   TLS_PSK_WITH_AES_128_CBC_SHA: No
   TLS_PSK_WITH_AES_128_CCM_8: No
   TLS_EMPTY_RENEGOTIATIONINFO_SCSV: No

   finished.

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`modem_key_mgmt`
* :ref:`lte_lc_readme`

It uses the following `sdk-nrfxlib`_ library:

* :ref:`nrfxlib:nrf_modem`

In addition, it uses the following secure firmware component:

* :ref:`Trusted Firmware-M <ug_tfm>`
