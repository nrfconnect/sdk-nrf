.. _ncs_release_notes_latest:

Changes in |NCS| v1.4.99
########################

.. contents::
   :local:
   :depth: 2

The most relevant changes that are present on the master branch of the |NCS|, as compared to the latest release, are tracked in this file.

.. note::
    This file is a work in progress and might not cover all relevant changes.

Changelog
*********

The following sections provide detailed lists of changes by component.

nRF5
====

The following changes are relevant for the nRF52 and nRF53 Series.

nRF52832 SoC
------------

* Removed:

  * ``esb`` subystem - Removed support for older SoC revision workarounds.

    * Removed support for nRF52832 revision 1 workarounds for Errata 102, Errata 106, and Errata 107.
    * Removed support for nRF52832 revision 2 workarounds for Errata 143.

nRF5340 SoC
-----------

* Added:

  * ``esb`` subsystem - Added support for nRF5340 SoC (CPUNET) in ESB subsystem.

* Updated:

  * ``bl_boot`` library - Disabled clock interrupts before booting the application.
    This change fixes an issue where the :ref:`bootloader` sample would not be able to boot a Zephyr application on the nRF5340 SoC.

Connected Home over IP (CHIP)
-----------------------------

* CHIP is now supported for development as an |NCS| submodule for Windows, Linux, and macOS.
* Added:

  * :ref:`ug_chip` protocol user guide
  * :ref:`Bolt lock <chip_lock_sample>` sample
  * :ref:`Light switch <chip_light_switch_sample>` sample
  * :ref:`Light bulb <chip_light_bulb_sample>` sample

DFU Target
----------

* Renamed ``dfu_target_modem`` to ``dfu_target_modem_delta``.
* Moved all ``dfu_target`` code up one directory from :file:`subsys/dfu` to :file:`subsys/dfu/dfu_target`.
* Extracted stream flash functionality from ``dfu_target_mcuboot`` into ``dfu_target_stream_flash`` to facilitate code re-use for other ``dfu_targets`` which writes large objects to flash.

HTTP Update
-----------

* Added:

  * New sample :ref:`http_modem_delta_update_sample` which shows how to add modem delta upgrade support to an application.

* Updated:

  * Extracted certificate, button, and LED handling functionality from :ref:`http_application_update_sample` to :file:`samples/nrf9160/http_update/common`, to share them with :ref:`http_modem_delta_update_sample`.
  * Moved the :ref:`http_application_update_sample` sample from :file:`samples/nrf9160/http_application_update/` to :file:`samples/nrf9160/http_update/application_update`

Thread
------

* Added:

  * Development support for the nRF5340 DK in single-protocol configuration for the :ref:`ot_cli_sample`, :ref:`coap_client_sample`, and :ref:`coap_server_sample` samples.
  * Development support for nRF21540 FEM (front-end module) in the Thread samples.

* Optimized ROM and RAM used by Thread samples.
* Disabled Hardware Flow Control on the serial port in :ref:`coap_client_sample` and :ref:`coap_server_sample` samples.

Zigbee
------

* Added:

  * Development support for the nRF5340 DK in single-protocol configuration for the :ref:`zigbee_light_switch_sample`, :ref:`zigbee_light_bulb_sample`, and :ref:`zigbee_network_coordinator_sample` samples.
  * New ``zcl ping`` command in the :ref:`lib_zigbee_shell` library.
  * New libraries there were extracted from common code under :file:`subsys/zigbee/common`:

    * :ref:`lib_zigbee_application_utilities` library
    * :ref:`lib_zigbee_logger_endpoint` library

* Updated:

  * Updated :ref:`zboss` to version ``3_3_0_6+11_30_2020``.
    See :ref:`nrfxlib:zboss_changelog` for detailed information.

Bluetooth Mesh
--------------

* Added:

  * Time client model callbacks for all message types.
  * Support for the nRF52833 DK in the :ref:`bluetooth_mesh_light` and :ref:`bluetooth_mesh_light_switch` samples.
  * Support for :ref:`bt_mesh_light_xyl_readme`.

* Updated the :ref:`bt_mesh_lightness_srv_readme` to disable the light control server when manual control has taken effect.
  This follows the Mesh Model Specification section 6.2.3.

nRF9160
=======

* Updated:

  * :ref:`nrfxlib:nrf_modem` - BSD library has been renamed to ``nrf_modem`` (Modem library) and ``nrf_modem_lib`` (glue).
  * :ref:`lib_download_client` library:

    * Re-introduced optional TCP timeout (enabled by default) on the TCP socket used for the download.
      Upon timeout on a TCP socket, the HTTP download will fail and the ``ETIMEDOUT`` error will be returned via the callback handler.
    * Added an option to set the hostname for TLS Server Name Indication (SNI) extension.
      This option is valid only when TLS is enabled.

  * :ref:`lib_date_time` library - Added an API to check if the Date-Time library has obtained a valid date-time.
    If the function returns false, it implies that the library has not yet obtained valid date-time to base its calculations and time conversions on and hence other API calls that depend on the internal date-time will fail.

  * :ref:`serial_lte_modem` application:

    * Fixed an issue where FOTA downloads were interrupted if an AT command was issued.
    * Fixed an issue with overflowing HTTP request buffers.
    * Fixed issues with TCP/UDP server restart.
    * Added support for allowing only specified TCP/TLS client IP addresses (using the #XTCPFILTER command).

  * :ref:`lib_fota_download` library:

    * Enabled SNI TLS extension for HTTPS downloads.

  * :ref:`lib_nrf_cloud` library:

    * AWS Jobs replaced by nRF Connect for Cloud FOTA as the FOTA mechanism for devices connected to nRF Connect for Cloud.
    * Removed :option:`CONFIG_CLOUD_API` dependency from :option:`CONFIG_NRF_CLOUD_CONNECTION_POLL_THREAD`.
    * Added a new API :c:func:`nrf_cloud_send` that can be used for sending pre-encoded data to specified endpoint topics in nRF Connect for Cloud.

  * :ref:`asset_tracker` application:

    * Updated to use the new FOTA (fota_v2) based on nRF Connect for Cloud.

  * :ref:`at_cmd_parser_readme` library:

    * The library can now parse AT command strings with negative numbers in the range supported by the int32_t type.

  * :ref:`lwm2m_client` sample:

    * Added handling of network failures.
      The sample now tries to reconnect to the LTE network when a failure is reported.

  * :ref:`lib_azure_iot_hub` library:

    * Improved internal connection state handling.

  * :ref:`nrf_coap_client_sample` sample:

    * Updated the default server URL to ``californium.eclipseprojects.io``.

  * :ref:`mqtt_simple_sample` sample:

    * Updated the default server URL to ``mqtt.eclipseprojects.io``.

Common
======

The following changes are relevant for all device families.

sdk-nrfxlib
-----------

See the changelog for each library in the :doc:`nrfxlib documentation <nrfxlib:README>` for the most current information.

Crypto
~~~~~~

* Added:

  * nrf_cc3xx_platform v0.9.5, with the following highlights:

    * Added correct TRNG characterization values for nRF5340 devices.

    See the :ref:`crypto_changelog_nrf_cc3xx_platform` for detailed information.
  * nrf_cc3xx_mbedcrypto version v0.9.5, with the following highlights:

    * Built to match the nrf_cc3xx_platform v0.9.5 including correct TRNG characterization values for nRF5340 devices.

    See the :ref:`crypto_changelog_nrf_cc3xx_mbedcrypto` for detailed information.

* Updated:

  * Rewrote the :ref:`nrfxlib:nrf_security`'s library stripping mechanism to not use the POST_BUILD option in a custom build rule.
    The library stripping mechanism was non-functional in certain versions of SEGGER Embedded Studio Nordic Edition.

BSD library
~~~~~~~~~~~

* Added information about low accuracy mode to the :ref:`nrfxlib:gnss_extension` documentation.
* Added mutex protection for the :c:func:`nrf_getaddrinfo` function.


Trusted Firmware-M:
-------------------

* Added a simple sample that demonstrates how to integrate TF-M in an application.

Partition Manager:
------------------

* Changed naming convention for partition names in ``pm.yml`` and ``pm_static.yml``.
* Updated Partition Manager to prevent users from using partition names in ``pm.yml`` and ``pm_static.yml`` that match the names of the child images that define them in ``CMakeLists.txt``:

  * If the invalid naming scheme is used in ``pm.yml`` files, Partition Manager will now fail the builds.
  * If the invalid naming scheme is used in ``pm_static.yml`` files, the build will instead print a warning prompting the user to change this, if possible.
* Renamed ``b0`` and ``b0n`` container partitions to ``b0_provision`` and ``b0n_provision``, respectively.
* Renamed ``b0_image`` and ``b0n_image`` image partitions to appropriately match their child image name, ``b0`` and ``b0n``, respectively.

  **Migration notes:** While in development, you should rename partitions appropriately.
  You can still build firmware updates under the invalid scheme, but they will still be built with the improper sizes for the related partitions.

MCUboot
=======

The MCUboot fork in |NCS| (``sdk-mcuboot``) contains all commits from the upstream MCUboot repository up to and including ``3f49b5abf3``, plus some |NCS| specific additions.

The code for integrating MCUboot into |NCS| is located in :file:`ncs/nrf/modules/mcuboot`.

The following list summarizes the most important changes inherited from upstream MCUboot:

* Bootloader:

  * Added hardening against hardware level fault injection and timing attacks.
    See ``CONFIG_BOOT_FIH_PROFILE_HIGH`` and similar Kconfig options.
  * Introduced abstract crypto primitives to simplify porting.
  * Added ram-load upgrade mode (not enabled for Zephyr yet).
  * Renamed single-image mode to single-slot mode.
    See the ``CONFIG_SINGLE_APPLICATION_SLOT`` option.
  * Added a patch for turning off cache for Cortex-M7 before chain-loading.
  * Fixed an issue that caused HW stack protection to catch the chain-loaded application during its early initialization.
  * Added reset of Cortex SPLIM registers before boot.
  * Fixed a build issue that occurred if the CONF_FILE contained multiple file paths instead of a single file path.
  * Added watchdog feed on nRF devices.
    See the ``CONFIG_BOOT_WATCHDOG_FEED`` option.
  * Removed the ``flash_area_read_is_empty()`` port implementation function.
  * Updated the ARM core configuration to only be initialized when selected by the user.
    See the ``CONFIG_MCUBOOT_CLEANUP_ARM_CORE`` option.
  * Allowed the final data chunk in the image to be unaligned in the serial-recovery protocol.

* Image tool:

  * Updated the tool to print an image digest during verification.
  * Added a possibility to set a confirm flag for HEX files as well.
  * Updated the usage of ``--confirm`` to imply ``--pad``.
  * Fixed the argument handling of ``custom_tlvs``.


Mcumgr
======

The mcumgr library contains all commits from the upstream mcumgr repository up to and including snapshot ``74e77ad08``.

The following list summarizes the most important changes inherited from upstream mcumgr:

* Fixed an issue with devices running MCUboot v1.6.0 or earlier where a power outage during erase of a corrupted image in slot 1 could result in the device not being able to boot.
  In this case, it was not possible to update the device and mcumgr would return error code 6 (``MGMT_ERR_EBADSTATE``).
* Added support for invoking shell commands (shell management) from the mcumgr command line.


Zephyr
======

.. NOTE TO MAINTAINERS: The latest Zephyr commit appears in multiple places; make sure you update them all.

The Zephyr fork in |NCS| (``sdk-zephyr``) contains all commits from the upstream Zephyr repository up to and including ``ff720cd9b343``, plus some |NCS| specific additions.

For a complete list of upstream Zephyr commits incorporated into |NCS| since the most recent release, run the following command from the :file:`ncs/zephyr` repository (after running ``west update``):

.. code-block:: none

   git log --oneline ff720cd9b343 ^v2.4.0-ncs1

For a complete list of |NCS| specific commits, run:

.. code-block:: none

   git log --oneline manifest-rev ^ff720cd9b343

The current |NCS| release is based on Zephyr v2.4.99.

The following list summarizes the most important changes inherited from upstream Zephyr:

* Architectures:

  * Enabled interrupts before ``main()`` in single-thread kernel mode for Cortex-M architecture.
  * Introduced functionality for forcing core architecture HW initialization during system boot, for chain-loadable images.

* Boards:

  * Fixed arguments for the J-Link runners for nRF5340 DK and added the DAP Link (CMSIS-DAP) interface to the OpenOCD runner for nRF5340.
  * Marked the nRF5340 PDK as deprecated and updated the nRF5340 documentation to point to the :ref:`zephyr:nrf5340dk_nrf5340`.
  * Added enabling of LFXO pins (XL1 and XL2) for nRF5340.
  * Removed non-existing documentation links from partition definitions in the board devicetree files.
  * Updated documentation related to QSPI use.

* Kernel:

  * Restricted thread-local storage, which is now available only when the toolchain supports it.
    Toolchain support is initially limited to the toolchains bundled with the Zephyr SDK.
  * Added support for gathering basic thread runtime statistics.
  * Removed the following deprecated `kernel APIs <https://github.com/nrfconnect/sdk-zephyr/commit/c8b94f468a94c9d8d6e6e94013aaef00b914f75b>`_:

    * ``k_enable_sys_clock_always_on()``
    * ``k_disable_sys_clock_always_on()``
    * ``k_uptime_delta_32()``
    * ``K_FIFO_INITIALIZER``
    * ``K_LIFO_INITIALIZER``
    * ``K_MBOX_INITIALIZER``
    * ``K_MEM_SLAB_INITIALIZER``
    * ``K_MSGQ_INITIALIZER``
    * ``K_MUTEX_INITIALIZER``
    * ``K_PIPE_INITIALIZER``
    * ``K_SEM_INITIALIZER``
    * ``K_STACK_INITIALIZER``
    * ``K_TIMER_INITIALIZER``
    * ``K_WORK_INITIALIZER``
    * ``K_QUEUE_INITIALIZER``

  * Removed the following deprecated `system clock APIs <https://github.com/nrfconnect/sdk-zephyr/commit/d28f04110dcc7d1aadf1d791088af9aca467bd70>`_:

    * ``__ticks_to_ms()``
    * ``__ticks_to_us()``
    * ``sys_clock_hw_cycles_per_tick()``
    * ``z_us_to_ticks()``
    * ``SYS_CLOCK_HW_CYCLES_TO_NS64()``
    * ``SYS_CLOCK_HW_CYCLES_TO_NS()``

  * Removed the deprecated ``CONFIG_LEGACY_TIMEOUT_API`` option.
    All time-outs must now be specified using the ``k_timeout_t`` type.

  * Updated :c:func:`k_timer_user_data_get` to take a ``const struct k_timer *timer`` instead of a non-\ ``const`` pointer.
  * Added a :c:macro:`K_DELAYED_WORK_DEFINE` macro.
  * Added a :option:`CONFIG_MEM_SLAB_TRACE_MAX_UTILIZATION` option.
    If enabled, :c:func:`k_mem_slab_max_used_get` can be used to get a memory slab's maximum utilization in blocks.

  * Bug fixes:

    * Fixed a race condition between :c:func:`k_queue_append` and :c:func:`k_queue_alloc_append`.
    * Updated the kernel to no longer try to resume threads that are not suspended.
    * Updated the kernel to no longer attempt to queue threads that are already in the run queue.
    * Updated :c:func:`k_busy_wait` to return immediately on a zero time-out, and improved accuracy on nonzero time-outs.
    * The idle loop no longer unlocks and locks IRQs.
      This avoids a race condition; see `Zephyr issue 30573 <https://github.com/zephyrproject-rtos/zephyr/issues/30573>`_.
    * An arithmetic overflow that prevented long sleep times or absolute time-outs from working properly has been fixed; see `Zephyr issue #29066 <https://github.com/zephyrproject-rtos/zephyr/issues/29066>`_.
    * A logging issue where some kernel debug logs could not be removed was fixed; see `Zephyr issue #28955 <https://github.com/zephyrproject-rtos/zephyr/issues/28955>`_.

* Devicetree:

  * Removed the legacy DT macros.
  * Started exposing dependency ordinals for walking the dependency hierarchy.
  * Added documentation for the :ref:`DTS bindings <zephyr:devicetree_binding_index>`.

* Drivers:

  * Deprecated the ``DEVICE_INIT()`` macro.
    Use :c:macro:`DEVICE_DEFINE` instead.

  * ADC:

    * Improved the default routine that provides sampling intervals, to allow intervals shorter than 1 millisecond.

  * Bluetooth Controller:

    * Fixed and improved an issue where a connection event closed too early when more data could have been sent in the same connection event.
    * Fixed missing slave latency cancellation when initiating control procedures.
      Connection terminations are faster now.
    * Added experimental support for non-connectable non-scannable Extended Advertising with 255 byte PDU (without chaining).
    * Added experimental support for non-connectable scannable Extended Advertising with 255 byte PDU (without chaining).
    * Added experimental support for Extended Scanning with duration and period parameters (without active scanning for scan response or chained PDU).
    * Added experimental support for Periodic Advertising and Periodic Advertising Synchronization Establishment.

  * Bluetooth Host:

    * Updated the :c:enumerator:`BT_LE_ADV_OPT_DIR_ADDR_RPA` option.
      It must now be set when advertising towards a privacy-enabled peer, independent of whether privacy has been enabled or disabled.
    * Updated the signature of the :c:type:`bt_gatt_indicate_func_t` callback type by replacing the ``attr`` pointer with a pointer to the :c:struct:`bt_gatt_indicate_params` struct that was used to start the indication.
    * Added a destroy callback to the :c:struct:`bt_gatt_indicate_params` struct, which is called when the struct is no longer referenced by the stack.
    * Added advertising options to disable individual advertising channels.
    * Added experimental support for Periodic Advertising Sync Transfer.
    * Added experimental support for Periodic Advertising List.
    * Changed the permission bits in the discovery callback to always be set to zero since this is not valid information.
    * Fixed a regression in lazy loading of the Client Configuration Characteristics.
    * Fixed an issue where a security procedure failure could terminate the current GATT transaction when the transaction did not require security.

  * Display:

    * Added support for the ILI9488 display.
    * Refactored the ILI9340 driver to support multiple instances, rotation, and pixel format changing at runtime.
      Configuration of the driver instances is now done in devicetree.
    * Enhanced the SSD1306 driver to support communication via both SPI and I2C.

  * Flash:

    * Modified the nRF QSPI NOR driver so that it supports also nRF53 Series SoCs.

  * IEEE 802.15.4:

    * Updated the nRF5 IEEE 802.15.4 driver to version 1.9.

  * LED PWM:

    * Added a driver interface and implementation for PWM-driven LEDs.

  * Modem:

    * Reworked the command handler reading routine, to prevent data loss and reduce RAM usage.
    * Added the possibility of locking TX in the command handler.
    * Improved handling of HW flow control on the RX side of the UART interface.

  * Power:

    * Added multiple ``nrfx_power``-related fixes to reduce power consumption.

  * PWM:

    * Changed the GPIO configuration to use Nordic HAL, which allows support for GPIO pins above 31.
    * Added a check to ensure that the PWM period does not exceed a 16-bit value to prevent erroneous behavior.
    * Changed the PWM DT configuration to use a timer phandle instead of the previously used timer instance.

  * Regulator:

    * Introduced a new regulator driver infrastructure.

  * Sensor:

    * Added support for the IIS2ICLX 2-axis digital inclinometer.
    * Enhanced the BMI160 driver to support communication via both SPI and I2C.
    * Added device power management in the LIS2MDL magnetometer driver.

  * Serial:

    * Replaced the usage of ``k_delayed_work`` with ``k_timer`` in the nRF UART driver.
    * Fixed an issue in the nRF UARTE driver where spurious data could be received when the asynchronous API with hardware byte counting was used and the UART was switched back from the low power to the active state.
    * Removed the following deprecated definitions:

      * ``UART_ERROR_BREAK``
      * ``LINE_CTRL_BAUD_RATE``
      * ``LINE_CTRL_RTS``
      * ``LINE_CTRL_DTR``
      * ``LINE_CTRL_DCD``
      * ``LINE_CTRL_DSR``

  * SPI:

    * Added support for SPI emulators.

  * USB:

    * Fixed handling of zero-length packets (ZLP) in the Nordic Semiconductor USB Device Controller driver (usb_dc_nrfx).
    * Fixed initialization of the workqueue in the usb_dc_nrfx driver, to prevent fatal errors when the driver is reattached.
    * Fixed handling of the SUSPEND and RESUME events in the Bluetooth classes.
    * Made the USB DFU class compatible with the target configuration that does not have a secondary image slot.
    * Added support for using USB DFU within MCUboot with single application slot mode.


* Networking:

  * General:

    * Added support for DNS Service Discovery.
    * Deprecated legacy TCP stack (TCP1).
    * Added multiple minor TCP2 bugfixes and improvements.
    * Added network management events for DHCPv4.
    * Added periodic throughput printout to the :ref:`zephyr:sockets-echo-server-sample` sample.
    * Added an experimental option to set preemptive priority for networking threads (:option:`CONFIG_NET_TC_THREAD_PREEMPTIVE`).

  * LwM2M:

    * Made the endpoint name length configurable with Kconfig (see :option:`CONFIG_LWM2M_RD_CLIENT_ENDPOINT_NAME_MAX_LENGTH`).
    * Fixed PUSH FOTA block transfer with Opaque content format.
    * Added various improvements to the bootstrap procedure.
    * Fixed token generation.
    * Added separate response handling.
    * Fixed Registration Update to be sent on lifetime update, as required by the specification.
    * Added a new event (:c:enumerator:`LWM2M_RD_CLIENT_EVENT_NETWORK_ERROR`) that notifies the application about underlying socket errors.
      The event is reported after several failed registration attempts.
    * Improved integers packing in TLVs.
    * Added support for arguments of the LwM2M execute command.
    * Fixed buffer length check in :c:func:`lwm2m_engine_set`.
    * Added a possibility to acknowledge LwM2M requests early from the callback (:c:func:`lwm2m_acknowledge`).
    * Reworked the Bootstrap Delete operation to support all cases defined by the LwM2M specification.

  * OpenThread:

    * Updated the OpenThread version to commit ``69e97581e71a340776493dd9f5b65e11caec7954``.
    * Removed obsolete flash driver from the OpenThread platform.
    * Added new OpenThread options:

      * :option:`CONFIG_OPENTHREAD_NCP_BUFFER_SIZE`
      * :option:`CONFIG_OPENTHREAD_NUM_MESSAGE_BUFFERS`
      * :option:`CONFIG_OPENTHREAD_MAX_STATECHANGE_HANDLERS`
      * :option:`CONFIG_OPENTHREAD_TMF_ADDRESS_CACHE_ENTRIES`
      * :option:`CONFIG_OPENTHREAD_MAX_CHILDREN`
      * :option:`CONFIG_OPENTHREAD_MAX_IP_ADDR_PER_CHILD`
      * :option:`CONFIG_OPENTHREAD_LOG_PREPEND_LEVEL_ENABLE`
      * :option:`CONFIG_OPENTHREAD_MAC_SOFTWARE_ACK_TIMEOUT_ENABLE`
      * :option:`CONFIG_OPENTHREAD_MAC_SOFTWARE_RETRANSMIT_ENABLE`
      * :option:`CONFIG_OPENTHREAD_PLATFORM_USEC_TIMER_ENABLE`
      * :option:`CONFIG_OPENTHREAD_RADIO_LINK_IEEE_802_15_4_ENABLE`
      * :option:`CONFIG_OPENTHREAD_RADIO_LINK_TREL_ENABLE`
      * :option:`CONFIG_OPENTHREAD_CSL_SAMPLE_WINDOW`
      * :option:`CONFIG_OPENTHREAD_CSL_RECEIVE_TIME_AHEAD`
      * :option:`CONFIG_OPENTHREAD_MAC_SOFTWARE_CSMA_BACKOFF_ENABLE`
      * :option:`CONFIG_OPENTHREAD_PLATFORM_INFO`

    * Added support for RCP co-processor mode.

  * MQTT:

    * Fixed mutex protection on :c:func:`mqtt_disconnect`.
    * Switched the library to use ``zsock_*`` socket functions instead of POSIX names.
    * Changed the return value of :c:func:`mqtt_keepalive_time_left` to -1 when keep alive is disabled.

  * Sockets:

    * Enabled Maximum Fragment Length (MFL) extension on TLS sockets.
    * Added a :c:macro:`TLS_ALPN_LIST` socket option for TLS sockets.
    * Fixed a ``tls_context`` leak on ``ztls_socket()`` failure.
    * Fixed ``getaddrinfo()`` hints handling with AI_PASSIVE flag.

* Bluetooth Mesh:

  * Replaced the Configuration Server structure with Kconfig entries and a standalone Heartbeat API.
  * Added a separate API for adding keys and configuring features locally.
  * Fixed a potential infinite loop in model extension tree walk.
  * Added LPN and Friendship event handler callbacks.
  * Created separate internal submodules for keys, labels, Heartbeat, replay protection, and feature management.
  * :ref:`bluetooth_mesh_models_cfg_cli`:

    * Added an API for resetting a node (:c:func:`bt_mesh_cfg_node_reset`).
    * Added an API for setting network transmit parameters (:c:func:`bt_mesh_cfg_net_transmit_set`).


* Libraries/subsystems:

  * Settings:

    * Removed SETTINGS_USE_BASE64 support, which has been deprecated for more than two releases.

  * Storage:

    * :ref:`flash_map_api`: Added an API to get the value of an erased byte in the flash_area.
      See :c:func:`flash_area_erased_val`.
    * :ref:`stream_flash`: Eliminated the usage of the flash API internals.


  * File systems:

    * Enabled FCB to work with non-0xff erase value flash.
    * Added a :c:macro:`FS_MOUNT_FLAG_NO_FORMAT` flag to the FatFs options.
      This flag removes formatting capabilities from the FAT/exFAT file system driver and prevents unformatted devices to be formatted, to FAT or exFAT, on mount attempt.
    * Added support for the following :c:func:`fs_mount` flags: :c:macro:`FS_MOUNT_FLAG_READ_ONLY`, :c:macro:`FS_MOUNT_FLAG_NO_FORMAT`
    * Updated the FS API to not perform a runtime check of a driver interface when the :option:`CONFIG_NO_RUNTIME_CHECKS` option is enabled.

  * DFU:

    * Added shell module for MCUboot enabled application.
      See :option:`CONFIG_MCUBOOT_SHELL`.

* Build system:

  * Ensured that shields can be placed in other BOARD_ROOT folders.
  * Added basic support for Clang 10 with x86.
  * Fixed a bug that prevented compiling the :ref:`bootloader` with :option:`CONFIG_SB_SIGNING_PUBLIC_KEY`

* System:

  * Added an API that provides a printf family of functions (for example, :c:func:`cbprintf`) with a callback on character output, to perform in-place streaming of the formatted string.
  * Updated minimal libc to print stderr just like stdout.
  * Added an ``abort()`` function to minimal libc.
  * Updated the ring buffer to allow using the full buffer capacity instead of forcing an empty slot.
  * Added a :c:macro:`CLAMP` macro.
  * Added a feature for post-mortem analysis to the tracing library.

* Samples:

  * Added :ref:`zephyr:nrf-ieee802154-rpmsg-sample`.
  * Added :ref:`zephyr:cloud-tagoio-http-post-sample`.
  * Added :ref:`zephyr:civetweb-websocket-server-sample`.
  * :ref:`zephyr:led_ws2812_sample`: Updated to force SPIM on nRF52 DK.
  * :ref:`zephyr:cfb_custom_fonts`: Added support for ssd1306fb.
  * :ref:`zephyr:gsm-modem-sample`: Added suspend/resume shell commands.

* Logging:

  * Added STP transport and raw data output support for systrace.

* Modules:

  * Introduced a :option:`CONFIG_MBEDTLS_MEMORY_DEBUG` option for mbedtls.
  * Updated LVGL to v7.6.1.
  * Updated libmetal and openamp to v2020.10.
  * Updated nrfx in hal-nordic to version 2.4.0.
  * Updated the Trusted Firmware-M (TF-M) module to include support for the nRF5340 and nRF9160 platforms.


* Other:

  * Added initial LoRaWAN support.
  * Updated ``west flash`` support for ``nrfjprog`` to fail if a HEX file has UICR data and ``--erase`` was not specified.

Documentation
=============

In addition to documentation related to the changes listed above, the following documentation has been updated:

Samples
-------

* :ref:`zigbee_samples` - updated the structure to match the template

User guides
-----------

* :ref:`ug_nrf52` - updated with information about support for CHIP

Known issues
************

Known issues are only tracked for the latest official release.
See `known issues for nRF Connect SDK v1.4.2`_ for the list of issues valid for this release.
