.. _nrf_provisioning_sample:

nRF9160: nRF Device provisioning
################################

.. contents::
   :local:
   :depth: 2

This sample demonstrates how to use the :ref:`lib_nrf_provisioning` service on your device.

Requirements
************

The sample supports the following development kit:

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

The sample requires that private device key is installed on the device and the associated Device UUID is obtained from the Identity Service.

.. note::
   This sample requires modem firmware v1.3.x or later.

Overview
********

The sample shows how the device performs the following actions:

* Connects to nRF Device provisioning Service.
* Fetches available device specific provisioning commands.
* Decodes the commands.
* Acts on any AT commands, if available.
* Reports the results back to the server.
  In case of errors, stops processing the commands at the first error and reports it back to server.
* Sends ``FINISHED`` response if all the previous commands are executed without errors provided and ``FINISHED`` is one of the set provisioning commands.

User interface
**************

Device side interaction is not required.
You must fill the command queue at the server side.

Configuration
*************

|config|

Configuration options
=====================

Check and configure the following configuration options for the sample:

.. _CONFIG_NRF_PROVISIONING_ROOT_CA_SEC_TAG:

CONFIG_NRF_PROVISIONING_ROOT_CA_SEC_TAG
   Root CA security tag for the Nordic Semiconductor's provisioning service.
   Needs to be set explicitly and if not, the compilation fails.

.. _CONFIG_NRF_PROVISIONING_HTTP_HOSTNAME:

CONFIG_NRF_PROVISIONING_HTTP_HOSTNAME
   Hostname of the nRF Device provisioning service

.. _CONFIG_NRF_PROVISIONING_HTTP_PORT:

CONFIG_NRF_PROVISIONING_HTTP_PORT
   HTTP port of the nRF Device provisioning service

.. _CONFIG_NRF_PROVISIONING_HTTP_TIMEOUT_MS:

CONFIG_NRF_PROVISIONING_HTTP_TIMEOUT_MS
   HTTP timeout

.. _CONFIG_RF_PROVISIONING_HTTP_RX_BUF_S:

CONFIG_RF_PROVISIONING_HTTP_RX_BUF_S
   Response payload buffer size

.. _CONFIG_NRF_PROVISIONING_HTTP_TX_BUF_SZ:

CONFIG_NRF_PROVISIONING_HTTP_TX_BUF_SZ
   Command request body size

Building and running
********************

.. |sample path| replace:: :file:`samples/nrf9160/nrf_provisioning`

.. include:: /includes/build_and_run_ns.txt

Testing
=======

|test_sample|

#. |connect_kit|
#. |connect_terminal|
#. Add provisioning command(s) to nRF Provisioning Service.
#. Power on or reset your nRF9160 DK.
#. Observe that the sample starts and connects to the LTE network.
#. Observe that provisioning pauses and resumes while fetching and executing provisioning commands.

Sample Output
=============

The following is an example output of the sample when there is no commands queued on server side:

.. code-block:: console

	<inf> nrf_provisioning_sample: Establishing LTE link ...
	<inf> nrf_provisioning_http: Connected
	<inf> nrf_provisioning: Disconnected from network - provisioning paused
	<inf> nrf_provisioning: Connected; home network - provisioning resumed
	<inf> nrf_provisioning_sample: Modem connection restored
	<inf> nrf_provisioning_sample: Waiting for modem to acquire network time...
	<inf> nrf_provisioning_sample: Network time obtained
	<inf> nrf_provisioning_http: Connected
	<inf> nrf_provisioning_http: No more commands to process on server side

The following is an example output when the sample is processing commands from the server:

.. code-block:: console

	<inf> nrf_provisioning_sample: Establishing LTE link ...
	<inf> nrf_provisioning_http: Connected
	<inf> nrf_provisioning: Disconnected from network - provisioning paused
	<inf> nrf_provisioning: Disconnected from network - provisioning paused
	<inf> nrf_provisioning: Connected; home network - provisioning resumed
	<inf> nrf_provisioning_sample: Modem connection restored
	<inf> nrf_provisioning_sample: Waiting for modem to acquire network time...
	<inf> nrf_provisioning_sample: Network time obtained
	<inf> nrf_provisioning_http: Connected
	<inf> nrf_provisioning: Disconnected from network - provisioning paused
	<inf> nrf_provisioning: Provisioning done, rebooting...
	*** Booting Zephyr OS build zephyr-v3.3.0-3276-g4c765eaad495 ***

	<inf> nrf_provisioning_sample: Establishing LTE link ...
	<inf> nrf_provisioning_http: Connected
	<inf> nrf_provisioning: Disconnected from network - provisioning paused
	<inf> nrf_provisioning: Connected; home network - provisioning resumed
	<inf> nrf_provisioning_sample: Modem connection restored
	<inf> nrf_provisioning_sample: Waiting for modem to acquire network time...
	<inf> nrf_provisioning_sample: Network time obtained
	<inf> nrf_provisioning_http: Connected
	<inf> nrf_provisioning_http: No more commands to process on server side

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`lte_lc_readme`
* :ref:`modem_info_readme`
* :ref:`lib_nrf_provisioning`
