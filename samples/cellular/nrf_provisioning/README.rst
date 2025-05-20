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

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

The sample requires that the device's private key is installed on the device and the associated device UUID is obtained from the Identity Service.

.. note::
   This sample requires modem firmware v2.0.0 or later.

Overview
********

The sample shows how the device performs the following actions:

* Connects to the nRF Cloud Provisioning Service.
* Retrieves the device-specific provisioning configuration.
* Decodes the received commands.
* Executes any AT commands, if present.
* Reports the results back to the server.
	If an error occurs, stops processing further commands and reports the error to the server.
* Sends a ``FINISHED`` response if all commands are executed successfully and ``FINISHED`` is one of the provisioning commands.

User interface
**************

No user interaction with the device is required.
Provisioning configuration must be defined on the server side.
Refer to `nRF Cloud device claiming`_ and `nRF Cloud provisioning configuration`_ for more details.

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
* :file:`overlay-large_cert.conf` - Adjusts buffer sizes to handle nRF Cloud certificates.

Building and running
********************

.. |sample path| replace:: :file:`samples/cellular/nrf_provisioning`

.. include:: /includes/build_and_run_ns.txt

Testing
=======

|test_sample|

#. |connect_kit|
#. |connect_terminal|
#. Add a provisioning configuration using the `nRF Cloud Provisioning Service <nRF Cloud provisioning configuration_>`_.
#. Power on or reset your device.
#. Observe that the sample starts and connects to the LTE network.
#. Observe that provisioning pauses and resumes while fetching and executing provisioning commands.

Sample output
=============

The following is an example output of the sample when there is no provisioning configuration on the server side:

.. code-block:: console

	<inf> nrf_provisioning_sample: Establishing LTE link ...
	<inf> nrf_provisioning_sample: Provisioning started
	<inf> nrf_provisioning_http: Requesting commands
	<inf> nrf_provisioning_http: Connected
	<inf> nrf_provisioning_http: No more commands to process on server side
	<inf> nrf_provisioning_sample: Provisioning stopped

The following is an example output when the sample is processing commands from the server:

.. code-block:: console

	<inf> nrf_provisioning_sample: Establishing LTE link ...
	<inf> nrf_provisioning_sample: Provisioning started
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
	<inf> nrf_provisioning_sample: Provisioning stopped
	<inf> nrf_provisioning_sample: Provisioning done, rebooting...
	<inf> nrf_provisioning: Disconnected from network - provisioning paused

Provisioning with the nRF Cloud Provisioning Service using auto-onboarding
==========================================================================

Auto-onboarding is the easiest method to provision and onboard devices to the nRF Cloud.

To enable support of large certificates when building the sample, add the ``-DEXTRA_CONF_FILE=overlay-large_cert.conf`` option to the build command.

Complete the following steps to provision your device:

1. |connect_kit|
#. |connect_terminal|
#. Enter command ``nrf_provisioning token`` to generate an `attestation token <nRF Cloud Generating attestation tokens_>`_ for the device
#. Copy the content of the response.
#. Log in to the `nRF Cloud`_ portal.
#. Select **Security Services** in the left sidebar.
   A panel opens to the right.
#. Select :guilabel:`Claimed Devices`.
#. Click :guilabel:`Claim Device`.
   A pop-up opens.
#. Copy and paste the attestation token into the **Claim token** text box.
#. Select an existing provisioning rule or create a new rule to auto-onboard the device during the claiming process.
#. Click :guilabel:`Claim Device`.
   The device is now claimed and an entry appears on the **Claimed Devices** page.
#. Open the Serial Terminal and enter command ``nrf_provisioning now`` to start the provisioning process.
   The device connects to the nRF Cloud Provisioning Service and retrieves the provisioning configuration.

Refer to `nRF Cloud device claiming`_ for more details on managing claimed devices.

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`lte_lc_readme`
* :ref:`lib_nrf_provisioning`

It uses the following `sdk-nrfxlib`_ library:

* :ref:`nrfxlib:nrf_modem`
