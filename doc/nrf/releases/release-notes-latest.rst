.. _ncs_release_notes_latest:

Changes in |NCS| v1.5.99
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

Changelog
*********

The following sections provide detailed lists of changes by component.

nRF9160
=======

* Added:

  * :ref:`lib_modem_jwt` library, which provides an API to obtain a JSON Web Token (JWT) from the modem.  Functionality requires modem firmware v1.3.0 or higher.

  * :ref:`lib_modem_attest_token` library:

    * The library provides an API to get an attestation token from the modem.
    * Functionality requires modem firmware v1.3.0 or higher.

* Updated:

  * :ref:`lib_nrf_cloud` library:

    * Added cell-based location support to :ref:`lib_nrf_cloud_agps`.
    * Added Kconfig option :option:`CONFIG_NRF_CLOUD_AGPS_SINGLE_CELL_ONLY` to obtain cell-based location from nRF Connect for Cloud instead of using the modem's GPS.
    * Added function :c:func:`nrf_cloud_modem_fota_completed` which is to be called by the application after it re-initializes the modem (instead of rebooting) after a modem FOTA update.
    * Updated to include the FOTA type value in the :c:enumerator:`NRF_CLOUD_EVT_FOTA_DONE` event.
    * Updated configuration options for setting the source of the MQTT client ID (nRF Cloud device ID).

  * :ref:`asset_tracker` application:

    * Updated to handle new Kconfig options:

      * :option:`CONFIG_NRF_CLOUD_AGPS_SINGLE_CELL_ONLY`
      * :option:`CONFIG_NRF_CLOUD_AGPS_REQ_CELL_BASED_LOC`

  * A-GPS library:

    * Added the Kconfig option :option:`CONFIG_AGPS_SINGLE_CELL_ONLY` to support cell-based location instead of using the modem's GPS.

  * :ref:`modem_info_readme` library:

    * Updated to prevent reinitialization of param list in :c:func:`modem_info_init`.

  * :ref:`lib_fota_download` library:

    * Added an API to retrieve the image type that is being downloaded.
    * Added an API to cancel current downloading.

  * :ref:`lib_ftp_client` library:

    * Support subset of RFC959 FTP commands only.
    * Added support of STOU and APPE (besides STOR) for "put".
    * Added detection of socket errors, report with proprietary reply message.
    * Increased FTP payload size from NET_IPV4_MTU(576) to MSS as defined on modem side (708).
    * Added polling "226 Transfer complete" after data channel TX/RX, with a configurable timeout of 60 seconds.
    * Ignored the reply code of "UTF8 ON" command as some FTP server returns abnormal reply.

  * :ref:`at_params_readme` library:

    * Added function :c:func:`at_params_int64_get` that allows for getting of AT param list entries containing signed 64 bit integers.

  * :ref:`lte_lc_readme` library:

    * Added support for %XT3412 AT command notifications, which allows the application to get prewarnings before Tracking Area Updates.
    * Added support for neighbor cell measurements.
    * Added support for %XMODEMSLEEP AT command notifications which allows the application to get notifications related to modem sleep.
    * Added support for %CONEVAL AT command that can be used to evaluate the LTE radio signal state in a cell prior to data transmission.

  * :ref:`serial_lte_modem` application:

    * Fixed TCP/UDP port range issue (0~65535).
    * Added AT#XSLEEP=2 to power off UART interface.
    * Added support for the ``verbose``, ``uput``, ``mput`` commands and data mode to the FTP service.
    * Added URC (unsolicited response code) to the FOTA service.
    * Enabled all SLM services by default.
    * Updated the HTTP client service code to handle chunked HTTP responses.
    * Added data mode to the MQTT Publish service to support JSON-type payload.

  * :ref:`at_cmd_parser_readme`:

    * Added support for parsing parameters of type unsigned int or unsigned short.

  * :ref:`lib_spm` library:

    * Added support for the nRF9160 pulse-density modulation (PDM) and inter-IC sound (I2S) peripherals in non-secure applications.

  * :ref:`at_host_sample` sample:

    * Renamed nRF9160: AT Client sample to :ref:`at_host_sample`.

nRF5
====

Matter (Project CHIP)
---------------------

* Project CHIP has been officially renamed to `Matter`_.
* Updated:

  * Renamed occurrences of Project CHIP to Matter.

Common
======

There are no entries for this section yet.

MCUboot
=======

The MCUboot fork in |NCS| (``sdk-mcuboot``) contains all commits from the upstream MCUboot repository up to and including ``5f004461f9``, plus some |NCS| specific additions.

The code for integrating MCUboot into |NCS| is located in :file:`ncs/nrf/modules/mcuboot`.

The following list summarizes the most important changes inherited from upstream MCUboot:

* Added support for indicating serial recovery through LED.
* Made the debounce delay of the serial detect pin state configurable.
* Added support for mbed TLS ECDSA for signatures.
* Added an option to use GPIO PIN to enter to USB DFU class recovery.
* Added an optional check that prevents attempting to boot an image built for a different ROM address than the slot it currently resides in.
  The check is enabled if the image was signed with the ``IMAGE_F_ROM_FIXED`` flag.

Mcumgr
======

The mcumgr library contains all commits from the upstream mcumgr repository up to and including snapshot ``74e77ad08``.

The following list summarizes the most important changes inherited from upstream mcumgr:

* No changes yet

Zephyr
======

.. NOTE TO MAINTAINERS: The latest Zephyr commit appears in multiple places; make sure you update them all.

The Zephyr fork in |NCS| (``sdk-zephyr``) contains all commits from the upstream Zephyr repository up to and including ``8e1cfe9a46``, plus some |NCS| specific additions.

For a complete list of upstream Zephyr commits incorporated into |NCS| since the most recent release, run the following command from the :file:`ncs/zephyr` repository (after running ``west update``):

.. code-block:: none

   git log --oneline 8e1cfe9a46 ^v2.4.99-ncs1

For a complete list of |NCS| specific commits, run:

.. code-block:: none

   git log --oneline manifest-rev ^8e1cfe9a46

The current |NCS| release is based on Zephyr v2.5.99.

The following list summarizes the most important changes inherited from upstream Zephyr:

* Arm architecture:

   * Disallowed :option:`CONFIG_FP_HARDABI` when building applications with Trusted Firmware-M.

* Boards:

   * Enabled building with TF-M for non-secure applications on the nRF9160 DK.
   * Switched to ``sda-gpios, scl-gpios`` devicetree properties on the nRF9160 DK.
   * Added a ``pwm-led0`` alias for the nRF5340 application core.
   * Enabled TF-M testing in CI on QEMU (``mps2_an521_nonsecure``).


* Drivers:

  * Introduced the :c:macro:`DEVICE_DT_NAME` macro that returns a string name for a given devicetree node.
  * Introduced the :c:func:`device_usable_check` function that determines whether a device is ready for use.
  * Corrected several optional API functions to return ``-ENOSYS`` instead of ``-ENOTSUP`` value when their implementation is not available.
  * Removed the deprecated ``device_list_get()`` function.

  * Display:

    * Added a driver and a generic shield definition for Sharp memory displays of the LS0XX type.

  * Flash:

    * Implemented workaround for nRF52 anomaly 242 in the nRF SoC flash driver.
    * Added automatic selection of :option:`CONFIG_MPU_ALLOW_FLASH_WRITE` when the MPU is enabled for Arm based SoCs.
    * Deprecated :c:func:`flash_write_protection_set()`.
      The function will be removed in Zephyr v2.8.
      Responsibility for write/erase protection management has been moved to the driver-specific implementation of the :c:func:`flash_write()` and :c:func:`flash_erase()` API calls.
    * Improved the SPI NOR flash driver to support devices that power up with block protect bits set.

  * GPIO:

    * Used the nrfx GPIOTE channel allocator in the nRF GPIO driver to properly track GPIOTE channel allocations made in other modules.
    * Added the :option:`CONFIG_GPIO_NRF_INT_EDGE_USING_SENSE` option to allow using the GPIO SENSE mechanism instead of GPIOTE channels to generate edge interrupts in the nRF GPIO driver.

  * IEEE 802.15.4:

    * Moved all the glue code for the nRF IEEE 802.15.4 radio driver from the hal_nordic module to the main Zephyr repository.
    * Fixed the initialization order in the ieee802154_nrf5 driver.
    * Corrected the pool from which RX packets are allocated in the ieee802154_nrf5 driver.
    * Added blocking on the RX packet allocation in the ieee802154_nrf5 driver to avoid dropping already acknowledged frames.
    * Added the :option:`CONFIG_IEEE802154_NRF5_UICR_EUI64_ENABLE` option to allow loading EUI64 from UICR registers.

  * LED:

    * Added handling of power states in the PWM LED driver.

  * Sensors:

    * Reworked the BME280 sensor driver to obtain device pointers directly (used :c:macro:`DEVICE_DT_GET` instead of :c:func:`device_get_binding`).
    * Made the QDEC nrfx driver usable on nRF5340 (added required devicetree and Kconfig entries).
    * Fixed an out-of-bounds write on the stack in the DPS310 sensor driver.
    * Added multi-instance support in the IIS2DLPC, IIS2MDC, and LSM6DS0 sensor drivers.
    * Added support for the BMP388 pressure sensor.
    * Added support for the single measurement mode in the LIS2MDL sensor driver.
    * Fixed the reset delay in the initialization routine of the CCS811 sensor.
    * Added an option of deferring the BQ27421 sensor initialization to the first use of the sensor, to shorten the boot time.

  * Serial:

    * Updated the nRF UARTE driver to wait for the transmitter to go idle before powering down the UARTE peripheral in asynchronous mode.
    * Fixed the power down routine in the nRF UARTE driver. Now the RX interrupt is properly disabled.
    * Clarified the meaning of the ``timeout`` parameter of the :c:func:`uart_rx_enable` API function.
    * Added a low power mode of operation for particular instances of the nRF UARTE driver (see :option:`CONFIG_UART_0_NRF_ASYNC_LOW_POWER` and related options).

  * SPI:

    * Removed the ``CONFIG_SPI_[0-8]`` and ``CONFIG_SPI_[0-8]_OP_MODES`` symbols that are no longer used by any in-tree driver.

  * USB:

    * Added Kconfig configuration of the stack size for the mass storage disk operations thread (:option:`CONFIG_MASS_STORAGE_STACK_SIZE`).
    * Added Kconfig configuration of inquiry parameters for the mass storage class (:option:`CONFIG_MASS_STORAGE_INQ_VENDOR_ID`, :option:`CONFIG_MASS_STORAGE_INQ_PRODUCT_ID`, :option:`CONFIG_MASS_STORAGE_INQ_REVISION`).
    * Fixed handling of the OUT buffer in the Bluetooth class.
    * Fixed a possible deadlock in :c:func:`usb_transfer_sync`.
    * Fixed clearing of endpoint flags during :c:func:`usb_dc_ep_disable` in the Nordic Semiconductor USB Device Controller driver (usb_dc_nrfx).
    * Removed the ``CONFIG_USB_DFU_WAIT_DELAY_MS`` option.
      The :c:func:`wait_for_usb_dfu` function now takes the delay as a parameter.
    * Deprecated the :option:`CONFIG_USB_HID_PROTOCOL_CODE` option in favor of the added :c:func:`usb_hid_set_proto_code` function that allows setting the HID Boot Interface protocol code per device.

* Kernel:

  * Merged a new work queue implementation.
    See `this comment <kwork API changes_>`_ for details on the API changes.
  * Added a :c:macro:`K_SEM_MAX_LIMIT` define that users should provide in :c:func:`k_sem_init` as the limit value of semaphores that do not have explicit maximum limits and are instead just used for counting.
    This is meant as a replacement for using ``UINT_MAX``.
  * Moved the :option:`CONFIG_THREAD_MONITOR` and :option:`CONFIG_THREAD_NAME` options from experimental to production quality.
  * Removed the deprecated ``k_mem_domain_destroy`` and ``k_mem_domain_remove_thread`` APIs.
  * Updated the :c:func:`device_usable_check` and :c:func:`device_is_ready` functions so that they can be called from user space.

* Networking:

  * General:

    * Added UDP commands to the network shell.
    * Added verification of the network interface status before sending a packet.
    * Added missing translations for ``getaddrinfo()`` error codes.
    * Added a separate work queue for TCP2.
    * Added multiple bug fixes for IEEE 802.15.4 L2.
    * Fixed memory management issues in TCP2 when running out of memory.
    * Added connection establishment timer for TCP2.

  * LwM2M:

    * Fixed a bug where large LwM2M endpoint names were not encoded properly in the registration message.
    * Added API functions to update minimum/maximum observe period of a resource.

  * OpenThread:

    * Updated the OpenThread version to commit ``8f7024c3e9beb47a48cfc1e3185f5fce82fffba9``.
    * Added external heap implementation in OpenThread platform.
    * Removed an obsolete ``CONFIG_OPENTHREAD_NCP_BUFFER_SIZE`` option.
    * Added the following OpenThread options:

      * :option:`CONFIG_OPENTHREAD_COAP_BLOCK`
      * :option:`CONFIG_OPENTHREAD_MASTERKEY`
      * :option:`CONFIG_OPENTHREAD_SRP_CLIENT`
      * :option:`CONFIG_OPENTHREAD_SRP_SERVER`

  * MQTT:

    * Fixed logging of UTF-8 strings.

  * Sockets:

    * Fixed TLS sockets access from user space.

  * CoAP:

    * Added a symbol for the default COAP version.
    * Fixed a discovery response formatting.
    * Updated a few API functions to accept a const pointer when appropriate.

* Bluetooth:

  * Bluetooth Host:

    * Fixed a crash where an ATT timeout occurred on a disconnected ATT channel.
    * Removed definitions and functions that were deprecated since the v2.3.0 release.
    * Changed the pairing procedure to fail pairing when both sides have the same public key.
    * Fixed an issue where GATT requests might deadlock RX thread.
    * Fixed an issue where a fixed passkey that was previously set could not be cleared.
    * Fixed an issue where callbacks for "security changed" and "pairing failed" were not always called.
    * Changed the pairing procedure to fail early if the remote device cannot achieve the required security level.
    * Fixed an incomplete bond overwrite during the pairing procedure when the peer is not using the IRK stored in the bond.
    * Fixed an issue where GATT notifications and Writes Without Response might be sent out of order.
    * Changed buffer ownership of :c:func:`bt_l2cap_chan_send`.
      The application must now release the buffer for all returned errors.

  * Bluetooth Mesh:

    * Fixed error handling for the Friendship counter.
    * Added a replay check on segmented transport messages.
    * Added a poll callback for the Friend callback.
    * Added BabbleSim test suite.
    * Updated the publish period divisor of the health server to be stored persistently.
    * Added GATT Proxy callbacks.
    * Added address matching for Config Client response messages.
    * Added basic model behavior in the Mesh sample.
    * Added a composition data traversal API.
    * Added an acknowledged model message API.
    * Fixed the Config Server's LPN time reporting.

* Libraries/subsystems:

  * File systems:

    * Added an :c:func:`fs_file_t_init` function for initializing :c:struct:`fs_file_t` objects.
      All :c:struct:`fs_file_t` objects must now be initialized by calling this function before they can be used.
    * Added an :c:func:`fs_dir_t_init` function for initializing :c:struct:`fs_dir_t` objects.
      All :c:struct:`fs_dir_t` objects must now be initialized by calling this function before they can be used.
    * Deprecated the :option:`CONFIG_FS_LITTLEFS_FC_MEM_POOL` option and replaced it with :option:`CONFIG_FS_LITTLEFS_FC_HEAP_SIZE`.

  * Random:

    * Fixed a non-aligned access issue in the system timer based number generator.

  * Storage:

    * :ref:`zephyr:stream_flash`:

      * Fixed error handling for erase errors to not update the last erased page offset on failure.
      * Fixed error handling to not update the stream flash contex on synchronization failure while flushing the stream.


Matter (Project CHIP)
=====================

The Matter fork in the |NCS| (``sdk-connectedhomeip``) contains all commits from the upstream Matter repository up to and including ``0d5b9559ae``.

The following list summarizes the most important changes inherited from the upstream Matter:

* Added:

  * Completed the persistent storage feature, which allows Project CHIP devices to successfully communicate with each other even after reboot.
  * Added support for OpenThread's Service Registration Protocol (SRP) to enable the discovery of Project CHIP nodes using the DNS-SD protocol.
  * Added support for Network Commissioning Cluster, used when provisioning a Project CHIP node.
  * Enabled CHIP Reliable Messaging Protocol (CRMP) for the User Datagram Protocol (UDP) traffic within a Project CHIP network.

Documentation
=============

There are no entries for this section yet.

Known issues
************

Known issues are only tracked for the latest official release.
See `known issues for nRF Connect SDK v1.5.0`_ for the list of issues valid for this release.
