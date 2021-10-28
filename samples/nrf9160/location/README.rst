.. _location_sample:

.. |sample path| replace:: :file:`samples/nrf9160/location`

nRF9160: Location
#################

.. contents::
   :local:
   :depth: 2

The Location sample demonstrates how to retrieve device's position using selected methods, such as GNSS, cellular positioning or WiFi positioning.
This is achieved utilizing :ref:`lib_location`.

Requirements
************

The sample supports the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: thingy91_nrf9160_ns, nrf9160dk_nrf9160_ns

.. include:: /includes/spm.txt

See also the requirements in :ref:`lib_location`.

Overview
********

The Location sample retrieves the location multiple times to illustrate different ways of retrieving devices location.
This is done simply by implementing each individual location request in a separate function within the sample, and those functions are then called in the main().
In addition to :ref:`lib_location`, this sample uses :ref:`lte_lc_readme` to control LTE connection.

Configuration
*************

|config|

Setup*
======

.. note::
   Add information about the initial setup here, for example, that the user must install or enable some library before they can compile this sample, or set up and select a specific backend.
   Most samples do not need this section.

Configuration options*
======================

.. note::
   * You do not need to list all configuration options of the sample, but only the ones that are important for the sample and make sense for the user to know about.
   * The syntax below allows sample configuration options to link to the option descriptions in the same way as the library configuration options link to the corresponding Kconfig descriptions (``:kconfig:`SAMPLE_CONFIG```, which results in :kconfig:`SAMPLE_CONFIG`).
   * For each configuration option, list the symbol name and the string describing it.

Check and configure the following configuration options for the sample:

.. option:: SAMPLE_CONFIG - Sample configuration

   The sample configuration defines ...

.. option:: ANOTHER_CONFIG - Another configuration

   This configuration option specifies ...

Additional configuration*
=========================

.. note::
   * Add this section to describe and link to any library configuration options that might be important to run this sample.
     You can link to options with ``:kconfig:`CONFIG_XXX```.
   * You do not need to list all possible configuration options, but only the ones that are relevant.

Check and configure the following library options that are used by the sample:

* :kconfig:`CONFIG_BT_DEVICE_NAME` - Defines the Bluetooth® device name.
* :kconfig:`CONFIG_BT_LBS` - Enables the :ref:`lbs_readme`.

Configuration files*
====================

The sample provides predefined configuration files for typical use cases.
You can find the configuration files in the |sample path| directory.

The following files are available:

* :file:`esp_8266_nrf9160ns.overlay` - DTC overlay for ESP8266 WiFi chip support.
* :file:`overlay-esp-wifi.conf` - Config overlay for ESP8266 WiFi chip support.
* :file:`overlay-pgps.conf` - Config overlay for P-GPS support.

Building and running
********************

.. note::

   Before building the sample, you must configure a location provider and an API key as instructed in :ref:`lib_location`.

.. include:: /includes/build_and_run_nrf9160.txt

ESP8266 WiFi support
====================

To build the Location sample with ESP8266 WiFi chip support, use the ``-DDTC_OVERLAY_FILE=esp_8266_nrf9160ns.overlay -DOVERLAY_CONFIG="overlay-esp-wifi.conf"`` options.
For example:

``west build -p -b nrf9160dk_nrf9160ns -- -DDTC_OVERLAY_FILE=esp_8266_nrf9160ns.overlay -DOVERLAY_CONFIG="overlay-esp-wifi.conf"``

See :ref:`cmake_options` for more instructions on how to add this option.

P-GPS support
=============

To build the Location sample with P-GPS support, use the ``-DOVERLAY_CONFIG="overlay-pgps.conf"`` options.
For example:

``west build -p -b nrf9160dk_nrf9160ns -- -DOVERLAY_CONFIG="overlay-pgps.conf"``

See :ref:`cmake_options` for more instructions on how to add this option.

Testing
=======

|test_sample|

#. |connect_kit|
#. |connect_terminal|
#. Observe that the sample prints to the terminal

Sample output
=============

Below you'll see an example output of the sample::

   .. code-block:: console

      Location sample started

      Connecting to LTE...
      Connected to LTE

      Requesting location with short GNSS timeout to trigger fallback to cellular...
      [00:00:06.481,262] <wrn> location: Timeout occurred
      [00:00:06.487,335] <wrn> location: Failed to acquire location using 'GNSS', trying with 'Cellular' next
      Got location:
      method: cellular
      latitude: 12.887095
      longitude: 55.580397
      accuracy: 12500.0 m
      Google maps URL: https://maps.google.com/?q=12.887095,55.580397

      Requesting location with the default configuration...
      Got location:
      method: GNSS
      latitude: 12.893736
      longitude: 55.575859
      accuracy: 4.4 m
      date: 2021-10-28
      time: 13:36:29.072 UTC
      Google maps URL: https://maps.google.com/?q=12.893736,55.575859

      Requesting location with high GNSS accuracy...
      Got location:
      method: GNSS
      latitude: 12.893755
      longitude: 55.575879
      accuracy: 2.8 m
      date: 2021-10-28
      time: 13:36:32.339 UTC
      Google maps URL: https://maps.google.com/?q=12.893755,55.575879

      Requesting 30s periodic GNSS location...
      Got location:
      method: GNSS
      latitude: 12.893765
      longitude: 55.575912
      accuracy: 4.4 m
      date: 2021-10-28
      time: 13:36:33.536 UTC
      Google maps URL: https://maps.google.com/?q=12.893765,55.575912

      Got location:
      method: GNSS
      latitude: 12.893892
      longitude: 55.576090
      accuracy: 8.4 m
      date: 2021-10-28
      time: 13:37:04.685 UTC
      Google maps URL: https://maps.google.com/?q=12.893892,55.576090

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`lib_location`
* :ref:`lte_lc_readme`
* :ref:`lib_date_time`

In addition, it uses the following samples:

* :ref:`secure_partition_manager`
