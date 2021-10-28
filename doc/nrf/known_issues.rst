.. _known_issues:

Known issues
############

.. raw:: html
   :file: includes/filter.js

.. raw:: html

   Filter by versions:

   <select name="versions" id="versions-select">
     <option value="all" selected>All versions</option>
     <option value="v1-4-0">v1.4.0</option>
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

   When adding a new version, add it to the dropdown list above.

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

.. rst-class:: v1-3-0 v1-3-1

External antenna performance setting
  For nRF9160 DK v0.15.0, you must set the :option:`CONFIG_NRF9160_GPS_COEX0_STRING` option to ``AT%XCOEX0`` when building the preprogrammed Asset Tracker to achieve the best external antenna performance.

.. rst-class:: v1-3-0 v1-3-1

NCSDK-5574: Warnings during FOTA
   The :ref:`asset_tracker` application prints warnings and error messages during successful FOTA.

.. rst-class:: v0-3-0 v0-4-0 v1-0-0 v1-1-0 v1-2-0 v1-2-1 v1-3-0 v1-3-1

High current consumption in Asset Tracker
  The :ref:`asset_tracker` sample might show up to 2.5 mA current consumption in idle mode with ``CONFIG_POWER_OPTIMIZATION_ENABLE=y``.

.. rst-class:: v0-3-0 v0-4-0 v1-0-0

Sending data before connecting to nRF Cloud
  The :ref:`asset_tracker` sample does not wait for connection to nRF Cloud before trying to send data.
  This causes the sample to crash if the user toggles one of the switches before the board is connected to the cloud.

Other issues
============

.. rst-class:: v1-3-0 v1-3-1

NCSDK-5666: LTE Sensor Gateway
  The :ref:`lte_sensor_gateway` sample crashes when Thingy:52 is flipped.

.. rst-class:: v1-2-0 v1-2-1 v1-3-0 v1-3-1

``nrf_send`` is blocking
  The :cpp:func:`nrf_send` function in the :ref:`nrfxlib:bsdlib` might be blocking for several minutes, even if the socket is configured for non-blocking operation.
  The behavior depends on the cellular network connection.

.. rst-class:: v1-2-0

GPS sockets and SUPL client library stops working
  The :ref:`gps_with_supl_support_sample` sample stops working if :ref:`supl_client` support is enabled, but the SUPL host name cannot be resolved.

  **Workaround:** Insert a delay (``k_sleep()``) of a few seconds after the ``printf`` on line 294 in :file:`main.c`.

.. rst-class:: v1-0-0 v1-1-0 v1-2-0

Calling nrf_connect immediately causes fail
  nrf_connect fails if called immediately after initialization of the device.
  A delay of 1000 ms is required for this to work as intended.

.. rst-class:: v0-3-0 v0-4-0 v1-0-0 v1-1-0 v1-2-0

Problems with RTT Viewer/Logger
  The SEGGER Control Block cannot be found by automatic search by the RTT Viewer/Logger.

  **Workaround:** Set the RTT Control Block address to 0 and it will try to search from address 0 and upwards.
  If this does not work, look in the ``builddir/zephyr/zephyr.map`` file to find the address of the ``_SEGGER_RTT`` symbol in the map file and use that as input to the viewer/logger.

.. rst-class:: v1-0-0 v1-1-0 v1-2-0 v1-2-1 v1-3-0 v1-3-1

Receive error with large packets
  nRF91 fails to receive large packets (over 4000 bytes).

.. rst-class:: v0-3-0 v0-4-0 v1-0-0

Modem FW reset on debugger connection through SWD
  If a debugger (for example, J-Link) is connected via SWD to the nRF9160, the modem firmware will reset.
  Therefore, the LTE modem cannot be operational during debug sessions.

nRF5
****

nRF5340
=======

.. rst-class:: v1-3-0 v1-3-1

FOTA does not work
  FOTA with the :ref:`zephyr:smp_svr_sample` does not work.

nRF52820
========

.. rst-class:: v1-3-0 v1-3-1

Missing :file:`CMakeLists.txt`
  The :file:`CMakeLists.txt` file for developing applications that emulate nRF52830 on the nRF52833 DK is missing.

  **Workaround:** Create a :file:`CMakeLists.txt` file in the :file:`ncs/zephyr/boards/arm/nrf52833dk_nrf52820` folder with the following content::

    zephyr_compile_definitions(DEVELOP_IN_NRF52833)
    zephyr_compile_definitions(NRFX_COREDEP_DELAY_US_LOOP_CYCLES=3)

  You can `download this file <nRF52820 CMakeLists.txt_>`_ from the upstream Zephyr repository.
  After you add it, the file is automatically included by the build system.

Multi-Protocol Service Layer (MPSL)
===================================

.. rst-class:: v1-3-0 v1-3-1

KRKNWK-6361: Antenna Diversity not supported on nRF52840
  The Antenna Diversity feature is not supported on nRF52840 devices.

Thread
======

.. rst-class:: v1-3-0 v1-3-1

NCSDK-5014: Building with SES not possible
  It is not possible to build Thread samples using SEGGER Embedded Studio (SES).
  SES does not support :file:`.cpp` files in |NCS| projects.

.. rst-class:: v1-3-0 v1-3-1

KRKNWK-6358: CoAP client sample provisioning issues
  It is not possible to provision the :ref:`coap_client_sample` sample to servers that it cannot directly communicate with.
  This is because Link Local Address is used for communication.

.. rst-class:: v1-3-0 v1-3-1

KRKNWK-6408: ``diag`` command not supported
  The ``diag`` command is not yet supported by Thread in the |NCS|.

Zigbee
======

.. rst-class:: v1-3-0 v1-3-1

Potential delay during FOTA
  There might be a noticeable delay (~220 ms) between calling the ZBOSS API and on-the-air activity.

.. rst-class:: v1-3-0 v1-3-1

ZBOSS alarms inaccurate
  On average, ZBOSS alarms last longer by 6.4 percent than Zephyr alarms.

  **Workaround:** Use Zephyr alarms.

nRF Desktop
===========

.. rst-class:: v1-3-0 v1-3-1 v1-4-0

DESK-978: Directed advertising issues with SoftDevice Link Layer
  Directed advertising (:option:`CONFIG_DESKTOP_BLE_DIRECT_ADV`) should not be used by the :ref:`nrf_desktop` application when the :ref:`nrfxlib:softdevice_controller` is in use, because that leads to reconnection problems.
  For more detailed information, see the ``Known issues and limitations`` section of the SoftDevice Controller's :ref:`nrfxlib:softdevice_controller_changelog`.

  **Workaround:** Directed advertising is disabled by default for nRF Desktop.

Subsystems
**********

|BLE|
=====

SoftDevice Link Layer
---------------------

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

Wait time required after a directed high duty cycle advertiser times out
  When selecting :option:`CONFIG_BT_LL_SOFTDEVICE`, if a directed high duty cycle advertiser times out, the application might have to wait a short time before starting a new connectable advertiser.
  Otherwise, starting the advertiser will fail.

.. rst-class:: v1-1-0

Assert risk after performing a DLE procedure
  The :ref:`nrfxlib:softdevice_controller` 0.3.0-3.prealpha might assert when receiving a packet with an CRC error on LE Coded PHY after performing a DLE procedure where RX Octets is changed to a value above 140.

Other issues
------------

.. rst-class:: v1-3-0 v1-3-1

NCSDK-5711: High-throughput transmission can deadlock the receive thread
  High-throughput transmission can deadlock the receive thread if the connection is suddenly disconnected.

.. rst-class:: v1-2-0 v1-2-1

Only secure applications can use |BLE|
  Bluetooth LE cannot be used in a non-secure application, for example, an application built for the ``nrf5340_dk_nrf5340_cpuappns`` board.

  **Workaround:** Use the ``nrf5340_dk_nrf5340_cpuapp`` board instead.

.. rst-class:: v1-2-0 v1-2-1

Peripheral HIDS keyboard sample cannot be used with nRF Bluetooth LE Controller
  The :ref:`peripheral_hids_keyboard` sample cannot be used with the :ref:`nrfxlib:softdevice_controller` because the NFC subsystem does not work with the controller library.
  The library uses the MPSL Clock driver, which does not provide an API for asynchronous clock operation.
  NFC requires this API to work correctly.

.. rst-class:: v1-2-0 v1-2-1

Peripheral HIDS mouse sample advertising issues
  When the :ref:`peripheral_hids_mouse` sample is used with the Zephyr Bluetooth LE Controller, directed advertising does not time out and the regular advertising cannot be started.

.. rst-class:: v1-2-0 v1-2-1

Central HIDS sample issues with directed advertising
  The :ref:`bluetooth_central_hids` sample cannot connect to a peripheral that uses directed advertising.

.. rst-class:: v1-1-0

Unstable samples
  Bluetooth Low Energy peripheral samples are unstable in some conditions (when pairing and bonding are performed and then disconnections/re-connections happen).

.. rst-class:: v1-1-0 v1-2-0 v1-2-1

:option:`CONFIG_BT_SMP` alignment requirement
  When running the :ref:`bluetooth_central_dfu_smp` sample, the :option:`CONFIG_BT_SMP` configuration must be aligned between this sample and the Zephyr counterpart (:ref:`zephyr:smp_svr_sample`).
  However, security is not enabled by default in the Zephyr sample.

.. rst-class:: v1-0-0 v1-1-0 v1-2-0 v1-2-1

Reconnection issues on some operating systems
  On some operating systems, the :ref:`nrf_desktop` application is unable to reconnect to a host.

.. rst-class:: v1-0-0 v1-1-0

:ref:`central_uart` cannot handle long strings
  A too long 212-byte string cannot be handled when entered to the console to send to :ref:`peripheral_uart`.

.. rst-class:: v1-0-0

:ref:`bluetooth_central_hids` loses UART connectivity
  After programming a HEX file to the nrf52_pca10040 board, UART connectivity is lost when using the BLE Controller.
  The board must be reset to get UART output.

.. rst-class:: v1-0-0 v1-1-0

Samples crashing on nRF51 when using GPIO
  On nRF51 devices, |BLE| samples that use GPIO might crash when buttons are pressed frequently.
  In such case, the GPIO ISR introduces latency that violates real-time requirements of the Radio ISR.
  nRF51 is more sensitive to this issue than nRF52 (faster core).

.. rst-class:: v0-4-0

GATT Discovery Manager missing support
  The :ref:`gatt_dm_readme` is not supported on nRF51 devices.

.. rst-class:: v0-4-0

Samples do not work with SD Controller v0.1.0
  |BLE| samples cannot be built with the :ref:`nrfxlib:softdevice_controller` v0.1.0.

.. rst-class:: v0-3-0 v0-4-0 v1-0-0

LED Button Service reporting issue
  :ref:`peripheral_lbs` does not report the Button 1 state correctly.

.. rst-class:: v0-3-0 v0-4-0 v1-0-0 v1-1-0 v1-2-0 v1-2-1

MITM protection missing for central samples
  The central samples (:ref:`central_uart`, :ref:`bluetooth_central_hids`) do not support any pairing methods with MITM protection.

.. rst-class:: v0-3-0

Peripheral UART string size issue
  :ref:`peripheral_uart` cannot handle the corner case that a user attempts to send a string of more than 211 bytes.

.. rst-class:: v0-3-0

Reconnection issues after bonding
  The peripheral samples (:ref:`peripheral_uart`, :ref:`peripheral_lbs`, :ref:`peripheral_hids_mouse`) have reconnection issues after performing bonding (LE Secure Connection pairing enable) with nRF Connect for Desktop.
  These issues result in disconnection.

.. rst-class:: v1.7.0 v1.6.1 v1.6.0 v1.5.1 v1.5.0 v1.4.2 v1.4.1 v1.4.0 v1.3.2 v1.3.1 v1.3.0 v1.2.1 v1.2.0 v1.1.0 v1.0.0 v0.4.0 v0.3.0

Limited advertising initiated from host API does not end after a certain time
  In order to follow the Bluetooth Specification, limited advertising cannot proceed for longer than 180 seconds.

  **Workaround:** The application must ensure that limited advertising is disabled after a certain time.

Bluetooth Mesh
==============

.. rst-class:: v1-3-0 v1-3-1

NCSDK-5580: nRF5340 only supports SoftDevice Controller
  On nRF5340, only the :ref:`nrfxlib:softdevice_controller` is supported for Bluetooth Mesh.

Bootloader
==========

.. rst-class:: v1-1-0

Public keys revocation
  Public keys are not revoked when subsequent keys are used.

.. rst-class:: v1-1-0

Incompatibility with nRF51
  The bootloader does not work properly on nRF51.

.. rst-class:: v0-3-0 v0-4-0 v1-0-0 v1-1-0 v1-2-0 v1-2-1

Immutable bootloader not supported in SES
  Building and programming the immutable bootloader (see :ref:`ug_bootloader`) is not supported in SEGGER Embedded Studio.

.. rst-class:: v0-3-0 v0-4-0 v1-0-0 v1-1-0 v1-2-0 v1-2-1

Immutable bootloader board restrictions
  The immutable bootloader can only be used with the following boards:

  * nrf52840_pca10056
  * nrf9160_pca10090

Build system
============

.. rst-class:: v1-3-0 v1-3-1

Build configuration issues
  The build configuration consisting of :ref:`bootloader`, :ref:`secure_partition_manager`, and application does not work.

  **Workaround:** Either include MCUboot in the build or use MCUboot instead of the immutable bootloader.

.. rst-class:: v1-3-0 v1-3-1

Flash commands only program one core
  ``west flash`` and ``ninja flash`` only program one core, even if multiple cores are included in the build.

  **Workaround:** Execute the flash command from inside the build directory of the child image that is placed on the other core (for example, :file:`build/hci_rpmsg`).

.. rst-class:: v1-1-0 v1-2-0 v1-2-1 v1-3-0 v1-3-1

Secure Partition Manager and application building together
  It is not possible to build and program :ref:`secure_partition_manager` and the application individually.


DFU and FOTA
============

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

.. rst-class:: v0-4-0 v1-0-0

FW upgrade is broken for multi-image builds
  Firmware upgrade using mcumgr or USB DFU is broken for multi-image builds, because the devicetree configuration is not used.
  Therefore, it is not possible to upload the image.

  **Workaround:** Build MCUboot and the application separately.

NFC
===

.. rst-class:: v1-2-0 v1-2-1

Sample incompatibility with the nRF5340 PDK
  The :ref:`nfc_tnep_poller` and :ref:`nfc_tag_reader` samples cannot be run on the nRF5340 PDK.
  There is an incorrect number of pins defined in the MDK files, and the pins required for :ref:`st25r3911b_nfc_readme` cannot be configured properly.

.. rst-class:: v1-1-0 v1-2-0 v1-2-1

Unstable NFC tag samples
  NFC tag samples are unstable when exhaustively tested (performing many repeated read and/or write operations).
  NFC tag data might be corrupted.

Secure Partition Manager (SPM)
==============================

.. rst-class:: v1-3-0 v1-3-1

NCSIDB-114: Default logging causes crash
  Enabling default logging in the :ref:`secure_partition_manager` sample makes it crash if the sample logs any data after the application has booted (for example, during a SecureFault, or in a secure service).
  At that point, RTC1 and UARTE0 are non-secure.

  **Workaround:** Do not enable logging and add a breakpoint in the fault handling, or try a different logging backend.


MCUboot
*******

.. rst-class:: v1-2-0 v1-2-1

Recovery with the USB does not work
  The MCUboot recovery feature using the USB interface does not work.

nrfxlib
*******

Crypto
======

.. rst-class:: v1-3-0 v1-3-1

NCSDK-5546: Oberon missing symbols for HKDF
  nRF Oberon v3.0.5 is missing symbols for HKDF using SHA1, which will be fixed in an upcoming version of the library.

  **Workaround:** Use a different backend (for example, vanilla mbed TLS) for HKDF/HMAC using SHA1.

.. rst-class:: v1-3-0 v1-3-1

Limited support for Nordic Security Module
  The :ref:`nrfxlib:nrf_security` is currently only fully supported on nRF52840 and nRF9160 devices.
  It gives compile errors on nRF52832, nRF52833, nRF52820, nRF52811, and nRF52810.

  **Workaround:** To fix the errors, cherry-pick commits in `nrfxlib PR #205 <https://github.com/nrfconnect/sdk-nrfxlib/pull/205>`_.

.. rst-class:: v0-4-0 v1-0-0

Glue layer symbol renaming issue
  The :ref:`nrfxlib:nrf_security` glue layer is broken because symbol renaming is not handled correctly.
  Therefore, the behavior is undefined when selecting multiple back-ends for the same algorithm (for example, AES).

GNSS sockets
============

.. rst-class:: v0-4-0 v1-0-0

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

nrfx
****

MDK
===

.. rst-class:: v1-2-0 v1-2-1

Incorrect pin definition for nRF5340
  For nRF5340, the pins **P1.12** to **P1.15** are unavailable due to an incorrect pin number definition in the MDK.

nrfx_saadc driver
=================

.. rst-class:: v0-4-0 v1-0-0 v1-1-0

Samples might be swapped
  Samples might be swapped when buffer is set after starting the sample process, when more than one channel is sampled.
  This can happen when the sample task is connected using PPI and setting buffers and sampling are not synchronized.

nrfx_uarte driver
=================

.. rst-class:: v0-4-0 v1-0-0 v1-1-0

RX and TX not disabled in uninit
  The driver does not disable RX and TX in uninit, which can cause higher power consumption.

nrfx_uart driver
================

.. rst-class:: v0-4-0 v1-0-0

tx_buffer_length set incorrectly
  The nrfx_uart driver might incorrectly set the internal tx_buffer_length variable when high optimization level is set during compilation.

Zephyr
******

.. rst-class:: v1-3-0 v1-3-1

NCSIDB-108: Thread context switch might lead to a kernel fault
  If the Zephyr kernel preempts the current thread and performs a context switch to a new thread while the current thread is executing a secure service, the behavior is undefined and might lead to a kernel fault.
  To prevent this situation, a thread that aims to call a secure service must temporarily lock the kernel scheduler (:cpp:func:`k_sched_lock`) and unlock the scheduler (:cpp:func:`k_sched_unlock`) after returning from the secure call.

.. rst-class:: v1-0-0

Counter Alarm sample does not work
  The :ref:`zephyr:alarm_sample` does not work.
  A fix can be found in `Pull Request #16736 <https://github.com/zephyrproject-rtos/zephyr/pull/16736>`_.

.. rst-class:: v1-0-0

USB Mass Storage Sample Application compilation issues
  :ref:`zephyr:usb_mass` does not compile.

In addition to these known issues, check the current issues in the `official Zephyr repository`_, since these might apply to the |NCS| fork of the Zephyr repository as well.
To get help and report issues that are not related to Zephyr but to the |NCS|, go to Nordic's `DevZone`_.
