.. _nrf_cloud_rest_fota:

Cellular: nRF Cloud REST FOTA
#############################

.. contents::
   :local:
   :depth: 2

.. note::

   The :ref:`lib_nrf_cloud_rest` library has been deprecated and it will be removed in one of the future releases.
   Use the :ref:`nrf_cloud_coap_fota_sample` sample instead.

The REST FOTA sample demonstrates how to use the `nRF Cloud REST API`_ to perform Firmware Over-the-Air (FOTA) updates over REST on your device.
This covers modem, application, and full modem FOTA updates (FMFU).
Also, with the nRF9160 DK, it supports SMP FOTA updates.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

The sample requires an `nRF Cloud`_ account.

Your device must be onboarded to nRF Cloud.
If it is not, follow the instructions in `Device on-boarding <nrf_cloud_rest_fota_onboarding_>`_.

.. note::
   This sample requires modem firmware v1.3.x or later for an nRF9160 SiP and v2.0.0 or later for nRF91x1 SiPs.

.. include:: /includes/external_flash_nrf91.txt

.. note::
   Full modem FOTA requires development kit version 0.14.0 or higher if you are using an nRF9160 DK.

Overview
********

You can update your device firmware on the `nRF Cloud`_ portal or directly through the `nRF Cloud REST API`_.
See the `nRF Cloud Getting Started FOTA documentation`_ for details.

Limitations
***********

This sample requires the network carrier to provide date and time to the modem.
Without a valid date and time, the modem cannot generate JWTs with an expiration time.

User interface
**************

Once the device is onboarded and connected, if you want to perform an update check immediately, press **Button 1**.
This will bypass the wait time specified by the :ref:`CONFIG_REST_FOTA_JOB_CHECK_RATE_MIN <CONFIG_REST_FOTA_JOB_CHECK_RATE_MIN>` option.
**Button 1** is configured by the :ref:`CONFIG_REST_FOTA_BUTTON_EVT_NUM <CONFIG_REST_FOTA_BUTTON_EVT_NUM>` option.

The configured LTE LED (**LED 1** by default, :ref:`CONFIG_REST_FOTA_LTE_LED_NUM <CONFIG_REST_FOTA_LTE_LED_NUM>`) is lit once an LTE connection is established.

.. _nrf_cloud_rest_fota_onboarding:

Onboarding your device to nRF Cloud
***********************************

You must onboard your device to nRF Cloud for this sample to function.
You only need to do this once for each device.

To onboard your device, install `nRF Cloud Utils`_ and follow the instructions in the README.

Configuration
*************
|config|

Configuration options
=====================

Check and configure the following configuration options for the sample:

.. _CONFIG_REST_FOTA_JOB_CHECK_RATE_MIN:

CONFIG_REST_FOTA_JOB_CHECK_RATE_MIN - Update check rate
   Defines how often the sample checks for FOTA updates.
   You can modify this value at runtime by adding or updating the ``"fotaInterval"`` item in the desired config section of the device's shadow.
   Use the `nRF Cloud`_ portal or the REST API to perform the config update.

.. _CONFIG_REST_FOTA_LTE_LED_NUM:

CONFIG_REST_FOTA_LTE_LED_NUM - LTE LED number
   Defines the LED used to indicate connection to the LTE network.

.. _CONFIG_REST_FOTA_BUTTON_EVT_NUM:

CONFIG_REST_FOTA_BUTTON_EVT_NUM - Button number
   Defines the button to use for manual FOTA update check.

.. include:: /libraries/modem/nrf_modem_lib/nrf_modem_lib_trace.rst
   :start-after: modem_lib_sending_traces_UART_start
   :end-before: modem_lib_sending_traces_UART_end

Building and running
********************

.. |sample path| replace:: :file:`samples/cellular/nrf_cloud_rest_fota`

.. include:: /includes/build_and_run_ns.txt

The configuration files for this sample are located in :file:`samples/cellular/nrf_cloud_rest_fota`.
See :ref:`configure_application` for information on how to configure the parameters.

To create a FOTA test version of this sample, add the following parameter to your build command:

``-DEXTRA_CONF_FILE=overlay_fota_test.conf``

To enable full modem FOTA, add the following parameter to your build command:

``-DEXTRA_CONF_FILE=overlay_full_modem_fota.conf``

Also, if you are using an nRF9160 DK, specify your development kit version by appending it to the board name.
For example, if you are using version 1.0.1, use the board name ``nrf9160dk@1.0.1/nrf9160/ns`` in your build command.

To enable SMP FOTA (nRF9160 DK only), add the following parameters to your build command:

* ``-DEXTRA_CONF_FILE=overlay_smp_fota.conf``
* ``-DEXTRA_DTC_OVERLAY_FILE=nrf9160dk_mcumgr_client_uart2.overlay``

Once the nRF9160 has been flashed, change the switch **SW10** to the nRF52 position to be able to flash the nRF52840 on the DK.
The nRF52840 device on your DK must be running an SMP compatible firmware, such as the :ref:`smp_svr` sample.
Otherwise, the REST FOTA sample will not be able to connect to the nRF52840 and will keep trying to connect.
Build the :ref:`smp_svr` sample for the ``nrf9160dk/nrf52840`` board with the following parameters:

* ``-DEXTRA_CONF_FILE=overlay-serial.conf``
* ``-DEXTRA_DTC_OVERLAY_FILE=nrf9160dk_nrf52840_mcumgr_svr.overlay``

To change :ref:`smp_svr` sample's application version, set the :kconfig:option:`CONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION` Kconfig option.

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`lib_nrf_cloud`
* :ref:`lib_nrf_cloud_rest`
* :ref:`lib_modem_jwt`
* :ref:`lte_lc_readme`
* :ref:`dk_buttons_and_leds_readme`
* :ref:`modem_info_readme`
* :ref:`lib_at_host`

In addition, it uses the following secure firmware component:

* :ref:`Trusted Firmware-M <ug_tfm>`
