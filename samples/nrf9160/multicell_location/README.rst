.. _multicell_location:

nRF9160: Multicell location
###########################

.. contents::
   :local:
   :depth: 2

The Multicell location sample demonstrates how to use :ref:`lib_multicell_location` library to get a device's position based on LTE cell measurements.


Requirements
************

The sample supports the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: thingy91_nrf9160_ns, nrf9160dk_nrf9160_ns


.. include:: /includes/spm.txt


Overview
********

You can use this sample as a starting point to implement multicell location services in an application that needs the location of the device.

The sample acquires LTE cell information from :ref:`lte_lc_readme`.
The cell information is passed on to the :ref:`lib_multicell_location` library, where an HTTP request is generated and sent to the location service of choice.
Responses from location services are parsed and returned to the sample, which displays the responses on a terminal.

Currently, the sample can be used with the location services supported by the :ref:`lib_multicell_location` library, which are `nRF Cloud Location Services`_, `HERE Positioning`_ and `Skyhook Precision Location`_.
Note that nRF Cloud currently is a single-cell location service, and does not make use of neighboring cells in location resolution.
Before you use the services, see the :ref:`lib_multicell_location` library documentation and the respective location service documentation for the required setup.


Trigger location requests
*************************

Location requests can be triggered in the following ways and can be controlled by Kconfig options:

*  Pressing **Button 1**.
*  Periodically, with a configurable interval.
*  Changing of the current LTE cell, indicating that the device has moved.


Cell measurements
*****************

Data from LTE cell measurements is used to resolve a device's location.
There are two types of cells for which properties can be measured:

* Current cell to which the device is connected
* Neighboring cells

The most detailed information is measured for the current cell.
See :c:enumerator:`lte_lc_cells_info` in :ref:`lte_lc_readme` API for more details on the information that is available for the different cells.
:ref:`lte_lc_readme` library offers APIs to start and stop measurements, and the results are received in a callback.

Cell measurements are in general influenced by the following factors:

*  Network configuration
*  LTE connection state
*  Radio conditions

Cell measurement results might vary with the state of the LTE connection, and with the configurations of the LTE network that the device is connected to.
For instance, the ``timing advance`` property, which can be used to estimate the distance between a device and the LTE cell, is only valid and available in RRC connected mode.
Based on the network configurations, neighbor cell measurements might not be available in RRC connected mode.
In this case, the device must enter RRC idle mode to perform neighbor cell measurements.
This means that in some networks and configurations, it might not be possible to obtain timing advance for the current cell and neighbor cell measurements at the same time.

Multicell location requests are based on cell measurements.
While deciding on when to perform cell measurements, keep in mind the following factors:

*  Timing advance for the current cell is available in RRC connected mode and not in RRC idle mode.
*  Neighbor cell measurements might not be possible in RRC connected mode, depending on the network configuration.
*  Neighbor cell measurements can, with some exceptions, be done in RRC idle mode and PSM.
   The number of cells that can be found varies with network configuration.


Configuration
*************

|config|

Setup
=====

To use one of the supported location services, you must have an account and a configured authorization method.
Follow the documentation on `nRF Cloud`_, `HERE Positioning`_ or `Skyhook Precision Location`_ for account creation and authorization method details.


Configuration options
=====================

Check and configure the following configuration options for the sample:

.. options-from-kconfig::
   :show-type:
   :only-visible:


Additional configuration
========================

Check and configure the following library options that are used by the sample:


* :kconfig:`CONFIG_MULTICELL_LOCATION_SERVICE_NRF_CLOUD`
* :kconfig:`CONFIG_MULTICELL_LOCATION_SERVICE_HERE`
* :kconfig:`CONFIG_MULTICELL_LOCATION_SERVICE_SKYHOOK`

For the location service that is used, the authorization method can be set with one of the following options:

* :kconfig:`CONFIG_MULTICELL_LOCATION_NRF_CLOUD_API_KEY`
* :kconfig:`CONFIG_MULTICELL_LOCATION_HERE_API_KEY`
* :kconfig:`CONFIG_MULTICELL_LOCATION_SKYHOOK_API_KEY`

See :ref:`lib_multicell_location` for more information on the various configuration options that exist for the services.

Building and running
********************

.. |sample path| replace:: :file:`samples/nrf9160/multicell_location`

.. note::

   Before building the sample, you must configure a location provider and an API key as instructed in :ref:`lib_multicell_location`.

.. include:: /includes/build_and_run_nrf9160.txt


Testing
=======

|test_sample|

#. |connect_kit|
#. |connect_terminal|
#. Observe that the sample starts:

   .. code-block:: console

      *** Booting Zephyr OS build v2.4.99-ncs1-4938-g540567aae240  ***
      <inf> multicell_location_sample: Multicell location sample has started
      <inf> multicell_location: Provisioning certificate
      <inf> multicell_location_sample: Connecting to LTE network, this may take several minutes

#. Wait until an LTE connection is established. A successful LTE connection is indicated by the following entry in the log:

   .. code-block:: console

      <inf> multicell_location_sample: Network registration status: Connected - roaming

#. Press **Button 1** on the device to trigger a cell measurement and location request:

   .. code-block:: console

      <inf> multicell_location_sample: Button 1 pressed, starting cell measurements

#. Observe that cell measurements are displayed on the terminal:

   .. code-block:: console

      <inf> multicell_location_sample: Neighbor cell measurements received
      <inf> multicell_location_sample: Current cell:
      <inf> multicell_location_sample:     MCC: 242
      <inf> multicell_location_sample:     MNC: 001
      <inf> multicell_location_sample:     Cell ID: 1654712
      <inf> multicell_location_sample:     TAC: 3410
      <inf> multicell_location_sample:     EARFCN: 1650
      <inf> multicell_location_sample:     Timing advance: 65535
      <inf> multicell_location_sample:     Measurement time: 645008
      <inf> multicell_location_sample:     Physical cell ID: 292
      <inf> multicell_location_sample:     RSRP: 57
      <inf> multicell_location_sample:     RSRQ: 30
      <inf> multicell_location_sample: Neighbor cell 1
      <inf> multicell_location_sample:     EARFCN: 1650
      <inf> multicell_location_sample:     Time difference: -8960
      <inf> multicell_location_sample:     Physical cell ID: 447
      <inf> multicell_location_sample:     RSRP: 33
      <inf> multicell_location_sample:     RSRQ: -17
      <inf>multicell_location_sample: Neighbor cell 2
      <inf> multicell_location_sample:     EARFCN: 100
      <inf> multicell_location_sample:     Time difference: 24
      <inf> multicell_location_sample:     Physical cell ID: 447
      <inf> multicell_location_sample:     RSRP: 19
      <inf> multicell_location_sample:     RSRQ: 4
      <inf> multicell_location_sample: Neighbor cell 3
      <inf> multicell_location_sample:     EARFCN: 3551
      <inf> multicell_location_sample:     Time difference: 32
      <inf> multicell_location_sample:     Physical cell ID: 281
      <inf> multicell_location_sample:     RSRP: 41
      <inf> multicell_location_sample:     RSRQ: 13

#. Confirm that location request is sent, and that the response is received:

   .. code-block:: console

      <inf> multicell_location_sample: Sending location request...
      <inf> multicell_location_sample: Location obtained:
      <inf> multicell_location_sample:     Latitude: 63.4216744
      <inf> multicell_location_sample:     Longitude: 10.4373742
      <inf> multicell_location_sample:     Accuracy: 310

   The request might take a while to complete.

#. Observe that cell measurement and location request happen after the periodic interval has passed:

   .. code-block:: console

      <inf> multicell_location_sample: Periodical start of cell measurements

#. Observe that the sample continues with cell measurements and location requests as explained in the previous steps.


Dependencies
************

This sample uses the following |NCS| libraries and drivers:

* :ref:`lib_multicell_location`
* :ref:`lte_lc_readme`
* :ref:`dk_buttons_and_leds_readme`

It uses the following `sdk-nrfxlib`_ library:

* :ref:`nrfxlib:nrf_modem`

It uses the following Zephyr libraries:

* ``include/console.h``
* :ref:`zephyr:kernel_api`:

  * ``include/kernel.h``

In addition, it uses the following samples:

* :ref:`secure_partition_manager`
