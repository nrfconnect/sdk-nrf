.. _ncs_release_notes_latest:

Changes in |NCS| v1.3.99
########################

The most relevant changes that are present on the master branch of the |NCS|, as compared to the latest release, are tracked in this file.
Note that this file is a work in progress and might not cover all relevant changes.


Change log
**********

See the `master branch section in the change log`_ for a list of the most important changes.

sdk-nrfxlib
===========

See the change log for each library in the :doc:`nrfxlib documentation <nrfxlib:README>` for the most current information.

sdk-zephyr
==========

The Zephyr fork in |NCS| contains all commits from the upstream Zephyr repository up to and including ``4ef29b34e3``, plus some |NCS| specific additions.

The following list summarizes the most important changes inherited from upstream Zephyr:

* Kernel:

  * The ``CONFIG_KERNEL_DEBUG`` Kconfig option, which was used to enable ``printk()`` based debugging of the kernel internals, has been removed.
    The kernel now uses the standard Zephyr logging API at DBG log level for this purpose.
    The logging module used for the kernel is named ``os``.

* Networking:

  * LwM2M:

    * Fixed a bug where a FOTA socket was not closed after the download (PULL mode).
    * Added a Kconfig option :option:`CONFIG_LWM2M_SECONDS_TO_UPDATE_EARLY` that specifies how long before the time-out the Registration Update will be sent.
    * Added ObjLnk resource type support.

  * MQTT:

    * The ``utf8`` pointer in the :c:type:`mqtt_utf8` struct is now const.
    * The default ``clean_session`` value is now configurable with Kconfig (see :option:`CONFIG_MQTT_CLEAN_SESSION`).

  * OpenThread:

    * Updated the OpenThread revision to upstream commit e653478c503d5b13207b01938fa1fa494a8b87d3.
    * Implemented a missing ``enable`` API function for the OpenThread interface.
    * Cleaned up the OpenThread Kconfig file.
      OpenThread dependencies are now enabled automatically.
    * Allowed the application to register a callback function for OpenThread state changes.
    * Reimplemented the logger glue layer for better performance.
    * Updated the OpenThread thread priority class to be configurable.
    * Added several Kconfig options to customize the OpenThread stack.

  * Socket offloading:

    * Removed dependency to the :option:`CONFIG_NET_SOCKETS_POSIX_NAMES` configuration option.

* Bluetooth:

  * Added support for LE Advertising Extensions.
  * Added APIs for application-controlled data length and PHY updates.
  * Added legacy OOB pairing support.
  * Multiple improvements to OOB data access and pairing.
  * Deprecated ``BT_LE_SCAN_FILTER_DUPLICATE``.
    Use :cpp:enumerator:`BT_LE_SCAN_OPT_FILTER_DUPLICATE <bt_gap::BT_LE_SCAN_OPT_FILTER_DUPLICATE>` instead.
  * Deprecated ``BT_LE_SCAN_FILTER_WHITELIST``.
    Use :cpp:enumerator:`BT_LE_SCAN_OPT_FILTER_WHITELIST <bt_gap::BT_LE_SCAN_OPT_FILTER_WHITELIST>` instead.
  * Deprecated ``bt_le_scan_param::filter_dup``.
    Use ``bt_le_scan_param::options`` instead.
  * Deprecated ``bt_conn_create_le()``.
    Use :cpp:func:`bt_conn_le_create` instead.
  * Deprecated ``bt_conn_create_auto_le()``.
     Use :cpp:func:`bt_conn_le_create_auto` instead.
  * Deprecated ``bt_conn_create_slave_le()``.
    Use :cpp:func:`bt_le_adv_start` instead, with ``bt_le_adv_param::peer`` set to the remote peer's address.
  * Deprecated the ``BT_LE_ADV_*`` macros.
    Use the ``BT_GAP_ADV_*`` enums instead.

* Bluetooth LE Controller:

  * Updated the Controller to be 5.2 compliant.
  * Made PHY support configurable.
  * Updated the Controller to only use control procedures supported by the peer.
  * Added support for the nRF52820 SoC.
  * Removed the legacy Controller.

* Bluetooth Mesh:

  * Removed the ``net_idx`` parameter from the Health Client model APIs because it can be derived (by the stack) from the ``app_idx`` parameter.

* Storage and file systems:

  * Fixed a possible NULL pointer dereference when using any of the ``fs_`` functions.
    The functions will now return an error code in this case.
  * Fixed a garbage-collection issue in the NVS subsystem.

* Devicetree:

  * Removed all nRF-specific aliases to particular hardware peripherals, because they are no longer needed now that nodes can be addressed by node labels.
    For example, you should now use ``DT_NODELABEL(i2c0)`` instead of ``DT_ALIAS(i2c_0)``.

* Build system:

  * Renamed the ``TEXT_SECTION_OFFSET`` symbol to ``ROM_START_OFFSET``.
  * Added a number of iterable section macros to the set of linker macros, including ``Z_ITERABLE_SECTION_ROM`` and ``Z_ITERABLE_SECTION_RAM``.
  * Added a new Zephyr Build Configuration package with support for specific build configuration for Zephyr derivatives (including forks).
    See :ref:`zephyr:cmake_pkg` for more information.

* Samples:

  * Updated the :ref:`zephyr:nrf-system-off-sample` to better support low-power states of Nordic Semiconductor devices.
  * Updated the :ref:`zephyr:usb_mass` to perform all application-level configuration before the USB subsystem starts.
    The sample now also supports FAT file systems on external storage.

Modules:

  * Introduced a ``depends`` keyword that can be added to a module's :file:`module.yml` file to declare dependencies to other modules.
    This allows to correctly establish the order of processing.

Other:

  * Implemented ``nanosleep`` in the POSIX subsystem.
  * Deprecated the Zephyr-specific types in favor of the standard C99 int types.

The following list contains |NCS| specific additions:

*
