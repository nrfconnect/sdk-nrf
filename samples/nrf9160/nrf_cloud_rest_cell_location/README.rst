.. _nrf_cloud_rest_cell_pos_sample:

nRF9160: nRF Cloud REST cellular location
#########################################

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
It requires one of the following:

* A device provisioned and associated with an `nRF Cloud`_ account.
  The sample supports JITP provisioning through REST.
* A private key installed on the device and its associated public key registered with an `nRF Cloud`_ account.

.. note::
   This sample requires modem firmware v1.3.x or later.

Overview
********

After the sample initializes and connects to the network, it enters single-cell mode and sends a single-cell location request to nRF Cloud using network data obtained from the :ref:`modem_info_readme` library.
To enable multi-cell mode, press **button 1**.
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

If you have the option :ref:`CONFIG_REST_CELL_LOCATION_DO_JITP <CONFIG_REST_CELL_LOCATION_DO_JITP>` enabled and you press **button 1** when prompted at startup, it requests for just-in-time provisioning (JITP) with nRF Cloud through REST.
This is useful when initially provisioning and associating a device on nRF Cloud.
You only need to do this once for each device.

If you have not requested for JITP before starting up your device, the sample asks if the location card on the nRF Cloud portal should be enabled in the device's shadow.
This is only valid for provisioned devices.
Press **button 1** to perform the shadow update.
The location card displays the device's location on a map.
This is not required for the sample to function.
You only need to do this once for each device.

After the sample completes any requested JITP or shadow updates, pressing the **button 1** toggles between single-cell and multi-cell mode.

Configuration
*************
|config|


Configuration options
=====================

Check and configure the following Kconfig options for the sample:

.. _CONFIG_REST_CELL_LOCATION_DO_JITP:

CONFIG_REST_CELL_LOCATION_DO_JITP - Enable prompt to perform JITP via REST
   This configuration option defines whether the application prompts the user for just-in-time provisioning on startup.

.. include:: /libraries/modem/nrf_modem_lib/nrf_modem_lib_trace.rst
   :start-after: modem_lib_sending_traces_UART_start
   :end-before: modem_lib_sending_traces_UART_end

Building and running
********************

.. |sample path| replace:: :file:`samples/nrf9160/nrf_cloud_rest_cell_location`

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
