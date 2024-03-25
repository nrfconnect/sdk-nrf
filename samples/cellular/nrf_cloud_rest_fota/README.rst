.. _nrf_cloud_rest_fota:

Cellular: nRF Cloud REST FOTA
#############################

.. contents::
   :local:
   :depth: 2

The REST FOTA sample demonstrates how to use the `nRF Cloud REST API`_ to perform Firmware Over-the-Air (FOTA) updates on your device.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

The sample requires an `nRF Cloud`_ account and modem firmware v1.3.x or later for an nRF9160 DK, or modem firmware v2.x.x for the nRF91x1 DKs.

.. note::
   Full modem FOTA requires development kit version 0.14.0 or higher if you are using an nRF9160 DK.

.. include:: /includes/external_flash_nrf91.txt

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

If you want to perform an update check immediately, press the button configured by the Kconfig option :ref:`CONFIG_REST_FOTA_BUTTON_EVT_NUM <CONFIG_REST_FOTA_BUTTON_EVT_NUM>`.
This will bypass the wait time specified by the :ref:`CONFIG_REST_FOTA_JOB_CHECK_RATE_MIN <CONFIG_REST_FOTA_JOB_CHECK_RATE_MIN>` option.

If you have the option :ref:`CONFIG_REST_FOTA_DO_JITP <CONFIG_REST_FOTA_DO_JITP>` enabled and you press the button configured by the :ref:`CONFIG_REST_FOTA_BUTTON_EVT_NUM <CONFIG_REST_FOTA_BUTTON_EVT_NUM>` option when prompted at startup, it will request just-in-time provisioning (JITP) through REST with nRF Cloud.
This is useful when initially provisioning and associating a device on nRF Cloud.
You only need to do this once for each device.

If you have enabled the :ref:`CONFIG_REST_FOTA_ENABLE_LED <CONFIG_REST_FOTA_ENABLE_LED>` option, an LED configured by the :ref:`CONFIG_REST_FOTA_LED_NUM <CONFIG_REST_FOTA_LED_NUM>` option indicates the state of the connection to the LTE network.

Configuration
*************
|config|

Configuration options
=====================

Check and configure the following configuration options for the sample:

.. _CONFIG_REST_FOTA_JOB_CHECK_RATE_MIN:

CONFIG_REST_FOTA_JOB_CHECK_RATE_MIN - Update check rate
   This configuration option defines how often the sample checks for FOTA updates.

.. _CONFIG_REST_FOTA_DL_TIMEOUT_MIN:

CONFIG_REST_FOTA_DL_TIMEOUT_MIN - Download timeout value
   This configuration option defines how long to wait for a download to complete.

.. _CONFIG_REST_FOTA_ENABLE_LED:

CONFIG_REST_FOTA_ENABLE_LED - Enable LED
   This configuration option defines if the LED will be used to indicate connection to the LTE network.

.. _CONFIG_REST_FOTA_LED_NUM:

CONFIG_REST_FOTA_LED_NUM - Enable LED
   This configuration option defines if the LED is to be used.

.. _CONFIG_REST_FOTA_BUTTON_EVT_NUM:

CONFIG_REST_FOTA_BUTTON_EVT_NUM - Button number
   This configuration option defines the button to use for device interactions.

.. _CONFIG_REST_FOTA_DO_JITP:

CONFIG_REST_FOTA_DO_JITP - Enable prompt to perform JITP via REST
   This configuration option defines if the application will prompt the user for just-in-time provisioning on startup.

.. include:: /libraries/modem/nrf_modem_lib/nrf_modem_lib_trace.rst
   :start-after: modem_lib_sending_traces_UART_start
   :end-before: modem_lib_sending_traces_UART_end

Building and running
********************

.. |sample path| replace:: :file:`samples/cellular/nrf_cloud_rest_fota`

.. include:: /includes/build_and_run_ns.txt

The configuration file for this sample is located in :file:`samples/cellular/nrf_cloud_rest_fota`.
See :ref:`configure_application` for information on how to configure the parameters.

To create a FOTA test version of this sample, add the following parameter to your build command:

``-DEXTRA_CONF_FILE=overlay_fota_test.conf``

To enable full modem FOTA, add the following parameter to your build command:

``-DEXTRA_CONF_FILE=overlay_full_modem_fota.conf``

Also, if you are using an nRF9160 DK, specify your development kit version by appending it to the board name.
For example, if you are using version 1.0.1, use the board name ``nrf9160dk@1.0.1/nrf9160/ns`` in your build command.

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
