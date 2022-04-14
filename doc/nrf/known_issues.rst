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
     <option value="v1-9-0" selected>v1.9.0</option>
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

   When updating this file, add entries in the following format:

   .. rst-class:: vXXX vYYY

   JIRA-XXXX: Title of the issue
     Description of the issue.
     Start every sentence on a new line.

     There can be several paragraphs, but they must be indented correctly.

     **Workaround:** The last paragraph contains the workaround.

nRF9160
*******

Asset tracker
=============

.. rst-class:: v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0

NCSDK-6898: Setting :kconfig:`CONFIG_SECURE_BOOT` does not work
  The immutable bootloader is not able to find the required metadata in the MCUboot image.
  See the related NCSDK-6898 known issue in `Build system`_ for more details.

  **Workaround:** Set :kconfig:`CONFIG_FW_INFO` in MCUboot.

.. rst-class:: v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0

External antenna performance setting
  The preprogrammed Asset Tracker does not come with the best external antenna performance.

  **Workaround:** If you are using nRF9160 DK v0.15.0 or higher and Thingy:91 v1.4.0 or higher, set :kconfig:`CONFIG_NRF9160_GPS_ANTENNA_EXTERNAL` to ``y``.
  Alternatively, for nRF9160 DK v0.15.0, you can set the :kconfig:`CONFIG_NRF9160_GPS_COEX0_STRING` option to ``AT%XCOEX0`` when building the preprogrammed Asset Tracker to achieve the best external antenna performance.

.. rst-class:: v1-3-2 v1-3-1 v1-3-0

NCSDK-5574: Warnings during FOTA
   The nRF9160: Asset Tracker application prints warnings and error messages during successful FOTA.

.. rst-class:: v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0 v1-0-0 v0-4-0 v0-3-0

NCSDK-6689: High current consumption in Asset Tracker
  The nRF9160: Asset Tracker might show up to 2.5 mA current consumption in idle mode with :kconfig:`CONFIG_POWER_OPTIMIZATION_ENABLE` set to ``y``.

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

.. rst-class:: v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0

CIA-463: Wrong network mode parameter reported to cloud
  The network mode string present in ``deviceInfo`` (nRF Cloud) and ``dev`` (Azure IoT Hub and AWS IoT) JSON objects that is reported to cloud might contain wrong network modes.
  The network mode string contains the network modes that the modem is configured to use, not what the modem actually connects to the LTE network with.

Serial LTE Modem
================

.. rst-class:: v1-9-0

NCSDK-13895: Build failure for target Thingy:91 with secure_bootloader overlay
  Building the application for Thingy:91 fails if secure_bootloader overlay is included.

Other issues
============

.. rst-class:: v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-3-1 v1-2-1 v1-2-0

NCSDK-12912: LwM2M carrier library does not recover if initial network connection fails
  When the device is switched on, if :cpp:func:`lte_lc_connect()` returns an error at timeout, it will cause :cpp:func:`lwm2m_carrier_init()` to fail.
  Thus, the device will fail to connect to carrier device management servers.

  **Workaround**: Increase :kconfig:`CONFIG_LTE_NETWORK_TIMEOUT` to allow :ref:`lte_lc_readme` more time to successfully connect.

.. rst-class:: v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-1 v1-5-0 v1-4-2 v1-3-1 v1-2-1 v1-2-0

NCSDK-12913: LwM2M carrier library will fail to initialize if phone number is not present in SIM
  The SIM phone number is needed during the :ref:`liblwm2m_carrier_readme` library start-up.
  For new SIM cards, it may take some time before the phone number is received by the network.
  The LwM2M carrier library does not wait for this to happen.
  Thus, the device can fail to connect to the carrier's device management servers.

  **Workaround**: Use one of the following workarounds:

  * Reboot or power-cycle the device after the SIM has received a phone number from the network.
  * Apply the following commits, depending on your |NCS| version:

    * `v1.7-branch <https://github.com/nrfconnect/sdk-nrf/pull/6287>`_
    * `v1.6-branch <https://github.com/nrfconnect/sdk-nrf/pull/6286>`_
    * `v1.4-branch <https://github.com/nrfconnect/sdk-nrf/pull/6270>`_

.. rst-class:: v1-8-0

NCSDK-13106: When replacing a Verizon SIM card, the LwM2M carrier library does not reconnect to the device management servers
  When a Verizon SIM card is replaced with a new Verizon SIM card, the library fails to fetch the correct PSK for the bootstrap server.
  Thus, the device fails to connect to the carrier's device management servers.

  **Workaround**: Use one of the following workarounds:

  * Use the :kconfig:`CONFIG_LWM2M_CARRIER_USE_CUSTOM_PSK` and :kconfig:`CONFIG_LWM2M_CARRIER_CUSTOM_PSK` configuration options to set the appropriate PSK needed for Verizon test or live servers.
    This PSK can be obtain from the carrier.
  * After inserting a new SIM card, reboot the device again.

.. rst-class:: v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0

NCSDK-11684: Failure loading KMU registers on nRF9160 devices
  Certain builds will experience problems loading HUK to KMU due to a bug in nrf_cc3xx_platform library prior to version 0.9.12.
  The problem arises in certain builds depending on alignment of code.
  The reason for the issue is improper handling of PAN 7 on nRF9160 devices.

  **Workaround**: Update to nrf_cc3xx_platform/nrf_cc3xx_mbedcrypto v0.9.12 or newer versions if KMU is needed.

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

.. rst-class:: v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0

NCSDK-8075: Invalid initialization of ``mbedtls_entropy_context`` mutex type
  The calls to :cpp:func:`mbedtls_entropy_init` do not zero-initialize the member variable ``mutex`` when ``nrf_cc3xx`` is enabled.

  **Workaround:** Zero-initialize the structure type before using it or make it a static variable to ensure that it is zero-initialized.

.. rst-class:: v1-4-2 v1-4-1 v1-4-0

NRF91-989: Unable to bootstrap after changing SIMs
  In some cases, swapping the SIM card may trigger the bootstrap Pre-Shared Key to be deleted from the device. This can prevent future bootstraps from succeeding.

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

Calling nrf_connect immediately causes fail
  nrf_connect fails if called immediately after initialization of the device.
  A delay of 1000 ms is required for this to work as intended.

.. rst-class:: v1-2-0 v1-1-0 v1-0-0 v0-4-0 v0-3-0

Problems with RTT Viewer/Logger
  The SEGGER Control Block cannot be found by automatic search by the RTT Viewer/Logger.

  **Workaround:** Set the RTT Control Block address to 0 and it will try to search from address 0 and upwards.
  If this does not work, look in the :file:`builddir/zephyr/zephyr.map` file to find the address of the ``_SEGGER_RTT`` symbol in the map file and use that as input to the viewer/logger.

.. rst-class:: v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0 v1-0-0

Receive error with large packets
  nRF91 fails to receive large packets (over 4000 bytes).

.. rst-class:: v1-0-0 v0-4-0 v0-3-0

Modem FW reset on debugger connection through SWD
  If a debugger (for example, J-Link) is connected through SWD to the nRF9160, the modem firmware will reset.
  Therefore, the LTE modem cannot be operational during debug sessions.

.. rst-class:: v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0

NCSDK-9441: Fmfu SMP server sample is unstable with the newest J-Link version
  Full modem serial update does not work on development kit with debugger chip version delivered with J-Link software > 6.88a

  **Workaround:** Downgrade the debugger chip to the firmware released with J-Link 6.88a or use another way of transferring serial data to the chip.

.. rst-class:: v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0

NCSDK-10106: Elevated current consumption when using applications without :ref:`nrfxlib:nrf_modem` on nRF9160
  When running applications that do not enable :ref:`nrfxlib:nrf_modem` on nRF9160 with build code B1A, current consumption will stay at 3 mA when in sleep.

  **Workaround:** Enable :ref:`nrfxlib:nrf_modem`.

.. rst-class:: v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0

CIA-351: Connectivity issues with Azure IoT Hub
  If a ``device-bound`` message is sent to the device while it is in the LTE Power Saving Mode (PSM), the TCP connection will most likely be terminated by the server.
  Known symptoms of this are frequent reconnections to cloud, messages sent to Azure IoT Hub never arriving, and FOTA images being downloaded twice.

  **Workaround:** Avoid using LTE Power Saving Mode (PSM) and extended DRX intervals longer than approximately 30 seconds. This will reduce the risk of the issue occurring, at the cost of increased power consumption.

nRF5
****

nRF5340
=======

.. rst-class:: v1-9-0 v1-8-0 v1-7-1 v1-7-0

NCSDK-11432: DFU: Erasing secondary slot returns error response
  Trying to erase secondary slot results in an error response.
  Slot is still erased.
  This issue is only occurring when the application is compiled for multi-image.

.. rst-class:: v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0

NCSDK-9786: Wrong FLASH_PAGE_ERASE_MAX_TIME_US for the nRF53 network core
  ``FLASH_PAGE_ERASE_MAX_TIME_US`` defines the execution window duration when doing the flash operation synchronously along the radio operations (:kconfig:`CONFIG_SOC_FLASH_NRF_PARTIAL_ERASE` not enabled).

  The ``FLASH_PAGE_ERASE_MAX_TIME_US`` value of the nRF53 network core is lower than required.
  For this reason, if :kconfig:`CONFIG_SOC_FLASH_NRF_RADIO_SYNC_MPSL` is set to ``y`` and :kconfig:`CONFIG_SOC_FLASH_NRF_PARTIAL_ERASE` is set to ``n``, a flash erase operation on the nRF5340 network core will result in an MPSL timeslot OVERSTAYED assert.

  **Workaround:** Increase ``FLASH_PAGE_ERASE_MAX_TIME_US`` (defined in :file:`ncs/zephyr/soc/arm/nordic_nrf/nrf53/soc.h`) from 44850UL to 89700UL (the same value as for the application core).

.. rst-class:: v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0

NCSDK-7234: UART output is not received from the network core
  The UART output is not received from the network core if the application core is programmed and running with a non-secure image (using the ``nrf5340dk_nrf5340_cpuapp_ns`` build target).

.. rst-class:: v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0

KRKNWK-6756: 802.15.4 Service Layer (SL) library support for the nRF53
  The binary variant of the 802.15.4 Service Layer (SL) library for the nRF53 does not support such features as synchronization of **TIMER** with **RTC** or timestamping of received frames.
  For this reason, 802.15.4 features like delayed transmission or delayed reception are not available for the nRF53.

.. rst-class:: v1-3-2 v1-3-1 v1-3-0

FOTA does not work
  FOTA with the :ref:`zephyr:smp_svr_sample` does not work.

.. rst-class:: v1-9-0

NCSDK-13925: Build warning in the RF test samples when the nRF21540 EK support is enabled.
  :ref:`radio_test` and :ref:`direct_test_mode` build with warnings for nRF5340 with the :ref:`ug_radio_fem_nrf21540_ek` support.

  **Workaround:** Change the parameter type in the :c:func:`nrf21540_tx_gain_set()` function in :file:`ncs/nrf/samples/bluetooth/direct_test_mode/src/fem/nrf21540.c` from :c:type:`uint8_t` to :c:type:`uint32_t`.

nRF52820
========

.. rst-class:: v1-3-2 v1-3-1 v1-3-0

Missing :file:`CMakeLists.txt`
  The :file:`CMakeLists.txt` file for developing applications that emulate nRF52820 on the nRF52833 DK is missing.

  **Workaround:** Create a :file:`CMakeLists.txt` file in the :file:`ncs/zephyr/boards/arm/nrf52833dk_nrf52820` folder with the following content::

    zephyr_compile_definitions(DEVELOP_IN_NRF52833)
    zephyr_compile_definitions(NRFX_COREDEP_DELAY_US_LOOP_CYCLES=3)

  You can `download this file <nRF52820 CMakeLists.txt_>`_ from the upstream Zephyr repository.
  After you add it, the file is automatically included by the build system.

Thread
======

.. rst-class:: v1-7-0

|NCS| v1.7.0 will not be certified for Thread
   Due to the issues KRKNWK-11555: Devices lose connection after a long time running and KRKNWK-11264: Some boards assert during high traffic, |NCS| v1.7.0 will not undergo the certification process, and is not intended to be used in final Thread products.

.. rst-class:: v1-7-0

KRKNWK-11555: Devices lose connection after a long time running
   Connection is sometimes lost after Key Sequence update.

.. rst-class:: v1-7-0

KRKNWK-11264: Some boards assert during high traffic
   The issue appears when traffic is high during a corner case, and has been observed after running stress tests for a few hours.

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
  For this reason, OpenThread retransmissions are disabled by default when the :kconfig:`CONFIG_NRF_802154_ENCRYPTION` Kconfig option is enabled.
  You can enable the retransmissions at your own risk.

.. rst-class:: v1-6-1 v1-6-0

KRKNWK-11037:  ``Udp::GetEphemeralPort`` can cause infinite loop
  Using ``Udp::GetEphemeralPort`` in OpenThread can potentially cause an infinite loop.

  **Workaround:** Avoid using ``Udp::GetEphemeralPort``.

.. rst-class:: v1-5-2 v1-5-1

KRKNWK-9461 / KRKNWK-9596 : Multiprotocol sample crashes with some smartphones
  With some smartphones, the multiprotocol sample crashes on the nRF5340 due to timer timeout inside the 802.15.4 radio driver logic.

.. rst-class:: v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0

KRKNWK-9094: Possible deadlock in shell subsystem
  Issuing OpenThread commands too fast might cause a deadlock in the shell subsystem.

  **Workaround:** If possible, avoid invoking a new command before execution of the previous one has completed.

.. rst-class:: v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0

KRKNWK-6848: Reduced throughput
  Performance testing for the :ref:`ot_coprocessor_sample` sample shows a decrease of throughput of around 10-20% compared with the standard OpenThread.

.. rst-class:: v1-4-2 v1-4-1 v1-4-0

KRKNWK-7885: Throughput is lower when using CC310 nrf_security backend
  A decrease of throughput of around 5-10% has been observed for the :ref:`CC310 nrf_security backend <nrfxlib:nrf_security_backends_cc3xx>` when compared with :ref:`nrf_oberon <nrf_security_backends_oberon>` or :ref:`the standard mbedtls backend <nrf_security_backends_orig_mbedtls>`.
  CC310 nrf_security backend is used by default for nRF52840 boards.
  The source of throughput decrease is coupled to the cost of RTOS mutex locking when using the :ref:`CC310 nrf_security backend <nrfxlib:nrf_security_backends_cc3xx>` when the APIs are called with shorter inputs.

  **Workaround:** Use AES-CCM ciphers from the nrf_oberon backend by setting the following options:

  * :kconfig:`CONFIG_OBERON_BACKEND` to ``y``
  * :kconfig:`CONFIG_OBERON_MBEDTLS_AES_C` to ``y``
  * :kconfig:`CONFIG_OBERON_MBEDTLS_CCM_C` to ``y``
  * :kconfig:`CONFIG_CC3XX_MBEDTLS_AES_C` to ``n``

.. rst-class:: v1-4-2 v1-4-1 v1-4-0

KRKNWK-7721: MAC counter updating issue
  The ``RxDestAddrFiltered`` MAC counter is not being updated.
  This is because the ``PENDING_EVENT_RX_FAILED`` event is not implemented in Zephyr.

  **Workaround:** To fix the error, cherry-pick commits from the upstream `Zephyr PR #29226 <https://github.com/zephyrproject-rtos/zephyr/pull/29226>`_.

.. rst-class:: v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0

KRKNWK-7962: Logging interferes with shell output
  :kconfig:`CONFIG_LOG_MODE_MINIMAL` is configured by default for most OpenThread samples.
  It accesses the UART independently from the shell backend, which sometimes leads to malformed output.

  **Workaround:** Disable logging or enable a more advanced logging option.

.. rst-class:: v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0

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

.. rst-class:: v1-9-0

KRKNWK-12937: Activation of Sleepy End Device must be done at the very first commissioning procedure for light switch sample
   After programming the :ref:`Zigbee light switch <zigbee_light_switch_sample>` sample and its first commissioning, Zigbee End Device joins the Zigbee network as a normal End Device. Pressing **Button 3** does not switch the device to the Sleepy End Device configuration.

   **Workaround:** Keep **Button 3** pressed during the first commissioning procedure.

.. rst-class:: v1-9-0

KRKNWK-12522: Incorrect Read Attributes Response on reading multiple attributes when the first attribute is unsupported
   When reading multiple attributes at once and the first one is not supported, the Read Attributes Response contains two records for the first supported attribute.
   The first one record has the Status field filled with Unsupported Attribute whereas the second record contains actual data.

.. rst-class:: v1-9-0

KRKNWK-12615: Get Group Membership Command returns all groups the node is assigned to
   Get Group Membership Command returns all groups the node is assigned to regardless of the destination endpoint.

.. rst-class:: v1-9-0 v1-8-0

KRKNWK-11704: NCP communication gets stuck
  The communication between the SoC and the NCP Host sometimes stops on the SoC side.
  The device neither sends nor accepts incoming packets.
  Currently, there is no workaround for this issue.

.. rst-class:: v1-8-0

KRKNWK-11465: OTA Client issues in the Image Block Request
  OTA Client cannot send Image Block Request with ``MinimumBlockPeriod`` attribute value set to ``0``.

  **Workaround**: Complete the following steps to mitigate this issue:

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

  #. In :file:`zboss/src/zcl/zcl_ota_upgrade_commands.c` file in the :file:`nrfxlib` directory, change the penultimate argument of the :c:macro:`ZB_ZCL_OTA_UPGRADE_SEND_IMAGE_BLOCK_REQ` macro to ``delay`` in :c:func:`zb_zcl_ota_upgrade_send_block_requset` and :c:func:`resend_buffer` functions.

.. rst-class:: v1-9-0 v1-8-0

KRKNWK-12115: Simultaneous commissioning of many devices can cause the Coordinator device to assert
  The Zigbee Coordinator device can assert when multiple devices are being commissioned simultaneously.
  In some cases, the device can end up in the low memory state as the result.

  **Workaround:** To lower the likelihood of the Coordinator device asserting, increase its scheduler queue and buffer pool by completing the following steps:

  1. Create your own custom memory configuration file by creating an empty header file for your application, similar to :file:`include/zb_mem_config_custom.h` header file in the :ref:`Zigbee light switch <zigbee_light_switch_sample>` sample.
  #. Copy the contents of :file:`zb_mem_config_max.h` memory configuration file to the memory configuration header file you've just created.
     The Zigbee Network Coordinator sample uses the contents of the memory configuration file by default.
  #. In your custom memory configuration file, locate the following code:

     .. code-block:: c

        /* Now if you REALLY know what you do, you can study zb_mem_config_common.h and redefine some configuration parameters, like:
        #undef ZB_CONFIG_SCHEDULER_Q_SIZE
        #define ZB_CONFIG_SCHEDULER_Q_SIZE 56
        */

  #. Replace the code you've just located with the following code:

     .. code-block:: c

        /* Increase Scheduler queue size. */
        undef ZB_CONFIG_SCHEDULER_Q_SIZE
        define ZB_CONFIG_SCHEDULER_Q_SIZE XYZ
        /* Increase buffer pool size. */
        undef ZB_CONFIG_IOBUF_POOL_SIZE
        define ZB_CONFIG_IOBUF_POOL_SIZE XYZ

  #. To increase the scheduler queue size, replace ``XYZ`` next to ``ZB_CONFIG_SCHEDULER_Q_SIZE`` with the value of your choice, ranging from ``48U`` to ``256U``.
  #. To increase the buffer pool size, replace ``XYZ`` next to ``ZB_CONFIG_IOBUF_POOL_SIZE`` with the value of your choice, ranging from ``48U`` to ``127U``.

.. rst-class:: v1-9-0 v1-8-0

KRKNWK-11826: Zigbee Router does not accept new child devices if the maximum number of children is reached
  Once the maximum number of children devices on a Zigbee Router is reached and one of them leaves the network, the Zigbee Router does not update the flags inside beacon frames to indicate that it cannot accept new devices.

**Workaround**: If the maximum number of child devices has been reached, call ``bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING)`` on the parent router from the ``ZB_ZDO_SIGNAL_LEAVE_INDICATION`` signal handler.

.. rst-class:: v1-9-0 v1-8-0

KRKNWK-11602: Zigbee device becomes not operable after receiving malformed packet
  When any Zigbee device receives a malformed packet that does not match the Zigbee packet structure, it causes ZBOSS to assert.
  The device is not automatically restarted.

**Workaround**: Power-cycle the Zigbee device.

.. rst-class:: v1-9-0 v1-8-0

KRKNWK-12017: Zigbee End Device doesn't recover from broken rejoin procedure
  If the Device Announcement packet is not acknowledged by the End Device's parent, joiner logic is stopped and device doesn't recover.

  **Workaround**: Complete the following steps to detect when the rejoin procedure breaks and reset the device:

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

   **Workaround**: Increase the value of :c:macro:`TX_BUFFERS_POOL_SIZE` or define shorter polling interval (:c:macro:`NCP_TRANSPORT_REFRESH_TIME`).

.. rst-class:: v1-5-2 v1-5-1

KRKNWK-8200: Sleepy End Device halts during the commissioning
  If the turbo poll is disabled in the ``ZB_BDB_SIGNAL_DEVICE_FIRST_START`` signal, SED halts during the commissioning.

  **Workaround**: Use the development libraries link or use ``ZB_BDB_SIGNAL_STEERING`` signal with successful status to disable turbo poll.
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

  **Workaround**: Use the development libraries link or check for Extended PAN ID in the steering signal handler.
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

  **Workaround**: To use the sensing cluster with physical sensor, copy the implementation and extend it with the selected sensor logic and properties.
  For more information, see the `declaring custom cluster`_ guide.

.. rst-class:: v1-5-2 v1-5-1

KRKNWK-6336: OTA transfer may be aborted after the MAC-level packet retransmission
  If the device receives the APS ACK for a packet that was not successfully acknowledged on the MAC level, the OTA client cluster implementation stops the image transfer.

  **Workaround**: Add a watchdog timer that will restart the OTA image transfer.

.. rst-class:: v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0

KRKNWK-7831: Factory reset broken on coordinator with Zigbee shell
  A coordinator with the :ref:`lib_zigbee_shell` component enabled could assert after executing the ``bdb factory_reset`` command.

  **Workaround:** Call the ``bdb_reset_via_local_action`` function twice to remove all the network information.

.. rst-class:: v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0

KRKNWK-7723: OTA upgrade process restarting after client reset
  After the reset of OTA Upgrade Client, the client will start the OTA upgrade process from the beginning instead of continuing the previous process.

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

Matter (Project CHIP)
=====================

.. rst-class:: v1-9-0

KRKNWK-12950: Android CHIPTool opens the commissioning window using an incorrect PIN code
  Android CHIPTool uses a random code instead of a user-provided PIN code to open the commissioning window on a Matter device.


.. rst-class:: v1-9-0 v1-8-0 v1-7-1 v1-7-0

KRKNWK-11225: Android CHIPTool cannot communicate with a Matter device after the device reboots
  Android CHIPTool does not implement any mechanism to recover a secure session to a Matter device after the device has rebooted and lost the session.
  As a result, the device can no longer decrypt and process messages sent by Android CHIPTool as the controller keeps using stale cryptographic keys.

  **Workaround** Do not reboot the device after commissioning it with Android CHIPTool.

.. rst-class:: v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0

KRKNWK-10589: Android CHIPTool crashes when commissioning a Matter device
  In random circumstances, Android CHIPTool crashes when trying to connect to a Matter device over Bluetooth® LE.

  **Workaround** Restart the application and try to commission the Matter device again.
  If the problem persists, clear the application data and try again.

.. rst-class:: v1-6-1 v1-6-0

KRKNWK-10387: Matter service is needlessly advertised over Bluetooth® LE during DFU
  The Matter samples can be configured to include the support for Device Firmware Upgrade (DFU) over Bluetooth LE.
  When the DFU procedure is started, the Matter Bluetooth LE service is needlessly advertised, revealing the device identifiers such as Vendor and Product IDs.
  The service is meant to be advertised only during the device commissioning.

.. rst-class:: v1-5-2 v1-5-1 v1-5-0

KRKNWK-9214: Pigweed submodule may not be accessible from some regions
  The ``west update`` command may generate log notifications about the failure to access the pigweed submodule.
  As a result, the Matter samples will not build.

  **Workaround:** Execute the following commands in the root folder:

    .. code-block::

       git -C modules/lib/matter submodule set-url third_party/pigweed/repo https://github.com/google/pigweed.git
       git -C modules/lib/matter submodule sync third_party/pigweed/repo
       west update

HomeKit
=======
.. rst-class:: v1-9-0 v1-8-0

KRKNWK-13095: Change in KVS key naming scheme causes an error for updated devices.
  A previous implementation allowed for empty key in domain.
  This has been restricted during refactoring.

.. rst-class:: v1-9-0

KRKNWK-13022: Activating DFU causes increased power consumption
  Currently shell is used to initiate DFU mode, which leads to increased power consumption.

.. _krknwk_10611:

.. rst-class:: v1-6-0

KRKNWK-10611: DFU fails with external flash memory
  DFU will fail when using external flash memory for update image storage.
  For this reason, DFU with external flash memory cannot be performed on HomeKit accessories.

.. rst-class:: v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0

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

.. rst-class:: v1-7-1 v1-7-0

NCSDK-12337: Possible assertion failure at boot of an USB-connected host
  During the booting procedure of a host device connected through USB, the HID report subscriptions might be disabled and enabled a few times without disconnecting the USB.
  This can result in improper subscription handling and assertion failure in the :ref:`nrf_desktop_hid_state`.

  **Workaround:** Manually cherry-pick and apply commit with fix from main (commit hash: ``3dbd4b47752671b61d13a4e5813163e9f8aef840``).

.. rst-class:: v1-7-1 v1-7-0

NCSDK-11626: HID keyboard LEDs are not turned off when host disconnects
  The HID keyboard LEDs, indicating among others state of Caps Lock and Num Lock, may not be updated after host disconnection.
  The problem replicates only if there is no other connected host.

  **Workaround**: Do not use HID keyboard LEDs.

.. rst-class:: v1-7-1 v1-7-0

NCSDK-11378: Empty HID boot report forwarding issue
  An empty HID boot report is not forwarded to the host computer by the nRF Desktop dongle upon peripheral disconnection.
  The host computer may not receive information that key that was previously reported as pressed was released.

  **Workaround**: Do not enable HID boot protocol on the nRF Desktop dongle.

.. rst-class:: v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0 v1-0-0

NCSDK-8304: HID configurator issues for peripherals connected over Bluetooth® LE to Linux host
  Using :ref:`nrf_desktop_config_channel_script` for peripherals connected to host directly over Bluetooth LE may result in receiving improper HID feature report ID.
  In such case, the device will provide HID input reports, but it cannot be configured with the HID configurator.

  **Workaround:** Use BlueZ in version 5.56 or higher.

.. rst-class:: v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0 v1-0-0

NCSDK-10907: Potential race condition related to HID input reports
  After the protocol mode changes, the :ref:`nrf_desktop_usb_state` and the :ref:`nrf_desktop_hids` modules might forward HID input reports related to the previously used protocol.

.. rst-class:: v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0

DESK-978: Directed advertising issues with SoftDevice Link Layer
  Directed advertising (``CONFIG_DESKTOP_BLE_DIRECT_ADV``) should not be used by the :ref:`nrf_desktop` application when the :ref:`nrfxlib:softdevice_controller` is in use, because that leads to reconnection problems.
  For more detailed information, see the ``Known issues and limitations`` section of the SoftDevice Controller's :ref:`nrfxlib:softdevice_controller_changelog`.

  .. note::
     The Kconfig option name changed from ``CONFIG_DESKTOP_BLE_DIRECT_ADV`` to :kconfig:`CONFIG_CAF_BLE_ADV_DIRECT_ADV` beginning with the |NCS| v1.5.99.

  **Workaround:** Directed advertising is disabled by default for nRF Desktop.

.. rst-class:: v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0

NCSDK-12020: Current consumption for Gaming Mouse increased by 1400mA
  When not in the sleep mode, the Gaming Mouse reference design has current consumption higher by 1400mA.

  **Workaround:** Change ``pwm_pin_set_cycles`` to ``pwm_pin_set_usec`` in function :c:func:`led_pwm_set_brightness` in Zephyr's driver :file:`led_pwm.c` file.

.. rst-class:: v1-9-0

NCSDK-14117: Build fails for nRF52840DK in the ``prj_b0_wwcb`` configuration
  The build failure is caused by outdated Kconfig options in the nRF52840 DK's ``prj_b0_wwcb`` configuration.
  The nRF52840 DK's ``prj_b0_wwcb`` configuration does not explicitly define static partition map either.

  **Workaround:** Manually cherry-pick and apply commit with fix from ``main`` (commit hash: ``cf4c465aceeb00d83a4f50edf67ce8c26427ac52``).

nRF Machine Learning
====================

.. rst-class:: v1-9-0

NCSDK-13923: Device may crash during Bluetooth bonding
  The device programmed with the nRF Machine Learning application may crash during Bluetooth bonding because of insufficient Bluetooth RX thread stack size.

  **Workaround:** Manually cherry-pick and apply the commit with the fix from the ``main`` branch (commit hash: ``4870fcd8316bd3a4b53ca0054f0ce35e1a8c567d``).

Pelion
======

.. rst-class:: v1-6-1 v1-6-0

NCSDK-10196: DFU fails for some configurations with the quick session resume feature enabled
  Enabling :kconfig:`CONFIG_PELION_QUICK_SESSION_RESUME` together with the OpenThread network backend leads to the quick session resume failure during the DFU packet exchange.
  This is valid for the :ref:`nRF52840 DK <ug_nrf52>` and the :ref:`nRF5340 DK <ug_nrf5340>`.

  **Workaround:** Use the quick session resume feature only for configurations with the cellular network backend.

Common Application Framework (CAF)
==================================

.. rst-class:: v1-8-0

NCSDK-13247: Sensor manager dereferences NULL pointer on wake up for sensors without trigger
  :ref:`caf_sensor_manager` dereferences NULL pointer while handling a :c:struct:`wake_up_event` if a configured sensor does not use trigger.
  This leads to undefined behavior.

  **Workaround** Manually cherry-pick and apply commit with fix from main (commit hash: ``3db6da76206d379c223afe2de646218e60e4f339``).

.. rst-class:: v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0

NCSDK-13058: Directed advertising does not work
  The directed advertising feature enabled with the :kconfig:`CONFIG_CAF_BLE_ADV_DIRECT_ADV` option does not work as intended.
  Using directed advertising towards peers that enable privacy may result in connection establishing problems.

  **Workaround** Manually cherry-pick and apply commit with fix from main (commit hash: ``c61c677872943bcf7905ddeec8b24b07ae50752e``).

Subsystems
**********

Bluetooth® LE
=============

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

  **Workaround:** Increase the ECC stack size by setting :kconfig:`CONFIG_BT_HCI_ECC_STACK_SIZE` to ``1140``.

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

  **Workaround:** Set :kconfig:`CONFIG_BT_L2CAP_TX_BUF_COUNT` >= ``CONFIG_BT_ATT_TX_MAX`` + 2.

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

:kconfig:`CONFIG_BT_SMP` alignment requirement
  When running the :ref:`bluetooth_central_dfu_smp` sample, the :kconfig:`CONFIG_BT_SMP` configuration must be aligned between this sample and the Zephyr counterpart (:ref:`zephyr:smp_svr_sample`).
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

Bluetooth mesh
==============

.. rst-class:: v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0

NCSDK-5580: nRF5340 only supports SoftDevice Controller
  On nRF5340, only the :ref:`nrfxlib:softdevice_controller` is supported for Bluetooth® mesh.

.. rst-class:: v1-6-1 v1-6-0

NCSDK-10200: The device stops sending Secure Network Beacons after re-provisioning
  Bluetooth® mesh stops sending Secure Network Beacons if the device is re-provisioned after reset through Config Node Reset message or ``bt_mesh_reset()`` call.

  **Workaround:** Reboot the device after re-provisioning.

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

The combination of nRF Secure Immutable Bootloader and MCUBoot fails to upgrade both the application and MCUBoot
  Due to a change in dependency handling in MCUBoot, MCUBoot does not read any update as a valid update.
  Issue related to NCSDK-8681.

Build system
============

.. rst-class:: v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0

NCSDK-10672: :file:`dfu_application.zip` is not updated during build
  In the configuration with MCUboot, the :file:`dfu_application.zip` might not be properly updated after code or configuration changes, because of missing dependencies.

  **Workaround:** Clear the build if :file:`dfu_application.zip` is going to be released to make sure that it is up to date.

.. rst-class:: v1-4-2 v1-4-1 v1-4-0

NCSDK-6898: Overriding child images
  Adding child image overlay from the :file:`CMakeLists.txt` top-level file located in the :file:`samples` directory overrides the existing child image overlay.

  **Workaround:** Apply the configuration from the overlay to the child image manually.

.. rst-class:: v1-4-2 v1-4-1 v1-4-0

NCSDK-6777: Project out of date when :kconfig:`CONFIG_SECURE_BOOT` is set
  The DFU :file:`.zip` file is regenerated even when no changes are made to the files it depends on.
  As a consequence, SES displays a "Project out of date" message even when the project is not out of date.

  **Workaround:** Apply the fix from `sdk-nrf PR #3241 <https://github.com/nrfconnect/sdk-nrf/pull/3241>`_.

.. rst-class:: v1-4-2 v1-4-1 v1-4-0

NCSDK-6848: MCUboot must be built from source when included
  The build will fail if either :kconfig:`CONFIG_MCUBOOT_BUILD_STRATEGY_SKIP_BUILD` or :kconfig:`CONFIG_MCUBOOT_BUILD_STRATEGY_USE_HEX_FILE` is set.

  **Workaround:** Set :kconfig:`CONFIG_MCUBOOT_BUILD_STRATEGY_FROM_SOURCE` instead.

.. rst-class:: v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0 v1-0-0 v0-4-0 v0-3-0

KRKNWK-7827: Application build system is not aware of the settings partition
  The application build system is not aware of partitions, including the settings partition, which can result in application code overlapping with other partitions.
  As a consequence, writing to overlapping partitions might remove or damage parts of the firmware, which can lead to errors that are difficult to debug.

  **Workaround:** Define and use a code partition to shrink the effective flash memory available for the application.
  You can use one of the following solutions:

  * :ref:`partition_manager` from |NCS| - see the page for all configuration options.
    For example, for single image (without bootloader and with the settings partition used), set the :kconfig:`CONFIG_PM_SINGLE_IMAGE` Kconfig option to ``y`` and define the value for :kconfig:`CONFIG_PM_PARTITION_SIZE_SETTINGS_STORAGE` to the required settings storage size.
  * :ref:`Devicetree code partition <zephyr:flash_map_api>` from Zephyr.
    Set :kconfig:`CONFIG_USE_DT_CODE_PARTITION` Kconfig option to ``y``.
    Make sure that the code partition is defined and chosen correctly (``offset`` and ``size``).

.. rst-class:: v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0

NCSDK-6117: Build configuration issues
  The build configuration consisting of :ref:`bootloader`, :ref:`secure_partition_manager`, and application does not work.

  **Workaround:** Either include MCUboot in the build or use MCUboot instead of the immutable bootloader.

.. rst-class:: v1-3-2 v1-3-1 v1-3-0

Flash commands only program one core
  ``west flash`` and ``ninja flash`` only program one core, even if multiple cores are included in the build.

  **Workaround:** Execute the flash command from inside the build directory of the child image that is placed on the other core (for example, :file:`build/hci_rpmsg`).

.. rst-class:: v1-9-0 v1-8-0 v1-7-1 v1-7-0

NCSDK-11234: Statically defined "pcd_sram" partition may cause ARM usage fault
  Inconsistency between SRAM memory partitions in Partition Manager and DTS could lead to improper memory usage.
  For example, one SRAM region may be used for multiple purposes at the same time.

  **Workaround:** Ensure that partitions defined by DTS and Partition Manager are consistent.

.. rst-class:: v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0

NCSDK-7982: Partition manager: Incorrect partition size linkage from name conflict
  The partition manager will incorrectly link a partition's size to the size of its container if the container partition's name matches its child image's name in :file:`CMakeLists.txt`.
  This can cause the inappropriately-sized partition to overwrite another partition beyond its intended boundary.

  **Workaround:** Rename the container partitions in the :file:`pm.yml` and :file:`pm_static.yml` files to something that does not match the child images' names, and rename the child images' main image partition to its name in :file:`CMakeLists.txt`.

DFU and FOTA
============

.. rst-class:: v1-9-0 v1-8-0 v1-7-1 v1-7-0

NCSDK-11308: Powering off device immediately after serial recovery of the nRF53 network core firmware results in broken firmware
  The network core will not be able to boot if the device is powered off too soon after completing a serial recovery update procedure of the network core firmware.
  This is because the firmware is being copied from shared RAM to network core flash **after** mcumgr indicates that the serial recovery procedure has completed.

  **Workaround:** Use one of the following workarounds:

  * Wait for 30 seconds before powering off the device after performing serial recovery of the nRF53 network core firmware.
  * Re-upload the network core firmware with a new serial recovery procedure if the device was powered off too soon in a previous procedure.

.. rst-class:: v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0

NCSDK-6238: Socket API calls may hang when using Download client
  When using the :ref:`lib_download_client` library with HTTP (without TLS), the application might not process incoming fragments fast enough, which can starve the :ref:`nrfxlib:nrf_modem` buffers and make calls to the Modem library hang.
  Samples and applications that are affected include those that use :ref:`lib_download_client` to download files through HTTP, or those that use :ref:`lib_fota_download` with modem updates enabled.

  **Workaround:** Set :kconfig:`CONFIG_DOWNLOAD_CLIENT_RANGE_REQUESTS`.

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
  In the :ref:`aws_fota_sample` and :ref:`http_application_update_sample` samples, the download is stopped if the socket connection times out before the modem can delete the modem firmware.
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

NFC
===

.. rst-class:: v1-2-1 v1-2-0

Sample incompatibility with the nRF5340 PDK
  The :ref:`nfc_tnep_poller` and :ref:`nfc_tag_reader` samples cannot be run on the nRF5340 PDK.
  There is an incorrect number of pins defined in the MDK files, and the pins required for :ref:`st25r3911b_nfc_readme` cannot be configured properly.

.. rst-class:: v1-2-1 v1-2-0 v1-1-0

Unstable NFC tag samples
  NFC tag samples are unstable when exhaustively tested (performing many repeated read and/or write operations).
  NFC tag data might be corrupted.

Secure Partition Manager (SPM)
==============================

.. rst-class:: v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0

NCSIDB-114: Default logging causes crash
  Enabling default logging in the :ref:`secure_partition_manager` sample makes it crash if the sample logs any data after the application has booted (for example, during a SecureFault, or in a secure service).
  At that point, RTC1 and UARTE0 are non-secure.

  **Workaround:** Do not enable logging and add a breakpoint in the fault handling, or try a different logging backend.

.. rst-class:: v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0

NCSDK-8232: Secure Partition Manager and application building together
  It is not possible to build and program :ref:`secure_partition_manager` and the application individually.

.. rst-class:: v1-5-2 v1-5-1 v1-5-0

CIA-248: Samples with default SPM config fails to build for ``thingy91_nrf9160_ns``
   All samples using the default SPM config fails to build for the ``thingy91_nrf9160_ns``  build target if the sample is not set up with MCUboot.

   **Workaround:** Use the main branch.

MCUboot
*******

.. rst-class:: v1-2-1 v1-2-0

Recovery with the USB does not work
  The MCUboot recovery feature using the USB interface does not work.

nrfxlib
*******

Crypto
======

.. rst-class:: v1-9-0

NCSDK-13825: Mbed TLS legacy APIs from Oberon has limited TLS/DTLS support
  The legacy Mbed TLS APIs in Oberon for TLS/DTLS do not support the RSA and DHE algorithms.

.. rst-class:: v1-9-0

NCSDK-13841: Limited support for RSA in PSA Crypto APIs
  The provided RSA implementation in the PSA Crypto APIs has limited support in accelerators.
  Only the CryptoCell accelerator supports RSA in PSA Crypto APIs. Currently, the only supported mode is PKCS1-v1.5. The key size needs to be smaller than 2048 bits and the supported hash functions are SHA-1/SHA-224/SHA-256.

.. rst-class:: v1-9-0

NCSDK-13843: Limited support for MAC in PSA Crypto APIs
  The provided message authentication codes (MAC) implementation in the PSA Crypto APIs has limited support in accelerators. Only the CryptoCell accelerator supports MAC operations in PSA Crypto APIs and the supported hash algorithms are SHA-1/SHA-224/SHA-256.

.. rst-class:: v1-9-0

NCSDK-13843: Limited support for key derivation in PSA Crypto APIs
  The provided key derivation implementation in the PSA Crypto APIs has limited support in accelerators. Only the CryptoCell accelerator supports key derivation in PSA Crypto APIs and the supported hash algorithms are the SHA-1/SHA-224/SHA-256.

.. rst-class:: v1-9-0

NCSDK-13844: Limited support for GCM in PSA Crypto APIs in Oberon
  The provided GCM implementation of the PSA Crypto APIs in the Oberon accelerator only supports 12 bytes IV.

.. rst-class:: v1-9-0

NCSDK-13842: Limited ECC support in PSA Crypto APIs
  The provided ECDSA implementation in the CryptoCell accelerator does not support 521 bit curves.

.. rst-class:: v1-9-0

NCSDK-13900: Limited AES CBC PKCS7 support in PSA Crypto APIs
  The provided implementation in the CryptoCell accelerator for AES CBC with PKCS7 padding does not support multipart APIs.

.. rst-class:: v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0

NCSDK-5883: CMAC behavior issues
  CMAC glued with multiple backends may behave incorrectly due to memory allocation issues.

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
  If a socket is opened and closed multiple times, this  might starve the system.

Multiprotocol Service Layer (MPSL)
==================================

.. rst-class:: v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0

DRGN-16642: If radio notifications on ACTIVE are used, MPSL may assert
  When radio notifications are used with :c:enumerator:`MPSL_RADIO_NOTIFICATION_TYPE_INT_ON_ACTIVE` or :c:enumerator:`MPSL_RADIO_NOTIFICATION_TYPE_INT_ON_BOTH`, MPSL may assert.

.. rst-class:: v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0

DRGN-16506: Higher current consumption between timeslot events made with :c:macro:`MPSL_TIMESLOT_HFCLK_CFG_NO_GUARANTEE`
  When timeslot requests are made with :c:macro:`MPSL_TIMESLOT_HFCLK_CFG_NO_GUARANTEE`, the current consumption between events is higher than expected.

  **Workaround:** Use :c:macro:`MPSL_TIMESLOT_HFCLK_CFG_XTAL_GUARANTEED` instead of :c:macro:`MPSL_TIMESLOT_HFCLK_CFG_NO_GUARANTEE` when requesting a timeslot.

.. rst-class:: v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0

DRGN-15979: :kconfig:`CONFIG_CLOCK_CONTROL_NRF_K32SRC_RC_CALIBRATION` must be set when :kconfig:`CONFIG_CLOCK_CONTROL_NRF_K32SRC_RC` is set
  MPSL requires RC clock calibration to be enabled when the RC clock is used as the Low Frequency clock source.

.. rst-class:: v1-5-0 v1-4-2 v1-4-1

DRGN-15223: :kconfig:`CONFIG_SYSTEM_CLOCK_NO_WAIT` is not supported for nRF5340
  Using :kconfig:`CONFIG_SYSTEM_CLOCK_NO_WAIT` with nRF5340 devices might not work as expected.

.. rst-class:: v1-4-2 v1-4-1

DRGN-15176: :kconfig:`CONFIG_SYSTEM_CLOCK_NO_WAIT` is ignored when Low Frequency Clock is started before initializing MPSL
  If the application starts the Low Frequency Clock before calling :c:func:`mpsl_init()`, the clock configuration option :kconfig:`CONFIG_SYSTEM_CLOCK_NO_WAIT` has no effect.
  MPSL will wait for the Low Frequency Clock to start.

  **Workaround:** When :kconfig:`CONFIG_SYSTEM_CLOCK_NO_WAIT` is set, do not start the Low Frequency Clock.

.. rst-class:: v1-4-0 v1-3-2 v1-3-1 v1-3-0

DRGN-15064: External Full swing and External Low swing not working
  Even though the MPSL Clock driver accepts a Low Frequency Clock source configuration for External Full swing and External Low swing, the clock control system is not configured correctly.
  For this reason, do not use :c:macro:`CLOCK_CONTROL_NRF_K32SRC_EXT_FULL_SWING` and :c:macro:`CLOCK_CONTROL_NRF_K32SRC_EXT_LOW_SWING`.

.. rst-class:: v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0

DRGN-6362: Do not use the synthesized low frequency clock source
  The synthesized low frequency clock source is neither tested nor intended for usage with MPSL.

.. rst-class:: v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0

DRGN-14153: Radio Notification power performance penalty
  The Radio Notification feature has a power performance penalty proportional to the notification distance.
  This means an additional average current consumption of about 600 µA for the duration of the radio notification.

.. rst-class:: v1-5-0 v1-4-2 v1-4-1 v1-4-0

DRGN-11059: Front-end module API not implemented for SoftDevice Controller
  Front-end module API is currently not implemented for SoftDevice Controller.
  It is only available for 802.15.4.

.. rst-class:: v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0

KRKNWK-8842: MPSL does not support nRF21540 revision 1 or older
  The nRF21540 revision 1 or older is not supported by MPSL.
  This also applies to kits that contain this device.

  **Workaround:** Check nordicsemi.com for the latest information on availability of the product version of nRF21540.

802.15.4 Radio driver
=====================


.. rst-class:: v1-9-0

KRKNWK-12482: Reception of correct frames can occasionally end in failure with error ``NRF_802154_RX_ERROR_RUNTIME``
  This issue can occur for the ``nrf5340dk_nrf5340_cpunet`` target if a custom application (other than :ref:`multiprotocol-rpmsg-sample` sample or :ref:`zephyr:nrf-ieee802154-rpmsg-sample` sample) is used.

.. rst-class:: v1-7-1 v1-7-0

KRKNWK-11384: Assertion with Bluetooth® LE and multiprotocol usage
  The device might assert on rare occasions during the use of Bluetooth® LE and 802.15.4 multiprotocol.

.. rst-class:: v1-9-0 v1-8-0 v1-7-1 v1-7-0

KRKNWK-11204:
  Transmitting the 802.15.4 frame with improperly populated Auxiliary Security Header field might result in assert.

  **Workaround:** Make sure that you populate the Auxiliary Security Header field according to the IEEE Std 802.15.4-2015 specification, section 9.4.

.. rst-class:: v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0

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

.. rst-class:: v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0

DRGN-16859: The vendor-specific HCI command Zephyr Write TX Power Level may return "Unknown Advertiser Identifier (0x42)" when setting advertising output power.
  The SoftDevice Controller will return this error code if the command is issued before advertising parameters are set.

  **Workaround:** Configure the advertiser before setting TX Power using HCI LE Set Advertising Parameters

.. rst-class:: v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0

DRGN-16808: Assertion on nRF53 Series devices when the RC oscillator is used as the Low Frequency clock source
  The SoftDevice Controller might assert when :kconfig:`CONFIG_CLOCK_CONTROL_NRF_K32SRC_RC` is set on nRF53 Series devices and the device is connected as a peripheral.

  **Workaround:** Do not use the RC oscillator as the Low Frequency clock source.

.. rst-class:: v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0

DRGN-16650: Undefined behavior when extended scanning is enabled
  When extended scanning is enabled and :kconfig:`CONFIG_BT_BUF_ACL_RX_SIZE` is set to a value less than 251, it may result in asserts or undefined behavior.

  **Workaround:** Set :kconfig:`CONFIG_BT_BUF_EVT_RX_SIZE` to 255 when extended scanning is enabled.

.. rst-class:: v1-9-0 v1-8-0

DRGN-16013: Initiating connections over extended advertising is not supported when external radio coexistence and FEM support are enabled
  The initiator can assert when initiating a connection to an extended advertiser when both external radio coexistence and FEM are enabled.

  **Workaround:**  Do not enable both coex (:kconfig:`CONFIG_MPSL_CX_BT`) and FEM (:kconfig:`CONFIG_MPSL_FEM`) when support for extended advertising packets is enabled (:kconfig:`CONFIG_BT_EXT_ADV`).

.. rst-class:: v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0 v1-0-0

DRGN-16394: The host callback provided to :c:func:`sdc_enable()` is always called after every advertising event
  This will cause slightly increased power consumption.

.. rst-class:: v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0 v1-0-0

DRGN-16394: The peripheral accepts a channel map where all channels are marked as bad
  If the initiator sends a connection request with all channels marked as bad, the peripheral will always listen on data channel 0.

.. rst-class:: v1-8-0 v1-7-1 v1-7-0

DRGN-16317: The SoftDevice Controller may discard LE Extended Advertising Reports
  If there is insufficient memory available or the Host is not able to process HCI events in time, the SoftDevice Controller can discard LE Extended Advertising Reports.
  If advertising data is split across multiple reports and any of these are discarded, the Host will not be able to reassemble the data.

  **Workaround:** Increase :kconfig:`CONFIG_BT_BUF_EVT_RX_COUNT` until events are no longer discarded.

.. rst-class:: v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0 v1-0-0

DRGN-16341: The SoftDevice Controller may discard LE Extended Advertising Reports
  Extended Advertising Reports with data length of 228 are discarded.

.. rst-class:: v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0

DRGN-15903: :kconfig:`BT_CTLR_TX_PWR` is ignored by the SoftDevice Controller
  Using :kconfig:`BT_CTLR_TX_PWR` does not set TX power.

  **Workaround:** Use the HCI command Zephyr Write Tx Power Level to dynamically set TX power.

.. rst-class:: v1-7-1 v1-7-0

DRGN-16113: Active scanner assert when performing extended scanning
  The active scanner might assert when performing extended scanning on Coded PHY with a full whitelist.

  **Workaround:**  On nRF52 series devices, do not use coex and fem. On nRF53 series devices, do not use CODED PHY.

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
  That is, set :kconfig:`CONFIG_BT_EXT_ADV` to use HCI LE Set Extended Scan Enable instead.

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
  The SoftDevice Controller may assert if:

  * :c:union:`sdc_cfg_t` with :c:member:`event_length` is set to less than 2500 us and the PHY is updated from 2M to 1M, or from either 1M or 2M to Coded PHY.
  * :c:union:`sdc_cfg_t` with :c:member:`event_length` is set to less than 7500 us and a PHY update to Coded PHY is performed.

  | The default value of :kconfig:`CONFIG_SDC_MAX_CONN_EVENT_LEN_DEFAULT` is 7500 us.
  | The minimum event length supported by :kconfig:`CONFIG_SDC_MAX_CONN_EVENT_LEN_DEFAULT` is 2500 us.

  **Workaround:**
    * Set :c:union:`sdc_cfg_t` with :c:member:`event_length` to at least 2500 us if the application is using 1M PHY.
    * Set :c:union:`sdc_cfg_t` with :c:member:`event_length` to at least 7500 us if the application is using Coded PHY.

.. rst-class:: v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0 v1-0-0

DRGN-13594: The channel map provided by the LE Host Set Channel Classification HCI command is not always applied on the secondary advertising channels
  In this case, the extended advertiser can send secondary advertising packets on channels which are disabled by the Host.

.. rst-class:: v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0 v1-0-0

DRGN-15338: Extended scanner may generate reports containing truncated data from chained advertising PDUs
  The scanner reports partial data from advertising PDUs when there is not enough storage space for the full report.

.. rst-class:: v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0 v1-0-0

DRGN-15469: Slave connections can disconnect prematurely if there are scheduling conflicts with other roles
  This is more likely to occur during long-running events such as flash operations.

.. rst-class:: v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0

DRGN-15369: Radio output power cannot be set using the vendor-specific HCI command Zephyr Write TX Power Level for all power levels
  The command returns "Unsupported Feature or Parameter value (0x11)" if the chosen power level is not supported by the used hardware platform.

  **Workaround** Only select output power levels that are supported by the hardware platform.

.. rst-class:: v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0 v1-0-0

DRGN-15694: An assert can occur when running an extended advertiser with maximum data length and minimum interval on Coded PHY
  The assert only occurs if there are scheduling conflicts.

  **Workaround** Ensure the advertising interval is configured to at least 30 milliseconds when advertising on LE Coded PHY.

.. rst-class:: v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0 v1-0-0

DRGN-15484: A connectable or scannable advertiser may end with sending a packet without listening for the ``CONNECT_IND``, ``AUX_CONNECT_REQ``, ``SCAN_REQ``, or ``AUX_SCAN_REQ``
  These packets sent by the scanner or initiator can end up ignored in some cases.

.. rst-class:: v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0 v1-0-0

DRGN-15531: The coding scheme provided by the LE Set PHY HCI Command is ignored after a remote initiated PHY procedure
  The PHY options set by the host in LE Set PHY command are reverted when the remote initiates a PHY update.
  This happens because of the automatic reply of a PHY update Request in the SoftDevice Controller.
  This makes it impossible to change the PHY preferred coding in both directions.
  When receiving on S2, the SoftDevice Controller will always transmit on S8 even when configured to prefer S2.

.. rst-class:: v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0 v1-0-0

DRGN-15758: The controller may still have pending events after :c:func:`sdc_hci_evt_get()` returns false
  This will only occur if the host has masked out events.

.. rst-class:: v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0

DRGN-15251: Very rare assertion fault when connected as peripheral on Coded PHY
  The controller might assert when the following conditions are met:

  * The device is connected as a peripheral.
  * The connection PHY is set to LE Coded PHY.
  * The devices have performed a data length update, and the supported values are above the minimum specification defined values.
  * A packet is received with a CRC error.

  **Workaround:** Do not enable :kconfig:`CONFIG_BT_DATA_LEN_UPDATE` for applications that require Coded PHY as a peripheral device.

.. rst-class:: v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0

DRGN-15310: HCI Read RSSI fails
  The command "HCI Read RSSI" always returns "Command Disallowed (0x0C)".

.. rst-class:: v1-5-0

DRGN-15465: Corrupted advertising data when :kconfig:`CONFIG_BT_EXT_ADV` is set
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

  **Workaround:** Manually enable :kconfig:`CONFIG_SOFTDEVICE_CONTROLLER_MULTIROLE` for the project configuration.

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
  Some event reports may still occur after a reset.

.. rst-class:: v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0

DRGN-15226: Link disconnects with reason "LMP Response Timeout (0x22)"
  If the slave receives an encryption request while the "HCI LE Long Term Key Request" event is disabled, the link disconnects with the reason "LMP Response Timeout (0x22)".
  The event is disabled when :kconfig:`CONFIG_BT_SMP` and/or :kconfig:`CONFIG_BT_CTLR_LE_ENC` is disabled.

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

DRGN-11222: A Central may disconnect prematurely if there are scheduling conflicts while doing a control procedure with an instant
  This bug will only be triggered in extremely rare cases.

.. rst-class:: v1-1-0 v1-0-0

DRGN-13231: A control packet may be sent twice even after the packet is ACKed
  This only occurs if the radio is forced off due to an unforeseen condition.

.. rst-class:: v1-1-0 v1-0-0

DRGN-13350: HCI LE Set Extended Scan Enable returns "Unsupported Feature or Parameter value (0x11)"
  This occurs when duplicate filtering is enabled.

  **Workaround** Do not enable duplicate filtering in the controller.

.. rst-class:: v1-1-0 v1-0-0

DRGN-12122: ``secondary_max_skip`` cannot be set to a non-zero value
  HCI LE Set Advertising Parameters will return "Unsupported Feature or Parameter value (0x11)" when ``secondary_max_skip`` is set to a non-zero value.

.. rst-class:: v1-1-0 v1-0-0

DRGN-13079: An assert occurs when setting a secondary PHY to 0 when using HCI LE Set Extended Advertising Parameters
  This issue occurs when the advertising type is set to legacy advertising.

.. rst-class:: v1-1-0

:kconfig:`CONFIG_BT_HCI_TX_STACK_SIZE` requires specific value
  :kconfig:`CONFIG_BT_HCI_TX_STACK_SIZE` must be set to 1536 when selecting :kconfig:`CONFIG_BT_LL_SOFTDEVICE`.

.. rst-class:: v1-1-0

:kconfig:`CONFIG_SYSTEM_WORKQUEUE_STACK_SIZE` requires specific value
  :kconfig:`CONFIG_SYSTEM_WORKQUEUE_STACK_SIZE` must be set to 2048 when selecting :kconfig:`CONFIG_BT_LL_SOFTDEVICE` on :ref:`central_uart` and :ref:`central_bas`.

.. rst-class:: v1-1-0

:kconfig:`CONFIG_NFCT_IRQ_PRIORITY` requires specific value
  :kconfig:`CONFIG_NFCT_IRQ_PRIORITY` must be set to 5 or less when selecting :kconfig:`CONFIG_BT_LL_SOFTDEVICE` on :ref:`peripheral_hids_keyboard`.

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
  In some cases, the host may restart advertising sooner than the SoftDevice Controller is able to free its connection context.

  **Workaround:** Wait 500 ms before restarting a connectable advertiser

.. rst-class:: v1-1-0 v1-0-0

Assert risk after performing a DLE procedure
  The controller could assert when receiving a packet with a CRC error on LE Coded PHY after performing a DLE procedure where RX Octets is changed to a value above 140.

.. rst-class:: v1-0-0

No data issue when connected to multiple devices
  :c:func:`hci_data_get()` may return "No data available" when there is data available.
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
  The controller may assert when writing to flash.

.. rst-class:: v1-0-0

Timeout without sending packet
  A directed advertiser may time out without sending a packet on air.

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

Trusted Firmware-M (TF-M)
*************************

.. rst-class:: v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0

NCSDK-11195: Build errors when enabling :kconfig:`CONFIG_BUILD_WITH_TFM` option
  Configuring the BUILD_WITH_TFM configuration in SES project configuration or using ``west -t menuconfig`` results in build errors.

  **Workaround:** Set ``CONFIG_BUILD_WITH_TFM=y`` in project configuration file (:file:`prj.conf`) or through west command line (``west build -- -DCONFIG_BUILD_WITH_TFM=y``).

.. rst-class:: v1-9-0

NCSDK-13530: TF-M minimal build has increased in size
  TF-M minimal build exceeds 32 kB due to increased dependencies in the TF-M crypto partition.

.. rst-class:: v1-9-0

KRKNWK-12777: FLASHACCER event triggered after soft reset
  After soft reset, TF-M sometimes triggers a FLASHACCERR event and execution halts.

.. rst-class:: v1-9-0 v1-8-0

NCSDK-12342: SecureFault exception while accessing protected storage
  When accessing protected storage, a SecureFault exception is sometimes triggered and execution halts.

.. rst-class:: v1-9-0 v1-8-0

NCSDK-12483: Missing debug symbols
  Some debug symbols are missing sometimes.

  **Workaround:** Add the following text in the application :file:`CMakeLists.txt` file: ``zephyr_link_libraries(-Wl,--undefined=my_missing_debug_symbol)``.

.. rst-class:: v1-9-0

NCSDK-14015: Execution halts during boot
  When the :kconfig:`CONFIG_RPMSG_SERVICE` is enabled on the nRF5340 SoC together with TF-M, the firmware does not boot.
  This option is used by OpenThread and Bluetooth modules.

  **Workaround:** Place the ``rpmsg_nrf53_sram`` partition inside the ``sram_nonsecure`` partition using :ref:`partition_manager`.

Zephyr
******

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

  **Workaround:** Set :kconfig:`CONFIG_MAIN_STACK_SIZE` to ``2048``.

SEGGER Embedded Studio Nordic Edition
*************************************

.. rst-class:: v1-4-2 v1-4-1 v1-4-0

NCSDK-6852: Extra CMake options might not be applied in version 5.10d
  If you specify :guilabel:`Extra CMake Build Options` in the :guilabel:`Open nRF Connect SDK Project` dialog and at the same time select an :guilabel:`nRF Connect Toolchain Version` of the form ``X.Y.Z``, the additional CMake options are discarded.

  **Workaround:** Select ``NONE (Use SES settings / environment PATH)`` from the :guilabel:`nRF Connect Toolchain Version` drop-down if you want to specify :guilabel:`Extra CMake Build Options`.

.. rst-class:: v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0

NCSDK-8372: Project name collision causes SES Nordic Edition to load the wrong project
  Some samples that are located in different folders use the same project name.
  For example, there is a ``light_switch`` project both in the :file:`samples/bluetooth/mesh/` folder and in the :file:`samples/zigbee/` folder.
  When you select one of these samples from the project list in the :guilabel:`Open nRF Connect SDK Project` dialog, the wrong sample might be selected.
  Check the :guilabel:`Build Directory` field in the dialog to see if this is the case.
  The field indicates the path to the project that SES Nordic Edition will load.

  **Workaround:** If the path in :guilabel:`Build Directory` points to the wrong project, select the correct project by using the :guilabel:`...` button for :guilabel:`Projects` and navigating to the correct project location.
  The build directory will update automatically.

.. rst-class:: v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0

NCSDK-9992: Multiple extra CMake options applied as single option
  If you specify two or more :guilabel:`Extra CMake Build Options` in the :guilabel:`Open nRF Connect SDK Project` dialog, those will be incorrectly treated as one option where the second option becomes a value to the first.
  For example: ``-DFOO=foo -DBAR=bar`` will define the CMake variable ``FOO`` having the value ``foo -DBAR=bar``.

  **Workaround:** Create a CMake preload script containing ``FOO`` and ``BAR`` settings, and then specify ``-C <pre-load-script>.cmake`` in :guilabel:`Extra CMake Build Options`.

----

In addition to these known issues, check the current issues in the `official Zephyr repository`_, since these might apply to the |NCS| fork of the Zephyr repository as well.
To get help and report issues that are not related to Zephyr but to the |NCS|, go to Nordic's `DevZone`_.
