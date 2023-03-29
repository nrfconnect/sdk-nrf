.. _known_issues:

Known issues
############

.. contents::
   :local:
   :depth: 2

Known issues listed on this page *and* tagged with the :ref:`latest official release version <release_notes>` are valid for the current state of development.
Use the drop-down filter to see known issues for previous releases and check if they are still valid.

.. raw:: html
   :file: includes/filter.js

.. raw:: html

   Filter by versions:

   <select name="versions" id="versions-select">
     <option value="all">All versions</option>
     <option value="v2-3-0" selected>v2.3.0</option>
     <option value="v2-2-0">v2.2.0</option>
     <option value="v2-1-4">v2.1.4</option>
     <option value="v2-1-3">v2.1.3</option>
     <option value="v2-1-2">v2.1.2</option>
     <option value="v2-1-1">v2.1.1</option>
     <option value="v2-1-0">v2.1.0</option>
     <option value="v2-0-2">v2.0.2</option>
     <option value="v2-0-1">v2.0.1</option>
     <option value="v2-0-0">v2.0.0</option>
     <option value="v1-9-2">v1.9.2</option>
     <option value="v1-9-1">v1.9.1</option>
     <option value="v1-9-0">v1.9.0</option>
     <option value="v1-8-0">v1.8.0</option>
     <option value="v1-7-1">v1.7.1</option>
     <option value="v1-7-0">v1.7.0</option>
     <option value="v1-6-1">v1.6.1</option>
     <option value="v1-6-0">v1.6.0</option>
     <option value="v1-5-2">v1.5.2</option>
     <option value="v1-5-1">v1.5.1</option>
     <option value="v1-5-0">v1.5.0</option>
     <option value="v1-4-2">v1.4.2</option>
     <option value="v1-4-1">v1.4.1</option>
     <option value="v1-4-0">v1.4.0</option>
     <option value="v1-3-2">v1.3.2</option>
     <option value="v1-3-1">v1.3.1</option>
     <option value="v1-3-0">v1.3.0</option>
     <option value="v1-2-1">v1.2.1</option>
     <option value="v1-2-0">v1.2.0</option>
     <option value="v1-1-0">v1.1.0</option>
     <option value="v1-0-0">v1.0.0</option>
     <option value="v0-4-0">v0.4.0</option>
     <option value="v0-3-0">v0.3.0</option>
   </select>


.. HOWTO

   When adding a new version, add it to the dropdown list above and move the "selected" option next to it.
   Once "selected" is moved, only issues that are valid for the new version will be displayed when entering the page.

   Known issues process is described at https://projecttools.nordicsemi.no/confluence/pages/viewpage.action?pageId=82556815

   When updating this file, add entries in the following format:

   .. rst-class:: vXXX vYYY

   JIRA-XXXX: Title of the issue (with mandatory JIRA issue number since nRF Connect SDK v1.7.0)
     Description of the issue.
     Start every sentence on a new line and pay attention to indentations.

     There can be several paragraphs, but they must be indented correctly.

     **Workaround:** The last paragraph contains the workaround.
     The workaround is optional.

nRF9160
*******

Asset tracker
=============

.. rst-class:: v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0

NCSDK-6898: Setting :kconfig:option:`CONFIG_SECURE_BOOT` does not work
  The immutable bootloader is not able to find the required metadata in the MCUboot image.
  See the related NCSDK-6898 known issue in `Build system`_ for more details.

  **Workaround:** Set :kconfig:option:`CONFIG_FW_INFO` in MCUboot.

.. rst-class:: v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0

External antenna performance setting
  The preprogrammed Asset Tracker does not come with the best external antenna performance.

  **Workaround:** If you are using nRF9160 DK v0.15.0 or higher and Thingy:91 v1.4.0 or higher, set :kconfig:option:`CONFIG_NRF9160_GPS_ANTENNA_EXTERNAL` to ``y``.
  Alternatively, for nRF9160 DK v0.15.0, you can set the :kconfig:option:`CONFIG_NRF9160_GPS_COEX0_STRING` option to ``AT%XCOEX0`` when building the preprogrammed Asset Tracker to achieve the best external antenna performance.

.. rst-class:: v1-3-2 v1-3-1 v1-3-0

NCSDK-5574: Warnings during FOTA
   The nRF9160: Asset Tracker application prints warnings and error messages during successful FOTA.

.. rst-class:: v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0 v1-0-0 v0-4-0 v0-3-0

NCSDK-6689: High current consumption in Asset Tracker
  The nRF9160: Asset Tracker might show up to 2.5 mA current consumption in idle mode with :kconfig:option:`CONFIG_POWER_OPTIMIZATION_ENABLE` set to ``y``.

.. rst-class:: v1-0-0 v0-4-0 v0-3-0

Sending data before connecting to nRF Cloud
  The nRF9160: Asset Tracker application does not wait for connection to nRF Cloud before trying to send data.
  This causes the application to crash if the user toggles one of the switches before the kit is connected to the cloud.

.. rst-class:: v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0 v1-0-0 v0-4-0 v0-3-0

IRIS-2676: Missing support for FOTA on nRF Cloud
  The nRF9160: Asset Tracker application does not support the nRF Cloud FOTA_v2 protocol.

  **Workaround:** The implementation for supporting the nRF Cloud FOTA_v2 can be found in the following commits:

					* cef289b559b92186cc54f0257b8c9adc0997f334
					* 156d4cf3a568869adca445d43a786d819ae10250
					* f520159f0415f011ae66efb816384a8f7bade83d

Asset Tracker v2
================

.. rst-class:: v2-2-0

CIA-845: The application cannot be built with :file:`overlay-carrier.conf` (carrier library) enabled for Nordic Thingy:91
  Building with :ref:`liblwm2m_carrier_readme` library enabled for Nordic Thingy:91 will result in a ``FLASH`` overflow and a build error.

.. rst-class:: v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0

CIA-463: Wrong network mode parameter reported to cloud
  The network mode string present in ``deviceInfo`` (nRF Cloud) and ``dev`` (Azure IoT Hub and AWS IoT) JSON objects that is reported to cloud might contain wrong network modes.
  The network mode string contains the network modes that the modem is configured to use, not what the modem actually connects to the LTE network with.

.. rst-class:: v1-9-2 v1-9-1 v1-9-0

NCSDK-14235: Timestamps that are sent in cloud messages drift over time
  Due to a bug in the :ref:`lib_date_time` library, timestamps that are sent to cloud drift because they are calculated incorrectly.

.. rst-class:: v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-0

CIA-604: ATv2 cannot be built for the ``thingy91_nrf9160_ns`` build target with ``SECURE_BOOT`` enabled
  Due to the use of static partitions with the Thingy:91, there is insufficient room in the flash memory to enable both the primary and secondary bootloaders.

.. rst-class:: v2-0-2 v2-0-1 v2-0-0

CIA-661: Asset Tracker v2 application configured for LwM2M cannot be built for the ``nrf9160dk_nrf9160_ns`` build target with modem traces or Memfault enabled
  The :ref:`asset_tracker_v2` application configured for LwM2M cannot be built for the ``nrf9160dk_nrf9160_ns`` build target with :kconfig:option:`CONFIG_NRF_MODEM_LIB_TRACE` for modem traces or :file:`overlay-memfault.conf` for Memfault due to memory constraints.

  **Workaround:** Use one of the following workarounds for modem traces:

  * Use Secure Partition Manager instead of TF-M by setting ``CONFIG_SPM`` to ``y`` and :kconfig:option:`CONFIG_BUILD_WITH_TFM` to ``n``.
  * Reduce the value of :kconfig:option:`CONFIG_NRF_MODEM_LIB_SHMEM_TRACE_SIZE` to 8 Kb, however, this might lead to loss of modem traces.

  For Memfault, use Secure Partition Manager instead of TF-M by setting ``CONFIG_SPM`` to ``y`` and :kconfig:option:`CONFIG_BUILD_WITH_TFM` to ``n``.

.. rst-class:: v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0

CIA-890: The application cannot be built with :file:`overlay-debug.conf` and :kconfig:option:`CONFIG_DEBUG_OPTIMIZATIONS` set to ``y``
  Due to insufficient flash space for the application when it is not optimized, the :ref:`asset_tracker_v2` application cannot be built with :file:`overlay-debug.conf` and :kconfig:option:`CONFIG_DEBUG_OPTIMIZATIONS` set to ``y``.

  **Workaround:** To free up flash space when debugging locally, comment out the following Kconfig options in the :file:`prj.conf` file:

  * :kconfig:option:`CONFIG_BOOTLOADER_MCUBOOT`
  * :kconfig:option:`CONFIG_IMG_MANAGER`
  * :kconfig:option:`CONFIG_MCUBOOT_IMG_MANAGER`
  * :kconfig:option:`CONFIG_IMG_ERASE_PROGRESSIVELY`
  * :kconfig:option:`CONFIG_BUILD_S1_VARIANT`

  This removes the partitions for the MCUboot bootloader, the secondary bootloader, and the secondary application image slot.
  Any functionality depending on those will not work with this configuration.

  Alternatively, disable logging for non-relevant modules or libraries in the :file:`overlay-debug.conf` file until the image fits in flash.

Serial LTE Modem
================

.. rst-class:: v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0

NCSDK-13895: Build failure for target Thingy:91 with secure_bootloader overlay
  Building the application for Thingy:91 fails if secure_bootloader overlay is included.

.. rst-class:: v2-3-0

NCSDK-20047: SLM logging over RTT is not available
  There is a conflict with MCUboot RTT logging.
  In order to save power, SLM configures MCUboot to use RTT instead of UART for logging.
  SLM itself uses RTT for logging as well.
  With a recent change, MCUboot exclusively takes control of the RTT logging, which causes the conflict.

  **Workaround:** Remove ``CONFIG_USE_SEGGER_RTT=y`` and ``CONFIG_RTT_CONSOLE=y`` from :file:`child_image\mcuboot.conf`.

.. _known_issues_other:

Other issues
============

.. rst-class:: v2-3-0

CIA-892: Assert or crash when printing long strings with the ``%s`` qualifier
  When logging a string with the ``%s`` qualifier, the maximum length of any inserted string is 1022 bytes.
  If the string is longer, an assert or a crash can occur.
  Given that the default value of the :kconfig:option:`CONFIG_LOG_PRINTK` Kconfig option has been changed from ``n`` to ``y`` in the |NCS| v2.3.0 release, the :c:func:`printk` function may also cause this issue, unless the application disables this option.

  **Workaround:** To fix the issue, cherry-pick commits from the upstream `Zephyr PR #54901 <https://github.com/zephyrproject-rtos/zephyr/pull/54901>`_.

.. rst-class:: v2-3-0 v2-2-0

CIA-857: LTE Link Controller spurious events
  When using the :ref:`lte_lc_readme` library, a memory comparison is done between padded structs that may result in comparing bytes that have undefined initialization values.
  This may cause spurious ``LTE_LC_EVT_CELL_UPDATE`` and ``LTE_LC_EVT_PSM_UPDATE`` events even though the information contained in the event has not changed since last update event.

.. rst-class:: v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0

NCSDK-15512: Modem traces retrieval incompatible with TF-M
  Enabling modem traces with :kconfig:option:`CONFIG_UART NRF_MODEM_LIB_TRACE_BACKEND_UART` set to ``y`` will disable TF-M logging from secure processing environment (using :kconfig:option:`CONFIG_TFM_LOG_LEVEL_SILENCE` set to ``y``) including output from hardware fault handlers.
  You can either use **UART1** for TF-M output or for modem traces, but not for both.

.. rst-class:: v2-0-2 v2-0-1 v2-0-0

NCSDK-15471: Compilation with SUPL client library fails when TF-M is enabled
  Building an application that uses the SUPL client library fails if TF-M is used.

  **Workaround:** Use one of the following workarounds:

  * Use Secure Partition Manager instead of TF-M.
  * Disable the FPU by setting :kconfig:option:`CONFIG_FPU` to ``n``.

.. rst-class:: v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0

CIA-351: Connectivity issues with Azure IoT Hub
  If a ``device-bound`` message is sent to the device while it is in the LTE Power Saving Mode (PSM), the TCP connection will most likely be terminated by the server.
  Known symptoms of this are frequent reconnections to cloud, messages sent to Azure IoT Hub never arriving, and FOTA images being downloaded twice.

  **Workaround:** Avoid using LTE Power Saving Mode (PSM) and extended DRX intervals longer than approximately 30 seconds. This will reduce the risk of the issue occurring, at the cost of increased power consumption.

.. rst-class:: v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0

NCSDK-10106: Elevated current consumption when using applications without :ref:`nrfxlib:nrf_modem` on nRF9160
  When running applications that do not enable :ref:`nrfxlib:nrf_modem` on nRF9160 with build code B1A, current consumption will stay at 3 mA when in sleep.

  **Workaround:** Enable :ref:`nrfxlib:nrf_modem`.

.. rst-class:: v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0

NCSDK-8075: Invalid initialization of ``mbedtls_entropy_context`` mutex type
  The calls to :cpp:func:`mbedtls_entropy_init` do not zero-initialize the member variable ``mutex`` when ``nrf_cc3xx`` is enabled.

  **Workaround:** Zero-initialize the structure type before using it or make it a static variable to ensure that it is zero-initialized.

.. rst-class:: v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0 v1-0-0

Receive error with large packets
  nRF91 fails to receive large packets (over 4000 bytes).

.. rst-class:: v1-9-2 v1-9-1 v1-9-0

The time returned by :ref:`lib_date_time` library becomes incorrect after one week of uptime
  The time returned by :ref:`lib_date_time` library becomes incorrect after one week elapses.
  This is due to an issue with clock_gettime() API.

.. _tnsw_46156:

.. rst-class:: v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0

TNSW-46156: The LwM2M carrier library can in some cases trigger a modem bug related to PDN deactivation (AT+CGACT) in NB-IoT mode.
  This causes excessive network signaling which can make interactions with the modem (AT commands or :ref:`nrfxlib:nrf_modem` functions) hang for several minutes.

  **Workaround:** Add a small delay before PDN deactivation as shown in `Pull Request #10379 <https://github.com/nrfconnect/sdk-nrf/pull/10379>`_.

.. rst-class:: v2-3-0 v2-2-0

NCSDK-18746: LwM2M carrier library fails to complete non-secure bootstrap when using a custom URI and the default security tag
  If the LwM2M carrier library is operating in generic mode (not connecting to any of the predefined supported carriers), and the :kconfig:option:`CONFIG_LWM2M_CARRIER_CUSTOM_URI` is set to connect to a non-secure server, the library attempts to retrieve a PSK from :kconfig:option:`CONFIG_LWM2M_CARRIER_SERVER_SEC_TAG` even though a PSK is not needed for the non-secure bootstrap.
  The PSK in this ``sec_tag`` is not used, but reading the ``sec_tag`` causes the bootstrap to fail if the :kconfig:option:`CONFIG_LWM2M_CARRIER_SERVER_SEC_TAG` is set to the default value (``0``).

  **Workaround:** Assign any non-zero value to :kconfig:option:`CONFIG_LWM2M_CARRIER_SERVER_SEC_TAG`.

.. rst-class:: v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-3-1 v1-2-1 v1-2-0

NCSDK-12912: LwM2M carrier library does not recover if initial network connection fails
  When the device is switched on, if :cpp:func:`lte_lc_connect()` returns an error at timeout, it will cause :cpp:func:`lwm2m_carrier_init()` to fail.
  Thus, the device will fail to connect to carrier device management servers.

  **Workaround:** Increase :kconfig:option:`CONFIG_LTE_NETWORK_TIMEOUT` to allow :ref:`lte_lc_readme` more time to successfully connect.

.. rst-class:: v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-1 v1-5-0 v1-4-2 v1-3-1 v1-2-1 v1-2-0

NCSDK-12913: LwM2M carrier library will fail to initialize if phone number is not present in SIM
  The SIM phone number is needed during the :ref:`liblwm2m_carrier_readme` library start-up.
  For new SIM cards, it might take some time before the phone number is received by the network.
  The LwM2M carrier library does not wait for this to happen.
  Thus, the device can fail to connect to the carrier's device management servers.

  **Workaround:** Use one of the following workarounds:

  * Reboot or power-cycle the device after the SIM has received a phone number from the network.
  * Apply the following commits, depending on your |NCS| version:

    * `v1.7-branch <https://github.com/nrfconnect/sdk-nrf/pull/6287>`_
    * `v1.6-branch <https://github.com/nrfconnect/sdk-nrf/pull/6286>`_
    * `v1.4-branch <https://github.com/nrfconnect/sdk-nrf/pull/6270>`_

.. rst-class:: v1-8-0

NCSDK-13106: When replacing a Verizon SIM card, the LwM2M carrier library does not reconnect to the device management servers
  When a Verizon SIM card is replaced with a new Verizon SIM card, the library fails to fetch the correct PSK for the bootstrap server.
  Thus, the device fails to connect to the carrier's device management servers.

  **Workaround:** Use one of the following workarounds:

  * Use the :kconfig:option:`CONFIG_LWM2M_CARRIER_USE_CUSTOM_PSK` and :kconfig:option:`CONFIG_LWM2M_CARRIER_CUSTOM_PSK` configuration options to set the appropriate PSK needed for Verizon test or live servers.
    This PSK can be obtain from the carrier.
  * After inserting a new SIM card, reboot the device again.

.. rst-class:: v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0

NCSDK-11684: Failure loading KMU registers on nRF9160 devices
  Certain builds will experience problems loading HUK to KMU due to a bug in nrf_cc3xx_platform library prior to version 0.9.12.
  The problem arises in certain builds depending on alignment of code.
  The reason for the issue is improper handling of PAN 7 on nRF9160 devices.

  **Workaround:** Update to nrf_cc3xx_platform/nrf_cc3xx_mbedcrypto v0.9.12 or newer versions if KMU is needed.

.. rst-class:: v1-7-1 v1-7-0

NCSDK-11033: Dial-up usage not working
  Dial-up usage with MoSh PPP does not work and causes the nRF9160 DK to crash when it is connected to a PC.

  **Workaround:** Manually pick the fix available in Zephyr to the `Zephyr issue #38516`_.

.. rst-class:: v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0

NCSDK-7856: Faulty indirection on ``nrf_cc3xx`` memory slab when freeing the platform mutex
  The :cpp:func:`mutex_free_platform` function has a bug where a call to :cpp:func:`k_mem_slab_free` provides wrong indirection on a parameter to free the platform mutex.

  **Workaround:** Write the call to free the mutex in the following way: ``k_mem_slab_free(&mutex_slab, &mutex->mutex)``.
  The change adds ``&`` before the parameter ``mutex->mutex``.

.. rst-class:: v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0

NCSDK-7914: The ``nrf_cc3xx`` RSA implementation does not deduce missing parameters
  The calls to :cpp:func:`mbedtls_rsa_complete` will not deduce all types of missing RSA parameters when using ``nrf_cc3xx`` v0.9.6 or earlier.

  **Workaround:** Calculate the missing parameters outside of this function or update to ``nrf_cc3xx`` v0.9.7 or later.

.. rst-class:: v1-4-2 v1-4-1 v1-4-0

NRF91-989: Unable to bootstrap after changing SIMs
  In some cases, swapping the SIM card might trigger the bootstrap Pre-Shared Key to be deleted from the device. This can prevent future bootstraps from succeeding.

.. rst-class:: v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0

NCSDK-5666: LTE Sensor Gateway
  The :ref:`lte_sensor_gateway` sample crashes when Thingy:52 is flipped.

.. rst-class:: v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0

NCSDK-6073: ``nrf_send`` is blocking
  The :cpp:func:`nrf_send` function in the :ref:`nrfxlib:nrf_modem` might be blocking for several minutes, even if the socket is configured for non-blocking operation.
  The behavior depends on the cellular network connection.

  **Workaround:** For |NCS| v1.4.0, set the non-blocking mode for a partial workaround for non-blocking operation.

.. rst-class:: v1-2-0

GPS sockets and SUPL client library stops working
  The `nRF9160: GPS with SUPL client library <https://developer.nordicsemi.com/nRF_Connect_SDK/doc/1.2.0/nrf/samples/nrf9160/gps/README.html>`_ sample stops working if :ref:`supl_client` support is enabled, but the SUPL host name cannot be resolved.

  **Workaround:** Insert a delay (``k_sleep()``) of a few seconds after the ``printf`` on line 294 in :file:`main.c`.

.. rst-class:: v1-2-0 v1-1-0 v1-0-0

Calling ``nrf_connect()`` immediately causes fail
  ``nrf_connect()`` fails if called immediately after initialization of the device.
  A delay of 1000 ms is required for this to work as intended.

.. rst-class:: v1-2-0 v1-1-0 v1-0-0 v0-4-0 v0-3-0

Problems with RTT Viewer/Logger
  The SEGGER Control Block cannot be found by automatic search by the RTT Viewer/Logger.

  **Workaround:** Set the RTT Control Block address to 0 and it will try to search from address 0 and upwards.
  If this does not work, look in the :file:`builddir/zephyr/zephyr.map` file to find the address of the ``_SEGGER_RTT`` symbol in the map file and use that as input to the viewer/logger.

.. rst-class:: v1-0-0 v0-4-0 v0-3-0

Modem FW reset on debugger connection through SWD
  If a debugger (for example, J-Link) is connected through SWD to the nRF9160, the modem firmware will reset.
  Therefore, the LTE modem cannot be operational during debug sessions.

.. rst-class:: v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0

NCSDK-9441: Fmfu SMP server sample is unstable with the newest J-Link version
  Full modem serial update does not work on development kit with debugger chip version delivered with J-Link software > 6.88a

  **Workaround:** Downgrade the debugger chip to the firmware released with J-Link 6.88a or use another way of transferring serial data to the chip.

.. rst-class:: v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0

NCSDK-20026: LTE Sensor Gateway fails to run
  The :ref:`lte_sensor_gateway` sample system faults when the board version is not included with the build target while building the sample.
  This is due to an issue with the low-power UART and HCI drivers.

  **Workaround:** Include the board version when building the :ref:`bluetooth-hci-lpuart-sample` sample and the :ref:`lte_sensor_gateway` sample.
  For example, for board version 1.1.0, the samples must be built in the following way:

  The :ref:`bluetooth-hci-lpuart-sample` sample:

  .. parsed-literal::
     :class: highlight

     west build --board nrf9160dk_nrf52840@1.1.0

  The :ref:`lte_sensor_gateway` sample:

  .. parsed-literal::
     :class: highlight

     west build --board nrf9160dk_nrf9160_ns@1.1.0

.. rst-class:: v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0

CIA-738: FMFU does not use external flash partitions
  Full modem FOTA support assumes full control of the external flash; it does not use an external flash partition.
  It cannot be combined with other usages, such as storing settings or P-GPS data.

.. rst-class:: v2-2-0

NCSDK-18376: P-GPS in external flash fails to inject first prediction during load sequence
  When using external flash with P-GPS, stored predictions cannot be reliably read to satisfy the modem's need for ephemeris data when actively downloading and storing predictions to the same external flash.
  Once the download is complete, the predictions can be reliably read.

nRF7002
*******

.. rst-class:: v2-3-0

SHEL-1352: Incorrect base address used in the OTP TX trim coefficients
  Incorrect base address used for TX trim coefficients in the One-Time Programmable (OTP) memory results in transmit power deviating by +/- 2 dB from the target value.

nRF5
****

nRF5340
=======

.. rst-class:: v2-2-0

NCSDK-20070: The :ref:`direct_test_mode` antenna switching does not work on the nRF5340 DK with the nRF21540 EK shield.
  The antenna select DTM command does not have any effect because the GPIO pin which controls antenna is not forwarded to nRF5340DK network core.

  **Workaround** Add a ``<&gpio1 6 0>`` entry in :file:`samples/bluetooth/direct_test_mode/conf/remote_shell/pin_fwd.dts`.

.. rst-class:: v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0

NCSDK-16856: Increased power consumption observed for the Low Power UART sample on nRF5340 DK
  The power consumption of the :ref:`lpuart_sample` sample measured using the nRF5340 DK v2.0.0 is about 200 uA higher than expected.

  **Workaround:** Disconnect flow control lines for VCOM2 with the SW7 DIP switch on the board.

.. rst-class:: v2-0-2 v2-0-1 v2-0-0

NCSDK-16338: Setting gain for nRF21540 Front-end module does not work in the :ref:`radio_test` sample
  Setting FEM gain for nRF21540 Front-end module does not work in the :ref:`radio_test` sample with the nRF5340 SoC.

  **Workaround:** Enable the SPI 0 instance in the nRF5340 network core DTS overlay file.

.. rst-class:: v1-9-0

NCSDK-13925: Build warning in the RF test samples when the nRF21540 EK support is enabled
  :ref:`radio_test` and :ref:`direct_test_mode` build with warnings for nRF5340 with the :ref:`ug_radio_fem_nrf21540_ek` support.

  **Workaround:** Change the parameter type in the :c:func:`nrf21540_tx_gain_set()` function in :file:`ncs/nrf/samples/bluetooth/direct_test_mode/src/fem/nrf21540.c` from :c:type:`uint8_t` to :c:type:`uint32_t`.

.. rst-class:: v2-0-0 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0

NCSDK-11432: DFU: Erasing secondary slot returns error response
  Trying to erase secondary slot results in an error response.
  Slot is still erased.
  This issue is only occurring when the application is compiled for multi-image.

.. rst-class:: v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0

NCSDK-9786: Wrong FLASH_PAGE_ERASE_MAX_TIME_US for the nRF53 network core
  ``FLASH_PAGE_ERASE_MAX_TIME_US`` defines the execution window duration when doing the flash operation synchronously along the radio operations (:kconfig:option:`CONFIG_SOC_FLASH_NRF_PARTIAL_ERASE` not enabled).

  The ``FLASH_PAGE_ERASE_MAX_TIME_US`` value of the nRF53 network core is lower than required.
  For this reason, if :kconfig:option:`CONFIG_SOC_FLASH_NRF_RADIO_SYNC_MPSL` is set to ``y`` and :kconfig:option:`CONFIG_SOC_FLASH_NRF_PARTIAL_ERASE` is set to ``n``, a flash erase operation on the nRF5340 network core will result in an MPSL timeslot OVERSTAYED assert.

  **Workaround:** Increase ``FLASH_PAGE_ERASE_MAX_TIME_US`` (defined in :file:`ncs/zephyr/soc/arm/nordic_nrf/nrf53/soc.h`) from 44850UL to 89700UL (the same value as for the application core).

.. rst-class:: v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0

NCSDK-7234: UART output is not received from the network core
  The UART output is not received from the network core if the application core is programmed and running with a non-secure image (using the ``nrf5340dk_nrf5340_cpuapp_ns`` build target).

.. rst-class:: v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0

KRKNWK-6756: 802.15.4 Service Layer (SL) library support for the nRF53
  The binary variant of the 802.15.4 Service Layer (SL) library for the nRF53 does not support such features as synchronization of **TIMER** with **RTC** or timestamping of received frames.
  For this reason, 802.15.4 features like delayed transmission or delayed reception are not available for the nRF53.

.. rst-class:: v1-3-2 v1-3-1 v1-3-0

FOTA does not work
  FOTA with the :ref:`zephyr:smp_svr_sample` does not work.

Nordic Thingy:53
================

.. rst-class:: v2-2-0

NCSDK-18263: |NCS| samples may fail to boot on Thingy:53
  |NCS| samples and applications that are not listed under :ref:`thingy53_compatible_applications` fail to boot on Nordic Thingy:53.
  The MCUboot bootloader is not built together with these samples, but the Thingy:53's :ref:`static Partition Manager memory map <ug_pm_static>` requires it (the application image does not start at the beginning of the internal ``FLASH``.)

  **Workaround:** Revert the ``9a8680372fdb6e09f3d6537c8c6751dd5c50bf86`` commit in the sdk-zephyr repository and revert the ``1f9765df5acbb36afff0ce40c94ba65d44d19d70`` commit in sdk-nrf.
  During conflict resolution, make sure to update the :file:`west.yml` file in the sdk-nrf to point to the reverting commit in sdk-zephyr.

nRF52820
========

.. rst-class:: v1-3-2 v1-3-1 v1-3-0

Missing :file:`CMakeLists.txt`
  The :file:`CMakeLists.txt` file for developing applications that emulate nRF52820 on the nRF52833 DK is missing.

  **Workaround:** Create a :file:`CMakeLists.txt` file in the :file:`ncs/zephyr/boards/arm/nrf52833dk_nrf52820` folder with the following content:

  .. parsed-literal::
    :class: highlight

    zephyr_compile_definitions(DEVELOP_IN_NRF52833)
    zephyr_compile_definitions(NRFX_COREDEP_DELAY_US_LOOP_CYCLES=3)

  You can `download this file <nRF52820 CMakeLists.txt_>`_ from the upstream Zephyr repository.
  After you add it, the file is automatically included by the build system.

Thread
======

.. rst-class:: v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1

KRKNWK-14756: Increased average latency during communication with nRF5340-based SED
  The measured average latency (RTT) of the Echo Request/Response transaction sometimes shows a slight increase over the baseline when the receiver is a Sleepy End Device based on the nRF5340 SoC platform.

.. rst-class:: v2-0-0

KRKNWK-14231: Device stops receiving after switching from SSED to MED
  Trying to switch to the MED mode after working as CSL Receiver makes the device stop receiving frames.

  **Workaround:** Before invoking :c:func:`otThreadSetLinkMode` to change the device mode, make sure to set the CSL Period to ``0`` with :c:func:`otLinkCslSetPeriod`.

.. rst-class:: v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0

KRKNWK-9094: Possible deadlock in shell subsystem
  Issuing OpenThread commands too fast might cause a deadlock in the shell subsystem.

  **Workaround:** If possible, avoid invoking a new command before execution of the previous one has completed.

.. rst-class:: v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0

KRKNWK-6848: Reduced throughput
  Performance testing for the :ref:`ot_coprocessor_sample` sample shows a decrease of throughput of around 10-20% compared with the standard OpenThread.

.. rst-class:: v1-9-0

KRKNWK-13059: Wrong MAC frame counter is reported sometimes
  The reporting of the wrong MAC frame counter causes the neighbor to drop subsequent frames from the device due to security checks.
  This issue only affects to Thread 1.2 builds.

  **Workaround:** To fix the issue, update the sdk-zephyr repository by cherry-picking the commit with the hash ``1ab6be252335ceec5a966b36fbc79883ebd1c4d1``.

.. rst-class:: v1-7-0

KRKNWK-11555: Devices lose connection after a long time running
   Connection is sometimes lost after Key Sequence update.

   .. note::
      Due to this issue, |NCS| v1.7.0 will not undergo the certification process, and is not intended to be used in final Thread products.

.. rst-class:: v1-7-0

KRKNWK-11264: Some boards assert during high traffic
   The issue appears when traffic is high during a corner case, and has been observed after running stress tests for a few hours.

   .. note::
      Due to this issue, |NCS| v1.7.0 will not undergo the certification process, and is not intended to be used in final Thread products.

.. rst-class:: v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0

Zephyr systems with OpenThread become unresponsive after some time
   Systems become unresponsive after running around 49.7 days.

   **Workaround:** Rebooting the system regularly avoids the issue.
   To fix the error, cherry-pick commits from the upstream `Zephyr issue #39704 <https://github.com/zephyrproject-rtos/zephyr/issues/39704>`_.

.. rst-class:: v1-6-1 v1-6-0

KRKNWK-10633: Incorrect data when using ACK-based Probing with Link Metrics
  When using the ACK-based Probing enhanced with Link Metrics, the Thread Information Element contains fixed data instead of the correct Link Metrics data for the acknowledged frame.

.. rst-class:: v1-6-1 v1-6-0

KRKNWK-10467: Security issues for retransmitted frames with Thread 1.2
  The Thread 1.2 current implementation does not guarantee that all retransmitted frames will be secured when using the transmission security capabilities of the radio driver.
  For this reason, OpenThread retransmissions are disabled by default when the :kconfig:option:`CONFIG_NRF_802154_ENCRYPTION` Kconfig option is enabled.
  You can enable the retransmissions at your own risk.

.. rst-class:: v1-6-1 v1-6-0

KRKNWK-11037:  ``Udp::GetEphemeralPort`` can cause infinite loop
  Using ``Udp::GetEphemeralPort`` in OpenThread can potentially cause an infinite loop.

  **Workaround:** Avoid using ``Udp::GetEphemeralPort``.

.. rst-class:: v1-5-2 v1-5-1

KRKNWK-9461 / KRKNWK-9596 : Multiprotocol sample crashes with some smartphones
  With some smartphones, the multiprotocol sample crashes on the nRF5340 due to timer timeout inside the 802.15.4 radio driver logic.

.. rst-class:: v1-4-2 v1-4-1 v1-4-0

KRKNWK-7885: Throughput is lower when using CC310 nrf_security backend
  A decrease of throughput of around 5-10% has been observed for the :ref:`CC310 nrf_security backend <nrfxlib:nrf_security_backends_cc3xx>` when compared with :ref:`nrf_oberon <nrf_security_backends_oberon>` or :ref:`the standard mbedtls backend <nrf_security_backends_orig_mbedtls>`.
  CC310 nrf_security backend is used by default for nRF52840 boards.
  The source of throughput decrease is coupled to the cost of RTOS mutex locking when using the :ref:`CC310 nrf_security backend <nrfxlib:nrf_security_backends_cc3xx>` when the APIs are called with shorter inputs.

  **Workaround:** Use AES-CCM ciphers from the nrf_oberon backend by setting the following options:

  * :kconfig:option:`CONFIG_OBERON_BACKEND` to ``y``
  * :kconfig:option:`CONFIG_OBERON_MBEDTLS_AES_C` to ``y``
  * :kconfig:option:`CONFIG_OBERON_MBEDTLS_CCM_C` to ``y``
  * :kconfig:option:`CONFIG_CC3XX_MBEDTLS_AES_C` to ``n``

.. rst-class:: v1-4-2 v1-4-1 v1-4-0

KRKNWK-7721: MAC counter updating issue
  The ``RxDestAddrFiltered`` MAC counter is not being updated.
  This is because the ``PENDING_EVENT_RX_FAILED`` event is not implemented in Zephyr.

  **Workaround:** To fix the error, cherry-pick commits from the upstream `Zephyr PR #29226 <https://github.com/zephyrproject-rtos/zephyr/pull/29226>`_.

.. rst-class:: v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0

KRKNWK-7962: Logging interferes with shell output
  :kconfig:option:`CONFIG_LOG_MODE_MINIMAL` is configured by default for most OpenThread samples.
  It accesses the UART independently from the shell backend, which sometimes leads to malformed output.

  **Workaround:** Disable logging or enable a more advanced logging option.

.. rst-class:: v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0

KRKNWK-7803: Automatically generated libraries are missing otPlatLog for NCP
  When building OpenThread libraries using a different sample than the :ref:`ot_coprocessor_sample` sample, the :file:`ncp_base.cpp` is not compiled with the :c:func:`otPlatLog` function.
  This results in a linking failure when building the NCP with these libraries.

  **Workaround:** Use the :ref:`ot_coprocessor_sample` sample to create OpenThread libraries.

.. rst-class:: v1-3-1 v1-3-0

NCSDK-5014: Building with SES not possible
  It is not possible to build Thread samples using SEGGER Embedded Studio (SES).
  SES does not support :file:`.cpp` files in |NCS| projects.

.. rst-class:: v1-3-2 v1-3-1 v1-3-0

KRKNWK-6358: CoAP client sample provisioning issues
  It is not possible to provision the :ref:`coap_client_sample` sample to servers that it cannot directly communicate with.
  This is because Link Local Address is used for communication.

.. rst-class:: v1-3-2 v1-3-1 v1-3-0

KRKNWK-6408: ``diag`` command not supported
  The ``diag`` command is not yet supported by Thread in the |NCS|.

Zigbee
======

.. rst-class:: v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0

KRKNWK-14024: Fatal error when the network coordinator factory resets in the Identify mode
  A fatal error occurs when the :ref:`Zigbee network coordinator <zigbee_network_coordinator_sample>` triggers factory reset in the Identify mode.

  **Workaround:** Modify your application, so that the factory reset is requested only after the Identify mode ends.

.. rst-class:: v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0

KRKNWK-12937: Activation of Sleepy End Device must be done at the very first commissioning procedure for Zigbee light switch sample
   After programming the :ref:`Zigbee light switch <zigbee_light_switch_sample>` sample and its first commissioning, Zigbee End Device joins the Zigbee network as a normal End Device. Pressing **Button 3** does not switch the device to the Sleepy End Device configuration.

   **Workaround:** Keep **Button 3** pressed during the first commissioning procedure.

.. rst-class:: v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0

KRKNWK-12615: Get Group Membership Command returns all groups the node is assigned to
   Get Group Membership Command returns all groups the node is assigned to regardless of the destination endpoint.

.. rst-class:: v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0

KRKNWK-12115: Simultaneous commissioning of many devices can cause the Coordinator device to assert
  The Zigbee Coordinator device can assert when multiple devices are being commissioned simultaneously.
  In some cases, the device can end up in the low memory state as the result.

  **Workaround:** To lower the likelihood of the Coordinator device asserting, increase its scheduler queue and buffer pool by completing the following steps:

  1. Create your own custom memory configuration file by creating an empty header file for your application, similar to :file:`include/zb_mem_config_custom.h` header file in the :ref:`Zigbee light switch <zigbee_light_switch_sample>` sample.
  #. Copy the contents of :file:`zb_mem_config_max.h` memory configuration file to the memory configuration header file you have just created.
     The Zigbee Network Coordinator sample uses the contents of the memory configuration file by default.
  #. In your custom memory configuration file, locate the following code:

     .. code-block:: c

        /* Now if you REALLY know what you do, you can study zb_mem_config_common.h and redefine some configuration parameters, like:
        #undef ZB_CONFIG_SCHEDULER_Q_SIZE
        #define ZB_CONFIG_SCHEDULER_Q_SIZE 56
        */

  #. Replace the code you have just located with the following code:

     .. code-block:: c

        /* Increase Scheduler queue size. */
        undef ZB_CONFIG_SCHEDULER_Q_SIZE
        define ZB_CONFIG_SCHEDULER_Q_SIZE XYZ
        /* Increase buffer pool size. */
        undef ZB_CONFIG_IOBUF_POOL_SIZE
        define ZB_CONFIG_IOBUF_POOL_SIZE XYZ

  #. To increase the scheduler queue size, replace ``XYZ`` next to ``ZB_CONFIG_SCHEDULER_Q_SIZE`` with the value of your choice, ranging from ``48U`` to ``256U``.
  #. To increase the buffer pool size, replace ``XYZ`` next to ``ZB_CONFIG_IOBUF_POOL_SIZE`` with the value of your choice, ranging from ``48U`` to ``127U``.

.. rst-class:: v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0

KRKNWK-11826: Zigbee Router does not accept new child devices if the maximum number of children is reached
  Once the maximum number of children devices on a Zigbee Router is reached and one of them leaves the network, the Zigbee Router does not update the flags inside beacon frames to indicate that it cannot accept new devices.

  **Workaround:** If the maximum number of child devices has been reached, call ``bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING)`` on the parent router from the ``ZB_ZDO_SIGNAL_LEAVE_INDICATION`` signal handler.

.. rst-class:: v1-9-2 v1-9-1 v1-9-0 v1-8-0

KRKNWK-11704: NCP communication gets stuck
  The communication between the SoC and the NCP Host sometimes stops on the SoC side.
  The device neither sends nor accepts incoming packets.
  Currently, there is no workaround for this issue.

.. rst-class:: v1-9-2 v1-9-1 v1-9-0

KRKNWK-12522: Incorrect Read Attributes Response on reading multiple attributes when the first attribute is unsupported
   When reading multiple attributes at once and the first one is not supported, the Read Attributes Response contains two records for the first supported attribute.
   The first one record has the Status field filled with Unsupported Attribute whereas the second record contains actual data.

.. rst-class:: v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0

KRKNWK-12017: Zigbee End Device does not recover from broken rejoin procedure
  If the Device Announcement packet is not acknowledged by the End Device's parent, joiner logic is stopped and the device does not recover.

  **Workaround:** Complete the following steps to detect when the rejoin procedure breaks and reset the device:

  1. Introduce helper variable ``joining_signal_received``.
  #. Extend ``zigbee_default_signal_handler()`` by completing the following steps:

     a. Set ``joining_signal_received`` to ``true`` in the following signals: ``ZB_BDB_SIGNAL_DEVICE_FIRST_START``, ``ZB_BDB_SIGNAL_DEVICE_REBOOT``, ``ZB_BDB_SIGNAL_STEERING``.
     #. If ``leave_type`` is set to ``ZB_NWK_LEAVE_TYPE_REJOIN``, set ``joining_signal_received`` to ``false`` in the ``ZB_ZDO_SIGNAL_LEAVE`` signal.
     #. Handle the ``ZB_NLME_STATUS_INDICATION`` signal to detect when End Device failed to transmit packet to its parent, reported by signal's status ``ZB_NWK_COMMAND_STATUS_PARENT_LINK_FAILURE``.

  See the following snippet for an example:

  .. code-block:: c

     /* Add helper variable that will be used for detecting broken rejoin procedure. */
     /* Flag indicating if joining signal has been received since restart or leave with rejoin. */
     bool joining_signal_received = false;
     /* Extend the zigbee_default_signal_handler() function. */
     case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
         ...
         joining_signal_received = true;
         break;
     case ZB_BDB_SIGNAL_DEVICE_REBOOT:
         ...
         joining_signal_received = true;
         break;
     case ZB_BDB_SIGNAL_STEERING:
         ...
         joining_signal_received = true;
         break;
     case ZB_ZDO_SIGNAL_LEAVE:
         if (status == RET_OK) {
             zb_zdo_signal_leave_params_t *leave_params = ZB_ZDO_SIGNAL_GET_PARAMS(sig_hndler, zb_zdo_signal_leave_params_t);
             LOG_INF("Network left (leave type: %d)", leave_params->leave_type);

             /* Set joining_signal_received to false so broken rejoin procedure can be detected correctly. */
             if (leave_params->leave_type == ZB_NWK_LEAVE_TYPE_REJOIN) {
                 joining_signal_received = false;
             }
         ...
         break;
     case ZB_NLME_STATUS_INDICATION: {
         zb_zdo_signal_nlme_status_indication_params_t *nlme_status_ind =
             ZB_ZDO_SIGNAL_GET_PARAMS(sig_hndler, zb_zdo_signal_nlme_status_indication_params_t);
         if (nlme_status_ind->nlme_status.status == ZB_NWK_COMMAND_STATUS_PARENT_LINK_FAILURE) {

             /* Check for broken rejoin procedure and restart the device to recover. */
             if (stack_initialised && !joining_signal_received) {
                 zb_reset(0);
             }
         }
         break;
     }

.. rst-class:: v1-8-0

KRKNWK-11465: OTA Client issues in the Image Block Request
  OTA Client cannot send Image Block Request with ``MinimumBlockPeriod`` attribute value set to ``0``.

  **Workaround:** Complete the following steps to mitigate this issue:

  1. Restore the default ``MinimumBlockPeriod`` attribute value by adding the following snippet in :file:`zigbee_fota.c` file to the :c:func:`zigbee_fota_abort` function and to the :file:`zigbee_fota_zcl_cb` function in the case where the ``ZB_ZCL_OTA_UPGRADE_STATUS_FINISH`` status is handled:

     .. code-block:: c

        /* Variable that store new value for MinimumBlockPeriod attribute. */
        zb_uint16_t minimum_block_period_new_value = NEW_VALUE;
        /* Set attribute value. */
        zb_uint8_t status = zb_zcl_set_attr_val(
                CONFIG_ZIGBEE_FOTA_ENDPOINT,
                ZB_ZCL_CLUSTER_ID_OTA_UPGRADE,
                ZB_ZCL_CLUSTER_CLIENT_ROLE,
                ZB_ZCL_ATTR_OTA_UPGRADE_MIN_BLOCK_REQUE_ID,
                (zb_uint8_t*)&minimum_block_period_new_value,
                ZB_FALSE);
        /* Check if new value was set correctly. */
        if (status != ZB_ZCL_STATUS_SUCCESS) {
                LOG_ERR("Failed to update Minimum Block Period attribute");
        }

  #. In :file:`zboss/src/zcl/zcl_ota_upgrade_commands.c` file in the :file:`nrfxlib` directory, change the penultimate argument of the 360
  :c:macro:`ZB_ZCL_OTA_UPGRADE_SEND_IMAGE_BLOCK_REQ` macro to ``delay`` in :c:func:`zb_zcl_ota_upgrade_send_block_requset` and :c:func:`resend_buffer` functions.

.. rst-class:: v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0

KRKNWK-11602: Zigbee device becomes not operable after receiving malformed packet
  When any Zigbee device receives a malformed packet that does not match the Zigbee packet structure, the ZBOSS stack asserts.
  In the |NCS| versions before the v1.9.0 release, the device is not automatically restarted.

  **Workaround:** Depends on your version of the |NCS|:

  * Before the |NCS| v1.9.0: Power-cycle the Zigbee device.
  * After and including the |NCS| v1.9.0: Wait for the device to restart automatically.

Given these two options, we recommend to upgrade your |NCS| version to the latest available one.

.. rst-class:: v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0

KRKNWK-7723: OTA upgrade process restarting after client reset
  After the reset of OTA Upgrade Client, the client will start the OTA upgrade process from the beginning instead of continuing the previous process.

.. rst-class:: v1-6-1 v1-6-0

KRKNWK-8211: Leave signal generated twice
  The ``ZB_ZDO_SIGNAL_LEAVE`` signal is generated twice during Zigbee Coordinator factory reset.

.. rst-class:: v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0

KRKNWK-9714: Device association fails if the Request Key packet is retransmitted
  If the Request Key packet for the TCLK is retransmitted and the coordinator sends two new keys that are different, a joiner logic error happens that leads to unsuccessful key verification.

.. rst-class:: v1-6-1 v1-6-0

KRKNWK-9743 Timer cannot be stopped in Zigbee routers and coordinators
  The call to the ``zb_timer_enable_stop()`` API has no effect on the timer logic in Zigbee routers and coordinators.

.. rst-class:: v1-6-1 v1-6-0

KRKNWK-10490: Deadlock in the NCP frame fragmentation logic
  If the last piece of a fragmented NCP command is not delivered, the receiving side becomes unresponsive to further commands.

.. rst-class:: v1-5-2 v1-5-1

KRKNWK-8478: NCP host application crash on exceeding :c:macro:`TX_BUFFERS_POOL_SIZE`
  If the NCP host application exceeds the :c:macro:`TX_BUFFERS_POOL_SIZE` pending requests, the application will crash on an assertion.

   **Workaround:** Increase the value of :c:macro:`TX_BUFFERS_POOL_SIZE` or define shorter polling interval (:c:macro:`NCP_TRANSPORT_REFRESH_TIME`).

.. rst-class:: v1-5-2 v1-5-1

KRKNWK-8200: Sleepy End Device halts during the commissioning
  If the turbo poll is disabled in the ``ZB_BDB_SIGNAL_DEVICE_FIRST_START`` signal, SED halts during the commissioning.

  **Workaround:** Use the development libraries link or use ``ZB_BDB_SIGNAL_STEERING`` signal with successful status to disable turbo poll.
  See the following snippet for an example:

  .. code-block:: c

     /* Workaround for KRKNWK-8200 (turbo poll) */
     switch(sig)
     {
     case ZB_BDB_SIGNAL_DEVICE_REBOOT:
     case ZB_BDB_SIGNAL_STEERING:
             if (status == RET_OK) {
                     zb_zdo_pim_permit_turbo_poll(0);
                     zb_zdo_pim_set_long_poll_interval(2000);
             }
             break;
     }

.. rst-class:: v1-5-2 v1-5-1

KRKNWK-8200: Successful signal on commissioning fail
  A successful steering signal is generated if the commissioning fails during TCLK exchange.

  **Workaround:** Use the development libraries link or check for Extended PAN ID in the steering signal handler.
  If it is equal to zero, handle the signal as if it had unsuccessful status.
  See the following snippet for an example:

  .. code-block:: c

     /* Workaround for KRKNWK-8200 (signal status) */
     switch(sig)
     {
     case ZB_BDB_SIGNAL_STEERING:
             if (status == RET_OK) {
                     zb_ext_pan_id_t extended_pan_id;
                     zb_get_extended_pan_id(extended_pan_id);
                     if (!(ZB_IEEE_ADDR_IS_VALID(extended_pan_id))) {
                            zb_buf_set_status(bufid, -1);
                            status = -1;
                     }
             }
             break;
     }

.. rst-class:: v1-5-2 v1-5-1

KRKNWK-9461 / KRKNWK-9596: Multiprotocol sample crashes with some smartphones
  With some smartphones, the multiprotocol sample crashes on the nRF5340 due to timer timeout inside the 802.15.4 radio driver logic.

.. rst-class:: v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1

KRKNWK-6348: ZCL Occupancy Sensing cluster is not complete
  The ZBOSS stack provides only definitions of constants and an abstract cluster definition (sensing cluster without sensors).

  **Workaround:** To use the sensing cluster with physical sensor, copy the implementation and extend it with the selected sensor logic and properties.
  For more information, see the `declaring custom cluster`_ guide.

.. rst-class:: v1-5-2 v1-5-1

KRKNWK-6336: OTA transfer might be aborted after the MAC-level packet retransmission
  If the device receives the APS ACK for a packet that was not successfully acknowledged on the MAC level, the OTA client cluster implementation stops the image transfer.

  **Workaround:** Add a watchdog timer that will restart the OTA image transfer.

.. rst-class:: v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0

KRKNWK-7831: Factory reset broken on coordinator with Zigbee shell
  A coordinator with the :ref:`lib_zigbee_shell` component enabled could assert after executing the ``bdb factory_reset`` command.

  **Workaround:** Call the ``bdb_reset_via_local_action`` function twice to remove all the network information.

.. rst-class:: v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0

KRKNWK-6318: Device assert after multiple Leave requests
  If a device that rejoins the network receives Leave requests several times in a row, the device could assert.

.. rst-class:: v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0

KRKNWK-6071: ZBOSS alarms inaccurate
  On average, ZBOSS alarms last longer by 6.4 percent than Zephyr alarms.

  **Workaround:** Use Zephyr alarms.

.. rst-class:: v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0

KRKNWK-5535: Device assert if flooded with multiple Network Address requests
  The device could assert if it receives Network Address requests every 0.2 second or more frequently.

.. rst-class:: v1-5-0

KRKNWK-9119: Zigbee shell does not work with ZBOSS development libraries
    Because of changes to the ZBOSS API, the :ref:`lib_zigbee_shell` library cannot be enabled when :ref:`zigbee_samples` are built with the :ref:`nrfxlib:zboss` development libraries.

    **Workaround:** Use only the production version of :ref:`nrfxlib:zboss` when using :ref:`lib_zigbee_shell`.

.. rst-class:: v1-5-0

KRKNWK-9145: Corrupted payload in commands of the Scenes cluster
  When receiving Scenes cluster commands, the payload is corrupted when using the :ref:`nrfxlib:zboss` production libraries.

  **Workaround:** Use the development version of :ref:`nrfxlib:zboss`.

.. rst-class:: v1-4-2 v1-4-1 v1-4-0

KRKNWK-7836: Coordinator asserting when flooded with ZDO commands
  Executing a high number of ZDO commands can cause assert on the coordinator with the :ref:`lib_zigbee_shell` component enabled.

.. rst-class:: v1-3-1 v1-3-0

KRKNWK-6073: Potential delay during FOTA
  There might be a noticeable delay (~220 ms) between calling the ZBOSS API and on-the-air activity.

Matter
======

.. rst-class:: v2-3-0 v2-2-0

KRKNWK-16575: Applications with factory data support do not boot up properly on nRF5340
  When the Matter sample is built for ``nrf5340dk_nrf5340_cpuapp`` build target with the :kconfig:option:`CONFIG_CHIP_FACTORY_DATA` Kconfig option set to ``y`` the application returns prematurely the error code 200016 because the factory data partition is not aligned with the :kconfig:option:`CONFIG_FPROTECT_BLOCK_SIZE` Kconfig option.

  **Workaround:** Manually cherry-pick and apply commit from the main branch (commit hash: ``ec9ad82637b0383ebf91eb1155813450ad9fcffb``).

.. rst-class:: v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1

KRKNWK-15846: Android CHIP Tool crashes when subscribing in the :guilabel:`LIGHT ON/OFF & LEVEL CLUSTER`
  The Android CHIP Tool crashes when attempting to start the subscription after typing minimum and maximum subscription interval values.
  Also, the Subscription window in the :guilabel:`LIGHT ON/OFF & LEVEL CLUSTER` contains faulty GUI layout (overlapping captions) used when passing minimum and maximum subscription interval values.
  This affects the Android CHIP Tool revision used for the |NCS| v2.2.0, v2.1.1, and v2.1.2 releases.

  .. note::
      The support for the Android CHIP Tool is removed as of the |NCS| v2.3.0 for Matter in the |NCS|. Use CHIP Tool for Linux or macOS instead, as described in :ref:`ug_matter_gs_testing`.

.. rst-class:: v2-2-0 v2-1-2 v2-1-1

KRKNWK-15913: Factory data set parsing issues
  The ``user`` field in the factory data set is not properly parsed. The field should be of the ``MAP`` type instead of the ``BSTR`` type.

  **Workaround:** Manually cherry-pick and apply commit with fix to ``sdk-connectedhomeip`` (commit hash: ``3875c6f78c77212a3f62a5c825ff9b4e5054bbb4``).

.. rst-class:: v2-1-2 v2-1-1

KRKNWK-15749: Invalid ZAP Tool revision used
  The ZAP Tool revision used for the |NCS| v2.1.1 and v2.1.2 releases is not compatible with the :file:`zap` files that define the Data Model in |NCS| Matter samples.
  This results in the ZAP Tool not being able to parse :file:`zap` files from Matter samples.

  **Workaround:** Check out the proper ZAP Tool revision with the following commands, where *<NCS_root_directory>* is the path to your |NCS| installation:

    .. code-block::

       cd <NCS_root_directory>/modules/lib/matter/
       git -C third_party/zap/repo/ checkout -f 2ae226
       git add third_party/zap/repo/

.. rst-class:: v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0

KRKNWK-14473: Unreliable communication with the window covering sample
  The :ref:`window covering sample <matter_window_covering_sample>` might rarely become unresponsive for a couple of seconds after commissioning to the Matter network.

  **Workaround:** Switch from SSED to SED role.

.. rst-class:: v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0

KRKNWK-15088: Android CHIP Tool shuts down on changing the sensor type
  When you change the current sensor type after activating the monitoring of another sensor type, the application shuts down.

  **Workaround:** Restart the application and select the desired sensor type again.

  .. note::
      The support for the Android CHIP Tool is removed as of the |NCS| v2.3.0 for Matter in the |NCS|. Use CHIP Tool for Linux or macOS instead, as described in :ref:`ug_matter_gs_testing`.

.. rst-class:: v2-0-2

KRKNWK-14748: Matter command times out when a Matter device becomes a Thread router
  When a Full Thread Device becomes a router, it will ignore incoming packets for a short period of time, typically between 1-2 seconds.
  This might disrupt the communication over Matter and lead to transaction timeouts.

  In more recent versions of Matter, this problem has been eliminated by enhancing Matter's Message Reliability Protocol.
  This fix will be included in the future versions of the |NCS|.

.. rst-class:: v2-0-2 v2-0-1 v2-0-0

KRKNWK-14206: CHIP Tool for Android might crash when using Cluster Interactive Tool screen
  Cluster Interaction Tool screen crashes when trying to send a command that takes an optional argument.

.. rst-class:: v2-0-2 v2-0-1 v2-0-0

KRKNWK-14180: The QSPI sleep mode is not handled efficiently in Matter samples on the nRF53 SoC
  QSPI is active during every Bluetooth LE connection in the Matter samples that are programmed on the nRF53 SoC.
  This results in higher power consumption, for example during commissioning into the Matter network.

.. rst-class:: v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0

KRKNWK-11225: CHIP Tool for Android cannot communicate with a Matter device after the device reboots
  CHIP Tool for Android does not implement any mechanism to recover a secure session to a Matter device after the device has rebooted and lost the session.
  As a result, the device can no longer decrypt and process messages sent by CHIP Tool for Android as the controller keeps using stale cryptographic keys.

  **Workaround:** Do not reboot the device after commissioning it with CHIP Tool for Android.

  .. note::
      The support for the Android CHIP Tool is removed as of the |NCS| v2.3.0 for Matter in the |NCS|. Use CHIP Tool for Linux or macOS instead, as described in :ref:`ug_matter_gs_testing`.

.. rst-class:: v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0

KRKNWK-10589: CHIP Tool for Android crashes when commissioning a Matter device
  In random circumstances, CHIP Tool for Android crashes when trying to connect to a Matter device over Bluetooth® LE.

  **Workaround:** Restart the application and try to commission the Matter device again.
  If the problem persists, clear the application data and try again.

.. rst-class:: v1-9-2 v1-9-1 v1-9-0

KRKNWK-12950: CHIP Tool for Android opens the commissioning window using an incorrect PIN code
  CHIP Tool for Android uses a random code instead of a user-provided PIN code to open the commissioning window on a Matter device.

.. rst-class:: v1-6-1 v1-6-0

KRKNWK-10387: Matter service is needlessly advertised over Bluetooth® LE during DFU
  The Matter samples can be configured to include the support for Device Firmware Upgrade (DFU) over Bluetooth LE.
  When the DFU procedure is started, the Matter Bluetooth LE service is needlessly advertised, revealing the device identifiers such as Vendor and Product IDs.
  The service is meant to be advertised only during the device commissioning.

.. rst-class:: v1-5-2 v1-5-1 v1-5-0

KRKNWK-9214: Pigweed submodule might not be accessible from some regions
  The ``west update`` command might generate log notifications about the failure to access the pigweed submodule.
  As a result, the Matter samples will not build.

  **Workaround:** Execute the following commands in the root folder:

    .. code-block::

       git -C modules/lib/matter submodule set-url third_party/pigweed/repo https://github.com/google/pigweed.git
       git -C modules/lib/matter submodule sync third_party/pigweed/repo
       west update

HomeKit
=======

.. rst-class:: v2-3-0 v2-2-0

KRKNWK-16503: OTA DFU using the iOS Home app (over UARP) does not work on the nRF5340 SoC
  Application core cannot be upgraded due to a problem with uploading images for two cores.
  Uploading the network core image overrides an already uploaded application core image.

  **Workaround:** Manually cherry-pick and apply commit from the main branch (commit hash: ``09874a36edf21ced7d3c9356de07df6f0ff3d457``).

.. rst-class:: v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0

KRKNWK-13010: Dropping from Thread to Bluetooth LE takes too long
  Dropping from Thread to Bluetooth LE, after a Thread Border Router is powered off, takes much longer for FTD accessories than estimated in TCT030 test case.
  It takes between 3-4 minutes for the FTD accessory to detect that the Thread network connection is lost.
  The accessory then waits the specified 65 seconds and falls back to Bluetooth LE in case the Thread network is not available again.

  **Workaround:** Specification for TCT030 is going to be updated.

.. rst-class:: v2-0-2 v2-0-1 v2-0-0

KRKNWK-14130: Bluetooth LE TX configuration is set to ``0`` dBm by default
  The minimum Bluetooth LE TX configuration required is at least ``4`` dBm.
  For HomeKit multiprotocol samples, this should be ``8`` dBm.
  This results in a weak signal on the SoC itself.

  **Workaround:** You need to configure the appropriate dBm values for Bluetooth LE and Thread manually in the source code.

.. rst-class:: v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0

KRKNWK-14081: HomeKit SDK light bulb example does not work with MTD
  If the MTD is set to ``y`` in the light bulb sample, user is not able to communicate with the device after it has been added to the Home app using an iPhone and a HomePod Mini.

.. rst-class:: v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0

NCSDK-13947: Net core downgrade prevention does not work on nRF5340
  HAP certification requirements are not met because of this issue.

.. rst-class:: v2-0-2 v2-0-1 v2-0-0

KRKNWK-13607: Stateless switch application crashes upon factory reset
  When running Thread test suit on the stateless switch application, the CI crashes upon factory reset.

.. rst-class:: v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0

KRKNWK-13249: Unexpected assertion in HAP Bluetooth Peripheral Manager
  When Bluetooth LE layer emits callback with a connect or disconnect event, one of its parameters is an underlying Bluetooth LE connection object.
  On rare occasions, this connection object is no longer valid by the time it is processed in HomeKit, and this results in assertion.
  There is no proven workaround yet.

.. rst-class:: v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0

KRKNWK-11729: Stateless switch event characteristic value not handled according to specification in Bluetooth LE mode
  The stateless programmable switch application does not handle the value of the stateless switch event characteristic in the Bluetooth LE mode according to the specification.
  According to the specification, the accessory is expected to return null once the characteristic has been read or after 10 seconds have passed.
  In its current implementation in the |NCS|, the characteristic value does not change to null immediately after it is read, and changes to null after 5 seconds instead.

  **Workaround:** The HomeKit specification in point 11.47 is going to be updated.

.. rst-class:: v1-9-2 v1-9-1 v1-9-0

KRKNWK-13063: RTT logs do not work with the Light Bulb multiprotocol sample with DFU on nRF52840
  The Light Bulb multiprotocol sample with Nordic DFU activated in debug version for nRF52840 platform does not display RTT logs properly.

  **Workaround:** Disable RTT logs for the bootloader.

.. rst-class:: v1-9-2 v1-9-1 v1-9-0

KRKNWK-13064: Nordic DFU is not compliant with HAP certification requirements
  Some of the HAP certification requirements are not met by the Nordic DFU solution.

  **Workaround:** Cherry-pick changes from `PR #332 in sdk-homekit repo`_.

.. rst-class:: v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0

KRKNWK-12474: Multiprotocol samples on nRF52840 might not switch to Thread
  Samples might not switch to Thread mode as expected and remain in Bluetooth mode instead.
  The issue is related to older iOS versions.

  **Workaround:** Update your iPhone to iOS 15.4.

.. rst-class:: v1-9-2 v1-9-1 v1-9-0 v1-8-0

KRKNWK-13095: Change in KVS key naming scheme causes an error for updated devices
  A previous implementation allowed for empty key in domain.
  This has been restricted during refactoring.

  **Workaround:** Cherry-pick changes from `PR #329 in sdk-homekit repo`_.

.. rst-class:: v1-9-2 v1-9-1 v1-9-0

KRKNWK-13022: Activating DFU causes increased power consumption
  Currently shell is used to initiate DFU mode, which leads to increased power consumption.

.. _krknwk_10611:

.. rst-class:: v1-6-0

KRKNWK-10611: DFU fails with external flash memory
  DFU will fail when using external flash memory for update image storage.
  For this reason, DFU with external flash memory cannot be performed on HomeKit accessories.

.. rst-class:: v1-7-1 v1-7-0 v1-6-1 v1-6-0

KRKNWK-9422: On-mesh commissioning does not work
  Thread's on-mesh commissioning does not work for the HomeKit accessories.

.. rst-class:: v1-6-1 v1-6-0

Invalid NFC payload
  Invalid NFC payload occurs if the HomeKit accessory is paired.

.. rst-class:: v1-6-1

Build error when building with DEBUG_SETUP_CODE configuration
  The following build error is observed with DEBUG_SETUP_CODE - invalid file path in CMakeFile.

.. rst-class:: v1-6-1

HomeKit accessory fails to start
  Occasionally, the accessory fails to start after a factory reset attempt.

.. rst-class:: v1-8-0 v1-7-1 v1-7-0

KRKNWK-11666: CLI command ``hap services`` returns incorrect results
  Observed issues with the command include float values not printed, values not updated, and read callbacks shown as "<No read callback>" even though present.

.. rst-class:: v1-8-0 v1-7-1 v1-7-0

KRKNWK-11365: HAP tool's ``provision`` command freezes
  This issue happens on macOS when an EUI argument is not passed in attempt to read EUI from a connected board.

nRF Desktop
===========

.. note::
    nRF Desktop is also affected by the Bluetooth LE issue :ref:`NCSDK-19865 <ncsdk_19865>`.

.. rst-class:: v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0 v1-0-0

NCSDK-8304: HID configurator issues for peripherals connected over Bluetooth® LE to Linux host
  Using :ref:`nrf_desktop_config_channel_script` for peripherals connected to host directly over Bluetooth LE might result in receiving improper HID feature report ID.
  In such case, the device will provide HID input reports, but it cannot be configured with the HID configurator.

  **Workaround:** Use BlueZ in version 5.56 or higher.

.. rst-class:: v2-3-0 v2-2-0 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0

NCSDK-20366: Possible bus fault in :ref:`nrf_desktop_selector` while toggling a hardware selector
  The GPIO port index calculation in the GPIO interrupt handling function assumes that GPIO devices in Zephyr are placed in memory next to each other with order matching GPIO port indexes, which might not be true.
  Using an invalid port index in the function leads to undefined behavior.

  **Workaround:** Manually cherry-pick and apply the commit with the fix from the ``main`` branch (commit hash: ``6179413498d1ae1c9c79255aeca2739d108e482d``).

.. rst-class:: v2-2-0

NCSDK-19970: MCUboot bootloader fails to swap images on nRF52840 DK that uses external flash
   The MCUboot bootloader cannot access external flash because the chosen ``nordic,pm-ext-flash`` is not defined in the MCUboot child image's devicetree.
   As a result ``nrf52840dk_nrf52840`` using ``prj_mcuboot_qspi.conf`` configuration fails to swap images after a complete image transfer.

   **Workaround**: Manually cherry-pick and apply the commit with the fix from the ``main`` branch (commit hash: ``7cea1b7e681a39ce2e2143b6b03132d95b7606ab``).
   Make sure to also cherry-pick and apply the commit that fixes a build system issue (commit `ec23df1fa305e99194ceac87a028f6da206a3ff1` from ``main`` branch).
   This is needed to ensure that the introduced DTS overlay will be applied to the MCUboot child image.

.. rst-class:: v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0

NCSDK-18552: :ref:`nrf_desktop_nrf_profiler_sync` build fails
  The build failure is caused by the invalid name of the module's source file.

  **Workaround:** Manually cherry-pick and apply the commit with the fix from the ``main`` branch (commit hash: ``28ff23ac26c079eb966893e9a64a624bf4f50b71``).

.. rst-class:: v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0

NCSDK-17088: :ref:`nrf_desktop_ble_qos` might crash on application start
  The Bluetooth LE Quality of Service (QoS) module might trigger an ARM fault on application start.
  The ARM fault is caused by invalid memory alignment.

  **Workaround:** Manually cherry-pick and apply the commit with the fix from the ``main`` branch (commit hash: ``f236a8eff32adbe201d486cd11d4fa8853b90bd7``).

.. rst-class:: v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0

NCSDK-17706: :ref:`nrf_desktop_failsafe` does not continue an interrupted settings erase operation
  The failsafe module does not continue an interrupted settings erase operation on a subsequent boot.
  Because of that, the application might be booted with only partially erased settings.

  **Workaround:** Manually cherry-pick and apply the commit with the fix from the ``main`` branch (commit hash: ``0581d50bab2ba54d78c1cb7ad37397bccf1fec5b``).

.. rst-class:: v1-9-2 v1-9-1 v1-9-0 v1-8-0

NCSDK-13858: Possible crash at the start of Bluetooth LE advertising when using SW Split Link Layer
  The nRF Desktop peripheral can crash at the start of the advertising when using SW Split Link Layer (:kconfig:option:`CONFIG_BT_LL_SW_SPLIT`).
  The crash is caused by an issue of the Bluetooth Controller.
  The size of the resolving list filter is invalid, which causes accessing memory areas that are located out of array.

  **Workaround:** Manually cherry-pick and apply commit with fix to ``sdk-zephyr`` (commit hash: ``15ebdfafe2b2932533aa8d71afd49d4b03d27ce4``).

.. rst-class:: v1-7-1 v1-7-0

NCSDK-12337: Possible assertion failure at boot of an USB-connected host
  During the booting procedure of a host device connected through USB, the HID report subscriptions might be disabled and enabled a few times without disconnecting the USB.
  This can result in improper subscription handling and assertion failure in the :ref:`nrf_desktop_hid_state`.

  **Workaround:** Manually cherry-pick and apply commit with fix from main (commit hash: ``3dbd4b47752671b61d13a4e5813163e9f8aef840``).

.. rst-class:: v1-7-1 v1-7-0

NCSDK-11626: HID keyboard LEDs are not turned off when host disconnects
  The HID keyboard LEDs, indicating among others state of Caps Lock and Num Lock, might not be updated after host disconnection.
  The problem replicates only if there is no other connected host.

  **Workaround:** Do not use HID keyboard LEDs.

.. rst-class:: v1-7-1 v1-7-0

NCSDK-11378: Empty HID boot report forwarding issue
  An empty HID boot report is not forwarded to the host computer by the nRF Desktop dongle upon peripheral disconnection.
  The host computer might not receive information that key that was previously reported as pressed was released.

  **Workaround:** Do not enable HID boot protocol on the nRF Desktop dongle.

.. rst-class:: v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0 v1-0-0

NCSDK-10907: Potential race condition related to HID input reports
  After the protocol mode changes, the :ref:`nrf_desktop_usb_state` and the :ref:`nrf_desktop_hids` modules might forward HID input reports related to the previously used protocol.

.. rst-class:: v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0

DESK-978: Directed advertising issues with SoftDevice Link Layer
  Directed advertising (``CONFIG_DESKTOP_BLE_DIRECT_ADV``) should not be used by the :ref:`nrf_desktop` application when the :ref:`nrfxlib:softdevice_controller` is in use, because that leads to reconnection problems.
  For more detailed information, see the ``Known issues and limitations`` section of the SoftDevice Controller's :ref:`nrfxlib:softdevice_controller_changelog`.

  .. note::
     The Kconfig option name changed from ``CONFIG_DESKTOP_BLE_DIRECT_ADV`` to :kconfig:option:`CONFIG_CAF_BLE_ADV_DIRECT_ADV` beginning with the |NCS| v1.5.99.

  **Workaround:** Directed advertising is disabled by default for nRF Desktop.

.. rst-class:: v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0

NCSDK-12020: Current consumption for Gaming Mouse increased by 1400mA
  When not in the sleep mode, the Gaming Mouse reference design has current consumption higher by 1400mA.

  **Workaround:** Change ``pwm_pin_set_cycles`` to ``pwm_pin_set_usec`` in function :c:func:`led_pwm_set_brightness` in Zephyr's driver :file:`led_pwm.c` file.

.. rst-class:: v1-9-2 v1-9-1 v1-9-0

NCSDK-14117: Build fails for nRF52840DK in the ``prj_b0_wwcb`` configuration
  The build failure is caused by outdated Kconfig options in the nRF52840 DK's ``prj_b0_wwcb`` configuration.
  The nRF52840 DK's ``prj_b0_wwcb`` configuration does not explicitly define static partition map either.

  **Workaround:** Manually cherry-pick and apply commit with fix from ``main`` (commit hash: ``cf4c465aceeb00d83a4f50edf67ce8c26427ac52``).

.. _known_issues_nrf5340audio:

nRF5340 Audio
=============

.. rst-class:: v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0

OCT-2501: Charging over seven hours results in error
  Since the nRF5340 Audio DK uses a large battery, the nPM1100 can go into error when charging time surpasses 7 hours.
  This is because of a protection timeout on the PMIC.
  The charging is stopped when this error occurs.

  **Workaround:** To start the charging again, turn the nRF5340 Audio DK off and then on again.

.. rst-class:: v2-3-0

OCT-2472: Headset fault upon gateway reset in the bidirectional stream mode
  The headset may react with a usage fault when the :kconfig:option:`CONFIG_STREAM_BIDIRECTIONAL` application Kconfig option is set to ``y`` and the gateway is reset during a stream.
  This issue is under investigation.

.. rst-class:: v2-3-0

OCT-2470: Discovery of Media Control Service fails
  When restarting or resetting the gateway, one or more headsets may run into a condition where the discovery of the Media Control Service fails.

.. rst-class:: v2-3-0 v2-2-0

OCT-2070: Detection issues with USB-C to USB-C connection
  Using USB-C to USB-C when connecting an nRF5340 Audio DK to PC is not correctly detected on some Windows systems.

.. rst-class:: v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0

OCT-2154: USB audio interface does not work correctly on macOS
  The audio stream is intermittent on the headset side after connecting the gateway to a Mac computer and starting audio stream.
  This issue occurs sporadically after building the nRF5340 Audio application with the default USB port as the audio source.

.. rst-class:: v2-2-0

OCT-2347: Stream reestablishment issues in CIS
  In the CIS mode, if a stream is running and the headset is reset, the gateway cannot reestablish the stream properly.

Controller subsystem for nRF5340 Audio
--------------------------------------

The following known issues apply to the LE Audio subsystem (NET core controller) for nRF5340 used in the nRF5340 Audio application.

.. rst-class:: v2-3-0 v2-2-0

OCX-138: Some conformance tests not passing
   Not all Bluetooth conformance tests cases pass.

.. rst-class:: v2-3-0 v2-2-0

OCX-152: OCX-146: 40 ms ACL interval may cause TWS to be unstable
  There may be combinations of ACL intervals and other controller settings that cause instabilities to connected or true wireless stereo setups.

  **Workaround:** Use an alternative ACL connection interval.

.. rst-class:: v2-3-0 v2-2-0

OCX-155: Larger timestamps than intended
   The timestamps/Service Data Unit references (SDU refs), may occasionally be larger than intended and then duplicated in the next interval.

.. rst-class:: v2-3-0 v2-2-0

OCX-156: PTO is not supported
   The controller does not support Pre-Transmission Offset.

.. rst-class:: v2-3-0 v2-2-0

OCX-157: OCT-140: Interleaved broadcasts streaming issues
  Interleaved broadcasts are unable to stream with certain Quality of Service (QoS) configurations.

  **Workaround:** Set retransmits (RTN) to ``<= 2`` and octets per frame to ``<= 80`` for stereo.

.. rst-class:: v2-3-0 v2-2-0

OCX-165: Collisions of BIS and ACL on the same broadcaster/central
   Broadcast Isochronous Stream (BIS) and Asynchronous Connection-oriented Logical transports (ACL) may collide in the scheduler.

   **Workaround:** Create the ACLs before creating any BISes.

.. rst-class:: v2-3-0 v2-2-0

OCX-168: Issues with reestablishing streams
   Syncing of broadcast receivers takes longer than in version 3310, especially for high retransmit (RTN) values.

   **Workaround:** Set retransmits (RTN) to a lower value to reduce the resynchronization time.


nRF Machine Learning
====================

.. rst-class:: v2-2-0

NCSDK-18532: MCUboot bootloader does not swap images after OTA DFU on nRF5340 DK and Thingy:53
   The MCUboot bootloader cannot access external flash because the chosen ``nordic,pm-ext-flash`` is not defined in the MCUboot child image's devicetree.
   As a result, MCUboot cannot swap images that are received during the OTA update.

   **Workaround**: Manually cherry-pick and apply the commit with the fix from the ``main`` branch (commit hash: ``f54f6bbd423b12a595e76425e688f034926b8018``) to fix the issue for ``nrf5340dk_nrf5340_cpuapp``.
   Similar fix needs to be applied for the Thingy:53 board.
   Make sure to also cherry-pick and apply the commit that fixes a build system issue (commit `ec23df1fa305e99194ceac87a028f6da206a3ff1` from ``main`` branch).
   This is needed to ensure that the introduced DTS overlay will be applied to the MCUboot child image.

.. rst-class:: v1-9-0

NCSDK-13923: Device might crash during Bluetooth bonding
  The device programmed with the nRF Machine Learning application might crash during Bluetooth bonding because of insufficient Bluetooth RX thread stack size.

  **Workaround:** Manually cherry-pick and apply the commit with the fix from the ``main`` branch (commit hash: ``4870fcd8316bd3a4b53ca0054f0ce35e1a8c567d``).

.. rst-class:: v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0

NCSDK-16644: :ref:`nrf_machine_learning_app` does not go to sleep and does not wake up on Thingy:53
  nRF Machine learning application on Thingy:53 does not sleep after a period of inactivity and does not wake up after an activity occurs.

  **Workaround:** Manually cherry-pick and apply two commits with the fix from the ``main`` branch (commit hashes: ``7381bfcb9c23cd6f78e6ef7fd3ff82b700f81b0f``, ``7e8c23a6632632f0cee885abe955e37a6942911d``).

Pelion
======

.. rst-class:: v1-6-1 v1-6-0

NCSDK-10196: DFU fails for some configurations with the quick session resume feature enabled
  Enabling :kconfig:option:`CONFIG_PELION_QUICK_SESSION_RESUME` together with the OpenThread network backend leads to the quick session resume failure during the DFU packet exchange.
  This is valid for the :ref:`nRF52840 DK <ug_nrf52>` and the :ref:`nRF5340 DK <ug_nrf5340>`.

  **Workaround:** Use the quick session resume feature only for configurations with the cellular network backend.

Common Application Framework (CAF)
==================================

.. rst-class:: v1-8-0

NCSDK-13247: Sensor manager dereferences NULL pointer on wake up for sensors without trigger
  :ref:`caf_sensor_manager` dereferences NULL pointer while handling a :c:struct:`wake_up_event` if a configured sensor does not use trigger.
  This leads to undefined behavior.

  **Workaround:** Manually cherry-pick and apply commit with fix from main (commit hash: ``3db6da76206d379c223afe2de646218e60e4f339``).

.. rst-class:: v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0

NCSDK-13058: Directed advertising does not work
  The directed advertising feature enabled with the :kconfig:option:`CONFIG_CAF_BLE_ADV_DIRECT_ADV` option does not work as intended.
  Using directed advertising towards peers that enable privacy might result in connection establishing problems.

  **Workaround:** Manually cherry-pick and apply commit with fix from main (commit hash: ``c61c677872943bcf7905ddeec8b24b07ae50752e``).

.. rst-class:: v2-2-0

NCSDK-18587: :ref:`caf_ble_adv` leaves off state on peer disconnection
  If Bluetooth peer disconnects while the module is in the off state, the Bluetooth LE advertising module enters the ready state.
  The module must remain in the off state until :c:struct:`wake_up_event` is received.

  **Workaround:** Manually cherry-pick and apply commit with fix from main (commit hash: ``d5f1390f724b08ebe6bc72d0ff7ba2296a6f4acd``).

.. rst-class:: v2-0-2 v2-0-1 v2-0-0

NCSDK-15675: Possible advertising start failure and module state error in :ref:`caf_ble_adv`
  If a new peer is selected twice in a quick succession, the second peer selection might cause an advertising start failure and a module state error reported by the :ref:`caf_ble_adv`.
  See the commit with fix mentioned in the workaround for details.

  **Workaround:** Manually cherry-pick and apply commit with fix from main (commit hash: ``934a25ac23125758e350b64bca23885486682109``).

.. rst-class:: v2-0-2 v2-0-1 v2-0-0

NCSDK-15707: Visual glitches when updating an RGB LED's color in :ref:`caf_leds`
  Due to changes in the default DTS of the boards, the default PWM period has been increased.
  The first LED channel is updated one PWM period before other channels.
  This causes visual glitches for LEDs with more than one color channel when the LED color is being updated.
  A shorter LED PWM period mitigates the observed issue.
  See :ref:`caf_leds` for more information.

  **Workaround:** Make sure your application includes the devicetree overlay file in which PWM period is decreased.
  For example, include the following commit to solve the issue for the :ref:`nrf_machine_learning_app` application for Nordic Thingy:53: ``fa2b57cddbaacf393c77def5d0302e1a45138d21``.

.. rst-class:: v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0

NCSDK-16644: :ref:`caf_sensor_manager` macro incorrectly converts float to sensor value
  :ref:`caf_sensor_manager` macro :c:macro:`FLOAT_TO_SENSOR_VALUE` might convert float to sensor value incorrectly, because of missing brackets around macro argument.

  **Workaround:** Manually cherry-pick and apply the commit with the fix from the ``main`` branch (commit hash: ``7e8c23a6632632f0cee885abe955e37a6942911d``).

.. _ncsidb_925:

.. rst-class:: v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0

NCSIDB-925: Event subscribers in the :ref:`app_event_manager` may overlap when using a non-default naming convention
  In order to locate the event subscribers start and stop address ranges, sections have to be sorted by the name with added postfixes.
  Hence, using a non-default event naming scheme may break the expected subscribers sorting.
  In this situation, one of the event subscribers arrays might be placed inside the other.
  So, when the event related to the outer subscribers is processed, the event subscribers that are inside are also executed.
  To resolve this issue, a new implementation was introduced that uses a section naming that cannot be used as event name (invalid variable identifier).

  **Workaround:** Use the default event names, ensuring that each event name ends with the ``_event`` postfix.
  Make sure that the event name does not start the same way as another event.
  For example, creating the following events: ``rx_event`` and ``rx_event_error_event`` would still cause the issue.

Subsystems
**********

Bluetooth® LE
=============

.. rst-class:: v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0 v1-0-0 v0-4-0 v0-3-0

NCSDK-19942: HID samples do not work with Android 13
  The :ref:`peripheral_hids_keyboard` and :ref:`peripheral_hids_mouse` samples enter a connection-disconnection loop when you try to connect to them from a device that is running Android 13.

.. _ncsdk_19865:

.. rst-class:: v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0 v1-0-0 v0-4-0 v0-3-0

NCSDK-19865: GATT Robust Caching issues for bonded peers
  The Client Supported Features value written by a bonded peer may not be stored in the non-volatile memory.
  Change awareness of the bonded peer is lost on reboot.
  After reboot, each bonded peer is initially marked as change-unaware.

  **Workaround:** Disable the GATT Caching feature (:kconfig:option:`CONFIG_BT_GATT_CACHING`).
  Make sure that Bluetooth bonds are removed together with disabling GATT Caching if the functionality is disabled during a firmware upgrade.

.. rst-class:: v2-3-0 v2-2-0

NCSDK-18518: Cannot build peripheral UART and peripheral LBS samples for the nRF52810 and nRF52811 devices with the Zephyr Bluetooth LE Controller
  The :ref:`peripheral_uart` and :ref:`peripheral_lbs` samples fail to build for the nRF52810 and nRF52811 devices when the :kconfig:option:`CONFIG_BT_LL_CHOICE` Kconfig option is set to :kconfig:option:`CONFIG_BT_LL_SW_SPLIT`.

  **Workaround:** Use the SoftDevice Controller: set the :kconfig:option:`CONFIG_BT_LL_CHOICE` Kconfig option to :kconfig:option:`CONFIG_BT_LL_SOFTDEVICE`.

.. rst-class:: v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0

NCSDK-19727: Cannot build the :ref:`peripheral_hids_mouse` sample with security disabled.
  Build fails when the :kconfig:option:`CONFIG_BT_HIDS_SECURITY_ENABLED` Kconfig option is disabled.

  **Workaround:** Build the sample with its default configuration, that is, enable the :kconfig:option:`CONFIG_BT_HIDS_SECURITY_ENABLED` Kconfig option.

.. rst-class:: v2-0-2

DRGN-17695: The BT RX thread stack might overflow if the :kconfig:option:`CONFIG_BT_SMP` is enabled
  When performing SMP pairing MPU FAULTs might be triggered because the stack is not large enough.

  **Workaround:** Increase the stack size manually in the project configuration file (:file:`prj.conf`) using :kconfig:option:`CONFIG_BT_RX_STACK_SIZE`.

.. rst-class:: v2-0-2 v2-0-1 v2-0-0

NCSDK-15527: Advertising in the :ref:`peripheral_hr_coded` sample and scanning in the :ref:`bluetooth_central_hr_coded` sample cannot be started when using the SW Split Link Layer
  The :kconfig:option:`CONFIG_BT_CTLR_ADV_EXT` option required by these samples is disabled by default in the SW Split Link Layer.

  **Workaround:** Enable the :kconfig:option:`CONFIG_BT_CTLR_ADV_EXT` option in the project configuration file (:file:`prj.conf`).

.. rst-class:: v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0

NCSDK-15229: Incorrect peer's throughput calculation in the :ref:`ble_throughput` sample
  The peer's measured throughput is understated because it includes a delay, during which there is no data transfer.

  **Workaround:** Manually cherry-pick and apply commit with fix from main (commit hash: ``05871f9b9c2aebf0a3c188a61b3788baea783180``).

.. rst-class:: v2-0-2 v2-0-1 v2-0-0

NCSDK-16060: :ref:`peripheral_lbs` sample build fails when the :kconfig:option:`CONFIG_BT_LBS_SECURITY_ENABLED` option is disabled
  Build failure is caused by the undefined ``conn_auth_info_callbacks`` structure.

  **Workaround:** Manually cherry-pick and apply commit with fix from ``main`` (commit hash: ``32c827b20f3c5ab85a359e572d366da310fe2767``).

.. rst-class:: v2-0-2 v2-0-1 v2-0-0

NCSDK-15724: Bluetooth's Peripheral UART sample fails to start on Thingy:53
  Enabling USB by the :ref:`Peripheral UART's <peripheral_uart>` main function ends with error because the USB was already enabled by the Thingy:53-specific code.

  **Workaround:** Manually cherry-pick and apply commit with fix from ``main`` (commit hash: ``b834ff8860f3a30fe19c99dbf4c0c99b0b017245``).

.. rst-class:: v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0 v1-0-0

NCSDK-13459: Uninitialized size in hids_boot_kb_outp_report_read
  When reading from the boot keyboard output report characteristic, the :ref:`hids_readme` calls the registered callback with uninitialized report size.

  **Workaround:** Manually cherry-pick and apply commit with fix from main (commit hash: ``f18250dad6cbd9778de7af4b8a774b58e55fe720``).

.. rst-class:: v1-8-0

NCSDK-12886: Peripheral UART sample building issue with nRF52811
  The :ref:`peripheral_uart` sample built for nRF52811 asserts on the nRF52840 DK in rev. 2.1.0 (build target: ``nrf52840dk_nrf52811``).

.. rst-class:: v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0 v1-0-0

NCSDK-9820: Notification mismatch in :ref:`peripheral_lbs`
  When testing the :ref:`peripheral_lbs` sample, if you press and release **Button 1** while holding one of the other buttons, the notification for button release is the same as for the button press.

.. rst-class:: v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0 v1-0-0

NCSDK-9106: Bluetooth® ECC thread stack size too small
  The Bluetooth ECC thread used during the pairing procedure with LE Secure Connections might overflow when an interrupt is triggered when the stack usage is at its maximum.

  **Workaround:** Increase the ECC stack size by setting ``CONFIG_BT_HCI_ECC_STACK_SIZE`` to ``1140``.

.. rst-class:: v1-5-0 v1-4-2 v1-4-1 v1-4-0

DRGN-15435: GATT notifications and Writes Without Response might be sent out of order
  GATT notifications and Writes Without Response might be sent out of order when not using a complete callback.

  **Workaround:** Always set a callback for notifications and Writes Without Response.

.. rst-class:: v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0 v1-0-0

DRGN-15448: Incomplete bond overwrite during pairing procedure when peer is not using the IRK stored in the bond
  When pairing with a peer that has deleted its bond information and is using a new IRK to establish the connection, the existing bond is not overwritten during the pairing procedure.
  This can lead to MIC errors during reconnection if the old LTK is used instead.

.. rst-class:: v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0 v1-0-0

NCSDK-8321: NUS shell transport sample does not display the initial shell prompt
  NUS shell transport sample does not display the initial shell prompt ``uart:~$`` on the remote terminal.
  Also, few logs with sending errors are displayed on the terminal connected directly to the DK.
  This issue is caused by the shell being enabled before turning on the notifications for the NUS service by the remote peer.

  **Workaround:** Enable the shell after turning on the NUS notifications or block it until turning on the notifications.

.. rst-class:: v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0 v1-0-0

NCSDK-8224: Callbacks for "security changed" and "pairing failed" are not always called
  The pairing failed and security changed callbacks are not called when the connection is disconnected during the pairing procedure or the required security is not met.

  **Workaround:** Application should use the disconnected callback to handle pairing failed.

.. rst-class:: v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0 v1-0-0

NCSDK-8223: GATT requests might deadlock RX thread
  GATT requests might deadlock the RX thread when all TX buffers are taken by GATT requests and the RX thread tries to allocate a TX buffer for a response.
  This causes a deadlock because only the RX thread releases the TX buffers for the GATT requests.
  The deadlock is resolved by a 30 second timeout, but the ATT bearer cannot transmit without reconnecting.

  **Workaround:** Set :kconfig:option:`CONFIG_BT_L2CAP_TX_BUF_COUNT` >= ``CONFIG_BT_ATT_TX_MAX`` + 2.

.. rst-class:: v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0 v1-0-0

NCSDK-6845: Pairing failure with simultaneous pairing on multiple connections
  When using LE Secure Connections pairing, the pairing fails with simultaneous pairing on multiple connections.
  The failure reason is unspecified.

  **Workaround:** Retry the pairing on the connections that failed one by one after the pairing procedure has finished.

.. rst-class:: v1-4-0 v1-3-2 v1-3-1 v1-3-0

NCSDK-6844: Security procedure failure can terminate GATT client request
  A security procedure terminates the GATT client request that is currently in progress, unless the request was the reason to initiate the security procedure.
  If a new GATT client request is queued at this time, this might potentially cause a GATT transaction violation and fail as well.

  **Workaround:** Do not initiate a security procedure in parallel with GATT client requests.

.. rst-class:: v1-3-0

NCSDK-5711: High-throughput transmission can deadlock the receive thread
  High-throughput transmission can deadlock the receive thread if the connection is suddenly disconnected.

.. rst-class:: v1-2-1 v1-2-0

Only secure applications can use Bluetooth® LE
  Bluetooth LE cannot be used in a non-secure application, for example, an application built for the ``nrf5340_dk_nrf5340_cpuappns`` build target.

  **Workaround:** Use the ``nrf5340_dk_nrf5340_cpuapp`` build target instead.

.. rst-class:: v1-2-1 v1-2-0

Peripheral HIDS keyboard sample cannot be used with nRF Bluetooth® LE Controller
  The :ref:`peripheral_hids_keyboard` sample cannot be used with the :ref:`nrfxlib:softdevice_controller` because the NFC subsystem does not work with the controller library.
  The library uses the MPSL Clock driver, which does not provide an API for asynchronous clock operation.
  NFC requires this API to work correctly.

.. rst-class:: v1-2-1 v1-2-0

Peripheral HIDS mouse sample advertising issues
  When the :ref:`peripheral_hids_mouse` sample is used with the Zephyr Bluetooth® LE Controller, directed advertising does not time out and the regular advertising cannot be started.

.. rst-class:: v1-2-1 v1-2-0

Central HIDS sample issues with directed advertising
  The :ref:`bluetooth_central_hids` sample cannot connect to a peripheral that uses directed advertising.

.. rst-class:: v1-1-0

Unstable samples
  Bluetooth® Low Energy peripheral samples are unstable in some conditions (when pairing and bonding are performed and then disconnections/re-connections happen).

.. rst-class:: v1-2-1 v1-2-0 v1-1-0

:kconfig:option:`CONFIG_BT_SMP` alignment requirement
  When running the :ref:`bluetooth_central_dfu_smp` sample, the :kconfig:option:`CONFIG_BT_SMP` configuration must be aligned between this sample and the Zephyr counterpart (:ref:`zephyr:smp_svr_sample`).
  However, security is not enabled by default in the Zephyr sample.

.. rst-class:: v1-2-1 v1-2-0 v1-1-0 v1-0-0

Reconnection issues on some operating systems
  On some operating systems, the :ref:`nrf_desktop` application is unable to reconnect to a host.

.. rst-class:: v1-1-0 v1-0-0

:ref:`central_uart` cannot handle long strings
  A too long 212-byte string cannot be handled when entered to the console to send to :ref:`peripheral_uart`.

.. rst-class:: v1-0-0

:ref:`bluetooth_central_hids` loses UART connectivity
  After programming a HEX file to the nrf52_pca10040 board, UART connectivity is lost when using the Bluetooth® LE Controller.
  The board must be reset to get UART output.

.. rst-class:: v1-1-0 v1-0-0

Samples crashing on nRF51 when using GPIO
  On nRF51 devices, Bluetooth® LE samples that use GPIO might crash when buttons are pressed frequently.
  In such case, the GPIO ISR introduces latency that violates real-time requirements of the Radio ISR.
  nRF51 is more sensitive to this issue than nRF52 (faster core).

.. rst-class:: v0-4-0

GATT Discovery Manager missing support
  The :ref:`gatt_dm_readme` is not supported on nRF51 devices.

.. rst-class:: v0-4-0

Samples do not work with SD Controller v0.1.0
  Bluetooth® LE samples cannot be built with the :ref:`nrfxlib:softdevice_controller` v0.1.0.

.. rst-class:: v1-0-0 v0-4-0 v0-3-0

LED Button Service reporting issue
  :ref:`peripheral_lbs` does not report the Button 1 state correctly.

.. rst-class:: v1-2-1 v1-2-0 v1-1-0 v1-0-0 v0-4-0 v0-3-0

MITM protection missing for central samples
  The central samples (:ref:`central_uart`, :ref:`bluetooth_central_hids`) do not support any pairing methods with MITM protection.

.. rst-class:: v0-3-0

Peripheral UART string size issue
  :ref:`peripheral_uart` cannot handle the corner case that a user attempts to send a string of more than 211 bytes.

.. rst-class:: v0-3-0

Reconnection issues after bonding
  The peripheral samples (:ref:`peripheral_uart`, :ref:`peripheral_lbs`, :ref:`peripheral_hids_mouse`) have reconnection issues after performing bonding (LE Secure Connection pairing enable) with nRF Connect for Desktop.
  These issues result in disconnection.

.. rst-class:: v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0

NCSDK-17883: Cannot build peripheral UART sample with security (:kconfig:option:`CONFIG_BT_NUS_SECURITY_ENABLED`) disabled
  The :ref:`peripheral_uart` sample fails to build when the :kconfig:option:`BT_NUS_SECURITY_ENABLED` Kconfig option is disabled.

  **Workaround:** In :file:`main.c` file, search for the ``#else`` entry of the ``#if defined(CONFIG_BT_NUS_SECURITY_ENABLED)`` item and add an empty declaration of the ``conn_auth_info_callbacks`` structure, just after the similar empty definition of ``conn_auth_callbacks``.

Bluetooth mesh
==============

.. rst-class:: v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-1 v1-9-0 v1-8-0 v1-7-1

NCSDK-16800: RPL is not cleared on IV index recovery
  After recovering the IV index, a node doesn't clear the replay protection list, which leads to incorrect triggering of the replay attack protection mechanism.

  **Workaround:** Call ``bt_mesh_rpl_reset`` twice after the IV index recovery is done.

.. rst-class:: v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-1 v1-9-0 v1-8-0 v1-7-1

NCSDK-16798: Friend Subscription List might have duplicate entries
  If a Low Power node loses a Friend Subscription List Add Confirm message, it repeats the request.
  The Friend does not check both the transaction number and the presence of the addresses in the subscription list.
  This causes a situation where the Friend fills the subscription list with duplicate addresses.

.. rst-class:: v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0

NCSDK-16782: The extended advertiser might not work with Bluetooth mesh
  Using the extended advertiser instead of the legacy advertiser can lead to getting composition data while provisioning to fail.
  This problem might manifest in the sample :ref:`bluetooth_ble_peripheral_lbs_coex`, as it is using the extended advertiser.

.. rst-class:: v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-1 v1-9-0

NCSDK-16579: Advertising Node Identity and Network ID might not work with the extended advertiser
  Advertising Node Identity and Network ID do not work with the extended advertising API when the :kconfig:option:`CONFIG_BT_MESH_ADV_EXT_GATT_SEPARATE` option is enabled.

  **Workaround:** Don't enable the :kconfig:option:`CONFIG_BT_MESH_ADV_EXT_GATT_SEPARATE` option.

.. rst-class:: v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-1 v1-9-0 v1-8-0 v1-7-1

NCSDK-14399: Legacy advertiser can occasionally do more message retransmissions than requested
  When using the legacy advertiser, the stack sleeps for at least 50 ms after starting advertising a message, which might result in more messages to be advertised than requested.

.. rst-class:: v2-0-2 v2-0-1 v2-0-0 v1-9-1 v1-9-0 v1-8-0 v1-7-1

NCSDK-16061: IV update procedure fails on the device
  Bluetooth® mesh device does not undergo IV update and fails to participate in the procedure initiated by any other node unless it is rebooted after the provisioning.

  **Workaround:** Reboot the device after provisioning.

.. rst-class:: v1-6-1 v1-6-0

NCSDK-10200: The device stops sending Secure Network Beacons after re-provisioning
  Bluetooth® mesh stops sending Secure Network Beacons if the device is re-provisioned after reset through Config Node Reset message or ``bt_mesh_reset()`` call.

  **Workaround:** Reboot the device after re-provisioning.

.. rst-class:: v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0

NCSDK-5580: nRF5340 only supports SoftDevice Controller
  On nRF5340, only the :ref:`nrfxlib:softdevice_controller` is supported for Bluetooth® mesh.

Bluetooth direction finding
===========================

.. rst-class:: v1-9.0

Antenna switching does not work on targets ``nrf5340dk_nrf5340_cpuapp`` and ``nrf5340dk_nrf5340__cpuapp_ns``
  Antenna switching does not work when direction finding sample applications are built for ``nrf5340dk_nrf5340_cpuapp`` and ``nrf5340dk_nrf5340_cpuapp_ns`` targets. That is caused by GPIO pins that are responsible for access to antenna switches, not being assigned to network core.


Bootloader
==========

.. rst-class:: v1-5-2 v1-5-1 v1-5-0

NCSDK-7173: nRF5340 network core bootloader cannot be built stand-alone
  The :ref:`nc_bootloader` sample does not compile when built stand-alone.
  It compiles without problems when included as a child image.

  **Workaround:** Include the :ref:`nc_bootloader` sample as child image instead of compiling it stand-alone.

.. rst-class:: v1-1-0

Public keys revocation
  Public keys are not revoked when subsequent keys are used.

.. rst-class:: v1-1-0

Incompatibility with nRF51
  The bootloader does not work properly on nRF51.

.. rst-class:: v1-2-1 v1-2-0 v1-1-0 v1-0-0 v0-4-0 v0-3-0

Immutable bootloader not supported in SES
  Building and programming the immutable bootloader (see :ref:`ug_bootloader`) is not supported in SEGGER Embedded Studio.

.. rst-class:: v1-2-1 v1-2-0 v1-1-0 v1-0-0 v0-4-0 v0-3-0

Immutable bootloader board restrictions
  The immutable bootloader can only be used with the following boards:

  * ``nrf52840_pca10056``
  * ``nrf9160_pca10090``

.. rst-class:: v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0

nRF Secure Immutable Bootloader and netboot can overwrite non-OTP provisioning data
  In architectures that do not have OTP regions, b0 and b0n images incorrectly linked to the size of their container can overwrite provisioning partition data from their image sizes.
  Issue related to NCSDK-7982.

.. rst-class:: v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0

The combination of nRF Secure Immutable Bootloader and MCUboot fails to upgrade both the application and MCUboot
  Due to a change in dependency handling in MCUboot, MCUboot does not read any update as a valid update.
  Issue related to NCSDK-8681.

Build system
============

.. rst-class:: v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0

NCSIDB-816: nanopb/protoc not specified correctly by the |NCS| Toolchain
  The |NCS| Toolchain includes the nanopb/protoc tool when installed, but the path to the tool is not propagated correctly to the |NCS| build system.

  **Workaround:** Locate the nanopb :file:`generator-bin` directory in your |NCS| Toolchain and add its path to your system's global path.
  This makes the protoc tool executable findable.

.. rst-class:: v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0

NCSDK-6117: Build configuration issues
  The build configuration consisting of :ref:`bootloader`, Secure Partition Manager, and application does not work.
  (The SPM has been deprecated with the |NCS| v2.1.0.)

  **Workaround:** Either include MCUboot in the build or use MCUboot instead of the immutable bootloader.

.. rst-class:: v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0

NCSDK-10672: :file:`dfu_application.zip` is not updated during build
  In the configuration with MCUboot, the :file:`dfu_application.zip` might not be properly updated after code or configuration changes, because of missing dependencies.

  **Workaround:** Clear the build if :file:`dfu_application.zip` is going to be released to make sure that it is up to date.

.. rst-class:: v1-4-2 v1-4-1 v1-4-0

NCSDK-6898: Overriding child images
  Adding child image overlay from the :file:`CMakeLists.txt` top-level file located in the :file:`samples` directory overrides the existing child image overlay.

  **Workaround:** Apply the configuration from the overlay to the child image manually.

.. rst-class:: v1-4-2 v1-4-1 v1-4-0

NCSDK-6777: Project out of date when :kconfig:option:`CONFIG_SECURE_BOOT` is set
  The DFU :file:`.zip` file is regenerated even when no changes are made to the files it depends on.
  As a consequence, SES displays a "Project out of date" message even when the project is not out of date.

  **Workaround:** Apply the fix from `sdk-nrf PR #3241 <https://github.com/nrfconnect/sdk-nrf/pull/3241>`_.

.. rst-class:: v1-4-2 v1-4-1 v1-4-0

NCSDK-6848: MCUboot must be built from source when included
  The build will fail if either :kconfig:option:`CONFIG_MCUBOOT_BUILD_STRATEGY_SKIP_BUILD` or :kconfig:option:`CONFIG_MCUBOOT_BUILD_STRATEGY_USE_HEX_FILE` is set.

  **Workaround:** Set :kconfig:option:`CONFIG_MCUBOOT_BUILD_STRATEGY_FROM_SOURCE` instead.

.. rst-class:: v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0 v1-0-0 v0-4-0 v0-3-0

KRKNWK-7827: Application build system is not aware of the settings partition
  The application build system is not aware of partitions, including the settings partition, which can result in application code overlapping with other partitions.
  As a consequence, writing to overlapping partitions might remove or damage parts of the firmware, which can lead to errors that are difficult to debug.

  **Workaround:** Define and use a code partition to shrink the effective flash memory available for the application.
  You can use one of the following solutions:

  * :ref:`partition_manager` from |NCS| - see the page for all configuration options.
    For example, for single image (without bootloader and with the settings partition used), set the :kconfig:option:`CONFIG_PM_SINGLE_IMAGE` Kconfig option to ``y`` and define the value for :kconfig:option:`CONFIG_PM_PARTITION_SIZE_SETTINGS_STORAGE` to the required settings storage size.
  * :ref:`Devicetree code partition <zephyr:flash_map_api>` from Zephyr.
    Set :kconfig:option:`CONFIG_USE_DT_CODE_PARTITION` Kconfig option to ``y``.
    Make sure that the code partition is defined and chosen correctly (``offset`` and ``size``).

.. rst-class:: v1-3-2 v1-3-1 v1-3-0

Flash commands only program one core
  ``west flash`` and ``ninja flash`` only program one core, even if multiple cores are included in the build.

  **Workaround:** Execute the flash command from inside the build directory of the child image that is placed on the other core (for example, :file:`build/hci_rpmsg`).

.. rst-class:: v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0

NCSDK-11234: Statically defined "pcd_sram" partition might cause ARM usage fault
  Inconsistency between SRAM memory partitions in Partition Manager and DTS could lead to improper memory usage.
  For example, one SRAM region might be used for multiple purposes at the same time.

  **Workaround:** Ensure that partitions defined by DTS and Partition Manager are consistent.

.. rst-class:: v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0

NCSDK-7982: Partition manager: Incorrect partition size linkage from name conflict
  The partition manager will incorrectly link a partition's size to the size of its container if the container partition's name matches its child image's name in :file:`CMakeLists.txt`.
  This can cause the inappropriately-sized partition to overwrite another partition beyond its intended boundary.

  **Workaround:** Rename the container partitions in the :file:`pm.yml` and :file:`pm_static.yml` files to something that does not match the child images' names, and rename the child images' main image partition to its name in :file:`CMakeLists.txt`.

DFU and FOTA
============

.. rst-class:: v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0

NCSDK-18357: Serial recovery does not work on nRF5340 network core
  If a network core serial recovery is attempted using MCUboot serial recovery, the upload will complete, but the image will not be transferred to the network core and the upgrade will fail.

.. rst-class:: v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0

NCSDK-18422: Serial recovery fails to write to slots in QSPI
  If a slot resides in an external QSPI storage area and this area is written to in MCUboot's serial recovery system, the writing will not work due to memory buffer alignment offsets and requirements with the underlying flash driver.

.. rst-class:: v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0

NCSDK-18108: ``s1`` variant image configuration mismatch
  If an image with an ``s1`` variant is configured and the ``s0`` image configuration is changed using menuconfig, these changes will not be reflected in the ``s1`` configuration, which can lead to a differing build configuration or the build does not upgrade.

.. rst-class:: v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0

NCSDK-11308: Powering off device immediately after serial recovery of the nRF53 network core firmware results in broken firmware
  The network core will not be able to boot if the device is powered off too soon after completing a serial recovery update procedure of the network core firmware.
  This is because the firmware is being copied from shared RAM to network core flash **after** mcumgr indicates that the serial recovery procedure has completed.

  **Workaround:** Use one of the following workarounds:

  * Wait for 30 seconds before powering off the device after performing serial recovery of the nRF53 network core firmware.
  * Re-upload the network core firmware with a new serial recovery procedure if the device was powered off too soon in a previous procedure.

.. rst-class:: v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0

NCSDK-6238: Socket API calls might hang when using Download client
  When using the :ref:`lib_download_client` library with HTTP (without TLS), the application might not process incoming fragments fast enough, which can starve the :ref:`nrfxlib:nrf_modem` buffers and make calls to the Modem library hang.
  Samples and applications that are affected include those that use :ref:`lib_download_client` to download files through HTTP, or those that use :ref:`lib_fota_download` with modem updates enabled.

  **Workaround:** Set :kconfig:option:`CONFIG_DOWNLOAD_CLIENT_RANGE_REQUESTS`.

.. rst-class:: v1-1-0

Jobs not received after reset
  When using :ref:`lib_aws_fota`, no new jobs are received on the device if the device is reset during a firmware upgrade or loses the MQTT connection.

  **Workaround:** Delete the stalled in progress job from AWS IoT.

.. rst-class:: v1-1-0

Stalled download
  :ref:`lib_fota_download` does not resume a download if the device loses the connection.

  **Workaround:** Call :cpp:func:`fota_download_start` again with the same arguments when the connection is re-established to resume the download.

.. rst-class:: v1-1-0

Offset not retained with an MCUboot target
  When using the MCUboot target in :ref:`lib_dfu_target`, the write/downloaded offset is not retained when the device is reset.

.. rst-class:: v1-1-0

Download stopped on socket connection timeout
  In the nRF9160: AWS FOTA and :ref:`http_application_update_sample` samples, the download is stopped if the socket connection times out before the modem can delete the modem firmware.
  A fix for this issue is available in commit `38625ba7 <https://github.com/nrfconnect/sdk-nrf/commit/38625ba775adda3cdc7dbf516eeb3943c7403227>`_.

  **Workaround:** Call :cpp:func:`fota_download_start` again with the same arguments.

.. rst-class:: v1-1-0

Update event triggered by an error event
  If the last fragment of a :ref:`lib_fota_download` is received but is corrupted, or if the last write is unsuccessful, the library emits an error event as expected.
  However, it also emits an apply/request update event, even though the downloaded data is invalid.

.. rst-class:: v1-0-0 v0-4-0

FW upgrade is broken for multi-image builds
  Firmware upgrade using mcumgr or USB DFU is broken for multi-image builds, because the devicetree configuration is not used.
  Therefore, it is not possible to upload the image.

  **Workaround:** Build MCUboot and the application separately.

Enhanced ShockBurst (ESB)
=========================

.. rst-class:: v2-3-0 v2-2-0

NCSDK-20092: ESB does not send packet longer than 63 bytes
  ESB does not support sending packets longer than 63 bytes, but has no such hardware limitation.

NFC
===

.. rst-class:: v2-2-0

NCSDK-19168: The :ref:`peripheral_nfc_pairing` and :ref:`central_nfc_pairing` samples cannot pair using OOB data.
  The :ref:`nfc_ndef_ch_rec_parser_readme` library parses AC records in an invalid way.
  As a result, the samples cannot parse OOB data for pairing.

  **Workaround:** Revert the :file:`subsys/nfc/ndef/ch_record_parser.c` file to the state from the :ref:`ncs_release_notes_210`.

  .. code-block::

     cd <NCS_root_directory>
     git checkout v2.1.0 -- subsys/nfc/ndef/ch_record_parser.c

.. rst-class:: v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0

NCSDK-19347: NFC Reader samples return false errors with value ``1``.
  The :c:func:`nfc_t4t_isodep_transmit` function of the :ref:`nfc_t4t_isodep_readme` library can return ``1`` as error code even if a delayed operation has been scheduled correctly and should return ``0``.
  This happens when the ISO-DEP frame is sent for the first time.
  In samples, this error is propagated from the higher level :c:func:`nfc_t4t_hl_procedure_ndef_tag_app_select` function.
  The :ref:`tnep_poller_readme` library operations can call the application error callback with error code ``1``, meaning that a delayed operation has been scheduled successfully.

  **Workaround:** Ignore the :ref:`tnep_poller_readme` error callback with error value ``1``.
  Treat the return value ``1`` of the functions :c:func:`nfc_t4t_isodep_transmit` and :c:func:`nfc_t4t_hl_procedure_ndef_tag_app_select` as success.

.. rst-class:: v1-2-1 v1-2-0

Sample incompatibility with the nRF5340 PDK
  The :ref:`nfc_tnep_poller` and :ref:`nfc_tag_reader` samples cannot be run on the nRF5340 PDK.
  There is an incorrect number of pins defined in the MDK files, and the pins required for :ref:`st25r3911b_nfc_readme` cannot be configured properly.

.. rst-class:: v1-2-1 v1-2-0 v1-1-0

Unstable NFC tag samples
  NFC tag samples are unstable when exhaustively tested (performing many repeated read and/or write operations).
  NFC tag data might be corrupted.

nRF Profiler
============

.. rst-class:: v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0

NCSDK-18398: Build fails if shell is enabled
   Enabling the Zephyr's :ref:`zephyr:shell_api` module together with :ref:`nrf_profiler` results in a build failure because of the bug in the :file:`CMakeLists.txt` file.

   **Workaround:** Manually cherry-pick and apply the commit with the fix from the ``main`` branch (commit hash: ``fdac428e902ebd96885160dd3ae5d08d21642926``).

Secure Partition Manager (SPM)
==============================

.. note::
    The Secure Partition Manager (SPM) is deprecated as of |NCS| v2.1.0 and removed after |NCS| v2.2.0. It is replaced by :ref:`Trusted Firmware-M (TF-M) <ug_tfm>`.

.. rst-class:: v2-2-0

NCSDK-19156: Building SPM for other boards than ``nrf5340dk_nrf5340_cpuapp`` and ``nrf9160dk_nrf9160`` fails with compilation error in cortex_m_systick.c
  This happens because the :kconfig:option:`CONFIG_CORTEX_M_SYSTICK` configuration option is enabled while the systick node is disabled in the devicetree.

  **Workaround** Enable the systick node in a DTS overlay file for the SPM build by completing the following steps:

  1. Create an overlay file :file:`systick_enabled.overlay` with the following content:

     .. code-block::

        &systick {
          status = "okay";
        };

  #. Add the overlay file as a build argument to SPM:

     .. parsed-literal::
       :class: highlight

       west build -- -Dspm_DTC_OVERLAY_FILE=systick_enabled.overlay

.. rst-class:: v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0

NCSIDB-114: Default logging causes crash
  Enabling default logging in the Secure Partition Manager sample makes it crash if the sample logs any data after the application has booted (for example, during a SecureFault, or in a secure service).
  At that point, RTC1 and UARTE0 are non-secure.

  **Workaround:** Do not enable logging and add a breakpoint in the fault handling, or try a different logging backend.

.. rst-class:: v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0

NCSDK-8232: Secure Partition Manager and application building together
  It is not possible to build and program Secure Partition Manager and the application individually.

.. rst-class:: v1-5-2 v1-5-1 v1-5-0

CIA-248: Samples with default SPM config fails to build for ``thingy91_nrf9160_ns``
   All samples using the default SPM config fails to build for the ``thingy91_nrf9160_ns``  build target if the sample is not set up with MCUboot.

   **Workaround:** Use the main branch.

MCUboot
*******

.. rst-class:: v2-0-2 v2-0-1 v2-0-0

NCSDK-15494: Unable to build with RSA and ECIES-X25519 image encryptions
  Building MCUboot with either RSA or ECIES-X25519 image encryptions feature enabled is not possible.

  **Workaround:** To fix the issue, update the ``sdk-mcuboot`` repository by cherry-picking the upstream commits with the following hashes: ``7315e424b91503819307d33ebbc3140103470dd8`` and ``0f7db390d3537bff0feee20f900f9720f90f33f9``.

.. rst-class:: v1-2-1 v1-2-0

Recovery with the USB does not work
  The MCUboot recovery feature using the USB interface does not work.

nrfxlib
*******

Crypto
======

.. rst-class:: v2-0-2 v2-0-1 v2-0-0

NCSDK-15697: ECDH key generation for Curve25519 is failing with the legacy Mbed TLS APIs for CryptoCell
  This only affects the functions ``mbedtls_ecdh_make_params_edwards`` and ``mbedtls_ecdh_read_params_edwards``.

.. rst-class:: v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0

NCSDK-13843: Limited support for MAC in PSA Crypto APIs
  The provided message authentication codes (MAC) implementation in the PSA Crypto APIs has limited support in accelerators. Only the CryptoCell accelerator supports MAC operations in PSA Crypto APIs and the supported hash algorithms are SHA-1/SHA-224/SHA-256.

.. rst-class:: v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0

NCSDK-13843: Limited support for key derivation in PSA Crypto APIs
  The provided key derivation implementation in the PSA Crypto APIs has limited support in accelerators. Only the CryptoCell accelerator supports key derivation in PSA Crypto APIs and the supported hash algorithms are the SHA-1/SHA-224/SHA-256.

.. rst-class:: v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0

NCSDK-13842: Limited ECC support in PSA Crypto APIs
  The provided ECDSA implementation in the CryptoCell accelerator does not support 521 bit curves.

.. rst-class:: v1-9-2 v1-9-1 v1-9-0

NCSDK-13825: Mbed TLS legacy APIs from Oberon has limited TLS/DTLS support
  The legacy Mbed TLS APIs in Oberon for TLS/DTLS do not support the RSA and DHE algorithms.

.. rst-class:: v1-9-2 v1-9-1 v1-9-0

NCSDK-13841: Limited support for RSA in PSA Crypto APIs
  The provided RSA implementation in the PSA Crypto APIs has limited support in accelerators.
  Only the CryptoCell accelerator supports RSA in PSA Crypto APIs. Currently, the only supported mode is PKCS1-v1.5. The key size needs to be smaller than 2048 bits and the supported hash functions are SHA-1/SHA-224/SHA-256.

.. rst-class:: v1-9-2 v1-9-1 v1-9-0

NCSDK-13844: Limited support for GCM in PSA Crypto APIs in Oberon
  The provided GCM implementation of the PSA Crypto APIs in the Oberon accelerator only supports 12 bytes IV.

.. rst-class:: v1-9-2 v1-9-1 v1-9-0

NCSDK-13900: Limited AES CBC PKCS7 support in PSA Crypto APIs
  The provided implementation in the CryptoCell accelerator for AES CBC with PKCS7 padding does not support multipart APIs.

.. rst-class:: v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0

NCSDK-5883: CMAC behavior issues
  CMAC glued with multiple backends might behave incorrectly due to memory allocation issues.

  **Workaround:** Disable glued CMAC and use only one of the enabled backends.

.. rst-class:: v1-3-1 v1-3-0

NCSDK-5546: Oberon missing symbols for HKDF
  nRF Oberon v3.0.5 is missing symbols for HKDF using SHA1, which will be fixed in an upcoming version of the library.

  **Workaround:** Use a different backend (for example, vanilla Mbed TLS) for HKDF/HMAC using SHA1.

.. rst-class:: v1-3-1 v1-3-0

Limited support for Nordic Security Module
  The :ref:`nrfxlib:nrf_security` is currently only fully supported on nRF52840 and nRF9160 devices.
  It gives compile errors on nRF52832, nRF52833, nRF52820, nRF52811, and nRF52810.

  **Workaround:** To fix the errors, cherry-pick commits in `nrfxlib PR #205 <https://github.com/nrfconnect/sdk-nrfxlib/pull/205>`_.

.. rst-class:: v1-0-0 v0-4-0

Glue layer symbol renaming issue
  The :ref:`nrfxlib:nrf_security` glue layer is broken because symbol renaming is not handled correctly.
  Therefore, the behavior is undefined when selecting multiple back-ends for the same algorithm (for example, AES).

GNSS sockets
============

.. rst-class:: v1-0-0 v0-4-0

Cold start and A-GPS data not supported
  Forcing a cold start and writing A-GPS data is not yet supported.

.. rst-class:: v0-4-0

Hard-fault with GPS in running mode
  Implementation might hard-fault when GPS is in running mode and messages are not read fast enough.

.. rst-class:: v0-4-0

NMEA strings might return wrong length
  NMEA strings are valid c-strings (0-terminated), but the read function might return wrong length.

.. rst-class:: v0-4-0

Closing sockets
  Sockets can only be closed when GPS is in stopped mode.
  Moreover, closing a socket does not properly clean up all memory resources.
  If a socket is opened and closed multiple times, this might starve the system.

Modem Library
=============

.. rst-class:: v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0

NCSDK-14312: The :c:func:`nrf_recv` function crashes occasionally
  During execution, in rare cases, the :c:func:`nrf_recv` function crashes because of a race condition.

.. rst-class:: v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0

NCSDK-13360: The :c:func:`nrf_recv` function crashes if closed by another thread while receiving data
  When calling the :c:func:`nrf_recv` function, closing the socket from another thread while it is receiving data causes a crash.

Multiprotocol Service Layer (MPSL)
==================================

.. rst-class:: v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0

DRGN-18247: Assertion with :c:enumerator:`MPSL_CLOCK_HF_LATENCY_BEST`
  When setting the ramp-up time of the high-frequency crystal oscillator with :c:enumerator:`MPSL_CLOCK_HF_LATENCY_BEST`, an assert in MPSL occurs.

  **Workaround:** Use :c:enumerator:`MPSL_CLOCK_HF_LATENCY_TYPICAL` instead of :c:enumerator:`MPSL_CLOCK_HF_LATENCY_BEST` when setting the time it takes for the HFCLK to ramp up.

.. rst-class:: v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0

DRGN-15979: :kconfig:option:`CONFIG_CLOCK_CONTROL_NRF_K32SRC_RC_CALIBRATION` must be set when :kconfig:option:`CONFIG_CLOCK_CONTROL_NRF_K32SRC_RC` is set
  MPSL requires RC clock calibration to be enabled when the RC clock is used as the Low Frequency clock source.

.. rst-class:: v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0

DRGN-14153: Radio Notification power performance penalty
  The Radio Notification feature has a power performance penalty proportional to the notification distance.
  This means an additional average current consumption of about 600 µA for the duration of the radio notification.

.. rst-class:: v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0

KRKNWK-8842: MPSL does not support nRF21540 revision 1 or older
  The nRF21540 revision 1 or older is not supported by MPSL.
  This also applies to kits that contain this device.

  **Workaround:** Check the `Nordic Semiconductor website`_ for the latest information on availability of the product version of nRF21540.

.. rst-class:: v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0

DRGN-18308: More than one user of the scheduler could cause an assert
  Examples of users of the scheduler include Bluetooth LE, IEEE 802.15.4 radio driver, timeslot (for example, flash driver).

.. rst-class:: v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0

DRGN-18555: Requesting timeslots with type ``MPSL_TIMESLOT_REQ_TYPE_EARLIEST`` can cause an assert
  When requesting timeslots with type ``MPSL_TIMESLOT_REQ_TYPE_EARLIEST``, an assert could occur in MPSL, indicating that there is already an ``EARLIEST`` request pending.

.. rst-class:: v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0

DRGN-16642: If radio notifications on ACTIVE are used, MPSL might assert
  When radio notifications are used with :c:enumerator:`MPSL_RADIO_NOTIFICATION_TYPE_INT_ON_ACTIVE` or :c:enumerator:`MPSL_RADIO_NOTIFICATION_TYPE_INT_ON_BOTH`, MPSL might assert.

.. rst-class:: v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0

DRGN-6362: Do not use the synthesized low frequency clock source
  The synthesized low frequency clock source is neither tested nor intended for usage with MPSL.

.. rst-class:: v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0

NCSIDB-731: :ref:`timeslot_sample` crashes when calling kernel APIs from zero latency interrupts
  Calling kernel APIs is not allowed from zero latency interrupts.

.. rst-class:: v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0

DRGN-17014: High Frequency Clock staying active
  The High Frequency Clock will stay active if it is turned on between timing events.
  This could occur during Low Frequency Clock calibration when using the RC oscillator as the Low Frequency Clock source.

.. rst-class:: v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0

DRGN-16506: Higher current consumption between timeslot events made with :c:macro:`MPSL_TIMESLOT_HFCLK_CFG_NO_GUARANTEE`
  When timeslot requests are made with :c:macro:`MPSL_TIMESLOT_HFCLK_CFG_NO_GUARANTEE`, the current consumption between events is higher than expected.

  **Workaround:** Use :c:macro:`MPSL_TIMESLOT_HFCLK_CFG_XTAL_GUARANTEED` instead of :c:macro:`MPSL_TIMESLOT_HFCLK_CFG_NO_GUARANTEE` when requesting a timeslot.

.. rst-class:: v1-5-0 v1-4-2 v1-4-1

DRGN-15223: :kconfig:option:`CONFIG_SYSTEM_CLOCK_NO_WAIT` is not supported for nRF5340
  Using :kconfig:option:`CONFIG_SYSTEM_CLOCK_NO_WAIT` with nRF5340 devices might not work as expected.

.. rst-class:: v1-4-2 v1-4-1

DRGN-15176: :kconfig:option:`CONFIG_SYSTEM_CLOCK_NO_WAIT` is ignored when Low Frequency Clock is started before initializing MPSL
  If the application starts the Low Frequency Clock before calling :c:func:`mpsl_init()`, the clock configuration option :kconfig:option:`CONFIG_SYSTEM_CLOCK_NO_WAIT` has no effect.
  MPSL will wait for the Low Frequency Clock to start.

  **Workaround:** When :kconfig:option:`CONFIG_SYSTEM_CLOCK_NO_WAIT` is set, do not start the Low Frequency Clock.

.. rst-class:: v1-4-0 v1-3-2 v1-3-1 v1-3-0

DRGN-15064: External Full swing and External Low swing not working
  Even though the MPSL Clock driver accepts a Low Frequency Clock source configuration for External Full swing and External Low swing, the clock control system is not configured correctly.
  For this reason, do not use :c:macro:`CLOCK_CONTROL_NRF_K32SRC_EXT_FULL_SWING` and :c:macro:`CLOCK_CONTROL_NRF_K32SRC_EXT_LOW_SWING`.

.. rst-class:: v1-5-0 v1-4-2 v1-4-1 v1-4-0

DRGN-11059: Front-end module API not implemented for SoftDevice Controller
  Front-end module API is currently not implemented for SoftDevice Controller.
  It is only available for 802.15.4.

802.15.4 Radio driver
=====================

In addition to the known issues listed here, see also :ref:`802.15.4 Radio driver limitations <nrf_802154_limitations>` for permanent limitations.

.. rst-class:: v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0

KRKNWK-14950: MPSL asserts during operation with heavy load
  An operation under heavy load can end with a crash, with the MPSL message ``ASSERT: 108, 659``.
  This issue was observed very rarely during stress tests on the devices from the nRF53 and nRF52 Series.

.. rst-class:: v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0

KRKNWK-14898: CSMA-CA backoff parameters might not be randomized in a uniform way
  The backoff parameters of the CSMA-CA operation are generated by the logic which might not return the uniformly distributed random numbers.

.. rst-class:: v1-7-1 v1-7-0

KRKNWK-11384: Assertion with Bluetooth® LE and multiprotocol usage
  The device might assert on rare occasions during the use of Bluetooth® LE and 802.15.4 multiprotocol.

.. rst-class:: v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0

KRKNWK-8133: CSMA-CA issues
  Using CSMA-CA with the open-source variant of the 802.15.4 Service Layer (SL) library causes an assertion fault.
  CSMA-CA support is currently not available in the open-source SL library.

.. rst-class:: v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-2 v1-4-0

KRKNWK-6255: RSSI parameter adjustment is not applied
  The RADIO: RSSI parameter adjustment errata (153 for nRF52840, 225 for nRF52833 and nRF52820, 87 for nRF5340) are not applied for RSSI, LQI, Energy Detection, and CCA values used by the 802.15.4 protocol.
  There is an expected offset up to +/- 6 dB in extreme temperatures of values based on RSSI measurement.

  **Workaround:** To apply RSSI parameter adjustments, cherry-pick the commits in `hal_nordic PR #88 <https://github.com/zephyrproject-rtos/hal_nordic/pull/88>`_, `sdk-nrfxlib PR #381 <https://github.com/nrfconnect/sdk-nrfxlib/pull/381>`_, and `sdk-zephyr PR #430 <https://github.com/nrfconnect/sdk-zephyr/pull/430>`_.

SoftDevice Controller
=====================

In addition to the known issues listed here, see also :ref:`softdevice_controller_limitations` for permanent limitations.

.. rst-class:: v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0

DRGN-18261: SoftDevice Controller wrongly uses non-zero randomness for the first advertising event
  The controller uses non-zero randomness even after calling :c:func:`sdc_hci_cmd_vs_set_adv_randomness` with a valid :c:type:`adv_handle` parameter.

.. rst-class:: v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0

DRGN-18358: Scanning can get hardfaults
  The SoftDevice Controller ends up in the ``HardFault`` handler after receiving an invalid response to a scan request.

.. rst-class:: v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0

DRGN-18411: Bluetooth LE Connection complete event has wrong :c:type:`peer_address_type` if resolved address
  The :c:type:`peer_address_type` parameter in the Bluetooth LE Connection Complete event is incorrectly set to ``2`` or ``3`` in case the connection is established to a device whose address was resolved.

.. rst-class:: v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0

DRGN-18420: Periodic advertiser can get ``NULL`` pointer dereference
  The controller could dereference a ``NULL`` pointer when starting a periodic advertiser.

.. rst-class:: v2-2-0

DRGN-18586: Assert when starting Periodic Advertisement Sync Transfer while Periodic Advertising is not enabled
  When initiating Periodic Advertising Sync Transfer (PAST) as advertiser, the controller might assert if the periodic advertising train is not running.

.. rst-class:: v2-2-0

DRGN-18655: Wrongly set the address if calling :c:func:`bt_ctlr_set_public_addr` before :c:func:`bt_enable`
  :c:func:`bt_ctlr_set_public_addr` accesses uninitialized memory if called before :c:func:`bt_enable`.

.. rst-class:: v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0

DRGN-18568: Using :kconfig:option:`CONFIG_MPSL_FEM` Kconfig option lowers the value of radio output power
  The actual value is lower than the default one in case the :kconfig:option:`CONFIG_BT_CTLR_TX_PWR_ANTENNA` or :kconfig:option:`CONFIG_BT_CTLR_TX_PWR` Kconfig options are used together with the :kconfig:option:`CONFIG_MPSL_FEM` Kconfig option.

.. rst-class:: v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0

DRGN-16013: Initiating connections over extended advertising is not supported when external radio coexistence and FEM support are enabled
  The initiator can assert when initiating a connection to an extended advertiser when both external radio coexistence and FEM are enabled.

  **Workaround:**  Do not enable both coex (:kconfig:option:`CONFIG_MPSL_CX_BT`) and FEM (:kconfig:option:`CONFIG_MPSL_FEM`) when support for extended advertising packets is enabled (:kconfig:option:`CONFIG_BT_EXT_ADV`).

.. rst-class:: v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0

DRGN-18105: Controller might abandon a link due to an MPSL issue
  The controller can abandon a link because of an issue in MPSL, causing a disconnect on the remote side.

.. rst-class:: v2-1-2 v2-1-1 v2-1-0

DRGN-17851: When a Bluetooth role is running, the controller might assert due to an MPSL issue
  When a Bluetooth role is running, the controller can assert because of an issue in MPSL.

.. rst-class:: v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0

DRGN-18089: Controller wrongly erases the previous periodic advertising reports
  When creating a periodic sync, the controller could in some cases erase periodic advertising reports for the previously created syncs.

.. rst-class:: v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0

DRGN-19744: Controller asserts if the sync timeout is shorter than the periodic advertising interval
  The controller asserts when trying to sync to a periodic advertiser with a sync timeout shorter than the periodic advertising interval.

.. rst-class:: v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0

DRGN-17776: Controller wrongly accepts the ``connSupervisionTimeout`` value set to ``0``
  The controller should not accept ``CONNECT_IND``, ``AUX_CONNECT_REQ``, and ``CONNECTION_UPDATE_REQ`` packets with the ``connSupervisionTimeout`` value set to ``0``.

.. rst-class:: v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0

DRGN-17777: Controller wrongly accepts LL_PAUSE_ENC_REQ packet received on an unencrypted link
  The ``LL_PAUSE_ENC_REQ`` packet shall be sent encrypted and the controller should not accept this packet on an unencrypted link.

.. rst-class:: v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0

DRGN-17656: Hard fault with periodic advertising synchronization
  When creating a periodic advertising synchronization, a hard fault might occur if receiving legacy advertising PDUs.

.. rst-class:: v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0

DRGN-17454: Wrong data length is selected if the even length is greater than 65535 us
  If the event length (``CONFIG_BT_CTLR_SDC_MAX_CONN_EVENT_LEN_DEFAULT``) was set to a value greater than 65535 us, the auto-selected maximum data length was set to 27 bytes due to an integer overflow.

.. rst-class:: v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0

DRGN-17651: Incorrect memory usage if configuring fewer TX/RX buffers than the default value
  When using the memory macros with less TX/RX packet count than the default value, the actual memory usage might be higher than expected.

.. rst-class:: v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0

DRGN-15903: :kconfig:option:`BT_CTLR_TX_PWR` is ignored by the SoftDevice Controller
  Using :kconfig:option:`BT_CTLR_TX_PWR` does not set TX power.

  **Workaround:** Use the HCI command Zephyr Write Tx Power Level to dynamically set TX power.

.. rst-class:: v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0

DRGN-17710: Periodic advertiser delay
  The periodic advertiser sends its ``AUX_SYNC_IND`` packet 40 us later than the one indicated in the ``SyncInfo`` of the ``AUX_ADV_IND`` packet.

.. rst-class:: v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0

DRGN-17710: Scanner packet reception delay
  The scanner attempts to receive the first ``AUX_SYNC_IND`` packet 40 us later than the one indicated in the ``SyncInfo`` of the ``AUX_ADV_IND``.
  This might result in the device failing to establish a synchronization to the periodic advertiser.

.. rst-class:: v1-9-2 v1-9-1 v1-9-0

DRGN-17110: Wrong address type in the LE Periodic Advertising Sync Established event when the Periodic Advertiser List is used to establish a synchronization
  The SoftDevice Controller sometimes does not set the address type when the Periodic Advertiser List is used to establish a synchronization to a Periodic Advertiser.

.. rst-class:: v1-9-2 v1-9-1 v1-9-0

DRGN-17110: The Advertiser Address Type in the LE Periodic Advertising Sync Established event is not set to 0x02 or 0x03, even if the advertiser's address was resolved (DRGN-17110)
  In the case the address is resolved, the reported address type is still set to 0x00 or 0x01.

.. rst-class:: v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0

DRGN-16859: The vendor-specific HCI commands Zephyr Write TX Power Level and Zephyr Read TX Power Level might return "Unknown Advertiser Identifier (0x42)" when setting advertising output power
  The SoftDevice Controller will return this error code if the command is issued before advertising parameters are set.

  **Workaround:** Configure the advertiser before setting TX Power using HCI LE Set Advertising Parameters

.. rst-class:: v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0

DRGN-16808: Assertion on nRF53 Series devices when the RC oscillator is used as the Low Frequency clock source
  The SoftDevice Controller might assert when :kconfig:option:`CONFIG_CLOCK_CONTROL_NRF_K32SRC_RC` is set on nRF53 Series devices and the device is connected as a peripheral.

  **Workaround:** Do not use the RC oscillator as the Low Frequency clock source.

.. rst-class:: v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0

DRGN-16650: Undefined behavior when extended scanning is enabled
  When extended scanning is enabled and :kconfig:option:`CONFIG_BT_BUF_ACL_RX_SIZE` is set to a value less than 251, it might result in asserts or undefined behavior.

  **Workaround:** Set :kconfig:option:`CONFIG_BT_BUF_EVT_RX_SIZE` to 255 when extended scanning is enabled.

.. rst-class:: v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0 v1-0-0

DRGN-16394: The host callback provided to :c:func:`sdc_enable()` is always called after every advertising event
  This will cause slightly increased power consumption.

.. rst-class:: v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0 v1-0-0

DRGN-16394: The peripheral accepts a channel map where all channels are marked as bad
  If the initiator sends a connection request with all channels marked as bad, the peripheral will always listen on data channel 0.

.. rst-class:: v1-8-0 v1-7-1 v1-7-0

DRGN-16317: The SoftDevice Controller might discard LE Extended Advertising Reports
  If there is insufficient memory available or the Host is not able to process HCI events in time, the SoftDevice Controller can discard LE Extended Advertising Reports.
  If advertising data is split across multiple reports and any of these are discarded, the Host will not be able to reassemble the data.

  **Workaround:** Increase :kconfig:option:`CONFIG_BT_BUF_EVT_RX_COUNT` until events are no longer discarded.

.. rst-class:: v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0 v1-0-0

DRGN-16341: The SoftDevice Controller might discard LE Extended Advertising Reports
  Extended Advertising Reports with data length of 228 are discarded.

.. rst-class:: v1-7-1 v1-7-0

DRGN-16113: Active scanner assert when performing extended scanning
  The active scanner might assert when performing extended scanning on Coded PHY with a full whitelist.

  **Workaround:** On nRF52 Series devices, do not use coex and fem. On nRF53 series devices, do not use CODED PHY.

.. rst-class:: v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0 v1-0-0

DRGN-16079: LLPM mode assertion
  An assertion can happen when in the LLPM mode and the connection interval is more than 1 ms.

  **Workaround:** Only use 1 ms connection interval when in LLPM mode.

.. rst-class:: v1-6-1 v1-6-0

DRGN-15993: Assertion with legacy advertising commands
  An assertion can happen when using legacy advertising commands after HCI LE Clear Advertising Sets.

  **Workaround:** Do not mix legacy and extended advertising HCI commands.

.. _drgn_15852:

.. rst-class:: v1-6-0 v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0

DRGN-15852: In rare cases on nRF53 Series devices, an assert can occur while scanning
  This only occurs when the host started scanning using HCI LE Set Scan Enable.
  This is default configuration of the Bluetooth® host.

  **Workaround:** Use extended scanning commands.
  That is, set :kconfig:option:`CONFIG_BT_EXT_ADV` to use HCI LE Set Extended Scan Enable instead.

.. rst-class:: v1-6-0 v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0

DRGN-15852: In rare cases on nRF53 Series, the scanner generates corrupted advertising reports
  The following fields are affected:

  * Event_Type
  * Address_Type
  * Direct_Address_Type
  * TX_Power
  * Advertising_SID

.. rst-class:: v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0 v1-0-0

DRGN-15586: "HCI LE Set Scan Parameters" accepts a scan window greater than the scan interval
  This can result in undefined behavior. It should fail with the result "Invalid HCI Command Parameters (0x12)".

  **Workaround:** The application should make sure the scan window is set to less than or equal to the scan interval.

.. rst-class:: v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0

DRGN-15547: Assertion when updating PHY and the event length is configured too low
  The SoftDevice Controller might assert in the following cases:

  * :c:union:`sdc_cfg_t` with :c:member:`event_length` is set to less than 2500 us and the PHY is updated from 2M to 1M, or from either 1M or 2M to Coded PHY.
  * :c:union:`sdc_cfg_t` with :c:member:`event_length` is set to less than 7500 us and a PHY update to Coded PHY is performed.

  The default value of :kconfig:option:`CONFIG_SDC_MAX_CONN_EVENT_LEN_DEFAULT` is 7500 us.
  The minimum event length supported by :kconfig:option:`CONFIG_SDC_MAX_CONN_EVENT_LEN_DEFAULT` is 2500 us.

  **Workaround:**
    * Set :c:union:`sdc_cfg_t` with :c:member:`event_length` to at least 2500 us if the application is using 1M PHY.
    * Set :c:union:`sdc_cfg_t` with :c:member:`event_length` to at least 7500 us if the application is using Coded PHY.

.. rst-class:: v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0 v1-0-0

DRGN-13594: The channel map provided by the LE Host Set Channel Classification HCI command is not always applied on the secondary advertising channels
  In this case, the extended advertiser can send secondary advertising packets on channels which are disabled by the Host.

.. rst-class:: v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0 v1-0-0

DRGN-15338: Extended scanner might generate reports containing truncated data from chained advertising PDUs
  The scanner reports partial data from advertising PDUs when there is not enough storage space for the full report.

.. rst-class:: v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0 v1-0-0

DRGN-15469: Slave connections can disconnect prematurely if there are scheduling conflicts with other roles
  This is more likely to occur during long-running events such as flash operations.

.. rst-class:: v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0

DRGN-15369: Radio output power cannot be set using the vendor-specific HCI command Zephyr Write TX Power Level for all power levels
  The command returns "Unsupported Feature or Parameter value (0x11)" if the chosen power level is not supported by the used hardware platform.

  **Workaround:** Only select output power levels that are supported by the hardware platform.

.. rst-class:: v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0 v1-0-0

DRGN-15694: An assert can occur when running an extended advertiser with maximum data length and minimum interval on Coded PHY
  The assert only occurs if there are scheduling conflicts.

  **Workaround:** Ensure the advertising interval is configured to at least 30 milliseconds when advertising on LE Coded PHY.

.. rst-class:: v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0 v1-0-0

DRGN-15484: A connectable or scannable advertiser might end with sending a packet without listening for the ``CONNECT_IND``, ``AUX_CONNECT_REQ``, ``SCAN_REQ``, or ``AUX_SCAN_REQ``
  These packets sent by the scanner or initiator can end up ignored in some cases.

.. rst-class:: v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0 v1-0-0

DRGN-15531: The coding scheme provided by the LE Set PHY HCI Command is ignored after a remote initiated PHY procedure
  The PHY options set by the host in LE Set PHY command are reverted when the remote initiates a PHY update.
  This happens because of the automatic reply of a PHY update Request in the SoftDevice Controller.
  This makes it impossible to change the PHY preferred coding in both directions.
  When receiving on S2, the SoftDevice Controller will always transmit on S8 even when configured to prefer S2.

.. rst-class:: v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0 v1-0-0

DRGN-15758: The controller might still have pending events after :c:func:`sdc_hci_evt_get()` returns false
  This will only occur if the host has masked out events.

.. rst-class:: v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0

DRGN-15251: Very rare assertion fault when connected as peripheral on Coded PHY
  The controller might assert when the following conditions are met:

  * The device is connected as a peripheral.
  * The connection PHY is set to LE Coded PHY.
  * The devices have performed a data length update, and the supported values are above the minimum specification defined values.
  * A packet is received with a CRC error.

  **Workaround:** Do not enable :kconfig:option:`CONFIG_BT_DATA_LEN_UPDATE` for applications that require Coded PHY as a peripheral device.

.. rst-class:: v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0

DRGN-15310: HCI Read RSSI fails
  The command "HCI Read RSSI" always returns "Command Disallowed (0x0C)".

.. rst-class:: v1-5-0

DRGN-15465: Corrupted advertising data when :kconfig:option:`CONFIG_BT_EXT_ADV` is set
  Setting scan response data for a legacy advertiser on a build with extended advertising support corrupts parts of the advertising data.
  When using ``BT_LE_ADV_OPT_USE_NAME`` (which is the default configuration in most samples), the device name is put in the scan response.
  This corrupts the advertising data.

  **Workaround:** Do not set scan response data.
  That implies not using the ``BT_LE_ADV_OPT_USE_NAME`` option, or the :c:macro:`BT_LE_ADV_CONN_NAME` macro when initializing Bluetooth.
  Instead, use :c:macro:`BT_LE_ADV_CONN`, and if necessary set the device name in the advertising data manually.

.. rst-class:: v1-5-2 v1-5-1 v1-5-0

DRGN-15475: Samples might not initialize the SoftDevice Controller HCI driver correctly
  Samples using both the advertising and the scanning state, but not the connected state, fail to initialize the SoftDevice Controller HCI driver.
  As a result, the function :c:func:`bt_enable()` returns an error code.

  **Workaround:** Manually enable :kconfig:option:`CONFIG_SOFTDEVICE_CONTROLLER_MULTIROLE` for the project configuration.

.. rst-class:: v1-5-0

DRGN-15382: The SoftDevice Controller cannot be qualified on nRF52832
  The SoftDevice Controller cannot be qualified on nRF52832.

  **Workaround:** Upgrade to v1.5.1 or use the main branch.

.. rst-class:: v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0 v1-0-0

DRGN-14008: The advertising data is cleared every time the advertising set is configured
  This causes the "HCI LE Set Extended Advertising Parameters" command to accept data which cannot be fit within the advertising interval instead of returning "Packet Too Long (0x45)".
  This would only occur if the advertising set is configured to use maximum data length on LE Coded PHY with an advertising interval less than 30 milliseconds.

.. rst-class:: v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0 v1-0-0

DRGN-15291: The generation of QoS Connection events is not disabled after an HCI reset
  Some event reports might still occur after a reset.

.. rst-class:: v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0

DRGN-15226: Link disconnects with reason "LMP Response Timeout (0x22)"
  If the slave receives an encryption request while the "HCI LE Long Term Key Request" event is disabled, the link disconnects with the reason "LMP Response Timeout (0x22)".
  The event is disabled when :kconfig:option:`CONFIG_BT_SMP` and/or :kconfig:option:`CONFIG_BT_CTLR_LE_ENC` is disabled.

.. rst-class:: v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0

DRGN-11963: LL control procedures cannot be initiated at the same time
  The LL control procedures (LE start encryption and LE connection parameter update) cannot be initiated at the same time or more than once.
  The controller will return an HCI error code "Controller Busy (0x3a)", as per specification's chapter 2.55.

  **Workaround:** Do not initiate these procedures at the same time.

.. rst-class:: v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0 v1-0-0

DRGN-13921: Directed advertising issues using RPA in TargetA
  The SoftDevice Controller will generate a resolvable address for the TargetA field in directed advertisements if the target device address is in the resolving list with a non-zero IRK, even if privacy is not enabled and the local device address is set to a public address.

  **Workaround:** Remove the device address from the resolving list.

.. rst-class:: v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0 v1-0-0

DRGN-10367: Advertiser times out earlier than expected
  If an extended advertiser is configured with limited duration, it will time out after the first primary channel packet in the last advertising event.

.. rst-class:: v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0 v1-0-0

DRGN-11222: A Central might disconnect prematurely if there are scheduling conflicts while doing a control procedure with an instant
  This bug will only be triggered in extremely rare cases.

.. rst-class:: v1-1-0 v1-0-0

DRGN-13231: A control packet might be sent twice even after the packet is ACKed
  This only occurs if the radio is forced off due to an unforeseen condition.

.. rst-class:: v1-1-0 v1-0-0

DRGN-13350: HCI LE Set Extended Scan Enable returns "Unsupported Feature or Parameter value (0x11)"
  This occurs when duplicate filtering is enabled.

  **Workaround:** Do not enable duplicate filtering in the controller.

.. rst-class:: v1-1-0 v1-0-0

DRGN-12122: ``secondary_max_skip`` cannot be set to a non-zero value
  HCI LE Set Advertising Parameters will return "Unsupported Feature or Parameter value (0x11)" when ``secondary_max_skip`` is set to a non-zero value.

.. rst-class:: v1-1-0 v1-0-0

DRGN-13079: An assert occurs when setting a secondary PHY to 0 when using HCI LE Set Extended Advertising Parameters
  This issue occurs when the advertising type is set to legacy advertising.

.. rst-class:: v1-1-0

:kconfig:option:`CONFIG_BT_HCI_TX_STACK_SIZE` requires specific value
  :kconfig:option:`CONFIG_BT_HCI_TX_STACK_SIZE` must be set to 1536 when selecting :kconfig:option:`CONFIG_BT_LL_SOFTDEVICE`.

.. rst-class:: v1-1-0

:kconfig:option:`CONFIG_SYSTEM_WORKQUEUE_STACK_SIZE` requires specific value
  :kconfig:option:`CONFIG_SYSTEM_WORKQUEUE_STACK_SIZE` must be set to 2048 when selecting :kconfig:option:`CONFIG_BT_LL_SOFTDEVICE` on :ref:`central_uart` and :ref:`central_bas`.

.. rst-class:: v1-1-0

:kconfig:option:`CONFIG_NFCT_IRQ_PRIORITY` requires specific value
  :kconfig:option:`CONFIG_NFCT_IRQ_PRIORITY` must be set to 5 or less when selecting :kconfig:option:`CONFIG_BT_LL_SOFTDEVICE` on :ref:`peripheral_hids_keyboard`.

.. rst-class:: v1-1-0

Several issues for nRF5340
  The following issues can occur when using SoftDevice Controller with nRF5340:

  * Poor performance when performing active scanning.
  * The controller could assert when receiving extended advertising packets.
  * The ``T_IFS`` could in certain conditions be off by 5 us.
  * The radio could stay in the TX state longer than expected.
    This issue can only occur when sending a packet on either LE 1M or LE 2M PHY after receiving or transmitting a packet on LE Coded PHY.
    If this occurs while performing a Link Layer Control Procedure, the controller could end up retransmitting an acknowledged packet, resulting in a disconnect.

.. rst-class:: v1-1-0 v1-0-0

Sending control packet twice
  A control packet could be sent twice even after the packet was acknowledged.
  This would only occur if the radio was forced off due to an unforeseen condition.

.. rst-class:: v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0 v1-0-0

DRGN-13029: The application will not immediately restart a connectable advertiser after a high duty cycle advertiser times out
  In some cases, the host might restart advertising sooner than the SoftDevice Controller is able to free its connection context.

  **Workaround:** Wait 500 ms before restarting a connectable advertiser

.. rst-class:: v1-1-0 v1-0-0

Assert risk after performing a DLE procedure
  The controller could assert when receiving a packet with a CRC error on LE Coded PHY after performing a DLE procedure where RX Octets is changed to a value above 140.

.. rst-class:: v1-0-0

No data issue when connected to multiple devices
  :c:func:`hci_data_get()` might return "No data available" when there is data available.
  This issue will only occur when connected to multiple devices at the same time.

.. rst-class:: v1-0-0

Assert on LE Write Suggested Default Data Length
  The controller will assert if the host issues LE Write Suggested Default Data Length.

.. rst-class:: v1-0-0

HCI LE Set Privacy Mode appears as not supported
  The controller does not indicate support for HCI LE Set Privacy Mode although it is supported.

.. rst-class:: v1-0-0

Assert if advertising data is set after HCI Reset
  The controller will assert if advertising data is set after HCI Reset without first setting advertising parameters.

.. rst-class:: v1-0-0

Assert on writing to flash
  The controller might assert when writing to flash.

.. rst-class:: v1-0-0

Timeout without sending packet
  A directed advertiser might time out without sending a packet on air.

nrfx
****

MDK
===

.. rst-class:: v1-2-1 v1-2-0

Incorrect pin definition for nRF5340
  For nRF5340, the pins **P1.12** to **P1.15** are unavailable due to an incorrect pin number definition in the MDK.

nrfx_saadc driver
=================

.. rst-class:: v1-1-0 v1-0-0 v0-4-0

Samples might be swapped
  Samples might be swapped when buffer is set after starting the sample process, when more than one channel is sampled.
  This can happen when the sample task is connected using PPI and setting buffers and sampling are not synchronized.

nrfx_uarte driver
=================

.. rst-class:: v1-1-0 v1-0-0 v0-4-0

RX and TX not disabled in uninit
  The driver does not disable RX and TX in uninit, which can cause higher power consumption.

nrfx_uart driver
================

.. rst-class:: v1-0-0 v0-4-0

tx_buffer_length set incorrectly
  The nrfx_uart driver might incorrectly set the internal tx_buffer_length variable when high optimization level is set during compilation.

.. _known_issue_tfm:

Trusted Firmware-M (TF-M)
*************************

.. rst-class:: v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0

NCSDK-17501: Partition Manager: Ignored alignment for partitions
  The ``align`` setting for some partitions was set incorrectly, sometimes creating overlapping partitions.
  Because of this, assertions fail in the debug builds of TF-M and the board does not boot.

  This issue affects the following configuration files:

  * :file:`pm.yml.bt_fast_pair`
  * :file:`pm.yml.emds`
  * :file:`pm.yml.file_system`
  * :file:`pm.yml.memfault`
  * :file:`pm.yml.nvs`
  * :file:`pm.yml.pgps`
  * :file:`pm.yml.tfm`
  * :file:`pm.yml.zboss`

  **Workaround:** Edit the affected configurations file so that ``align`` is correctly placed inside the ``placement`` section of the config file.

.. rst-class:: v2-3-0 v2-2-0

NCSDK-19536: TF-M does not compile when the board is missing a ``uart1`` node and TF-M logging is enabled
  TF-M does not compile when a ``uart1`` node is not defined in a board's devicetree configuration and TF-M logging is enabled.

  **Workaround:** Use one of the following workarounds:

  * Add uart1 node with baudrate property in the devicetree configuration.
  * Disable TF-M logging by enabling the :kconfig:option:`CONFIG_TFM_LOG_LEVEL_SILENCE` option.

.. rst-class:: v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0

NCSDK-15909: TF-M failing to build with Zephyr SDK 0.14.2
  TF-M may fail to build due to flash overflow with Zephyr SDK 0.14.2 when ``TFM_PROFILE_TYPE_NOT_SET`` is set to ``y``.

  **Workaround:** Use one of the following workarounds:

  * Increase the TF-M partition :kconfig:option:`CONFIG_PM_PARTITION_SIZE_TFM`.
  * Use Zephyr SDK version 0.14.1.

.. rst-class:: v2-3-0 v2-2-0 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0

NCSDK-16916: TF-M non-secure storage size might be incorrect when having multiple storage partitions
  TF-M non-secure storage partition ``nonsecure_storage`` size is incorrectly calculated when it has multiple storage partitions inside.

  **Workaround:** Use one of the following workarounds:

  * Set the sizes of the storage partitions to a multiple of :kconfig:option:`CONFIG_NRF_SPU_FLASH_REGION_SIZE`.
  * Use a static partition file.

.. rst-class:: v2-0-2 v2-0-1 v2-0-0

TF-M is not supported for Thingy:91 v1.5.0 and lower versions
  TF-M does not support Thingy:91 v1.5.0 and lower versions when using the factory-programmed bootloader to upgrade the firmware.
  TF-M is compatible with all versions of the Thingy:91 if you first upgrade the bootloader using an external debug probe.
  Additionally, TF-M functions while using the bootloader to upgrade the firmware if you upgrade the bootloader to |NCS| v2.0.0.

.. rst-class:: v2-0-2 v2-0-1 v2-0-0

NCSDK-15382: TF-M uses more RAM compared to SPM in the minimal configuration
  TF-M uses 64 KB of RAM in the minimal configuration, while SPM uses 32 KB of RAM.

  **Workaround:** Free up memory in the application if needed or keep ``CONFIG_SPM`` enabled in the application.

.. rst-class:: v2-0-2 v2-0-1 v2-0-0

NCSDK-15379: TF-M does not support FP Hard ABI
  TF-M does not support enabling the Float Point Hard Application Binary Interface configuration enabled with :kconfig:option:`CONFIG_FP_HARDABI`.

  **Workaround:** Use :kconfig:option:`CONFIG_FP_SOFTABI` or keep ``CONFIG_SPM`` enabled in the application.

.. rst-class:: v2-0-2 v2-0-1 v2-0-0

NCSDK-15345: TF-M does not support non-secure partitions in external flash
  TF-M does not support configuring a non-secure partition in external flash, such as non-secure storage or MCUboot secondary partition.
  Partitions that TF-M will attempt to configure as non-secure are: ``tfm_nonsecure``, ``nonsecure_storage``, and ``mcuboot_secondary``.

  **Workaround:** Do not put non-secure partitions in external flash or keep ``CONFIG_SPM`` enabled in the application.

.. rst-class:: v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0

NCSDK-12483: Missing debug symbols
  Some debug symbols are missing sometimes in the library model.

  **Workaround:** Add the text ``zephyr_link_libraries(-Wl,--undefined=my_missing_debug_symbol)`` in the application's :file:`CMakeLists.txt` file.

.. rst-class:: v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0

NCSDK-12342: Potential SecureFault exception while accessing protected storage
  When accessing protected storage, a SecureFault exception is sometimes triggered and execution halts.

.. rst-class:: v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0

NCSDK-11195: Build errors when enabling :kconfig:option:`CONFIG_BUILD_WITH_TFM` option
  Configuring the BUILD_WITH_TFM configuration in SES project configuration or using ``west -t menuconfig`` results in build errors.

  **Workaround:** Set ``CONFIG_BUILD_WITH_TFM=y`` in project configuration file (:file:`prj.conf`) or through west command line (``west build -- -DCONFIG_BUILD_WITH_TFM=y``).

.. rst-class:: v1-9-2 v1-9-1 v1-9-0

NCSDK-12306: Enabling debug configuration causes usage fault on nRF9160
  When the debug configuration :kconfig:option:`CONFIG_TFM_CMAKE_BUILD_TYPE_DEBUG` is enabled, a usage fault is triggered during boot on nRF9160.

.. rst-class:: v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0

NCSDK-14590: Usage fault in interrupt handlers when using FPU extensions
  When the :kconfig:option:`ARM_NONSECURE_PREEMPTIBLE_SECURE_CALLS` Kconfig option is disabled, a usage fault can be triggered when an interrupt handler uses FPU extensions while interrupting the secure processing environment.

  **Workaround:** Do not disable the :kconfig:option:`ARM_NONSECURE_PREEMPTIBLE_SECURE_CALLS` option when the :kconfig:option:`FPU` option is enabled.

.. rst-class:: v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0

NCSDK-15443: TF-M cannot be booted by MCUboot without enabling :kconfig:option:`CONFIG_MCUBOOT_CLEANUP_ARM_CORE` in MCUboot
  TF-M cannot be booted by MCUboot unless MCUboot cleans up the ARM hardware state to reset values before booting TF-M.

  **Workaround:** Upgrade MCUboot with :kconfig:option:`CONFIG_MCUBOOT_CLEANUP_ARM_CORE` enabled or keep ``CONFIG_SPM`` enabled in the application.

.. rst-class:: v1-9-2 v1-9-1 v1-9-0

NCSDK-13530: TF-M minimal build has increased in size
  TF-M minimal build exceeds 32 kB due to increased dependencies in the TF-M crypto partition.

.. rst-class:: v1-9-0

KRKNWK-12777: FLASHACCER event triggered after soft reset
  After soft reset, TF-M sometimes triggers a FLASHACCERR event and execution halts.

.. rst-class:: v1-9-0

NCSDK-14015: Execution halts during boot
  When the :kconfig:option:`CONFIG_RPMSG_SERVICE` is enabled on the nRF5340 SoC together with TF-M, the firmware does not boot.
  This option is used by OpenThread and Bluetooth modules.

  **Workaround:** Place the ``rpmsg_nrf53_sram`` partition inside the ``sram_nonsecure`` partition using :ref:`partition_manager`.

.. rst-class:: v1-9-0

NCSDK-13949: TF-M Secure Image copies FICR to RAM on nRF9160
  TF-M Secure Image copies the FICR to RAM address between ``0x2003E000`` and ``0x2003F000`` during boot on nRF9160.

Samples
*******

.. rst-class:: v2-3-0

NCSDK-20046: IPC service sample does not work with ``nrf5340dk_nrf5340_cpuapp``
  The :ref:`ipc_service_sample` sample does not work with the ``nrf5340dk_nrf5340_cpuapp`` :ref:`build target <app_boards_names_zephyr>` due to a misconfiguration.
  The application core does not log anything, while the network core seems to work and bond, but cannot transfer data.
  When using UART, there is no output visible.

.. rst-class:: v2-2-0

NCSDK-18847: :ref:`radio_test` sample does not build with support for Skyworks front-end module
  When building a sample with support for a front-end module different from nRF21540, the sample uses a non-existing configuration to initialize TX power data.
  This causes a compilation error because the source file containing code for a generic front-end module is not included in the build.

  **Workaround:** Do not use the :kconfig:option:`CONFIG_RADIO_TEST_POWER_CONTROL_AUTOMATIC` Kconfig option and replace ``CONFIG_GENERIC_FEM`` with ``CONFIG_MPSL_FEM_SIMPLE_GPIO`` in the :file:`CMakeLists.txt` file of the sample.

.. rst-class:: v2-3-0

NCSDK-19858: :ref:`at_monitor_readme` library and :ref:`nrf_cloud_mqtt_multi_service` sample heap overrun
  Occasionally, the :ref:`at_monitor_readme` library heap becomes overrun, presumably due to one of the registered AT event listeners becoming stalled.
  This has only been observed with the :ref:`nrf_cloud_mqtt_multi_service` sample.

.. rst-class:: v2-3-0 v2-2-0

NCSDK-20095: Build warning in the RF test samples when the minimal pinout generic/Skyworks FEM is used
  The :ref:`radio_test` and :ref:`direct_test_mode` samples build with a warning about generic/Skyworks FEM in minimal pinout configuration.

Zephyr
******

.. rst-class:: v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0

NCSDK-20104: MCUboot configuration can prevent application from being able to run
  MCUboot will, by default, create a read-only RAM region on the MPU that is used for the stack guard feature (enabled by default).
  The intention is that the application that gets booted clears this region.
  If the user application's startup variables reside in this memory location, the application will stop with a fault and be unable to start.
  This issue replaces the issue NCSDK-18426, which mentioned a fault in the firmware when using RTT on nRF52 Series devices.

  **Workaround:** Enable :kconfig:option:`CONFIG_MCUBOOT_CLEANUP_ARM_CORE`` in MCUboot configuration.

.. rst-class:: v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0

LwM2M engine uses incorrect encoding of object links when SenML-CBOR content format is used
  Some servers might fail to decode payload from Zephyr LwM2M client.
  This has been reported in `Zephyr issue #52527 <https://github.com/zephyrproject-rtos/zephyr/issues/52527>`_.

.. rst-class:: v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0

NCSIDB-840: Compilation of I2C TWIM driver fails when PINCTRL is disabled
  The I2C driver for TWIM peripherals (:file:`i2c_nrfx_twim.c`) cannot be compiled with :kconfig:option:`CONFIG_PINCTRL` set to ``n``.

  **Workaround:** Wrap the call to ``pinctrl_apply_state()`` on line 292 of :file:`i2c_nrfx_twim.c` in a ``#ifdef CONFIG_PINCTRL`` block.
  Additionally, when :kconfig:option:`CONFIG_PM_DEVICE` is set to ``y``, ``#ifdef CONFIG_PINCTRL`` from line 307 and the corresponding ``#endif`` need to be removed.

.. rst-class:: v1-9-2 v1-9-1 v1-9-0

The time returned by clock_gettime() API becomes incorrect after one week of uptime
  The time returned by POSIX clock_gettime() API becomes incorrect after one week elapses.
  This is due to an overflow in the uptime conversion.

.. rst-class:: v1-4-2 v1-4-1 v1-4-0

NCSDK-6330: USB Mass Storage Sample Application fails MSC Tests from USB3CV test tool
  :ref:`zephyr:usb_mass` fails the USB3CV compliance Command Set Test from the MSC Tests suite.

.. rst-class:: v1-4-2 v1-4-1 v1-4-0

NCSDK-6328: USB CDC ACM Composite Sample Application fails Chapter 9 Tests from USB3CV test tool
  :ref:`zephyr:usb_cdc-acm_composite` fails the USB3CV compliance TD 9.1: Device Descriptor Test from the Chapter 9 Test suite.

.. rst-class:: v1-4-2 v1-4-1 v1-4-0

NCSDK-6331: WebUSB sample application fails Chapter 9 Tests from USB3CV test tool
  :ref:`zephyr:webusb-sample` fails the USB3CV compliance TD 9.21: LPM L1 Suspend Resume Test from the Chapter 9 Test suite.

.. rst-class:: v1-3-1 v1-3-0

NCSIDB-108: Thread context switch might lead to a kernel fault
  If the Zephyr kernel preempts the current thread and performs a context switch to a new thread while the current thread is executing a secure service, the behavior is undefined and might lead to a kernel fault.
  To prevent this situation, a thread that aims to call a secure service must temporarily lock the kernel scheduler (:cpp:func:`k_sched_lock`) and unlock the scheduler (:cpp:func:`k_sched_unlock`) after returning from the secure call.

.. rst-class:: v1-0-0

Counter Alarm sample does not work
  The :ref:`zephyr:alarm_sample` does not work.
  A fix can be found in `Pull Request #16736 <https://github.com/zephyrproject-rtos/zephyr/pull/16736>`_.

.. rst-class:: v1-3-0 v1-2-1 v1-2-0 v1-1-0 v1-0-0

USB Mass Storage Sample Application compilation issues
  :ref:`zephyr:usb_mass` does not compile.

.. rst-class:: v1-7-1 v1-7-0 v1-4-2 v1-4-1 v1-4-0

NCSDK-6832: SMP Server sample fails upon initialization
  The :ref:`zephyr:smp_svr_sample` will fail upon initialization when using the :file:`bt-overlay.conf` Kconfig overlay file.
  This happens because of a stack overflow.

  **Workaround:** Set :kconfig:option:`CONFIG_MAIN_STACK_SIZE` to ``2048``.

SEGGER Embedded Studio Nordic Edition
*************************************

.. note::
    SEGGER Embedded Studio Nordic Edition support has been deprecated with the |NCS| v2.0.0 release.
    |VSC| is now the default IDE for the |NCS|.
    Use the `Open an existing application <Migrating IDE_>`_ option in the |nRFVSC| to migrate your application.

.. rst-class:: v1-4-2 v1-4-1 v1-4-0

NCSDK-6852: Extra CMake options might not be applied in version 5.10d
  If you specify :guilabel:`Extra CMake Build Options` in the **Open nRF Connect SDK Project** dialog and at the same time select an **nRF Connect Toolchain Version** of the form ``X.Y.Z``, the additional CMake options are discarded.

  **Workaround:** Select ``NONE (Use SES settings/environment PATH)`` from the :guilabel:`nRF Connect Toolchain Version` drop-down if you want to specify :guilabel:`Extra CMake Build Options`.

.. rst-class:: v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0

NCSDK-8372: Project name collision causes SES Nordic Edition to load the wrong project
  Some samples that are located in different folders use the same project name.
  For example, there is a ``light_switch`` project both in the :file:`samples/bluetooth/mesh/` folder and in the :file:`samples/zigbee/` folder.
  When you select one of these samples from the project list in the **Open nRF Connect SDK Project** dialog, the wrong sample might be selected.
  Check the **Build Directory** field in the dialog to see if this is the case.
  The field indicates the path to the project that SES Nordic Edition will load.

  **Workaround:** If the path in **Build Directory** points to the wrong project, select the correct project by using the :guilabel:`...` button for :guilabel:`Projects` and navigating to the correct project location.
  The build directory will update automatically.

.. rst-class:: v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0

NCSDK-9992: Multiple extra CMake options applied as single option
  If you specify two or more :guilabel:`Extra CMake Build Options` in the **Open nRF Connect SDK Project** dialog, those will be incorrectly treated as one option where the second option becomes a value to the first.
  For example: ``-DFOO=foo -DBAR=bar`` will define the CMake variable ``FOO`` having the value ``foo -DBAR=bar``.

  **Workaround:** Create a CMake preload script containing ``FOO`` and ``BAR`` settings, and then specify ``-C <pre-load-script>.cmake`` in :guilabel:`Extra CMake Build Options`.

Zephyr repository issues
************************

In addition to these known issues, check the current issues in the `official Zephyr repository`_, since these might apply to the |NCS| fork of the Zephyr repository as well.

Reporting |NCS| issues
**********************

To get help and report issues that are not related to Zephyr but to the |NCS|, go to Nordic's `DevZone`_.
