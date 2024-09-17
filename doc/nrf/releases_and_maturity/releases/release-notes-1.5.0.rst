.. _ncs_release_notes_150:

|NCS| v1.5.0 Release Notes
##########################

.. contents::
   :local:
   :depth: 2

|NCS| delivers reference software and supporting libraries for developing low-power wireless applications with Nordic Semiconductor products.
It includes the TF-M, MCUboot and the Zephyr RTOS open source projects, which are continuously integrated and re-distributed with the SDK.

The |NCS| is where you begin building low-power wireless applications with Nordic Semiconductor nRF52, nRF53, and nRF91 Series devices.
nRF53 Series devices are now supported for production.
Wireless protocol stacks and libraries may indicate support for development or support for production for different series of devices based on verification and certification status. See the release notes and documentation for those protocols and libraries for further information.

Highlights
**********

* Introduced support for nRF5340 DK.
* Added minimal configuration to Bluetooth: Peripheral UART sample for nRF52810 and nRF52811 SoCs.
* All the models in Bluetooth Mesh Models v1.0.x are now supported for development.
* Added experimental support for Trusted Firmware-M (TF-M) for nRF5340 and nRF9160.
* Integrated Apple HomeKit ecosystem support into the |NCS|.
* Integrated Project Connected Home over IP, which is supported for development.
* Added production support for nRF5340 for Thread in single protocol configuration, and development support for Thread in multiprotocol operation with Bluetooth LE.
* Added development support for nRF5340 for Zigbee, including multiprotocol operation with Bluetooth LE.
* Added nRF9160: Asset Tracker v2 as an ultra-low-power, real-time configurable application.
* Implemented major updates to Modem library (previously called BSD library).
* Added support for full modem DFU for nRF9160, over-the-air (OTA) and via UART.
* Introduced support for Edge Impulse machine learning models.

Release tag
***********

The release tag for the |NCS| manifest repository (|ncs_repo|) is **v1.5.0**.
Check the :file:`west.yml` file for the corresponding tags in the project repositories.

To use this release, check out the tag in the manifest repository and run ``west update``.
See :ref:`cloning_the_repositories` for more information.

Supported modem firmware
************************

This version of the |NCS| has been tested with the following modem firmware for cellular IoT applications:

* mfw_nrf9160_1.2.3
* mfw_nrf9160_1.1.4


Use the latest version of the nRF Programmer app of `nRF Connect for Desktop`_ to update the modem firmware.
See :ref:`nrf9160_gs_updating_fw_modem` for instructions.

Changelog
*********

The following sections provide detailed lists of changes by component.

nRF9160
=======

* Added:

  * :ref:`asset_tracker_v2` application, which is a low-power asset tracking example for nRF9160 DK and Thingy:91.
  * :ref:`fmfu_smp_svr_sample` sample, which shows how to add the full modem serial update functionality to an application using SMP Server.
  * :ref:`http_modem_full_update_sample` sample, which demonstrates how to add full modem upgrade support to an application. Note that this requires an external flash memory of a minimum of 2 MB to work, hence the sample will only work on the nRF9160 DK v0.14.0 or later.
  * :ref:`http_modem_delta_update_sample` sample, which demonstrates how to add delta modem upgrade support to an application.
  * :ref:`lib_fmfu_mgmt`, which is a new library that implements parts of the mcumgr management protocol for doing full modem serial updates.

* Updated:

  * nRF9160: Asset Tracker application - Updated to use the new FOTA (fota_v2) based on nRF Connect for Cloud.
  * :ref:`serial_lte_modem` application:

    * Fixed an issue where FOTA downloads were interrupted if an AT command was issued.
    * Fixed an issue with overflowing HTTP request buffers.
    * Fixed issues with TCP/UDP server restart.
    * Added support for allowing only specified TCP/TLS client IP addresses (using the #XTCPFILTER command).
    * Fixed the unsolicited result code (URC) message format following the 3GPP TS27.007 specification.
    * Fixed a bug that occurred when the size of an AT command was exactly the maximum buffer size.
    * Optimized SRAM usage.
    * Disabled external XTAL usage for UARTE by default.
    * Added a recovery mechanism for cases where a UART frame error happens.
    * Enhanced data mode support for TCP/UDP proxy.

  * :ref:`lwm2m_client` sample - Added handling of network failures. The sample now tries to reconnect to the LTE network when a failure is reported.
  * :ref:`nrf_coap_client_sample` sample - Updated the default server URL to ``californium.eclipseprojects.io``.
  * nRF9160: Simple MQTT sample - Updated the default server URL to ``mqtt.eclipseprojects.io``.
  * Extracted the certificate, button, and LED handling functionality from :ref:`http_application_update_sample` sample to :file:`samples/nrf9160/http_update/common`, to share them with :ref:`http_modem_delta_update_sample` sample.
  * Moved the :ref:`http_application_update_sample` sample from ``samples/nrf9160/http_application_update/`` to ``samples/nrf9160/http_update/application_update``.
  * :ref:`lib_download_client` library:

    * Reintroduced the optional TCP timeout (enabled by default) on the TCP socket that is used for the download.
      Upon timeout on a TCP socket, the HTTP download will fail and the ``ETIMEDOUT`` error will be returned via the callback handler.
    * Added an option to set the hostname for TLS Server Name Indication (SNI) extension.
      This option is valid only when TLS is enabled.

  * :ref:`lib_date_time` library :

    * Added an API to check if the Date-Time library has obtained a valid date-time. If the function returns false, it implies that the library has not yet obtained valid date-time to base its calculations and time conversions on, and hence other API calls that depend on the internal date-time will fail.


  * :ref:`lib_fota_download` library - Enabled SNI TLS extension for HTTPS downloads.
  * :ref:`lib_nrf_cloud` library:

    * nRF Connect for Cloud FOTA replaced AWS Jobs as the FOTA mechanism for devices connected to nRF Connect for Cloud.
    * Removed :kconfig:option:`CONFIG_CLOUD_API` dependency from :kconfig:option:`CONFIG_NRF_CLOUD_CONNECTION_POLL_THREAD`.
    * Added a new API :c:func:`nrf_cloud_send` that can be used for sending pre-encoded data to specified endpoint topics in nRF Connect for Cloud.

  * :ref:`at_cmd_parser_readme` library - The library can now parse AT command strings with negative numbers in the range supported by the int32_t type.
  * :ref:`lib_azure_iot_hub` library - Improved the internal connection state handling.
  * :ref:`lte_lc_readme` library - Added support for manufacturer-specific default eDRX/PSM values.
  * :ref:`liblwm2m_carrier_readme` library - Updated to v0.10.2. See :ref:`liblwm2m_carrier_changelog` for details.

* Removed:

  * USB-UART bridge sample

DFU target
----------

* Added:

  * New target ``dfu_target_full_modem``, which is used for full modem firmware updates. This requires an external flash memory of a minimum of 2 MB to work.

* Updated:

  * Renamed ``dfu_target_modem`` to ``dfu_target_modem_delta``.
  * Moved the ``dfu_target`` code from :file:`subsys/dfu` to :file:`subsys/dfu/dfu_target`.
  * Extracted the stream flash memory functionality from ``dfu_target_mcuboot`` to ``dfu_target_stream_flash`` to facilitate code reuse for other ``dfu_targets``, which write large objects to the flash memory.

nRF5
====

The following changes are relevant for the nRF52 and nRF53 Series.

nRF52832 SoC
------------

* Updated:

  * Removed support for nRF52832 revision 1 workarounds in :ref:`esb_readme` for Errata 102, Errata 106, and Errata 107.

nRF5340 SoC
-----------

* Added:

  * :ref:`multiprotocol-rpmsg-sample` sample for network core, which enables support for multiprotocol, IEEE 802.15.4, and Bluetooth LE applications.

* Updated:


  * :ref:`esb_readme` subsystem - Added support for nRF5340 (CPUNET) in the ESB subsystem.
  * Secure Partition Manager (SPM) subsystem - Added support for nRF5340 peripherals in non-secure applications.
  * :ref:`ble_samples` - Added configuration overlays for child image to the required Bluetooth LE samples so that no Kconfig updates in the :zephyr:code-sample:`bluetooth_hci_ipc` sample are needed by default.
  * :ref:`nrf5340_empty_app_core` sample - Disabled the kernel memory pool option :kconfig:option:`CONFIG_KERNEL_MEM_POOL` to reduce the memory footprint.
  * ``bl_boot`` library - Disabled clock interrupts before booting the application. This change fixes an issue where the :ref:`bootloader` sample would not be able to boot a Zephyr application on the nRF5340 SoC.


Front-end module (FEM)
----------------------

* Added support for nRF21540 revision 1 or older to :ref:`direct_test_mode` and :ref:`radio_test` samples.
* Added support for RF front-end Modules (FEM) in :ref:`mpsl` library. The front-end module feature in MPSL currently supports the SKY66112-11 device, but does not support nRF21540 revision 1 or older.


Bluetooth LE
------------

* Added:

  * Support for nRF21540 front-end module (revision 1 or older) to :ref:`direct_test_mode` sample.
  * Minimal configuration (:file:`prj_minimal.conf`) to the :ref:`peripheral_uart` sample, thus enabling support for building for nRF52810 and nRF52811 devices.
  * :ref:`cts_client_readme` service that is used to retrieve the current time from a connected peer that is running the GATT server with the `Current Time Service <Current Time Service Specification_>`_.
  * :ref:`peripheral_cts_client` sample that demonstrates how to use the :ref:`cts_client_readme`.

* Updated:

  * :ref:`ble_throughput` sample:

    * Uses :ref:`Zephyr's Shell <zephyr:shell_api>` for setting up the test parameters.
    * Role selection can be done using buttons instead of UART.
    * Fixed the throughput calculation on the application level by removing the dependency from remote terminal speed (disabled UART flow control).

  * :ref:`bluetooth_central_hids` sample - Restored *numeric comparison* pairing.

Project Connected Home over IP (Project CHIP)
---------------------------------------------

* Project CHIP is now supported for development as an |NCS| submodule for Windows, Linux, and macOS.
* Added:

  * :ref:`ug_chip` protocol user guide
  * :ref:`Project CHIP door lock <chip_lock_sample>` sample
  * :ref:`Project CHIP light switch <chip_light_switch_sample>` sample
  * :ref:`Project CHIP light bulb <chip_light_bulb_sample>` sample

Bluetooth Mesh
--------------

* Added:

  * Time client model callbacks for all message types.
  * Support for the nRF52833 DK in the :ref:`bluetooth_mesh_light` and :ref:`bluetooth_mesh_light_switch` samples.
  * Support for the following mesh models:

    * :ref:`bt_mesh_light_xyl_readme`
    * :ref:`bt_mesh_light_hsl_readme`
    * :ref:`bt_mesh_scheduler_readme`
  * Scene Current/Target Scene Get API (Gets the current/target scene that the scene server is set to).
  * Sensor Client All Get API (Reads sensor data from all sensors on a server).
  * Generic Client Properties Get API (Gets the list of Generic Client Properties on the bound server).

* Updated:

  * :ref:`bt_mesh_lightness_srv_readme` to disable the light control server when manual control has taken effect.
    This follows the Mesh Model Specification section 6.2.3.
  * Deleting a non-existing scene from the Scene Server returns success, instead of not found.
  * Removed the Light CTL setup server publications, which was not in use.
  * Disabled light control on all changes in lightness.
  * Added model reset callbacks so that the Mesh stack can be reset.
  * Implemented missing Light Linear Level Get/Set API functions.
  * Fixed several bugs.

Apple HomeKit Ecosystem support
-------------------------------

* Integrated HomeKit Accessory Development Kit (ADK) v5.1.
  MFi licensees can get access to the HomeKit repository by contacting us via Nordic `DevZone`_ private ticket.
* Enabled Thread certification by inheritance.
* HomeKit samples now use qualified Bluetooth LE libraries.

nRF IEEE 802.15.4 radio driver
------------------------------

* Production support for nRF5340 in single protocol configuration.
* Development support for nRF5340 in multiprotocol configuration (IEEE 802.15.4 and Bluetooth LE).
* Added PA/LNA GPIO interface support for RF front-end modules (FEM) in the radio driver. The front-end module feature in the radio driver currently has support for the SKY66112-11 device, but does not support nRF21540 revision 1 or older.

Thread
------

* Added:

  * Production support for nRF5340 in single protocol configuration.
  * Development support for nRF5340 in multiprotocol configuration (Thread and Bluetooth LE).
  * Support for nRF5340 for all samples except the :ref:`ot_coprocessor_sample` sample.
  * PA/LNA GPIO interface support for RF front-end modules (FEM) in Thread. The front-end module feature in Thread currently has support for SKY66112-11 device, but does not support nRF21540 revision 1 or older.

* Updated:

  * Optimized ROM and RAM used by Thread samples.
  * Disabled Hardware Flow Control on the serial port in :ref:`coap_client_sample` and :ref:`coap_server_sample` samples.
  * Thread 1.1 pre-built libraries:

    * Included the latest OpenThread changes.
    * Added libraries for nRF5340 platform.
    * Libraries will be certified after the release in multiple feature variants (certification IDs and status can be found in the compatibility matrices in the `Nordic Semiconductor Infocenter`_).

Zigbee
------

* Added:

  * Development support for :ref:`NCP (Network Co-Processor) <ug_zigbee_platform_design_ncp_details>`.
  * Development support for the nRF5340 DK in single and multi-protocol (Zigbee and Bluetooth LE) configuration for the :ref:`zigbee_light_switch_sample`, :ref:`zigbee_light_bulb_sample`, and :ref:`zigbee_network_coordinator_sample` samples.
  * PA/LNA GPIO interface support for RF front-end modules (FEM) in Zigbee. The front-end module feature in Zigbee currently has support for SKY66112-11 device, but does not support nRF21540 revision 1 or older.
  * :ref:`zigbee_ncp_sample` sample, which is a Network Co-Processor example for nRF52840 (DK and dongle) and nRF52833 DK.

	* Supports USB and UART transports.
	* Enables USB DFU when USB transport is used.

  * New ``zcl ping`` command in the :ref:`lib_zigbee_shell` library.
  * New libraries that were extracted from common code under :file:`subsys/zigbee/common`:

    * :ref:`lib_zigbee_application_utilities` library
    * :ref:`lib_zigbee_logger_endpoint` library

* Updated:

  * ZBOSS Zigbee stack to version 3_3_0_6+11_30_2020. See :ref:`zboss_configuration` for detailed information.
  * Added development (unstable) :ref:`zboss` libraries (v3.5.0.0). See :ref:`zboss_configuration` for detailed information.

nRF Desktop
-----------

Added selective HID report subscription in the USB state.
This allows the USB HID instance on the peripheral to subscribe only for a subset of HID reports.
If USB does not subscribe for the given HID report, Bluetooth LE HIDs can still receive it.

Common
======

The following changes are relevant for all device families.

Edge Impulse
------------

* Added :ref:`ei_wrapper` library that runs the machine learning model.
* Added :ref:`ei_wrapper_sample` sample that demonstrates the functionality of :ref:`ei_wrapper`.
* Added :ref:`ei_data_forwarder_sample` sample that demonstrates the usage of `Edge Impulse's data forwarder`_ to provide sensor data to `Edge Impulse studio`_ when :ref:`ug_edge_impulse` respectively.

Trusted Firmware-M
------------------

* Added a simple sample :ref:`tfm_hello_world` that demonstrates how to integrate TF-M in an application.
* Enabled the use of platform code that resides outside of the Trusted Firmware-M repository.
  This allows configurable memory partitioning in the |NCS|.
* Added support for running the :ref:`download_sample` sample with TF-M.

Partition Manager
-----------------

* Changed the naming convention for partition names in ``pm.yml`` and ``pm_static.yml``.
* Updated Partition Manager to prevent users from using partition names in ``pm.yml`` and ``pm_static.yml`` that match the names of the child images that define them in ``CMakeLists.txt``:

  * If the invalid naming scheme is used in ``pm.yml`` files, Partition Manager will now fail the builds.
  * If the invalid naming scheme is used in ``pm_static.yml`` files, the build will instead print a warning prompting the user to change this, if possible.
* Renamed ``b0`` and ``b0n`` container partitions to ``b0_container`` and ``b0n_container``, respectively.
* Renamed ``b0_image`` and ``b0n_image`` image partitions to appropriately match their child image name, ``b0`` and ``b0n``, respectively.

  **Migration notes:** While in development, you should rename partitions appropriately.
  You can still build firmware updates under the invalid scheme, but they will still be built with the improper sizes for the related partitions.

sdk-nrfxlib
-----------

See the changelog for each library in the :doc:`nrfxlib documentation <nrfxlib:README>` for additional information.

Modem library
+++++++++++++

* BSD library has been renamed to ``nrf_modem`` (Modem library) and ``nrf_modem_lib`` (glue).
* Updated to version 1.0.1. See the :ref:`nrfxlib:nrf_modem_changelog` for detailed information.

Crypto
++++++

* Added:

  * nrf_cc3xx_platform v0.9.7, with the following highlights:

    * Fixed an issue with mutex slab allocation in Zephyr RTOS platform file.
    * The library is built against mbed TLS v2.24.0.

    See the :ref:`crypto_changelog_nrf_cc3xx_platform` for detailed information.

  * Added nrf_cc3xx_mbedcrypto v0.9.7, with the following highlights:

    * Fixed issues where ``mbedtls_rsa_complete`` was not able to deduce missing parameters.
    * Fixed an issue with calculating the correct salt length for certain combinations of RSA key and digest sizes.
    * Added missing function: ``mbedtls_ecp_write_key``.
    * The library is built against mbed TLS v2.24.0.

    See the :ref:`crypto_changelog_nrf_cc3xx_mbedcrypto` for detailed information.

  * Added nrf_oberon v3.0.8, with the following highlights:

    * Added APIs for doing ECDH calculation using secp256r1 in incremental steps.
    * Added ``ocrypto_`` APIs for SHA-224 and SHA-384.
    * Added ``ocrypto_`` APIs for pbkdf2 for SHA-1 and SHA-256.
    * The library is built against mbed TLS v2.24.0.

    See the :ref:`nrfxlib:crypto_changelog_oberon` for detailed information.

* Updated:

  * :ref:`nrf_security`:

    * Added Kconfig options for TLS/DTLS and x509 certificates.
    * Added Kconfig options for ``PK`` and ``PK_WRITE`` (:kconfig:option:`CONFIG_MBEDTLS_PK_C` and :kconfig:option:`CONFIG_MBEDTLS_PK_WRITE_C`).
    * Rewrote the stripping mechanism of the library to not use the ``POST_BUILD`` option in a custom build rule.
      The library stripping mechanism was non-functional in certain versions of SEGGER Embedded Studio Nordic Edition.

SoftDevice Controller
+++++++++++++++++++++

See the :ref:`nrfxlib:softdevice_controller_changelog` for detailed information.

* Renamed and reconfigured the libraries. Following are the new names of the libraries:

  * :file:`libsoftdevice_controller_peripheral.a`
  * :file:`libsoftdevice_controller_central.a`
  * :file:`libsoftdevice_controller_multirole.a`

* All libraries are now compatible with all the platforms within a given device family.
  The smallest sized library fitting the use case of the application will automatically be selected.
  In most cases, the final binary size is reduced.

Multiprotocol Service Layer
+++++++++++++++++++++++++++

See the :ref:`mpsl_changelog` for detailed information.

* Added a new signal ``MPSL_TIMESLOT_SIGNAL_OVERSTAYED`` to the MPSL timeslot. This signal is given to the application when the closing of timeslot session is delayed beyond a limit.
* Added a new clock configuration option :c:member:`skip_wait_lfclk_started` in :c:struct:`mpsl_clock_lfclk_cfg_t`, which does not wait for the start of Low Frequency Clock.
* Added support for RF front-end modules (FEM) in MPSL. The front-end module feature in MPSL currently supports the SKY66112-11 device, but does not support nRF21540 revision 1 or older.


nrfx
----

See the `Changelog for nrfx 2.4.0`_ for detailed information.

MCUboot
=======

sdk-mcuboot
-----------

The MCUboot fork in |NCS| (``sdk-mcuboot``) contains all commits from the upstream MCUboot repository up to and including ``3fc59410b6``, plus some |NCS| specific additions.

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
  * Updated the ``CONFIG_BOOT_DIRECT_XIP_REVERT`` option to be valid only in xip-mode.
  * Added an offset parameter to the tinycrypt ctr mode so that it can be properly used as a streaming cipher.
  * Configured the bootloader to use a minimal CBPRINTF (:kconfig:option:`CONFIG_CBPRINTF_NANO`) implementation.
  * Configured logging to use :kconfig:option:`CONFIG_LOG_MINIMAL` by default.
  * Fixed a vulnerability with nokogiri<=1.11.0.rc4.
  * Introduced a bootutil_public library that contains code common to MCUboot and the DFU application.
    See :kconfig:option:`CONFIG_MCUBOOT_BOOTUTIL_LIB`.

* Image tool:

  * Updated the tool to print an image digest during verification.
  * Added a possibility to set a confirm flag for HEX files as well.
  * Updated the usage of ``--confirm`` to imply ``--pad``.
  * Fixed the argument handling of ``custom_tlvs``.
  * Added support for setting a fixed ROM address in the image header.

Mcumgr
======

The mcumgr library contains all commits from the upstream mcumgr repository up to and including snapshot ``74e77ad08``.

The following list summarizes the most important changes inherited from upstream mcumgr:

* Fixed an issue with devices running MCUboot v1.6.0 or earlier where a power outage during erase of a corrupted image in slot 1 could result in the device not being able to boot.
  In this case, it was not possible to update the device and mcumgr would return error code 6 (``MGMT_ERR_EBADSTATE``).
* Added support for invoking shell commands (shell management) from the mcumgr command line.
* Added optional verification of an uploaded direct-xip binary, which will reject any binary that cannot boot from the base address of the offered upload slot.
  This verification can be enabled through :kconfig:option:`CONFIG_IMG_MGMT_REJECT_DIRECT_XIP_MISMATCHED_SLOT`.

Zephyr
======

sdk-zephyr
----------

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
  * Fixed inline assembly code in Cortex-M system calls.

* Boards:

  * Added support for :ref:`board versioning <zephyr:application_board_version>`.
    With this concept, multiple board revisions can now share a single folder and board name.
  * Fixed arguments for the J-Link runners for nRF5340 DK and added the DAP Link (CMSIS-DAP) interface to the OpenOCD runner for nRF5340.
  * Marked the nRF5340 PDK as deprecated and updated the nRF5340 documentation to point to the :ref:`zephyr:nrf5340dk_nrf5340`.
  * Added enabling of LFXO pins (XL1 and XL2) for nRF5340.
  * Removed non-existing documentation links from partition definitions in the board devicetree files.
  * Updated documentation related to QSPI use.

* Kernel:

  * Restricted thread-local storage, which is now available only when the toolchain supports it.
    Toolchain support is initially limited to the toolchains bundled with the Zephyr SDK.
  * Added support for :ref:`condition variables <zephyr:condvar>`.
  * Added support for aligned allocators.
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
  * Added a :kconfig:option:`CONFIG_MEM_SLAB_TRACE_MAX_UTILIZATION` option.
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
  * Added the ``UICR`` and ``FICR`` peripherals to the devicetree.
  * Changed the interrupt priorities in devicetree for Nordic Semiconductor devices to default to ``NRF_DEFAULT_IRQ_PRIORITY`` instead of hard-coded values.

* Drivers:

  * Deprecated the ``DEVICE_INIT()`` macro.
    Use :c:macro:`DEVICE_DEFINE` instead.
  * Introduced macros (:c:macro:`DEVICE_DT_DEFINE` and related ones) that allow defining devices using information from devicetree nodes directly and referencing structures of such devices at build time.
    Most drivers have been updated to use these new macros for creating their instances.
  * Deprecated the ``DEVICE_AND_API_INIT()`` macro.
    Use :c:macro:`DEVICE_DEFINE` or :c:macro:`DEVICE_DT_INST_DEFINE` instead.

  * ADC:

    * Improved the default routine that provides sampling intervals, to allow intervals shorter than 1 millisecond.
    * Reworked, extended, and improved the ``adc_shell`` driver to make testing an ADC peripheral simpler.
    * Introduced the ``adc_sequence_options::user_data`` field.

  * Bluetooth Controller:

    * Fixed and improved an issue where a connection event closed too early when more data could have been sent in the same connection event.
    * Fixed missing slave latency cancellation when initiating control procedures.
      Connection terminations are faster now.
    * Added experimental support for non-connectable scannable Extended Advertising with 255 byte PDU (without chaining and privacy support).
    * Added experimental support for connectable non-scannable Extended Advertising with 255 byte PDU (without chaining and privacy support).
    * Added experimental support for non-connectable non-scannable Extended Advertising with 255 byte PDU (without chaining and privacy support).
    * Added experimental support for Extended Scanning with duration and period parameters (without active scanning for scan response or chained PDU).
    * Added experimental support for Periodic Advertising and Periodic Advertising Synchronization Establishment.

  * Bluetooth HCI:

    * Added support for ISO packets to the RPMsg driver.
    * Added the possibility of discarding advertising reports to avoid time-outs in the User Channel, STM32 IPM, and SPI based drivers.

  * Bluetooth Host:

    * Added an API to unregister scanner callbacks.
    * Fixed an issue where ATT activity after the ATT time-out expired led to invalid memory access.
    * Added support for LE Secure connections pairing in parallel on multiple connections.
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

  * Clock control:

    * Changed the definition (parameters and return values) of the API function :c:func:`clock_control_async_on`.
    * Added support for the audio clock in nRF53 Series SoCs.
    * Added missing handling of the HFCLK192M_STARTED event in nRF53 Series SoCs.

  * Counter:

    * Excluded selection of nRF TIMER0 and RTC0 when the Bluetooth Controller is enabled.

  * Display:

    * Added support for the ILI9488 display.
    * Refactored the ILI9340 driver to support multiple instances, rotation, and pixel format changing at runtime.
      Configuration of the driver instances is now done in devicetree.
    * Enhanced the SSD1306 driver to support communication via both SPI and I2C.

  * Ethernet:

    * Added driver for the W5500 Ethernet controller.

  * Flash:

    * Modified the nRF QSPI NOR driver so that it also supports nRF53 Series SoCs.
    * Added missing selection of :kconfig:option:`CONFIG_FLASH_HAS_PAGE_LAYOUT` for the SPI NOR and AT45 family flash drivers.
    * Refactored the nRF QSPI NOR driver so that it no longer depends on :kconfig:option:`CONFIG_MULTITHREADING`.
    * Removed ``CONFIG_NORDIC_QSPI_NOR_QE_BIT``.
      Use the ``quad-enable-requirements`` devicetree property instead.
    * Added JESD216 support to the nRF QSPI NOR driver.

  * GPIO:

    * Added support for controlling LED intensity to the SX1509B driver.
    * Added an emulated GPIO driver.

  * IEEE 802.15.4:

    * Updated the nRF5 IEEE 802.15.4 driver to version 1.9.
    * Production support for IEEE 802.15.4 in the single-protocol configuration on nRF5340.
    * Development support for IEEE 802.15.4 in the multi-protocol configuration on nRF5340.
    * Added reservation of the TIMER peripheral used by the nRF5 IEEE 802.15.4 driver.
    * Added support for sending packets with specified TX time using the nRF5 IEEE 802.15.4 driver.
    * Implemented the RX failed notification for the nRF5 IEEE 802.15.4 driver.

  * LED PWM:

    * Added a driver interface and implementation for PWM-driven LEDs.

  * Modem:

    * Reworked the command handler reading routine, to prevent data loss and reduce RAM usage.
    * Added the possibility of locking TX in the command handler.
    * Improved handling of HW flow control on the RX side of the UART interface.
    * Added the possibility of defining commands with a variable number of arguments.
    * Introduced :c:func:`gsm_ppp_start` and :c:func:`gsm_ppp_stop` functions to allow restarting the networking stack without rebooting the device.
    * Added support for Quectel BG9x modems.

  * Power:

    * Added multiple ``nrfx_power``-related fixes to reduce power consumption.

  * PWM:

    * Changed the GPIO configuration to use Nordic HAL, which allows support for GPIO pins above 31.
    * Added a check to ensure that the PWM period does not exceed a 16-bit value to prevent erroneous behavior.
    * Changed the PWM DT configuration to use a timer handle instead of the previously used timer instance.

  * Regulator:

    * Introduced a new regulator driver infrastructure.

  * Sensor:

    * Added support for the IIS2ICLX 2-axis digital inclinometer.
    * Enhanced the BMI160 driver to support communication via both SPI and I2C.
    * Added device power management in the LIS2MDL magnetometer driver.
    * Refactored the FXOS8700 driver to support multiple instances.
    * Added support for the Invensense ICM42605 motion sensor.
    * Added support for power management in the BME280 sensor driver.

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

    * Refactored the :c:func:`uart_poll_out` implementation in the nRF UARTE driver to fix incorrect handling of HW flow control and power management.

  * SPI:

    * Added support for SPI emulators.

  * Timer:

    * Extended the nRF RTC Timer driver with a vendor-specific API that allows using the remaining compare channels of the RTC that provides the system clock.

  * USB:

    * Fixed handling of zero-length packets (ZLP) in the Nordic Semiconductor USB Device Controller driver (usb_dc_nrfx).
    * Fixed initialization of the workqueue in the usb_dc_nrfx driver, to prevent fatal errors when the driver is reattached.
    * Fixed handling of the SUSPEND and RESUME events in the Bluetooth classes.
    * Made the USB DFU class compatible with the target configuration that does not have a secondary image slot.
    * Added support for using USB DFU within MCUboot with single application slot mode.
    * Removed heap allocations from the usb_dc_nrfx driver to fix problems with improper memory sizes.
      Now the driver uses static buffers for OUT endpoints and memory slabs for FIFO elements.

* Networking:

  * General:

    * Added support for DNS Service Discovery.
    * Deprecated legacy TCP stack (TCP1).
    * Added multiple minor TCP2 bugfixes and improvements.
    * Added support for RX packet queueing in TCP2.
    * Added network management events for DHCPv4.
    * Added periodic throughput printout to the :zephyr:code-sample:`sockets-echo-server` sample.
    * Added an experimental option to set preemptive priority for networking threads (:kconfig:option:`CONFIG_NET_TC_THREAD_PREEMPTIVE`).
    * Added a Kconfig option that enables a hostname update on link address change (:kconfig:option:`CONFIG_NET_HOSTNAME_UNIQUE_UPDATE`).
    * Added multiple fixes to the DHCP implementation.
    * Added support for the Distributed Switch Architecture (DSA).

  * LwM2M:

    * Made the endpoint name length configurable with Kconfig (see :kconfig:option:`CONFIG_LWM2M_RD_CLIENT_ENDPOINT_NAME_MAX_LENGTH`).
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
    * Added support for Bootstrap Discovery.

  * OpenThread:

    * Updated the OpenThread version to commit ``f7825b96476989ae506a79963613f971095c8ae0``.
    * Removed obsolete flash driver from the OpenThread platform.
    * Added new OpenThread options:

      * ``CONFIG_OPENTHREAD_NCP_BUFFER_SIZE``
      * :kconfig:option:`CONFIG_OPENTHREAD_NUM_MESSAGE_BUFFERS`
      * :kconfig:option:`CONFIG_OPENTHREAD_MAX_STATECHANGE_HANDLERS`
      * :kconfig:option:`CONFIG_OPENTHREAD_TMF_ADDRESS_CACHE_ENTRIES`
      * :kconfig:option:`CONFIG_OPENTHREAD_MAX_CHILDREN`
      * :kconfig:option:`CONFIG_OPENTHREAD_MAX_IP_ADDR_PER_CHILD`
      * :kconfig:option:`CONFIG_OPENTHREAD_LOG_PREPEND_LEVEL_ENABLE`
      * :kconfig:option:`CONFIG_OPENTHREAD_MAC_SOFTWARE_ACK_TIMEOUT_ENABLE`
      * :kconfig:option:`CONFIG_OPENTHREAD_MAC_SOFTWARE_RETRANSMIT_ENABLE`
      * ``CONFIG_OPENTHREAD_PLATFORM_USEC_TIMER_ENABLE``
      * :kconfig:option:`CONFIG_OPENTHREAD_RADIO_LINK_IEEE_802_15_4_ENABLE`
      * :kconfig:option:`CONFIG_OPENTHREAD_RADIO_LINK_TREL_ENABLE`
      * :kconfig:option:`CONFIG_OPENTHREAD_CSL_SAMPLE_WINDOW`
      * :kconfig:option:`CONFIG_OPENTHREAD_CSL_RECEIVE_TIME_AHEAD`
      * :kconfig:option:`CONFIG_OPENTHREAD_MAC_SOFTWARE_CSMA_BACKOFF_ENABLE`
      * :kconfig:option:`CONFIG_OPENTHREAD_PLATFORM_INFO`
      * :kconfig:option:`CONFIG_OPENTHREAD_RADIO_WORKQUEUE_STACK_SIZE`

    * Added support for RCP co-processor mode.
    * Fixed multicast packet reception.

  * MQTT:

    * Fixed mutex protection on :c:func:`mqtt_disconnect`.
    * Switched the library to use ``zsock_*`` socket functions instead of POSIX names.
    * Changed the return value of :c:func:`mqtt_keepalive_time_left` to -1 when keep alive is disabled.

  * Sockets:

    * Enabled Maximum Fragment Length (MFL) extension on TLS sockets.
    * Added a :c:macro:`TLS_ALPN_LIST` socket option for TLS sockets.
    * Fixed a ``tls_context`` leak on ``ztls_socket()`` failure.
    * Fixed ``getaddrinfo()`` hints handling with AI_PASSIVE flag.

  * CoAP:

    * Added a retransmission counter to the :c:struct:`coap_pending` structure to simplify the retransmission logic.
    * Added a Kconfig option to randomize the initial ACK time-out, as specified in RFC 7252 (:kconfig:option:`CONFIG_COAP_RANDOMIZE_ACK_TIMEOUT`).
    * Fixed encoding of long options (larger than 268 bytes).

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
    * Updated the FS API to not perform a runtime check of a driver interface when the :kconfig:option:`CONFIG_NO_RUNTIME_CHECKS` option is enabled.

  * DFU:

    * Added shell module for MCUboot enabled application.
      See :kconfig:option:`CONFIG_MCUBOOT_SHELL`.
    * Reworked the implementation to use MCUboot's bootutil_public library instead of the Zephyr implementation of the same API.

  * IPC:

    * Added a ``subsys/ipc`` subsystem that provides multi-endpoint capabilities to the OpenAMP integration.

* Build system:

  * Updated west to v0.9.0.
  * Renamed sanitycheck to Twister.
  * Ensured that shields can be placed in other BOARD_ROOT folders.
  * Added basic support for Clang 10 with x86.
  * Fixed a bug that prevented compiling the :ref:`bootloader` with :kconfig:option:`CONFIG_SB_SIGNING_PUBLIC_KEY`

* System:

  * Added an API that provides a printf family of functions (for example, :c:func:`cbprintf`) with a callback on character output, to perform in-place streaming of the formatted string.
  * Updated minimal libc to print stderr just like stdout.
  * Added an ``abort()`` function to minimal libc.
  * Updated the ring buffer to allow using the full buffer capacity instead of forcing an empty slot.
  * Added a :c:macro:`CLAMP` macro.
  * Added a feature for post-mortem analysis to the tracing library.

* Samples:

  * Added :zephyr:code-sample:`nrf_ieee802154_rpmsg`.
  * Added :zephyr:code-sample:`tagoio-http-post`.
  * Added Civetweb WebSocket Server sample.
  * :zephyr:code-sample:`led-strip`: Updated to force SPIM on nRF52 DK.
  * :zephyr:code-sample:`cfb-custom-fonts`: Added support for ssd1306fb.
  * Generic GSM modem: Added suspend/resume shell commands.
  * :zephyr:code-sample:`updatehub-fota`: Added support for Bluetooth LE IPSP, 802.15.4, modem, and Wi-Fi.

* Logging:

  * Added STP transport and raw data output support for systrace.

* Modules:

  * Introduced a :kconfig:option:`CONFIG_MBEDTLS_MEMORY_DEBUG` option for mbedtls.
  * Updated LVGL to v7.6.1.
  * Updated libmetal and openamp to v2020.10.
  * Updated nrfx in hal-nordic to version 2.4.0.
  * Updated the Trusted Firmware-M (TF-M) module to v1.2.0.
  * Moved the nrfx glue code from the hal_nordic module repository to the main Zephyr repository.
  * Updated the Trusted Firmware-M (TF-M) module to include support for the nRF5340 and nRF9160 platforms.

* Other:

  * Renamed the ``sanitycheck`` script to ``twister``.
  * Added initial LoRaWAN support.
  * Updated ``west flash`` support for ``nrfjprog`` to fail if a HEX file has UICR data and ``--erase`` was not specified.
  * Added an API to correlate system time with external time sources (see :ref:`zephyr:timeutil_api`).

* Power management:

  * Overhauled the naming and did some general cleanup.
  * Added a notifier API to register an object to receive notifications when the system changes power state.

* Shell:

  * Updated documentation.
  * Optimized the tab feature and the select command.
  * Enhanced and improved the help command.

* Toolchain:

  * Added initial support for LLVM/Clang (version 10, on the x86 architecture).
  * Added the environment variable ``LLVM_TOOLCHAIN_PATH`` for locating the LLVM toolchain.

* USB:

  * Fixed the handling of zero-length packet (ZLP) in the nRF USB Device Controller Driver.
  * Changed the USB DFU wait delay to be configurable with Kconfig (``CONFIG_USB_DFU_WAIT_DELAY_MS``).

Additions specific to |NCS|
+++++++++++++++++++++++++++

The following list contains |NCS| specific additions:

* Added support for the |NCS|'s :ref:`partition_manager`, which can be used for flash partitioning.
* Added the following network socket and address extensions to the :ref:`zephyr:bsd_sockets_interface` interface to support the functionality provided by the BSD library:

  * AF_LTE family.
  * NPROTO_AT protocol.
  * NPROTO_PDN protocol to be used in conjunction with AF_LTE.
  * NPROTO_DFU protocol to be used in conjunction with AF_LOCAL.
  * SOCK_MGMT socket type, used in conjunction with AF_LTE.
  * SOL_PDN protocol level and associated socket option values (SO_PDN_CONTEXT_ID option for PDN sockets, SO_PDN_STATE option for PDN sockets to retrieve the state of the PDN connection).
  * SOL_DFU protocol level and associated socket options. This includes a SO_DFU_ERROR socket option for DFU socket that can be used when an operation on a DFU socket returns -ENOEXEC, indicating that the modem has rejected the operation to retrieve the reason for the error.
  * TLS_SESSION_CACHE socket option for TLS session caching.
  * SO_BINDTODEVICE socket option.
  * SO_SNDTIMEO socket option.
  * SO_SILENCE_ALL to disable/enable all the replies to unexpected traffics.
  * SO_IP_ECHO_REPLY to disable/enable replies to IPv4 ICMPs.
  * SO_IPV6_ECHO_REPLY to disable/enable replies to IPv6 ICMPs.
  * MSG_TRUNC socket flag.
  * MSG_WAITALL socket flag required to support the corresponding NRF counterpart flag, for translation within the offloading interface.

* Added support for enabling TLS caching when using the :ref:`zephyr:mqtt_socket_interface` library.
  See :c:macro:`TLS_SESSION_CACHE`.
* Modified the SoC devicetree :file:`.dtsi` files to prefer the CryptoCell CC310 hardware as the system entropy source on SoCs where support is available (nRF52840, nRF5340, nRF9160).
* Added Zigbee L2 layer.
* Added readout of IEEE 802.15.4 EUI-64 address in the non-secure build from the FICR registers in the secure zone (nRF IEEE 802.15.4 Radio Driver).
* Added TF-M adjustments to support TF-M in |NCS|.
* Disabled the automatic printing of OpenThread settings when building OpenThread.

Documentation
=============

In addition to documentation related to the changes listed above, the following documentation has been updated:

* :ref:`ncs_introduction` - Added information about the repositories, tools and configuration, and west.
* :ref:`gs_installing` - Updated with information about installing GN tools.
* :ref:`gs_programming` -  Added more information about west flash.
* Restructured the User guides section and moved the content to :ref:`dev-model`, :ref:`ug_app_dev`, :ref:`protocols`, and the root level.
* Added the following user guides:

  * :ref:`app_memory`
  * :ref:`app_power_opt`
  * :ref:`ug_tfm`
  * :ref:`ug_radio_fem`
  * :ref:`ug_edge_impulse`
  * :ref:`ug_chip`
* :ref:`ug_nrf9160` - Added information about TF-M, board revisions, and full modem firmware update.
* :ref:`ug_nrf5340` - Added and updated information about:

  * TF-M, multiprotocol support, and available samples for Thread and Zigbee.
  * Building and programming using SEGGER Embedded Studio, multi-image build using west, and disabling readback protection.

* :ref:`ug_nrf52` - Added sections on Project CHIP, Thread, and Zigbee support.
* :ref:`ug_bt_mesh` - Added :ref:`ug_bt_mesh_model_config_app`, :ref:`bt_mesh_ug_reserved_ids`, and :ref:`ug_bt_mesh_vendor_model` (plus subpages).
* :ref:`ug_thread`:

  * Added information about nRF5340 to :ref:`thread_ot_memory` and :ref:`ug_thread_architectures`.
  * :ref:`ug_thread_configuring` - Updated information about IEEE 802.15.4 EUI-64 configuration.
  * :ref:`ug_thread_tools` - Added information on installing `wpantund`_.
* :ref:`ug_zigbee`:

  * Updated :ref:`zigbee_ug_supported_features`, :ref:`ug_zigbee_platform_design_ncp`, and :ref:`ug_zigbee_tools`.
  * :ref:`ug_zigbee_configuring` - Updated mandatory and optional configuration options, logger options, section on power saving during sleep and added IEEE 802.15.4 EUI-64 configuration.
* Documentation updates for Homekit.


Applications and samples
------------------------

* nRF9160:

  * :ref:`serial_lte_modem` - Added documentation for new commands.
    Fixed the syntax and examples of some existing commands.
  * Added a note about :kconfig:option:`CONFIG_MQTT_KEEPALIVE` option to the :ref:`aws_iot`, :ref:`azure_iot_hub`, and cloud client samples.
* Bluetooth:

  * Added a note about child-image overlay to the :ref:`bluetooth_central_hr_coded` and :ref:`peripheral_hr_coded` samples.
  * :ref:`shell_bt_nus` - Updated the testing section.
  * :ref:`ble_throughput` - Updated to reflect the new implementation and usage.
* Bluetooth mesh:

  * Moved the contents in Configuring models to :ref:`ug_bt_mesh_model_config_app`.
  * Renamed the following samples:

    * Bluetooth: Mesh light control sample to :ref:`bluetooth_mesh_light_lc`.
    * Bluetooth: Mesh sensor client to :ref:`bluetooth_mesh_sensor_client`.
    * Bluetooth: Mesh sensor server to :ref:`bluetooth_mesh_sensor_server`.
* Thread:

  * Added information on FEM support.
  * :ref:`ot_cli_sample` - Added information about minimal configuration and updated the information on activating sample extensions, testing, and dependencies.
* Zigbee:

  * Added information on FEM support and updated the dependencies sections.
  * :ref:`zigbee_light_switch_sample` - Added a section on :ref:`zigbee_light_switch_activating_variants`.
* Updated the configuration sections of the following samples:

  * :ref:`download_sample`
  * nRF9160: Simple MQTT
  * Bluetooth: Peripheral Alexa Gadgets
* :ref:`bootloader` - Added information on bootloader overlays and building the sample from SEGGER Embedded Studio and command line.
* Added information about FEM support to the :ref:`radio_test` and :ref:`direct_test_mode` samples.

Libraries and drivers
---------------------

* :ref:`liblwm2m_carrier_readme` - Removed the version dependency table from :ref:`lwm2m_certification`.
* :ref:`lib_dfu_target` - Added information about full modem upgrade and updated the configuration.
* :ref:`lib_aws_iot` - Added information on initializing the library and connecting to AWS IoT broker.
* :ref:`app_event_manager` - Updated the documentation to describe events with dynamic data size.
* :ref:`lib_entropy_cc310` - Updated information about driver behavior in secure and non-secure applications.

nrfxlib
-------

* :ref:`nrf_cc310_mbedcrypto_readme` - Added API.
* :ref:`nrf_modem`:

  * :ref:`architecture` - Added information on shared memory configuration.
  * ``tls_dtls_configuration`` - Added information on supported cipher suites.
  * :ref:`nrf_modem_ug_porting` - Added information about the modem functions.
* :ref:`mpsl` - Added :ref:`mpsl_fem`.
* :ref:`nrf_802154_sl` - Added.
* :ref:`nrf_security` - Updated to reflect the features supported by different backends.
* :ref:`softdevice_controller` - Updated the Bluetooth LE feature support.
* :ref:`zboss` - Added the types of ZBOSS libraries that are available.


Known issues
************

See `known issues for nRF Connect SDK v1.5.0`_ for the list of issues valid for this release.
