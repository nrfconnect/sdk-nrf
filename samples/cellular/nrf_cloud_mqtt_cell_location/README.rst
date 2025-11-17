.. _nrf_cloud_mqtt_cell_location:

Cellular: nRF Cloud MQTT cellular location
##########################################

.. contents::
   :local:
   :depth: 2

This sample demonstrates how to use the `nRF Cloud MQTT API`_ for nRF Cloud's cellular location service on your device.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

The sample requires an `nRF Cloud`_ account.
Your device must be onboarded to nRF Cloud.
If it is not, follow the instructions in `Device on-boarding <nrf_cloud_mqtt_cell_location_onboarding>`_.

Overview
********

The sample can work in two modes: single-cell mode and multi-cell mode.

After the sample initializes and connects to the network, it enters the single-cell mode and sends a single-cell location request to nRF Cloud.
For this purpose, the sample uses network data obtained from the :ref:`modem_info_readme` library.
In the multi-cell mode, the sample requests for neighbor cell measurement using the :ref:`lte_lc_readme` library.

If the modem provides neighbor cell data, the sample sends a multi-cell location request to nRF Cloud.
Otherwise, the request is single-cell.

In either mode, the sample sends a new location request if a change in the cell ID is detected.

See the `nRF Cloud Location Services documentation`_ for additional information.

User interface
**************

Button 1:
   Toggle between the single-cell and multi-cell mode.

.. _nrf_cloud_mqtt_cell_location_onboarding:

Setup
=====

You must onboard your device to nRF Cloud for this sample to function.
You only need to do this once for each device.
To onboard your device, install `nRF Cloud Utils`_ and follow the instructions in the README.

Configuration
*************

|config|

Configuration options
=====================

Check and configure the following Kconfig options for the sample:

.. options-from-kconfig::
   :show-type:

.. include:: /libraries/modem/nrf_modem_lib/nrf_modem_lib_trace.rst
   :start-after: modem_lib_sending_traces_UART_start
   :end-before: modem_lib_sending_traces_UART_end

Building and running
********************

.. |sample path| replace:: :file:`samples/cellular/nrf_cloud_mqtt_cell_location`

.. include:: /includes/build_and_run_ns.txt

Testing
=======

|test_sample|

#. |connect_kit|
#. |connect_terminal|
#. Reset the development kit.
#. Observe in the terminal window that the application starts.
   This is indicated by output similar to the following (there is also a lot of additional information about the LTE connection):

   .. code-block:: console

      *** Booting nRF Connect SDK v3.1.99-11052a0a30aa ***
      *** Using Zephyr OS v4.2.99-9a699b5802cb ***
      Attempting to boot slot 0.
      Attempting to boot from address 0x8200.
      I: Trying to get Firmware version
      I: Verifying signature against key 0.
      I: Hash: 0x3e...f9
      I: Firmware signature verified.
      Firmware version 2
      �[00:00:00.254,821] <inf> spi_nor: GD25LE255E@0: 32 MiBy flash
      *** Booting My Application v1.0.0-11052a0a30aa ***
      *** Using nRF Connect SDK v3.1.99-11052a0a30aa ***
      *** Using Zephyr OS v4.2.99-9a699b5802cb ***
      [00:00:00.293,273] <inf> nrf_cloud_mqtt_cell_location: nRF Cloud MQTT Cellular Location Sample, version: 1.0.0
      [00:00:00.725,555] <inf> nrf_cloud_mqtt_cell_location: Device ID: 7e699894-79b6-11f0-a2b4-db93a314a2aa
      [00:00:00.726,196] <inf> nrf_cloud_mqtt_cell_location: Using LTE LC neighbor search type GCI extended complete for 5 cells
      [00:00:01.026,397] <inf> nrf_cloud_credentials: Sec Tag: 16842753; CA: Yes, Client Cert: Yes, Private Key: Yes
      [00:00:01.026,428] <inf> nrf_cloud_credentials: CA Size: 1824, AWS: Likely, CoAP: Likely
      [00:00:01.026,458] <inf> nrf_cloud_mqtt_cell_location: nRF Cloud credentials detected!
      [00:00:01.026,489] <inf> nrf_cloud_mqtt_cell_location: Enabling connectivity...
      +CGEV: EXCE STATUS 0
      +CEREG: 2,"8169","014A0305",7
      %MDMEV: PRACH CE-LEVEL 0
      +CSCON: 1
      [00:00:02.237,792] <inf> nrf_cloud_mqtt_cell_location: RRC mode: connected
      +CGEV: ME PDN ACT 0
      %MDMEV: SEARCH STATUS 2
      +CEREG: 1,"8169","014A0305",7,,,"11100000","11100000"
      [00:00:03.007,202] <inf> nrf_cloud_mqtt_cell_location: Connected to network
      %XTIME: "40","52118131101140","00"
      +CGEV: IPV6 0
      [00:00:03.611,328] <inf> nrf_cloud_fota_common: Saved job: , type: 7, validate: 0, bl: 0x0
      [00:00:03.635,589] <inf> nrf_cloud_info: Device ID: 7e699894-79b6-11f0-a2b4-db93a314a2aa
      [00:00:03.644,775] <inf> nrf_cloud_info: IMEI:      359400123456789
      [00:00:03.747,497] <inf> nrf_cloud_info: UUID:      7e699894-79b6-11f0-a2b4-db93a314a2aa
      [00:00:03.748,168] <inf> nrf_cloud_info: Modem FW:  mfw_nrf91x1_2.0.2
      [00:00:03.748,199] <inf> nrf_cloud_info: Protocol:          MQTT
      [00:00:03.748,229] <inf> nrf_cloud_info: Download protocol: HTTPS
      [00:00:03.748,260] <inf> nrf_cloud_info: Sec tag:           16842753
      [00:00:03.748,291] <inf> nrf_cloud_info: Host name:         mqtt.nrfcloud.com
      [00:00:03.748,321] <inf> nrf_cloud_mqtt_cell_location: Connecting to nRF Cloud
      [00:00:06.037,445] <inf> nrf_cloud_log: Changing cloud log level from:1 to:3
      [00:00:06.037,475] <inf> nrf_cloud_log: Changing cloud logging enabled to:1
      [00:00:06.498,901] <inf> nrf_cloud_mqtt_cell_location: Connection to nRF Cloud ready
      [00:00:06.499,420] <inf> nrf_cloud_info: Team ID:   73cae6fb-3fe2-4119-ba53-e094be327b3e
      [00:00:06.534,606] <inf> nrf_cloud_mqtt_cell_location: Device status sent to nRF Cloud
      [00:00:06.534,637] <inf> nrf_cloud_mqtt_cell_location: Getting current cell info...
      [00:00:06.565,093] <inf> nrf_cloud_mqtt_cell_location: Current cell info: Cell ID: 21627653, TAC: 33129, MCC: 242, MNC: 1
      [00:00:06.565,093] <inf> nrf_cloud_mqtt_cell_location: Performing single-cell request
      [00:00:06.565,124] <inf> nrf_cloud_mqtt_cell_location: Request configuration:
      [00:00:06.565,155] <inf> nrf_cloud_mqtt_cell_location:   High confidence interval   = false
      [00:00:06.565,216] <inf> nrf_cloud_mqtt_cell_location:   Fallback to rough location = true
      [00:00:06.565,246] <inf> nrf_cloud_mqtt_cell_location:   Reply with result          = true
      [00:00:07.177,429] <inf> nrf_cloud_mqtt_cell_location: Cellular location request fulfilled with single-cell
      [00:00:07.177,459] <inf> nrf_cloud_mqtt_cell_location: Lat: 63.423117, Lon: 10.436068, Uncertainty: 1618 m

Troubleshooting
===============

If you are not getting the output similar to the one in `Testing`_, check the following potential issues:

The network carrier does not provide date and time
   The sample requires the network carrier to provide date and time to the modem.
   Without a valid date and time, the modem cannot generate JWTs with an expiration time.

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`lib_nrf_cloud`
* :ref:`lib_modem_jwt`
* :ref:`lte_lc_readme`
* :ref:`modem_info_readme`
* :ref:`dk_buttons_and_leds_readme`

In addition, it uses the following secure firmware component:

* :ref:`Trusted Firmware-M <ug_tfm>`
