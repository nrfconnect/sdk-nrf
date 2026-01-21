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

After the sample initializes and connects to the network, it sends a cell location request to nRF Cloud.
For this purpose, the sample uses network data obtained from the :ref:`lib_location` library.

See the `nRF Cloud Location Services documentation`_ page for additional information.

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

      *** Booting My Application v1.0.0-4ac881908aba ***
      *** Using nRF Connect SDK v3.1.99-4ac881908aba ***
      *** Using Zephyr OS v4.2.99-8116dbcc6173 ***
      [00:00:00.292,480] <inf> nrf_cloud_mqtt_cell_location_sample: nRF Cloud MQTT Cellular Location Sample, version: 1.0.0
      [00:00:00.630,432] <inf> nrf_cloud_mqtt_cell_location_sample: Connecting to LTE...
      +CGEV: EXCE STATUS 0
      +CEREG: 2,"816B","0338D100",7
      +CSCON: 1
      +CGEV: ME PDN ACT 0
      +CEREG: 1,"816B","0338D100",7,,,"11100000","11100000"
      [00:00:02.451,141] <inf> nrf_cloud_mqtt_cell_location_sample: Connected to network
      [00:00:02.451,324] <inf> nrf_cloud_mqtt_cell_location_sample: Waiting for current time
      %XTIME: "40","52115241146540","00"
      +CGEV: IPV6 0
      [00:00:03.364,898] <inf> nrf_cloud_fota_common: Saved job: , type: 7, validate: 0, bl: 0x0
      [00:00:03.477,264] <inf> nrf_cloud_info: Device ID: 7e699894-79b6-11f0-a2b4-db93a314a2aa
      [00:00:03.483,886] <inf> nrf_cloud_info: IMEI:      359400123456789
      [00:00:03.575,439] <inf> nrf_cloud_info: UUID:      7e699894-79b6-11f0-a2b4-db93a314a2aa
      [00:00:03.581,115] <inf> nrf_cloud_info: Modem FW:  mfw_nrf91x1_2.0.2-FOTA-TEST
      [00:00:03.581,146] <inf> nrf_cloud_info: Protocol:          MQTT
      [00:00:03.581,176] <inf> nrf_cloud_info: Download protocol: HTTPS
      [00:00:03.581,207] <inf> nrf_cloud_info: Sec tag:           2147483650
      [00:00:03.581,237] <inf> nrf_cloud_info: Host name:         mqtt.nrfcloud.com
      +CEREG: 1,"816B","030D7C00",7,,,"11100000","11100000"
      [00:00:07.560,516] <inf> nrf_cloud_log: Changing cloud log level from:1 to:3
      [00:00:07.560,546] <inf> nrf_cloud_log: Changing cloud logging enabled to:1
      [00:00:08.033,996] <inf> nrf_cloud_mqtt_cell_location_sample: Connection to nRF Cloud ready
      [00:00:08.034,118] <inf> nrf_cloud_mqtt_cell_location_sample: Requesting location with the default configuration...
      [00:00:08.034,637] <inf> nrf_cloud_info: Team ID:   73cae6fb-3fe2-4119-ba53-e094be327b3e
      %NCELLMEAS: 0,"030D7C00","24201","816B",95,6400,454,58,15,7426,6400,14,58,15,0,6400,173,53,5,0,6400,239,51,1,0,6400,255,51,1,0,7380
      %NCELLMEAS: 0,"030D7C00","24201","816B",95,7380,6400,454,56,11,7443,1,0
      %NCELLMEAS: 0,"030D7C00","24201","816B",95,7380,6400,454,58,15,7506,1,4,6400,14,58,15,0,6400,173,53,5,0,6400,239,51,1,0,6400,255,51,-1,0
      [00:00:08.764,373] <inf> nrf_cloud_mqtt_cell_location_sample: Cellular location request fulfilled with multi-cell
      [00:00:08.764,404] <inf> nrf_cloud_mqtt_cell_location_sample: Lat: 63.435616, Lon: 10.404310, Uncertainty: 1226 m
      [00:00:08.764,404] <inf> nrf_cloud_mqtt_cell_location_sample: Google maps URL: https://maps.google.com/?q=63.435616,10.404310
      +CSCON: 0


Troubleshooting
===============

If you are not getting the output similar to the one in `Testing`_, check the following potential issues:

The network carrier does not provide date and time
   The sample requires the network carrier to provide date and time to the modem.
   Without a valid date and time, the modem cannot generate JWTs with an expiration time.

Credentials are missing on the configured sec tag:

   .. code-block:: console

      [00:00:03.698,913] <err> nrf_cloud_transport: Could not connect to nRF Cloud MQTT Broker mqtt.nrfcloud.com, port: 8883. err: -111
      [00:00:03.698,974] <err> nrf_cloud_mqtt_cell_location_sample: NRF_CLOUD_EVT_TRANSPORT_CONNECT_ERROR: -9

Credentials are not registered or the device ID does not match the registered one:

   .. code-block:: console

      [00:00:08.287,078] <err> nrf_cloud_transport: MQTT input error: -128
      [00:00:08.287,139] <err> nrf_cloud_transport: Error disconnecting from cloud: -128
      [00:00:08.287,170] <err> nrf_cloud_mqtt_cell_location_sample: NRF_CLOUD_EVT_TRANSPORT_DISCONNECTED

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`lib_nrf_cloud`
* :ref:`lib_modem_jwt`
* :ref:`lte_lc_readme`
* :ref:`modem_info_readme`

In addition, it uses the following secure firmware component:

* :ref:`Trusted Firmware-M <ug_tfm>`
