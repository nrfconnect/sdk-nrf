.. _nrf_cloud_coap_cell_location:

Cellular: nRF Cloud CoAP cellular location
##########################################

.. contents::
   :local:
   :depth: 2

This sample demonstrates how to use the `nRF Cloud CoAP API`_ for nRF Cloud's cellular location service on your device.

.. note::

   This sample focuses on API usage. It is recommended to use the :ref:`lib_location` library to gather cell information in practice.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

The sample requires an `nRF Cloud`_ account.
Your device must be onboarded to nRF Cloud.
If it is not, follow the instructions in `Device on-boarding <nrf_cloud_coap_cell_location_onboarding>`_.

.. note::
   This sample requires modem firmware v1.3.x or later for an nRF9160 DK, or modem firmware v2.x.x for the nRF9161 and nRF9151 DKs.

Overview
********

The sample can work in two modes: single-cell mode and multi-cell mode.

After the sample initializes and connects to the network, it enters the single-cell mode and sends a single-cell location request to nRF Cloud.
For this purpose, the sample uses network data obtained from the :ref:`modem_info_readme` library.
In the multi-cell mode, the sample requests for neighbor cell measurement using the :ref:`lte_lc_readme` library.

If the modem provides neighbor cell data, the sample sends a multi-cell location request to nRF Cloud.
Otherwise, the request is single-cell.

In either mode, the sample sends a new location request if a change in cell ID is detected.

See the `nRF Cloud Location Services documentation`_ for additional information.

User interface
**************

Button 1:
   Toggle between the single-cell and multi-cell mode.

.. _nrf_cloud_coap_cell_location_onboarding:

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

.. _CONFIG_COAP_CELL_CHANGE_CONFIG:

CONFIG_COAP_CELL_CHANGE_CONFIG - Enable changing request configuration
   Set this to use the next combination of ``hi_conf`` and ``fallback`` flags after performing both single- and multi-cell location requests.

.. _CONFIG_COAP_CELL_DEFAULT_DOREPLY_VAL:

CONFIG_COAP_CELL_DEFAULT_DOREPLY_VAL - Enable return of location from cloud
   If enabled, request the cloud to return the location information.

.. _CONFIG_COAP_CELL_DEFAULT_FALLBACK_VAL:

CONFIG_COAP_CELL_DEFAULT_FALLBACK_VAL - Enable fallback to coarse location
   If enabled and the location of the cell tower or Wi-Fi® access points cannot be found, return area-level location based on the cellular tracking area code.
   Otherwise an error will be returned indicating location is not known.

.. _CONFIG_COAP_CELL_DEFAULT_HICONF_VAL:

CONFIG_COAP_CELL_DEFAULT_HICONF_VAL - Enable high-confidence result
   Enable a 95% confidence interval for the location, instead of the default 68%.

.. _CONFIG_COAP_CELL_SEND_DEVICE_STATUS:

CONFIG_COAP_CELL_SEND_DEVICE_STATUS - Send device status
   Send device status to nRF Cloud on initial connection.

.. include:: /libraries/modem/nrf_modem_lib/nrf_modem_lib_trace.rst
   :start-after: modem_lib_sending_traces_UART_start
   :end-before: modem_lib_sending_traces_UART_end

Building and running
********************

.. |sample path| replace:: :file:`samples/cellular/nrf_cloud_coap_cell_location`

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

      *** Booting nRF Connect SDK v3.0.99-065f86a42eaa ***
      *** Using Zephyr OS v4.1.99-82973b4f6d2a ***
      Attempting to boot slot 0.
      Attempting to boot from address 0x8200.
      I: Trying to get Firmware version
      I: Verifying signature against key 0.
      I: Hash: 0x3e...f9
      I: Firmware signature verified.
      Firmware version 2
      �[00:00:00.254,821] <inf> spi_nor: GD25LE255E@0: 32 MiBy flash
      *** Booting My Application v1.0.0-065f86a42eaa ***
      *** Using nRF Connect SDK v3.0.99-065f86a42eaa ***
      *** Using Zephyr OS v4.1.99-82973b4f6d2a ***
      [00:00:00.292,907] <inf> nrf_cloud_coap_cell_location_sample: nRF Cloud CoAP Cellular Location Sample, version: 1.0.0
      [00:00:00.715,637] <inf> nrf_cloud_coap_cell_location_sample: Device ID: 7e699894-79b6-11f0-a2b4-db93a314a2aa
      [00:00:00.721,313] <inf> nrf_cloud_coap_cell_location_sample: Using LTE LC neighbor search type GCI extended complete for 5 cells
      [00:00:01.023,040] <inf> nrf_cloud_credentials: Sec Tag: 16842753; CA: Yes, Client Cert: Yes, Private Key: Yes
      [00:00:01.023,101] <inf> nrf_cloud_credentials: CA Size: 1824, AWS: Likely, CoAP: Likely
      [00:00:01.023,101] <inf> nrf_cloud_coap_cell_location_sample: nRF Cloud credentials detected!
      [00:00:01.023,132] <inf> nrf_cloud_coap_cell_location_sample: Enabling connectivity...
      +CGEV: EXCE STATUS 0
      +CEREG: 2,"0910","020A6B0C",7
      %MDMEV: PRACH CE-LEVEL 0
      +CSCON: 1
      [00:00:02.995,330] <inf> nrf_cloud_coap_cell_location_sample: RRC mode: connected
      +CGEV: ME PDN ACT 0,0
      +CNEC_ESM: 50,0
      %MDMEV: SEARCH STATUS 2
      +CEREG: 5,"0910","020A6B0C",7,,,"11100000","11100000"
      [00:00:07.840,423] <inf> nrf_cloud_coap_cell_location_sample: Connected to network
      [00:00:07.840,515] <inf> nrf_cloud_coap_cell_location_sample: Waiting for modem to acquire network time...
      [00:00:10.841,247] <inf> nrf_cloud_coap_cell_location_sample: Network time obtained
      [00:00:10.841,308] <inf> nrf_cloud_info: Device ID: 7e699894-79b6-11f0-a2b4-db93a314a2aa
      [00:00:10.841,857] <inf> nrf_cloud_info: IMEI:      359400123456789
      [00:00:10.941,680] <inf> nrf_cloud_info: UUID:      7e699894-79b6-11f0-a2b4-db93a314a2aa
      [00:00:10.948,211] <inf> nrf_cloud_info: Modem FW:  mfw_nrf91x1_2.0.2
      [00:00:10.948,242] <inf> nrf_cloud_info: Protocol:          CoAP
      [00:00:10.948,272] <inf> nrf_cloud_info: Download protocol: HTTPS
      [00:00:10.948,303] <inf> nrf_cloud_info: Sec tag:           16842753
      [00:00:10.948,303] <inf> nrf_cloud_info: CoAP JWT Sec tag:  16842753
      [00:00:10.948,333] <inf> nrf_cloud_info: Host name:         coap.nrfcloud.com
      [00:00:12.721,099] <inf> nrf_cloud_coap_transport: Request authorization with JWT
      [00:00:12.986,877] <inf> nrf_cloud_coap_transport: Authorization result_code: 2.01
      [00:00:12.986,999] <inf> nrf_cloud_coap_transport: Authorized
      [00:00:12.987,213] <inf> nrf_cloud_coap_transport: DTLS CID is active
      [00:00:13.716,003] <inf> nrf_cloud_coap_cell_location_sample: Device status sent to nRF Cloud
      [00:00:13.716,033] <inf> nrf_cloud_coap_cell_location_sample: Getting current cell info...
      [00:00:13.725,494] <inf> nrf_cloud_coap_cell_location_sample: Current cell info: Cell ID: 34237196, TAC: 2320, MCC: 242, MNC: 2
      [00:00:13.725,524] <inf> nrf_cloud_coap_cell_location_sample: Performing single-cell request
      [00:00:13.725,524] <inf> nrf_cloud_coap_cell_location_sample: Request configuration:
      [00:00:13.725,555] <inf> nrf_cloud_coap_cell_location_sample:   High confidence interval   = false
      [00:00:13.725,616] <inf> nrf_cloud_coap_cell_location_sample:   Fallback to rough location = true
      [00:00:13.725,646] <inf> nrf_cloud_coap_cell_location_sample:   Reply with result          = true
      [00:00:13.991,180] <inf> nrf_cloud_coap_cell_location_sample: Cellular location request fulfilled with single-cell
      [00:00:13.991,210] <inf> nrf_cloud_coap_cell_location_sample: Lat: 63.423740, Lon: 10.435896, Uncertainty: 1056 m

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
* :ref:`lib_nrf_cloud_coap`
* :ref:`lib_modem_jwt`
* :ref:`lte_lc_readme`
* :ref:`modem_info_readme`
* :ref:`dk_buttons_and_leds_readme`

In addition, it uses the following secure firmware component:

* :ref:`Trusted Firmware-M <ug_tfm>`
