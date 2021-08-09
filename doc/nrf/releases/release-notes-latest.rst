.. _ncs_release_notes_latest:

Changes in |NCS| v1.6.99
########################

.. contents::
   :local:
   :depth: 2

The most relevant changes that are present on the master branch of the |NCS|, as compared to the latest release, are tracked in this file.

.. note::
   This file is a work in progress and might not cover all relevant changes.

Highlights
**********

There are no entries for this section yet.

Known issues
************

Known issues are only tracked for the latest official release.
See `known issues for nRF Connect SDK v1.6.1`_ for the list of issues valid for this release.

Changelog
*********

The following sections provide detailed lists of changes by component.

nRF9160
=======

* Updated:

  * :ref:`lib_nrf_cloud` library:

    * Added function :c:func:`nrf_cloud_uninit`, which can be used to uninitialize the nRF Cloud library.  If :ref:`cloud_api_readme` is used, call :c:func:`cloud_uninit`
    * Added function :c:func:`nrf_cloud_shadow_device_status_update`, which sets the device status in the device's shadow.
    * Added function :c:func:`nrf_cloud_modem_info_json_encode`, which encodes modem information into a cJSON object formatted for use with nRF Cloud.
    * Added function :c:func:`nrf_cloud_service_info_json_encode`, which encodes service information into a cJSON object formatted for use with nRF Cloud.
    * Removed function ``nrf_cloud_sensor_attach()``, the associated structure ``nrf_cloud_sa_param``, and event ``NRF_CLOUD_EVT_SENSOR_ATTACHED``. These items provided no useful functionality.

  * :ref:`serial_lte_modem` application:

    * Added a separate document page to explain data mode mechanism and how it works.
    * Removed datatype in all sending AT commands. If no sending data is specified, switch data mode to receive and send any arbitrary data.
    * Added a separate document page to describe the FOTA service.
    * Added IPv6 support to Socket and ICMP services.

  * :ref:`asset_tracker_v2` application:

    * Changed the custom module responsible for controlling the LEDs to CAF LEDs module.

nRF5
====

The following changes are relevant for the nRF52 and nRF53 Series.

nRF Desktop
-----------

* Settings backend changed from FCB to NVS.

Bluetooth LE
------------

* Updated:

  * :ref:`ble_samples` - Changed the Bluetooth sample Central DFU SMP name to :ref:`Central SMP Client <bluetooth_central_dfu_smp>`.

Matter
------

* Added:

  * :ref:`Weather station <matter_weather_station_sample>` sample.
  * :ref:`Template <matter_template_sample>` sample with a guide about :ref:`ug_matter_creating_accessory`.

Zigbee
------

* Added:

  * :ref:`lib_zigbee_zcl_scenes` library with documentation.
    This library was separated from the Zigbee light bulb sample.

Common
======

The following changes are relevant for all device families.

Pelion
------

* Updated Pelion Device Management Client library version to 4.10.0.

MCUboot
=======

The MCUboot fork in |NCS| (``sdk-mcuboot``) contains all commits from the upstream MCUboot repository up to and including ``2fce9769b1``, plus some |NCS| specific additions.

The code for integrating MCUboot into |NCS| is located in :file:`ncs/nrf/modules/mcuboot`.

The following list summarizes the most important changes inherited from upstream MCUboot:

* No changes yet


Mcumgr
======

The mcumgr library contains all commits from the upstream mcumgr repository up to and including snapshot ``74e77ad08``.

The following list summarizes the most important changes inherited from upstream mcumgr:

* No changes yet

Zephyr
======

.. NOTE TO MAINTAINERS: The latest Zephyr commit appears in multiple places; make sure you update them all.

The Zephyr fork in |NCS| (``sdk-zephyr``) contains all commits from the upstream Zephyr repository up to and including ``730acbd6ed`` (``v2.6.0-rc1``), plus some |NCS| specific additions.

For a complete list of upstream Zephyr commits incorporated into |NCS| since the most recent release, run the following command from the :file:`ncs/zephyr` repository (after running ``west update``):

.. code-block:: none

   git log --oneline v2.6.0-rc1 ^v2.4.99-ncs1

For a complete list of |NCS| specific commits, run:

.. code-block:: none

   git log --oneline manifest-rev ^v2.6.0-rc1

The current |NCS| release is based on Zephyr v2.6.0-rc1.
See the :ref:`zephyr:zephyr_2.6` Release Notes for an overview of the most important changes inherited from upstream Zephyr.

Matter (Project CHIP)
=====================

The Matter fork in the |NCS| (``sdk-connectedhomeip``) contains all commits from the upstream Matter repository up to, and including, ``b77bfb047374b7013dbdf688f542b9326842a39e``.

The following list summarizes the most important changes inherited from the upstream Matter:

* Added:

  * Support for Certificate-Authenticated Session Establishment (CASE) for communication among operational Matter nodes.
  * Support for OpenThread's DNS Client to enable Matter node discovery on Thread devices.

Documentation
=============

There are no entries for this section yet.
