.. _nrf_cloud_coap_fota_sample:

Cellular: nRF Cloud CoAP FOTA
#############################

.. contents::
   :local:
   :depth: 2

The nRF Cloud CoAP FOTA sample demonstrates how to use the `nRF Cloud CoAP API`_ to perform Firmware Over-the-Air (FOTA) updates over CoAP on your device.
This covers modem, application, and full modem FOTA updates (FMFU).
Also, with the nRF9160 DK, it supports SMP FOTA updates to the firmware on the nRF52840 SoC.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

The sample requires an `nRF Cloud`_ account.

Your device must be onboarded to nRF Cloud.
If it is not, follow the instructions in `Device on-boarding <nrf_cloud_coap_fota_sample_onboarding_>`_.

.. note::
   This sample requires modem firmware v1.3.x or later for an nRF9160 SiP and v2.0.0 or later for nRF9161 and nRF9151 SiPs.

.. include:: /includes/external_flash_nrf91.txt

.. note::
   Full modem FOTA requires development kit version 0.14.0 or higher if you are using an nRF9160 DK.

Overview
********

You can update your device firmware on the `nRF Cloud`_ portal or directly through the `nRF Cloud CoAP API`_.
See the `nRF Cloud Getting Started FOTA documentation`_ for details.

Limitations
***********

This sample requires the network carrier to provide date and time to the modem.
Without a valid date and time, the modem cannot generate JSON Web Tokens (JWT) with an expiration time.

User interface
**************

Button 1:
   Check for a FOTA update right away without waiting for the timeout.

.. _nrf_cloud_coap_fota_sample_onboarding:

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

Check and configure the following configuration options for the sample:

.. _CONFIG_COAP_FOTA_JOB_CHECK_RATE_MIN:

CONFIG_COAP_FOTA_JOB_CHECK_RATE_MIN - Update check rate
   Defines how often the sample checks for FOTA updates.
   You can modify this value at runtime by adding or updating the ``"fotaInterval"`` item in the desired config section of the device's shadow.
   Use the `nRF Cloud`_ portal or the REST API to perform the config update.

.. _CONFIG_COAP_FOTA_LTE_LED_NUM:

CONFIG_COAP_FOTA_LTE_LED_NUM - LTE LED number
   Defines the LED used to indicate the connection to the LTE network.

.. _CONFIG_COAP_FOTA_BUTTON_EVT_NUM:

CONFIG_COAP_FOTA_BUTTON_EVT_NUM - Button number
   Defines the button to use for a manual FOTA update check.

.. include:: /libraries/modem/nrf_modem_lib/nrf_modem_lib_trace.rst
   :start-after: modem_lib_sending_traces_UART_start
   :end-before: modem_lib_sending_traces_UART_end

Building and running
********************

.. |sample path| replace:: :file:`samples/cellular/nrf_cloud_coap_fota`

.. include:: /includes/build_and_run_ns.txt

The configuration files for this sample are located in the :file:`samples/cellular/nrf_cloud_coap_fota` folder.
See :ref:`configure_application` on how to configure the parameters.

To create a FOTA test version of this sample, change the ``PATCHLEVEL`` in the :file:`VERSION` file.

To enable full modem FOTA, add the following parameter to your build command:

``-DEXTRA_CONF_FILE=full_modem_fota.conf``

Also, if you are using an nRF9160 DK, specify your development kit version by appending it to the board name.
For example, if you are using version 1.0.1, use the board name ``nrf9160dk@1.0.1/nrf9160/ns`` in your build command.

To enable SMP FOTA (nRF9160 DK only), add the following parameters to your build command:

* ``-DEXTRA_CONF_FILE=smp_fota.conf``
* ``-DEXTRA_DTC_OVERLAY_FILE=nrf9160dk_mcumgr_client_uart2.overlay``

Once you have flashed your nRF9160 DK, change the switch **SW10** to the **nRF52** position to be able to flash the nRF52840 firmware on the DK.
The nRF52840 device on your DK must be running firmware compatible with SMP, such as the :ref:`smp_svr` sample.
Otherwise, the CoAP FOTA sample cannot connect to the nRF52840 and will keep trying to connect.
Build the :ref:`smp_svr` sample for the ``nrf9160dk/nrf52840`` board with the following parameters:

* ``-DEXTRA_CONF_FILE=overlay-serial.conf``
* ``-DEXTRA_DTC_OVERLAY_FILE=nrf9160dk_nrf52840_mcumgr_svr.overlay``

To change :ref:`smp_svr` sample's application version, set the :kconfig:option:`CONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION` Kconfig option.

Testing
=======

|test_sample|

#. |connect_kit|
#. |connect_terminal|
#. Reset the development kit.
#. Observe in the terminal window that the application starts.
   This is indicated by output similar to the following (there is also a lot of additional information about the LTE connection):

   .. code-block:: console

      *** Booting nRF Connect SDK v3.1.99-7ff1f906b7b0 ***
      *** Using Zephyr OS v4.1.99-2cc69ef97a5c ***
      Attempting to boot slot 0.
      Attempting to boot from address 0x8200.
      I: Trying to get Firmware version
      I: Verifying signature against key 0.
      I: Hash: 0x3e...f9
      I: Firmware signature verified.
      Firmware version 2
      I: Setting monotonic counter (version: 2, slot: 0)
      �[00:00:00.254,821] <inf> spi_nor: GD25LE255E@0: 32 MiBy flash
      *** Booting My Application v1.0.0-7ff1f906b7b0 ***
      *** Using nRF Connect SDK v3.1.99-7ff1f906b7b0 ***
      *** Using Zephyr OS v4.1.99-2cc69ef97a5c ***
      [00:00:00.311,981] <inf> nrf_cloud_fota_sample: nRF Cloud FOTA Sample, version: 1.0.0
      [00:00:00.689,270] <inf> nrf_cloud_fota_common: Saved job: , type: 7, validate: 0, bl: 0x0
      [00:0:00.699,066] <inf> nrf_cloud_fota_sample: Application Name: N/A
      [00:00:00.705,902] <inf> nrf_cloud_fota_sample: nRF Connect SDK version: 3.1.99-7ff1f906b7b0
      [00:00:00.912,902] <inf> nrf_cloud_credentials: Sec Tag: 16842753; CA: Yes, Client Cert: Yes, Private Key: Yes
      [00:00:00.923,309] <inf> nrf_cloud_credentials: CA Size: 1824, AWS: Likely, CoAP: Likely
      [00:00:00.931,854] <inf> nrf_cloud_fota_sample: nRF Cloud credentials detected!
      [00:00:00.939,605] <inf> nrf_cloud_fota_sample: Enabling connectivity...
      +CGEV: EXCE STATUS 0
      +CEREG: 2,"0901","020A7716",7
      %MDMEV: PRACH CE-LEVEL 0
      +CSCON: 1
      +CGEV: ME PDN ACT 0,0
      +CNEC_ESM: 50,0
      %MDMEV: SEARCH STATUS 2
      +CEREG: 5,"0901","020A7716",7,,,"11100000","11100000"
      [00:00:05.376,922] <inf> nrf_cloud_fota_sample: Connected to LTE
      [00:00:05.415,954] <inf> nrf_cloud_fota_sample: Waiting for modem to acquire network time...
      [00:00:08.425,659] <inf> nrf_cloud_fota_sample: Network time obtained
      [00:00:08.432,525] <inf> nrf_cloud_info: Device ID: 7e699894-79b6-11f0-a2b4-db93a314a2aa
      [00:00:08.447,601] <inf> nrf_cloud_info: IMEI:      359400123456789
      [00:00:08.545,837] <inf> nrf_cloud_info: UUID:      7e699894-79b6-11f0-a2b4-db93a314a2aa
      [00:00:08.560,821] <inf> nrf_cloud_info: Modem FW:  mfw_nrf91x1_2.0.2
      [00:00:08.567,657] <inf> nrf_cloud_info: Protocol:          CoAP
      [00:00:08.574,096] <inf> nrf_cloud_info: Download protocol: CoAP
      [00:00:08.580,535] <inf> nrf_cloud_info: Sec tag:           16842753
      [00:00:08.587,341] <inf> nrf_cloud_info: CoAP JWT Sec tag:  16842753
      [00:00:08.594,146] <inf> nrf_cloud_info: Host name:         coap.nrfcloud.com
      [00:00:10.981,414] <inf> nrf_cloud_coap_transport: Request authorization with JWT
      [00:00:11.336,212] <inf> nrf_cloud_coap_transport: Authorization result_code: 2.01
      [00:00:11.344,299] <inf> nrf_cloud_coap_transport: Authorized
      [00:00:11.350,616] <inf> nrf_cloud_coap_transport: DTLS CID is active
      [00:00:12.139,984] <inf> nrf_cloud_fota_sample: Sending device status...
      [00:00:12.565,460] <inf> nrf_cloud_fota_sample: FOTA enabled in device shadow
      [00:00:12.572,998] <inf> nrf_cloud_fota_poll: Checking for FOTA job...
      [00:00:13.205,413] <inf> nrf_cloud_fota_poll: No pending FOTA job
      [00:00:13.211,975] <inf> nrf_cloud_fota_sample: Checking for shadow delta...
      [00:00:13.554,901] <inf> nrf_cloud_log: Changing cloud log level from:1 to:3
      [00:00:13.562,347] <inf> nrf_cloud_log: Changing cloud logging enabled to:1
      [00:00:14.165,435] <inf> nrf_cloud_fota_sample: Desired interval: 5.000000
      [00:00:14.541,442] <inf> nrf_cloud_fota_sample: Updated shadow delta sent
      [00:00:14.548,645] <inf> nrf_cloud_fota_sample: Checking for FOTA job in 5 minute(s) or when button 1 is pressed

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`lib_nrf_cloud`
* :ref:`lib_nrf_cloud_coap`
* :ref:`lib_modem_jwt`
* :ref:`lte_lc_readme`
* :ref:`dk_buttons_and_leds_readme`
* :ref:`modem_info_readme`
* :ref:`lib_at_host`

In addition, it uses the following secure firmware component:

* :ref:`Trusted Firmware-M <ug_tfm>`
