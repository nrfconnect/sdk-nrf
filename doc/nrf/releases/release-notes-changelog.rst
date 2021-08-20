.. _ncs_release_notes_changelog:

Changelog for |NCS| v1.6.99
###########################

.. contents::
   :local:
   :depth: 2

The most relevant changes that are present on the master branch of the |NCS|, as compared to the latest official release, are tracked in this file.

.. note::
   This file is a work in progress and might not cover all relevant changes.

Known issues
************

Known issues are only tracked for the latest official release.
See `known issues for nRF Connect SDK v1.6.1`_ for the list of issues valid for this release.

Changelog
*********

The following sections provide detailed lists of changes by component.

nRF9160
=======

* Added:

  * :ref:`at_monitor_readme` library:

    * A library to reschedule and dispatch AT notifications from the Modem library AT interface to AT monitors.

  * :ref:`at_monitor_sample` sample:

    * A sample to show the usage of the :ref:`at_monitor_readme` library.

  * :ref:`lib_nrf_cloud_rest` library:

    * The library enables devices to use nRF Cloud's REST-based device API.

* Updated:

  * :ref:`lib_nrf_cloud` library:

    * Added function :c:func:`nrf_cloud_uninit`, which can be used to uninitialize the nRF Cloud library.  If :ref:`cloud_api_readme` is used, call :c:func:`cloud_uninit`
    * Added function :c:func:`nrf_cloud_shadow_device_status_update`, which sets the device status in the device's shadow.
    * Added function :c:func:`nrf_cloud_modem_info_json_encode`, which encodes modem information into a cJSON object formatted for use with nRF Cloud.
    * Added function :c:func:`nrf_cloud_service_info_json_encode`, which encodes service information into a cJSON object formatted for use with nRF Cloud.
    * Added function :c:func:`nrf_cloud_client_id_get`, which returns the client ID used to identify the device with nRF Cloud.
    * Added function :c:func:`nrf_cloud_tenant_id_get`, which returns the tenant ID used to identify a customer account with nRF Cloud.
    * Added function :c:func:`nrf_cloud_register_gateway_state_handler` to implement a custom callback on shadow update events.
    * Added Kconfig option :kconfig:`CONFIG_NRF_CLOUD_GATEWAY`, which enables functionality to behave as an nRF Cloud gateway.
    * Removed function ``nrf_cloud_sensor_attach()``, the associated structure ``nrf_cloud_sa_param``, and event ``NRF_CLOUD_EVT_SENSOR_ATTACHED``. These items provided no useful functionality.
    * Added the option to use the P-GPS API independent of nRF Cloud MQTT transport.

  * :ref:`serial_lte_modem` application:

    * Added a separate document page to explain data mode mechanism and how it works.
    * Removed datatype in all sending AT commands. If no sending data is specified, switch data mode to receive and send any arbitrary data.
    * Added a separate document page to describe the FOTA service.
    * Added IPv6 support to all SLM services.
    * Added the GNSS service to replace the existing GPS test functionality.
    * Added the optional support of location services from nRF Cloud, like A-GPS, P-GPS, and cellular positioning.

  * :ref:`asset_tracker_v2` application:

    * Changed the custom module responsible for controlling the LEDs to CAF LEDs module.
    * Added support for A-GPS when configuring the application for AWS IoT.
    * Added support for P-GPS when configuring the application for AWS IoT.

  * :ref:`at_cmd_readme` library:

    * The library has been deprecated in favor of Modem library's native AT interface.

  * :ref:`at_notif_readme` library:

    * The library has been deprecated in favor of the :ref:`at_monitor_readme` library.

  * Board names:

    * The ``nrf9160dk_nrf9160ns`` and the ``nrf5340dk_nrf5340_cpuappns`` boards have been renamed respectively to ``nrf9160dk_nrf9160_ns`` and ``nrf5340dk_nrf5340_cpuapp_ns``, in a change inherited from upstream Zephyr.
    * The ``thingy91_nrf9160ns`` board has been renamed to ``thingy91_nrf9160_ns`` for consistency with the changes inherited from upstream Zephyr.

nRF5
====

The following changes are relevant for the nRF52 and nRF53 Series.

Front-end module (FEM)
----------------------

* Added support for nRF21540 to nRF5340 network core.
* Added support for RF front-end Modules (FEM) for nRF5340 in :ref:`mpsl` library. The front-end module feature for nRF5340 in MPSL currently supports nRF21540, but does not support SKY66112-11 device.

nRF Desktop
-----------

* Settings backend changed from FCB to NVS.

Bluetooth LE
------------

* Updated:

  * :ref:`ble_samples` - Changed the Bluetooth® sample Central DFU SMP name to :ref:`Central SMP Client <bluetooth_central_dfu_smp>`.
  * :ref:`direction_finding_connectionless_rx` and :ref:`direction_finding_connectionless_tx` samples - Added default configuration for ``nrf52833dk_nrf52820`` and ``nrf5340dk_nrf5340_cpuapp``, and ``nrf5340dk_nrf5340_cpuapp_ns`` boards.

Matter
------

* Added:

  * :ref:`Thngy:53 Weather station <matter_weather_station_app>` application.
  * :ref:`Template <matter_template_sample>` sample with a guide about :ref:`ug_matter_creating_accessory`.
  * :ref:`ug_matter_tools` page with information about building options for Matter controllers.
  * PA/LNA GPIO interface support for RF front-end modules (FEM) in Matter.

Zigbee
------

* Added:

  * :ref:`lib_zigbee_zcl_scenes` library with documentation.
    This library was separated from the Zigbee light bulb sample.
  * Added production support for :ref:`radio front-end module (FEM) <ug_radio_fem>` for nRF52 Series devices and nRF21540 EK.
  * :ref:`zigbee_template_sample` sample.
    This minimal Zigbee router application can be used as the starting point for developing custom Zigbee devices.
  * Added API for vendor-specific NCP commands.
  * Added API for Zigbee command for getting active nodes.

* Updated:

  * Fixed the KRKNWK-9743 known issue where the timer could not be stopped in Zigbee routers and coordinators.
  * Fixed the KRKNWK-10490 known issue that would cause a deadlock in the NCP frame fragmentation logic.
  * Fixed the KRKNWK-6071 known issue with inaccurate ZBOSS alarms.
  * Fixed the KRKNWK-5535 known issue where the device would assert if flooded with multiple Network Address requests.
  * Fixed an issue where the NCS would assert in the host application when the host started just after SoC's SysReset.

* Updated:

  * :ref:`zigbee_ug_logging_stack_logs` - Improved printing ZBOSS stack logs.
    Added new backend options to print ZBOSS stack logs with option for using binary format.

Common
======

The following changes are relevant for all device families.

sdk-nrfxlib
-----------

See the changelog for each library in the :doc:`nrfxlib documentation <nrfxlib:README>` for additional information.

Modem library
+++++++++++++

* Updated :ref:`nrf_modem` to version 1.3.0.
  See the :ref:`nrfxlib:nrf_modem_changelog` for detailed information.
* Added a new API for AT commands.
  See :ref:`nrfxlib:nrf_modem_at` for more information.
* Added a new API for modem delta firmware updates.
  See :ref:`nrfxlib:nrf_modem_delta_dfu` for more information.

* The AT socket API is now deprecated.
* The DFU socket API is now deprecated.

Pelion
------

* Updated Pelion Device Management Client library version to 4.10.0.

MCUboot
=======

.. TODO update this following https://github.com/nrfconnect/sdk-nrf/pull/5189. Delete this comment once that is complete.

The MCUboot fork in |NCS| (``sdk-mcuboot``) contains all commits from the upstream MCUboot repository up to and including ``2fce9769b1``, plus some |NCS| specific additions.

The code for integrating MCUboot into |NCS| is located in :file:`ncs/nrf/modules/mcuboot`.

The following list summarizes the most important changes inherited from upstream MCUboot:

* No changes yet


Mcumgr
======

.. TODO update this following https://github.com/nrfconnect/sdk-nrf/pull/5189. Delete this comment once that is complete.

The mcumgr library contains all commits from the upstream mcumgr repository up to and including snapshot ``74e77ad08``.

The following list summarizes the most important changes inherited from upstream mcumgr:

* No changes yet

Zephyr
======

.. NOTE TO MAINTAINERS: All the Zephyr commits in the below git commands must be handled specially after each upmerge and each NCS release.

The Zephyr fork in |NCS| (``sdk-zephyr``) contains all commits from the upstream Zephyr repository up to and including ``14f09a3b00``, plus some |NCS| specific additions.

For a complete list of upstream Zephyr commits incorporated into |NCS| since the most recent release, run the following command from the :file:`ncs/zephyr` repository (after running ``west update``):

.. code-block:: none

   git log --oneline 14f09a3b00 ^v2.6.0-rc1-ncs1

For a complete list of |NCS| specific commits, run:

.. code-block:: none

   git log --oneline manifest-rev ^14f09a3b00

The current |NCS| master branch is based on the Zephyr v2.7 development branch.

The following list summarizes the most important changes inherited from upstream Zephyr:

.. TODO update the following sections to reflect https://github.com/nrfconnect/sdk-nrf/pull/5189. Delete this comment once that is complete.

* Arches/Boards:

.. TODO

* Bluetooth:

.. TODO

* Devicetree:

.. TODO

* Documentation:

.. TODO

* Drivers:

.. TODO

* Kernel:

.. TODO

* Networking:

.. TODO

* Testing:

.. TODO

* Other:

.. TODO

  * A config option for ``memcpy`` that skips the word-based loop before the byte-based loop was added.
    It is now enabled by default if :kconfig:`SIZE_OPTIMIZATIONS` is set.
    As result, any application-specific assumptions about ``memcpy`` read or write size behavior should be rechecked if this option is enabled.

Matter (Project CHIP)
=====================

The Matter fork in the |NCS| (``sdk-connectedhomeip``) contains all commits from the upstream Matter repository up to, and including, ``b77bfb047374b7013dbdf688f542b9326842a39e``.

The following list summarizes the most important changes inherited from the upstream Matter:

* Added:

  * Support for Certificate-Authenticated Session Establishment (CASE) for communication among operational Matter nodes.
  * Support for OpenThread's DNS Client to enable Matter node discovery on Thread devices.
  * Fixed the known issue KRKNWK-10387 where Matter service was needlessly advertised over Bluetooth LE during DFU.
    Now if Matter pairing mode is not opened and the Bluetooth LE advertising is needed due to DFU requirements, only the SMP service is advertised.

Documentation
=============

* Added:

  * User guide :ref:`ug_nrf_cloud`.

.. TODO update this following https://github.com/nrfconnect/sdk-nrf/pull/5189. Delete this comment once that is complete.

* Updated:

  * Renamed :ref:`ncs_release_notes_changelog` (this page).
  * :ref:`gs_installing` - added information about the version folder created when extracting the GNU Arm Embedded Toolchain.
