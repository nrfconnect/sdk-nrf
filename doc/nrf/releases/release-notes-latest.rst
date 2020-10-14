.. _ncs_release_notes_latest:

Changes in |NCS| v1.3.99
########################

.. contents::
   :local:
   :depth: 2

The most relevant changes that are present on the master branch of the |NCS|, as compared to the latest release, are tracked in this file.
Note that this file is a work in progress and might not cover all relevant changes.


Changelog
*********

See the `master branch section in the changelog`_ for a list of the most important changes.


sdk-mcuboot
===========

The MCUboot fork in |NCS| contains all commits from the upstream MCUboot repository up to and including ``5a6e18148d``, plus some |NCS| specific additions.

The following list summarizes the most important changes inherited from upstream MCUboot:

  * Fixed an issue where after erasing an image, an image trailer might be left behind.
  * Added a ``CONFIG_BOOT_INTR_VEC_RELOC`` option to relocate interrupts to application.
  * Fixed single image compilation with serial recovery.
  * Added support for single-image applications (see `CONFIG_SINGLE_IMAGE_DFU`).
  * Added a ``CONFIG_BOOT_SIGNATURE_TYPE_NONE`` option to disable the cryptographic check of the image.
  * Reduced the minimum number of members in SMP packet for serial recovery.
  * Introduced direct execute-in-place (XIP) mode (see ``CONFIG_BOOT_DIRECT_XIP``).
  * Fixed kramdown CVE-2020-14001.
  * Modified the build system to let the application use a private key that is located in the user project configuration directory.
  * Added support for nRF52840 with ECC keys and CryptoCell.
  * Allowed to set VTOR to a relay vector before chain-loading the application.
  * Allowed using a larger primary slot in swap-move.
    Before, both slots had to be the same size, which imposed an unused sector in the secondary slot.
  * Fixed bootstrapping in swap-move mode.
  * Fixed an issue that caused an interrupted swap-move operation to potentially brick the device if the primary image was padded.
  * Various fixes, enhancements, and changes needed to work with the latest Zephyr version.

sdk-nrfxlib
===========

See the changelog for each library in the :doc:`nrfxlib documentation <nrfxlib:README>` for the most current information.

sdk-zephyr
==========

.. NOTE TO MAINTAINERS: The latest Zephyr commit appears in multiple places; make sure you update them all.

The Zephyr fork in |NCS| contains all commits from the upstream Zephyr repository up to and including ``0aaee6dcf5``, plus some |NCS| specific additions.

For a complete list of upstream Zephyr commits incorporated into |NCS| since the most recent release, run the following command from the :file:`ncs/zephyr` repository (after running ``west update``):

.. code-block:: none

   git log --oneline 0aaee6dcf5 ^v2.3.0-rc1-ncs1

For a complete list of |NCS| specific commits, run:

.. code-block:: none

   git log --oneline manifest-rev ^0aaee6dcf5

The following list summarizes the most important changes inherited from upstream Zephyr:

* Architectures:

  * Fixed parsing of Cortex-M MemManage Stacking Errors to correctly report thread stack corruptions.
  * Extended the interrupt vector relaying feature to support Cortex-M Mainline architecture variants.
  * Aligned the Cortex-M vector table according to VTOR requirements.
  * Fixed booting in no-multithreading mode.

* Kernel:

  * Removed the ``CONFIG_KERNEL_DEBUG`` Kconfig option, which was used to enable ``printk()`` based debugging of the kernel internals.
    The kernel now uses the standard Zephyr logging API at DBG log level for this purpose.
    The logging module used for the kernel is named ``os``.
  * Added :c:func:`k_delayed_work_pending` to check if work has been submitted.
  * Updated the kernel to not call swap if the next thread that is ready is the current thread.

* Boards:

  * Introduced a board definition for the nRF5340 DK.
  * Added the netif capability for the :ref:`zephyr:nrf52840dk_nrf52840`.
  * Listed GPIO as a supported feature in nRF-based board definitions.
  * Modified I2C1 and SPI2 pin assignments in the :ref:`zephyr:nrf5340pdk_nrf5340` to match
    the standard location for I2C and SPI in the Arduino header.
  * Added nRF52820 nrfx defines for emulation on the :ref:`zephyr:nrf52833dk_nrf52833`.
  * Added support for the :ref:`zephyr:nrf21540dk_nrf52840`.


* Networking:

  * Switched networking threads to use the kernel stack.

  * LwM2M:

    * Fixed a bug where a FOTA socket was not closed after the download (PULL mode).
    * Added a Kconfig option :option:`CONFIG_LWM2M_SECONDS_TO_UPDATE_EARLY` that specifies how long before the time-out the Registration Update will be sent.
    * Added ObjLnk resource type support.
    * Fixed Security and Server object instance matching.
    * Fixed handling of fds polling (in case there is another socket open).
    * Made ``send()`` calls on the same socket thread-safe.
    * Fixed the size of the :c:struct:`sockaddr` structure that was insufficient when provided on an IPv6 socket while IPv4 was enabled as well.
    * Fixed PUSH mode FOTA.
    * Fixed bootstrap procedure.

  * MQTT:

    * The ``utf8`` pointer in the :c:struct:`mqtt_utf8` struct is now const.
    * The default ``clean_session`` value is now configurable with Kconfig (see :option:`CONFIG_MQTT_CLEAN_SESSION`).
    * Prevented double CONNACK event notification on server reject.

  * OpenThread:

    * Updated the OpenThread revision to upstream commit ``ac86fe52e62e60a66aeeb1c905cb1294709147e9``.
    * Implemented a missing ``enable`` API function for the OpenThread interface.
    * Cleaned up the OpenThread Kconfig file.
      OpenThread dependencies are now enabled automatically.
    * Allowed the application to register a callback function for OpenThread state changes.
    * Reimplemented the logger glue layer for better performance.
    * Updated the OpenThread thread priority class to be configurable.
    * Added several Kconfig options to customize the OpenThread stack.
    * Added Sleep to Transmit as hardware radio capability (``OT_RADIO_CAPS_SLEEP_TO_TX``).
    * Removed retransmissions from the radio capabilities (``OT_RADIO_CAPS_TRANSMIT_RETRIES``).
    * Added a configuration option to select the OpenThread version (either 1.1 or 1.2).
    * Added configuration options for NCP vendor hooks (see :option:`CONFIG_OPENTHREAD_NCP_VENDOR_HOOK_SOURCE`).
    * Allowed use of custom mbed TLS (see :option:`CONFIG_OPENTHREAD_MBEDTLS_LIB_NAME`).
    * Removed double-buffering in UART send.
    * Fixed the network initialization when :option:`CONFIG_NET_CONFIG_MY_IPV6_ADDR` is not set.
    * Added a Kconfig option that allows to link Zephyr with precompiled OpenThread libraries (see :option:`CONFIG_OPENTHREAD_SOURCES`).
    * Added a Kconfig option to compile with Diagnostic functions support (see :option:`CONFIG_OPENTHREAD_DIAG`).

  * Socket offloading:

    * Removed dependency to the :option:`CONFIG_NET_SOCKETS_POSIX_NAMES` configuration option.
    * Fixed an issue where the network interface was missing for offloaded drivers (#27037).
    * Updated the ``close()`` socket call to no longer use ``ioctl()`` underneath.
      It now has a separate entry in a socket vtable.

  * IP:

    * Fixed an issue where IPv6 RS messages did not comply with RFC4291.
    * Added infrastructure for collecting stack timing statistics for network packet pass-through (see :option:`CONFIG_NET_PKT_TXTIME_STATS_DETAIL` and :option:`CONFIG_NET_PKT_RXTIME_STATS_DETAIL`).

  * TCP:

    * Made the new TCP stack the default one (see :option:`CONFIG_NET_TCP2`).
    * Removed ``net_tcp_init()`` for non-native stacks.
    * Implemented a blocking connect in the TCP2 stack.
    * Fixed unaligned access in the TCP2 stack.

  * Networking configuration:

    * Added support for initialization from application.

* Bluetooth:

  * Added support for LE Advertising Extensions.
  * Added APIs for application-controlled data length and PHY updates.
  * Added legacy OOB pairing support.
  * Multiple improvements to OOB data access and pairing.
  * Deprecated ``BT_LE_SCAN_FILTER_DUPLICATE``.
    Use :c:enumerator:`BT_LE_SCAN_OPT_FILTER_DUPLICATE` instead.
  * Deprecated ``BT_LE_SCAN_FILTER_WHITELIST``.
    Use :c:enumerator:`BT_LE_SCAN_OPT_FILTER_WHITELIST` instead.
  * Deprecated ``bt_le_scan_param::filter_dup``.
    Use :c:member:`bt_le_scan_param.options` instead.
  * Deprecated ``bt_conn_create_le()``.
    Use :c:func:`bt_conn_le_create` instead.
  * Deprecated ``bt_conn_create_auto_le()``.
    Use :c:func:`bt_conn_le_create_auto` instead.
  * Deprecated ``bt_conn_create_slave_le()``.
    Use :c:func:`bt_le_adv_start` instead, with :c:member:`bt_le_adv_param.peer` set to the remote peer's address.
  * Deprecated the ``BT_LE_ADV_*`` macros.
    Use the ``BT_GAP_ADV_*`` enums instead.
  * Updated L2CAP RX MTU to be controlled by :option:`CONFIG_BT_L2CAP_RX_MTU` (instead of :option:`CONFIG_BT_RX_BUF_LEN`) when :option:`CONFIG_BT_HCI_ACL_FLOW_CONTROL` is disabled.
    If :option:`CONFIG_BT_RX_BUF_LEN` is changed from its default value, :option:`CONFIG_BT_L2CAP_RX_MTU` should be set to ``CONFIG_BT_RX_BUF_LEN - 8``.
  * Added support for periodic advertisement to the Host.
  * Added a :c:member:`bt_conn_auth_cb.bond_deleted` callback to the Host.
  * Added support for starting a persistent advertiser when the maximum number of connections has been reached.
  * Fixed the settings of Advertising Data on extended advertising instances.
  * Updated the SMP implementation in the Host to reject legacy pairing early in SC-only mode.
  * Fixed an issue with :c:func:`bt_gatt_service_unregister` not clearing CCC information, which might result in no space to store the CCC configuration.
  * Added support in L2CAP for elevating the security level before sending the connection request if the application has set a required security level on the channel.
  * Added an option to disable GATT security checks (see :option:`CONFIG_BT_CONN_DISABLE_SECURITY`).
  * Added support for automatic discovery of CCC when subscribing (see :option:`CONFIG_BT_GATT_AUTO_DISCOVER_CCC`).
  * Fixed an issue where a peripheral might not store CCC in non-volatile memory in case of multiple CCC changes (due to a race condition).
  * Fixed a deadlock in receiving a disconnected event when disconnecting with pending GATT Write commands.
  * Fixed an issue where a persistent advertiser would not be started due to a race condition.

* Bluetooth LE Controller:

  * Updated the Controller to be 5.2 compliant.
  * Made PHY support configurable.
  * Updated the Controller to only use control procedures supported by the peer.
  * Added support for the nRF52820 SoC.
  * Removed the legacy Controller.
  * Implemented a function to remove auxiliary advertising sets (``ll_adv_aux_set_remove()``).
  * Implemented a function to remove all primary channels and auxiliary channels of an advertising set (``ll_adv_aux_set_clear()``).
  * Fixed overflow that could happen when using uninitialized PDU.
  * Removed redundant :option:`CONFIG_BT_LL_SW_SPLIT` conditional.
  * Enforced that the Read RSSI command is supported if the Connection State is supported.
  * Added missing aux acquire on periodic advertising.
  * Updated the implementation to schedule non-overlapping sync PDUs.
  * Fixed the handling of HCI commands for extended advertising.
  * Added a terminate event for extended advertising.
  * Filled the missing Periodic Advertising interval in the Extended Advertising Report when auxiliary PDUs contain Sync Info fields.
  * Filled the referenced event counter of the Periodic Advertising SYNC_IND PDU into the Sync Info structure in the Common Extended Advertising Header Format.
  * Switched HCI threads to use the kernel stack.

* Bluetooth Mesh:

  * Removed the ``net_idx`` parameter from the Health Client model APIs because it can be derived (by the stack) from the ``app_idx`` parameter.
  * Documented :ref:`Mesh Shell commands <zephyr:bluetooth_mesh_shell>`.
  * Allowed to configure the advertiser stack size (see :option:`CONFIG_BT_MESH_ADV_STACK_SIZE`).
  * Resolved a corner case where the segmented sending would be rescheduled before the segments were done sending.
  * Fixed dangling transport segmentation buffer pointer when Friend feature is enabled.
  * Switched advertising threads to use the kernel stack.

* Bluetooth shell:

  * Added an advertising option for undirected one-time advertising.
  * Added an advertising option for advertising using identity address when local privacy is enabled.
  * Added an advertising option for directed advertising to privacy-enabled peer when local privacy is disabled.
  * Updated the info command to print PHY and data length information.

* Drivers:

  * Bluetooth HCI:

    * Fixed missing ``gpio_dt_flags`` in :c:struct:`spi_cs_control` in the HCI driver over SPI transport.

  * Clock control:

    * Fixed an issue in the nRF clock control driver that could lead to a fatal error during the system initialization, when calibration was started before kernel services became available.
    * Reworked the nRF clock control driver implementation to use the On-Off Manager.

  * Display:

    * Added support for temperature sensors in the SSD16xx driver.

  * Entropy:

    * Fixed a race condition in the nRF5 entropy driver that could result in missing the wake-up event (which caused the ``kernel.memory_protection.stack_random`` test to fail).

  * EEPROM:

    * Fixed chip-select GPIO flags extraction from DTS in AT2x driver.

  * Flash:

    * Extended the flash API with the :c:func:`flash_get_parameters` function.
    * Fixed an issue in the Nordic Semiconductor nRF flash driver (soc_flash_nrf) that caused operations to fail if a Bluetooth central had multiple connections.
    * Added support for a 2 IO pin setup in the nRF QSPI NOR flash driver (nrf_qspi_nor).
    * Added support for sub-word lengths of read and write transfers in the nRF QSPI NOR flash driver (nrf_qspi_nor).
    * Improved the handling of erase operations in the nRF QSPI NOR flash driver (nrf_qspi_nor), the AT45 family flash driver (spi_flash_at45), and the SPI NOR flash driver (spi_nor).
      Now the operation is not started if it cannot be completed successfully.
    * Established the unrestricted alignment of flash reads for all drivers.
    * Enhanced the nRF QSPI NOR flash driver (nrf_qspi_nor) so that it supports unaligned read offset, read length, and buffer offset.
    * Added SFDP support in the SPI NOR flash driver (spi_nor).
    * Fixed a regression in the nRF flash driver (soc_flash_nrf) when using the :option:`CONFIG_BT_CTLR_LOW_LAT` option.

  * GPIO:

    * Removed deprecated API functions and macros.
    * Improved allocation of GPIOTE channels in the nRF GPIO driver (gpio_nrfx).

  * I2C:

    * Fixed handling of scattered transactions in the nRF TWIM nrfx driver (i2c_nrfx_twim) by introducing an optional concatenation buffer.
    * Used a time limit (100 ms) when waiting for transactions to complete, in both nRF drivers.

  * IEEE 802.15.4:

    * Added 802.15.4 multiprotocol support (see :option:`CONFIG_NRF_802154_MULTIPROTOCOL_SUPPORT`).
    * Added the Kconfig option :option:`CONFIG_IEEE802154_VENDOR_OUI_ENABLE` for defining OUI.

  * LoRa:

    * Added support for SX126x transceivers.

  * PWM:

    * Clarified the expected API behavior regarding zero pulse length and non-zero pulse equal to period length.

  * Sensors:

    * Added support for the IIS2DH accelerometer.
    * Added the :c:func:`sensor_attr_get` API function for getting the value of a sensor attribute.
    * Added support for the :ref:`zephyr:wsen-itds`.

  * Serial:

    * Clarified in the UART API that the :c:enumerator:`UART_RX_RDY` event is to be generated before :c:enumerator:`UART_RX_DISABLED` if any received data remains.
      Updated all drivers in this regard.
    * Changed the nRF UART nrfx drivers (uart_nrfx_uart/uarte) to use the DT ``hw-flow-control`` property instead of Kconfig options.
    * Fixed disabling of the TX interrupt in the uart_nrfx_uart driver.
    * Fixed the uart_nrfx_uarte driver to prevent spurious :c:enumerator:`UART_RX_BUF_REQUEST` events.
    * Removed counters reset from :c:func:`uart_rx_enable` in the nrf_uarte driver.
    * Changed wrappers of optional API functions to always be present and return ``-ENOTSUP`` when a given function is not implemented in the driver that is used.
    * Added another error code (``-EACCES``) that can be returned by the :c:func:`uart_rx_buf_rsp` API function.
      Updated all existing drivers that implement this function accordingly.
    * Added initial clean-up of the receiver state in the nRF UARTE driver (uart_nrfx_uarte).
    * Added initial disabling of the UART peripheral before its pins are configured in the nRF UART/UARTE drivers (uart_nrfx_uart/uarte).

  * SPI:

    * Updated the implementation of the nRF SPIM driver (spi_nrfx_spim) to support data rates higher than 8 Mbps in the nRF5340 SoC.
    * Changed wrappers of optional API functions to always be present and return ``-ENOTSUP`` when a given function is not implemented in the driver that is used.
    * Updated the ``cs-gpios`` properties in DT SPI nodes with proper GPIO flags specifying the active level.
      Updated the related drivers to use the flags from ``cs-gpios`` properties instead of hard-coded values.

  * Timer:

    * Fixed an issue in the nRF Real Time Counter Timer driver (nrf_rtc_timer) that could cause time-outs to be triggered prematurely.
    * Fixed announcing of kernel ticks in the nRF Real Time Counter Timer driver (nrf_rtc_timer) that made some kernel tests fail when the :option:`CONFIG_TICKLESS_KERNEL` option was disabled.

  * USB:

    * Unified endpoint helper macros across all USB device drivers.
    * Fixed handling of fragmented transfers on the control OUT endpoint in the Nordic Semiconductor USB Device Controller driver (usb_dc_nrfx).
    * Introduced names for threads used in USB classes, to aid debugging.
    * Updated the way the :c:func:`usb_enable` function should be used.
      For some samples, this function was invoked automatically on system boot-up to enable the USB subsystem, but now it must be called explicitly by the application.
      If your application relies on any of the following Kconfig options, it must also enable the USB subsystem:

      * :option:`CONFIG_OPENTHREAD_NCP_SPINEL_ON_UART_ACM`
      * :option:`CONFIG_USB_DEVICE_NETWORK_ECM`
      * :option:`CONFIG_USB_DEVICE_NETWORK_EEM`
      * :option:`CONFIG_USB_DEVICE_NETWORK_RNDIS`
      * :option:`CONFIG_TRACING_BACKEND_USB`
      * :option:`CONFIG_USB_UART_CONSOLE`

    * Fixed an issue that CDC ACM was not accepting OUT transfers after Resume from Suspend.
    * Fixed an issue with remote wake-up requests in the nRF driver.
    * Updated the implementation of the HID class to allow sending data only in CONFIGURED state.
    * Updated to use the kernel stack for threads not running in user space.

  * Watchdog:

    * Updated the description of the :c:func:`wdt_feed` API function to reflect an additional error return code.

* Storage and file systems:

  * Fixed a possible NULL pointer dereference when using any of the ``fs_`` functions.
    The functions will now return an error code in this case.
  * Fixed a garbage-collection issue in the NVS subsystem.
  * Added the Kconfig option :option:`CONFIG_FS_FATFS_EXFAT` for enabling exFAT support.
  * Added support for file open flags to fs and POSIX API.

* Management:

  * MCUmgr:

    * Moved mcumgr into its own directory.
    * Switched UDP port to use the kernel stack.
    * Added missing socket close in error path for SMP.

  * Added support for Open Supervised Device Protocol (OSDP) (see :option:`CONFIG_OSDP`).

  * updatehub:

    * Moved updatehub from :file:`lib` to :file:`subsys/mgmt`.
    * Fixed out-of-bounds access and added a return value check for ``flash_img_init()``.
    * Fixed a ``getaddrinfo`` resource leak.

* Settings:

  * Updated the implementation to return an error rather than faulting if there is an attempt to read a setting from a channel that does not support reading.
  * Disallowed modifying the content of a static subtree name.

* LVGL:

  * Updated the library to the new major release v7.0.2.
  * Aligned LVGL Kconfig constants with suggested defaults from upstream.

* Tracing:

  * Updated the API to check if the init function exists prior to calling it.

* Logging:

  * Fixed immediate logging with multiple backends.
  * Switched logging thread to use the kernel stack.
  * Allowed users to disable all shell backends at once using :option:`CONFIG_SHELL_LOG_BACKEND`.
  * Added a logging backend for the Spinel protocol.
  * Fixed timestamp calculation when using NEWLIB.

* Shell:

  * Switched to use the kernel stack.
  * Fixed the select command.
  * Fixed prompting dynamic commands.

* libc:

  * Simplified newlib malloc arena definition.

* Devicetree:

  * Removed all nRF-specific aliases to particular hardware peripherals, because they are no longer needed now that nodes can be addressed by node labels.
    For example, you should now use ``DT_NODELABEL(i2c0)`` instead of ``DT_ALIAS(i2c_0)``.

* Build system:

  * Renamed the ``TEXT_SECTION_OFFSET`` symbol to ``ROM_START_OFFSET``.
  * Added a number of iterable section macros to the set of linker macros, including ``Z_ITERABLE_SECTION_ROM`` and ``Z_ITERABLE_SECTION_RAM``.
  * Added a new Zephyr Build Configuration package with support for specific build configuration for Zephyr derivatives (including forks).
    See :ref:`zephyr:cmake_pkg` for more information.
  * Removed the set of ``*_if_kconfig()`` CMake functions.
    Use ``_ifdef(CONFIG_ ...)`` instead.
  * BOARD, SOC, DTS, and ARCH roots can now be specified in each module's :file:`zephyr/module.yml` file (see :ref:`modules_build_settings`).
    If you use something similar to ``source $(SOC_DIR)/<path>``, change it to ``rsource <relative>/<path>`` or similar.

* Samples:

  * Updated the :ref:`zephyr:nrf-system-off-sample` to better support low-power states of Nordic Semiconductor devices.
  * Updated the :ref:`zephyr:usb_mass` to perform all application-level configuration before the USB subsystem starts.
    The sample now also supports FAT file systems on external storage.
  * Updated the :ref:`zephyr:nvs-sample` sample to do a full chip erase when programming.
  * Fixed the build of the :ref:`zephyr:bluetooth-mesh-onoff-level-lighting-vnd-sample` application with mcumgr.
  * Added new commands ``write_unaligned`` and ``write_pattern`` to the :ref:`zephyr:samples_flash_shell`.
  * Fixed the ``cmd_hdr`` and ``acl_hdr`` usage in the :ref:`zephyr:bluetooth-hci-spi-sample` sample.
  * Removed the NFC sample.
  * Updated the configuration for extended advertising in the :ref:`zephyr:bluetooth-hci-uart-sample` and :ref:`zephyr:bluetooth-hci-rpmsg-sample` samples.

* Modules:

  * Introduced a ``depends`` keyword that can be added to a module's :file:`module.yml` file to declare dependencies to other modules.
    This allows to correctly establish the order of processing.

* Testing infrastructure:

  * sanitycheck:

    * Added an ``integration`` option that is used to list platforms to use in integration testing (CI) and avoids whitelisting platforms.
    * Updated sanitycheck to not expect a PASS result from build_only instances.
    * Added a command line option for the serial_pty script.
    * Added support for ``only_tags`` in the platform definition.
    * Disabled returning errors on warnings.

* Other:

  * Implemented ``nanosleep`` in the POSIX subsystem.
  * Deprecated the Zephyr-specific types in favor of the standard C99 int types.
  * Removed ``CONFIG_NET_IF_USERSPACE_ACCESS``, because it is no longer needed.
  * Renamed some attributes in the :c:struct:`device` struct: ``config_info`` to ``config``, ``driver_api`` to ``api``, and ``driver_data`` to ``data``.

The following list contains |NCS| specific additions:

* Added support for the |NCS|'s :ref:`partition_manager`, which can be used for flash partitioning.
* Added the following network socket and address extensions to the :ref:`zephyr:bsd_sockets_interface` interface to support the functionality provided by the :ref:`nrfxlib:bsdlib`:

  AF_LTE, NPROTO_AT, NPROTO_PDN, NPROTO_DFU, SOCK_MGMT, SO_RCVTIMEO, SO_BINDTODEVICE, SOL_PDN, SOL_DFU, SO_PDN_CONTEXT_ID, SO_PDN_STATE, SOL_DFU, SO_DFU_ERROR, TLS_SESSION_CACHE, SO_SNDTIMEO, MSG_TRUNC, SO_SILENCE_ALL, SO_IP_ECHO_REPLY, SO_IPV6_ECHO_REPLY
* Added support for enabling TLS caching when using the :ref:`zephyr:mqtt_socket_interface` library.
  See :c:macro:`TLS_SESSION_CACHE`.
* Updated the nrf9160ns DTS to support accessing the CryptoCell CC310 hardware from non-secure code.
