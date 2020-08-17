.. _ncs_release_notes_latest:

Changes in |NCS| v1.3.99
########################

The most relevant changes that are present on the master branch of the |NCS|, as compared to the latest release, are tracked in this file.
Note that this file is a work in progress and might not cover all relevant changes.


Changelog
*********

See the `master branch section in the changelog`_ for a list of the most important changes.

sdk-nrfxlib
===========

See the changelog for each library in the :doc:`nrfxlib documentation <nrfxlib:README>` for the most current information.

sdk-zephyr
==========

.. NOTE TO MAINTAINERS: The latest Zephyr commit appears in multiple places; make sure you update them all.

The Zephyr fork in |NCS| contains all commits from the upstream Zephyr repository up to and including ``4ef29b34e3``, plus some |NCS| specific additions.

For a complete list of upstream Zephyr commits incorporated into |NCS| since the most recent release, run the following command from the :file:`ncs/zephyr` repository (after running ``west update``):

.. code-block:: none

   git log --oneline 4ef29b34e3 ^v2.3.0-rc1-ncs1

For a complete list of |NCS| specific commits, run:

.. code-block:: none

   git log --oneline manifest-rev ^4ef29b34e3

The following list summarizes the most important changes inherited from upstream Zephyr:

* Architectures:

  * Fixed parsing of Cortex-M MemManage Stacking Errors to correctly report thread stack corruptions.

* Kernel:

  * The ``CONFIG_KERNEL_DEBUG`` Kconfig option, which was used to enable ``printk()`` based debugging of the kernel internals, has been removed.
    The kernel now uses the standard Zephyr logging API at DBG log level for this purpose.
    The logging module used for the kernel is named ``os``.

* Boards:

  * Introduced a board definition for the nRF5340 DK.
  * Added the netif capability for the :ref:`zephyr:nrf52840dk_nrf52840`.
  * Listed GPIO as a supported feature in nRF-based board definitions.
  * Modified I2C1 and SPI2 pin assignments in the :ref:`zephyr:nrf5340pdk_nrf5340` to match
    the standard location for I2C and SPI in the Arduino header.
  * Added nRF52820 nrfx defines for emulation on the :ref:`zephyr:nrf52833dk_nrf52833`.

* Networking:

  * LwM2M:

    * Fixed a bug where a FOTA socket was not closed after the download (PULL mode).
    * Added a Kconfig option :option:`CONFIG_LWM2M_SECONDS_TO_UPDATE_EARLY` that specifies how long before the time-out the Registration Update will be sent.
    * Added ObjLnk resource type support.

  * MQTT:

    * The ``utf8`` pointer in the :cpp:class:`mqtt_utf8` struct is now const.
    * The default ``clean_session`` value is now configurable with Kconfig (see :option:`CONFIG_MQTT_CLEAN_SESSION`).

  * OpenThread:

    * Updated the OpenThread revision to upstream commit ``e653478c503d5b13207b01938fa1fa494a8b87d3``.
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
    Use :cpp:member:`bt_le_scan_param::options` instead.
  * Deprecated ``bt_conn_create_le()``.
    Use :cpp:func:`bt_conn_le_create` instead.
  * Deprecated ``bt_conn_create_auto_le()``.
    Use :cpp:func:`bt_conn_le_create_auto` instead.
  * Deprecated ``bt_conn_create_slave_le()``.
    Use :cpp:func:`bt_le_adv_start` instead, with :cpp:member:`bt_le_adv_param::peer` set to the remote peer's address.
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

* Drivers:

  * Clock control:

    * Fixed an issue in the nRF clock control driver that could lead to a fatal error during the system initialization, when calibration was started before kernel services became available.

  * Display:

    * Added support for temperature sensors in the SSD16xx driver.

  * Entropy:

    * Fixed a race condition in the nRF5 entropy driver that could result in missing the wake-up event (which caused the ``kernel.memory_protection.stack_random`` test to fail).

  * Flash:

    * Extended the flash API with the :cpp:func:`flash_get_parameters` function.
    * Fixed an issue in the Nordic Semiconductor nRF flash driver (soc_flash_nrf) that caused operations to fail if a Bluetooth central had multiple connections.
    * Added support for a 2 IO pin setup in the nRF QSPI NOR flash driver (nrf_qspi_nor).
    * Added support for sub-word lengths of read and write transfers in the nRF QSPI NOR flash driver (nrf_qspi_nor).
    * Improved the handling of erase operations in the nRF QSPI NOR flash driver (nrf_qspi_nor), the AT45 family flash driver (spi_flash_at45), and the SPI NOR flash driver (spi_nor).
      Now the operation is not started if it cannot be completed successfully.

  * GPIO:

    * Removed deprecated API functions and macros.
    * Improved allocation of GPIOTE channels in the nRF GPIO driver (gpio_nrfx).

  * I2C:

    * Fixed handling of scattered transactions in the nRF TWIM nrfx driver (i2c_nrfx_twim) by introducing an optional concatenation buffer.
    * Used a time limit (100 ms) when waiting for transactions to complete, in both nRF drivers.

  * LoRa:

    * Added support for SX126x transceivers.

  * PWM:

    * Clarified the expected API behavior regarding zero pulse length and non-zero pulse equal to period length.

  * Sensors:

    * Added support for the IIS2DH accelerometer.
    * Added the :cpp:func:`sensor_attr_get` API function for getting the value of a sensor attribute.

  * Serial:

    * Clarified in the UART API that the :cpp:enumerator:`UART_RX_RDY <uart_interface::UART_RX_RDY>` event is to be generated before :cpp:enumerator:`UART_RX_DISABLED <uart_interface::UART_RX_DISABLED>` if any received data remains.
      Updated all drivers in this regard.
    * Changed the nRF UART nrfx drivers (uart_nrfx_uart/uarte) to use the DT ``hw-flow-control`` property instead of Kconfig options.
    * Fixed disabling of the TX interrupt in the uart_nrfx_uart driver.
    * Fixed the uart_nrfx_uarte driver to prevent spurious :cpp:enumerator:`UART_RX_BUF_REQUEST <uart_interface::UART_RX_BUF_REQUEST>` events.

  * SPI:

    * Updated the ``cs-gpios`` properties in DT SPI nodes with proper GPIO flags specifying the active level.
      Updated the related drivers to use the flags from ``cs-gpios`` properties instead of hard-coded values.

  * Timer:

    * Fixed an issue in the nRF Real Time Counter Timer driver (nrf_rtc_timer) that could cause time-outs to be triggered prematurely.
    * Fixed announcing of kernel ticks in the nRF Real Time Counter Timer driver (nrf_rtc_timer) that made some kernel tests fail when the :option:`CONFIG_TICKLESS_KERNEL` option was disabled.

  * USB:

    * Unified endpoint helper macros across all USB device drivers.
    * Fixed handling of fragmented transfers on the control OUT endpoint in the Nordic Semiconductor USB Device Controller driver (usb_dc_nrfx).
    * Introduced names for threads used in USB classes, to aid debugging.

  * Watchdog:

    * Updated the description of the :cpp:func:`wdt_feed` API function to reflect an additional error return code.

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

* Modules:

  * Introduced a ``depends`` keyword that can be added to a module's :file:`module.yml` file to declare dependencies to other modules.
    This allows to correctly establish the order of processing.

* Other:

  * Implemented ``nanosleep`` in the POSIX subsystem.
  * Deprecated the Zephyr-specific types in favor of the standard C99 int types.

The following list contains |NCS| specific additions:

* Added support for the |NCS|'s :ref:`partition_manager`, which can be used for flash partitioning.
* Added the following network socket and address extensions to the :ref:`zephyr:bsd_sockets_interface` interface to support the functionality provided by the :ref:`nrfxlib:bsdlib`:

  AF_LTE, NPROTO_AT, NPROTO_PDN, NPROTO_DFU, SOCK_MGMT, SO_RCVTIMEO, SO_BINDTODEVICE, SOL_PDN, SOL_DFU, SO_PDN_CONTEXT_ID, SO_PDN_STATE, SOL_DFU, SO_DFU_ERROR, TLS_SESSION_CACHE, SO_SNDTIMEO, MSG_TRUNC, SO_SILENCE_ALL, SO_IP_ECHO_REPLY, SO_IPV6_ECHO_REPLY
* Added support for enabling TLS caching when using the :ref:`zephyr:mqtt_socket_interface` library.
  See :c:macro:`TLS_SESSION_CACHE`.
* Updated the nrf9160ns DTS to support accessing the CryptoCell CC310 hardware from non-secure code.
