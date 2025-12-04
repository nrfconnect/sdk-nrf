.. _nrf_cloud_rest_cell_location:

Cellular: nRF Cloud REST cellular location
##########################################

.. contents::
   :local:
   :depth: 2

This sample demonstrates how to use the `nRF Cloud REST API`_ for nRF Cloud's cellular location service on your device.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

The sample requires an `nRF Cloud`_ account.
Your device must be onboarded to nRF Cloud.
If it is not, follow the instructions in `Device on-boarding <nrf_cloud_rest_cell_location_onboarding>`_.

Overview
********

After the sample initializes and connects to the network, it sends a cell location request to nRF Cloud.
For this purpose, the sample uses network data obtained from the :ref:`lib_location` library.

See the `nRF Cloud Location Services documentation`_ page for additional information.

.. _nrf_cloud_rest_cell_location_onboarding:

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

.. |sample path| replace:: :file:`samples/cellular/nrf_cloud_rest_cell_location`

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

      *** Booting nRF Connect SDK v3.1.99-34fcb6d9589a ***
      *** Using Zephyr OS v4.2.99-de567402f088 ***
      Attempting to boot slot 0.
      Attempting to boot from address 0x8200.
      I: Trying to get Firmware version
      I: Verifying signature against key 0.
      I: Hash: 0x3e...f9
      I: Firmware signature verified.
      Firmware version 2
      �[00:00:00.254,516] <inf> spi_nor: GD25LE255E@0: 32 MiBy flash
      *** Booting My Application v1.0.0-34fcb6d9589a ***
      *** Using nRF Connect SDK v3.1.99-34fcb6d9589a ***
      *** Using Zephyr OS v4.2.99-de567402f088 ***
      [00:00:00.309,265] <inf> nrf_cloud_rest_cell_location_sample: nRF Cloud REST Cellular Location Sample, version: 1.0.0
      [00:00:00.652,099] <inf> nrf_cloud_rest_cell_location_sample: Connecting to LTE...
      +CEREG: 2,"8169","014A0302",7
      +CSCON: 1
      +CEREG: 1,"8169","014A0302",7,,,"00000100","00101000"
      [00:00:02.620,635] <inf> nrf_cloud_rest_cell_location_sample: Connected to network
      [00:00:02.628,692] <inf> nrf_cloud_rest_cell_location_sample: Waiti%XTIME: "40","52114261635340","00"
      ng for current time
      [00:00:02.640,441] <inf> nrf_cloud_rest_cell_location_sample: Requesting location with the default configuration...
      %NCELLMEAS: 0,"014A0302","24201","8169",80,6400,106,47,9,1081,6400,247,51,16,0,6400,128,45,5,0,1967
      %NCELLMEAS: 0,"014A0302","24201","8169",80,1967,6400,106,50,22,1971,1,0
      %NCELLMEAS: 0,"014A0302","24201","8169",80,1967,6400,106,50,22,1971,1,0
      [00:00:02.803,710] <inf> nrf_cloud_info: Device ID: 7e699894-79b6-11f0-a2b4-db93a314a2aa
      [00:00:02.812,713] <inf> nrf_cloud_info: IMEI:      359400123456789
      [00:00:02.937,774] <inf> nrf_cloud_info: UUID:      7e699894-79b6-11f0-a2b4-db93a314a2aa
      [00:00:02.946,655] <inf> nrf_cloud_info: Modem FW:  mfw_nrf91x1_2.0.2-FOTA-TEST
      [00:00:02.954,376] <inf> nrf_cloud_info: Protocol:          REST
      [00:00:02.960,815] <inf> nrf_cloud_info: Download protocol: HTTPS
      [00:00:02.967,346] <inf> nrf_cloud_info: Sec tag:           2147483650
      [00:00:02.974,334] <inf> nrf_cloud_info: Host name:         api.nrfcloud.com
      +CEREG: 1,"8169","014ACE00",7,,,"00000100","00101000"
      [00:00:16.267,608] <inf> nrf_cloud_rest_cell_location_sample: Cellular location request fulfilled with multi-cell
      [00:00:16.278,289] <inf> nrf_cloud_rest_cell_location_sample: Lat: 63.407496, Lon: 10.452719, Uncertainty: 1367 m
      [00:00:16.289,001] <inf> nrf_cloud_rest_cell_location_sample: Google maps URL: https://maps.google.com/?q=63.407496,10.452719
      +CSCON: 0
      +CEREG: 1,"8169","014A0305",7,,,"00000100","00101000"

Troubleshooting
===============

If you are not getting the output similar to the one in `Testing`_, check the following potential issues:

The network carrier does not provide date and time
   The sample requires the network carrier to provide date and time to the modem.
   Without a valid date and time, the modem cannot generate JWTs with an expiration time.

Credentials are missing on the configured sec tag:

   .. code-block:: console

      [00:00:02.738,159] <err> nrf_cloud_jwt: Failed to generate JWT, error: -8
      [00:00:02.745,574] <err> nrf_cloud_rest: Failed to auto-generate JWT, error: -8
      [00:00:02.753,601] <err> nrf_cloud_rest: Could not format HTTP auth header, err: -8
      [00:00:02.761,962] <err> nrf_cloud_rest_cell_location_sample: Request failed, error: -8

Credentials are not registered or the device ID does not match the registered one:

   .. code-block:: console

      [00:00:07.062,957] <err> nrf_cloud_codec_internal: REST error msg: Auth token is malformed.
      [00:00:07.071,990] <err> nrf_cloud_rest: nRF Cloud REST error code: 40100
      [00:00:07.079,437] <err> nrf_cloud_rest: REST request was rejected. Response status: 401
      [00:00:07.088,226] <err> nrf_cloud_rest: Response body: {"message":"Auth token is malformed.","code":40100}
      [00:00:07.098,846] <err> nrf_cloud_rest_cell_location_sample: Request failed, error: -77
      [00:00:07.107,574] <err> nrf_cloud_rest_cell_location_sample: nRF Cloud error code: 40100

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`lib_nrf_cloud`
* :ref:`lib_nrf_cloud_rest`
* :ref:`lib_modem_jwt`
* :ref:`lte_lc_readme`
* :ref:`modem_info_readme`

In addition, it uses the following secure firmware component:

* :ref:`Trusted Firmware-M <ug_tfm>`
