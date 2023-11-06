.. _nrf_provisioning_sample:

Cellular: nRF Device provisioning
#################################

.. contents::
   :local:
   :depth: 2

This sample demonstrates how to use the :ref:`lib_nrf_provisioning` service on your device.

Requirements
************

The sample supports the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf9161dk_nrf9161_ns

.. include:: /includes/tfm.txt

The sample requires that the device's private key is installed on the device and the associated device UUID is obtained from the Identity Service.

.. note::
   This sample requires modem firmware v2.0.0 or later.

Overview
********

The sample shows how the device performs the following actions:

* Connects to nRF Cloud Provisioning Service.
* Fetches available device-specific provisioning configuration.
* Decodes the commands.
* Acts on any AT commands, if available.
* Reports the results back to the server.
  In the case of an error, stops processing the commands at the first error and reports it back to server.
* Sends ``FINISHED`` response if all the previous commands are executed without errors provided and ``FINISHED`` is one of the set provisioning commands.

User interface
**************

Device side interaction is not required.
You must define the provisioning configuration at the server side.
See `nRF Cloud provisioning configuration`_.

Configuration
*************

|config|

Configuration options
=====================

Check and configure the following configuration options for the sample:

.. _CONFIG_NRF_PROVISIONING_ROOT_CA_SEC_TAG:

CONFIG_NRF_PROVISIONING_ROOT_CA_SEC_TAG
   Root CA security tag for the nRF Cloud Provisioning Service.
   Needs to be set explicitly and if not, the compilation fails.

.. _CONFIG_RF_PROVISIONING_RX_BUF_S:

CONFIG_RF_PROVISIONING_RX_BUF_S
   Configures the response payload buffer size.

.. _CONFIG_NRF_PROVISIONING_TX_BUF_SZ:

CONFIG_NRF_PROVISIONING_TX_BUF_SZ
   Configures the command request buffer size.

HTTP options
------------

.. _CONFIG_NRF_PROVISIONING_HTTP_HOSTNAME:

CONFIG_NRF_PROVISIONING_HTTP_HOSTNAME
   Configures the hostname of the nRF Cloud Provisioning Service.

.. _CONFIG_NRF_PROVISIONING_HTTP_PORT:

CONFIG_NRF_PROVISIONING_HTTP_PORT
   Configures the HTTP port of the nRF Cloud Provisioning Service.

.. _CONFIG_NRF_PROVISIONING_HTTP_TIMEOUT_MS:

CONFIG_NRF_PROVISIONING_HTTP_TIMEOUT_MS
   Configures the HTTP timeout.

CoAP options
------------

.. _CONFIG_NRF_PROVISIONING_COAP_HOSTNAME:

CONFIG_NRF_PROVISIONING_COAP_HOSTNAME
   Configures the hostname of the nRF Cloud Provisioning Service.

.. _CONFIG_NRF_PROVISIONING_COAP_PORT:

CONFIG_NRF_PROVISIONING_COAP_PORT
   Configures the CoAP port of the nRF Cloud Provisioning Service.

.. _CONFIG_NRF_PROVISIONING_COAP_DTLS_SESSION_CACHE:

CONFIG_NRF_PROVISIONING_COAP_DTLS_SESSION_CACHE
   Enables DTLS session cache.

Configuration files
===================

The sample provides predefined configuration files for typical use cases.

The following files are available:

* :file:`prj.conf` - Standard default configuration file.
* :file:`overlay-coap.conf` - Enables CoAP transfer protocol support.
* :file:`overlay-at_shell.conf` - Enables writing of large certificates from AT shell.

Building and running
********************

.. |sample path| replace:: :file:`samples/cellular/nrf_provisioning`

.. include:: /includes/build_and_run_ns.txt

Testing
=======

|test_sample|

#. |connect_kit|
#. |connect_terminal|
#. Add a provisioning configuration using the nRF Cloud Provisioning Service.
   See `nRF Cloud provisioning configuration`.
#. Power on or reset your device.
#. Observe that the sample starts and connects to the LTE network.
#. Observe that provisioning pauses and resumes while fetching and executing provisioning commands.

Sample output
=============

The following is an example output of the sample when there is no provisioning configuration on the server side:

.. code-block:: console

	<inf> nrf_provisioning_sample: Establishing LTE link ...
	<inf> nrf_provisioning_http: Requesting commands
	<inf> nrf_provisioning_http: Connected
	<inf> nrf_provisioning_http: No more commands to process on server side

The following is an example output when the sample is processing commands from the server:

.. code-block:: console

	<inf> nrf_provisioning_sample: Establishing LTE link ...
	<inf> nrf_provisioning_http: Requesting commands
	<inf> nrf_provisioning_http: Connected
	<inf> nrf_provisioning_http: Processing commands
	<inf> nrf_provisioning: Disconnected from network - provisioning paused
	<inf> nrf_provisioning: Connected; home network - provisioning resumed
	<inf> nrf_provisioning_sample: Modem connection restored
	<inf> nrf_provisioning_sample: Waiting for modem to acquire network time...
	<inf> nrf_provisioning_sample: Network time obtained
	<inf> nrf_provisioning_http: Sending response to server
	<inf> nrf_provisioning_http: Requesting commands
	<inf> nrf_provisioning_http: Connected
	<inf> nrf_provisioning_http: No more commands to process on server side

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`lte_lc_readme`
* :ref:`lib_nrf_provisioning`

It uses the following `sdk-nrfxlib`_ library:

* :ref:`nrfxlib:nrf_modem`
