.. _nrf_cloud_rest_cell_location:

Cellular: nRF Cloud REST cellular location
##########################################

.. contents::
   :local:
   :depth: 2

.. note::

   The :ref:`lib_nrf_cloud_rest` library has been deprecated and it will be removed in one of the future releases.
   Use the :ref:`nrf_cloud_coap_cell_location` sample instead.

This sample demonstrates how to use the `nRF Cloud REST API`_ for nRF Cloud's cellular location service on your device.

.. note::

   This sample focuses on API usage. It is recommended to use the :ref:`lib_location` library to gather cell information in practice.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

The sample requires an `nRF Cloud`_ account.
Your device must be onboarded to nRF Cloud.
If it is not, follow the instructions in `Device on-boarding <nrf_cloud_rest_location_onboarding_>`_.

.. note::
   This sample requires modem firmware v1.3.x or later for an nRF9160 DK, or modem firmware v2.x.x for the nRF91x1 DKs.

Overview
********

After the sample initializes and connects to the network, it enters single-cell mode and sends a single-cell location request to nRF Cloud using network data obtained from the :ref:`modem_info_readme` library.
To enable multi-cell mode, press **Button 1**.
In multi-cell mode, the sample requests for neighbor cell measurement using the :ref:`lte_lc_readme` library.
If neighbor cell data is measured, the sample sends a multi-cell location request to nRF Cloud.
Otherwise, the request is single-cell.
In either mode, the sample sends a new location request if a change in cell ID is detected.

See the `nRF Cloud Location Services documentation`_ for additional information.

Limitations
***********

The sample requires the network carrier to provide date and time to the modem.
Without a valid date and time, the modem cannot generate JWTs with an expiration time.

User interface
**************

Pressing **Button 1** toggles between single-cell and multi-cell mode.

Set the :ref:`CONFIG_REST_CELL_CHANGE_CONFIG <CONFIG_REST_CELL_CHANGE_CONFIG>` Kconfig config value to try all combinations of the :c:struct:`nrf_cloud_location_config` structure values ``hi_conf`` and ``fallback``.

.. _nrf_cloud_rest_location_onboarding:

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

Check and configure the following Kconfig options for the sample:

.. _CONFIG_REST_CELL_CHANGE_CONFIG:

CONFIG_REST_CELL_CHANGE_CONFIG - Enable changing request configuration
   Set this to use the next combination of ``hi_conf`` and ``fallback`` flags after performing both single- and multi-cell location requests.

.. _CONFIG_REST_CELL_DEFAULT_DOREPLY_VAL:

CONFIG_REST_CELL_DEFAULT_DOREPLY_VAL - Enable return of location from cloud
   If enabled, request the cloud to return the location information.

.. _CONFIG_REST_CELL_DEFAULT_FALLBACK_VAL:

CONFIG_REST_CELL_DEFAULT_FALLBACK_VAL - Enable fallback to coarse location
   If enabled and the location of the cell tower or Wi-Fi® access points cannot be found, return area-level location based on the cellular tracking area code.
   Otherwise an error will be returned indicating location is not known.

.. _CONFIG_REST_CELL_DEFAULT_HICONF_VAL:

CONFIG_REST_CELL_DEFAULT_HICONF_VAL - Enable high confidence result
   Enable a 95% confidence interval for the location, instead of the default 68%.

.. _CONFIG_REST_CELL_SEND_DEVICE_STATUS:

CONFIG_REST_CELL_SEND_DEVICE_STATUS - Send device status
   Send device status to nRF Cloud on initial connection.

.. include:: /libraries/modem/nrf_modem_lib/nrf_modem_lib_trace.rst
   :start-after: modem_lib_sending_traces_UART_start
   :end-before: modem_lib_sending_traces_UART_end

Building and running
********************

.. |sample path| replace:: :file:`samples/cellular/nrf_cloud_rest_cell_location`

.. include:: /includes/build_and_run_ns.txt

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`lib_nrf_cloud`
* :ref:`lib_nrf_cloud_rest`
* :ref:`lib_modem_jwt`
* :ref:`lte_lc_readme`
* :ref:`modem_info_readme`
* :ref:`app_event_manager`
* :ref:`caf_overview`

In addition, it uses the following secure firmware component:

* :ref:`Trusted Firmware-M <ug_tfm>`
