:orphan:

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
     <option value="v1-5-0" selected>v1.5.0</option>
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

.. rst-class:: v1-5-0 v1-4-2 v1-4-1 v1-4-0

NCSDK-6898: Setting :option:`CONFIG_SECURE_BOOT` does not work
  The immutable bootloader is not able to find the required metadata in the MCUboot image.
  See the related NCSDK-6898 known issue in `Build system`_ for more details.

  **Workaround:** Set :option:`CONFIG_FW_INFO` in MCUboot.

.. rst-class:: v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0

External antenna performance setting
  The preprogrammed Asset Tracker does not come with the best external antenna performance.

  **Workaround:** If you are using nRF9160 DK v0.15.0 or higher and Thingy:91 v1.4.0 or higher, set :option:`CONFIG_NRF9160_GPS_ANTENNA_EXTERNAL` to ``y``.
  Alternatively, for nRF9160 DK v0.15.0, you can set the :option:`CONFIG_NRF9160_GPS_COEX0_STRING` option to ``AT%XCOEX0`` when building the preprogrammed Asset Tracker to achieve the best external antenna performance.

.. rst-class:: v1-3-2 v1-3-1 v1-3-0

NCSDK-5574: Warnings during FOTA
   The :ref:`asset_tracker` application prints warnings and error messages during successful FOTA.

.. rst-class:: v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0 v1-0-0 v0-4-0 v0-3-0

NCSDK-6689: High current consumption in Asset Tracker
  The :ref:`asset_tracker` application might show up to 2.5 mA current consumption in idle mode with ``CONFIG_POWER_OPTIMIZATION_ENABLE=y``.

.. rst-class:: v1-0-0 v0-4-0 v0-3-0

Sending data before connecting to nRF Connect for Cloud
  The :ref:`asset_tracker` application does not wait for connection to nRF Connect for Cloud before trying to send data.
  This causes the application to crash if the user toggles one of the switches before the kit is connected to the cloud.

.. rst-class:: v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0 v1-0-0 v0-4-0 v0-3-0

IRIS-2676: Missing support for FOTA on nRF Connect for Cloud
  The :ref:`asset_tracker` application does not support the nRF Connect for Cloud FOTA_v2 protocol.

  **Workaround:** The implementation for supporting the nRF Connect for Cloud FOTA_v2 can be found in the following commits:

					* cef289b559b92186cc54f0257b8c9adc0997f334
					* 156d4cf3a568869adca445d43a786d819ae10250
					* f520159f0415f011ae66efb816384a8f7bade83d

Other issues
============

.. rst-class:: v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0

NCSDK-7856: Faulty indirection on ``nrf_cc3xx`` memory slab when freeing the platform mutex
  The :cpp:func:`mutex_free_platform` function has a bug where a call to :cpp:func:`k_mem_slab_free` provides wrong indirection on a parameter to free the platform mutex.

  **Workaround:** Write the call to free the mutex in the following way: ``k_mem_slab_free(&mutex_slab, &mutex->mutex)``.
  The change adds ``&`` before the parameter ``mutex->mutex``.

.. rst-class:: v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0

NCSDK-7914: The ``nrf_cc3xx`` RSA implementation does not deduce missing parameters
  The calls to :cpp:func:`mbedtls_rsa_complete` will not deduce all types of missing RSA parameters when using ``nrf_cc3xx`` v0.9.6 or earlier.

  **Workaround:** Calculate the missing parameters outside of this function or update to ``nrf_cc3xx`` v0.9.7 or later.

.. rst-class:: v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0

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
  The :ref:`gps_with_supl_support_sample` sample stops working if :ref:`supl_client` support is enabled, but the SUPL host name cannot be resolved.

  **Workaround:** Insert a delay (``k_sleep()``) of a few seconds after the ``printf`` on line 294 in :file:`main.c`.

.. rst-class:: v1-2-0 v1-1-0 v1-0-0

Calling nrf_connect immediately causes fail
  nrf_connect fails if called immediately after initialization of the device.
  A delay of 1000 ms is required for this to work as intended.

.. rst-class:: v1-2-0 v1-1-0 v1-0-0 v0-4-0 v0-3-0

Problems with RTT Viewer/Logger
  The SEGGER Control Block cannot be found by automatic search by the RTT Viewer/Logger.

  **Workaround:** Set the RTT Control Block address to 0 and it will try to search from address 0 and upwards.
  If this does not work, look in the ``builddir/zephyr/zephyr.map`` file to find the address of the ``_SEGGER_RTT`` symbol in the map file and use that as input to the viewer/logger.

.. rst-class:: v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0 v1-0-0

Receive error with large packets
  nRF91 fails to receive large packets (over 4000 bytes).

.. rst-class:: v1-0-0 v0-4-0 v0-3-0

Modem FW reset on debugger connection through SWD
  If a debugger (for example, J-Link) is connected via SWD to the nRF9160, the modem firmware will reset.
  Therefore, the LTE modem cannot be operational during debug sessions.

nRF5
****

nRF5340
=======

.. rst-class:: v1-5-0

KRKNWK-6756: 802.15.4 Service Layer (SL) library support for the nRF53
  The binary variant of the 802.15.4 Service Layer (SL) library for the nRF53 does not support such features as synchronization of **TIMER** with **RTC** or timestamping of received frames.
  For this reason, 802.15.4 features like delayed transmission or delayed reception are not available for the nRF53.

.. rst-class:: v1-3-2 v1-3-1 v1-3-0

FOTA does not work
  FOTA with the :ref:`zephyr:smp_svr_sample` does not work.

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

.. rst-class:: v1-5-0 v1-4-2 v1-4-1 v1-4-0

KRKNWK-6848: Reduced throughput
  Performance testing for :ref:`NCP sample <ot_ncp_sample>` shows a decrease of throughput of around 10-20% compared with the standard OpenThread.

.. rst-class:: v1-4-2 v1-4-1 v1-4-0

KRKNWK-7885: Throughput is lower when using CC310 nrf_security backend
  A decrease of throughput of around 5-10% has been observed for the :ref:`CC310 nrf_security backend <nrfxlib:nrf_security_backends_cc3xx>` when compared with :ref:`nrf_oberon <nrf_security_backends_oberon>` or :ref:`the standard mbedtls backend <nrf_security_backends_orig_mbedtls>`.
  CC310 nrf_security backend is used by default for nRF52840 boards.
  The source of throughput decrease is coupled to the cost of RTOS mutex locking when using the :ref:`CC310 nrf_security backend <nrfxlib:nrf_security_backends_cc3xx>` when the APIs are called with shorter inputs.

  **Workaround:** Use AES-CCM ciphers from the nrf_oberon backend by setting the following options:

  * :option:`CONFIG_OBERON_BACKEND` to ``y``
  * :option:`CONFIG_OBERON_MBEDTLS_AES_C` to ``y``
  * :option:`CONFIG_OBERON_MBEDTLS_CCM_C` to ``y``
  * :option:`CONFIG_CC3XX_MBEDTLS_AES_C` to ``n``

.. rst-class:: v1-4-2 v1-4-1 v1-4-0

KRKNWK-7721: MAC counter updating issue
  The ``RxDestAddrFiltered`` MAC counter is not being updated.
  This is because the ``PENDING_EVENT_RX_FAILED`` event is not implemented in Zephyr.

  **Workaround:** To fix the error, cherry-pick commits from the upstream `Zephyr PR #29226 <https://github.com/zephyrproject-rtos/zephyr/pull/29226>`_.

.. rst-class:: v1-5-0 v1-4-2 v1-4-1 v1-4-0

KRKNWK-7962: Logging interferes with shell output
  :option:`CONFIG_LOG_MINIMAL` is configured by default for most OpenThread samples.
  It accesses the UART independently from the shell backend, which sometimes leads to malformed output.

  **Workaround:** Disable logging or enable a more advanced logging option.

.. rst-class:: v1-5-0 v1-4-2 v1-4-1 v1-4-0

KRKNWK-7803: Automatically generated libraries are missing otPlatLog for NCP
  When building OpenThread libraries using a different sample than the :ref:`Thread NCP sample <ot_ncp_sample>`, the :file:`ncp_base.cpp` is not compiled with the :c:func:`otPlatLog` function.
  This results in a linking failure when building the NCP with these libraries.

  **Workaround:** Use the :ref:`Thread NCP sample <ot_ncp_sample>` to create OpenThread libraries.

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

.. rst-class:: v1-4-2 v1-4-1 v1-4-0

KRKNWK-7836: Coordinator asserting when flooded with ZDO commands
  Executing a high number of ZDO commands can cause assert on the coordinator with the :ref:`lib_zigbee_shell` component enabled.

.. rst-class:: v1-5-0 v1-4-2 v1-4-1 v1-4-0

KRKNWK-7831: Factory reset broken on coordinator with Zigbee shell
  A coordinator with the :ref:`lib_zigbee_shell` component enabled could assert after executing the ``bdb factory_reset`` command.

  **Workaround:** Call the ``bdb_reset_via_local_action`` function twice to remove all the network information.

.. rst-class:: v1-5-0 v1-4-2 v1-4-1 v1-4-0

KRKNWK-7723: OTA upgrade process restarting after client reset
  After the reset of OTA Upgrade Client, the client will start the OTA upgrade process from the beginning instead of continuing the previous process.

.. rst-class:: v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0

KRKNWK-6318: Device assert after multiple Leave requests
  If a device that rejoins the network receives Leave requests several times in a row, the device could assert.

.. rst-class:: v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0

KRKNWK-6071: ZBOSS alarms inaccurate
  On average, ZBOSS alarms last longer by 6.4 percent than Zephyr alarms.

  **Workaround:** Use Zephyr alarms.

.. rst-class:: v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0

KRKNWK-5535: Device assert if flooded with multiple Network Address requests
  The device could assert if it receives Network Address requests every 0.2 second or more frequently.

.. rst-class:: v1-3-1 v1-3-0

KRKNWK-6073: Potential delay during FOTA
  There might be a noticeable delay (~220 ms) between calling the ZBOSS API and on-the-air activity.

.. rst-class:: v1-5-0

KRKNWK-9119: Zigbee shell does not work with ZBOSS development libraries
    Because of changes to the ZBOSS API, the :ref:`lib_zigbee_shell` library cannot be enabled when :ref:`zigbee_samples` are built with the :ref:`nrfxlib:zboss` development libraries.

    **Workaround:** Use only the production version of :ref:`nrfxlib:zboss` when using :ref:`lib_zigbee_shell`.

.. rst-class:: v1-5-0

KRKNWK-9145: Corrupted payload in commands of the Scenes cluster
  When receiving Scenes cluster commands, the payload is corrupted when using the :ref:`nrfxlib:zboss` production libraries.

  **Workaround:** Use the development version of :ref:`nrfxlib:zboss`.

nRF Desktop
===========

.. rst-class:: v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0

DESK-978: Directed advertising issues with SoftDevice Link Layer
  Directed advertising (:option:`CONFIG_DESKTOP_BLE_DIRECT_ADV`) should not be used by the :ref:`nrf_desktop` application when the :ref:`nrfxlib:softdevice_controller` is in use, because that leads to reconnection problems.
  For more detailed information, see the ``Known issues and limitations`` section of the SoftDevice Controller's :ref:`nrfxlib:softdevice_controller_changelog`.

  **Workaround:** Directed advertising is disabled by default for nRF Desktop.

.. rst-class:: v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0 v1-0-0

NCSDK-8304: HID configurator issues for peripherals connected over Bluetooth LE to Linux host
  Using :ref:`nrf_desktop_config_channel_script` for peripherals connected to host directly over Bluetooth LE may result in receiving improper HID feature report ID.
  In such case, the device will provide HID input reports, but it cannot be configured with the HID configurator.

  **Workaround:** Connect the nRF Desktop peripheral through USB or using the nRF Desktop dongle.

Subsystems
**********

Bluetooth LE
============

.. rst-class:: v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0 v1-0-0

NCSDK-8224: Callbacks for "security changed" and "pairing failed" are not always called
  The pairing failed and security changed callbacks are not called when the connection is disconnected during the pairing procedure or the required security is not met.

  **Workaround:** Application should use the disconnected callback to handle pairing failed.

.. rst-class:: v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0 v1-0-0

NCSDK-8223: GATT requests might deadlock RX thread
  GATT requests might deadlock the RX thread when all TX buffers are taken by GATT requests and the RX thread tries to allocate a TX buffer for a response.
  This causes a deadlock because only the RX thread releases the TX buffers for the GATT requests.
  The deadlock is resolved by a 30 second time-out, but the ATT bearer cannot transmit without reconnecting.

  **Workaround:** Set :option:`CONFIG_BT_L2CAP_TX_BUF_COUNT` >= :option:`CONFIG_BT_ATT_TX_MAX` + 2.

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

Only secure applications can use Bluetooth LE
  Bluetooth LE cannot be used in a non-secure application, for example, an application built for the ``nrf5340_dk_nrf5340_cpuappns`` build target.

  **Workaround:** Use the ``nrf5340_dk_nrf5340_cpuapp`` build target instead.

.. rst-class:: v1-2-1 v1-2-0

Peripheral HIDS keyboard sample cannot be used with nRF Bluetooth LE Controller
  The :ref:`peripheral_hids_keyboard` sample cannot be used with the :ref:`nrfxlib:softdevice_controller` because the NFC subsystem does not work with the controller library.
  The library uses the MPSL Clock driver, which does not provide an API for asynchronous clock operation.
  NFC requires this API to work correctly.

.. rst-class:: v1-2-1 v1-2-0

Peripheral HIDS mouse sample advertising issues
  When the :ref:`peripheral_hids_mouse` sample is used with the Zephyr Bluetooth LE Controller, directed advertising does not time out and the regular advertising cannot be started.

.. rst-class:: v1-2-1 v1-2-0

Central HIDS sample issues with directed advertising
  The :ref:`bluetooth_central_hids` sample cannot connect to a peripheral that uses directed advertising.

.. rst-class:: v1-1-0

Unstable samples
  Bluetooth Low Energy peripheral samples are unstable in some conditions (when pairing and bonding are performed and then disconnections/re-connections happen).

.. rst-class:: v1-2-1 v1-2-0 v1-1-0

:option:`CONFIG_BT_SMP` alignment requirement
  When running the :ref:`bluetooth_central_dfu_smp` sample, the :option:`CONFIG_BT_SMP` configuration must be aligned between this sample and the Zephyr counterpart (:ref:`zephyr:smp_svr_sample`).
  However, security is not enabled by default in the Zephyr sample.

.. rst-class:: v1-2-1 v1-2-0 v1-1-0 v1-0-0

Reconnection issues on some operating systems
  On some operating systems, the :ref:`nrf_desktop` application is unable to reconnect to a host.

.. rst-class:: v1-1-0 v1-0-0

:ref:`central_uart` cannot handle long strings
  A too long 212-byte string cannot be handled when entered to the console to send to :ref:`peripheral_uart`.

.. rst-class:: v1-0-0

:ref:`bluetooth_central_hids` loses UART connectivity
  After programming a HEX file to the nrf52_pca10040 board, UART connectivity is lost when using the Bluetooth LE Controller.
  The board must be reset to get UART output.

.. rst-class:: v1-1-0 v1-0-0

Samples crashing on nRF51 when using GPIO
  On nRF51 devices, Bluetooth LE samples that use GPIO might crash when buttons are pressed frequently.
  In such case, the GPIO ISR introduces latency that violates real-time requirements of the Radio ISR.
  nRF51 is more sensitive to this issue than nRF52 (faster core).

.. rst-class:: v0-4-0

GATT Discovery Manager missing support
  The :ref:`gatt_dm_readme` is not supported on nRF51 devices.

.. rst-class:: v0-4-0

Samples do not work with SD Controller v0.1.0
  Bluetooth LE samples cannot be built with the :ref:`nrfxlib:softdevice_controller` v0.1.0.

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

.. rst-class:: v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0

NCSDK-5580: nRF5340 only supports SoftDevice Controller
  On nRF5340, only the :ref:`nrfxlib:softdevice_controller` is supported for Bluetooth mesh.

Bootloader
==========

.. rst-class:: v1-5-0

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

  * nrf52840_pca10056
  * nrf9160_pca10090

.. rst-class:: v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0

Immutable bootloader and netboot can overwrite non-OTP provisioning data
  In architectures that do not have OTP regions, b0 and b0n images incorrectly linked to the size of their container can overwrite provisioning partition data from their image sizes.
  Issue related to NCSDK-7982.

Build system
============

.. rst-class:: v1-5-0

Missing files or permissions when building on Windows
  Because of the Windows path length limitiations, the build can fail with errors related to permissions or missing files if some paths in the build are too long.

  **Workaround:** Shorten the build folder name, e.g. from "build_nrf5340dk_nrf5340_cpuappns" to "build", or shorten the path to the build folder in some other way.

.. rst-class:: v1-4-2 v1-4-1 v1-4-0

NCSDK-6898: Overriding child images
  Adding child image overlay from the :file:`CMakeLists.txt` top-level file located in the :file:`samples` directory overrides the existing child image overlay.

  **Workaround:** Apply the configuration from the overlay to the child image manually.

.. rst-class:: v1-4-2 v1-4-1 v1-4-0

NCSDK-6777: Project out of date when :option:`CONFIG_SECURE_BOOT` is set
  The DFU :file:`.zip` file is regenerated even when no changes are made to the files it depends on.
  As a consequence, SES displays a "Project out of date" message even when the project is not out of date.

  **Workaround:** Apply the fix from `sdk-nrf PR #3241 <https://github.com/nrfconnect/sdk-nrf/pull/3241>`_.

.. rst-class:: v1-4-2 v1-4-1 v1-4-0

NCSDK-6848: MCUboot must be built from source when included
  The build will fail if either :option:`CONFIG_MCUBOOT_BUILD_STRATEGY_SKIP_BUILD` or :option:`CONFIG_MCUBOOT_BUILD_STRATEGY_USE_HEX_FILE` is set.

  **Workaround:** Set :option:`CONFIG_MCUBOOT_BUILD_STRATEGY_FROM_SOURCE` instead.

.. rst-class:: v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0 v1-0-0 v0-4-0 v0-3-0

KRKNWK-7827: Application build system is not aware of the settings partition
  The application build system is not aware of partitions, including the settings partition, which can result in application code overlapping with other partitions.
  As a consequence, writing to overlapping partitions might remove or damage parts of the firmware, which can lead to errors that are difficult to debug.

  **Workaround:** Define and use a code partition to shrink the effective flash memory available for the application.
  You can use one of the following solutions:

  * :ref:`partition_manager` from |NCS| - see the page for all configuration options.
    For example, for single image (without bootloader and with the settings partition used), set the :option:`CONFIG_PM_SINGLE_IMAGE` Kconfig option to ``y`` and define the value for :option:`CONFIG_PM_PARTITION_SIZE_SETTINGS_STORAGE` to the required settings storage size.
  * :ref:`Devicetree code partition <zephyr:flash_map_api>` from Zephyr.
    Set :option:`CONFIG_USE_DT_CODE_PARTITION` Kconfig option to ``y``.
    Make sure that the code partition is defined and chosen correctly (``offset`` and ``size``).

.. rst-class:: v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0

NCSDK-6117: Build configuration issues
  The build configuration consisting of :ref:`bootloader`, :ref:`secure_partition_manager`, and application does not work.

  **Workaround:** Either include MCUboot in the build or use MCUboot instead of the immutable bootloader.

.. rst-class:: v1-3-2 v1-3-1 v1-3-0

Flash commands only program one core
  ``west flash`` and ``ninja flash`` only program one core, even if multiple cores are included in the build.

  **Workaround:** Execute the flash command from inside the build directory of the child image that is placed on the other core (for example, :file:`build/hci_rpmsg`).

.. rst-class:: v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0

NCSDK-8232: Secure Partition Manager and application building together
  It is not possible to build and program :ref:`secure_partition_manager` and the application individually.

.. rst-class:: v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0

NCSDK-7982: partition manager: Incorrect partition size linkage from name conflict
  Partition manager will incorrectly link a partition's size to the size of its container if the container partition's name matches its child image's name in ``CMakeLists.txt``.
  This can cause the inappropriately-sized partition to overwrite another partition beyond its intended boundary.

  **Workaround:** Rename the container partitions in the ``pm.yml`` and ``pm_static.yml`` files to something that does not match the child images' names, and rename the child images' main image partition to its name in ``CMakeLists.txt``.

DFU and FOTA
============

.. rst-class:: v1-5-0 v1-4-2 v1-4-1 v1-4-0

NCSDK-6238: Socket API calls may hang when using Download client
  When using the :ref:`lib_download_client` library with HTTP (without TLS), the application might not process incoming fragments fast enough, which can starve the :ref:`nrfxlib:nrf_modem` buffers and make calls to the Modem library hang.
  Samples and applications that are affected include those that use :ref:`lib_download_client` to download files through HTTP, or those that use :ref:`lib_fota_download` with modem updates enabled.

  **Workaround:** Set :option:`CONFIG_DOWNLOAD_CLIENT_RANGE_REQUESTS`.

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

Download stopped on socket connection time-out
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

.. rst-class:: v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0

NCSIDB-114: Default logging causes crash
  Enabling default logging in the :ref:`secure_partition_manager` sample makes it crash if the sample logs any data after the application has booted (for example, during a SecureFault, or in a secure service).
  At that point, RTC1 and UARTE0 are non-secure.

  **Workaround:** Do not enable logging and add a breakpoint in the fault handling, or try a different logging backend.


MCUboot
*******

.. rst-class:: v1-2-1 v1-2-0

Recovery with the USB does not work
  The MCUboot recovery feature using the USB interface does not work.

nrfxlib
*******

Crypto
======

.. rst-class:: v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0

NCSDK-5883: CMAC behavior issues
  CMAC glued with multiple backends may behave incorrectly due to memory allocation issues.

  **Workaround:** Disable glued CMAC and use only one of the enabled backends.

.. rst-class:: v1-3-1 v1-3-0

NCSDK-5546: Oberon missing symbols for HKDF
  nRF Oberon v3.0.5 is missing symbols for HKDF using SHA1, which will be fixed in an upcoming version of the library.

  **Workaround:** Use a different backend (for example, vanilla mbed TLS) for HKDF/HMAC using SHA1.

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

.. rst-class:: v1-5-0 v1-4-2 v1-4-1

DRGN-15223: `CONFIG_SYSTEM_CLOCK_NO_WAIT` is not supported for nRF5340
  Using :option:`CONFIG_SYSTEM_CLOCK_NO_WAIT` with nRF5340 devices might not work as expected.

.. rst-class:: v1-4-2 v1-4-1

DRGN-15176: `CONFIG_SYSTEM_CLOCK_NO_WAIT` is ignored when Low Frequency Clock is started before initializing MPSL
  If the application starts the Low Frequency Clock before calling :c:func:`mpsl_init()`, the clock configuration option :option:`CONFIG_SYSTEM_CLOCK_NO_WAIT` has no effect.
  MPSL will wait for the Low Frequency Clock to start.

  **Workaround:** When :option:`CONFIG_SYSTEM_CLOCK_NO_WAIT` is set, do not start the Low Frequency Clock.

.. rst-class:: v1-4-0 v1-3-2 v1-3-1 v1-3-0

DRGN-15064: External Full swing and External Low swing not working
  Even though the MPSL Clock driver accepts a Low Frequency Clock source configuration for External Full swing and External Low swing, the clock control system is not configured correctly.
  For this reason, do not use :c:macro:`CLOCK_CONTROL_NRF_K32SRC_EXT_FULL_SWING` and :c:macro:`CLOCK_CONTROL_NRF_K32SRC_EXT_LOW_SWING`.

.. rst-class:: v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0

DRGN-6362: Do not use the synthesized low frequency clock source
  The synthesized low frequency clock source is neither tested nor intended for usage with MPSL.

.. rst-class:: v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0

DRGN-14153: Radio Notification power performance penalty
  The Radio Notification feature has a power performance penalty proportional to the notification distance.
  This means an additional average current consumption of about 600 µA for the duration of the radio notification.

.. rst-class:: v1-5-0 v1-4-2 v1-4-1 v1-4-0

DRGN-11059: Front-end module API not implemented for SoftDevice Controller
  Front-end module API is currently not implemented for SoftDevice Controller.
  It is only available for 802.15.4.

.. rst-class:: v1-5-0 v1-4-2 v1-4-1 v1-4-0

KRKNWK-8842: MPSL does not support nRF21540 revision 1 or older
  The nRF21540 revision 1 or older is not supported by MPSL.
  This also applies to kits that contain this device.

  **Workaround:** Check nordicsemi.com for the latest information on availability of the product version of nRF21540.

802.15.4 Radio driver
=====================

.. rst-class:: v1-5-0 v1-4-2 v1-4-2 v1-4-0

KRKNWK-6255: RSSI parameter adjustment is not applied
  The RADIO: RSSI parameter adjustment errata (153 for nRF52840, 225 for nRF52833 and nRF52820, 87 for nRF5340) are not applied for RSSI, LQI, Energy Detection, and CCA values used by the 802.15.4 protocol.
  There is an expected offset up to +/- 6 dB in extreme temperatures of values based on RSSI measurement.

    **Workaround:** To apply RSSI parameter adjustments, cherry-pick the commits in `hal_nordic PR #88 <https://github.com/zephyrproject-rtos/hal_nordic/pull/88>`_, `sdk-nrfxlib PR #381 <https://github.com/nrfconnect/sdk-nrfxlib/pull/381>`_, and `sdk-zephyr PR #430 <https://github.com/nrfconnect/sdk-zephyr/pull/430>`_.

.. rst-class:: v1-5-0

KRKNWK-8133: CSMA-CA issues
  Using CSMA-CA with the open-source variant of the 802.15.4 Service Layer (SL) library causes an assertion fault.
  CSMA-CA support is currently not available in the open-source SL library.

SoftDevice Controller
=====================

.. rst-class:: v1-5-0

DRGN-15382: The SoftDevice Controller cannot be qualified on nRF52832
  The SoftDevice Controller cannot be qualified on nRF52832.

  **Workaround:** Upgrade to v1.5.1 (once available) or use the master branch.

.. rst-class:: v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0

DRGN-15226: Link disconnects with reason "LMP Response Timeout (0x22)"
  If the slave receives an encryption request while the "HCI LE Long Term Key Request" event is disabled, the link disconnects with the reason "LMP Response Timeout (0x22)".
  The event is disabled when :option:`CONFIG_BT_SMP` and/or :option:`CONFIG_BT_CTLR_LE_ENC` is disabled.

.. rst-class:: v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0

DRGN-11963: LL control procedures cannot be initiated at the same time
  The LL control procedures (LE start encryption and LE connection parameter update) cannot be initiated at the same time or more than once.
  The controller will return an HCI error code "Controller Busy (0x3a)", as per specification's chapter 2.55.

  **Workaround:** Do not initiate these procedures at the same time.

.. rst-class:: v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0 v1-0-0

DRGN-8476: Long packets not supported in connections on Coded PHY
  In connections, the Link Layer payload size is limited to 27 bytes on LE Coded PHY.

.. rst-class:: v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0 v1-0-0

DRGN-9083: AAR populated with zero IRK
  If the application has set an all zeroes IRK for a device in the resolving list, then a resolvable address that can be resolved with the all zeroes IRK will be reported to the application as that device in the advertisement report or the connected event.

.. rst-class:: v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0 v1-0-0

DRGN-13921: Directed advertising issues using RPA in TargetA
  The SoftDevice Controller will generate a resolvable address for the TargetA field in directed advertisements if the target device address is in the resolving list with a non-zero IRK, even if privacy is not enabled and the local device address is set to a public address.

  **Workaround:** Remove the device address from the resolving list.

.. rst-class:: v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0 v1-0-0

DRGN-11297: Maximum CI before entering LLPM-mode
  The maximum connection interval that can be active when switching to a connection interval of 1 ms is 10 ms.

  **Workaround:** An application that needs to use a higher interval than 10 ms needs to perform two connection updates to use 1 ms connection interval:

  * A first update to 10 ms connection interval.
  * A second update to 1 ms connection interval.

.. rst-class:: v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0 v1-0-0

DRGN-10305: Scanner can't have more than 16 seconds scan window
  If the scanner is configured with a scan window larger than 16 seconds, the scanner will truncate the scan window to 16 seconds.

.. rst-class:: v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0 v1-0-0

DRGN-8569: SEVONPEND flag must not be modified
  Applications must not modify the SEVONPEND flag in the SCR register when running in priority levels higher than 6 (priority level numerical values lower than 6) as this can lead to undefined behavior.

.. rst-class:: v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0

DRGN-6362: Synthesized low frequency clock source not tested
  Synthesized low frequency clock source is not tested or intended for use with the Bluetooth LE stack.

.. rst-class:: v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0 v1-0-0

DRGN-10367: Advertiser times out earlier than expected
  If an extended advertiser is configured with limited duration, it will time out after the first primary channel packet in the last advertising event.

.. rst-class:: v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0 v1-0-0

DRGN-12259: HCI Receiver and Transmitter Test commands not available
  The HCI Receiver and Transmitter Test commands are not available.

  **Workaround:** To perform a radio test, use a DTM application:

  * For nRF52, use the DTM application in the nRF5 SDK.
  * For nRF53, use :ref:`direct_test_mode`.

.. rst-class:: v1-1-0

:option:`CONFIG_BT_HCI_TX_STACK_SIZE` requires specific value
  :option:`CONFIG_BT_HCI_TX_STACK_SIZE` must be set to 1536 when selecting :option:`CONFIG_BT_LL_SOFTDEVICE`.

.. rst-class:: v1-1-0

:option:`CONFIG_SYSTEM_WORKQUEUE_STACK_SIZE` requires specific value
  :option:`CONFIG_SYSTEM_WORKQUEUE_STACK_SIZE` must be set to 2048 when selecting :option:`CONFIG_BT_LL_SOFTDEVICE` on :ref:`central_uart` and :ref:`central_bas`.

.. rst-class:: v1-1-0

:option:`CONFIG_NFCT_IRQ_PRIORITY` requires specific value
  :option:`CONFIG_NFCT_IRQ_PRIORITY` must be set to 5 or less when selecting :option:`CONFIG_BT_LL_SOFTDEVICE` on :ref:`peripheral_hids_keyboard`.

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

.. rst-class:: v1-1-0 v1-0-0

Wait time required after a directed high duty cycle advertiser times out
  The application is unable to restart a connectable advertiser right after a high-duty-cycle advertiser times out.

  **Workaround:** Wait 500 ms before restarting a connectable advertiser

.. rst-class:: v1-1-0 v1-0-0

Assert risk after performing a DLE procedure
  The controller could assert when receiving a packet with a CRC error on LE Coded PHY after performing a DLE procedure where RX Octets is changed to a value above 140.

.. rst-class:: v1-1-0 v1-0-0

Assert when using HCI LE Set Extended Advertising Parameters
  The controller will assert when setting secondary PHY to 0 when using HCI LE Set Extended Advertising Parameters and the advertising type is set to legacy advertising.

.. rst-class:: v1-1-0 v1-0-0

HCI issues with duplicate filtering
  HCI LE Set Extended Scan Enable returns `UNSUPPORTED_FEATURE` when duplicate filtering is enabled.

.. rst-class:: v1-1-0 v1-0-0

HCI issues with `secondary_max_skip`
  HCI LE Set Advertising Parameters returns `UNSUPPORTED_FEATURE` when `secondary_max_skip` is set to a non-zero value.

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

Time-out without sending packet
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

.. rst-class:: v1-4-2 v1-4-1 v1-4-0

NCSDK-6832: SMP Server sample fails upon initialization
  The :ref:`zephyr:smp_svr_sample` will fail upon initialization when using the :file:`bt-overlay.conf` Kconfig overlay file.
  This happens because of a stack overflow.

  **Workaround:** Set :option:`CONFIG_MAIN_STACK_SIZE` to ``2048``.

SEGGER Embedded Studio Nordic Edition
*************************************

.. rst-class:: v1-4-2 v1-4-1 v1-4-0

NCSDK-6852: Extra CMake options might not be applied in version 5.10d
  If you specify :guilabel:`Extra CMake Build Options` in the :guilabel:`Open nRF Connect SDK Project` dialog and at the same time select an :guilabel:`nRF Connect Toolchain Version` of the form ``X.Y.Z``, the additional CMake options are discarded.

  **Workaround:** Select ``NONE (Use SES settings / environment PATH)`` from the  :guilabel:`nRF Connect Toolchain Version` drop-down if you want to specify :guilabel:`Extra CMake Build Options`.

.. rst-class:: v1-5-0 v1-4-2 v1-4-1 v1-4-0

NCSDK-8372: Project name collision causes SES Nordic Edition to load the wrong project
  Some samples that are located in different folders use the same project name.
  For example, there is a ``light_switch`` project both in the :file:`samples/bluetooth/mesh/` folder and in the :file:`samples/zigbee/` folder.
  When you select one of these samples from the project list in the :guilabel:`Open nRF Connect SDK Project` dialog, the wrong sample might be selected.
  Check the :guilabel:`Build Directory` field in the dialog to see if this is the case.
  The field indicates the path to the project that SES Nordic Edition will load.

  **Workaround:** If the path in :guilabel:`Build Directory` points to the wrong project, select the correct project by using the :guilabel:`...` button for :guilabel:`Projects` and navigating to the correct project location.
  The build directory will update automatically.

----

In addition to these known issues, check the current issues in the `official Zephyr repository`_, since these might apply to the |NCS| fork of the Zephyr repository as well.
To get help and report issues that are not related to Zephyr but to the |NCS|, go to Nordic's `DevZone`_.
