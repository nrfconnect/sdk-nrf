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

nRF5340 SoC
-----------

* Updated:

  * ``bl_boot`` library - Disabled clock interrupts before booting the application.
    This change fixes an issue where the :ref:`bootloader` sample would not be able to boot a Zephyr application on the nRF5340 SoC.

Thread
------

* Added:

  * Development support for the nRF5340 DK in single-protocol configuration for the :ref:`ot_cli_sample`, :ref:`coap_client_sample`, and :ref:`coap_server_sample` samples.

* Optimized ROM and RAM used by Thread samples.
* Disabled Hardware Flow Control on the serial port in :ref:`coap_client_sample` and :ref:`coap_server_sample` samples.

Zigbee
------

* Added:

  * Development support for the nRF5340 DK in single-protocol configuration for the :ref:`zigbee_light_switch_sample`, :ref:`zigbee_light_bulb_sample`, and :ref:`zigbee_network_coordinator_sample` samples.

* Updated:

  * Updated :ref:`zboss` to version ``3_3_0_6+11_30_2020``.
    See :ref:`nrfxlib:zboss_changelog` for detailed information.

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

MCUboot
=======

sdk-mcuboot
-----------

The MCUboot fork in |NCS| contains all commits from the upstream MCUboot repository up to and including ``5a6e18148d``, plus some |NCS| specific additions.
The list of the most important recent changes can be found in :ref:`ncs_release_notes_140`.


Zephyr
======

sdk-zephyr
----------

.. NOTE TO MAINTAINERS: The latest Zephyr commit appears in multiple places; make sure you update them all.

The Zephyr fork in |NCS| contains all commits from the upstream Zephyr repository up to and including ``35264cc214fd``, plus some |NCS| specific additions.

For a complete list of upstream Zephyr commits incorporated into |NCS| since the most recent release, run the following command from the :file:`ncs/zephyr` repository (after running ``west update``):

.. code-block:: none

   git log --oneline 35264cc214fd ^v2.4.0-ncs1

For a complete list of |NCS| specific commits, run:

.. code-block:: none

   git log --oneline manifest-rev ^35264cc214fd

The current |NCS| release is based on Zephyr v2.4.99.

The following list summarizes the most important changes inherited from upstream Zephyr:

* Boards:

  * Fixed arguments for the J-Link runners for nRF5340 DK and added the DAP Link (CMSIS-DAP) interface to the OpenOCD runner for nRF5340.
  * Marked the nRF5340 PDK as deprecated and updated the nRF5340 documentation to point to the :ref:`zephyr:nrf5340dk_nrf5340`.
  * Added enabling of LFXO pins (XL1 and XL2) for nRF5340.
  * Removed non-existing documentation links from partition definitions in the board devicetree files.
  * Updated documentation related to QSPI use.

* Kernel:

  * Restricted thread-local storage, which is now available only when the toolchain supports it.
    Toolchain support is initially limited to the toolchains bundled with the Zephyr SDK.
  * Fixed a race condition between :c:func:`k_queue_append` and :c:func:`k_queue_alloc_append`.
  * Updated the kernel to no longer try to resume threads that are not suspended.
  * Updated the kernel to no longer attempt to queue threads that are already in the run queue.
  * Updated :c:func:`k_busy_wait` to return immediately on a zero time-out, and improved accuracy on nonzero time-outs.
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

  * Updated :c:func:`k_timer_user_data_get` to take a ``const struct k_timer *timer`` instead of a non-\ ``const`` pointer.

* Drivers:

  * Deprecated the ``DEVICE_INIT()`` macro.
    Use :c:macro:`DEVICE_DEFINE` instead.

  * ADC:

    * Improved the default routine that provides sampling intervals, to allow intervals shorter than 1 millisecond.

  * Display:

    * Added support for the ILI9488 display.
    * Refactored the ILI9340 driver to support multiple instances, rotation, and pixel format changing at runtime.
      Configuration of the driver instances is now done in devicetree.
    * Enhanced the SSD1306 driver to support communication via both SPI and I2C.

  * Flash:

    * Modified the nRF QSPI NOR driver so that it supports also nRF53 Series SoCs.

  * IEEE 802.15.4:

    * Updated the nRF5 IEEE 802.15.4 driver to version 1.9.

  * Modem:

    * Reworked the command handler reading routine, to prevent data loss and reduce RAM usage.
    * Added the possibility of locking TX in the command handler.
    * Improved handling of HW flow control on the RX side of the UART interface.

  * Sensor:

    * Added support for the IIS2ICLX 2-axis digital inclinometer.
    * Enhanced the BMI160 driver to support communication via both SPI and I2C.
    * Added device power management in the LIS2MDL magnetometer driver.

  * Serial:

    * Fixed an issue in the nRF UARTE driver where spurious data could be received when the asynchronous API with hardware byte counting was used and the UART was switched back from the low power to the active state.
    * Removed the following deprecated definitions:

      * ``UART_ERROR_BREAK``
      * ``LINE_CTRL_BAUD_RATE``
      * ``LINE_CTRL_RTS``
      * ``LINE_CTRL_DTR``
      * ``LINE_CTRL_DCD``
      * ``LINE_CTRL_DSR``

  * USB:

    * Fixed handling of zero-length packets (ZLP) in the Nordic Semiconductor USB Device Controller driver (usb_dc_nrfx).
    * Fixed initialization of the workqueue in the usb_dc_nrfx driver, to prevent fatal errors when the driver is reattached.
    * Fixed handling of the SUSPEND and RESUME events in the Bluetooth classes.

* Networking:

  * General:

    * Added support for DNS Service Discovery.
    * Deprecated legacy TCP stack (TCP1).
    * Added multiple minor TCP2 bugfixes and improvements.
    * Added network management events for DHCPv4.

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

  * OpenThread:

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
      * :option:`CONFIG_OPENTHREAD_CONFIG_PLATFORM_INFO`

  * MQTT:

    * Fixed mutex protection on :c:func:`mqtt_disconnect`.
    * Switched the library to use ``zsock_*`` socket functions instead of POSIX names.

  * Sockets:

    * Enabled Maximum Fragment Length (MFL) extension on TLS sockets.
    * Added a :c:macro:`TLS_ALPN_LIST` socket option for TLS sockets.
    * Fixed a ``tls_context`` leak on ``ztls_socket()`` failure.
