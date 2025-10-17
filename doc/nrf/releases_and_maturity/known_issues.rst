.. _known_issues:

Known issues
############

.. contents::
   :local:
   :depth: 3

Known issues listed on this page *and* tagged with the :ref:`latest official release version <release_notes>` are valid for the current state of development.
Use the drop-down filter to see known issues for previous releases and check if they are still valid.

A known issue can list one or both of the following entries:

* **Affected platforms:**

  If a known issue does not have any specific platforms listed, it is valid for all hardware platforms.

* **Workaround:**

  Some known issues have a workaround.
  Sometimes, they are discovered later and added over time.

.. version-filter::
  :default: v3-1-1
  :container: dl/dt
  :tags: [("wontfix", "Won't fix")]

.. page-filter::
  :name: issues

  wontfix    Won't fix

.. HOWTO

   When adding a new version, set it as the default value of the version-filter directive.
   Once the version is updated, only issues that are valid for the new version will be displayed when entering the page.

   When updating this file, add entries in the following format:

   .. rst-class:: wontfix vXXX vYYY

   JIRA-XXXX: Title of the issue (with mandatory JIRA issue number since nRF Connect SDK v1.7.0)
     Description of the issue.
     Start every sentence on a new line and pay attention to indentations.

     There can be several paragraphs, but they must be indented correctly.

     **Affected platforms:** Write what hardware platform is affected by this issue.
     If an issue touches all hardware platforms, this line is not needed.

     **Workaround:** The last paragraph contains the workaround.
     The workaround is optional.

Protocols
*********

The issues in this section are related to :ref:`protocols`.

Amazon Sidewalk
===============

After the |NCS| release v2.9.1, the Amazon Sidewalk protocol is no longer a part of the |NCS|.
For current known issues and release information, see the `Amazon Sidewalk documentation`_.

.. rst-class:: v2-9-0-nRF54H20-1 v2-9-2 v2-9-1 v2-9-0 v2-8-0 v2-7-0 v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0 v2-5-3 v2-5-2 v2-5-1 v2-5-0

KRKNWK-20330: The Amazon Sidewalk application crashes on startup when the :kconfig:option:`CONFIG_NANOPB` Kconfig option is enabled
  The Amazon Sidewalk libraries include a nanopb implementation that conflicts with the one in Zephyr, causing the application to crash immediately after startup.

.. rst-class:: v2-8-0

KRKNWK-19647: nRF52 has high power consumption in sleep mode for sub-GHz sample
  nRF52840 device experiences high current consumption in sleep mode with Bluetooth® LE link when using sub-GHz Amazon libraries.
  This issue does not occur with Bluetooth LE-only configurations.

  **Affected platforms:** nRF52840

.. rst-class:: v2-8-0

KRKNWK-19647: nRF53 has high power consumption in sleep mode
  The nRF5340 device has high current consumption in sleep mode across all link configurations.

  **Affected platforms:** nRF5340

.. rst-class:: v2-9-0-nRF54H20-1 v2-9-2 v2-9-1 v2-9-0 v2-8-0

KRKNWK-19459: AES crypt error occurs after a factory reset
  In the Sidewalk DUT application, authentication fails when starting FSK transport and performing a factory reset.

  **Workaround:** After factory reset, perform a power reset.

.. rst-class:: v2-8-0

KRKNWK-19573: Trusted storage for Sidewalk keys fails when settings backend changes
  When migrating from NVS to ZMS, the settings storage backend changes, resulting in the loss of trusted keys.
  Subsequently, the device becomes inoperable (bricked) after performing DFU with those settings.

  **Workaround:** Do not change settings backend when using trusted storage.

.. rst-class:: v2-8-0

KRKNWK-18776: DFU swap takes too long when running a simple Hello World application
  After firmware upload, the device resets to MCUboot and takes several minutes to switch to the new image.
  You must wait for the device to complete this operation.

  **Affected platforms:** nRF54L15 DK

.. rst-class:: v2-8-0

KRKNWK-19545: High initial power consumption by Semtech radio
  The device experiences high initial current consumption caused by Semtech radio before initialization.

  **Workaround:** Initialize Sidewalk over LoRa or FSK to start Semtech radio.

.. rst-class:: v2-8-0 v2-7-0

KRKNWK-17860: Fatal DFU error in the Sidewalk application mode
  A fatal error occurs when attempting to perform DFU in the Sidewalk application mode instead of the DFU mode.
  The error involves an assertion in the QSPI, which is in the deep sleep mode to save energy.

  **Workaround:** The DFU service must be used only when the device is in the DFU mode.

.. rst-class:: v2-7-0 v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0

KRKNWK-18465: BUS fault on FSK during the FACTORY_RESET call
  The *Bluetooth* LE link is deinitialized.
  However, a race condition might occur where an event added by the Bluetooth LE link is not removed.
  Acting on the event without a valid Bluetooth LE handle leads to dereference of ``NULL``.
  To avoid the error, you need to check for ``NULL`` before the event is processed in the event queue.
  The issue requires a fix in the Sidewalk stack.

  **Workaround:** Perform a factory reset as the error is recoverable.

.. rst-class:: v2-7-0 v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0

KRKNWK-18511: Advertising fails to start (``err -12``) upon registration
  The Sidewalk end device is trying to restart advertising, for example, on time sync lost.
  This happens even when the device is connected through Bluetooth LE, leading to an error.
  The Bluetooth LE connection management requires a fix in the Sidewalk stack.

  **Workaround:** The issue fixes upon automatic restart of the Bluetooth LE advertising.

.. rst-class:: v2-5-3 v2-5-2 v2-5-1 v2-5-0

KRKNWK-17860: QSPI assert occurs when performing DFU in the Sidewalk application mode
  The DFU must be performed only in the DFU mode.
  The DFU Bluetooth service can be used in the Sidewalk mode, however, using it leads to assertion failure, resulting in a Zephyr fatal error.

.. rst-class:: v2-5-3 v2-5-2 v2-5-1 v2-5-0

KRKNWK-17800: After reconnecting to the network, the end device cannot find the route to its host
  After the device disconnects from Sidewalk servers, the sensor monitoring app over Bluetooth LE shows an error ``-38 (SID_ERROR_NO_ROUTE_AVAILABLE)``.

  **Workaround:** The device needs to be reset manually.

.. rst-class:: v2-7-0 v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0 v2-5-3 v2-5-2 v2-5-1 v2-5-0

KRKNWK-17750: Error occurs when sending multiple messages in a short period of time
  When sending multiple Sidewalk messages in a short period of time, the internal queues might become full, showing misleading error messages, such as ``-12 (SID_ERROR_INCOMPATIBLE_PARAMS)``.

  **Workaround:** The message must be resent after the protocol empties the queues.

.. rst-class:: v2-5-3 v2-5-2 v2-5-1 v2-5-0

KRKNWK-17244: CMake warnings when building the Sidewalk samples
  CMake warnings (``No SOURCES given to Zephyr library``) show up in the build log of a Sidewalk application.
  The application builds successfully, but the error might obfuscate other important warnings.

.. rst-class:: v2-5-3 v2-5-2 v2-5-1 v2-5-0

KRKNWK-17374: Sporadic Zephyr fatal error after disconnecting on FSK
  After disconnecting on FSK, Zephyr fatal error occurs due to assertion in the semaphore module.
  The error reproduces rarely (once per a few days).
  The device resets automatically in the release mode, however, in the debug mode it needs to be reset manually.
  Currently, this issue occurs for Sidewalk v1.14 libraries, and it will be fixed in a future version.

.. rst-class:: v2-5-3 v2-5-2 v2-5-1 v2-5-0

KRKNWK-17035: Sensor monitor uplink messages are lost when the notification period is longer than 30 seconds
  If the notification period is set to longer than 30 seconds, sensor monitor uplink messages are lost.

  **Workaround:** The notification period is set to 15 seconds by default.

.. rst-class:: v2-5-3 v2-5-2 v2-5-1 v2-5-0

KRKNWK-14583: Bus fault after flash, before the :file:`Nordic_MFG.hex` data flash
  For sub-GHz samples, when the :file:`Nordic_MFG.hex` file is missing, the device throws a hard fault during initializing the Sidewalk stack.
  Proper error handling will be implemented, but the temporary solution is to write a manufacturing hexadecimal code to the device and reset it.

.. rst-class:: v2-7-0 v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0 v2-5-3 v2-5-2 v2-5-1 v2-5-0

KRKNWK-14299: NRPA MAC address cannot be set in Zephyr
  The non-resolvable private address (NRPA) cannot be set in the connectable mode for Bluetooth LE.
  Currently, there is no workaround for this issue.

Bluetooth LE
============

.. rst-class: v3-0-2 v3-0-1 v3-0-0 v2-9-0-nRF54H20-1 v2-9-2 v2-9-1 v2-9-0 v2-8-0 v2-7-0

NCSDK-33631: Preemptible ``bt_gatt_notify`` assert
  Calling the :c:func:`bt_gatt_notify` function from a preemptible thread can lead to an assertion.

  **Workaround:** Call the :c:func:`bt_gatt_notify` function from a cooperative thread or call the :c:func:`k_sched_lock` function before and :c:func:`k_sched_unlock` after calling :c:func:`bt_gatt_notify`.

.. rst-class:: v3-0-2 v3-0-1 v3-0-0 v2-9-0-nRF54H20-1 v2-9-2 v2-9-1 v2-9-0 v2-8-0

NCSDK-31457: Implementation of L2CAP PDU recombination through ACL in pool
  Due to L2CAP recombination implementation through ACL in pool, it is possible that Controller sends more ACL data towards Host than Host has buffers in the ACL in pool.
  When the Host and Controller run on the same core, a deadlock can occur.
  When the Host and Controller run on separate cores, the ``<err> bt_hci_driver: No available ACL buffers!`` error might be printed.

  **Workaround:** Increase the value of the :kconfig:option:`CONFIG_BT_BUF_ACL_RX_COUNT_EXTRA` Kconfig option.
  This workaround is only applicable for the |NCS| v3.0.2, v3.0.1 and v3.0.0 releases.

.. rst-class:: v3-1-1 v3-1-0 v3-0-2 v3-0-1 v3-0-0 v2-9-2 v2-9-1 v2-9-0-nRF54H20-1 v2-9-0 v2-8-0 v2-7-99-cs2 v2-7-99-cs1 v2-7-0 v2-6-4 v2-6-3 v2-6-2 v2-6-99-cs2 v2-6-99-cs1 v2-6-1 v2-6-0 v2-5-3 v2-5-2 v2-5-1 v2-5-0 v2-4-4 v2-4-3 v2-4-2 v2-4-1 v2-4-0 v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2-dev1 v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0 v1-0-0

NCSDK-31487: Improper reuse of dynamic GATT service handles
  The Host reuses handles when registering dynamic services even while there are connected clients.
  This violates the Bluetooth Core Specification and creates a theoretical risk that a connected ATT client may send requests intended for an old service, which could be misinterpreted as requests for a new service.
  The issue occurs when unregistering and then registering services using the :c:func:`bt_gatt_service_unregister` and :c:func:`bt_gatt_service_register` functions while clients remain connected.

  **Workaround:** For a simple approach, register new services only while there are no connections or connectable advertisers.
  For a more flexible approach, you may register services while connected, but after unregistering any service, wait until all connections are gone and there are no connectable advertisers before registering new services.
  Alternatively, manually allocate handles by setting the :c:member:`bt_gatt_attr.handle` field in each :c:type:`bt_gatt_attr` before calling the :c:func:`bt_gatt_service_register` function to ensure handles are not reused while ATT bearers exist.

.. rst-class:: v3-1-1 v3-1-0 v3-0-2 v3-0-1 v3-0-0 v2-9-0-nRF54H20-1 v2-9-2 v2-9-1 v2-9-0 v2-8-0

NCSDK-31528: Deadlock on system workqueue with ``tx_notify`` in host
  If the :kconfig:option:`CONFIG_BT_HCI_ACL_FLOW_CONTROL` Kconfig option is disabled, blocking of the system workqueue can cause a deadlock in the Bluetooth Host when running out of buffers in the HCI commands pool.
  The Bluetooth Host uses the system workqueue to complete processing of transmitted ACL data.
  Thus, using any blocking API on the system workqueue blocks the Bluetooth Host.
  For the deadlock to occur, the following must happen simultaneously:

  * The :kconfig:option:`CONFIG_BT_HCI_ACL_FLOW_CONTROL` Kconfig option is disabled.
  * The system workqueue is blocked.
  * The HCI commands pool is empty.
  * A blocking Bluetooth Host API that uses the :c:func:`bt_hci_cmd_send_sync` function is called from any thread (including the system workqueue).

  An example of blocking the system workqueue is calling the :c:func:`bt_conn_get_tx_power_level` function in a receive callback (called when data is received over the connection).
  Calling such a function can result in a deadlock, since it uses the :c:func:`bt_hci_cmd_send_sync` function to complete its operation.

  **Workaround:** Do not block the system workqueue.
  Alternatively, increase the value of the :kconfig:option:`CONFIG_BT_BUF_CMD_TX_COUNT` Kconfig option to increase the HCI commands pool.
  This does not guarantee that the problem is solved, as multiple blocking calls might exhaust the buffer pool.
  You can also use the (experimental) :kconfig:option:`CONFIG_BT_CONN_TX_NOTIFY_WQ` Kconfig option to use a separate workqueue for connection TX notify processing.

.. rst-class:: v3-0-2 v3-0-1 v3-0-0 v2-9-0-nRF54H20-1 v2-9-2 v2-9-1 v2-9-0 v2-8-0 v2-7-0 v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0

NCSDK-32999: The :zephyr:code-sample:`bluetooth_hci_ipc` sample and :ref:`ipc_radio` application do not support stream flow control and drop bytes
  With the deferred disconnection complete generation added in the |NCS| v2.6.0, there are cases where the :kconfig:option:`CONFIG_BT_HCI_ACL_FLOW_CONTROL` Kconfig option has value ``y``, and the value of :kconfig:option:`CONFIG_BT_BUF_CMD_TX_COUNT` is smaller than :kconfig:option:`CONFIG_BT_BUF_ACL_RX_COUNT` + 1.
  This might result in dropped HCI command packets.
  The visible symptom is a missing disconnect event and subsequent failure in connection, if a peripheral device falls out of range or is powered off.

  **Workaround:** Make sure that the value of the :kconfig:option:`CONFIG_BT_BUF_ACL_RX_COUNT` Kconfig option is smaller than the one for :kconfig:option:`CONFIG_BT_BUF_CMD_TX_COUNT` in your project.

.. rst-class:: v2-9-0-nRF54H20-1 v2-9-2 v2-9-1 v2-9-0 v2-8-0 v2-7-0 v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0

NCSDK-31095: Issues with the :kconfig:option:`CONFIG_SEGGER_SYSVIEW` Kconfig option
  Using this Kconfig option causes the data parameter in the macros :c:macro:`k_fifo_put`, :c:macro:`k_fifo_alloc_put`, :c:macro:`k_lifo_put`, and :c:macro:`k_lifo_alloc_put` to be evaluated multiple times.
  This can cause problems if the data parameter is a function call incrementing a reference counter.

  **Workaround:** Manually cherry-pick and apply the commit with the fix to ``sdk-zephyr`` (commit hash: ``f2f61094b5e1ba5b841d78e5dd88b2076fbc99ee`` from the `upstream Zephyr repository <https://github.com/zephyrproject-rtos/zephyr>`_).


.. rst-class:: v2-9-0-nRF54H20-1 v2-9-2 v2-9-1 v2-9-0 v2-8-0

DRGN-23363: A flash operation executed on the system workqueue might result in ``-ETIMEDOUT``
  A flash operation executed on the system workqueue might result in ``-ETIMEDOUT`` if there is an active Bluetooth LE connection.

  **Workaround:** Move flash operations to a different thread.

.. rst-class:: v2-9-2 v2-9-1 v2-9-0 v2-8-0 v2-7-0

NCSDK-30458: The Bluetooth LE applications built with the ``nordic-bt-rpc`` snippet do not work on nRF54H20 devices
  When using the ``nordic-bt-rpc`` sysbuild snippet for building Bluetooth LE applications for an nRF54H20 device in the Bluetooth RPC configuration, the build system creates an invalid memory map for the application core, resulting in a runtime failure.

  **Affected platforms:** nRF54H20

  **Workaround:** Add the ``status = "disabled";`` line in the ``cpurad-rw-partitions node`` definition in the :file:`snippets/nordic-bt-rpc/boards/nrf54h20dk_nrf54h20-mem-map-move.dtsi` file.

.. rst-class:: v2-7-0 v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0 v2-5-3 v2-5-2 v2-4-4 v2-4-3

DRGN-23231: The Bluetooth subsystem might sometimes deadlock when a Bluetooth link is lost during data transfer
  When this happens, the disconnected event is never delivered to the application.
  The issue occurs when the :kconfig:option:`CONFIG_BT_HCI_ACL_FLOW_CONTROL` Kconfig option is enabled.
  This option is enabled by default on the nRF5340 DK.

  **Affected platforms:** nRF5340

.. rst-class:: v2-9-0-nRF54H20-1 v2-9-2 v2-9-1 v2-9-0 v2-8-0 v2-7-0

NCSDK-28239: On nRF54H20, a Bluetooth LE disconnect event is not generated once the central device (mobile phone) moves out of range while data is being transferred (during DFU)
  This will prevent the device from establishing new BLE connections.

  **Affected platforms:** nRF54H20

  **Workaround:** Restart the device.

.. _ncsdk_19865:

.. rst-class:: v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0 v1-0-0 v0-4-0 v0-3-0

NCSDK-19865: GATT Robust Caching issues for bonded peers
  The Client Supported Features value written by a bonded peer might not be stored in the non-volatile memory.
  Change awareness of the bonded peer is lost on reboot.
  After reboot, each bonded peer is initially marked as change-unaware.

  **Workaround:** Disable the GATT Caching feature (:kconfig:option:`CONFIG_BT_GATT_CACHING`).
  Make sure that Bluetooth bonds are removed together with disabling GATT Caching if the functionality is disabled during a firmware upgrade.

.. rst-class:: v2-0-2

DRGN-17695: The BT RX thread stack might overflow if the :kconfig:option:`CONFIG_BT_SMP` is enabled
  When performing SMP pairing MPU FAULTs might be triggered because the stack is not large enough.

  **Workaround:** Increase the stack size manually in the project configuration file (:file:`prj.conf`) using :kconfig:option:`CONFIG_BT_RX_STACK_SIZE`.

.. rst-class:: v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0 v1-0-0

NCSDK-13459: Uninitialized size in hids_boot_kb_outp_report_read
  When reading from the boot keyboard output report characteristic, the :ref:`hids_readme` calls the registered callback with uninitialized report size.

  **Workaround:** Manually cherry-pick and apply commit with fix from main (commit hash: ``f18250dad6cbd9778de7af4b8a774b58e55fe720``).

.. rst-class:: v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0 v1-0-0

NCSDK-9106: Bluetooth ECC thread stack size too small
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

Only secure applications can use Bluetooth LE
  Bluetooth LE cannot be used in a non-secure application, for example, an application built for the ``nrf5340_dk_nrf5340_cpuappns`` board target.

  **Affected platforms:** nRF5340

  **Workaround:** Use the ``nrf5340_dk_nrf5340_cpuapp`` board target instead.

.. rst-class:: v1-2-1 v1-2-0 v1-1-0

:kconfig:option:`CONFIG_BT_SMP` alignment requirement
  When running the :ref:`bluetooth_central_dfu_smp` sample, the :kconfig:option:`CONFIG_BT_SMP` configuration must be aligned between this sample and the Zephyr counterpart (:zephyr:code-sample:`smp-svr`).
  However, security is not enabled by default in the Zephyr sample.

.. rst-class:: v2-5-2

DRGN-21390: The ``disconnected`` callback might not get called on nRF5340
  The Bluetooth host running on the nRF5340 application core might deadlock on disconnection.
  This is due to a recent bugfix in the SoftDevice Controller triggering a bug in the ATT queuing layer.

  **Workaround:** Either disable host flow control (:kconfig:option:`CONFIG_BT_HCI_ACL_FLOW_CONTROL`) or cherry-pick commits from the upstream: `Zephyr PR #65272 <https://github.com/zephyrproject-rtos/zephyr/pull/65272>`_.

.. rst-class:: v2-7-0 v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0 v2-5-3 v2-5-2 v2-5-1 v2-5-0 v2-4-4 v2-4-3 v2-4-2 v2-4-1 v2-4-0 v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-1 v1-9-0 v1-8-0 v1-7-1

DRGN-23518: ACL reassembly might deadlock
  This can happen if  the value of :kconfig:option:`CONFIG_BT_BUF_ACL_RX_COUNT` Kconfig option is less than or equal to the value of :kconfig:option:`CONFIG_BT_MAX_CONN`.

  **Workaround:** Ensure that the value of :kconfig:option:`CONFIG_BT_BUF_ACL_RX_COUNT` is greater than the value of :kconfig:option:`CONFIG_BT_MAX_CONN`.

.. rst-class:: v2-9-0-nRF54H20-1 v2-9-2 v2-9-1 v2-9-0 v2-8-0

DRGN-23511: Building of multilink Bluetooth applications fails
  This happens when the :kconfig:option:`CONFIG_BT_BUF_ACL_RX_COUNT` Kconfig option is not explicitly set.

  **Workaround:** Set the Kconfig value explicitly.

.. rst-class:: v2-9-0-nRF54H20-1 v2-9-2 v2-9-1 v2-9-0 v2-8-0

NCSDK-29354: Bluetooth traffic stalls while writing or erasing flash
  Using system workqueue for internal flash operations while Bluetooth is in use could result in Bluetooth hang or flash operation failures (timeout in MPSL flash synchronization).
  This happens because Bluetooth RX context waits for the connection TX notify that is done in the system workqueue context.

  **Affected platforms:** nRF52 Series, nRF54L15

  **Workaround:** Use a separate workqueue for connection TX notify processing (:kconfig:option:`CONFIG_BT_CONN_TX_NOTIFY_WQ`).

.. rst-class:: v3-1-1 v3-1-0 v3-0-2 v3-0-1 v3-0-0 v2-9-0-nRF54H20-1 v2-9-2 v2-9-1 v2-9-0 v2-8-0 v2-7-0

NCSDK-30959: The Bluetooth subsystem might deadlock when :kconfig:option:`CONFIG_BT_HCI_ACL_FLOW_CONTROL` is disabled
  When the :kconfig:option:`CONFIG_BT_HCI_ACL_FLOW_CONTROL` Kconfig option is disabled and your application blocks any of the Bluetooth Host callbacks, the Bluetooth subsystem might deadlock.

  **Workaround:** Enable the :kconfig:option:`CONFIG_BT_HCI_ACL_FLOW_CONTROL` Kconfig option or do not call the blocking API from any of the Bluetooth Host callbacks.

.. rst-class:: v3-1-1 v3-1-0 v3-0-2 v3-0-1 v3-0-0 v2-9-2 v2-9-1 v2-9-0-nRF54H20-1 v2-9-0 v2-8-0 v2-7-99-cs2 v2-7-99-cs1 v2-7-0

NCSDK-35430: Assertion can happen in the Bluetooth Host if an application is trying to send a synchronous command while the system workqueue is blocked
  If the Bluetooth Host or an application blocks the system workqueue, the Bluetooth Host TX processor cannot send commands or data towards the Controller.
  If this happens when a separate thread tries to send a synchronous command, an assertion can occur.
  This assertion is highly possible if the application is trying to send a synchronous command from the :c:member:`bt_conn_cb.connected` callback.

  **Workaround:** Increase the value of the :kconfig:option:`CONFIG_BT_BUF_CMD_TX_COUNT` Kconfig option.

Bluetooth Mesh
==============

The issues in this section are related to the :ref:`ug_bt_mesh` protocol.

.. rst-class:: v2-9-0-nRF54H20-1 v2-9-1 v2-9-0

NCSDK-31305: Assert in ``bt_mesh_adv_unref()`` when sending two messages in a row to a proxy client
  The ``bt_mesh_adv_unref()`` function could assert when messaging to a proxy node.

.. rst-class:: v3-0-2 v3-0-1 v3-0-0 v2-9-0-nRF54H20-1 v2-9-2 v2-9-1 v2-9-0

NCSDK-31962: Advertiser buffer leak on Suspend-Resume
  Calling the :c:func:`bt_mesh_adv_disable` function does not unreference advertising buffers that were previously allocated.
  Over multiple suspend-resume cycles, this can lead to buffer leaks and eventually exhaust the advertising buffer pool.

.. rst-class:: v3-1-1 v3-1-0 v3-0-2 v3-0-1 v3-0-0 v2-9-0-nRF54H20-1 v2-9-2 v2-9-1 v2-9-0

NCSDK-32928: Erasing settings partition without unprovisioning mesh breaks subsequent provisioning on TF-M enabled platforms
  When running Bluetooth Mesh on platforms with TF-M support (for example, ``nrf5340dk/nrf5340/cpuapp/ns``), erasing the settings partition without correctly unprovisioning Bluetooth Mesh will break subsequent provisionings.

  **Workaround:** Unprovision Bluetooth Mesh before erasing the settings partition.

.. rst-class:: v2-9-0-nRF54H20-1 v2-9-2 v2-9-1 v2-9-0 v2-8-0 v2-7-0 v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0 v2-5-3 v2-5-2 v2-5-1 v2-5-0 v2-4-4 v2-4-3 v2-4-2 v2-4-1 v2-4-0 v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0

NCSDK-30033: :ref:`bt_mesh_light_ctrl_srv_readme` never resumes the Lightness Controller automatically after it is disabled by a scene recall
  This occurs when the Lightness Controller is running and a scene recall for a scene with the Lightness Controller disabled is received.
  Recalling this scene causes the Lightness Controller to stop running.
  Unlike when setting the Lightness state through other messages, the Lightness Controller will not automatically resume after the time set in the :kconfig:option:`CONFIG_BT_MESH_LIGHT_CTRL_SRV_RESUME_DELAY` Kconfig option.

  **Workaround:** Explicitly re-enable the Lightness Controller using a Lightness Controller client to resume it after it is disabled by a scene recall.

.. rst-class:: v2-8-0 v2-7-0

NCSDK-29893: Adding dynamic names to devices does not work
  The advertiser options to automatically add the name in the scan response and the advertising data is removed.
  The Mesh files :file:`proxy_srv.c` and :file:`pb_gatt_srv.c` were using ``BT_LE_ADV_OPT_USE_NAME`` that has been removed.

.. rst-class:: v3-1-1 v3-1-0 v3-0-2 v3-0-1 v3-0-0 v2-9-0-nRF54H20-1 v2-9-2 v2-9-1 v2-9-0 v2-8-0 v2-7-0 v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0 v2-5-3 v2-5-2 v2-5-1 v2-5-0

NCSDK-28363: Local composition hash generator only considers the first portion of the Composition Data Page 0 (CDP0) when the size of CDP0 exceeds ``BT_TX_SDU_MAX``
  This affects applications that use the Large Composition Data server together with the default DFU Metadata format enabled using the :kconfig:option:`CONFIG_BT_MESH_DFU_METADATA` Kconfig option.
  Using the hash computed by calling the :c:func:`bt_mesh_dfu_metadata_comp_hash_local_get` function to compare metadata will wrongly conclude that two pages are equal if the difference between the pages is beyond the first ``BT_MESH_TX_SDU_MAX`` bytes of the page.

  **Workaround:** Make sure that, for applications with composition data longer than ``BT_TX_SDU_MAX``, any change to composition data as part of a DFU includes at least one change in the first ``BT_TX_SDU_MAX`` bytes of CDP0.
  Alternatively, create your own metadata scheme other than the default DFU Metadata format for such applications.

.. rst-class:: v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0 v2-5-3 v2-5-2 v2-5-1 v2-5-0

NCSDK-28052: Large Composition Data Server and Private Beacon Server model do not extend Configuration Server model
  The :ref:`bluetooth_mesh_models_priv_beacon_srv` model and :ref:`bluetooth_mesh_lcd_srv` model do not extend :ref:`bluetooth_mesh_models_cfg_srv` model.
  As a result, Composition Data Page 1 will have incorrect model relationship representations for these models.

.. rst-class:: v2-5-3 v2-5-2 v2-5-1 v2-5-0 v2-4-4 v2-4-3 v2-4-2 v2-4-1 v2-4-0 v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1

NCSDK-21625: Advertisements of Bluetooth Mesh GATT services are not stopped by :c:func:`bt_mesh_suspend` and not resumed by :c:func:`bt_mesh_resume`
  Functions :c:func:`bt_mesh_suspend` and :c:func:`bt_mesh_resume` do not work together with functions :c:func:`bt_disable` and :c:func:`bt_enable`.

  **Workaround:** To disable node identity advertisement, use ``bt_mesh_subnet_node_id_set`` instead.

.. rst-class:: v2-5-3 v2-5-2 v2-5-1 v2-5-0 v2-4-4 v2-4-3 v2-4-2 v2-4-1 v2-4-0 v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1

NCSDK-23087: Unsolicited Time Status messages rewrite periodic publishing TTL to zero forever
  The Time models specification mandates publishing unsolicited Time Status messages with TTL field value set to ``0``.
  The implementation rewrites the TTL field to ``0``, but does not write the initial value back, resulting in losing the initial value.

  **Workaround:** Configure the initial TTL value after an unsolicited Time Status message is sent.

.. rst-class:: v2-5-3 v2-5-2 v2-5-1 v2-5-0 v2-4-4 v2-4-3 v2-4-2 v2-4-1 v2-4-0 v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1

NCSDK-23220: The Heartbeat Publication Status message might be malformed after provisioning
  After provisioning and obtaining the Composition Data, reading the Heartbeat Publication and the Heartbeat Publication Status will contain garbage in the NetKeyIndex field.
  The reason for this is that the field was not initially cleared.

.. rst-class:: v2-5-3 v2-5-2 v2-5-1 v2-5-0

NCSDK-23308: Setting storage causes the device to reboot in the event of a clean operation
  For non-secure builds, whenever a flash erase while setting storage happens, it causes the device to reboot.
  The same issue can be seen where stored model data is large enough and changes cause defragging or cleaning.
  The EMDS partition is not affected by this.

.. rst-class:: v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-1 v1-9-0 v1-8-0 v1-7-1

NCSDK-16800: RPL is not cleared on IV index recovery
  After recovering the IV index, a node does not clear the replay protection list, which leads to incorrect triggering of the replay attack protection mechanism.

  **Workaround:** Call ``bt_mesh_rpl_reset ()`` twice after the IV index recovery is done.

.. rst-class:: v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-1 v1-9-0 v1-8-0 v1-7-1

NCSDK-16798: Friend Subscription List might have duplicate entries
  If a Low Power node loses a Friend Subscription List Add Confirm message, it repeats the request.
  The Friend does not check both the transaction number and the presence of the addresses in the subscription list.
  This causes a situation where the Friend fills the subscription list with duplicate addresses.

.. rst-class:: v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0

NCSDK-16782: The extended advertiser might not work with Bluetooth Mesh
  Using the extended advertiser instead of the legacy advertiser can lead to getting composition data while provisioning to fail.
  This problem might manifest in the sample :ref:`bluetooth_ble_peripheral_lbs_coex`, as it is using the extended advertiser.

.. rst-class:: v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-1 v1-9-0

NCSDK-16579: Advertising Node Identity and Network ID might not work with the extended advertiser
  Advertising Node Identity and Network ID do not work with the extended advertising API when the :kconfig:option:`CONFIG_BT_MESH_ADV_EXT_GATT_SEPARATE` option is enabled.

  **Workaround:** Do not enable the :kconfig:option:`CONFIG_BT_MESH_ADV_EXT_GATT_SEPARATE` option.

.. rst-class:: v2-5-3 v2-5-2 v2-5-1 v2-5-0 v2-4-4 v2-4-3 v2-4-2 v2-4-1 v2-4-0 v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-1 v1-9-0 v1-8-0 v1-7-1

NCSDK-21780: Sensor types with floating point representation lose precision
  Sensor types with floating point representation lose precision when converted to ``sensor_value`` in the sensor API callbacks.

.. rst-class:: wontfix v3-1-1 v3-1-0 v3-0-2 v3-0-1 v3-0-0 v2-9-0-nRF54H20-1 v2-9-2 v2-9-1 v2-9-0 v2-8-0 v2-7-0 v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0 v2-5-3 v2-5-2 v2-5-1 v2-5-0 v2-4-4 v2-4-3 v2-4-2 v2-4-1 v2-4-0 v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-1 v1-9-0 v1-8-0 v1-7-1

NCSDK-14399: Legacy advertiser can occasionally do more message retransmissions than requested
  When using the legacy advertiser, the stack sleeps for at least 50 ms after starting advertising a message, which might result in more messages to be advertised than requested.

.. rst-class:: v2-0-2 v2-0-1 v2-0-0 v1-9-1 v1-9-0 v1-8-0 v1-7-1

NCSDK-16061: IV update procedure fails on the device
  Bluetooth Mesh device does not undergo IV update and fails to participate in the procedure initiated by any other node unless it is rebooted after the provisioning.

  **Workaround:** Reboot the device after provisioning.

.. rst-class:: v1-6-1 v1-6-0

NCSDK-10200: The device stops sending Secure Network Beacons after re-provisioning
  Bluetooth Mesh stops sending Secure Network Beacons if the device is re-provisioned after reset through Config Node Reset message or ``bt_mesh_reset()`` call.

  **Workaround:** Reboot the device after re-provisioning.

.. rst-class:: v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0

NCSDK-5580: nRF5340 only supports SoftDevice Controller
  On nRF5340, only the :ref:`nrfxlib:softdevice_controller` is supported for Bluetooth Mesh.

  **Affected platforms:** nRF5340

Enhanced ShockBurst (ESB)
=========================

The issues in this section are related to the :ref:`ug_esb` protocol.

.. rst-class:: v2-9-0

NCSDK-30802: Packet retransmission is not working properly in the ESB for the nRF54 Series devices
  The device suddenly stops transmitting ESB packets.

  **Workaround:** Trigger the ``NRF_TIMER_TASK_CLEAR`` task after ``NRF_TIMER_TASK_STOP``.

.. rst-class:: v2-3-0 v2-2-0

NCSDK-20092: ESB does not send packet longer than 63 bytes
  ESB does not support sending packets longer than 63 bytes, but has no such hardware limitation.

Matter
======

The issues in this section are related to the :ref:`ug_matter` protocol.

.. rst-class:: v3-1-1

KRKNWK-20774: Fatal error on the nRF54LM20 SoC after enabling the :kconfig:option:`CONFIG_PICOLIBC` Kconfig option or dynamic heap memory allocation
  The nRF54LM20 SoC crashes after enabling the :kconfig:option:`CONFIG_PICOLIBC` Kconfig option or dynamic heap memory allocation while running a Matter sample.
  The root cause is that the nRF54LM20 DK devicetree overlay in the Matter samples configures the SRAM memory incorrectly.
  As a result, a restricted region of the SRAM is used, which leads to a crash.

  **Affected platforms:** nRF54LM20

  **Workaround:** Remove the following configuration from the :file:`boards/nrf54lm20dk_nrf54lm20a_cpuapp.overlay`, :file:`boards/nrf54lm20dk_nrf54lm20a_cpuapp_internal.overlay`, :file:`sysbuild/mcuboot/boards/nrf54lm20dk_nrf54lm20a_cpuapp.conf`, and :file:`sysbuild/mcuboot/boards/nrf54lm20dk_nrf54lm20a_cpuapp_internal.conf` files of the Matter sample:

    .. code-block:: console

      /* Restore full RRAM and SRAM space. By default, some parts are dedicated to FLRP. */
      &cpuapp_rram {
        reg = <0x0 DT_SIZE_K(2036)>;
      };

      &cpuapp_sram {
        reg = <0x20000000 DT_SIZE_K(512)>;
        ranges = <0x0 0x20000000 0x80000>;
      };

.. rst-class:: v3-1-0 v3-0-2 v3-0-1 v3-0-0

KRKNWK-20815: The NordicDevKit cluster does not work with the 0.1.0 version of the `Matter Cluster Editor app`_
  The ``NordicDevKit`` cluster from the :ref:`matter_manufacturer_specific_sample` is not supported in the version 0.1.0 of the `Matter Cluster Editor app`_.
  When the cluster XML file is loaded in the tool, saving the file may result in some fields not being saved correctly, causing them to become unavailable.

  **Workaround:** Upgrade to the latest version of the `Matter Cluster Editor app`_.

.. rst-class:: v3-1-0 v3-0-2 v3-0-1 v3-0-0 v2-9-0-nRF54H20-1 v2-9-2 v2-9-1 v2-9-0 v2-8-0 v2-7-0 v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0

KRKNWK-20445: PSA crypto implementation does not properly revert NOC keys when failsafe occurs
  If a Matter controller attempts to commission a device to the same Matter fabric to which it is already commissioned, and the failsafe procedure occurs (for example, some error occurs during the commissioning process), the new NOC key is discarded, but the old NOC key is not restored.
  This issue is caused by a bug where the same PSA key ID is used for both the active and pending NOC keys.
  When the pending key is being destroyed, the active key is also destroyed.
  The TC-OPCREDS-3.8 certification test fails on the |NCS| release v3.1.0.

  **Workaround:** Manually cherry-pick and apply the commit with the fix to ``sdk-connectedhomeip`` (commit hash: ``fe650a3ee4948ef1a2edd55a7fe4f6eb561c9e64``).
                  This fix can be applied only to the |NCS| release v3.1.0.
                  The workaround cannot be applied if the experimental :kconfig:option:`CONFIG_CHIP_STORE_KEYS_IN_KMU` Kconfig option is set to ``y``.

.. rst-class:: v3-0-2 v3-0-1 v3-0-0 v2-9-0-nRF54H20-1 v2-9-2 v2-9-1 v2-9-0 v2-8-0 v2-7-0

KRKNWK-19277: Invalid testing steps in the Light Switch README file
  The testing steps for groupcast binding are invalid in the :ref:`matter_light_switch_sample` sample documentation.

  **Workaround:** For groupcast binding, refer to the :ref:`ug_matter_group_communication` user guide instead of the sample documentation.

.. rst-class:: v3-0-2 v3-0-1 v3-0-0 v2-9-0-nRF54H20-1 v2-9-2 v2-9-1 v2-9-0 v2-8-0 v2-7-0

KRKNWK-20019: The identify time does not update for the endpoint 1 in the Matter Bridge application
  The identify cluster is enabled in the :file:`.zap` file, but not in the application.

  **Affected platforms:** nRF7002, nRF5340

  **Workaround:** Remove the identify cluster from the :file:`.zap` file, as this cluster is optional for the aggregator endpoint type.

.. rst-class:: v3-0-2 v3-0-1 v3-0-0 v2-9-0-nRF54H20-1 v2-9-2 v2-9-1 v2-9-0

KRKNWK-20562: The memory layout for the nRF54L10 target is invalid
  The declared non-volatile memory layout is 10 kB greater than the actual size.
  In case of using the memory area out of bounds, the device might crash or behave unexpectedly.

  **Workaround:** Change the nRF54L10 memory layout to end at ``0xFD000`` address.

.. rst-class:: v3-0-1 v3-0-0

KRKNWK-20308: The ``MyCluster.xml`` file example in the :ref:`ug_matter_creating_custom_cluster` user guide does not contain the ``ExtendedCommandResponse`` command
  You cannot use the file properly without this command.

  **Workaround:** Manually add the following entry to the local copy of the ``MyCluster.xml`` file inside the ``<clusterExtension code="0x0028">`` element:

    .. code-block:: xml

      <command source="server" code="0x01" name="ExtendedCommandResponse" optional="false" disableDefaultResponse="true">
        <description>Response to ExtendedCommand.</description>
        <arg name="arg1" type="int8u"/>
      </command>

.. rst-class:: v3-0-2 v3-0-1 v3-0-0 v2-9-1 v2-9-0 v2-8-0 v2-7-0

KRKNWK-20268: Certification test cast TC-ACE-2.2 fails with an unexpected error when testing access permissions for the ``NodeLabel`` attribute in the BridgedDeviceBasicInformation cluster
  The ``NodeLabel`` attribute has invalid access permissions.

  **Workaround:** Cherry-pick changes from the PR #604 in the `dedicated Matter fork`_ repository and regenerate the application ZAP files using ``west zap-generate``.

.. rst-class:: v2-9-1 v2-9-0 v2-8-0

KRKNWK-20035: Door Lock Attributes do not persist after reboot or reset
  The persistent storage fails to load due to a wrong value in the :kconfig:option:`CONFIG_TRUSTED_STORAGE_BACKEND_AEAD_MAX_DATA_SIZE` Kconfig option.

  **Workaround:** Cherry-pick changes from PR #20713 in the `sdk-nrf`_ repository.

.. rst-class:: v2-9-1 v2-9-0 v2-8-0 v2-7-0 v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0

KRKNWK-19915: Certification test case TC-DGGEN-2.1 fails on test step 10d when Total Operational Hours feature is enabled
  Total operational hours are saved to NVM with interval defined by :kconfig:option:`CONFIG_CHIP_OPERATIONAL_TIME_SAVE_INTERVAL` Kconfig option.
  The test case requires checking the value after one hour, restarting the DUT, and verifying if the value has not changed, causing failures if given option is greater than 1.

  **Workaround:** There are two alternative solutions to fix this issue:

    * Disable PICS DGGEN.S.A0003 in the :file:`GeneralDiagnosticsClusterTestPlan.xml` file, disable the ``TotalOperationalHours`` attribute in the General Diagnostics Cluster on Endpoint 0 using the ZAP Tool, and regenerate files.
    * Manually cherry-pick and apply the commit with the fix to ``sdk-connectedhomeip`` (commit hash: ``89e823e5d8065ce85a6e6b4c205e11db413c4535`` from the `upstream Matter SDK repository <https://github.com/project-chip/connectedhomeip>`_).

  .. note::

    The total operational hours feature is optional and this issue affects the certification process only when DGGEN.S.A0003 PICS is enabled.

.. rst-class:: v2-9-0

KRKNWK-19804: Descriptor Device Type List does not include OTA Requestor device on endpoint 0
  None of the Matter samples include the OTA Requestor Device Type on endpoint 0 in the Descriptor Device Type List.
  This causes the Matter 1.4 certification test IDM-10.5 to fail.

  **Workaround:** Manually cherry-pick and apply the commit with the fix to ``sdk-nrf`` (commit hash: ``9d1b1512b9176f7ecfd491c21659c90d4f119245``).

.. rst-class:: v2-9-1 v2-9-0 v2-8-0

KRKNWK-19806: RPU recovery fails during Wi-Fi® communication
   Wi-Fi is not reconnecting to previously connected network after RPU recovery.

  **Affected platforms:** nRF7002

  **Workaround:** Cherry-pick changes from PR #529 in the `Matter GitHub repository`_ .

.. rst-class:: v2-9-0-nRF54H20-1 v2-9-2 v2-9-1 v2-9-0 v2-8-0

NCSDK-28567: Swap time after DFU takes a very long time
  Due to the incorrect RRAM buffer configuration, the swap time after DFU can exceed three minutes.

  **Affected platforms:** nRF54L15, nRF54L10

  **Workaround:** Set the :kconfig:option:`CONFIG_NRF_RRAM_WRITE_BUFFER_SIZE` Kconfig option to ``1`` in the :file:`sysbuild/mcuboot/boards/nrf54l15dk_nrf54l15_cpuapp.conf` file of the application.

.. rst-class:: v2-9-0

KRKNWK-19846: Wrong command for the internal configuration build in the :ref:`matter_template_sample` sample documentation
  There is an obsolete and wrong command for building the sample for the nRF54L15 DK with support for Matter OTA DFU and DFU over Bluetooth SMP, and using internal MRAM only.

  **Affected platforms:** nRF54L15

  **Workaround:** Use the following command to build the sample for the nRF54L15 DK with support for Matter OTA DFU and DFU over Bluetooth SMP, and using internal MRAM only:

  .. code-block:: console

    west build -p -b nrf54l15dk/nrf54l15/cpuapp -- -DCONFIG_CHIP_DFU_OVER_BT_SMP=y -DFILE_SUFFIX=internal

.. rst-class:: v3-0-1 v3-0-0 v2-9-0

KRKNWK-19826: The Device Firmware Upgrade (DFU) fails for nRF5340 DK with RAM power down enabled
  The DFU fails for nRF5340 DK, if the application enables the :kconfig:option:`CONFIG_RAM_POWER_DOWN_LIBRARY` Kconfig option.
  This option is enabled by default for the ``release`` configuration of the following samples:

    * :ref:`matter_lock_sample`
    * :ref:`matter_light_switch_sample`
    * :ref:`matter_smoke_co_alarm_sample`
    * :ref:`matter_window_covering_sample`

  **Workaround:** Set the :kconfig:option:`CONFIG_RAM_POWER_DOWN_LIBRARY` Kconfig option to ``n`` in the :file:`prj_release.conf` file of the application.

.. rst-class:: v3-1-1 v3-1-0 v3-0-2 v3-0-1 v3-0-0 v2-9-0-nRF54H20-1 v2-9-2 v2-9-1 v2-9-0 v2-8-0

KRKNWK-19388: The smart plug functionality of Matter Bridge application does not work with Apple Home application
  The additional functionality of a smart plug that runs alongside the Matter Bridge functionality is not visible in the Apple Home application.
  It applies to every functionality reflected by the static Matter endpoint and run alongside dynamic endpoints represented by the Matter Bridge.

.. rst-class:: v2-7-0 v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0 v2-5-3 v2-5-2 v2-5-1 v2-5-0 v2-4-4 v2-4-3 v2-4-2 v2-4-1 v2-4-0

KRKNWK-19524: Reliability issues with multicast message delivery for certain access points
  Certain access points require Wi-Fi devices to send Multicast Listener Discovery version 2 (MLDv2) Report messages for multicast packets to be properly delivered to a given address.
  Matter over Wi-Fi devices only subscribed to multicast addresses within the internal IPv6 stack, without notifying the access points, leading to reliability issues with multicast message delivery.

  **Workaround:** To ensure reliable multicast communication, use the :c:func:`net_ipv6_mld_join` and :c:func:`net_ipv6_mld_leave` functions to explicitly subscribe or unsubscribe to multicast addresses.
  For reference, see the commit with the fix in the ``sdk-connectedhomeip`` repository (commit hash: ``08acaf44604acef6679bd7eb2d6b51245bcfa54c``).

.. rst-class:: v2-7-0

KRKNWK-19443: The device commissioning window can be opened for longer than 900 seconds, which violates the Matter specification
  A device using the Extended Announcement feature is allowed to open the commissioning window for longer than 900 s only if it is uncommissioned.
  However, the implementation with Extended Announcement enabled allows the device to open the commissioning window for longer than 900 s even when the device is commissioned as well.
  This leads to the TC-CADMIN-1.21 and TC-CADMIN-1.22 certification test cases failure.

  **Workaround:** Manually cherry-pick and apply the commit with the fix to ``sdk-connectedhomeip`` (commit hash: ``ba9faf2b1e321f009f8bf27f5800627c9e4826ea`` from the `upstream Matter SDK repository <https://github.com/project-chip/connectedhomeip>`_).

.. rst-class:: v2-7-0 v2-6-1 v2-6-0

NCSDK-29228: DFU over Bluetooth LE SMP can lead to an application crash
  Performing the DFU over SMP using a large application image with a specific size can lead to an MPU fault and application crash.
  The issue is not reproducible for all images above a certain file size, but rather it seems to occur only for some specific sizes.
  The reproduction rate is small and the specific conditions to trigger it are not well known.
  The exact root cause is not known, but is probably related to problems with communication between the QSPI and external flash chip.

  **Affected platforms:** nRF52840

  **Workaround:** There are two alternative solutions that seem to fix this issue:

    * Set the :kconfig:option:`CONFIG_NORDIC_QSPI_NOR_TIMEOUT_MS` Kconfig option to ``3500`` in the application :file:`prj.conf` file.
    * Manually cherry-pick and apply the commit with the fix to ``sdk-zephyr`` (commit hash: ``d1abe40fb0af5d6219a0bcd824c4ea93ab90877a`` from the `upstream Zephyr repository <https://github.com/zephyrproject-rtos/zephyr>`_).

.. rst-class:: v2-7-0 v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0 v2-5-3 v2-5-2 v2-5-1 v2-5-0 v2-4-4 v2-4-3 v2-4-2 v2-4-1 v2-4-0 v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0

KRKNWK-19300: The Matter weather station application has NVS size inconsistent with the settings partition size
  The settings partition size for Matter weather station is configured to the value of 64 kB.
  However, the application cannot use all 64 kB of the settings space, because it depends on the NVS size that is limited by the :kconfig:option:`CONFIG_SETTINGS_NVS_SECTOR_COUNT` Kconfig option to 32 kB.

  **Affected platforms:** Thingy53

  **Workaround:** Set the :kconfig:option:`CONFIG_SETTINGS_NVS_SECTOR_COUNT` Kconfig option to ``16`` in the application :file:`prj.conf` file.

  .. caution::

     The workaround can be applied only to a newly programmed or factory reset device.
     Changing the NVS sectors count for a device that is in-field and already has some data stored in settings might lead to the data corruption and undefined behavior.

.. rst-class:: v2-7-0

KRKNWK-19199: Matter Lock and Matter Template samples cannot be built in the release configuration for the nRF54H20 platform
  In the DTS overlay file for the ``nrf54h20dk/nrf54h20/cpuapp`` target, the watchdog configuration is missing, whereas in the release configuration, the :ref:`CONFIG_NCS_SAMPLE_MATTER_WATCHDOG<CONFIG_NCS_SAMPLE_MATTER_WATCHDOG>` Kconfig option is set to ``y``.
  Building samples with :makevar:`FILE_SUFFIX` variable set to ``release`` will fail for the ``nrf54h20dk/nrf54h20/cpuapp`` target.

  **Affected platforms:** nRF54H20

  **Workaround:** While building the Matter Lock or Matter Template sample with the :makevar:`FILE_SUFFIX` variable set to ``release``, set the :ref:`CONFIG_NCS_SAMPLE_MATTER_WATCHDOG<CONFIG_NCS_SAMPLE_MATTER_WATCHDOG>` Kconfig option to ``n``.

.. rst-class:: v2-7-0

NCSDK-27972: No Bluetooth advertising after a software reset
  The software reset fails to properly reboot the nRF54H20 device, resulting in malfunction of Bluetooth LE advertising.
  This issue affects the factory reset functionality of the Matter device.

  **Affected platforms:** nRF54H20

  **Workaround:** Press the **RESET** button on the nRF54H20 DK after performing a factory or software reset of the device.

.. rst-class:: v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0

KRKNWK-18965: Malformed group messages can cause memory leak
  Matter accessories utilizing group communication might experience memory leaks if the secure group message is malformed.

  **Workaround:** Manually cherry-pick and apply the commit with the fix to ``sdk-connectedhomeip`` (commit hash: ``bdf3e6e183cba3d13bc5103bf014b47841a14de1`` from the `upstream Matter SDK repository <https://github.com/project-chip/connectedhomeip>`_).

.. rst-class:: v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0 v2-5-3 v2-5-2 v2-5-1 v2-5-0

KRKNWK-18966: Malformed messages might cause crash of device
  Matter accessories might use already freed memory or perform a double free operation when a malformed message is received while waiting for a response to an ongoing exchange.

  **Workaround:** Manually cherry-pick and apply the commit with the fix to ``sdk-connectedhomeip``:

   * For |NCS| v2.6.1 and v2.6.0, commit hash ``a836887c9f8ac277ed02a184c8fe82f8365f5353`` from the `upstream Matter SDK repository <https://github.com/project-chip/connectedhomeip>`_.
   * For |NCS| v2.5.3, v2.5.2, v2.5.1, and v2.5.0, commit hash ``3c808ab05f1fe9c2452ac285c2cad559c060b8f6`` from the `upstream Matter SDK repository <https://github.com/project-chip/connectedhomeip>`_.

.. rst-class:: v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0 v2-5-3 v2-5-2 v2-5-1 v2-5-0

KRKNWK-18916: Issues related to the  :kconfig:option:`CONFIG_CHIP_LAST_FABRIC_REMOVED_ERASE_AND_PAIRING_START` Kconfig option
  When the Kconfig option is selected, there are two issues:

  * An assert might occur after removing the last fabric because the OpenThread interface is still active despite the Thread stack being disabled.
  * The device cannot be commissioned to the Matter network because the device's IEEE 802.15.4 Extended address is the same as the one saved in the SRP server.

  **Workaround:** Add the following lines to the :file:`fabric_table_delegate` file:

    * After Line 54 (After the ``DoFactoryReset()`` function) to provide a workaround for potential assert:

      .. code-block:: C++

        chip::DeviceLayer::ThreadStackMgrImpl().LockThreadStack();
        otIp6SetEnabled(chip::DeviceLayer::ThreadStackMgrImpl().OTInstance(), false);
        chip::DeviceLayer::ThreadStackMgrImpl().UnlockThreadStack();

    * After Line 56 (After the ``ErasePersistentInfo()`` function) to generate the new IEEE 802.15.4 Extended address and set it to avoid duplication:

      .. code-block:: C++

        otExtAddress newOtAddr = {};
        chip::Crypto::DRBG_get_bytes(reinterpret_cast<uint8_t*>(&newOtAddr), sizeof(newOtAddr));
        otLinkSetExtendedAddress(chip::DeviceLayer::ThreadStackMgrImpl().OTInstance(), &newOtAddr);

  .. note::

    For |NCS| versions v2.5.3, v2.5.2, v2.5.1, and v2.5.0, the :file:`fabric_table_delegate.h` file is located in the :file:`samples/matter/common/src/` directory, whereas for |NCS| versions v2.6.1 and v2.6.0, the file is located in the :file:`samples/matter/common/src/app` directory.

.. rst-class:: v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0

KRKNWK-18673: Bridged Light Bulb device type reports a failure when reading or writing specific ``onoff`` cluster attributes
  The Bridge has defined ZAP clusters properly for a bridged Light Bulb, but handling of specific ``onoff`` cluster attributes has not been implemented.

  **Workaround:** Manually cherry-pick and apply the commit with the fix to ``sdk-nrf`` (commit hash: ``79f3a901dd0787df9327640cb3bb889ccb023005``).

.. rst-class:: v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0

KRKNWK-18769: :ref:`matter_bridge_app` application does not print the hyperlink for displaying the setup QR code in the log
  This happens because the log module that displays this log entry has been disabled.

  **Workaround:** Remove the following line from the :file:`src/chip_project_config.h` header file:

  .. code-block:: C

     #define CHIP_CONFIG_LOG_MODULE_AppServer_PROGRESS 0

.. rst-class:: v2-7-0 v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0 v2-5-3 v2-5-2 v2-5-1 v2-5-0

KRKNWK-18556: While creating multiple subscriptions, the persistent subscriptions resumption feature works only for the first created subscriptions
  This happens when multiple subscriptions are created by multiple Matter controllers.
  The persistent subscriptions feature will be replaced by the Check In protocol from the Intermittently Connected Devices (ICD) Cluster.

.. rst-class:: v2-5-3 v2-5-2 v2-5-1 v2-5-0 v2-4-4 v2-4-3 v2-4-2 v2-4-1 v2-4-0 v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1

KRKNWK-18316: When the :kconfig:option:`CONFIG_PRINTK_SYNC` Kconfig option is enabled in a Matter over Thread application, the IEEE 802.15.4 radio driver might calculate invalid IEEE 802.15.4 radio frame timestamps
  This is caused by the implementation of synchronous ``printk`` in Zephyr using ``spinlock`` synchronization primitive, which can block Real Time Clock interrupts that are needed by the radio driver to calculate precise timestamps.

  **Workaround:** If it is enabled, disable the :kconfig:option:`CONFIG_PRINTK_SYNC` Kconfig option in your application.

.. rst-class:: v2-5-3 v2-5-2 v2-5-1 v2-5-0 v2-4-4 v2-4-3 v2-4-2 v2-4-1 v2-4-0 v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1

KRKNWK-18495: The Color Control server's ``RemainingTime`` attribute change might be reported every 100 ms, even though the color temperature value handled by the Color Control server is not modified
  This can result in the Thread network being spammed with unnecessary network traffic when controlling the brightness or color of the :ref:`matter_light_bulb_sample` sample.

  **Workaround:** Manually cherry-pick and apply the commit with the fix to ``sdk-connectedhomeip`` (commit hash: ``3da44025b18d17adacf0a4abf0456c5735399dbd``).

.. rst-class:: v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2

KRKNWK-18371: The GlobalSceneControl attribute from the OnOff cluster does not change in a way compatible with the Matter specification
  The GlobalSceneControl attribute from the OnOff cluster is not set to ``false`` after receiving the ``OffWithEffect`` command.
  This behavior is not compatible with the Matter specification.

  **Workaround:** Manually cherry-pick and apply the commit with the fix to ``sdk-connectedhomeip`` (commit hash: ``836390ed636ca36126dbcbe763d0f127626cba8d``).

.. rst-class:: v2-5-3 v2-5-2 v2-5-1 v2-5-0

KRKNWK-18315: SPAKE2+ Verifier is not regenerated when using non-default passcode
  When building factory data with a non-default passcode, the SPAKE2+ Verifier is not generated based on the selected passcode value, but uses the default passcode value (``20202021``).

  **Workaround:** Enable the :kconfig:option:`CONFIG_CHIP_FACTORY_DATA_GENERATE_SPAKE2_VERIFIER` Kconfig option to generate the SPAKE2+ Verifier based on the provided passcode.

.. rst-class:: v2-5-1 v2-5-0 v2-4-4 v2-4-3 v2-4-2 v2-4-1 v2-4-0

KRKNWK-18256: The Matter over Thread device might crash during the processing of the DNS resolve response
  The Matter core implementation handles DNS resolve responses for the Thread platform in a wrong way.
  If the DNS resolve response contains a TXT record with data size equal to 0 (either it is not present or its Time-To-Live (TTL) is equal to 0), the Matter device's application crashes.
  The application behavior for the responses containing a TXT record with data size not equal to 0 is correct.

  **Workaround:** Manually cherry-pick and apply the commit with the fix to ``sdk-connectedhomeip`` (commit hash: ``4997cd70ed53735e302186e7eda1bb28a216199a``).

.. rst-class:: v2-5-1 v2-5-0 v2-4-4 v2-4-3 v2-4-2 v2-4-1 v2-4-0 v2-3-0 v2-2-0

KRKNWK-18221: Memory leak in the deferred attribute persister
  The ``DeferredAttributePersister`` helper class is used to limit the flash wear for applications that include fast-changing, persistent cluster attributes.
  This class leaks a small heap memory buffer for each deferred attribute write, which can eventually lead to running out of heap memory until the device is rebooted.

  **Workaround:** Manually cherry-pick and apply the commit with the fix to ``sdk-connectedhomeip`` (commit hash: ``e79b0cf44c86ce35dabcf69b50903ac706c67465``).

.. rst-class:: v2-5-3 v2-5-2 v2-5-1 v2-5-0 v2-4-4 v2-4-3 v2-4-2 v2-4-1 v2-4-0 v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2

KRKNWK-17360: Groupcast communication does not work for multiple endpoints that are part of the same group on a single Matter node
  The Matter core implementation handles commands status in a wrong way for those targeted to a group.
  This issue is only visible when adding multiple endpoints that exist on the same Matter node to the same group, and results in an application crash after receiving a group command.
  When adding multiple Matter nodes with a single endpoint each to the same group, the communication works correctly.

  **Workaround:** Manually cherry-pick and apply the commit with the fix to ``sdk-connectedhomeip`` (commit hash: ``99f80de289491ad24a13dda9178a7a24c85324a7``).

.. rst-class:: v2-5-0

KRKNWK-17864: When using Wi-Fi low power mode, the communication with the device might not work after it re-connects to the newly respawned Wi-Fi network
  The communication with Matter over a Wi-Fi device sometimes does not work after it re-connects to the Wi-Fi network.
  The issue is only visible in cases of re-connection to the newly respawned Wi-Fi network that are triggered by rebooting the Wi-Fi access point.
  The root cause of the issue is not known but can be related to the usage of Wi-Fi in low power mode.
  After the application reboot, the device is always able to re-connect to the Wi-Fi network and operate normally.

  **Affected platforms:** nRF7002

  **Workaround:** Disable Wi-Fi low power mode for your application by setting :kconfig:option:`CONFIG_NRF_WIFI_LOW_POWER` to ``n`` in the application :file:`prj.conf`.

.. rst-class:: v2-5-3 v2-5-2 v2-5-1 v2-5-0

KRKNWK-17925: The nRF Toolbox application for iOS devices cannot control :ref:`matter_lock_sample` using NUS
  The nRF Toolbox application sends one additional character in all NUS commands, so they are not correctly parsed by the :ref:`matter_lock_sample`.
  The issue was observed only on the nRF Toolbox 5.0.9 version of the iOS system.

  **Workaround:** Use nRF Toolbox for iOS versions other than 5.0.9 or any version of nRF Toolbox for Android.

.. rst-class:: v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0 v2-5-3 v2-5-2 v2-5-1 v2-5-0

KRKNWK-17914: The links to Kconfig options from :file:`Kconfig.features` do not work in the |NCS| documentation
  The links to all Kconfig options defined in the :file:`modules/lib/matter/config/nrfconnect/Kconfig.features` file do not work in the documentation.

.. rst-class:: v2-4-3 v2-4-2 v2-4-1 v2-4-0 v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1

KRKNWK-17718: Failure at TC-DGSW-1.1 Matter Certification test
  The issue happens due to a wrong Software Diagnostic cluster feature map with features enabled by default.

  **Workaround:** Set the default value of the Software Diagnostic cluster feature map to 0 using the ZAP Tool and regenerate files, and manually cherry-pick and apply the commit with the fix to ``sdk-connectedhomeip`` (commit hash: ``70dd449ff943159365466ad5125f42a5bdcbfc0b``).

.. rst-class:: v2-4-3 v2-4-2 v2-4-1 v2-4-0

KRKNWK-17699: Failure at TC-BINFO-1.1 Matter Certification test for the lock sample
  The issue happens due to a noncompliant Basic Information cluster revision for Matter 1.1.0 in the lock sample.

  **Workaround:** Set the default value of the Basic Information cluster revision to 1 in :file:`samples/matter/lock/src/lock.zap` using the ZAP Tool and regenerate files.

.. rst-class:: v2-4-3 v2-4-2 v2-4-1 v2-4-0 v2-3-0 v2-2-0

KRKNWK-17594: Application does not always respond when forcing fail-safe cleanup
  This can happen because of the Thread interface being unnecessarily reset and can result in the TC-CNET-4.10 Matter Certification test failing.

  **Workaround:** Manually cherry-pick and apply the commit with the fix to ``sdk-connectedhomeip`` (commit hash: ``3b2d8e1367d9055a78d72365323cfbf60e054975``).

.. rst-class:: v2-4-3 v2-4-2 v2-4-1 v2-4-0 v2-3-0 v2-2-0

KRKNWK-17535: The application core can crash on nRF5340 after the OTA firmware update finishes if the factory data module is enabled
  In the initialization method of the factory data module, the factory data partition and a part of the application image is restricted by Fprotect, which makes it impossible to confirm the new image in the Matter thread.
  Instead, the confirmation must be performed before the factory data module is initialized.

  **Affected platforms:** nRF5340

  **Workaround:** Complete the following steps:

  1. Manually cherry-pick and apply the commit with the fix to ``sdk-connectedhomeip`` (commit hash: ``eeb7280620fff1e16a75cfa41338186fd952c432``).
  #. Add the following lines to the :file:`samples/matter/common/src/ota_util.cpp`:

     .. code-block::

        #include <platform/CHIPDeviceLayer.h>
        #include <zephyr/dfu/mcuboot.h>

        #ifndef CONFIG_SOC_SERIES_NRF53X
          VerifyOrReturn(mcuboot_swap_type() == BOOT_SWAP_TYPE_REVERT);
        #endif

        OTAImageProcessorImpl &imageProcessor = GetOTAImageProcessor();
        if(!boot_is_img_confirmed()){
          CHIP_ERROR err = System::MapErrorZephyr(boot_write_img_confirmed());
          if (CHIP_NO_ERROR == err) {
            imageProcessor.SetImageConfirmed();
            ChipLogProgress(SoftwareUpdate, "New firmware image confirmed");
          } else {
            ChipLogError(SoftwareUpdate, "Failed to confirm firmware image, it will be reverted on the next boot");
          }
        }

  #. Add the following line to the :file:`samples/matter/common/src/ota_util.h`:

     .. code-block::

        void OtaConfirmNewImage();

  #. Add the following lines to the ``AppTask::Init()`` method in the :file:`app_task.cpp` file located in a sample directory before initialization of the factory data module (``mFactoryDataProvider.Init()``):

     .. code-block::

        #ifdef CONFIG_CHIP_OTA_REQUESTOR
          /* OTA image confirmation must be done before the factory data init. */
          OtaConfirmNewImage();
        #endif

.. rst-class:: v2-4-0 v2-3-0

KRKNWK-17151: Application core can crash on nRF5340 when there is a high load on Zephyr's main thread
  The priority of Zephyr's main thread is set to the same value as the IPC thread's priority (``0``).
  Because of this setting, when Zephyr's main thread is working for a long time, an interrupt related to the IEEE 802.15.4 radio driver can occur and the application core can crash due to a lack of communication with the network core.
  To avoid blocking the communication between cores, the main thread priority should have a lower priority than the IPC priority.

  **Affected platforms:** nRF5340

  **Workaround:** Add an additional Kconfig option :kconfig:option:`CONFIG_MAIN_THREAD_PRIORITY` set to ``1`` to the build configuration.

.. rst-class:: v2-4-0

KRKNWK-17064: Incorrect links in the Matter documentation
  The following links to Matter SDK documentation point to the ``master`` version of the Matter SDK module instead of the commit SHA used for the |NCS| v2.4.0: `other controller setups`_, `CHIP Certificate Tool source files`_, and `Bluetooth LE Arbiter's header file`_.

  **Workaround:** Change ``master`` to the ``9e6386c`` commit SHA in the page URLs to see the content valid for the |NCS| v2.4.0 release.

.. rst-class:: v2-3-0 v2-2-0

KRKNWK-16728: Sleepy device might consume much power when commissioned to a commercial ecosystem
  The controllers in the commercial ecosystem fabric establish a subscription to a Matter device's attributes.
  The controller requests using some subscription intervals, and the Matter device might negotiate other values, but by default it just accepts the requested ones.
  In some cases, the selected intervals can be small, and the Matter device will have to report status very often, which results in high power consumption.

  **Workaround:** Implement ``OnSubscriptionRequested`` method in your application to set values of subscription report intervals that are appropriate for your use case.
  Additionally, register your class implementation to make ``InteractionModelEngine`` use it.
  This is an example of how your implementation could look:

  .. code-block::

     #include <app/ReadHandler.h>

     class SubscriptionApplicationCallback : public chip::app::ReadHandler::ApplicationCallback
     {
        CHIP_ERROR OnSubscriptionRequested(chip::app::ReadHandler & aReadHandler,
                                           chip::Transport::SecureSession & aSecureSession) override;
     };

     CHIP_ERROR SubscriptionApplicationCallback::OnSubscriptionRequested(chip::app::ReadHandler & aReadHandler,
                                                          chip::Transport::SecureSession & aSecureSession)
     {
        /* Set the interval in seconds appropriate for your application use case, example 60 seconds. */
        uint32_t exampleMaxInterval = 60;
        return aReadHandler.SetReportingIntervals(exampleMaxInterval);
     }

  The class implementation can be registered in your application code the following way:

  .. code-block::

      #include <app/InteractionModelEngine.h>

      SubscriptionApplicationCallback myClassInstance;

      chip::app::InteractionModelEngine::GetInstance()->RegisterReadHandlerAppCallback(&myClassInstance);

  You can use the ``ICDUtil`` module implementation that was introduced in the |NCS| v2.4.0 as a reference.
  It is located in the :file:`samples/matter/common/src/icd_util.cpp` file in the :file:`nrf` directory.

.. rst-class:: v2-3-0 v2-2-0

KRKNWK-16575: Applications with factory data support do not boot up properly on nRF5340
  When the Matter sample is built for ``nrf5340dk_nrf5340_cpuapp`` board target with the :kconfig:option:`CONFIG_CHIP_FACTORY_DATA` Kconfig option set to ``y``, the application returns prematurely the error code ``200016`` because the factory data partition is not aligned with the :kconfig:option:`CONFIG_FPROTECT_BLOCK_SIZE` Kconfig option.

  **Affected platforms:** nRF5340

  **Workaround:** Manually cherry-pick and apply commit from the main branch (commit hash: ``ec9ad82637b0383ebf91eb1155813450ad9fcffb``).

.. rst-class:: v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1

KRKNWK-16783: Accessory might become unresponsive after several hours
  A Matter accessory might stop sending Report Data messages due to an internal bug in the Matter stack v1.0.0.0 and thus become unresponsive for Matter controllers.

  **Workaround:** Manually cherry-pick and apply commit from the `dedicated Matter fork`_ (commit hash: ``23f08242f92973a7a3308b4d62a82c59cf6cf6b3``).

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

  **Affected platforms:** nRF5340

.. rst-class:: v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0

KRKNWK-11225: CHIP Tool for Android cannot communicate with a Matter device after the device reboots
  CHIP Tool for Android does not implement any mechanism to recover a secure session to a Matter device after the device has rebooted and lost the session.
  As a result, the device can no longer decrypt and process messages sent by CHIP Tool for Android as the controller keeps using stale cryptographic keys.

  **Workaround:** Do not reboot the device after commissioning it with CHIP Tool for Android.

  .. note::
      The support for the Android CHIP Tool is removed as of the |NCS| v2.3.0 for Matter in the |NCS|. Use CHIP Tool for Linux or macOS instead, as described in :ref:`ug_matter_gs_testing`.

.. rst-class:: v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0

KRKNWK-10589: CHIP Tool for Android crashes when commissioning a Matter device
  In random circumstances, CHIP Tool for Android crashes when trying to connect to a Matter device over Bluetooth LE.

  **Workaround:** Restart the application and try to commission the Matter device again.
  If the problem persists, clear the application data and try again.

.. rst-class:: v1-9-2 v1-9-1 v1-9-0

KRKNWK-12950: CHIP Tool for Android opens the commissioning window using an incorrect PIN code
  CHIP Tool for Android uses a random code instead of a user-provided PIN code to open the commissioning window on a Matter device.

.. rst-class:: v1-6-1 v1-6-0

KRKNWK-10387: Matter service is needlessly advertised over Bluetooth LE during DFU
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

Near Field Communication (NFC)
==============================

The issues in this section are related to the :ref:`ug_nfc` protocol.

.. rst-class: v3-1-1 v3-1-0 v3-0-2 v3-0-1 v3-0-0 v2-9-0-nRF54H20-1 v2-9-2 v2-9-1 v2-9-0 v2-8-0 v2-7-0

NCSDK-34404: Possible increased current consumption in NFC applications on the nRF54H20 and nRF54L devices
  If your application uses NFC on the nRF54H20 and nRF54L SoCs, the current consumption may remain high even after the NFC field is lost, due to the running high-frequency peripheral clock.

  **Affected platforms:** nRF54H20, nRF54L05, nRF54L10, nRF54L15

Thread
======

The issues in this section are related to the :ref:`ug_thread` protocol.

.. rst-class:: v2-9-0-nRF54H20-1 v2-9-2 v2-9-1 v2-9-0 v2-8-0

KRKNWK-19628: On nRF54H20, persistence of keys is not yet supported
  After rebooting the device, all persistent keys (Thread's network key) will be lost.
  This issue applies also to Matter.

  **Affected platforms:** nRF54H20

.. rst-class:: v2-7-0

KRKNWK-19376: OpenThread CLI sample does not work on the nRF52840 Dongle
  The sample should not use Partition Manager.
  To avoid a failure, the main function should have a check for whether the USB is already enabled.

  **Affected platforms:** nRF52840 Dongle

  **Workaround:** Manually cherry-pick and apply commit with fix from ``main`` (commit hash: ``f7b59d26db4cc55c6936a0a88f3daa7e0b7b2085``).

.. rst-class:: v2-6-1 v2-6-0

KRKNWK-19036: High power consumption after parent loss
  After a parent loss, an SED would keep the radio on between reattach attempts.

  **Workaround:** Manually cherry-pick and apply commit with fix to ``sdk-zephyr`` (commit hash: ``6c602a1bbd3b3f7811082bce391c6943663a2c64``).

.. rst-class:: v3-1-1 v3-1-0 v3-0-2 v3-0-1 v3-0-0 v2-9-0-nRF54H20-1 v2-9-2 v2-9-1 v2-9-0 v2-8-0 v2-7-0 v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0

KRKNWK-18612: nRF5340 sometimes fails to send a Child Update Response to an SSED child
  After performing an MLE Child Update Request by an SSED child, an nRF5340 parent sometimes does not respond with a Child Update Response.
  This is caused by the CSL transmit request being issued to the nRF 802.15.4 Radio Driver so late that it cannot be handled on time.
  On second attempt, the MLE Child Update Request and Response exchange works correctly.

  **Affected platforms:** nRF5340

.. rst-class:: v2-4-0 v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0

CVE-2023-2626: OpenThread KeyID Mode 2 Security Vulnerability
  This vulnerability impacts all Thread devices using OpenThread and allows an attacker in physical proximity to compromise non-router-capable devices and the entire Thread network in the case of router-capable devices.
  The vulnerability allows an attacker in physical proximity to inject arbitrary IPv6 packets into the Thread network via IEEE 802.15.4 frame transmissions.
  Because the Thread Management Framework (TMF) protocol does not have any additional layer of security, the attacker could exploit this vulnerability to update the Thread Network Key and gain full access to the Thread network.
  There is no known exploitation of vulnerability.

  Due to this issue, the Thread certifications for OpenThread libraries in all |NCS| releases up to v2.4.0 are deprecated.
  OpenThread libraries for selected |NCS| releases were patched with the OpenThread KeyID Mode 2 Security Vulnerability fix and re-certified by inheritance.
  The libraries are available through a DevZone request.

.. rst-class:: v2-0-0

KRKNWK-14231: Device stops receiving after switching from SSED to MED
  Trying to switch to the MED mode after working as CSL Receiver makes the device stop receiving frames.

  **Workaround:** Before invoking :c:func:`otThreadSetLinkMode` to change the device mode, make sure to set the CSL Period to ``0`` with :c:func:`otLinkCslSetPeriod`.

.. rst-class:: v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0

KRKNWK-9094: Possible deadlock in shell subsystem
  Issuing OpenThread commands too fast might cause a deadlock in the shell subsystem.

  **Workaround:** If possible, avoid invoking a new command before execution of the previous one has completed.

.. rst-class:: v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0

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

KRKNWK-9461/KRKNWK-9596 : Multiprotocol sample crashes with some smartphones
  With some smartphones, the multiprotocol sample crashes on the nRF5340 due to timer timeout inside the 802.15.4 radio driver logic.

  **Affected platforms:** nRF5340

.. rst-class:: v1-4-2 v1-4-1 v1-4-0

KRKNWK-7885: Throughput is lower when using CC310 nrf_security backend
  A decrease of throughput of around 5-10% has been observed for the :ref:`CC310 nrf_security backend <nrf_security_backends_cc3xx>` when compared with :ref:`nrf_oberon <nrf_security_backends_oberon>` or the standard Mbed TLS backend.
  CC310 nrf_security backend is used by default for nRF52840 boards.
  The source of throughput decrease is coupled to the cost of RTOS mutex locking when using the :ref:`CC310 nrf_security backend <nrf_security_backends_cc3xx>` when the APIs are called with shorter inputs.

  **Affected platforms:** nRF52840

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

Wi-Fi
=====

The issues in this section are related to the :ref:`ug_wifi` protocol.

.. rst-class:: v3-1-1 v3-1-0

SHEL-3745: Wi-Fi connection broken with nRF54H20
  Wi-Fi connection fails due to ``pbkdf2_sha1()`` error.

  **Affected platforms:** nRF7002, nRF54H20

.. rst-class:: v3-1-1 v3-1-0 v3-0-2 v3-0-1 v3-0-0 v2-9-0-nRF54H20-1 v2-9-2 v2-9-1 v2-9-0 v2-8-0

NRF7X-323: Wi-Fi ``ns`` builds take eight seconds to boot
  It takes eight seconds when autoloading credentials before the scan/connect procedure occurs.

  **Affected platforms:** nRF7002

.. rst-class:: v3-0-0

SHEL-3604: Crash during Wi-Fi SoftAP mode initialization
  When the :ref:`wifi_softap_sample` sample is built and run, a crash occurs during initialization due to a hostap stack overflow.

  **Workaround:** Increase the hostap stack size to 5800 bytes or cherry-pick changes from PR #2822 in the `sdk-zephyr`_ repository.

.. rst-class:: v3-0-0

SHEL-3596: For nRF5340DK + nRF7002EK, XiP is not working after Wi-Fi interface down/up
  When using external flash for storing nRF70 firmware patches and XiPing, seeing a hang when XiP is re-enabled and accessing the code.

  **Workaround:** Removing the XiP disable solves the issue and Wi-Fi works even after interface down + up.

  **Affected platforms:** nRF7002 EK

.. rst-class:: v2-9-0-nRF54H20-1 v2-9-2 v2-9-1 v2-9-0 v2-8-0

SHEL-3345 : Wi-Fi Coex (Bluetooth LE and Thread) is broken
  The Wi-Fi Coex is broken for Bluetooth LE and Thread due to missing devicetree node in ``cpuapp``.

  **Workaround:** Cherry-pick commits from the upstream: `Zephyr PR #83028`_.

  **Affected platforms:** nRF7002

.. rst-class:: v3-1-1 v3-1-0 v3-0-2 v3-0-1 v3-0-0 v2-9-0-nRF54H20-1 v2-9-2 v2-9-1 v2-9-0 v2-8-0

NCSDK-29650: Wi-Fi connection times are increased in ``_ns`` builds
  The latency has increased significantly in latest main branch.
  It goes beyond the WPA supplicant command timeout of 10 s, taking now 13 s and causing Wi-Fi connectivity failures.

  **Workaround:** Increase the connection timeout (10 s -> 15 s).

  **Affected platforms:** nRF7002

.. rst-class:: v3-1-1 v3-1-0 v3-0-2 v3-0-1 v3-0-0 v2-9-0-nRF54H20-1 v2-9-2 v2-9-1 v2-9-0 v2-8-0

NCSDK-29649: Flash firmware integrity check does not work due to Mbed TLS dependencies
  Due to this, nRF70 patches cannot be validated and corrupted patches can manifest in nRF70 bootup failures.

  **Affected platforms:** nRF7002

.. rst-class:: v3-1-1 v3-1-0 v3-0-2 v3-0-1 v3-0-0 v2-9-0-nRF54H20-1 v2-9-2 v2-9-1 v2-9-0 v2-8-0

NCSDK-29651: nRF54H20 Legacy crypto not supported
  PSA crypto is used by default, which limits Wi-Fi security only up to WPA2.

  **Affected platforms:** nRF7002, nRF54H20

.. rst-class:: v2-9-0-nRF54H20-1 v2-9-2 v2-9-1 v2-9-0 v2-8-0

SHEL-3226: In nRF70 driver page docs, link to doxygen API has been removed
  The link will be brought back in coming releases.

.. rst-class:: v2-7-0

SHEL-2878: WPA3 security mode not working
  WPA3 security mode is not working for nRF54L15 PDK and nRF7002 EB.

  **Affected platforms:** nRF7002, nRF54L15

.. rst-class:: v2-5-1

SHEL:2372: A new initialization sequence causes QSPI to not get initialized
  There is an ``RDSR -16`` error, and QSPI initialization failed.
  After a hardware reset, it fails again.

  **Affected platforms:** nRF7002

Zigbee
======

The issues in this section are related to the :ref:`ug_zigbee` protocol.
Starting with the |NCS| v3.0.0 release, known issues are moved to the `Zigbee R22`_ repository.

.. include:: /includes/zigbee_deprecation.txt

.. rst-class:: v2-9-0-nRF54H20-1 v2-9-2 v2-9-1 v2-9-0 v2-8-0 v2-7-0 v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0 v2-5-3 v2-5-2 v2-5-1 v2-5-0 v2-4-4 v2-4-3 v2-4-2 v2-4-1 v2-4-0 v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0

NCSIDB-1411: Clearing configuration data is not fully performed when processing the Leave Network command
   Configuration data of ZCL Reporting feature is not cleared when processing the leave network command, resulting in incomplete compliance with the specification.

   The Zigbee BDB 3.0 Specification, section 9.4 says that all Zigbee persistent data (with exceptions ..) must be cleared in response to the Mgmt_Leave_req command, however the stack leaves the ZCL Reporting parameter values ​​set (cached in memory) and continues to use them.

   **Workaround:** You can supplement the processing of the Leave Network command with ZCL Reporting parameter clearing by adding a :c:func:`zb_zcl_init_reporting_info` call in the ``ZB_ZDO_SIGNAL_LEAVE`` handler.
   See the following snippet for an example:

  .. code-block:: c

     case ZB_ZDO_SIGNAL_LEAVE:
         /* Device leaves the network. */
         if (status == RET_OK) {
             zb_zdo_signal_leave_params_t *leave_params =
                 ZB_ZDO_SIGNAL_GET_PARAMS(sig_hndler, zb_zdo_signal_leave_params_t);

             if (leave_params->leave_type == ZB_NWK_LEAVE_TYPE_RESET) {
                /* Workaround for NCSIDB-1411 - clearing ZCL Reporting parameters. */
                zb_zcl_init_reporting_info();
             }
        }
        /* Call default signal handler. */
        ZB_ERROR_CHECK(zigbee_default_signal_handler(bufid));
        break;

.. rst-class:: v2-9-0-nRF54H20-1 v2-9-2 v2-9-1 v2-9-0 v2-8-0 v2-7-0 v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0 v2-5-3 v2-5-2 v2-5-1 v2-5-0 v2-4-4 v2-4-3 v2-4-2 v2-4-1 v2-4-0 v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0

NCSIDB-1336: Zigbee Router device cannot rejoin after missing Network Key update or rotation
  If a Zigbee Router device does not receive a Network Key update or rotation messages (such as because of an ongoing reset or being powered off), it will not rejoin to a Zigbee Coordinator and will use old keys for communication.

  The Zigbee R22 Core Specification, section 4.3.6.1 says that “A device that is operating in a network and has missed a network key update may also use these procedures to receive the latest network key,” referring to the procedure to get a new network key.
  Since it uses the word MAY, not SHOULD, it means the ZBOSS stack implementation does not violate the specification.
  In addition, BDB 3.0 does not describe a procedure that can be used by a Zigbee Router device to verify if the security keys it uses are still valid.

  **Workaround:** You can extend a Zigbee Router device application to handle this scenario.
  This is done by handling the ``ZB_NLME_STATUS_INDICATION`` status that contains the ``ZB_NWK_COMMAND_STATUS_BAD_KEY_SEQUENCE_NUMBER`` status.
  It is not advised to proceed with a rejoin immediately after the first ``ZB_NWK_COMMAND_STATUS_BAD_KEY_SEQUENCE_NUMBER``, because this could make the network vulnerable to attackers trying to force a rejoin without knowing the network key.

  The signal handling logic is as follows:

  .. code-block:: c

     void zboss_signal_handler(zb_bufid_t bufid)
     {
             zb_zdo_app_signal_hdr_t *sig_hndler = NULL;
             zb_zdo_app_signal_type_t sig = zb_get_app_signal(bufid, &sig_hndler);

             /* Update network status LED. */
             zigbee_led_status_update(bufid, ZIGBEE_NETWORK_STATE_LED);



             switch (sig) {

     case ZB_NLME_STATUS_INDICATION:
             zb_zdo_signal_nlme_status_indication_params_t *nlme_status_ind = ZB_ZDO_SIGNAL_GET_PARAMS(sig_hndler, zb_zdo_signal_nlme_status_indication_params_t);


     if (nlme_status_ind->nlme_status.status ==  ZB_NWK_COMMAND_STATUS_BAD_KEY_SEQUENCE_NUMBER) {
                                     // optional check connection
                                     // optional rejoin if necessary
     }
     break;

     default:
             /* Call default signal handler. */
             ZB_ERROR_CHECK(zigbee_default_signal_handler(bufid));
             break;

             }
     }

  After receiving several (for example, five) status messages with a bad key sequence number, check if the device is connected to the network, such as by calling the :c:func:`zb_zdo_simple_desc_req` function.
  If the returned message status is not ``ZB_ZDP_STATUS_SUCCESS``, initiate the rejoin procedure by calling the :c:func:`zb_bdb_initiate_tc_rejoin` function.
  The device will switch the key and successfully rejoin network, whether the network is open or closed.

.. rst-class:: v2-9-0-nRF54H20-1 v2-9-2 v2-9-1 v2-9-0 v2-8-0 v2-7-0 v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0

KRKNWK-19894: The Device Firmware Upgrade (DFU) from the |NCS| v2.5.X or older to a newer version might cause fatal error or abnormal behavior
   The issue occurs when the device initially uses firmware built with the |NCS| v2.5.X or earlier, and has the ZCL Reporting feature configured (which means it reports selected attributes).
   Subsequently, the DFU is performed where the new firmware is built based on the |NCS| v2.6.0 or later.

   During the upgrade, the NVRAM content is preserved.
   However, part of this content has a structure that is incompatible with the new firmware, leading to abnormal behavior.

   The attribute reporting stops working correctly.
   The device frequently sends the Report Attributes command, but the messages are corrupted (empty).
   This condition of the device likely prevents remote reconfiguration.
   In some cases of cluster configuration, the firmware might encounter a fatal error every time it starts.

   **Workaround:** After upgrading the device, perform an NVRAM erase (complete or specifically targeting the part with the ``ZB_NVRAM_ZCL_REPORTING_DATA`` ID).
   This method prevents abnormal behavior, but it leads to the loss of configuration data, requiring you to reconfigure the device.

.. rst-class:: v2-7-0

KRKNWK-19263: FOTA DFU on the nRF5340 DK fails due to an invalid update image
  Performing FOTA DFU throws an error during an attempt to install the new image.

  **Workaround:** Manually cherry-pick and apply the commit with the fix from ``main`` (commit hash: ``cc5ae95668566b47b9f6bfccb99b3796de8cb076``).

.. rst-class:: v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0 v2-5-3 v2-5-2 v2-5-1 v2-5-0 v2-4-4 v2-4-3 v2-4-2 v2-4-1 v2-4-0 v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0

NCSISB-1204: Corrupted ZBOSS NVRAM causes an infinite boot loop
   NVRAM writing operations are not reset (for example, due to FOTA) and are power-down resistant.
   In such scenarios, the ongoing NVRAM record-writing operation will not be completed correctly.
   That means a corrupted NVRAM record.
   During boot, the ZBOSS stack reads NVRAM content, and in the event of detecting an NVRAM record, it generates a fatal error and leads to the device reset itself.
   From this point, the device boots, reads NVRAM, and detects corrupted records, which leads to a device reset.
   If such behavior is constantly repeated, the device will brick.

   **Workaround:** Request fixing patch from Nordic's `DevZone`_.

.. rst-class:: v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0 v2-5-3 v2-5-2 v2-5-1 v2-5-0 v2-4-4 v2-4-3 v2-4-2 v2-4-1 v2-4-0 v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0

NCSIDB-1246: Typos in Zigbee power configuration command macro causes battery alarms to not work
   A typo in the stack would prevent the ``BatteryVoltageThreshold1-3`` attributes from being properly configured, resulting in battery alarms not working.

.. rst-class:: v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0 v2-5-3 v2-5-2 v2-5-1 v2-5-0

KRKNWK-19026: Wrong RSSI values reported
  The Zephyr API to get the RSSI value from the radio driver was modified and adaptation is needed in the Zigbee integration.

  **Workaround:** Manually cherry-pick and apply commit with fix from ``main`` (commit hash: ``7ca42b219d1333b911d7671cf2a714bd93cbac45``).

.. rst-class:: v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0 v2-5-3 v2-5-2 v2-5-1 v2-5-0 v2-4-4 v2-4-3 v2-4-2 v2-4-1 v2-4-0 v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0

NCSIDB-1213: Subsequent Zigbee FOTA updates fail
  Once a Zigbee FOTA update is interrupted for any reason, the subsequent updates will fail until a device reboot.
  This is because :ref:`lib_dfu_target` resources are not freed.

  **Workaround:** Manually cherry-pick and apply commit with fix from ``main`` (commit hash: ``cef8a4b0e5afaed08627bcccbe2ac7b4b600978f``).

.. rst-class:: v2-5-3 v2-5-2 v2-5-1 v2-5-0

KRKNWK-18572: Bus fault when resetting the Zigbee light switch sample
  Usage of :kconfig:option:`CONFIG_RAM_POWER_DOWN_LIBRARY` leads to a bus fault at reset.

  **Workaround:** Choose :kconfig:option:`CONFIG_MINIMAL_LIBC` as :kconfig:option:`CONFIG_LIBC_IMPLEMENTATION`.

.. rst-class:: v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0

KRKNWK-16705: Router device is not fully operational in the distributed network
  The router node asserts in distributed network when a new device is being associated through the router.

  **Workaround:** Add a call to the :c:func:`zb_enable_distributed` function in your application after setting Zigbee Router role for the device.

.. rst-class:: v2-9-0-nRF54H20-1 v2-9-2 v2-9-1 v2-9-0 v2-8-0 v2-7-0 v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0 v2-5-3 v2-5-2 v2-5-1 v2-5-0 v2-4-4 v2-4-3 v2-4-2 v2-4-1 v2-4-0 v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0

KRKNWK-14024: Fatal error when the network coordinator factory resets in the Identify mode
  A fatal error occurs when the Zigbee network coordinator triggers factory reset in the Identify mode.

  **Workaround:** Modify your application, so that the factory reset is requested only after the Identify mode ends.

.. rst-class:: v2-9-0-nRF54H20-1 v2-9-2 v2-9-1 v2-9-0 v2-8-0 v2-7-0 v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0 v2-5-3 v2-5-2 v2-5-1 v2-5-0 v2-4-4 v2-4-3 v2-4-2 v2-4-1 v2-4-0 v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0

KRKNWK-12937: Activation of Sleepy End Device must be done at the very first commissioning procedure for Zigbee light switch sample
   After programming the Zigbee light switch sample and its first commissioning, Zigbee End Device joins the Zigbee network as a normal End Device. Pressing **Button 3** does not switch the device to the Sleepy End Device configuration.

   **Workaround:** Keep **Button 3** pressed during the first commissioning procedure.

.. rst-class:: v2-9-0-nRF54H20-1 v2-9-2 v2-9-1 v2-9-0 v2-8-0 v2-7-0 v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0 v2-5-3 v2-5-2 v2-5-1 v2-5-0 v2-4-4 v2-4-3 v2-4-2 v2-4-1 v2-4-0 v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0

KRKNWK-12615: Get Group Membership Command returns all groups the node is assigned to
   Get Group Membership Command returns all groups the node is assigned to regardless of the destination endpoint.

.. rst-class:: v2-9-0-nRF54H20-1 v2-9-2 v2-9-1 v2-9-0 v2-8-0 v2-7-0 v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0 v2-5-3 v2-5-2 v2-5-1 v2-5-0 v2-4-4 v2-4-3 v2-4-2 v2-4-1 v2-4-0 v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0

KRKNWK-12115: Simultaneous commissioning of many devices can cause the Coordinator device to assert
  The Zigbee Coordinator device can assert when multiple devices are being commissioned simultaneously.
  In some cases, the device can end up in the low memory state as the result.

  **Workaround:** To lower the likelihood of the Coordinator device asserting, increase its scheduler queue and buffer pool by completing the following steps:

  1. Create your own custom memory configuration file by creating an empty header file for your application, similar to :file:`include/zb_mem_config_custom.h` header file in the Zigbee light switch sample.
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

.. rst-class:: v2-8-0 v2-7-0 v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0 v2-5-3 v2-5-2 v2-5-1 v2-5-0 v2-4-4 v2-4-3 v2-4-2 v2-4-1 v2-4-0 v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0

KRKNWK-11826: Zigbee Router does not accept new child devices if the maximum number of children is reached
  Once the maximum number of children devices on a Zigbee Router is reached and one of them leaves the network, the Zigbee Router does not update the flags inside beacon frames to indicate that it cannot accept new devices.

.. rst-class:: v1-9-2 v1-9-1 v1-9-0 v1-8-0

KRKNWK-11704: NCP communication gets stuck
  The communication between the SoC and the NCP Host sometimes stops on the SoC side.
  The device neither sends nor accepts incoming packets.
  Currently, there is no workaround for this issue.

.. rst-class:: v1-9-2 v1-9-1 v1-9-0

KRKNWK-12522: Incorrect Read Attributes Response on reading multiple attributes when the first attribute is unsupported
   When reading multiple attributes at once and the first one is not supported, the Read Attributes Response contains two records for the first supported attribute.
   The first one record has the Status field filled with Unsupported Attribute whereas the second record contains actual data.

.. rst-class:: v2-9-0-nRF54H20-1 v2-9-2 v2-9-1 v2-9-0 v2-8-0 v2-7-0 v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0 v2-5-3 v2-5-2 v2-5-1 v2-5-0 v2-4-4 v2-4-3 v2-4-2 v2-4-1 v2-4-0 v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0

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

  1. Restore the default ``MinimumBlockPeriod`` attribute value by adding the following snippet in :file:`zigbee_fota.c` file to the :c:func:`zigbee_fota_abort` function and to the :c:func:`zigbee_fota_zcl_cb` function in the case where the ``ZB_ZCL_OTA_UPGRADE_STATUS_FINISH`` status is handled:

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

  #. In :file:`zboss/src/zcl/zcl_ota_upgrade_commands.c` file in the :file:`nrfxlib` directory, change the penultimate argument of the 360 :c:macro:`ZB_ZCL_OTA_UPGRADE_SEND_IMAGE_BLOCK_REQ` macro to ``delay`` in :c:func:`zb_zcl_ota_upgrade_send_block_requset` and :c:func:`resend_buffer` functions.

.. rst-class:: v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0

KRKNWK-11602: Zigbee device becomes not operable after receiving malformed packet
  When any Zigbee device receives a malformed packet that does not match the Zigbee packet structure, the ZBOSS stack asserts.
  In the |NCS| versions before the v1.9.0 release, the device is not automatically restarted.

  **Workaround:** Depends on your version of the |NCS|:

  * Before the |NCS| v1.9.0: Power-cycle the Zigbee device.
  * After and including the |NCS| v1.9.0: Wait for the device to restart automatically.

  Given these two options, it is recommended to upgrade your |NCS| version to the latest available one.

.. rst-class:: v2-9-0-nRF54H20-1 v2-9-2 v2-9-1 v2-9-0 v2-8-0 v2-7-0 v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0 v2-5-3 v2-5-2 v2-5-1 v2-5-0 v2-4-4 v2-4-3 v2-4-2 v2-4-1 v2-4-0 v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0

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
  A coordinator with the Zigbee Shell library component enabled could assert after executing the ``bdb factory_reset`` command.

  **Workaround:** Call the ``bdb_reset_via_local_action ()`` function twice to remove all the network information.

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
    Because of changes to the ZBOSS API, the Zigbee Shell library cannot be enabled when Zigbee samples are built with the ZBOSS development libraries.

    **Workaround:** Use only the production version of ZBOSS when using the Zigbee Shell library.

.. rst-class:: v1-5-0

KRKNWK-9145: Corrupted payload in commands of the Scenes cluster
  When receiving Scenes cluster commands, the payload is corrupted when using the ZBOSS production libraries.

  **Workaround:** Use the development version of ZBOSS.

.. rst-class:: v1-4-2 v1-4-1 v1-4-0

KRKNWK-7836: Coordinator asserting when flooded with ZDO commands
  Executing a high number of ZDO commands can cause assert on the coordinator with the Zigbee Shell library component enabled.

.. rst-class:: v1-3-1 v1-3-0

KRKNWK-6073: Potential delay during FOTA
  There might be a noticeable delay (~220 ms) between calling the ZBOSS API and on-the-air activity.

Applications
************

The issues in this section are related to :ref:`applications`.

Asset Tracker v2
================

The issues in this section are related to the Asset Tracker v2 application.
This application has been removed in the |NCS| v3.0.0 and replaced by `Asset Tracker Template <Asset Tracker Template_>`_.

.. rst-class:: v2-9-0-nRF54H20-1 v2-9-2 v2-9-1 v2-9-0 v2-8-0

NCSDK-30983: Asset Tracker v2 with :file:`overlay-lwm2m` (LwM2M overlay) reboots after GNSS fix
  This issue affects Asset Tracker v2 with the LwM2M overlay.
  After a GNSS fix, the device reboots due to a NULL pointer exception.

  **Affected platforms:** nRF91 Series

.. rst-class:: v2-2-0

CIA-845: The application cannot be built with :file:`overlay-carrier.conf` (carrier library) enabled for Nordic Thingy:91
  Building with :ref:`liblwm2m_carrier_readme` library enabled for Nordic Thingy:91 will result in a ``FLASH`` overflow and a build error.

  **Affected platforms:** Thingy:91

.. rst-class:: v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0

CIA-463: Wrong network mode parameter reported to cloud
  The network mode string present in ``deviceInfo`` (nRF Cloud) and ``dev`` (Azure IoT Hub and AWS IoT) JSON objects that is reported to cloud might contain wrong network modes.
  The network mode string contains the network modes that the modem is configured to use, not what the modem actually connects to the LTE network with.

  **Affected platforms:** nRF9160

.. rst-class:: v1-9-2 v1-9-1 v1-9-0

NCSDK-14235: Timestamps that are sent in cloud messages drift over time
  Due to a bug in the :ref:`lib_date_time` library, timestamps that are sent to cloud drift because they are calculated incorrectly.

  **Affected platforms:** nRF9160

.. rst-class:: v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-0

CIA-604: Asset Tracker v2 cannot be built for the ``thingy91_nrf9160_ns`` board target with ``SECURE_BOOT`` enabled
  Due to the use of static partitions with the Thingy:91, there is insufficient room in the flash memory to enable both the primary and secondary bootloaders.

  **Affected platforms:** Thingy:91

.. rst-class:: v2-0-2 v2-0-1 v2-0-0

CIA-661: Asset Tracker v2 application configured for LwM2M cannot be built for the ``nrf9160dk_nrf9160_ns`` board target with modem traces or Memfault enabled
  The Asset Tracker v2 application configured for LwM2M cannot be built for the ``nrf9160dk_nrf9160_ns`` board target with :kconfig:option:`CONFIG_NRF_MODEM_LIB_TRACE` for modem traces or :file:`overlay-memfault.conf` for Memfault due to memory constraints.

  **Affected platforms:** nRF9160

  **Workaround:** Use one of the following workarounds for modem traces:

  * Use Secure Partition Manager instead of TF-M by setting ``CONFIG_SPM`` to ``y`` and :kconfig:option:`CONFIG_BUILD_WITH_TFM` to ``n``.
  * Reduce the value of :kconfig:option:`CONFIG_NRF_MODEM_LIB_SHMEM_TRACE_SIZE` to 8 Kb, however, this might lead to loss of modem traces.

  For Memfault, use Secure Partition Manager instead of TF-M by setting ``CONFIG_SPM`` to ``y`` and :kconfig:option:`CONFIG_BUILD_WITH_TFM` to ``n``.

.. rst-class:: v2-4-3 v2-4-2 v2-4-1 v2-4-0 v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0

CIA-890: The application cannot be built with :file:`overlay-debug.conf` and :kconfig:option:`CONFIG_DEBUG_OPTIMIZATIONS` set to ``y``
  Due to insufficient flash space for the application when it is not optimized, the Asset Tracker v2 application cannot be built with :file:`overlay-debug.conf` and :kconfig:option:`CONFIG_DEBUG_OPTIMIZATIONS` set to ``y``.

  **Affected platforms:** nRF9160

  **Workaround:** To free up flash space when debugging locally, comment out the following Kconfig options in the :file:`prj.conf` file:

  * :kconfig:option:`CONFIG_BOOTLOADER_MCUBOOT`
  * :kconfig:option:`CONFIG_IMG_MANAGER`
  * :kconfig:option:`CONFIG_MCUBOOT_IMG_MANAGER`
  * :kconfig:option:`CONFIG_IMG_ERASE_PROGRESSIVELY`
  * :kconfig:option:`CONFIG_BUILD_S1_VARIANT`

  This removes the partitions for the MCUboot bootloader, the secondary bootloader, and the secondary application image slot.
  Any functionality depending on those will not work with this configuration.

  Alternatively, disable logging for non-relevant modules or libraries in the :file:`overlay-debug.conf` file until the image fits in flash.

nRF9160: Asset tracker
======================

The issues in this section are related to the nRF9160: Asset tracker application.
This application was removed in :ref:`nRF Connect SDK v1.9.0 <ncs_release_notes_190>` and replaced by Asset Tracker v2.

.. rst-class:: v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0

NCSDK-6898: Setting :kconfig:option:`CONFIG_SECURE_BOOT` does not work
  The immutable bootloader is not able to find the required metadata in the MCUboot image.
  See the related NCSDK-6898 known issue in `Build system`_ for more details.

  **Affected platforms:** nRF9160

  **Workaround:** Set :kconfig:option:`CONFIG_FW_INFO` in MCUboot.

.. rst-class:: v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0

External antenna performance setting
  The preprogrammed Asset Tracker does not come with the best external antenna performance.

  **Affected platforms:** nRF9160

  **Workaround:** If you are using nRF9160 DK v0.15.0 or higher and Thingy:91 v1.4.0 or higher, set :kconfig:option:`CONFIG_NRF9160_GPS_ANTENNA_EXTERNAL` to ``y``.
  Alternatively, for nRF9160 DK v0.15.0, you can set the :kconfig:option:`CONFIG_NRF9160_GPS_COEX0_STRING` option to ``AT%XCOEX0`` when building the preprogrammed Asset Tracker to achieve the best external antenna performance.

.. rst-class:: v1-3-2 v1-3-1 v1-3-0

NCSDK-5574: Warnings during FOTA
  The nRF9160: Asset Tracker application prints warnings and error messages during successful FOTA.

  **Affected platforms:** nRF9160

.. rst-class:: v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0 v1-0-0 v0-4-0 v0-3-0

NCSDK-6689: High current consumption in Asset Tracker
  The nRF9160: Asset Tracker might show up to 2.5 mA current consumption in idle mode with :kconfig:option:`CONFIG_POWER_OPTIMIZATION_ENABLE` set to ``y``.

  **Affected platforms:** nRF9160

.. rst-class:: v1-0-0 v0-4-0 v0-3-0

Sending data before connecting to nRF Cloud
  The nRF9160: Asset Tracker application does not wait for connection to nRF Cloud before trying to send data.
  This causes the application to crash if the user toggles one of the switches before the kit is connected to the cloud.

  **Affected platforms:** nRF9160

.. rst-class:: v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0 v1-0-0 v0-4-0 v0-3-0

IRIS-2676: Missing support for FOTA on nRF Cloud
  The nRF9160: Asset Tracker application does not support the nRF Cloud FOTA_v2 protocol.

  **Affected platforms:** nRF9160

  **Workaround:** The implementation for supporting the nRF Cloud FOTA_v2 can be found in the following commits:

					* cef289b559b92186cc54f0257b8c9adc0997f334
					* 156d4cf3a568869adca445d43a786d819ae10250
					* f520159f0415f011ae66efb816384a8f7bade83d

Serial LTE Modem
================

The issues in this section are related to the :ref:`serial_lte_modem` application.

.. rst-class:: wontfix v3-1-1 v3-1-0 v3-0-2 v3-0-1 v3-0-0 v2-9-0-nRF54H20-1 v2-9-2 v2-9-1 v2-9-0 v2-8-0 v2-7-0 v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0 v2-5-3 v2-5-2 v2-5-1 v2-5-0 v2-4-4 v2-4-3 v2-4-2 v2-4-1 v2-4-0

NCSDK-20457: Modem traces captured through UART are corrupted if RTT logs are simultaneously captured
  When capturing modem traces through UART with the `Cellular Monitor app`_ and simultaneously capturing RTT logs, for example, with J-Link RTT Viewer, the modem trace misses packets, and captured packets might have incorrect information.

  **Affected platforms:** nRF9160, nRF9161, nRF9151

  **Workaround:** If you need to capture modem traces and RTT logs at the same time, add the following change to :file:`nrf9160dk_nrf9160_ns.overlay` or :file:`nrf9161dk_nrf9161_ns.overlay`, depending on the board you are using.
  Otherwise, you can choose not to capture RTT logs.
  Having RTT logs enabled does not cause this issue.

  .. parsed-literal::
     :class: highlight

     &uart1 {
      hw-flow-control;
     };

  This increases the overall power consumption even when Serial LTE Modem is in sleep mode.

.. rst-class:: v2-5-0

NCSDK-24135: Serial LTE Modem (SLM) attempts to use UART hardware flow control even though Connectivity bridge does not support it
  With Thingy:91, Connectivity bridge in the nRF52840 SoC terminates the USB traffic and sends the traffic through UART to SLM in the nRF9160 SiP.
  The Connectivity bridge does not enable UART hardware flow control and since both ends need to enable it, SLM should not enable it either.
  Without hardware flow control, the buffer sizes must be set accordingly for the worst-case scenario.
  In this case, SLM uses the default buffer size of 3x256 bytes and will drop incoming UART traffic after the buffers are full.

  **Affected platforms:** Thingy:91

  **Workaround:** Set the :ref:`CONFIG_SLM_UART_RX_BUF_SIZE <CONFIG_SLM_UART_RX_BUF_SIZE>` Kconfig option to ``2048`` to ensure that there is adequate buffer space to receive traffic from the Connectivity bridge and disable the ``hw-flow-control`` from :file:`boards\thingy91_nrf9160_ns.overlay`.
  If even larger buffer spaces are required, set the :kconfig:option:`CONFIG_BRIDGE_BUF_SIZE` Kconfig option for Connectivity bridge and the :ref:`CONFIG_SLM_UART_RX_BUF_SIZE <CONFIG_SLM_UART_RX_BUF_SIZE>` Kconfig option for SLM, must be set accordingly.

.. rst-class:: v3-1-1 v3-1-0 v3-0-2 v3-0-1 v3-0-0 v2-9-0-nRF54H20-1 v2-9-2 v2-9-1 v2-9-0 v2-8-0 v2-7-0 v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0 v2-5-3 v2-5-2 v2-5-1 v2-5-0

NCSDK-23733: Full modem firmware update issue on the nRF91x1 DKs
  Failures of full modem firmware update on the nRF91x1 DK have been observed in certain conditions.
  If RTT is enabled (:kconfig:option:`CONFIG_USE_SEGGER_RTT` set to ``y``) and connected during the activation of the new firmware (when the modem or the device is reset), the update can fail with: ``XFOTA: 5,1,-116``

  **Affected platforms:** nRF9161, nRF9151

  **Workaround:** Make sure that RTT is disconnected when activating the new firmware.
  It can be disconnected right before triggering the reset and connected back once the update is complete.

.. rst-class:: v2-4-3 v2-4-2 v2-4-1 v2-4-0 v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0

NCSDK-13895: Build failure for target Thingy:91 with secure_bootloader overlay
  Building the application for Thingy:91 fails if ``secure_bootloader`` overlay is included.

  **Affected platforms:** Thingy:91, nRF9160

.. rst-class:: v2-3-0

NCSDK-20047: SLM logging over RTT is not available
  There is a conflict with MCUboot RTT logging.
  In order to save power, SLM configures MCUboot to use RTT instead of UART for logging.
  SLM itself uses RTT for logging as well.
  With a recent change, MCUboot exclusively takes control of the RTT logging, which causes the conflict.

  **Affected platforms:** nRF9160

  **Workaround:** Remove ``CONFIG_USE_SEGGER_RTT=y`` and ``CONFIG_RTT_CONSOLE=y`` from :file:`child_image\mcuboot.conf`.

.. rst-class:: v1-0-0 v0-4-0 v0-3-0

Modem FW reset on debugger connection through SWD
  If a debugger (for example, J-Link) is connected through SWD to the nRF9160, the modem firmware will reset.
  Therefore, the LTE modem cannot be operational during debug sessions.

  **Affected platforms:** nRF9160

nRF Desktop
===========

The issues in this section are related to the :ref:`nrf_desktop` application.

.. note::
    nRF Desktop is also affected by the Bluetooth LE issue :ref:`NCSDK-19865 <ncsdk_19865>`.

    nRF Desktop is also affected by :ref:`hogp_readme` library's issue :ref:`NCSDK-30288 <ncsdk_30288>`.
    The issue might cause accessing data under the ``NULL`` pointer in case of Bluetooth disconnection while forwarding a configuration channel operation in the :ref:`nrf_desktop_hid_forward`.

.. rst-class:: v3-1-1 v3-1-0

NCSDK-35650: Possible problems with sending HID input reports over Bluetooth LE after reconnection
  Bluetooth stack might leak ATT TX buffers during disconnection if an application delays ATT sent callback until data transmission is done by Bluetooth LE controller (:kconfig:option:`CONFIG_BT_ATT_SENT_CB_AFTER_TX`).
  Enabling the feature introduces an additional network buffer reference with the :c:func:`net_buf_ref` function.
  This extends the lifetime of the network buffer representation until data transmission is confirmed by an ACK from the remote.
  The reference is removed with the :c:func:`net_buf_unref` function when the TX callback passed to the :c:func:`bt_l2cap_send_pdu` function is called.
  The TX callback might not be called for data sent over Bluetooth LE during the link disconnection.
  This leads to missing net buffer reference removal.
  Eventually, the buffers cannot be reused after Bluetooth LE reconnection.

  **Workaround:** Apply the fix from `sdk-zephyr PR #3323`_.

.. rst-class:: v3-1-1 v3-1-0

NCSDK-34743: System state indication LED does not work on nRF54L15 DK
  The system state indication LED is kept turned off, because the PWM hardware peripheral attempts to drive the LED instead of GPIO.
  On the nRF54L05, nRF54L10 and nRF54L15 SoCs, you can only use the **GPIO1** port for PWM hardware peripheral output.
  The system state indication LED is connected to **GPIO2**.

  **Affected platforms:** nRF54L15 DK

  **Workaround:** Manually cherry-pick and apply the commit with the fix from the ``main`` branch (commit hash: ``744701bdf6f2ce7e3421649644ccd86cad8e26b4``).

.. rst-class:: v3-1-1 v3-1-0

NCSDK-34299: nRF Desktop application does not build or run due to the IronSide SE migration
  The :ref:`nrf_desktop` application is currently incompatible with the latest |NCS| transition to the IronSide SE solution - bundle 22.1.0 and higher for the ``nrf54h20dk/nrf54h20/cpuapp`` board target.
  As a result, you cannot use this application with that board target in any of the affected releases.

  **Affected platforms:** nRF54H20

  **Workaround:** Use a future |NCS| release that includes IronSide SE support in the :ref:`nrf_desktop` application, or use the |NCS| v3.0.0 release with the SUIT SDSC bundle (version up to 0.9.6).

.. rst-class:: v3-0-2 v3-0-1 v3-0-0

NCSDK-34584: The :ref:`nrf_desktop` application uses incorrect partition map for the nRF54L10 SoC
  In the :ref:`nrf_desktop` application, a partition map is incorrectly defined for each build configuration that is supported by the ``nrf54l15dk/nrf54l10/cpuapp`` board target.
  In the |NCS| releases affected by this issue, it was assumed that the NVM size of the nRF54L10 SoC was 10 KB larger than the actual one.
  The NVM size of the nRF54L10 SoC is equal to 1012 KB.

  The workaround fix for the nRF54L10 partition map is a breaking change and cannot be performed using DFU.
  The DFU procedure will fail if you attempt to upgrade the application firmware based on one of the |NCS| releases that is affected by this issue.

  **Affected platforms:** nRF54L10

  **Workaround:** Manually cherry-pick and apply the commit with the fix from the ``main`` branch of the ``sdk-nrf`` repository (commit hash: ``2658ceea216a9020450206347ad8a08d44b53e5c``).
  Additionally, you need to manually update the :file:`boards/nordic/nrf54l15dk/nrf54l15dk_nrf54l10_cpuapp.dts` DTS file in your local copy of the ``sdk-zephyr`` repository to avoid issues with build asserts that are used for RRAM validation.
  Correct the ``cpuapp_rram`` DTS node in the following way:

  .. code-block::

     &cpuapp_rram {
       reg = <0x0 DT_SIZE_K(1012)>;
     };

  Then, correct the ``storage_partition`` node in the following way:

  .. code-block::

     storage_partition: partition@f6000 {
       reg = <0xf6000 DT_SIZE_K(28)>;
     };

.. rst-class:: v3-0-2 v3-0-1 v3-0-0 v2-9-0-nRF54H20-1 v2-9-2 v2-9-1 v2-9-0 v2-8-0 v2-7-0

NCSDK-30504: USB High-Speed slight HID report rate drop
  While Bluetooth is in use, the device is sometimes too slow to provide the HID report over USB in time, which causes HID report rate drop to around 7999.9 Hz.
  The slight HID report rate drop over USB High-Speed might be visible, for example, during Bluetooth LE advertising.

  **Affected platforms:** nRF54H20

  **Workaround:** Disable Bluetooth LE advertising completely by disabling the :ref:`CONFIG_DESKTOP_BLE_ADV <config_desktop_app_options>` Kconfig option.
  This is not the correct way of disabling Bluetooth completely, and the Bluetooth stack will still be compiled in.
  You can use this workaround for testing the full 8000 Hz HID report rate.
  You can also enable the :ref:`CONFIG_DESKTOP_USB_HID_REPORT_SENT_ON_SOF <config_desktop_app_options>` Kconfig option to further improve the report rate.

.. rst-class:: v2-9-0-nRF54H20-1 v2-9-2 v2-9-1 v2-9-0 v2-8-0

NCSDK-30503: USB High-Speed HID report rate drops when motion is simulated
  The USB High-Speed HID report rate drops to around 5100 Hz when motion is simulated (the :ref:`CONFIG_DESKTOP_MOTION_SIMULATED_ENABLE <config_desktop_app_options>` Kconfig option is enabled).
  This happens due to the Zephyr's :ref:`zephyr:pm-system` integration issues.
  The system power management is not aware that a USB poll is about to happen after passing a HID report to the USB stack.
  Because of that, the system might enter sleep states, which delays providing subsequent HID report.

  **Affected platforms:** nRF54H20

  **Workaround:** Manually cherry-pick and apply the commit with the fix from the ``main`` branch (commit hash: ``5b2912dded7d71e701670eda14cdfbd5b837ac60``).

.. rst-class:: v2-7-0

NCSDK-27983: No Bluetooth advertising after a software reset
  The software reset fails to properly reboot the nRF54H20 device, resulting in malfunction of Bluetooth LE advertising.

  **Affected platforms:** nRF54H20

  **Workaround:** Press the **RESET** button on the nRF54H20 DK after performing a factory or software reset of the device.

.. rst-class:: v2-8-0

NCSDK-30261: Bluetooth LE connection parameter update might be triggered multiple times in the :ref:`nrf_desktop_ble_conn_params`
  An nRF Desktop dongle without LLPM support updates the connection parameters of a connected peripheral with LLPM support in a never-ending loop.
  The connection parameter update should be applied only when required connection parameters change.

  **Workaround:** Manually cherry-pick and apply the commit with the fix from the ``main`` branch (commit hash: ``cfbe5ba7f7fdc5a0fa4114f365a2cc1bab0db150``).

.. rst-class:: v2-4-3 v2-4-2 v2-4-1 v2-4-0 v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0

NCSDK-23704: Too small heap size on nRF5340 DK
  The heap memory pool size (:kconfig:option:`CONFIG_HEAP_MEM_POOL_SIZE`) is too small, which might cause an Event Manager out of memory (OOM) error in runtime.
  Too small heap size might also lead to issues when allocating memory for UUID in :ref:`gatt_dm_readme` during peer discovery, because the library also uses the heap to allocate internal data.

  **Affected platforms:** nRF5340 DK

  **Workaround:** Manually cherry-pick and apply commit with fix from ``main`` (commit hash: ``be97ae3074c38b7987d5183b1c09995cf19d61e8``).

.. rst-class:: v2-5-3 v2-5-2 v2-5-1 v2-5-0 v2-4-4 v2-4-3 v2-4-2 v2-4-1 v2-4-0 v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0

NCSDK-25928: :ref:`nrf_desktop_hid_state` keeps sending empty HID reports to lower priority subscriber after higher priority subscriber connects
  After a higher priority subscriber connects, the :ref:`nrf_desktop_hid_state` sends empty HID reports to a lower priority subscriber to report release of all pressed buttons.
  Because of an implementation bug related to invalid handling of empty report data (accessing memory under ``NULL`` pointer), the empty HID reports are sent to the lower priority subscriber in a never-ending loop (an empty HID report with a given ID is expected to be sent once).
  The issue replicates if an nRF Desktop peripheral connects to the host over USB while maintaining Bluetooth LE connection.

  **Workaround:** Manually cherry-pick and apply the commit with the fix from the ``main`` branch (commit hash: ``a87407fc29514b68a7bdaea5554f7b755466a77b``).

.. rst-class:: wontfix v3-1-1 v3-1-0 v3-0-2 v3-0-1 v3-0-0 v2-9-0-nRF54H20-1 v2-9-2 v2-9-1 v2-9-0 v2-8-0 v2-7-0 v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0 v2-5-3 v2-5-2 v2-5-1 v2-5-0 v2-4-4 v2-4-3 v2-4-2 v2-4-1 v2-4-0 v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0 v1-0-0

NCSDK-8304: HID configurator issues for peripherals connected over Bluetooth LE to Linux host
  Using :ref:`nrf_desktop_config_channel_script` for peripherals connected to host directly over Bluetooth LE might result in receiving improper HID feature report ID.
  In such case, the device will provide HID input reports, but it cannot be configured with the HID configurator.

  **Workaround:** Use BlueZ in version 5.56 or higher.

.. rst-class:: v2-4-3 v2-4-2 v2-4-1 v2-4-0 v2-3-0 v2-2-0 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0

NCSDK-22953: A HID output report interrupts scanning in the :ref:`nrf_desktop_ble_scan` even if there are no peripherals connected
  Connecting a HID dongle over USB to a HID host might lead to submitting a HID output report and stopping Bluetooth LE scanning even if there are no peripherals connected through the dongle.
  In this case, scanning stop delays establishing a connection with peripherals.

  **Workaround:** Manually cherry-pick and apply the commit with the fix from the ``main`` branch (commit hash: ``b4860f7c9475ff8e61a4c3b907968987bb6311bd``).

.. rst-class:: v2-4-3 v2-4-2 v2-4-1 v2-4-0 v2-3-0 v2-2-0 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0

NCSDK-22810: Assertion fail when logging :c:struct:`config_event` with an unknown status
  The :c:member:`config_event.status` field is set according to data received from the host computer over :ref:`nrf_desktop_config_channel`.
  Setting it to a value unknown to an nRF Desktop device should not cause assertion failure.

  **Workaround:** Manually cherry-pick and apply the commit with the fix from the ``main`` branch (commit hash: ``a49008a980dbddcb48c80736f76fedbfda9c03f6``).

.. rst-class:: v2-3-0 v2-2-0 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0

NCSDK-20366: Possible bus fault in :ref:`nrf_desktop_selector` while toggling a hardware selector
  The GPIO port index calculation in the GPIO interrupt handling function assumes that GPIO devices in Zephyr are placed in memory next to each other with order matching GPIO port indexes, which might not be true.
  Using an invalid port index in the function leads to undefined behavior.

  **Workaround:** Manually cherry-pick and apply the commit with the fix from the ``main`` branch (commit hash: ``6179413498d1ae1c9c79255aeca2739d108e482d``).

.. rst-class:: v2-2-0

NCSDK-19970: MCUboot bootloader fails to swap images on nRF52840 DK that uses external flash
   The MCUboot bootloader cannot access external flash because the chosen ``nordic,pm-ext-flash`` is not defined in the MCUboot child image's devicetree.
   As a result ``nrf52840dk_nrf52840`` using ``prj_mcuboot_qspi.conf`` configuration fails to swap images after a complete image transfer.

   **Affected platforms:** nRF52840

   **Workaround**: Manually cherry-pick and apply the commit with the fix from the ``main`` branch (commit hash: ``7cea1b7e681a39ce2e2143b6b03132d95b7606ab``).
   Make sure to also cherry-pick and apply the commit that fixes a build system issue (commit ``ec23df1fa305e99194ceac87a028f6da206a3ff1`` from ``main`` branch).
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

NCSDK-12337: Possible assertion failure at boot of a USB-connected host
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

NCSDK-12020: Current consumption for Gaming Mouse increased by 1400 mA
  When not in the sleep mode, the Gaming Mouse reference design has current consumption higher by 1400 mA.

  **Workaround:** Change ``pwm_pin_set_cycles`` to ``pwm_pin_set_usec`` in function :c:func:`led_pwm_set_brightness` in Zephyr's driver :file:`led_pwm.c` file.

.. rst-class:: v1-9-2 v1-9-1 v1-9-0

NCSDK-14117: Build fails for nRF52840DK in the ``prj_b0_wwcb`` configuration
  The build failure is caused by outdated Kconfig options in the nRF52840 DK's ``prj_b0_wwcb`` configuration.
  The nRF52840 DK's ``prj_b0_wwcb`` configuration does not explicitly define static partition map either.

  **Affected platforms:** nRF52840

  **Workaround:** Manually cherry-pick and apply commit with fix from ``main`` (commit hash: ``cf4c465aceeb00d83a4f50edf67ce8c26427ac52``).

.. rst-class:: v1-2-1 v1-2-0 v1-1-0 v1-0-0

Reconnection issues on some operating systems
  On some operating systems, the :ref:`nrf_desktop` application is unable to reconnect to a Bluetooth host.

.. _known_issues_nrf5340audio:

nRF5340 Audio
=============

The issues in this section are related to the :ref:`nrf53_audio_app` application.

.. rst-class:: v3-1-1 v3-1-0 v3-0-2 v3-0-1 v3-0-0 v2-9-0-nRF54H20-1 v2-9-2 v2-9-1 v2-9-0 v2-8-0 v2-7-0 v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0 v2-5-3 v2-5-2 v2-5-1 v2-5-0 v2-4-4 v2-4-3 v2-4-2 v2-4-1 v2-4-0 v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0

OCT-3406: If an audio stream was interrupted, stopped and restarted, or a received timestamp was wrong, the timestamp may be estimated
  This may result in the receiver continuously estimating timestamps, triggering prints "Invalid sdu_ref_us delta.." depending on the chosen log level.
  In this condition, the timestamp would be submitted with an offset to the rest of the system.

.. rst-class:: v3-1-1 v3-1-0

OCT-3265: Test with nRF5340 DK as sink will cause the I2S to interfere with other signals on the same pins
  Using the nRF5340 DK to drive an external digital-to-analog converter through I2S will not produce audio.
  This is due to the fact that the I2S signals are multiplexed with the QSPI flash signals on the nRF5340 DK.

.. rst-class::  v3-1-1 v3-1-0

OCT-3432: Selecting 32 bit (``CONFIG_AUDIO_BIT_DEPTH_32`` = `y`) as the I2S input on gateway may cause choppy audio
  Using 32-bit I2S input requires significantly more buffer handling and CPU time.
  Depending on the configuration, this may overload the system and cause I2S RX overruns.

.. rst-class:: v3-0-2 v3-0-1 v3-0-0

OCT-3368: For 16 and 24 kHz, the application may repeatedly print "audio_sync_timer: Unable to get new CC value"
  This may cause degraded audio quality.
  This issue is related to the issue OCT-2585 and it shows the same behavior.

.. rst-class:: v3-1-1 v3-1-0 v3-0-2 v3-0-1 v3-0-0 v2-9-0-nRF54H20-1 v2-9-2 v2-9-1 v2-9-0 v2-8-0

OCT-3179: CIS headset, potential for not establishing presentation synchronization lock in bidirectional mode
  If the CIS gateway is reset shortly after the headset has established synchronization lock, this issue might be triggered.

  **Affected platforms:** nRF5340 Audio DK

.. rst-class:: v2-9-0-nRF54H20-1 v2-9-2 v2-9-1 v2-9-0 v2-8-0

OCT-3174: Host on CIS gateway might throw assert if a connected headset is hard reset
  During a CIS stereo stream, if a headset suddenly disappears during the active stream, this issue might occur.

  **Affected platforms:** nRF5340 Audio DK

.. rst-class:: v2-9-0-nRF54H20-1 v2-9-2 v2-9-1 v2-9-0 v2-8-0

OCT-3152: Public address for broadcast source
  When trying to use a public address for the broadcast source, the address will still change on each boot and appears to be random.

  **Affected platforms:** nRF5340 Audio DK

.. rst-class:: v2-9-0-nRF54H20-1 v2-9-2 v2-9-1 v2-9-0 v2-8-0 v2-7-0 v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0 v2-5-3 v2-5-2 v2-5-1 v2-5-0 v2-4-4 v2-4-3 v2-4-2 v2-4-1 v2-4-0 v2-3-0 v2-2-0

OCT-2070: Detection issues with USB-C to USB-C connection
  Using USB-C to USB-C when connecting an nRF5340 Audio DK to PC is not correctly detected on some Windows systems.

  **Affected platforms:** nRF5340 Audio DK

.. rst-class:: v3-1-1 v3-1-0 v3-0-2 v3-0-1 v3-0-0 v2-9-0-nRF54H20-1 v2-9-2 v2-9-1 v2-9-0 v2-8-0 v2-7-0 v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0 v2-5-3 v2-5-2 v2-5-1 v2-5-0 v2-4-4 v2-4-3 v2-4-2 v2-4-1 v2-4-0 v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0

OCT-2154: USB audio interface does not work correctly on macOS
  The audio stream is intermittent on the headset side after connecting the gateway to a Mac computer and starting the audio stream.
  This issue occurs sporadically after building the nRF5340 Audio application with the default USB port as the audio source.

  **Affected platforms:** nRF5340 Audio DK

.. rst-class:: v3-1-1 v3-1-0 v3-0-2 v3-0-1 v3-0-0 v2-9-0-nRF54H20-1 v2-9-2 v2-9-1 v2-9-0 v2-8-0 v2-7-0 v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0

OCT-2172: The headset volume is not stored persistently
  This means the volume will fall back to default level after a reset.

  **Affected platforms:** nRF5340 Audio DK

.. rst-class:: v2-7-0 v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0 v2-5-3 v2-5-2 v2-5-1 v2-5-0 v2-4-4 v2-4-3 v2-4-2 v2-4-1 v2-4-0 v2-3-0

OCT-2325: Difficult to remove a failed DFU image
  If a problematic DFU image is deployed, causing the image-swap at boot to fail, the device might appear bricked with no obvious way of recovery.

  **Affected platforms:** nRF5340 Audio DK

.. rst-class:: v2-2-0

OCT-2347: Stream reestablishment issues in CIS
  In the CIS mode, if a stream is running and the headset is reset, the gateway cannot reestablish the stream properly.

  **Affected platforms:** nRF5340 Audio DK

.. rst-class:: wontfix v2-9-0-nRF54H20-1 v2-9-2 v2-9-1 v2-9-0 v2-8-0 v2-7-0 v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0 v2-5-3 v2-5-2 v2-5-1 v2-5-0 v2-4-4 v2-4-3 v2-4-2 v2-4-1 v2-4-0 v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0

OCT-2401: The HW codec has a variable (0-20 µs) audio interface (I2S) lock variability
  This will cause a static offset of the stream, which will cause an undesired extra L-R sync difference.

  **Affected platforms:** nRF5340 Audio DK

.. rst-class:: v2-3-0

OCT-2470: Discovery of Media Control Service fails
  When restarting or resetting the gateway, one or more headsets might run into a condition where the discovery of the Media Control Service fails.

   **Affected platforms:** nRF5340 Audio DK

.. rst-class:: v2-3-0

OCT-2472: Headset fault upon gateway reset in the bidirectional stream mode
  The headset might react with a usage fault when the :kconfig:option:`CONFIG_STREAM_BIDIRECTIONAL` application Kconfig option is set to ``y`` and the gateway is reset during a stream.
  This issue is under investigation.

  **Affected platforms:** nRF5340 Audio DK

.. rst-class:: wontfix v2-9-0-nRF54H20-1 v2-9-2 v2-9-1 v2-9-0 v2-8-0 v2-7-0 v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0 v2-5-3 v2-5-2 v2-5-1 v2-5-0 v2-4-4 v2-4-3 v2-4-2 v2-4-1 v2-4-0 v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0

OCT-2501: Charging over seven hours results in error
  Since the nRF5340 Audio DK uses a large battery, the nPM1100 can go into error when charging time surpasses 7 hours.
  This is because of a protection timeout on the PMIC.
  The charging is stopped when this error occurs.

  **Affected platforms:** nRF5340 Audio DK

  **Workaround:** To start the charging again, turn the nRF5340 Audio DK off and then on again.

.. rst-class:: v3-1-1 v3-1-0 v3-0-2 v3-0-1 v3-0-0 v2-9-0-nRF54H20-1 v2-9-2 v2-9-1 v2-9-0 v2-8-0 v2-7-0 v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0 v2-5-3 v2-5-2 v2-5-1 v2-5-0 v2-4-4 v2-4-3 v2-4-2 v2-4-1 v2-4-0

OCT-2539: Presentation delay might not work as expected under some configurations
  The data is not presented at the correct time.

  **Affected platforms:** nRF5340 Audio DK

.. rst-class:: v2-7-0 v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0 v2-5-3 v2-5-2 v2-5-1 v2-5-0 v2-4-4 v2-4-3 v2-4-2 v2-4-1 v2-4-0

OCT-2551: GPIO pin forwarding to the network core is not done for the **RGB2** LED pins
  This means that the **RGB2** LED does not reflect the controller status.

  **Affected platforms:** nRF5340 Audio DK

.. rst-class:: v2-4-3 v2-4-2 v2-4-1 v2-4-0

OCT-2558: Endpoint in BIS headset not set correctly
  This might impact possibility to adjust volume for right headset and might impact broadcast switching.

  **Affected platforms:** nRF5340 Audio DK

.. rst-class:: v2-5-3 v2-5-2 v2-5-1 v2-5-0 v2-4-4 v2-4-3 v2-4-2 v2-4-1 v2-4-0

OCT-2569: BIS headset stuck if toggling gateway power quickly
  BIS headset might enter an unwanted state if gateway power is toggled quickly or if headset is moved out of radio range.

  **Affected platforms:** nRF5340 Audio DK

  **Workaround:** Reset BIS headset

.. rst-class:: v2-9-0-nRF54H20-1 v2-9-2 v2-9-1 v2-9-0 v2-8-0 v2-7-0 v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0 v2-5-3 v2-5-2 v2-5-1 v2-5-0 v2-4-4 v2-4-3 v2-4-2 v2-4-1 v2-4-0

OCT-2585: Initial L-R sync might lock with an offset
  The left and right headset might lock as intended, but there will be a small static time offset between the two headsets.

  **Affected platforms:** nRF5340 Audio DK

.. rst-class:: v2-4-3 v2-4-2 v2-4-1 v2-4-0

OCT-2587: A CIS gateway will try to connect to a BIS gateway
  Non-connectable advertisement packets on the CIS gateway are not filtered out.
  Thereby seeing the BIS gateway, recognizing the name and trying to connect.
  Resulting in an error.

  **Affected platforms:** nRF5340 Audio DK

  **Workaround:** Use different :kconfig:option:`CONFIG_BT_DEVICE_NAME` for BIS and CIS.

.. rst-class:: v2-6-0 v2-5-3 v2-5-2 v2-5-1 v2-5-0

OCT-2712: Gateway cannot properly handle a connected headset if it continues to advertise
  The gateway will try to connect to a headset that is already connected.

  **Affected platforms:** nRF5340 Audio DK

.. rst-class:: v2-5-3 v2-5-2 v2-5-1 v2-5-0

OCT-2713: Gateway can only read one PAC record from each headset
  Some headsets might have several PAC records containing different supported configurations.
  Then the most optimal configuration might not be used, or the gateway might not be able to configure a headset since it does not know about all supported configurations.

  **Affected platforms:** nRF5340 Audio DK

.. rst-class:: v2-6-0 v2-5-3 v2-5-2 v2-5-1 v2-5-0

OCT-2715: There can be a state mismatch between streaming and media control in certain scenarios
  For example, if a stream is in the playing state but media control is in the pause state, it might not be possible to play or pause.

  **Affected platforms:** nRF5340 Audio DK

.. rst-class:: v2-5-3 v2-5-2 v2-5-1 v2-5-0 v2-4-4 v2-4-3 v2-4-2 v2-4-1 v2-4-0

OCT-2725: Data can be overwritten when publishing configurations too fast on :ref:`zephyr:zbus`
  When, for example, setting up a bidirectional stream, the configurations for sink and source might be received too fast, resulting in one of the configurations being lost.

  **Affected platforms:** nRF5340 Audio DK

.. rst-class:: v2-5-3 v2-5-2 v2-5-1 v2-5-0

OCT-2765: BIS headset stack overflow if gateway is periodically reset
  A BIS headset might get stack overflow if the BIS source is reset several times with short intervals.

  **Affected platforms:** nRF5340 Audio DK

.. rst-class:: v2-5-3 v2-5-2 v2-5-1 v2-5-0

OCT-2766: BIS headset might get into an infinite loop
  If a BIS headset is paused, the BIS broadcaster is reset, and if the headset is unpaused, it will end up in an infinite loop.

  **Affected platforms:** nRF5340 Audio DK

.. rst-class:: v2-5-3 v2-5-2 v2-5-1 v2-5-0

OCT-2767: Potential infinite loop for the re-established BIS stream
  If the stream is lost and re-established on a broadcast sink, an infinite loop might be triggered.

  **Affected platforms:** nRF5340 Audio DK

.. rst-class:: v2-6-0

OCT-2897: Interleaved packing issue
  Using interleaved packing on the controller might cause the left headset to disconnect.

  **Affected platforms:** nRF5340 Audio DK

.. rst-class:: v3-1-1 v3-1-0 v3-0-2 v3-0-1 v3-0-0 v2-9-0-nRF54H20-1 v2-9-2 v2-9-1 v2-9-0 v2-8-0 v2-7-0

OCT-3006: Setting a custom bitrate using :kconfig:option:`CONFIG_BT_AUDIO_BITRATE_UNICAST_SINK` will have no effect
  This is because the application reverts to one of the BAP presets.

  **Affected platforms:** nRF5340 Audio DK

.. rst-class:: v3-1-1 v3-1-0 v3-0-2 v3-0-1 v3-0-0 v2-9-0-nRF54H20-1 v2-9-2 v2-9-1 v2-9-0 v2-8-0 v2-7-0 v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0 v2-5-3 v2-5-2 v2-5-1 v2-5-0

OCT-3248: A race condition between capturing timers and the RTC tick resetting the HF timer
  This issue might cause time to jump ahead by about 30 µs.

  **Affected platforms:** nRF5340 Audio DK

Controller subsystem for nRF5340 Audio
--------------------------------------

The following known issues apply to the LE Audio subsystem (NET core controller) for nRF5340 used in the nRF5340 Audio application.

.. rst-class:: v2-4-3 v2-4-2 v2-4-1 v2-4-0 v2-3-0 v2-2-0

OCX-138: Some conformance tests not passing
   Not all Bluetooth conformance test cases pass.

.. rst-class:: v2-4-3 v2-4-2 v2-4-1 v2-4-0 v2-3-0 v2-2-0

OCX-152: OCX-146: 40 ms ACL interval might cause TWS to be unstable
  There might be combinations of ACL intervals and other controller settings that cause instabilities to connected or true wireless stereo setups.

  **Workaround:** Use an alternative ACL connection interval.

.. rst-class:: v2-5-3 v2-5-2 v2-5-1 v2-5-0 v2-4-4 v2-4-3 v2-4-2 v2-4-1 v2-4-0

OCX-153: Cannot create BIG sync after terminate pending BIG sync
  If a pending BIG sync is canceled by sending the LE Broadcast Isochronous Group Sync Established command, it is impossible for the host to create a new BIG sync afterwards.

.. rst-class:: v2-5-3 v2-5-2 v2-5-1 v2-5-0 v2-4-4 v2-4-3 v2-4-2 v2-4-1 v2-4-0 v2-3-0 v2-2-0

OCX-155: Larger timestamps than intended
   The timestamps/Service Data Unit references (SDU refs) might occasionally be larger than intended and then duplicated in the next interval.

.. rst-class:: v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0 v2-5-3 v2-5-2 v2-5-1 v2-5-0 v2-4-4 v2-4-3 v2-4-2 v2-4-1 v2-4-0 v2-3-0 v2-2-0

OCX-156: PTO is not supported
   The controller does not support Pre-Transmission Offset.

.. rst-class:: v2-4-3 v2-4-2 v2-4-1 v2-4-0 v2-3-0 v2-2-0

OCX-157: OCX-140: Interleaved broadcasts streaming issues
  Interleaved broadcasts cannot stream with certain Quality of Service (QoS) configurations.
  The controller cannot handle neither broadcasting interleaved BIS ISO packets nor syncing with interleaved BIS ISO packets when the SDU size is more than 100 bytes and retransmission time is 4.

  **Workaround:** Set retransmits (RTN) to ``<= 2`` and octets per frame to ``<= 80`` for stereo.

.. rst-class:: v2-3-0 v2-2-0

OCX-168: Issues with reestablishing streams
   Syncing of broadcast receivers takes longer than in version 3310, especially for high retransmit (RTN) values.

.. rst-class:: v2-5-3 v2-5-2 v2-5-1 v2-5-0 v2-4-4 v2-4-3 v2-4-2 v2-4-1 v2-4-0

OCX-178: Transport latency does not affect flush timeout
  Setting transport latency higher than 10 ms and higher retransmission time setting do not have an effect on the flush timeout setting.
  The flush timeout is always 1.

.. rst-class:: v2-5-3 v2-5-2 v2-5-1 v2-5-0 v2-4-4 v2-4-3 v2-4-2 v2-4-1 v2-4-0

OCX-183: Feature request control procedure initiated when controller is in progress of creating CIS and CIS is not yet established
  The controller might send a feature request to the remote device during CIS creating procedure even if the CIS has not been established.
  Due to this, the remote device might terminate the connection.

.. rst-class:: v2-4-3 v2-4-2 v2-4-1 v2-4-0

OCX-184: If 0 dBm TX power is selected, the FEM/PA TX/RX pins do not toggle correctly
  For the usage scenario like using other vendor's FEM or trying to expose radio TX/RX activity on GPIO, setting the max output power and the target output power to 0 dBm does not make FEM module work properly.

  **Workaround:** Set max TX power larger than 0 dBm.

.. rst-class:: v2-5-3 v2-5-2 v2-5-1 v2-5-0 v2-4-4 v2-4-3 v2-4-2 v2-4-1 v2-4-0

OCX-188/OCX-227: The controller reserves some pins (0.28 - 0.31), which might collide with FEM/PA features
  FEM feature cannot work properly on GPIO pins from **P0.28** to **P0.31**.

  **Workaround:** Use different pins for FEM/PA control.

.. rst-class:: v2-4-3 v2-4-2 v2-4-1 v2-4-0

OCX-189: When inputting –40 dBm to HCI_OPCODE_VS_SET_CONN_TX_PWR (0x3F6), the actual TX power is changed to –20 dBm
  The TX power for a connection cannot be less than –20 dBm.
  Controller still output –20 dBm if the setting is –40 dBm.

.. rst-class:: v2-5-3 v2-5-2 v2-5-1 v2-5-0

OCX-208: There is a chance that the scan report only shows legacy-ADV without ADV-EXT
  This might happen if a broadcast sink is moved in and out of radio range from the broadcast source or if the broadcast source is being rapidly reset.
  It will cause a broadcast sink to not be able to finish PA sync.

.. rst-class:: v2-5-3 v2-5-2 v2-5-1 v2-5-0

OCX-217: The controller can get unresponsive when a broadcast sink tries to sync to some broadcast sources
  This is only an issue if a broadcast sink tries to sync to certain broadcast sources made by other vendors.

.. rst-class:: v2-5-3 v2-5-2 v2-5-1 v2-5-0

OCX-218: The BIG info is not sent from the controller when a broadcast sink tries to sync to some broadcast sources
  This is only an issue if a broadcast sink tries to sync to certain broadcast sources made by other vendors.

.. rst-class:: v2-5-3 v2-5-2 v2-5-1 v2-5-0

OCX-223: The controller asserts on the CIS gateway if two paused headsets reset
  If a CIS headset pair is reset simultaneously a few times while remaining paused, the controller on the gateway might get unresponsive.

.. rst-class:: v2-6-0

OCX-238: The controller rejects CIS Requests with the same ``CIG_ID`` as a currently configured CIG
  This will affect the use of the controller in use cases in which it acts as both CIS Central and CIS Peripheral.
  Any configured CIG with the same ``CIG_ID`` will need to be removed before the device can act as a CIS Peripheral.

nRF Machine Learning
====================

The issues in this section are related to the :ref:`nrf_machine_learning_app` application.

.. rst-class:: v2-2-0

NCSDK-18532: MCUboot bootloader does not swap images after OTA DFU on nRF5340 DK and Thingy:53
  The MCUboot bootloader cannot access external flash because the chosen ``nordic,pm-ext-flash`` is not defined in the MCUboot child image's devicetree.
  As a result, MCUboot cannot swap images that are received during the OTA update.

  **Affected platforms:** nRF5340, Thingy:53

  **Workaround**: Manually cherry-pick and apply the commit with the fix from the ``main`` branch (commit hash: ``f54f6bbd423b12a595e76425e688f034926b8018``) to fix the issue for ``nrf5340dk_nrf5340_cpuapp``.
  Similar fix needs to be applied for the Thingy:53 board.
  Make sure to also cherry-pick and apply the commit that fixes a build system issue (commit ``ec23df1fa305e99194ceac87a028f6da206a3ff1`` from ``main`` branch).
  This is needed to ensure that the introduced DTS overlay will be applied to the MCUboot child image.

.. rst-class:: v1-9-0

NCSDK-13923: Device might crash during Bluetooth bonding
  The device programmed with the nRF Machine Learning application might crash during Bluetooth bonding because of insufficient Bluetooth RX thread stack size.

  **Workaround:** Manually cherry-pick and apply the commit with the fix from the ``main`` branch (commit hash: ``4870fcd8316bd3a4b53ca0054f0ce35e1a8c567d``).

.. rst-class:: v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0

NCSDK-16644: :ref:`nrf_machine_learning_app` does not go to sleep and does not wake up on Thingy:53
  nRF Machine learning application on Thingy:53 does not sleep after a period of inactivity and does not wake up after an activity occurs.

  **Affected platforms:** Thingy:53

  **Workaround:** Manually cherry-pick and apply two commits with the fix from the ``main`` branch (commit hashes: ``7381bfcb9c23cd6f78e6ef7fd3ff82b700f81b0f``, ``7e8c23a6632632f0cee885abe955e37a6942911d``).

Samples
*******

The issues in this section are related to :ref:`samples`.

.. rst-class:: v2-2-0

NCSDK-18263: |NCS| samples might fail to boot on Thingy:53
  |NCS| samples and applications that are not listed under :ref:`thingy53_compatible_applications` fail to boot on Nordic Thingy:53.
  The MCUboot bootloader is not built together with these samples, but the Thingy:53's :ref:`static Partition Manager memory map <ug_pm_static>` requires it (the application image does not start at the beginning of the internal ``FLASH``.)

  **Affected platforms:** Thingy:53

  **Workaround:** Revert the ``9a8680372fdb6e09f3d6537c8c6751dd5c50bf86`` commit in the sdk-zephyr repository and revert the ``1f9765df5acbb36afff0ce40c94ba65d44d19d70`` commit in sdk-nrf.
  During conflict resolution, make sure to update the :file:`west.yml` file in the sdk-nrf to point to the reverting commit in sdk-zephyr.

Bluetooth samples
=================

.. rst-class:: v3-0-2 v3-0-1 v3-0-0

NCSDK-33915: The :ref:`direct_test_mode` asserts on nRF54H20 devices
  The sample asserts during reception tests and sends too few packets during transmission tests.

  **Affected platforms:** nRF54H20

  **Workaround:** Move the ``errata216_on_wait()`` static function call from the ``radio_start()`` function to the test command handlers in :file:`dtm.c`, which enable radio: ``dtm_vendor_specific_pkt()``, ``dtm_test_receive()``, and ``dtm_test_transmit()``.

.. rst-class:: v3-1-1 v3-1-0 v3-0-2 v3-0-1 v3-0-0

NCSDK-33040: Output power tests running with the Anritsu tester fail
  This happens with the :ref:`direct_test_mode` sample.

  **Affected platforms:** nRF5340 nRF21540

.. rst-class:: v2-8-0 v2-7-0 v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0

NCSDK-26424: Directed advertising in the :ref:`peripheral_hids_mouse` sample does not start after disconnecting from a bonded peer
  When the sample disconnects from a peer, after successful pairing and subscription to reports, it cannot re-connect because directed advertising does not start.

.. rst-class:: v2-4-3 v2-4-2 v2-4-1 v2-4-0

NCSDK-21709: :ref:`peripheral_uart` sample does not work on nRF52810 and nRF52811 devices
  Initialization of the Bluetooth stack fails.

  **Affected platforms:** nRF52810, nRF52811

  **Workaround:** Enable the :kconfig:option:`CONFIG_BT_SETTINGS` Kconfig option in the project configuration file (:file:`prj_minimal.conf`).

.. rst-class:: v2-3-0 v2-2-0

NCSDK-18112: :ref:`bluetooth_central_dfu_smp` sample cannot do discovery on the :zephyr:code-sample:`smp-svr`
  The :ref:`bluetooth_central_dfu_smp` sample cannot perform the GATT discovery on a DK with the :zephyr:code-sample:`smp-svr`.

  **Workaround:** Enable the legacy LLCP mechanism (:kconfig:option:`CONFIG_BT_LL_SW_LLCP_LEGACY`).

.. rst-class:: wontfix v3-1-1 v3-1-0 v3-0-2 v3-0-1 v3-0-0 v2-9-0-nRF54H20-1 v2-9-2 v2-9-1 v2-9-0 v2-8-0 v2-7-0 v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0 v2-5-3 v2-5-2 v2-5-1 v2-5-0 v2-4-4 v2-4-3 v2-4-2 v2-4-1 v2-4-0 v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0 v1-0-0 v0-4-0 v0-3-0

NCSDK-19942: HID samples do not work with Android 13
  Bluetooth samples and applications that are set up for the HIDS use case and have the Bluetooth Privacy feature enabled (:kconfig:option:`CONFIG_BT_PRIVACY`) disconnect after a short period or enter a connection-disconnection loop when you try to connect to them from a device that is running Android 13.
  See the following list of affected samples and applications:

  * :ref:`peripheral_hids_mouse`
  * :ref:`fast_pair_input_device`
  * Fast Pair configurations of the :ref:`nrf_desktop` application

.. rst-class:: wontfix v3-1-1 v3-1-0 v3-0-2 v3-0-1 v3-0-0 v2-9-0-nRF54H20-1 v2-9-2 v2-9-1 v2-9-0 v2-8-0 v2-7-0 v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0 v2-5-3 v2-5-2 v2-5-1 v2-5-0 v2-4-3 v2-4-2 v2-4-1 v2-4-0 v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0 v1-0-0 v0-4-0 v0-3-0

NCSDK-34682: HID device reconnection may fail on Android 16 when privacy is enabled
  On Android 16, reconnecting to HID devices with privacy enabled may not work correctly after disconnection.
  When the Android device disconnects from the HID device, it attempts to automatically reconnect.
  However, although the reconnection is successful, the Bluetooth settings show the device as not connected, and HID reports are ignored.
  If the user manually tries to connect, the system gets stuck in a "connecting" loop that can persist for tens of seconds - or indefinitely in most cases.
  See the following list of affected samples and applications:

  * :ref:`peripheral_hids_mouse`
  * :ref:`fast_pair_input_device`
  * Fast Pair configurations of the :ref:`nrf_desktop` application

  **Workaround:** On :ref:`peripheral_hids_mouse`, you can disable the :kconfig:option:`CONFIG_BT_PRIVACY` Kconfig option.
  Alternatively, you can unpair the device and pair it again.

.. rst-class:: wontfix v3-1-1 v3-1-0 v3-0-2 v3-0-1 v3-0-0 v2-9-0-nRF54H20-1 v2-9-2 v2-9-1 v2-9-0 v2-8-0 v2-7-0 v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0 v2-5-3 v2-5-2 v2-5-1 v2-5-0 v2-4-3 v2-4-2 v2-4-1 v2-4-0 v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0 v1-0-0 v0-4-0 v0-3-0

NCSDK-26669: Reconnection with HID devices that have privacy enabled might not work on Android 14
  If an Android 14 device is disconnected from the HID device without user intervention through the Bluetooth UI from Android settings (for example, due to a connection timeout caused by the HID device rebooting), the device faces issues.
  If the disconnection occurs in the same session as successful Bluetooth pairing and bonding, the smartphone will not be able to automatically reconnect to the HID device.
  This limitation affects both background reconnection and explicit user attempts through the Bluetooth settings.
  See the following list of affected samples and applications:

  * :ref:`peripheral_hids_mouse`
  * :ref:`fast_pair_input_device`
  * Fast Pair configurations of the :ref:`nrf_desktop` application

  **Workaround:** After successful Bluetooth pairing and bonding, manually disconnect the Android device using the Bluetooth UI from Android settings, and then reconnect to the HID device.

.. rst-class:: v2-4-3 v2-4-2 v2-4-1 v2-4-0 v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0

NCSDK-18518: Cannot build peripheral UART and peripheral LBS samples for the nRF52810 and nRF52811 devices with the Zephyr Bluetooth LE Controller
  The :ref:`peripheral_uart` and :ref:`peripheral_lbs` samples fail to build for the nRF52810 and nRF52811 devices when the :kconfig:option:`CONFIG_BT_LL_CHOICE` Kconfig option is set to :kconfig:option:`CONFIG_BT_LL_SW_SPLIT`.

  **Affected platforms:** nRF52811, nRF52810

  **Workaround:** Use the SoftDevice Controller: set the :kconfig:option:`CONFIG_BT_LL_CHOICE` Kconfig option to :kconfig:option:`CONFIG_BT_LL_SOFTDEVICE`.

.. rst-class:: v2-2-0

NCSDK-20070: The :ref:`direct_test_mode` antenna switching does not work on the nRF5340 DK with the nRF21540 EK shield
  The antenna select DTM command does not have any effect because the GPIO pin which controls antenna is not forwarded to the nRF5340 DK network core.

  **Affected platforms:** nRF5340, nRF21540

  **Workaround** Add a ``<&gpio1 6 0>`` entry in :file:`samples/bluetooth/direct_test_mode/conf/remote_shell/pin_fwd.dts`.

.. rst-class:: v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0

NCSDK-19727: Cannot build the :ref:`peripheral_hids_mouse` sample with security disabled
  Build fails when the :kconfig:option:`CONFIG_BT_HIDS_SECURITY_ENABLED` Kconfig option is disabled.

  **Workaround:** Build the sample with its default configuration, that is, enable the :kconfig:option:`CONFIG_BT_HIDS_SECURITY_ENABLED` Kconfig option.

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

  **Affected platforms:** Thingy:53

  **Workaround:** Manually cherry-pick and apply commit with fix from ``main`` (commit hash: ``b834ff8860f3a30fe19c99dbf4c0c99b0b017245``).

.. rst-class:: v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0 v1-0-0

NCSDK-9820: Notification mismatch in :ref:`peripheral_lbs`
  When testing the :ref:`peripheral_lbs` sample, if you press and release **Button 1** while holding one of the other buttons, the notification for button release is the same as for the button press.

.. rst-class:: v1-8-0

NCSDK-12886: Peripheral UART sample building issue with nRF52811
  The :ref:`peripheral_uart` sample built for nRF52811 asserts on the nRF52840 DK in rev. 2.1.0 (board target: ``nrf52840dk_nrf52811``).

  **Affected platforms:** nRF52840, nRF52811

.. rst-class:: v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0 v1-0-0

NCSDK-8321: NUS shell transport sample does not display the initial shell prompt
  NUS shell transport sample does not display the initial shell prompt ``uart:~$`` on the remote terminal.
  Also, few logs with sending errors are displayed on the terminal connected directly to the DK.
  This issue is caused by the shell being enabled before turning on the notifications for the NUS service by the remote peer.

  **Workaround:** Enable the shell after turning on the NUS notifications or block it until turning on the notifications.

.. rst-class:: v1-2-1 v1-2-0

Peripheral HIDS keyboard sample cannot be used with nRF Bluetooth LE Controller
  The :ref:`peripheral_hids_keyboard` sample cannot be used with the :ref:`nrfxlib:softdevice_controller` because the NFC subsystem does not work with the controller library.
  The library uses the MPSL Clock driver, which does not provide an API for asynchronous clock operation.
  NFC requires this API to work correctly.

.. rst-class:: v2-9-0-nRF54H20-1 v2-9-2 v2-9-1 v2-9-0 v2-8-0 v2-7-0

NCSDK-33053: The Bluetooth Peripheral HIDS Keyboard with unmet dependency for the NFC OOB pairing Kconfig
  The :ref:`peripheral_hids_keyboard` sample defines the :kconfig:option:`CONFIG_NFC_OOB_PAIRING` Kconfig option that is used to enable the NFC OOB pairing feature.
  This option cannot be enabled because of the unmet dependency on the :kconfig:option:`CONFIG_HAS_HW_NRF_NFCT` Kconfig option.
  The :kconfig:option:`CONFIG_HAS_HW_NRF_NFCT` Kconfig option is not directly configurable by the user and derives its value from the status field of the ``nfct`` node defined in the devicetree configuration.
  Starting from the |NCS| v2.7.0 release, the ``nfct`` node is not enabled by default for all board targets that support the NFCT peripheral.
  The ``nfct`` node has to be enabled manually at the project level.

  **Workaround:** Enable the ``nfct`` node in the devicetree configuration as follows:

  .. code-block::

     &nfct {
       status = "okay";
     };

.. rst-class:: v3-0-2 v3-0-1 v3-0-0 v2-9-0-nRF54H20-1 v2-9-2 v2-9-1 v2-9-0 v2-8-0 v2-7-0 v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0 v2-5-3 v2-5-2 v2-5-1 v2-5-0 v2-4-4 v2-4-3 v2-4-2 v2-4-1 v2-4-0 v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0 v1-0-0 v0-4-0

NCSDK-33632: The Bluetooth Peripheral HIDS samples sometimes disconnect due to Bluetooth LL collisions
  The :ref:`peripheral_hids_keyboard` and the :ref:`peripheral_hids_mouse` samples use the default configuration of the Zephyr Bluetooth subsystem, which enables automatic initiation of Bluetooth Link Layer procedures (for example, the PHY Update procedure and the associated :kconfig:option:`CONFIG_BT_AUTO_PHY_UPDATE` Kconfig option).
  This can cause Link Layer procedure collisions during a connection with the Bluetooth Central device that does not properly handle these procedures.
  In this case, the peripheral devices can experience disconnections.

  **Workaround:** Apply the following configuration changes in your project configuration file (for example, :file:`prj.conf`):

  .. code-block::

     CONFIG_BT_GATT_AUTO_SEC_REQ=n
     CONFIG_BT_AUTO_PHY_UPDATE=n

.. rst-class:: v1-2-1 v1-2-0

Peripheral HIDS mouse sample advertising issues
  When the :ref:`peripheral_hids_mouse` sample is used with the Zephyr Bluetooth LE Controller, directed advertising does not time out and the regular advertising cannot be started.

.. rst-class:: v1-2-1 v1-2-0

Central HIDS sample issues with directed advertising
  The :ref:`bluetooth_central_hids` sample cannot connect to a peripheral that uses directed advertising.

.. rst-class:: v1-1-0

Unstable samples
  Bluetooth Low Energy peripheral samples are unstable in some conditions (when pairing and bonding are performed and then disconnections/re-connections happen).

.. rst-class:: v1-1-0 v1-0-0

:ref:`central_uart` cannot handle long strings
  A too long 212-byte string cannot be handled when entered to the console to send to :ref:`peripheral_uart`.

.. rst-class:: v1-0-0

:ref:`bluetooth_central_hids` loses UART connectivity
  After programming a HEX file to the ``nrf52_pca10040`` board, UART connectivity is lost when using the Bluetooth LE Controller.
  The board must be reset to get UART output.

  **Affected platforms:** nRF52832

.. rst-class:: v1-1-0 v1-0-0

Samples crashing on nRF51 when using GPIO
  On nRF51 devices, Bluetooth LE samples that use GPIO might crash when buttons are pressed frequently.
  In such case, the GPIO ISR introduces latency that violates real-time requirements of the Radio ISR.
  nRF51 is more sensitive to this issue than nRF52 (faster core).

  **Affected platforms:** nRF51 Series, nRF52832

.. rst-class:: v0-4-0

GATT Discovery Manager missing support
  The :ref:`gatt_dm_readme` is not supported on nRF51 devices.

  **Affected platforms:** nRF51 Series

.. rst-class:: v0-4-0

Samples do not work with SD Controller v0.1.0
  Bluetooth LE samples cannot be built with the :ref:`nrfxlib:softdevice_controller` v0.1.0.

.. rst-class:: v0-3-0

Peripheral UART string size issue
  :ref:`peripheral_uart` cannot handle the corner case that a user attempts to send a string of more than 211 bytes.

.. rst-class:: v1-0-0 v0-4-0 v0-3-0

LED Button Service reporting issue
  :ref:`peripheral_lbs` does not report the **Button 1** state correctly.

.. rst-class:: v1-2-1 v1-2-0 v1-1-0 v1-0-0 v0-4-0 v0-3-0

MITM protection missing for central samples
  The central samples (:ref:`central_uart`, :ref:`bluetooth_central_hids`) do not support any pairing methods with MITM protection.

.. rst-class:: v0-3-0

Reconnection issues after bonding
  The peripheral samples (:ref:`peripheral_uart`, :ref:`peripheral_lbs`, :ref:`peripheral_hids_mouse`) have reconnection issues after performing bonding (LE Secure Connection pairing enable) with nRF Connect for Desktop.
  These issues result in disconnection.

.. rst-class:: v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0

NCSDK-17883: Cannot build peripheral UART sample with security (:kconfig:option:`CONFIG_BT_NUS_SECURITY_ENABLED`) disabled
  The :ref:`peripheral_uart` sample fails to build when the :kconfig:option:`CONFIG_BT_NUS_SECURITY_ENABLED` Kconfig option is disabled.

  **Workaround:** In :file:`main.c` file, search for the ``#else`` entry of the ``#if defined(CONFIG_BT_NUS_SECURITY_ENABLED)`` item and add an empty declaration of the ``conn_auth_info_callbacks`` structure, just after the similar empty definition of ``conn_auth_callbacks``.

.. rst-class:: v1-9-0

Antenna switching does not work on targets ``nrf5340dk_nrf5340_cpuapp`` and ``nrf5340dk_nrf5340__cpuapp_ns``
  Antenna switching does not work when :ref:`direction finding sample <ble_samples>` applications are built for ``nrf5340dk_nrf5340_cpuapp`` and ``nrf5340dk_nrf5340_cpuapp_ns`` targets.
  That is caused by GPIO pins that are responsible for access to antenna switches, not being assigned to network core.

  **Affected platforms:** nRF5340

Bluetooth Fast Pair samples
===========================

.. note::

   Fast Pair samples are affected by the following known issues:

   * NCSDK-19942
   * NCSDK-34682
   * NCSDK-26669

.. rst-class:: v3-1-1 v3-1-0

NCSDK-34821: Fast Pair samples do not build or run due to the IronSide SE migration
  The :ref:`fast_pair_input_device` and :ref:`fast_pair_locator_tag` samples are currently incompatible with the latest |NCS| transition to the IronSide SE solution - bundle 22.1.0 and higher for the ``nrf54h20dk/nrf54h20/cpuapp`` board target.
  As a result, you cannot use these samples with that board target in any of the affected releases.

  **Affected platforms:** nRF54H20

  **Workaround:** Use a future |NCS| release that includes IronSide SE support in the Fast Pair samples or use the |NCS| v3.0.0 release with the SUIT SDSC bundle (version up to 0.9.6).

.. rst-class:: v3-0-2 v3-0-1 v3-0-0

NCSDK-34582: The :ref:`fast_pair_locator_tag` sample uses incorrect partition map for the nRF54L10 SoC
  The partition map for the ``nrf54l15dk/nrf54l10/cpuapp`` board target is incorrect in the :ref:`fast_pair_locator_tag` sample.
  In the |NCS| releases affected by this issue, it was assumed that the NVM size of the nRF54L10 SoC was 10 KB larger than the actual one.
  The NVM size of the nRF54L10 SoC is equal to 1012 KB.

  The workaround fix for the nRF54L10 partition map is a breaking change and cannot be performed using DFU.
  The DFU procedure will fail if you attempt to upgrade the sample firmware based on one of the |NCS| releases that is affected by this issue.

  **Affected platforms:** nRF54L10

  **Workaround:** Manually cherry-pick and apply the commit with the fix from the ``main`` branch of the ``sdk-nrf`` repository (commit hash: ``43d0e2eb129bde7b0f0fc462dd4b239765c69c59``).
  Additionally, you need to manually update the :file:`boards/nordic/nrf54l15dk/nrf54l15dk_nrf54l10_cpuapp.dts` DTS file in your local copy of the ``sdk-zephyr`` repository to avoid issues with build asserts that are used for RRAM validation.
  Correct the ``cpuapp_rram`` DTS node in the following way:

  .. code-block::

     &cpuapp_rram {
       reg = <0x0 DT_SIZE_K(1012)>;
     };

  Then, correct the ``storage_partition`` node in the following way:

  .. code-block::

     storage_partition: partition@f6000 {
       reg = <0xf6000 DT_SIZE_K(28)>;
     };

Bluetooth Mesh samples
======================

.. rst-class:: v3-0-2 v3-0-1 v3-0-0 v2-9-0-nRF54H20-1 v2-9-2 v2-9-1 v2-9-0

NCSDK-32703: :zephyr:code-sample:`ble_mesh_provisioner` sample gives MPU fault due to insufficient :kconfig:option:`CONFIG_BT_MESH_ADV_STACK_SIZE` Kconfig value
  Updates in the |NCS| resulted in an increase in the required size for the advertiser stack.
  This should be taken care of for the Mesh samples under the nRF tree.
  For evaluating Mesh samples in the Zephyr tree, inadequate default value for the :kconfig:option:`CONFIG_BT_MESH_ADV_STACK_SIZE` Kconfig option can result in MPU fault due to insufficient stack size.

  **Workaround:** It is recommended to set the value of the :kconfig:option:`CONFIG_BT_MESH_ADV_STACK_SIZE` Kconfig to 2048.

.. rst-class:: v3-1-1 v3-1-0 v3-0-2 v3-0-1 v3-0-0 v2-9-0-nRF54H20-1 v2-9-2 v2-9-1 v2-9-0 v2-8-0 v2-7-0 v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0 v2-5-3 v2-5-2 v2-5-1 v2-5-0

NCSDK-26844: :ref:`ble_mesh_dfu_distributor` sample is not able to complete self-update
  When attempting to use the :ref:`ble_mesh_dfu_distributor` sample to perform a self-update, the DFU process is not completed successfully.
  The new firmware is installed, but the remaining process will not be completed successfully.

  **Workaround:** Use the ``mesh models dfu cli target-imgs`` shell command to verify that the new firmware is running after the unsuccessful completion of the self-update.

.. rst-class:: v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0

NCSDK-26388: Compilation of Mesh Light sample can create an image without MCUboot
  This can happen when compiled with the point-to-point DFU overlay and ``--sysbuild`` option.

  **Workaround:** To get a correct image with MCUboot, :ref:`build the firmware without sysbuild <optional_build_parameters>` using the ``--no-sysbuild`` option.

.. rst-class:: v2-7-0 v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0

NCSDK-26403: Point-to-point DFU procedure :guilabel:`Test and Confirm` with erasing application area does not succeed in the Device Manager app on IOs for the Mesh Light sample
  After uploading the image and resetting the device, the mobile application cannot receive a confirmation back and the whole procedure fails.

  **Workaround:** Use the DFU procedures :guilabel:`Test` and :guilabel:`Confirm` separately.

.. rst-class:: v2-4-3 v2-4-2 v2-4-1 v2-4-0

NCSDK-21590: :ref:`bluetooth_mesh_sensor_client` sample does not compile for nRF52832
  Adding mesh shell support for the :ref:`bluetooth_mesh_sensor_client` sample increases the need for RAM.
  The :ref:`bluetooth_mesh_sensor_client` sample cannot compile because of RAM shortage.

  **Affected platforms:** nRF52832

  **Workaround:** Disable the mesh shell support in the :file:`prj.conf` file for the sensor client sample.

Cellular samples
================

.. rst-class:: v3-1-1 v3-1-0

IRIS-10159: nRF Cloud logging not working in the :ref:`nrf_cloud_rest_device_message` sample
  The sample does not react to runtime log level setting from the cloud side.
  This will be resolved in the next release.

  **Affected platforms:** nRF9150, nRF9151

.. rst-class:: v3-0-2 v3-0-1 v3-0-0 v2-9-0-nRF54H20-1 v2-9-2 v2-9-1 v2-9-0 v2-8-0

NCSDK-30050: The :ref:`nrf_cloud_rest_fota` sample with experimental SMP FOTA support enabled crashes with a secure fault
  This happens approximately one out of five times during an SMP FOTA update.

.. rst-class:: v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0

IRIS-8456: Wi-Fi builds of the :ref:`nrf_cloud_multi_service` sample crash and reboot
  This happens if no Wi-Fi APs are visible for more than a few minutes.

.. rst-class:: v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0

IRIS-8465: CoAP builds of the :ref:`nrf_cloud_multi_service` sample stall if connectivity is lost
  If PDN detaches for too long while the sample is connected, the sample cannot reconnect to nRF Cloud after PDN returns.

.. rst-class:: v2-5-3 v2-5-2 v2-5-1 v2-5-0

IRIS-7398: The :ref:`nrf_cloud_multi_service` sample does not support using the MCUboot secondary partition in external flash fails on the nRF9161 DK
  The sample can be built for the nRF9161 DK with the :file:`overlay_mcuboot_ext_flash.conf` overlay enabled, but the resultant application will not boot.

  **Affected platforms:** nRF9161

.. rst-class:: v3-0-2 v3-0-1 v3-0-0 v2-9-0-nRF54H20-1 v2-9-2 v2-9-1 v2-9-0 v2-8-0 v2-7-0 v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0 v2-5-3 v2-5-2 v2-5-1 v2-5-0 v2-4-4 v2-4-3 v2-4-2 v2-4-1 v2-4-0 v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0

IRIS-7381: :ref:`nrf_cloud_rest_cell_location` sample might attempt to take a neighbor cell measurement when a measurement is already in progress
  If cell information changes during a neighbor cell measurement, the sample will attempt to start a new measurement, resulting in warning and error log messages.

  **Affected platforms:** nRF9160, nRF9161

  **Workaround:** Ignore the error log message indicating that the neighbor cell measurement failed to start, error code ``-119``.
  The sample will send the location request to nRF Cloud when the active measurement completes.

.. rst-class:: v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0

NCSDK-5666: LTE Sensor Gateway
  The :ref:`lte_sensor_gateway` sample crashes when Thingy:52 is flipped.

  **Affected platforms:** nRF9160, Thingy:52

.. rst-class:: v1-2-0

GPS sockets and SUPL client library stops working
  The `nRF9160: GPS with SUPL client library`_ sample stops working if :ref:`supl_client` support is enabled, but the SUPL host name cannot be resolved.

  **Affected platforms:** nRF9160

  **Workaround:** Insert a delay (``k_sleep()``) of a few seconds after the ``printf`` on line 294 in :file:`main.c`.

.. rst-class:: v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0

NCSDK-9441: The :ref:`fmfu_smp_svr_sample` sample is unstable with the newest J-Link version
  Full modem serial update does not work on development kit with debugger chip version delivered with J-Link software > 6.88a

  **Affected platforms:** nRF9160

  **Workaround:** Downgrade the debugger chip to the firmware released with J-Link 6.88a or use another way of transferring serial data to the chip.

.. rst-class:: v1-7-1 v1-7-0

NCSDK-11033: Dial-up usage not working
  Dial-up usage with :ref:`MoSh PPP <modem_shell_application>` does not work and causes the nRF9160 DK to crash when it is connected to a PC.

  **Affected platforms:** nRF9160

  **Workaround:** Manually pick the fix available in Zephyr to the `Zephyr issue #38516`_.

Matter samples
==============

The issues in this section are related to :ref:`matter_samples`.

.. rst-class:: v3-1-1 v3-1-0

KRKNWK-20691: The :ref:`matter_lock_sample` sample does not update LED state on auto-relock
  The sample does not update the LED state to on after auto-relock timeout.
  The attribute value after auto-relock timeout is correct (locked), but the sample does not handle this change correctly, so the state of LED visualizing the lock state is wrong.

.. rst-class:: v2-9-1 v2-9-0 v2-8-0

KRKNWK-19806: RPU recovery fails on the nRF5340 DK with nRF7002 EK shield due to invalid MCUboot configuration

  **Affected platforms:** nRF5340, nRF7002

  **Workaround:** Cherry-pick changes from PR #19826 in the `sdk-nrf`_ repository.

.. rst-class:: v2-9-0-nRF54H20-1 v2-9-0

KRKNWK-19861: ICD DSLS does not work with the :ref:`matter_smoke_co_alarm_sample` sample
  According to the sample's documentation the ICD DSLS can be enabled by setting the :kconfig:option:`CONFIG_CHIP_ICD_DSLS_SUPPORT` Kconfig option to ``y``.
  Additionally, the ICD Management cluster's feature map has to be set to ``0xf`` in the sample's ``.zap`` file.
  This piece of information is missing from the documentation.
  It will be added in the next version.

.. rst-class:: v2-8-0

KRKNWK-19691: Smoke CO Alarm sample does not have PM device enabled
  This leads to not suspending QSPI, while it is not used, and increasing the device power consumption.

  **Workaround:** Set the :kconfig:option:`CONFIG_PM_DEVICE` Kconfig option to ``y`` in the sample's :file:`prj.conf` file.

.. rst-class:: v2-7-0

KRKNWK-19480: Lock sample does not allow for clearing the door lock user when using the schedules feature
  If the lock application is built with the :kconfig:option:`CONFIG_LOCK_SCHEDULES` Kconfig option and lock credentials are programmed by the controller, clearing of the user always fails.

  **Workaround:** Manually cherry-pick and apply commit with fix from main (commit hash: ``b60eb4900e62bb7c771397adb152552849052b18``).

.. rst-class:: v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0 v2-5-3 v2-5-2 v2-5-1 v2-5-0

KRKNWK-18242: Thermostat sample does support the AUTO system mode
  AUTO system mode is supported in a thermostat device but it is not reflected in the data model and the feature map indicates that AUTO system mode is not supported.

  **Workaround:** Set the feature map to ``0x23`` in the :file:`thermostat.zap` file and regenerate the data model, so that it indicates that the AUTO mode is supported by a thermostat device.

NFC samples
===========

The issues in this section are related to :ref:`nfc_samples`.

.. rst-class:: v2-2-0

NCSDK-19168: The :ref:`peripheral_nfc_pairing` and :ref:`central_nfc_pairing` samples cannot pair using OOB data
  The :ref:`nfc_ndef_ch_rec_parser_readme` library parses AC records in an invalid way.
  As a result, the samples cannot parse OOB data for pairing.

  **Workaround:** Revert the :file:`subsys/nfc/ndef/ch_record_parser.c` file to the state from the :ref:`ncs_release_notes_210`.

  .. code-block::

     cd <NCS_root_directory>
     git checkout v2.1.0 -- subsys/nfc/ndef/ch_record_parser.c

.. rst-class:: v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0

NCSDK-19347: NFC Reader samples return false errors with value ``1``
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

  **Affected platforms:** nRF5340

.. rst-class:: v1-2-1 v1-2-0 v1-1-0

Unstable NFC tag samples
  NFC tag samples are unstable when exhaustively tested (performing many repeated read and/or write operations).
  NFC tag data might be corrupted.

nRF5340 samples
===============

.. rst-class:: v2-3-0

NCSDK-20967: The :ref:`nrf_rpc_entropy_nrf53` sample does not work on the network core
  The network core will not work due a hard fault.

  **Affected platforms:** nRF5340

Peripheral samples
==================

.. rst-class:: v3-1-1 v3-1-0 v3-0-2 v3-0-1 v3-0-0

NCSDK-30519: The :ref:`radio_test` sample reports high packet error rate on the long-range data rates
  Packet error rate is very high in the receive mode when using the ``BLE_LR125KBIT`` and ``BLE_LR500KBIT`` data rates.

  **Affected platforms:** nRF54L15, nRF54L10, nRF54L05

.. rst-class:: v3-0-2 v3-0-1 v3-0-0

NCSDK-31981: Incorrect characters transmitted or received
  Incorrect characters are transmitted or received when using ``11110000`` pattern on 4 Mbit PHY with the :ref:`radio_test` sample.

  **Affected platforms:** nRF54L15

.. rst-class:: v2-2-0

NCSDK-18847: The :ref:`radio_test` sample does not build with support for Skyworks front-end module
  When building a sample with support for a front-end module different from nRF21540, the sample uses a non-existing configuration to initialize TX power data.
  This causes a compilation error because the source file containing code for a generic front-end module is not included in the build.

  **Workaround:** Do not use the :ref:`CONFIG_RADIO_TEST_POWER_CONTROL_AUTOMATIC <CONFIG_RADIO_TEST_POWER_CONTROL_AUTOMATIC>` Kconfig option and replace ``CONFIG_GENERIC_FEM`` with :kconfig:option:`CONFIG_MPSL_FEM_SIMPLE_GPIO` in the :file:`CMakeLists.txt` file of the sample.

.. rst-class:: v2-8-0 v2-7-0

NCSDK-30284: The :ref:`radio_test` sample only transmits the first burst of data in duty-cycle mode on the 4 Mbit data rate
  When the ``data_rate`` parameter is set to ``nrf_4Mbit_BT04``  or ``nrf_4Mbit_BT06``, sending the ``start_duty_cycle_modulated_tx`` command results in transmission of the first burst of data after which radio becomes inactive.

  **Affected platforms:** nRF54L15, nRF54H20

  **Workaround:** For ``nrf_4Mbit_BT04`` or ``nrf_4Mbit_BT06`` data rates, use only the constant transmission command ``start_tx_modulated_carrier`` instead of the duty-cycle one.

Other samples
=============

.. rst-class:: v3-1-1 v3-1-0

NCSDK-34698: CoreMark sample does not build or run due to the IronSide SE migration
  The :ref:`coremark_sample` sample is currently incompatible with the latest |NCS| transition to the IronSide SE solution - bundle 22.1.0 and higher for the ``nrf54h20dk/nrf54h20/cpuapp`` board target.
  As a result, you cannot use this sample with that board target in any of the affected releases.

  **Affected platforms:** nRF54H20

  **Workaround:** Use a future |NCS| release that includes IronSide SE support in the :ref:`coremark_sample` sample or use the |NCS| v3.0.0 release with the SUIT SDSC bundle (version up to 0.9.6).

.. rst-class:: v2-3-0

NCSDK-20046: IPC service sample does not work with ``nrf5340dk_nrf5340_cpuapp``
  The :ref:`ipc_service_sample` sample does not work with the ``nrf5340dk_nrf5340_cpuapp`` :ref:`board target <app_boards_names_zephyr>` due to a misconfiguration.
  The application core does not log anything, while the network core seems to work and bond, but cannot transfer data.
  When using UART, there is no output visible.

  **Affected platforms:** nRF5340

.. rst-class:: v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0

NCSDK-16856: Increased power consumption observed for the Low Power UART sample on nRF5340 DK
  The power consumption of the :ref:`lpuart_sample` sample measured using the nRF5340 DK v2.0.0 is about 200 uA higher than expected.

  **Affected platforms:** nRF5340

  **Workaround:** Disconnect flow control lines for VCOM2 with the SW7 DIP switch on the board.

.. rst-class:: v2-3-0

NCSDK-19858: :ref:`at_monitor_readme` library and :ref:`nrf_cloud_multi_service` sample heap overrun
  Occasionally, the :ref:`at_monitor_readme` library heap becomes overrun, presumably due to one of the registered AT event listeners becoming stalled.
  This has only been observed with the :ref:`nrf_cloud_multi_service` sample.

.. rst-class:: v2-3-0 v2-2-0

NCSDK-20095: Build warning in the RF test samples when the minimal pinout generic/Skyworks FEM is used
  The :ref:`radio_test` and :ref:`direct_test_mode` samples build with a warning about generic/Skyworks FEM in minimal pinout configuration.

.. rst-class:: v2-0-2 v2-0-1 v2-0-0

NCSDK-16338: Setting gain for nRF21540 Front-end module does not work in the :ref:`radio_test` sample
  Setting FEM gain for nRF21540 Front-end module does not work in the :ref:`radio_test` sample with the nRF5340 SoC.

  **Affected platforms:** nRF5340, nRF21540

  **Workaround:** Enable the SPI 0 instance in the nRF5340 network core DTS overlay file.

.. rst-class:: v1-9-0

NCSDK-13925: Build warning in the RF test samples when the nRF21540 EK support is enabled
  :ref:`radio_test` and :ref:`direct_test_mode` build with warnings for nRF5340 with the :ref:`ug_radio_fem_nrf21540ek` support.

  **Affected platforms:** nRF5340, nRF21540

  **Workaround:** Change the parameter type in the :c:func:`nrf21540_tx_gain_set` function in :file:`ncs/nrf/samples/bluetooth/direct_test_mode/src/fem/nrf21540.c` from :c:type:`uint8_t` to :c:type:`uint32_t`.

.. rst-class:: v1-5-2 v1-5-1 v1-5-0

NCSDK-7173: nRF5340 network core bootloader cannot be built stand-alone
  The :ref:`nc_bootloader` sample does not compile when built stand-alone.
  It compiles without problems when included as a child image.

  **Affected platforms:** nRF5340

  **Workaround:** Include the :ref:`nc_bootloader` sample as child image instead of compiling it stand-alone.

Libraries
*********

The issues in this section are related to :ref:`libraries`.

Binary libraries
================

.. rst-class:: v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0

NCSDK-26682: In the Verizon network, the :ref:`liblwm2m_carrier_readme` library fails to complete bootstrap process unless the same device has previously completed a bootstrap
  This is because one of the required pre-shared keys is not generated unless there is a pre-existing one in the modem.

  **Affected platforms:** nRF9161

  **Workaround:** Complete the following steps:

  #. Program any application that uses an earlier |NCS| version of the :ref:`liblwm2m_carrier_readme` library, for example `LwM2M carrier sample for v2.5.2`_.
  #. Wait till the application receives the :c:macro:`LWM2M_CARRIER_EVENT_BOOTSTRAPPED` event as described on the :ref:`LwM2M carrier sample description <lwm2m_carrier_sample_desc>` page.
  #. Program your application for |NCS| v2.6.0 .

.. _tnsw_46156:

.. rst-class:: v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0

TNSW-46156: The :ref:`liblwm2m_carrier_readme` library can in some cases trigger a modem bug related to PDN deactivation (AT+CGACT) in NB-IoT mode
  This causes excessive network signaling which can make interactions with the modem (AT commands or :ref:`nrfxlib:nrf_modem` functions) hang for several minutes.

  **Affected platforms:** nRF9160

  **Workaround:** Add a small delay before PDN deactivation as shown in `Pull Request #10379 <https://github.com/nrfconnect/sdk-nrf/pull/10379>`_.

.. rst-class:: v2-3-0 v2-2-0

NCSDK-18746: The :ref:`liblwm2m_carrier_readme` library fails to complete non-secure bootstrap when using a custom URI and the default security tag
  If the LwM2M carrier library is operating in generic mode (not connecting to any of the predefined supported carriers), and the :kconfig:option:`CONFIG_LWM2M_CARRIER_CUSTOM_URI` is set to connect to a non-secure server, the library attempts to retrieve a PSK from :kconfig:option:`CONFIG_LWM2M_CARRIER_SERVER_SEC_TAG` even though a PSK is not needed for the non-secure bootstrap.
  The PSK in this ``sec_tag`` is not used, but reading the ``sec_tag`` causes the bootstrap to fail if the :kconfig:option:`CONFIG_LWM2M_CARRIER_SERVER_SEC_TAG` is set to the default value (``0``).

  **Affected platforms:** nRF9160

  **Workaround:** Assign any non-zero value to :kconfig:option:`CONFIG_LWM2M_CARRIER_SERVER_SEC_TAG`.

.. rst-class:: v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-3-1 v1-2-1 v1-2-0

NCSDK-12912: The :ref:`liblwm2m_carrier_readme` library does not recover if initial network connection fails
  When the device is switched on, if the :c:func:`lte_lc_connect` function returns an error at timeout, it will cause ``lwm2m_carrier_init()`` to fail.
  Thus, the device will fail to connect to carrier device management servers.

  **Affected platforms:** nRF9160

  **Workaround:** Increase :kconfig:option:`CONFIG_LTE_NETWORK_TIMEOUT` to allow :ref:`lte_lc_readme` more time to successfully connect.

.. rst-class:: v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-1 v1-5-0 v1-4-2 v1-3-1 v1-2-1 v1-2-0

NCSDK-12913: The :ref:`liblwm2m_carrier_readme` library will fail to initialize if phone number is not present in SIM
  The SIM phone number is needed during the :ref:`liblwm2m_carrier_readme` library start-up.
  For new SIM cards, it might take some time before the phone number is received by the network.
  The LwM2M carrier library does not wait for this to happen.
  Thus, the device can fail to connect to the carrier's device management servers.

  **Affected platforms:** nRF9160

  **Workaround:** Use one of the following workarounds:

  * Reboot or power-cycle the device after the SIM has received a phone number from the network.
  * Apply the following commits, depending on your |NCS| version:

    * `v1.7-branch <https://github.com/nrfconnect/sdk-nrf/pull/6287>`_
    * `v1.6-branch <https://github.com/nrfconnect/sdk-nrf/pull/6286>`_
    * `v1.4-branch <https://github.com/nrfconnect/sdk-nrf/pull/6270>`_

.. rst-class:: v1-8-0

NCSDK-13106: When replacing a Verizon SIM card, the :ref:`liblwm2m_carrier_readme` library does not reconnect to the device management servers
  When a Verizon SIM card is replaced with a new Verizon SIM card, the library fails to fetch the correct PSK for the bootstrap server.
  Thus, the device fails to connect to the carrier's device management servers.

  **Affected platforms:** nRF9160

  **Workaround:** Use one of the following workarounds:

  * Use the :kconfig:option:`CONFIG_LWM2M_CARRIER_USE_CUSTOM_PSK` and :kconfig:option:`CONFIG_LWM2M_CARRIER_CUSTOM_PSK` configuration options to set the appropriate PSK needed for Verizon test or live servers.
    This PSK can be obtain from the carrier.
  * After inserting a new SIM card, reboot the device again.


Common Application Framework (CAF)
==================================

The issues in this section are related to :ref:`lib_caf`.

.. rst-class:: v2-9-0-nRF54H20-1 v2-9-2 v2-9-1 v2-9-0 v2-8-0 v2-7-0 v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0 v2-5-3 v2-5-2 v2-5-1 v2-5-0

NCSDK-31573: :ref:`caf_buttons` might fail to trigger wakeup from system off
  The issue replicates if a button is pressed right before system off (right before :ref:`caf_power_manager` calls :c:func:`sys_poweroff`).
  The CAF Buttons module handles related GPIO interrupt (and disables the GPIO interrupt), but the system off prevents CAF Buttons from performing buttons scan.
  The disabled GPIO interrupt leads to missing wakeup from system off for the button.

  The issue is easier to replicate in configurations with logs enabled, because the :c:macro:`LOG_PANIC` call significantly delays the :c:func:`sys_poweroff` function execution.

  **Workaround:** Cherry-pick commits with a fix from `sdk-nrf PR #20177`_.

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

NCSIDB-925: Event subscribers in the :ref:`app_event_manager` might overlap when using a non-default naming convention
  In order to locate the event subscribers start and stop address ranges, sections have to be sorted by the name with added postfixes.
  Hence, using a non-default event naming scheme might break the expected subscribers sorting.
  In this situation, one of the event subscribers arrays might be placed inside the other.
  So, when the event related to the outer subscribers is processed, the event subscribers that are inside are also executed.
  To resolve this issue, a new implementation was introduced that uses a section naming that cannot be used as event name (invalid variable identifier).

  **Workaround:** Use the default event names, ensuring that each event name ends with the ``_event`` postfix.
  Make sure that the event name does not start the same way as another event.
  For example, creating the following events: ``rx_event`` and ``rx_event_error_event`` would still cause the issue.

Modem libraries
===============

.. rst-class:: v2-2-0

CIA-857: LTE Link Controller spurious events
  When using the :ref:`lte_lc_readme` library, a memory comparison is done between padded structs that might result in comparing bytes that have undefined initialization values.
  This might cause spurious ``LTE_LC_EVT_CELL_UPDATE`` and ``LTE_LC_EVT_PSM_UPDATE`` events even though the information contained in the event has not changed since last update event.

  **Affected platforms:** nRF9160

.. rst-class:: v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0

NCSDK-15512: Modem traces retrieval incompatible with TF-M
  Enabling modem traces with :kconfig:option:`CONFIG_UART_NRF_MODEM_LIB_TRACE_BACKEND_UART` set to ``y`` will disable TF-M logging from secure processing environment (using :kconfig:option:`CONFIG_TFM_LOG_LEVEL_SILENCE` set to ``y``) including output from hardware fault handlers.
  You can either use **UART1** for TF-M output or for modem traces, but not for both.

  **Affected platforms:** nRF9160

Libraries for networking
========================

.. rst-class:: v3-1-1 v3-1-0 v3-0-2 v3-0-1 v3-0-0

IRIS-10140: CoAP error message: No client found for socket
  When the :kconfig:option:`CONFIG_NRF_CLOUD_COAP_DOWNLOADS` Kconfig option is enabled, an error "No client found for socket" can appear, depending on how the :ref:`CoAP client <zephyr:coap_client_interface>` library is used for authentication.
  This error message can be ignored for now.

.. rst-class:: v2-9-0-nRF54H20-1 v2-9-2 v2-9-1 v2-9-0 v2-8-0

CIA-1400: :ref:`lib_nrf_cloud` FOTA and P-GPS downloads over CoAP might fail to resume
  nRF Cloud FOTA and P-GPS downloads over CoAP might fail to resume at the correct file offset if the download is interrupted.

  **Workaround:** Disable the :kconfig:option:`CONFIG_NRF_CLOUD_COAP_DOWNLOADS` Kconfig option to use HTTP for downloads instead.

.. rst-class:: v2-7-0

NCSDK-28046: LwM2M firmware update cannot resume push mode delta update
  When LwM2M firmware update is used with modem delta image by writing image in CoAP block-wise transfer, previously failed or time-out transfer is not cleared out properly, and the FOTA process cannot resume.

  **Affected platforms:** nRF9160, nRF9161, nRF9151

  **Workaround:** Apply the fix from `sdk-nrf PR #15892`_.

.. rst-class:: v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0

NCSDK-26534: When :ref:`lib_lwm2m_client_utils` is used for downloading FOTA updates, ongoing updates cannot be cancelled by writing an empty URI
  This happens when downloading FOTA updates using LwM2M.

  **Workaround:** Apply the fix from `sdk-nrf PR #14474`_.

.. rst-class:: v2-7-0

NCSDK-28192: Hardfault in :c:func:`coap_codec_agnss_encode`
  When using the :ref:`lib_nrf_cloud_coap` library, if the ``net_info`` field of the request parameter provided to the :c:func:`coap_codec_agnss_encode` is ``NULL``,  a hardfault will occur.

  **Affected platforms:** nRF9160, nRF9161, nRF9151

  ** Workaround:** Apply the fix from `sdk-nrf PR #16242`_.

.. rst-class:: v3-1-1 v3-1-0 v3-0-2 v3-0-1 v3-0-0 v2-9-0-nRF54H20-1 v2-9-2 v2-9-1 v2-9-0 v2-8-0 v2-7-0 v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0 v2-5-3 v2-5-2 v2-5-1 v2-5-0 v2-4-4 v2-4-3 v2-4-2 v2-4-1 v2-4-0 v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0

CIA-351: Connectivity issues with :ref:`lib_azure_iot_hub`
  If a ``device-bound`` message is sent to the device while it is in the LTE Power Saving Mode (PSM), the TCP connection will most likely be terminated by the server.
  Known symptoms of this are frequent reconnections to cloud, messages sent to Azure IoT Hub never arriving, and FOTA images being downloaded twice.

  **Affected platforms:** nRF9160, nRF9161, nRF9151, nRF9131

  **Workaround:** Avoid using LTE Power Saving Mode (PSM) and extended DRX intervals longer than approximately 30 seconds. This will reduce the risk of the issue occurring, at the cost of increased power consumption.


Bluetooth libraries and services
================================

.. _ncsdk_30288:

.. rst-class:: v2-8-0 v2-7-0 v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0 v2-5-3 v2-5-2 v2-5-1 v2-5-0 v2-4-4 v2-4-3 v2-4-2 v2-4-1 v2-4-0 v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0 v1-0-0 v0-4-0

NCSDK-30288: The :c:func:`bt_hogp_rep_read` function of :ref:`hogp_readme` library does not forward ATT error code through the user callback
  The library passes value of ``0`` to the user instead of the error code.

  **Workaround:** Manually cherry-pick and apply the commit with the fix from the ``main`` branch (commit hash: ``0d227d82bbdf56a3c066021fa6c323a00107fe6f``).

.. rst-class:: v2-4-3 v2-4-2 v2-4-1 v2-4-0

NCSDK-23315: The :ref:`bt_le_adv_prov_readme` has an incorrect range and default value for the :kconfig:option:`CONFIG_BT_ADV_PROV_FAST_PAIR_ADV_BUF_SIZE` Kconfig option
  The buffer size has not been aligned to the new Fast Pair not discoverable advertising data size after size of the salt included in the data was increased from 1 byte to 2 bytes.
  Too small default buffer size results in an assertion failure when generating an advertising data for the Fast Pair not discoverable advertising with five Account Keys and the Battery Notification enabled.
  The assertion failure replicates in the :ref:`fast_pair_input_device` sample.

  **Workaround:** Manually cherry-pick and apply the commit with the fix from the ``main`` branch (commit hash: ``a8b668e82837295962348e9e681125c2ae11bb4e``).

.. rst-class:: v3-1-1 v3-1-0 v3-0-2 v3-0-1 v3-0-0 v2-9-0-nRF54H20-1 v2-9-2 v2-9-1 v2-9-0 v2-8-0 v2-7-0 v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0 v2-5-3 v2-5-2 v2-5-1 v2-5-0 v2-4-4 v2-4-3 v2-4-2 v2-4-1 v2-4-0 v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0

NCSDK-23682: The Fast Pair Seeker might be unable to bond again after losing the bonding information multiple times
  If the :kconfig:option:`CONFIG_BT_SETTINGS_CCC_LAZY_LOADING` Kconfig option is disabled on the Fast Pair Provider side, the Fast Pair Seeker that uses the RPA address to connect with the Provider might be unable to bond again after losing the bonding information multiple times.
  The issue is because of `Zephyr issue #64042 <https://github.com/zephyrproject-rtos/zephyr/issues/64042>`_.

  **Workaround:** Keep the :kconfig:option:`CONFIG_BT_SETTINGS_CCC_LAZY_LOADING` Kconfig option enabled.

.. rst-class:: v2-9-0-nRF54H20-1 v2-9-2 v2-9-1 v2-9-0 v2-8-0

NCSDK-32268: FMDN clock might not be correctly set after the system reboot for nRF54L Series devices
  The clock value in the Find My Device Network (FMDN) module is calculated using the kernel uptime function (:c:func:`k_uptime_get`).
  For nRF54L Series devices, the kernel uptime persists after a system reset if the reset reason is of a specific type, such as a software reset (for example, triggered by the :c:func:`sys_reboot` API during the DFU process).
  This platform-specific behavior breaks the assumption in the FMDN clock module that the uptime starts from zero after the system reboot.
  For more information, see the description of the ``CONFIG_BT_FAST_PAIR_FMDN_CLOCK_UPTIME_PERSISTENCE`` Kconfig option that activates a workaround for this issue as part of the following patch commit.

  **Affected platforms:** nRF54L Series

  **Workaround:** Manually cherry-pick and apply the commit with the fix from the ``main`` branch (commit hash: ``190839b44b678e2a90793e01471155fa9e579dc1``).

  .. note::
     From the |NCS| v3.1.0 and later, the kernel uptime value that is returned by the :c:func:`k_uptime_get` function is correctly set to ``0`` during the system bootup process for each reset type.
     As a result, the dedicated workaround in the FMDN clock module is no longer needed and has been removed together with the ``CONFIG_BT_FAST_PAIR_FMDN_CLOCK_UPTIME_PERSISTENCE`` Kconfig option.

Other libraries
===============

.. rst-class:: v2-4-3 v2-4-2 v2-4-1 v2-4-0 v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-1 v1-0-0 v0-4-0

NCSDK-22908: The :ref:`st25r3911b_nfc_readme` library returns a processing error
 The library returns a processing error in case the Rx complete event is received together with the FIFO water level event.

.. rst-class:: v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0

NCSDK-18398: Build fails if shell is enabled
   Enabling the Zephyr's :ref:`zephyr:shell_api` module together with :ref:`nrf_profiler` results in a build failure because of the bug in the :file:`CMakeLists.txt` file.

  **Workaround:** Manually cherry-pick and apply the commit with the fix from the ``main`` branch (commit hash: ``fdac428e902ebd96885160dd3ae5d08d21642926``).

.. rst-class:: v2-0-2 v2-0-1 v2-0-0

NCSDK-15471: Compilation with :ref:`SUPL client <supl_client>` library fails when TF-M is enabled
  Building an application that uses the SUPL client library fails if TF-M is used.

  **Affected platforms:** nRF9160

  **Workaround:** Use one of the following workarounds:

  * Use Secure Partition Manager instead of TF-M.
  * Disable the FPU by setting :kconfig:option:`CONFIG_FPU` to ``n``.

.. rst-class:: v1-9-2 v1-9-1 v1-9-0

The time returned by :ref:`lib_date_time` library becomes incorrect after one week of uptime
  The time returned by :ref:`lib_date_time` library becomes incorrect after one week elapses.
  This is due to an issue with ``clock_gettime()`` API.

  **Affected platforms:** nRF9160, nRF52840

Security
========

.. rst-class:: v3-1-1 v3-1-0 v3-0-2 v3-0-1 v3-0-0 v2-9-0-nRF54H20-1 v2-9-2 v2-9-1 v2-9-0 v2-8-0

NCSDK-29559: KMU slots 0-2 cannot be used to store keys in nRF54L15
  The application cannot use KMU slots 0-2 to store keys in the nRF54L15.
  The import of the keys will work but they will fail when used.
  These slots can still be used to store the CRACEN IKG seed using the :kconfig:option:`CONFIG_CRACEN_IKG_SEED_KMU_SLOT` Kconfig option.

Subsystems
**********

The issues in this section are related to different subsystems.

Build system
============

The issues in this section are related to :ref:`app_build_system`.

.. rst-class:: v3-1-1 v3-1-0 v3-0-2 v3-0-1 v3-0-0 v2-9-0-nRF54H20-1 v2-9-2 v2-9-1 v2-9-0 v2-8-0 v2-7-0 v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0 v2-5-3 v2-5-2 v2-5-1 v2-5-0 v2-4-4 v2-4-3 v2-4-2 v2-4-1 v2-4-0 v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0

Microsoft PC Manager’s real-time scanning can severely impact NCS build performance on Windows 11
  MsPcManagerService (from Microsoft PC Manager) is constantly scanning several files during compilation.

  **Workaround:** Stop the MsPcManagerService in the Windows Services panel.
  This improves the build speed significantly.
  For older Windows versions, check your antivirus system.
  It might slow down the build process.

.. rst-class:: v3-1-1 v3-1-0 v3-0-2 v3-0-1 v3-0-0

VSC-2974: On Windows, some sample paths are too long
  The maximum full path to an object file is 250 characters.

  **Workaround:** Symlink your development sample folder to a shorter path.
  For example:

  .. code-block::

     mklink  /J  C:\ncs\sample-dev C:\ncs\v3.0.0\nrf\samples\cellular\http_update\modem_full_update

  Import :file:`C:\ncs\sample-dev` to VS-Code, and the build should work.

.. rst-class:: v3-1-1 v3-1-0 v3-0-2 v3-0-1 v3-0-0 v2-9-0-nRF54H20-1 v2-9-2 v2-9-1 v2-9-0 v2-8-0

NCSDK-30119: For nRF54L15 SoC, dynamic partitioning for a project building with MCUboot's direct-xip mode is not supported
  Static partition manager file is required for building such project.

.. rst-class:: v2-7-0

NCSDK-28495: Sysbuild runs CMake code before processing application :file:`sysbuild.cmake`
  When using a :file:`sysbuild.cmake` file to set configuration for images that have variants, this extra configuration might end up not being applied to the variant image, causing the images to be incompatible.

  **Workaround:** Depending on your configuration:

  * If applying configuration to the default image and building in the direct XIP mode, also apply the configuration to ``mcuboot_secondary_app``.
  * If applying configuration to MCUboot with application secure boot enabled, also apply the configuration to ``s1_image``.
  * If applying configuration to the default image with application secure boot enabled and MCUboot disabled, also apply the configuration to ``s1_image``.

.. rst-class:: v2-7-0

NCSDK-28462: MCUboot signing configuration cannot be updated without pristine build
  When using :ref:`configuration_system_overview_sysbuild`, the MCUboot signing configuration cannot be updated in an already configured project.

  **Workaround:** Perform a :ref:`pristine build <zephyr:west-building-pristine>` to change any of the MCUboot signing configuration. Do not update it using menuconfig, guiconfig or the nRF Kconfig GUI, and instead :ref:`provide it on the command line <cmake_options>` when configuring the application if it needs setting manually.

.. rst-class:: v3-1-1 v3-1-0 v3-0-2 v3-0-1 v3-0-0 v2-9-0-nRF54H20-1 v2-9-2 v2-9-1 v2-9-0 v2-8-0 v2-7-0

NCSDK-28461: Sysbuild partition manager file changes cannot be propagated to builds unless they are pristine
  When using :ref:`configuration_system_overview_sysbuild` and a :ref:`partition_manager` file, the Partition Manager configuration for things such as MCUboot signing will not be updated if the Partition Manager configuration is changed in an already configured project.

  **Workaround:** Perform a :ref:`pristine build <zephyr:west-building-pristine>` after changing configuration in Partition Manager files.

.. rst-class:: v2-7-0

NCSDK-28451: Sysbuild silently does not use relative path (relative to application config dir) user-specified (PM_STATIC_YML_FILE) static PM files
  When building an application using :ref:`configuration_system_overview_sysbuild` with a :ref:`static partition file <ug_pm_static_providing>` specified using ``PM_STATIC_YML_FILE`` with a relative path, the relative path will be relative to the sysbuild folder in Zephyr, not to the application configuration directory, and the file will silently be ignored.

  **Workaround:** Use an absolute path when specifying the static partition file and ensure that the output shows the file as being used.

.. rst-class:: v3-0-2 v3-0-1 v3-0-0 v2-9-0-nRF54H20-1 v2-9-2 v2-9-1 v2-9-0 v2-8-0 v2-7-0 v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0 v2-5-3 v2-5-2 v2-5-1 v2-5-0 v2-4-4 v2-4-3 v2-4-2 v2-4-1 v2-4-0 v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0

NCSDK-20567: When building an application for MCUboot, the build system does not check whether the compiled application is too big for being an update image
  In this case, the update cannot be applied because the swap algorithm requires some free space in the secondary slot (even if the image fits inside the slot).
  Check the size requirements in the :ref:`MCUboot documentation <mcuboot:mcuboot_index_ncs>`.

.. rst-class:: v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0

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

.. rst-class:: v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0

NCSDK-9786: Wrong FLASH_PAGE_ERASE_MAX_TIME_US for the nRF53 network core
  ``FLASH_PAGE_ERASE_MAX_TIME_US`` defines the execution window duration when doing the flash operation synchronously along the radio operations (:kconfig:option:`CONFIG_SOC_FLASH_NRF_PARTIAL_ERASE` not enabled).

  The ``FLASH_PAGE_ERASE_MAX_TIME_US`` value of the nRF53 network core is lower than required.
  For this reason, if :kconfig:option:`CONFIG_SOC_FLASH_NRF_RADIO_SYNC_MPSL` is set to ``y`` and :kconfig:option:`CONFIG_SOC_FLASH_NRF_PARTIAL_ERASE` is set to ``n``, a flash erase operation on the nRF5340 network core will result in an MPSL timeslot OVERSTAYED assert.

  **Affected platforms:** nRF5340

  **Workaround:** Increase ``FLASH_PAGE_ERASE_MAX_TIME_US`` (defined in :file:`ncs/zephyr/soc/arm/nordic_nrf/nrf53/soc.h`) from ``44850UL`` to ``89700UL`` (the same value as for the application core).

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

.. rst-class:: v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0

NCSDK-7234: UART output is not received from the network core
  The UART output is not received from the network core if the application core is programmed and running with a non-secure image (using the ``nrf5340dk_nrf5340_cpuapp_ns`` board target).

  **Affected platforms:** nRF5340

.. rst-class:: v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0

NCSDK-7982: Partition manager: Incorrect partition size linkage from name conflict
  The partition manager will incorrectly link a partition's size to the size of its container if the container partition's name matches its child image's name in :file:`CMakeLists.txt`.
  This can cause the inappropriately-sized partition to overwrite another partition beyond its intended boundary.

  **Workaround:** Rename the container partitions in the :file:`pm.yml` and :file:`pm_static.yml` files to something that does not match the child images' names, and rename the child images' main image partition to its name in :file:`CMakeLists.txt`.

.. rst-class:: v1-3-2 v1-3-1 v1-3-0

Missing :file:`CMakeLists.txt`
  The :file:`CMakeLists.txt` file for developing applications that emulate nRF52820 on the nRF52833 DK is missing.

  **Affected platforms:** nRF52833, nRF52820

  **Workaround:** Create a :file:`CMakeLists.txt` file in the :file:`ncs/zephyr/boards/arm/nrf52833dk_nrf52820` folder with the following content:

  .. parsed-literal::
    :class: highlight

    zephyr_compile_definitions(DEVELOP_IN_NRF52833)
    zephyr_compile_definitions(NRFX_COREDEP_DELAY_US_LOOP_CYCLES=3)

  You can `download this file <nRF52820 CMakeLists.txt_>`_ from the upstream Zephyr repository.
  After you add it, the file is automatically included by the build system.

.. rst-class:: v2-8-0 v2-7-0 v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0 v2-5-3 v2-5-2 v2-5-1 v2-5-0 v2-4-4 v2-4-3 v2-4-2 v2-4-1 v2-4-0 v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0

NCSDK-29124: Cannot set the NSIB signing key using environment or command line value while using child image for a project
  Environment value ``SB_SIGNING_KEY_FILE`` and command line value passing under ``-DSB_SIGNING_KEY_FILE=<path_to_pem_file>`` are ignored by build system.
  Instead, an auto-generated key is used for signing.

  **Affected platforms:** nRF52 Series, nRF5340, nRF91 Series

  **Workaround:** Always set the :kconfig:option:`CONFIG_SB_SIGNING_KEY_FILE` value for the B0 child image.

Bootloader
==========

The issues in this section are related to :ref:`app_bootloaders`.

.. rst-class:: v3-1-1 v3-1-0 v3-0-2 v3-0-1 v3-0-0 v2-9-0-nRF54H20-1 v2-9-2 v2-9-1 v2-9-0 v2-8-0 v2-7-0 v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0 v2-5-3 v2-5-2 v2-5-1 v2-5-0 v2-4-4 v2-4-3 v2-4-2 v2-4-1 v2-4-0 v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0

NCSDK-19265: Monotonic counter update protection counter limit does not work
  The rollback protection in NSIB does not validate counter values if the values cannot be retrieved correctly or if they are larger than the largest supported value.
  In such a case, the image booted might be one whose values is lower than expected NV counter value or a device can be updated more times than expected by the :kconfig:option:`CONFIG_SB_NUM_VER_COUNTER_SLOTS` Kconfig option.
  This issue affects all devices running NSIB.

.. rst-class:: v3-1-1 v3-1-0 v3-0-2 v3-0-1 v3-0-0 v2-9-0-nRF54H20-1 v2-9-2 v2-9-1 v2-9-0 v2-8-0 v2-7-0 v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0 v2-5-3 v2-5-2 v2-5-1 v2-5-0 v2-4-4 v2-4-3 v2-4-2 v2-4-1 v2-4-0 v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0

NCSDK-31918: NSIB active slot function can give invalid results
  The function for getting the active s0 or s1 slot only checks the :c:struct:`fw_info` structure.
  The image itself is not validated nor is NSIB queried for which slot is active.
  This means that if an image is invalidated but has a higher firmware version or if the image is corrupt or even entirely missing (except for the :c:struct:`fw_info` structure), the response to this query will return the wrong image, possibly leading to the wrong update being loaded or the impossibility of loading a firmware update (depending upon firmware configuration).

.. rst-class:: v2-5-3 v2-5-2 v2-5-1 v2-5-0

NCSDK-24203: If fault injection hardening (FIH) is enabled, a bug is observed in the :c:func:`boot_image_check_hook` function
  Due to this, multicore applications cannot be booted for nRF5340 MCUboot builds with simultaneous multi-image update enabled.

  **Affected platforms:** nRF5340, Thingy:53

  **Workaround:** Disable fault injection hardening (FIH) or cherry-pick the commit in `PR #12846 <https://github.com/nrfconnect/sdk-nrf/pull/12846>`_.

.. rst-class:: v2-4-3 v2-4-2 v2-4-0 v2-4-1

NCSDK-23761: MCUboot fails to boot when both the :kconfig:option:`CONFIG_MCUBOOT_HW_DOWNGRADE_PREVENTION` and :kconfig:option:`CONFIG_BOOT_FIH_PROFILE_LOW` Kconfig options are enabled
  The MCUboot downgrade prevention mechanism relies on platform-specific implementation of hardware security counters.
  The |NCS| implementation of the hardware security counters is not compatible with the fault injection hardening code (FIH) of MCUboot.
  In the |NCS|, the Kconfig option :kconfig:option:`CONFIG_BOOT_FIH_PROFILE_LOW` of MCUboot is enabled by default for TF-M builds.
  You can enable the MCUboot Kconfig option :kconfig:option:`CONFIG_MCUBOOT_HW_DOWNGRADE_PREVENTION` in the |NCS| using the option :kconfig:option:`CONFIG_MCUBOOT_HARDWARE_DOWNGRADE_PREVENTION`.

  **Workaround:** To fix the issue, disable either the :kconfig:option:`CONFIG_MCUBOOT_HW_DOWNGRADE_PREVENTION` or :kconfig:option:`CONFIG_BOOT_FIH_PROFILE_LOW` Kconfig option in MCUboot.

.. rst-class:: v2-8-0 v2-7-0 v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0 v2-5-3 v2-5-2 v2-5-1 v2-5-0 v2-4-4 v2-4-3 v2-4-2 v2-4-1 v2-4-0 v2-3-0

SHEL-1352: Incorrect base address used in the OTP TX trim coefficients
  Incorrect base address used for TX trim coefficients in the One-Time Programmable (OTP) memory results in transmit power deviating by +/- 2 dB from the target value.

  **Affected platforms:** nRF7002

.. rst-class:: v2-2-0

NCSDK-18376: P-GPS in external flash fails to inject first prediction during load sequence
  When using external flash with P-GPS, stored predictions cannot be reliably read to satisfy the modem's need for ephemeris data when actively downloading and storing predictions to the same external flash.
  Once the download is complete, the predictions can be reliably read.

  **Affected platforms:** nRF9160

.. rst-class:: v1-1-0

Public keys revocation
  Public keys are not revoked when subsequent keys are used.

.. rst-class:: v1-1-0

Incompatibility with nRF51
  The bootloader does not work properly on nRF51.

  **Affected platforms:** nRF51 Series

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

.. rst-class:: v1-4-2 v1-4-1 v1-4-0

NRF91-989: Unable to bootstrap after changing SIMs
  In some cases, swapping the SIM card might trigger the bootstrap pre-shared key (PSK) to be deleted from the device.
  This can prevent future bootstraps from succeeding.

  **Affected platforms:** nRF9160

DFU and FOTA
============

The issues in this section are related to :ref:`app_dfu`.

.. rst-class:: v3-0-2 v3-0-1 v3-0-0 v2-9-0-nRF54H20-1

NCSDK-31270: [suit] Entering the recovery mode by **HW** button press is disabled by default
  This is done for samples with external flash enabled.

  **Affected platforms:** nRF54H20

  **Workaround:** In the manifest file, enable support for entering the recovery mode by **HW** button press.

.. rst-class:: wontfix v3-0-2 v3-0-1 v3-0-0 v2-9-0-nRF54H20-1

NCSDK-31300: [suit] Leaving the recovery mode using the default DFU candidate is not possible
  For samples with external flash and the Cache Pull mode, it is not possible to leave the recovery mode using the default DFU candidate.

  **Affected platforms:** nRF54H20

  **Workaround:** In such cases, preparation of a custom DFU candidate is needed.

.. rst-class:: v2-9-0-nRF54H20-1

NCSDK-31135: [suit] DFU through the external flash is failing sporadically
  DFU through the external flash is failing sporadically, although no error is reported.

  **Affected platforms:** nRF54H20

  **Workaround:** Repeat DFU procedure.

.. rst-class:: wontfix v3-0-2 v3-0-1 v3-0-0 v2-9-0-nRF54H20-1

NCSDK-30976: [suit] Update candidate envelope with oversized manifest are not rejected
  An update candidate envelope with a manifest that is too large to fit into its applicable storage slot is still attempted to be installed.
  This results in mismatches between digests for images or dependencies, leading to boot failure.

  **Affected platforms:** nRF54H20

.. rst-class:: v2-9-0-nRF54H20-1 v2-9-2 v2-9-1 v2-9-0 v2-8-0

NCSDK-30161: Combination of :kconfig:option:`CONFIG_ASSERT`, :kconfig:option:`CONFIG_SOC_NRF54H20_GPD`, and external flash causes an assertion during boot time
  A combination of these three will cause an assert.
  Disabling one of them will fix the issue.

  **Affected platforms:** nRF54H20

  **Workaround:** Set :kconfig:option:`CONFIG_ASSERT` to ``n`` or :kconfig:option:`CONFIG_SOC_NRF54H20_GPD` to ``n``.

.. rst-class:: v2-9-0-nRF54H20-1 v2-9-2 v2-9-1 v2-9-0 v2-8-0

NCSDK-30117: [suit] It is possible to declare a MEM component pointing to a memory region not assigned to particular core
  An update candidate with envelope for APP/RAD containing declared MEM component that points to memory range outside of space assigned to a particular core is accepted and installed.

  **Affected platforms:** nRF54H20

  **Workaround:** Declare MEM components pointing to correct regions.
  This issue will be fixed in further releases.

.. rst-class:: v2-9-0-nRF54H20-1 v2-9-2 v2-9-1 v2-9-0 v2-8-0

NCSDK-29682: [suit] cose-alg-sha-512 is not supported
  SUIT envelope using digest with algorithm cose-alg-sha-512 is rejected due to unsupported algorithm.

  **Affected platforms:** nRF54H20

  **Workaround:** Use sha-256 until next release of Nordic binaries where this issue will be fixed.

.. rst-class:: wontfix v3-0-2 v3-0-1 v3-0-0 v2-9-0-nRF54H20-1 v2-9-2 v2-9-1 v2-9-0 v2-8-0

NCSDK-28241: DFU transfer starts and fails if previous transfer is still in progress
  Since the mobile application implements the :guilabel:`Cancel` button, it is possible to restart the transfer on the mobile phone even if the first transfer is still in progress (image fetcher waits for a timeout event or the external flash is being erased, which takes ~5 seconds).
  As a result, such restarted transfer fails due to state mismatch in the DFU cache module.

  **Affected platforms:** nRF54H20

  **Workaround:** Repeat DFU transfer attempt - the next transfer attempt will succeed.

.. rst-class:: wontfix v3-0-2 v3-0-1 v3-0-0 v2-9-0-nRF54H20-1 v2-9-2 v2-9-1 v2-9-0 v2-8-0 v2-7-0 v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0 v2-5-3 v2-5-2 v2-5-1 v2-5-0 v2-4-4 v2-4-3 v2-4-2 v2-4-1 v2-4-0

NCSDK-21790: Errors during DFU when using nRF Connect for mobile app
  MCUmgr is incorrectly reporting an error when DFU is performed using the nRF Connect for mobile app.
  The DFU functionality is working as expected.

  **Affected platforms:** nRF5340

.. rst-class:: v2-5-3 v2-5-2 v2-5-1 v2-5-0 v2-4-4 v2-4-3 v2-4-2 v2-4-1 v2-4-0 v2-3-0

NCSDK-21379: Single slot network core updates on nRF5340 does not work properly
  Currently, a bug in the MCUboot code causes network core updates to not be applied when using the nRF5340 in a single slot configuration.

  **Affected platforms:** nRF5340

  **Workaround:** Use one of the following workarounds:

  * Enable the :kconfig:option:`CONFIG_BOOT_IMAGE_ACCESS_HOOKS` Kconfig option and copy the routine for updating the network core from :file:`loader.c` in MCUboot or :file:`nrf53_hooks.c`.
  * Use the dual-slot configuration of MCUboot.

.. rst-class:: v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0

CIA-738: FMFU does not use external flash partitions
  Full modem FOTA support assumes full control of the external flash; it does not use an external flash partition.
  It cannot be combined with other usages, such as storing settings or P-GPS data.

  **Affected platforms:** nRF9160

.. rst-class:: v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0

NCSDK-18357: Serial recovery does not work on nRF5340 network core
  If a network core serial recovery is attempted using MCUboot serial recovery, the upload will complete, but the image will not be transferred to the network core and the upgrade will fail.

  **Affected platforms:** nRF5340

.. rst-class:: v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0

NCSDK-18422: Serial recovery fails to write to slots in QSPI
  If a slot resides in an external QSPI storage area and this area is written to in MCUboot's serial recovery system, the writing will not work due to memory buffer alignment offsets and requirements with the underlying flash driver.

.. rst-class:: v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0

NCSDK-18108: ``s1`` variant image configuration mismatch
  If an image with an ``s1`` variant is configured and the ``s0`` image configuration is changed using menuconfig, these changes will not be reflected in the ``s1`` configuration, which can lead to a differing build configuration or the build does not upgrade.

.. rst-class:: wontfix v3-1-1 v3-1-0 v3-0-2 v3-0-1 v3-0-0 v2-9-0-nRF54H20-1 v2-9-2 v2-9-1 v2-9-0 v2-8-0 v2-7-0 v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0 v2-5-3 v2-5-2 v2-5-1 v2-5-0 v2-4-4 v2-4-3 v2-4-2 v2-4-1 v2-4-0 v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0

NCSDK-11308: Powering off device immediately after serial recovery of the nRF53 network core firmware results in broken firmware
  The network core will not be able to boot if the device is powered off too soon after completing a serial recovery update procedure of the network core firmware.
  This is because the firmware is being copied from shared RAM to network core flash **after** mcumgr indicates that the serial recovery procedure has completed.

  **Affected platforms:** nRF5340

  **Workaround:** Use one of the following workarounds:

  * Wait for 30 seconds before powering off the device after performing serial recovery of the nRF53 network core firmware.
  * Re-upload the network core firmware with a new serial recovery procedure if the device was powered off too soon in a previous procedure.

.. rst-class:: v2-0-0 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0

NCSDK-11432: DFU: Erasing secondary slot returns error response
  Trying to erase secondary slot results in an error response.
  Slot is still erased.
  This issue is only occurring when the application is compiled for multi-image.

  **Affected platforms:** nRF5340

.. rst-class:: v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0

NCSDK-6238: Socket API calls might hang when using Download client
  When using the Download client library with HTTP (without TLS), the application might not process incoming fragments fast enough, which can starve the :ref:`nrfxlib:nrf_modem` buffers and make calls to the Modem library hang.
  Samples and applications that are affected include those that use the Download client library to download files through HTTP, or those that use :ref:`lib_fota_download` with modem updates enabled.

  **Workaround:** Set ``CONFIG_DOWNLOAD_CLIENT_RANGE_REQUESTS`` with the Download client library.

.. rst-class:: v1-1-0

Jobs not received after reset
  When using :ref:`lib_aws_fota`, no new jobs are received on the device if the device is reset during a firmware upgrade or loses the MQTT connection.

  **Workaround:** Delete the stalled in progress job from AWS IoT.

.. rst-class:: v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0 v2-5-3 v2-5-2 v2-5-1 v2-5-0

NCSDK-24305: fota_download library sends FOTA_DOWNLOAD_EVT_FINISHED when unable to connect
  The Download client library do not resume a download if the device cannot connect to a target server.
  This causes the :ref:`lib_fota_download` library to incorrectly assume that the download has completed.

  **Workaround:** Set the :kconfig:option:`CONFIG_FOTA_SOCKET_RETRIES` Kconfig option to ``0``.

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

Logging
=======

.. rst-class:: v2-4-3 v2-4-2 v2-4-1 v2-4-0 v2-3-0

CIA-892: Assert or crash when printing long strings with the ``%s`` qualifier
  When logging a string with the ``%s`` qualifier, the maximum length of any inserted string is 1022 bytes.
  If the string is longer, an assert or a crash can occur.
  Given that the default value of the :kconfig:option:`CONFIG_LOG_PRINTK` Kconfig option has been changed from ``n`` to ``y`` in the |NCS| v2.3.0 release, the :c:func:`printk` function might also cause this issue, unless the application disables this option.

  **Workaround:** To fix the issue, cherry-pick commits from the upstream `Zephyr PR #54901 <https://github.com/zephyrproject-rtos/zephyr/pull/54901>`_.

.. rst-class:: v1-2-0 v1-1-0 v1-0-0 v0-4-0 v0-3-0

Problems with RTT Viewer/Logger
  The SEGGER Control Block cannot be found by automatic search by the RTT Viewer/Logger.

  **Affected platforms:** nRF9160

  **Workaround:** Set the RTT Control Block address to 0 and it will try to search from address 0 and upwards.
  If this does not work, look in the :file:`builddir/zephyr/zephyr.map` file to find the address of the ``_SEGGER_RTT`` symbol in the map file and use that as input to the viewer/logger.

NFC
===

.. rst-class:: v2-4-3 v2-4-2 v2-4-1 v2-4-0 v2-3-0

NCSDK-22799: Assert when requesting clock from the NFC interrupt context
  The NFC interrupt is a low latency interrupt.
  It calls the Zephyr subsystem API that can rarely cause undefined behavior.

  **Workaround:** To fix the issue, disable the :kconfig:option:`CONFIG_NFC_ZERO_LATENCY_IRQ` Kconfig option.

MCUboot
*******

The issues in this section are related to :ref:`MCUboot <mcuboot_wrapper>`.

.. rst-class:: v3-1-1

NCSDK-35259 Upgrade failed when encryption was enabled
  Encryption is currently not supported for nRF54LM20.

  **Affected platforms:** nRF54LM20

.. rst-class:: v3-1-1

NCSDK-34251 Device fails to boot when downgrade limit is reached
  The device will not boot when Hardware Downgrade Prevention reaches the number of available upgrades.

  **Affected platforms:** nRF54LM20

  **Workaround:** Cherry-pick changes from PR #23779 in the `sdk-nrf`_ repository.

.. rst-class:: v3-1-1 v3-1-0

NCSDK-34928: Confirmation of ``ipc_radio`` image fails when test and confirm are done for both images in one cycle on the nRF54H20
  When the test is triggered for both firmware images, it is not possible to confirm ``ipc_radio`` image after restart.

  **Affected platforms:** nRF54H20

  **Workaround:** Apply test and confirm for ``ipc_radio`` in next boot cycle (after restart).

.. rst-class:: v3-1-1 v3-1-0

NCSDK-34594: MCUboot might hang and is failing on FPROTECT assertion when using external flash
  FPROTECT can support locking up to 62 KB of RRAM on nRF54L SoCs when used by immutable MCUboot.
  Either the partition declared for MCUboot or its size might be too large for using FPROTECT (depends on MCUboot configuration).
  Without access to MCUboot logs, this might be misrecognized as an application that is not working or starting.

  **Affected platforms:** nRF54L15, nRF54L10, nRF54L05

  **Workaround:** Decrease MCUboot partition size or disable FPROTEC.

.. rst-class:: v3-1-1 v3-1-0

NCSDK-34894: Reset during "Swap with scratch" may not recover correct swap status
  After a reset during "Swap with scratch" operation, the current swap status may not recover correctly, leading to unnecessary flash writes.

  **Affected platforms:** nRF54H20

.. rst-class:: v3-0-2 v3-0-1 v3-0-0

NCSDK-33163: MCUboot does not boot new images
  MCUboot does not boot new images when old keys revocation is enabled.

  **Affected platforms:** nRF54L

  **Workaround:** To fix the issue, update the ``sdk-mcuboot`` repository by cherry-picking the upstream commits with the following hashes: ``85ed722d00dc4e60804b34a066f642d80a6bc67f``

.. rst-class:: v3-0-2 v3-0-1 v3-0-0

NCSDK-33207: MCUboot has its NVM protection disabled
  MCUboot built as immutable bootloader has fprotect disabled by default on the nRF54L DK.

  **Affected platforms:** nRF54L

  **Workaround:** Enable protection manually using the :kconfig:option:`CONFIG_FPROTECT` and  :kconfig:option:`CONFIG_FPROTECT_ALLOW_COMBINED_REGIONS` Kconfig options of MCUboot.

.. rst-class:: wontfix v3-1-1 v3-1-0 v3-0-2 v3-0-1 v3-0-0

NCSDK-29460: Encryption: Build error for default configuration on the ``nrf52840dk/nrf52840`` board target (ECDSA_P256)
  This happens because of inconsistency in the configuration of the signature check and the encryption key extraction: both must use the same base encryption algorithm.

  **Affected platforms:** nRF52840

  **Workaround:** Switch the signature algorithm to RSA or change the crypto library to TinyCrypt.

.. rst-class:: v2-9-0-nRF54H20-1 v2-9-2 v2-9-1 v2-9-0 v2-8-0

NCSDK-30867: KMU keys are not protected against being revoked by the application that is running in secure mode
  You must provision *locked* keys if you cannot trust the application running in secure mode.

  **Affected platforms:** nRF54L Series

.. rst-class:: v2-8-0

NCSDK-30263: direct-xip with revert does not work on nRF54L15
  The revert mechanism in direct-xip mode does not work on nRF54L15 devices.

  **Affected platforms:** nRF54L15

  **Workaround:** Manually cherry-pick the commit ``ff0e8fabe1566349dbfd1786b31b325b46be205a`` from the main branch of the ``sdk-nrf`` repository.

.. rst-class:: v2-9-0-nRF54H20-1 v2-9-2 v2-9-1 v2-9-0 v2-8-0

NCSDK-31066: MCUboot updates take a long time
  The current implementation of MCUboot does not optimally utilize RRAM, which causes longer update times.

  **Affected platforms:** nRF54L Series

  **Workaround:** Set MCUBoot's Kconfig option :kconfig:option:`CONFIG_NRF_RRAM_WRITE_BUFFER_SIZE` to ``32``.

.. rst-class:: v3-0-2 v3-0-1 v3-0-0 v2-9-0-nRF54H20-1 v2-9-2 v2-9-1 v2-9-0 v2-8-0 v2-7-0 v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0 v2-5-3 v2-5-2 v2-5-1 v2-5-0 v2-4-4 v2-4-3 v2-4-2 v2-4-1 v2-4-0 v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0

NCSDK-20567: Partitioning limitation with MCUboot swap move
  The swap algorithm in MCUboot (especially the default one - swap_move) requires some extra space to perform the swap move operation, so not entire partition space can be spent for the image.
  If the incoming image is too large, then the update will be impossible.
  If the installed image is too large, then undefined behavior might occur.

  **Workaround:** Make sure that the DFU update images are of a specific maximum size.
  Typically, for the nRF52, nRF53, and nRF91 devices, the size of the application must be less than ``mcuboot_primary_size - 80 - (mcuboot_primary_size/ 4096) * 12 -4096``.
  Some additional margin is suggested.

.. rst-class:: v3-1-1 v3-1-0 v3-0-2 v3-0-1 v3-0-0 v2-9-0-nRF54H20-1 v2-9-2 v2-9-1 v2-9-0 v2-8-0 v2-7-0 v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0 v2-5-3 v2-5-2 v2-5-1 v2-5-0 v2-4-4 v2-4-3 v2-4-2 v2-4-1 v2-4-0 v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0

NCSIDB-1194: MCUboot not properly disabling UARTE instances
  Increased power consumption might be observed (400 µA).

  **Workaround:** Disable all UART instances in MCUboot.
  If the booted application uses a UARTE instance, this issue might be covered during the application run-time.

.. rst-class:: v2-0-2 v2-0-1 v2-0-0

NCSDK-15494: Unable to build with RSA and ECIES-X25519 image encryptions
  Building MCUboot with either RSA or ECIES-X25519 image encryptions feature enabled is not possible.

  **Workaround:** To fix the issue, update the ``sdk-mcuboot`` repository by cherry-picking the upstream commits with the following hashes: ``7315e424b91503819307d33ebbc3140103470dd8`` and ``0f7db390d3537bff0feee20f900f9720f90f33f9``.

.. rst-class:: v1-2-1 v1-2-0

Recovery with the USB does not work
  The MCUboot recovery feature using the USB interface does not work.

.. rst-class:: v3-0-2 v3-0-1 v3-0-0 v2-9-1 v2-9-0 v2-8-0 v2-7-0

NCSDK-33173: MCUboot might fail on booting an application when LTO is enabled
  The MCUboot might fail on booting an application when it is built with Link Time Optimization enabled.
  This issue is transient and affects only some MCUboot builds for which it is 100% reproducible.

  **Affected platforms:** All ARM Cortex-M nRF CPUs

  **Workaround:** To fix the issue, update the ``sdk-mcuboot`` repository by cherry-picking the upstream commits with the following hash: ``766081bd6dfe26057fdbe3dca5d8eb5f64681beb``

.. rst-class:: v2-9-0 v2-8-0

NCSIDB-1431: Serial recovery responses to image list and slot info commands contain unexpected number of slot information entries
  The response contains too few slot information entries.

  **Affected platforms:** All

  **Workaround:** Update the ``sdk-mcuboot`` repository by cherry-picking the upstream commits with the following hash: ``cc7b97be4b5cc30a250a51dfeb51087e929d161``.


nrfxlib
*******

The issues in this section are related to components provided in :ref:`nrfxlib`.

Crypto
======

The issues in this section are related to :ref:`nrfxlib:crypto`.

.. rst-class:: v3-1-1 v3-1-0 v3-0-2 v3-0-1 v3-0-0 v2-9-0-nRF54H20-1 v2-9-2 v2-9-1 v2-9-0 v2-8-0 v2-7-0 v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0

NSCDK-26412: Updating to TF-M 2.0 using Mbed TLS 3.5.2 introduced a regression in resolving legacy crypto configurations from ``PSA_WANT_ALG_XXXX`` configurations
  Wi-Fi samples enabling OpenThread are affected by this bug as well as possible use cases with a dependency on some legacy features while using PSA crypto APIs.
  Unfortunately, the issue is not caught at build-time.
  It only appears at runtime.

  **Workaround:** Enable TF-M on TrustZone-enabled devices to circumvent this issue, or manually add missing legacy configurations in the :file:`psa_crypto-config.h.template` and :file:`psa_crypto_want_config.h.template` files.

.. rst-class:: v2-5-0 v2-4-4 v2-4-3 v2-4-2 v2-4-1 v2-4-0 v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0

NCSDK-25144: Enabling Kconfig option :kconfig:option:`CONFIG_SECURE_BOOT_CRYPTO` links ``nrf_cc310_bl`` into the main application
  Configuring secure boot with Kconfig option :kconfig:option:`CONFIG_SECURE_BOOT_CRYPTO` links the bootloader library ``nrf_cc310_bl`` into the main application, which is invalid.
  This makes crypto operations, such as ECDSA, fail.

  **Workaround:** Make sure the Kconfig option :kconfig:option:`CONFIG_SECURE_BOOT_CRYPTO` is only set when building the ``b0`` image.

.. rst-class:: v2-4-3 v2-4-2 v2-4-1 v2-4-0

NCSDK-22091: Selecting both :kconfig:option:`CONFIG_NORDIC_SECURITY_BACKEND` and :kconfig:option:`CONFIG_PSA_CORE` causes a build failure
  Selecting both :kconfig:option:`CONFIG_NORDIC_SECURITY_BACKEND` and :kconfig:option:`CONFIG_PSA_CORE` results in a build failure due to undefined references to different structs.

  **Workaround:** Manually define ``PSA_CORE_BUILTIN`` in the file :file:`nrf_security/configs/legacy_crypto_config.h.template`.

.. rst-class:: wontfix v3-1-1 v3-1-0 v3-0-2 v3-0-1 v3-0-0 v2-9-0-nRF54H20-1 v2-9-2 v2-9-1 v2-9-0 v2-8-0 v2-7-0 v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0 v2-5-3 v2-5-2 v2-5-1 v2-5-0 v2-4-4 v2-4-3 v2-4-2 v2-4-1 v2-4-0

NCSDK-22593: Selecting :kconfig:option:`CONFIG_PSA_WANT_ALG_CCM` without :kconfig:option:`CONFIG_MBEDTLS_AES_C` causes a build failure
  Selecting :kconfig:option:`CONFIG_PSA_WANT_ALG_CCM` without :kconfig:option:`CONFIG_MBEDTLS_AES_C` results in a build failure due to unsatisfied dependencies in :file:`check_config.h`.

  **Workaround:** Manually define ``MBEDTLS_AES_C`` in the file :file:`nrf_security/configs/nrf-config.h` or enable :kconfig:option:`CONFIG_PSA_WANT_ALG_GCM`.

.. rst-class:: v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0

NCSDK-21430: For ChaCha20/Poly1305 using nrf_cc3xx, incorrect tag will be produced if plaintext is empty
  For encryption, empty plaintext will result in incorrect tag.
  For decryption, correct tags will not be accepted.

.. rst-class:: v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0

NCSDK-20688: For AES GCM, calling :c:func:`psa_aead_update_ad` multiple times will result in an incorrect tag on CryptoCell
  It is only supported to feed the additional data for AES GCM once when using CryptoCell.

.. rst-class:: v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0

NCSDK-20287: Using AES GCM with other nonce sizes than 12 bytes will produce an incorrect tag on CryptoCell
  Both PSA Crypto and legacy Mbed TLS APIs are affected.

.. rst-class:: v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0

NCSDK-8075: Invalid initialization of ``mbedtls_entropy_context`` mutex type
  The calls to :cpp:func:`mbedtls_entropy_init` do not zero-initialize the member variable ``mutex`` when ``nrf_cc3xx`` is enabled.

  **Affected platforms:** nRF9160

  **Workaround:** Zero-initialize the structure type before using it or make it a static variable to ensure that it is zero-initialized.

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

.. rst-class:: v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0

NCSDK-11684: Failure loading KMU registers on nRF9160 devices
  Certain builds will experience problems loading HUK to KMU due to a bug in nrf_cc3xx_platform library prior to version 0.9.12.
  The problem arises in certain builds depending on alignment of code.
  The reason for the issue is improper handling of PAN 7 on nRF9160 devices.

  **Affected platforms:** nRF9160

  **Workaround:** Update to nrf_cc3xx_platform/nrf_cc3xx_mbedcrypto v0.9.12 or newer versions if KMU is needed.

.. rst-class:: v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0

NCSDK-7856: Faulty indirection on ``nrf_cc3xx`` memory slab when freeing the platform mutex
  The :cpp:func:`mutex_free_platform` function has a bug where a call to :cpp:func:`k_mem_slab_free` provides wrong indirection on a parameter to free the platform mutex.

  **Affected platforms:** nRF9160

  **Workaround:** Write the call to free the mutex in the following way: ``k_mem_slab_free(&mutex_slab, &mutex->mutex)``.
  The change adds ``&`` before the parameter ``mutex->mutex``.

.. rst-class:: v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0

NCSDK-7914: The ``nrf_cc3xx`` RSA implementation does not deduce missing parameters
  The calls to :cpp:func:`mbedtls_rsa_complete` will not deduce all types of missing RSA parameters when using ``nrf_cc3xx`` v0.9.6 or earlier.

  **Affected platforms:** nRF9160

  **Workaround:** Calculate the missing parameters outside of this function or update to ``nrf_cc3xx`` v0.9.7 or later.

.. rst-class:: v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0

NCSDK-5883: CMAC behavior issues
  CMAC glued with multiple backends might behave incorrectly due to memory allocation issues.

  **Workaround:** Disable glued CMAC and use only one of the enabled backends.

.. rst-class:: v1-3-1 v1-3-0

NCSDK-5546: Oberon missing symbols for HKDF
  nRF Oberon v3.0.5 is missing symbols for HKDF using SHA1, which will be fixed in an upcoming version of the library.

  **Workaround:** Use a different backend (for example, vanilla Mbed TLS) for HKDF/HMAC using SHA1.

.. rst-class:: v1-3-1 v1-3-0

Limited support for nRF Security subsystem (previously called Nordic Security Module)
  The :ref:`nrf_security` is currently only fully supported on nRF52840 and nRF9160 devices.
  It gives compile errors on nRF52832, nRF52833, nRF52820, nRF52811, and nRF52810.

  **Affected platforms:** nRF52832, nRF52833, nRF52820, nRF52811, nRF52810

  **Workaround:** To fix the errors, cherry-pick commits in `nrfxlib PR #205 <https://github.com/nrfconnect/sdk-nrfxlib/pull/205>`_.

.. rst-class:: v1-0-0 v0-4-0

Glue layer symbol renaming issue
  The :ref:`nrf_security` glue layer is broken because symbol renaming is not handled correctly.
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

Modem library
=============

The issues in this section are related to :ref:`nrfxlib:nrf_modem`.

.. rst-class:: v2-8-0

NCSDK-29993: The :c:func:`nrf_send` function with ``NRF_MSG_WAITACK`` flag will incorrectly set the ``errno`` to ``0xBAADBAAD`` if the socket is closed before the send operation finishes
  This will trigger an assert in :c:func:`nrf_modem_os_errno_set` if asserts are enabled.

  **Affected platforms:** nRF9161, nRF9151

  **Affected modem firmware versions:** v2.0.2

  **Workaround:** Disable asserts or remove the assert in :c:func:`nrf_modem_os_errno_set`.

.. rst-class:: v3-0-2 v3-0-1 v3-0-0 v2-9-0-nRF54H20-1 v2-9-2 v2-9-1 v2-9-0 v2-8-0 v2-7-0 v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0 v2-5-3 v2-5-2 v2-5-1 v2-5-0 v2-4-4 v2-4-3 v2-4-2 v2-4-1 v2-4-0 v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0

NCSDK-10106: Elevated current consumption when using applications without :ref:`nrfxlib:nrf_modem` on nRF9160
  When running applications that do not enable :ref:`nrfxlib:nrf_modem` on nRF9160 with build code B1A, current consumption will stay at 3 mA when in sleep.

  **Affected platforms:** nRF9160

  **Workaround:** Enable :ref:`nrfxlib:nrf_modem`.

.. rst-class:: v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0

NCSDK-14312: The :c:func:`nrf_recv` function crashes occasionally
  During execution, in rare cases, the :c:func:`nrf_recv` function crashes because of a race condition.

.. rst-class:: v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0

NCSDK-13360: The :c:func:`nrf_recv` function crashes if closed by another thread while receiving data
  When calling the :c:func:`nrf_recv` function, closing the socket from another thread while it is receiving data causes a crash.

.. rst-class:: v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0

NCSDK-6073: :c:func:`nrf_send` is blocking
  The :c:func:`nrf_send` function in the :ref:`nrfxlib:nrf_modem` might be blocking for several minutes, even if the socket is configured for non-blocking operation.
  The behavior depends on the cellular network connection.

  **Affected platforms:** nRF9160

  **Workaround:** For |NCS| v1.4.0, set the non-blocking mode for a partial workaround for non-blocking operation.

.. rst-class:: v2-4-1 v2-4-0 v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0

NRF91-1702: The modem might fail to attach to the network after a modem firmware update if the application core was not rebooted
  Performing a modem delta update without rebooting the application core (to reinitialize the modem and run the new firmware) might lead to a UICC initialization failure.
  The UICC failure can be confirmed by issuing the ``AT+CEREG?`` AT command to read the current network registration status after attempting a UICC activation.
  In case of the failure, the returned network registration status is ``90``.

  **Affected platforms:** nRF9160
  **Affect modem firmware versions:** v1.3.4 and v1.3.5

  **Workaround:** Reinitialize the :ref:`nrfxlib:nrf_modem` by calling :c:func:`nrf_modem_lib_shutdown` followed by :c:func:`nrf_modem_lib_init`,
  or reboot the application core as done in the existing |NCS| samples and applications.

Multiprotocol Service Layer (MPSL)
==================================

The issues in this section are related to :ref:`nrfxlib:mpsl`.

.. rst-class:: v3-0-2 v3-0-1 v3-0-0 v2-9-0-nRF54H20-1 v2-9-1 v2-9-0 v2-8-0

DRGN-25261: Radio events can stop running after a big temperature change triggers HFXO tuning
  MPSL needs to take XO tuning time into account.

  **Affected platforms:** nRF54L15, nRF54L10, nRF54L05

  **Workaround:** Increase the value of the :kconfig:option:`CONFIG_MPSL_HFCLK_LATENCY` Kconfig option by 512 microseconds to allow tuning to complete.

.. rst-class:: v3-0-2 v3-0-1 v3-0-0 v2-9-0-nRF54H20-1 v2-9-1 v2-9-0 v2-8-0

DRGN-25262: The default value of the :kconfig:option:`CONFIG_MPSL_HFCLK_LATENCY` Kconfig option is too small on nRF54L Series devices for some crystals
  This might prevent MPSL from running Bluetooth events.

  **Affected platforms:** nRF54L15, nRF54L10, nRF54L05

  **Workaround:** Increase the configured HFXO startup time to 1650 microseconds.

.. rst-class:: v3-1-1 v3-1-0 v3-0-2 v3-0-1 v3-0-0 v2-9-0-nRF54H20-1 v2-9-0

UARTE will have a frequency error beyond the specific limits
   In low temperatures and if :ref:`nrfxlib:mpsl` (MPSL) (Bluetooth LE, 802.15.4) is not used, UARTE will have a frequency error beyond the specific limits.

  **Affected platforms:** nRF54L15

  **Workaround:** Enable LFXO before using UART.

.. rst-class:: v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0 v2-5-3 v2-5-2 v2-5-1 v2-5-0 v2-4-4 v2-4-3 v2-4-2 v2-4-1 v2-4-0 v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0

DRGN-22314: MPSL can encounter fatal errors and crashes when used without SoftDevice Controller or the :ref:`nrfxlib:nrf_802154`
  This happens because of the of the allocation functions in the files :file:`nrfx_ppi.h` and :file:`nrfx_dppi.h` can allocate channels reserved by MPSL.

.. rst-class:: v3-1-1 v3-1-0 v3-0-2 v3-0-1 v3-0-0 v2-9-0-nRF54H20-1 v2-9-2 v2-9-1 v2-9-0 v2-8-0 v2-7-0 v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0 v2-5-3 v2-5-2 v2-5-1 v2-5-0 v2-4-4 v2-4-3 v2-4-2 v2-4-1 v2-4-0 v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0

DRGN-18247: Assertion with :c:enumerator:`MPSL_CLOCK_HF_LATENCY_BEST`
  When setting the ramp-up time of the high-frequency crystal oscillator with :c:enumerator:`MPSL_CLOCK_HF_LATENCY_BEST`, an assert in MPSL occurs.

  **Workaround:** Use :c:enumerator:`MPSL_CLOCK_HF_LATENCY_TYPICAL` instead of :c:enumerator:`MPSL_CLOCK_HF_LATENCY_BEST` when setting the time it takes for the HFCLK to ramp up.

.. rst-class:: v3-1-1 v3-1-0 v3-0-2 v3-0-1 v3-0-0 v2-9-0-nRF54H20-1 v2-9-2 v2-9-1 v2-9-0 v2-8-0 v2-7-0 v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0 v2-5-3 v2-5-2 v2-5-1 v2-5-0 v2-4-4 v2-4-3 v2-4-2 v2-4-1 v2-4-0 v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0

DRGN-15979: :kconfig:option:`CONFIG_CLOCK_CONTROL_NRF_K32SRC_RC_CALIBRATION` must be set when :kconfig:option:`CONFIG_CLOCK_CONTROL_NRF_K32SRC_RC` is set
  MPSL requires RC clock calibration to be enabled when the RC clock is used as the Low Frequency clock source.

.. rst-class:: v2-5-3 v2-5-2 v2-5-1 v2-5-0 v2-4-4 v2-4-3 v2-4-2 v2-4-1 v2-4-0 v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0

DRGN-14153: Radio Notification power performance penalty
  The Radio Notification feature has a power performance penalty proportional to the notification distance.
  This means an additional average current consumption of about 600 µA for the duration of the radio notification.

.. rst-class:: v3-1-1 v3-1-0 v3-0-2 v3-0-1 v3-0-0 v2-9-0-nRF54H20-1 v2-9-2 v2-9-1 v2-9-0 v2-8-0 v2-7-0 v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0 v2-5-3 v2-5-2 v2-5-1 v2-5-0 v2-4-4 v2-4-3 v2-4-2 v2-4-1 v2-4-0 v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0

KRKNWK-8842: MPSL does not support nRF21540 revision 1 or older
  The nRF21540 revision 1 or older is not supported by MPSL.
  This also applies to kits that contain this device.

  **Affected platforms:** nRF21540

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

  **Affected platforms:** nRF5340

.. rst-class:: v1-4-2 v1-4-1

DRGN-15176: :kconfig:option:`CONFIG_SYSTEM_CLOCK_NO_WAIT` is ignored when Low Frequency Clock is started before initializing MPSL
  If the application starts the Low Frequency Clock before calling the :c:func:`mpsl_init` function, the clock configuration option :kconfig:option:`CONFIG_SYSTEM_CLOCK_NO_WAIT` has no effect.
  MPSL will wait for the Low Frequency Clock to start.

  **Workaround:** When :kconfig:option:`CONFIG_SYSTEM_CLOCK_NO_WAIT` is set, do not start the Low Frequency Clock.

.. rst-class:: v1-4-0 v1-3-2 v1-3-1 v1-3-0

DRGN-15064: External Full swing and External Low swing not working
  Even though the MPSL Clock driver accepts a Low Frequency Clock source configuration for External Full swing and External Low swing, the clock control system is not configured correctly.
  For this reason, do not use :kconfig:option:`CLOCK_CONTROL_NRF_K32SRC_EXT_FULL_SWING` and :kconfig:option:`CLOCK_CONTROL_NRF_K32SRC_EXT_LOW_SWING`.

.. rst-class:: v1-5-0 v1-4-2 v1-4-1 v1-4-0

DRGN-11059: Front-end module API not implemented for SoftDevice Controller
  Front-end module API is currently not implemented for SoftDevice Controller.
  It is only available for 802.15.4.

802.15.4 Radio driver
=====================

The issues in this section are related to :ref:`nrfxlib:nrf_802154`.
In addition to the known issues listed here, see also :ref:`802.15.4 Radio driver limitations <nrf_802154_limitations>` for permanent limitations.

.. rst-class:: v3-1-1 v3-1-0 v3-0-2 v3-0-1 v3-0-0 v2-9-0-nRF54H20-1 v2-9-2 v2-9-1 v2-9-0 v2-8-0 v2-7-0

KRKNWK-19335: nRF 802.15.4 Radio Driver incorrectly reports an ``INVALID_FCS`` error if a frame is dropped while all receive buffers are full
  The nRF 802.15.4 Radio Driver reports an unjustified ``INVALID_FCS`` error in a specific scenario where at least one frame is dropped while the receive buffers limit is reached.
  This spurious error occurs even though the frame was not actually received with a bad FCS.

  **Affected platforms:** nRF54L Series

.. rst-class:: v2-9-0-nRF54H20-1 v2-9-2 v2-9-1 v2-9-0 v2-8-0

KRKNWK-19574: nRF 802.15.4 Radio Driver stuck in ``RADIO RXDISABLE`` state
  The nRF 802.15.4 Radio Driver might get stuck in the function :c:func:`wait_until_radio_disabled` while waiting for the radio peripheral transition from state ``RXDISABLE`` to ``DISABLED``.
  When used with MPSL, this issue might cause a crash caused by MPSL assertion failure.
  This phenomenon was observed very rarely during stress testing lasting several hours on the nRF54L15 DK v0.8.1.
  The root cause is not known.

  **Affected platforms:** nRF54L Series

.. rst-class:: v2-9-0-nRF54H20-1 v2-9-2 v2-9-1 v2-9-0 v2-8-0

KRKNWK-19689: nRF 802.15.4 Radio Driver transmits frames without ensuring ``CLOCK.EVENTS_XOTUNED``
  This issue affects only these transmissions that are started from sleep state and are not preceded by an automatic CCA operation.
  This includes the transmission of frames without CCA, continuous carriers, and modulated carriers.

  * For transmissions from a sleep state but preceded with automatic CCA operation, the XO tuning happens in parallel with CCA operation and does not affect transmission.
  * For transmissions of ACK frames generated automatically after receiving a frame, this issue does not occur.
  * For transmissions from states other than sleep, this issue does not occur.

  **Affected platforms:** nRF54L Series

  **Workaround:** Transmit frames preceded by CCA operation or switch to a receive state before transmission.

.. rst-class:: v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0 v2-5-3 v2-5-2 v2-5-1 v2-5-0 v2-4-4 v2-4-3 v2-4-2 v2-4-1 v2-4-0 v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0 v1-0-0

KRKNWK-18589: Timestamps for delayed operations triggering very shortly after a sleep request might be very inaccurate
  It was observed that the nRF 802.15.4 Radio Driver reported a too big timestamp by approximately ``UINT32_MAX``.

.. rst-class:: v2-5-3 v2-5-2 v2-5-1 v2-5-0 v2-4-4 v2-4-3 v2-4-2 v2-4-1 v2-4-0 v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0 v1-0-0

KRKNWK-18545: The device might enter a livelock state if AES encryption is done in a thread
  This can happen when the nRF 802.15.4 Radio Driver's multiprotocol feature is used together with Bluetooth LE (SoftDevice Controller).

  **Workaround:** When using the nRF 802.15.4 Radio Driver's multiprotocol feature together with SoftDevice Controller, do not use the :file:`sdc_soc.h` API, select the :kconfig:option:`CONFIG_BT_CTLR_CRYPTO` Kconfig option, or issue the ``HCI LE Encrypt`` command.

.. rst-class:: v2-4-0

KRKNWK-16976: multiprotocol_rpmsg application on nRF5340 network core occasionally crashes when Matter weather station application is stress tested
  The root cause of this issue is not known.
  When nRF5340 network core crashes it can hang or silently reset (see :kconfig:option:`CONFIG_RESET_ON_FATAL_ERROR`).
  The |NCS| does not provide a feature to allow the nRF5340 application core to detect a reset or failure of the nRF5340 network core and react properly (possibly resetting whole nRF5340 SoC).

  **Affected platforms:** nRF5340

  **Workaround:** Implement own mechanism to allow detection on nRF5340 application core a reset or crash of nRF5340 network core and react according to own requirements.

.. rst-class:: v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0

KRKNWK-14950: MPSL asserts during operation with heavy load
  An operation under heavy load can end with a crash, with the MPSL message ``ASSERT: 108, 659``.
  This issue was observed very rarely during stress tests on the devices from the nRF53 and nRF52 Series.

  **Affected platforms:** nRF5340, Thingy:53, nRF52832, nRF52833, nRF52840

.. rst-class:: v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0

KRKNWK-14898: CSMA-CA backoff parameters might not be randomized in a uniform way
  The backoff parameters of the CSMA-CA operation are generated by the logic which might not return the uniformly distributed random numbers.

.. rst-class:: v1-7-1 v1-7-0

KRKNWK-11384: Assertion with Bluetooth LE and multiprotocol usage
  The device might assert on rare occasions during the use of Bluetooth LE and 802.15.4 multiprotocol.

.. rst-class:: v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0

KRKNWK-6756: 802.15.4 Service Layer (SL) library support for the nRF53
  The binary variant of the 802.15.4 Service Layer (SL) library for the nRF53 does not support such features as synchronization of **TIMER** with **RTC** or timestamping of received frames.
  For this reason, 802.15.4 features like delayed transmission or delayed reception are not available for the nRF53.

  **Affected platforms:** nRF5340

.. rst-class:: v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0

KRKNWK-8133: CSMA-CA issues
  Using CSMA-CA with the open-source variant of the 802.15.4 Service Layer (SL) library causes an assertion fault.
  CSMA-CA support is currently not available in the open-source SL library.

.. rst-class:: v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-2 v1-4-0

KRKNWK-6255: RSSI parameter adjustment is not applied
  The RADIO: RSSI parameter adjustment errata (153 for nRF52840, 225 for nRF52833 and nRF52820, 87 for nRF5340) are not applied for RSSI, LQI, Energy Detection, and CCA values used by the 802.15.4 protocol.
  There is an expected offset up to +/- 6 dB in extreme temperatures of values based on RSSI measurement.

  **Affected platforms:** nRF5340, nRF52840, nRF52833, nRF52820

  **Workaround:** To apply RSSI parameter adjustments, cherry-pick the commits in `hal_nordic PR #88 <https://github.com/zephyrproject-rtos/hal_nordic/pull/88>`_, `sdk-nrfxlib PR #381 <https://github.com/nrfconnect/sdk-nrfxlib/pull/381>`_, and `sdk-zephyr PR #430 <https://github.com/nrfconnect/sdk-zephyr/pull/430>`_.

.. rst-class:: v3-1-1 v3-1-0 v3-0-2 v3-0-1 v3-0-0 v2-9-2 v2-9-1 v2-9-0 v2-8-0 v2-7-0 v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0 v2-5-3 v2-5-2 v2-5-1 v2-5-0 v2-4-4 v2-4-3 v2-4-2 v2-4-1 v2-4-0 v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0

KRKNWK-19974: CCA ED threshold is not optimal
  The default configuration value of :kconfig:option:`CONFIG_NRF_802154_CCA_ED_THRESHOLD` was not optimal and should be adjusted to correspond to -75 dBm.

  **Affected platforms:** nRF5340, nRF52840, nRF52833, nRF52820, nRF54L15

  **Workaround:** Depending on the SoC, the value should be set to the following:

    * nRF5340 and nRF52833 - Set :kconfig:option:`CONFIG_NRF_802154_CCA_ED_THRESHOLD` to ``18``
    * Other devices -  Set :kconfig:option:`CONFIG_NRF_802154_CCA_ED_THRESHOLD` to ``17``

SoftDevice Controller
=====================

The issues in this section are related to :ref:`nrfxlib:softdevice_controller`.
In addition to the known issues listed here, see also :ref:`softdevice_controller_limitations` for permanent limitations.

.. rst-class:: v3-1-0 v3-0-2 v3-0-1 v3-0-0 v2-9-0-nRF54H20-1 v2-9-2 v2-9-1 v2-9-0 v2-8-0

DRGN-26138: Using the radio in timeslots may lead to undefined behavior in the controller

  The controller may show reduced performance when EVENT registers are not cleared in timeslots or are set before the controller is enabled.

  **Affected platforms:** nRF54L and nRF54H Series

  **Workaround:** Clear the following radio events after using the radio in a timeslot:

  * ``EVENTS_READY``
  * ``EVENTS_END``
  * ``EVENTS_DISABLED``
  * ``EVENTS_ADDRESS``
  * ``EVENTS_PAYLOAD``

.. rst-class:: v3-0-2 v3-0-1 v3-0-0 v2-9-0

DRGN-25859: The controller may stop raising advertising reports to the host while scanning for extended advertising packets
  In busy environments, after some time the host application will stop getting scan callbacks from the scanner.

  **Workaround:** Disable the :kconfig:option:`CONFIG_BT_EXT_ADV` Kconfig option or call the :c:func:`hci_vs_sdc_scan_accept_ext_adv_packets_set` function with ``accept_ext_adv_packets`` set to ``false``.

.. rst-class:: v3-0-2 v3-0-1 v3-0-0 v2-9-0

DRGN-24930: An assert could happen when receiving on the Coded PHY with 7.5 ms ACL connection interval
  The controller could assert when receiving a packet over 27 bytes with a CRC error on the S8 Coded PHY.

  **Workaround:** Set the ACL connection interval to a value that is at least 10 ms or more.

.. rst-class:: v2-8-0

DRGN-23776: Sending CIS packets with invalid MIC
  When the CIS central running on an nRF5340 device is sending encrypted ISO packets, the MIC might be invalid.

.. rst-class:: v2-7-0 v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0 v2-5-3 v2-5-2 v2-5-1 v2-5-0 v2-4-4 v2-4-3 v2-4-2 v2-4-1 v2-4-0 v2-3-0

DRGN-22443: A rare assert when disabling a periodic advertising set with responses
  In some rare cases, the controller can assert when disabling a periodic advertising set with responses.

.. rst-class:: v2-7-0 v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0 v2-5-3 v2-5-2 v2-5-1 v2-5-0

DRGN-22441: The length byte of the HCI packet could be incorrect
  This could happen when the packet contains an LE BIG Sync Established event or LE BIG Complete event with status not equal to success.

.. rst-class:: v2-7-0 v2-6-2 v2-6-1 v2-6-0

DRGN-22652: Assert when receiving on the S8 Coded PHY
  This could happen in a connection where link peer device is transmitting on S8 Coded PHY.

  **Workaround:** Disable :kconfig:option:`CONFIG_BT_CTLR_DATA_LENGTH` and :kconfig:option:`CONFIG_BT_DATA_LEN_UPDATE` Kconfig options.

.. rst-class:: v2-7-0 v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0 v2-5-3 v2-5-2 v2-5-1 v2-5-0 v2-4-4 v2-4-3 v2-4-2 v2-4-1 v2-4-0 v2-3-0 v2-2-0 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0

DRGN-22686: Missing truncated advertising report
  The extended scanner would not generate a truncated advertising report after the coexistence interface aborted the reception of an AUX_CHAIN_IND packet.

.. rst-class:: v2-7-0 v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0 v2-5-3 v2-5-2 v2-5-1 v2-5-0 v2-4-4 v2-4-3 v2-4-2 v2-4-1 v2-4-0 v2-3-0 v2-2-0 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0

DRGN-22678: A very rare issue where the controller stopped generating advertising reports
  On nRF52 and nRF53 Series devices, this would happen at least one hour after the scanner started.
  On nRF54L and nRF54H Series devices, this would occur immediately after the scanner started.
  It would only happen when one of the following applies:

  * There was another central-like scheduling activity running.
    Examples of roles with such activities are the ACL central, periodic advertiser, isochronous broadcaster, and the CIS central.
    This activity was configured with an event length or event spacing equal or greater than the scan interval.
    This is typically only true for use cases where the application enables isochronous channels or uses very short scan windows.
  * The scanner was configured with scan window equal to scan interval (continuous scanning).
  * The central-like scheduling activity required less than one ms to complete at the point in time where the scanner started.

.. rst-class:: v2-7-0 v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0 v2-5-3 v2-5-2 v2-5-1 v2-5-0 v2-4-4 v2-4-3 v2-4-2 v2-4-1 v2-4-0

DRGN-22230: A rare issue where the scanner would be stuck in the synchronizing state after failing to receive an AUX_ADV_IND packet
  This could only happen when the corresponding ADV_EXT_IND packet contains a resolvable address, private address resolution is enabled, and the periodic advertising list is not used.

.. rst-class:: v2-7-0 v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0 v2-5-3 v2-5-2 v2-5-1 v2-5-0 v2-4-4 v2-4-3 v2-4-2 v2-4-1 v2-4-0 v2-3-0

DRGN-22705: The controller could generate the LE Advertising Set Terminated event one event sooner than expected
  This could only happen when all of the following apply:

  * A non-zero ``Max_Extended_Advertising_Events`` parameter was used in the LE Set Extended Advertising Enable command.
  * Other ongoing activities in the controller prevented the first advertising event from taking place when the advertising set was created.

.. rst-class:: v2-7-0

DRGN-22930: The SoftDevice Controller might de-reference a NULL pointer
  This can occur when using the vendor-specific HCI command ``Zephyr Write TX Power Level`` for a connection.

.. rst-class:: v2-7-0 v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0 v2-5-3 v2-5-2 v2-5-1 v2-5-0

DRGN-22879: The Central could fail to receive the last packet in an isochronous event
  This could only happen if the Connected Isochronous Stream Creation procedure was initiated by the host before the Encryption Start procedure completed.

.. rst-class:: v2-7-0 v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0 v2-5-3 v2-5-2 v2-5-1 v2-5-0 v2-4-4 v2-4-3 v2-4-2 v2-4-1 v2-4-0

DRGN-23002: An assert could happen when using the coexistence interface
  This could happen when any of the following controller activities were ongoing:

  * Isochronous Broadcaster
  * Connected Isochronous channel in the peripheral role
  * Periodic Sync with Responses

.. rst-class:: v2-7-0

DRGN-23291: LE Power Control was not being used for CIS
  This could happen when the CIS was not the first CIS in the CIG.

.. rst-class:: v2-9-0-nRF54H20-1 v2-9-1 v2-9-0 v2-8-0 v2-7-0 v2-6-3 v2-6-2 v2-6-1 v2-6-0

DRGN-24784: Disconnect could happen if multiple peripheral links were active and encrypted
  This could happen when the LE Long Term Key Request Reply command overwrites the session key of an active link.

.. rst-class:: v2-7-0 v2-6-2 v2-6-1 v2-6-0

DRGN-23204: The SoftDevice Controller in the peripheral role could terminate a connection due to a MIC failure during a valid encryption start procedure
  This could only happen if the LL_ENC_RSP packet was corrupted due to on-air interference.

.. rst-class:: v2-7-0 v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0

DRGN-23586: The received unframed Isochronous SDUs were not reported to be an SDU interval apart
  This could happen when the ISO interval is greater than the SDU interval and multiple SDUs can be received in a single ISO interval.

.. rst-class:: v2-7-0 v2-6-2 v2-6-1 v2-6-0 v2-5-3 v2-5-2 v2-5-1 v2-5-0 v2-4-4 v2-4-3 v2-4-2 v2-4-1 v2-4-0 v2-3-0 v2-2-0 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0

DRGN-23693: Wrong sleep clock accuracy
  The sleep clock accuracy communicated to the peer is too inaccurate if MPSL is initialized with a low frequency clock accuracy better than 20 ppm.

.. rst-class:: v2-7-0 v2-6-2 v2-6-1 v2-6-0 v2-5-3 v2-5-2 v2-5-1 v2-5-0 v2-4-4 v2-4-3 v2-4-2 v2-4-1 v2-4-0 v2-3-0 v2-2-0

DRGN-22036: A rare issue in the controller that could lead to a bus fault
  This could only happen when all of the following conditions are met:

  * The host was too slow at pulling HCI events.
  * One or more HCI events were masked in the controller.
  * The controller was raising ACL or ISO data to the host.

.. rst-class:: v2-7-0 v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0

DRGN-22633: The VS Set Connection Event Trigger command does not always trigger the provided task
  This happens on platforms with DPPI, if the PPI channel ID provided in the command is even-numbered.

  **Workaround:** On platforms with DPPI, provide a PPI channel ID that is odd-numbered.

.. rst-class:: v2-6-2 v2-6-1 v2-6-0

DRGN-22024: The controller might assert when the peripheral receives a connection update indication
  This only occurs when the central uses a wide receive window for the connection update, and both sends at the end of the receive window and sends a lot of data in the connection event with the connection update instant.

.. rst-class:: v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0

DRGN-21619: The controller might assert if the CIS peripheral stops receiving packets from the CIS central
  This only occurs when the window widening reaches at least half of the ISO interval in magnitude.
  Assuming worst case clock accuracies on both the central and the peripheral, this could occur with a supervision timeout of 2.4, 3.7, or 4.9 seconds, corresponding to an ISO interval of 5, 7.5, or 10 milliseconds, respectively.

  **Workaround:** Set the supervision timeout to a value lower than those mentioned above.

.. rst-class:: v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0

DRGN-21605: Value read by HCI ISO Read TX Timestamp is off by 40 µs
  The HCI command ``ISO Read TX Timestamp`` returns the last assigned sync reference for the ISO TX path and the value might be off by 40 µs.

.. rst-class:: v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0 v2-5-3 v2-5-2 v2-5-1 v2-5-0 v2-4-4 v2-4-3 v2-4-2 v2-4-1 v2-4-0 v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0

DRGN-22163: Assert when initiating a connection for a long time
  An assert can happen if the initiator runs for more than 2147 seconds before connecting.

.. rst-class:: v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0 v2-5-3 v2-5-2 v2-5-1 v2-5-0 v2-4-4 v2-4-3 v2-4-2 v2-4-1 v2-4-0 v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0

DRGN-21603: A rare race condition when encrypting a block
  An extremely rare race condition where using :c:func:`sdc_soc_ecb_block_encrypt` from an ISR can lead to encryption failures.

.. rst-class:: v2-6-2 v2-6-1 v2-6-0 v2-5-3 v2-5-2 v2-5-1 v2-5-0 v2-4-4 v2-4-3 v2-4-2 v2-4-1 v2-4-0 v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0

DRGN-21637: The peripheral waited for a link to time out when tearing down the connection
  This happens when the central acknowledges ``TERMINATE_IND`` in the same event as it was being sent.

.. rst-class:: v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0 v2-5-3 v2-5-2 v2-5-1 v2-5-0 v2-4-4 v2-4-3 v2-4-2 v2-4-1 v2-4-0 v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0

DRGN-19050: The controller accepts the HCI LE Set Random Address command while passive scanning is enabled
  The controller must return the error code ``0x0D`` in this case.

.. rst-class:: v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0 v2-5-3 v2-5-2 v2-5-1 v2-5-0

DRGN-21912: A BIS broadcaster transmits invalid parameters in the BIG Info
  This happens if a BIG was created (using the LL Create BIG or LL Create BIG Test commands) with ``num_bis`` set to ``1`` and ``packing`` set to ``1`` (interleaved).

.. rst-class:: v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0 v2-5-3 v2-5-2 v2-5-1 v2-5-0 v2-4-4 v2-4-3 v2-4-2 v2-4-1 v2-4-0 v2-3-0 v2-2-0

DRGN-21923: The controller-initiated autonomous LE Power Control Request procedure for Coded PHY could lead to a disconnection
  This happens only when an autonomous power control request was enabled and was about to be sent.

.. rst-class:: v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0 v2-5-3 v2-5-2 v2-5-1 v2-5-0

DRGN-21949: Assert on BIS sync
  The controller can assert if a BIS receiver stops receiving packets from the BIS broadcaster.

.. rst-class:: v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0 v2-5-3 v2-5-2 v2-5-1 v2-5-0 v2-4-4 v2-4-3 v2-4-2 v2-4-1 v2-4-0 v2-3-0

DRGN-21839: Rare failures when setting periodic advertising subevent data
  In some rare cases, the controller can generate an LE Periodic Advertising Subevent Data Request for a subevent for which it does not have the memory capacity.

.. rst-class:: v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0 v2-5-3 v2-5-2 v2-5-1 v2-5-0 v2-4-4 v2-4-3 v2-4-2 v2-4-1 v2-4-0 v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0

DRGN-21962: Assert when using SPI FEM with Coded PHY on nRF53 Series devices
  The controller can assert when scanning or advertising on Coded PHY using SPI FEM on nRF53 Series devices.

  **Affected platforms:** nRF5340, Thingy:53

.. rst-class:: v3-1-1 v3-1-0 v3-0-2 v3-0-1 v3-0-0 v2-9-0-nRF54H20-1 v2-9-2 v2-9-1 v2-9-0 v2-8-0 v2-7-0 v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0

DRGN-21293: The LE Read ISO TX Sync command is implemented according to the raised errata ES-23138
  In ES-23138, the return parameter ``TX_Time_Stamp`` is used as the SDU synchronization reference of the SDU previously scheduled for transmission.
  This differs from the Bluetooth Core Specification v5.4, which uses the CIG or BIG reference anchor point as the SDU synchronization reference.
  When the CIG or BIG is configured with an ``ISO_Interval`` that equals the ``SDU_Interval``, there is no difference between the CIG or BIG reference anchor point and the SDU synchronization reference.
  If several SDUs are transmitted during each ``ISO_Interval``, meaning that it is larger than the ``SDU_Interval``, our implementation of the LE Read ISO TX Sync command returns a unique SDU synchronization reference for each SDU.

.. rst-class:: v2-7-0 v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0 v2-5-3 v2-5-2 v2-5-1 v2-5-0 v2-4-4 v2-4-3 v2-4-2 v2-4-1 v2-4-0 v2-3-0 v2-2-0 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0

DRGN-20444: The HCI LE Create Connection command and the HCI LE Extended Create Connection command overwrite scan parameters
  This happens when the HCI LE Create Connection command or the HCI LE Extended Create Connection command is called after the scan parameters are set.

  **Workaround:** Set the scan parameters with the HCI LE Set Scan Parameters command or the HCI LE Set Extended Scan Parameters command again.

.. rst-class:: v2-5-3 v2-5-2 v2-5-1 v2-5-0 v2-4-4 v2-4-3 v2-4-2 v2-4-1 v2-4-0 v2-3-0

DRGN-20762: LE Set Periodic Advertising Subevent Data could fail unexpectedly if interrupted
  The LE Set Periodic Advertising Subevent Data command could fail when providing data at the same time as an ``AUX_SYNC_SUBEVENT_IND`` was sent.

.. rst-class:: v2-5-3 v2-5-2 v2-5-1 v2-5-0 v2-4-4 v2-4-3 v2-4-2 v2-4-1 v2-4-0 v2-3-0 v2-2-0 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0

DRGN-20815: A packet might not be received when sent at the instant of a Channel Map Update
  This could happen when the controller is acting as slave, while the master is sending a Channel Map Update.

.. rst-class:: v2-5-3 v2-5-2 v2-5-1 v2-5-0 v2-4-4 v2-4-3 v2-4-2 v2-4-1 v2-4-0 v2-3-0

DRGN-20432: LE Set Periodic Advertising Response Data command can assert if host behaves incorrectly
  This could happen if the LE Set Periodic Advertising Response Data command was issued more than once without fetching the Command Complete Event.

.. rst-class:: v2-5-3 v2-5-2 v2-5-1 v2-5-0 v2-4-4 v2-4-3 v2-4-2 v2-4-1 v2-4-0 v2-3-0 v2-2-0 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0

DRGN-20832: The controller would assert during cooperative active scanning or when running a cooperative initiator
  This could happen when the controller was about to send a scan request or connect indication.

.. rst-class:: v2-5-3 v2-5-2 v2-5-1 v2-5-0 v2-4-4 v2-4-3 v2-4-2 v2-4-1 v2-4-0 v2-3-0 v2-2-0 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0

DRGN-20862: The nRF5340 DK consumed too much current while scanning
  This could happen if the controller was running with TX power higher than 0 dB.

.. rst-class:: v2-5-1 v2-5-0 v2-4-2 v2-4-1 v2-4-0 v2-3-0 v2-2-0 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0

DRGN-21085: The controller would stop sending ACL data packets to the host when controller to host flow control was enabled
  This could happen when a disconnection occurred before the host had issued the Host Number of Complete Packets command for the remaining ACL data packets.

.. rst-class:: v2-5-1 v2-5-0 v2-4-2 v2-4-1 v2-4-0 v2-3-0 v2-2-0 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0

DRGN-21253: Rare assert on the scanner
  The scanner might assert when it schedules the reception of the next advertising packet.

.. rst-class:: v2-5-1 v2-5-0 v2-4-2 v2-4-1 v2-4-0

DRGN-21020: The continuous extended scanner sometimes stops generating advertising reports
  This can happen when the controller is running an extended cooperative scanner together with other activities, such as advertising or connection, while receiving data in an extended advertising event that uses ``AUX_CHAIN_IND``.

.. rst-class:: v2-5-3 v2-5-2 v2-5-1 v2-5-0 v2-4-4 v2-4-3 v2-4-2 v2-4-1 v2-4-0

DRGN-20956: Rare assert when terminating the Periodic Sync with Responses
  In rare cases, when a Periodic Sync with Responses is being terminated while it is waiting for a sync to a Periodic Advertiser with Responses, the controller can assert.

.. rst-class:: v2-4-2 v2-4-1 v2-4-0 v2-3-0 v2-2-0 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0

DRGN-19460: The continuous extended scanner cannot receive the ``AUX_ADV_IND`` packet
  This can happen if the time between the ``ADV_EXT_IND`` and ``AUX_ADV_IND`` is more than 840 µs.

.. rst-class:: v2-4-3 v2-4-2 v2-4-1 v2-4-0

DRGN-19580: The stack would dereference a ``NULL`` pointer
  This can happen if a resolvable :c:enum:`own_address_type` is used in the HCI Le Extended Create Connection V2 command while the resolving list is empty.

.. rst-class:: v2-4-3 v2-4-2 v2-4-1 v2-4-0

DRGN-19623: The HCI Reset would not reset the channel map
  The HCI Reset command would not clear the channel map set by the host using the HCI Le Set Host Channel Classification command.

.. rst-class:: v2-4-3 v2-4-2 v2-4-1 v2-4-0 v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0

DRGN-18411: The ``Peer_Address_Type`` parameter in the LE Connection Complete event was incorrectly set to ``2`` or ``3``
  This can happen when a connection is established to a device whose address is resolved.
  The least significant bit of the ``Peer_Address_Type`` parameter was set correctly.

.. rst-class:: v2-4-3 v2-4-2 v2-4-1 v2-4-0

DRGN-20118: The stack would assert when creating advertisers
  This can happen if you try to set up more advertisers than the available advertising sets.

.. rst-class:: v2-4-3 v2-4-2 v2-4-1 v2-4-0

DRGN-20085: The stack would assert when enabling an advertising set
  Enabling an extended advertising set would assert in cases where a host-provided address was not needed and the address was not set up for the advertising set.

.. rst-class:: v2-4-3 v2-4-2 v2-4-1 v2-4-0 v2-3-0 v2-2-0 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0

DRGN-20578: The stack would assert when receiving a non-compliant ``LL_PHY_RSP``
  The controller acting as the central would assert when receiving a non-compliant ``LL_PHY_RSP`` from a peer device.

.. rst-class:: v2-4-3 v2-4-2 v2-4-1 v2-4-0 v2-3-0 v2-2-0 v2-1-3 v2-1-2 v2-1-1 v2-1-0

DRGN-20654: The controller returns an invalid error for the Host Number of Completed Packets command
  This can happen if the Host Number of Complete Packets command was sent with a connection handle for which the controller had already raised a disconnect event.
  The controller would return ``BT_HCI_ERR_INVALID_PARAM`` to the command, which would mean that the host could not return the buffer to the controller.

.. rst-class:: v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0

DRGN-17562: One of the LE Transmit Power Reporting Events might not be reported to the host
  When multiple LE Transmit Power Reporting Events are generated at the same time for the same PHY, one of these events will be missed.
  This will occur only when there are simultaneous remote and local power level changes on the same PHY.

.. rst-class:: v3-1-1 v3-1-0 v3-0-2 v3-0-1 v3-0-0 v2-9-0-nRF54H20-1 v2-9-2 v2-9-1 v2-9-0 v2-8-0 v2-7-0 v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0 v2-5-3 v2-5-2 v2-5-1 v2-5-0 v2-4-4 v2-4-3 v2-4-2 v2-4-1 v2-4-0 v2-3-0 v2-2-0

DRGN-19039: Multirole advertiser not seen by peer in some cases
  This can happen when the controller attempts to reschedule the advertising events due to scheduling conflicts with the scanner or initiator and both of the following apply:

  * One device is either a multirole advertiser and scanner or a multirole advertiser and initiator, and has a scan window shorter than the scan interval, and an advertising interval shorter than the scan window.
  * The peer is a scanner or initiator with the same scan window.

  **Workaround:** Set the scan interval equal to either the scan window or the advertising interval.

.. rst-class:: v2-3-0

DRGN-18424: Rare assert when stopping the Periodic Advertiser role
  In rare cases, when a Periodic Advertiser instance is being stopped while another Periodic Advertiser instance is still running, the controller can assert.

.. rst-class:: v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0

DRGN-18424: Rare assert when stopping the Periodic Sync role
  In rare cases, when a Periodic Sync instance is being stopped while another Periodic Sync instance is still running, the controller can assert.

.. rst-class:: v2-3-0

DRGN-18651: Scanner might fail to deliver advertising reports
  There is a chance of failure if an advertising report is received just before the scanner times out or is disabled.
  This can lead to the scanner not delivering any further advertising reports when re-enabled.

.. rst-class:: v2-3-0 v2-2-0 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0

DRGN-18714: Assert when advertising or initiating after disconnection
  The controller could assert when starting a connectable advertiser or creating a connection too quickly after disconnection.

.. rst-class:: v2-3-0 v2-2-0

DRGN-18775: Periodic Advertisement Sync Transfer receiver fails to synchronize with large periodic advertising intervals
  The Periodic Advertisement Sync Transfer (PAST) sender might generate an incorrect SyncInfo field for periodic advertising intervals greater than five seconds.

.. rst-class:: v2-3-0 v2-2-0

DRGN-18833: Assert when the periodic sync is in progress
  The Periodic Advertisement Sync Transfer (PAST) sender could assert if the associated periodic sync was not fully established.

.. rst-class:: v2-3-0 v2-2-0

DRGN-18971: Low TX power on the nRF21540 DK in connected state
  TX power value goes to the default one on the nRF21540 DK in connected state when using MPSL FEM and manually configuring the radio power.

  **Affected platforms:** nRF21540

.. rst-class:: v2-3-0 v2-2-0

DRGN-19003: Fail to synchronize using the Periodic Advertising Sync Transfer procedure
  The controller cannot synchronize to a periodic advertising train using the Periodic Advertising Sync Transfer procedure if it has previously tried to do it while it was already synchronized to the periodic advertising train.

.. rst-class:: v2-3-0 v2-2-0 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0

DRGN-18840: Disconnect with transaction_collision when receiving LL_PHY_UPDATE_IND with "no change"
  The peripheral would disconnect with DIFFERENT_TRANSACTION_COLLISION when a collision of a connection update and a PHY update occurs even if the central asks for no change.

.. rst-class:: v2-3-0 v2-2-0 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0

DRGN-19058: Do not accept the advertising handle specified by the host
  The Controller would not accept an advertising handle provided in HCI commands with values above the configured number of advertising sets.

.. rst-class:: v2-3-0 v2-2-0 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0

DRGN-19197: The Advertiser fails to advertise when payload increases
  The advertiser fails in the first event when the payload is increased by a small amount.

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

.. rst-class:: v3-1-1 v3-1-0 v3-0-2 v3-0-1 v3-0-0 v2-9-0-nRF54H20-1 v2-9-2 v2-9-1 v2-9-0 v2-8-0 v2-7-0 v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0 v2-5-3 v2-5-2 v2-5-1 v2-5-0 v2-4-4 v2-4-3 v2-4-2 v2-4-1 v2-4-0 v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0

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

DRGN-17454: Wrong data length is selected if the even length is greater than 65535 µs
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
  The periodic advertiser sends its ``AUX_SYNC_IND`` packet 40 µs later than the one indicated in the ``SyncInfo`` of the ``AUX_ADV_IND`` packet.

.. rst-class:: v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0

DRGN-17710: Scanner packet reception delay
  The scanner attempts to receive the first ``AUX_SYNC_IND`` packet 40 µs later than the one indicated in the ``SyncInfo`` of the ``AUX_ADV_IND``.
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

  **Affected platforms:** nRF5340, Thingy:53

  **Workaround:** Do not use the RC oscillator as the Low Frequency clock source.

.. rst-class:: v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0

DRGN-16650: Undefined behavior when extended scanning is enabled
  When extended scanning is enabled and :kconfig:option:`CONFIG_BT_BUF_ACL_RX_SIZE` is set to a value less than 251, it might result in asserts or undefined behavior.

  **Workaround:** Set :kconfig:option:`CONFIG_BT_BUF_EVT_RX_SIZE` to 255 when extended scanning is enabled.

.. rst-class:: v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0 v1-0-0

DRGN-16394: The host callback provided to :c:func:`sdc_enable` is always called after every advertising event
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

  **Affected platforms:** nRF5340, Thingy:53, nRF52840, nRF52833, nRF52832

  **Workaround:** On nRF52 Series devices, do not use coex and fem. On nRF53 Series devices, do not use CODED PHY.

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
  This is default configuration of the Bluetooth host.

  **Affected platforms:** nRF5340, Thingy:53

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

  **Affected platforms:** nRF5340, Thingy:53

.. rst-class:: v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0 v1-0-0

DRGN-15586: "HCI LE Set Scan Parameters" accepts a scan window greater than the scan interval
  This can result in undefined behavior.
  It should fail with the result ``Invalid HCI Command Parameters (0x12)``.

  **Workaround:** The application should make sure the scan window is set to less than or equal to the scan interval.

.. rst-class:: v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0

DRGN-15547: Assertion when updating PHY and the event length is configured too low
  The SoftDevice Controller might assert in the following cases:

  * :c:union:`sdc_cfg_t` with :c:member:`event_length` is set to less than 2500 µs and the PHY is updated from 2M to 1M, or from either 1M or 2M to Coded PHY.
  * :c:union:`sdc_cfg_t` with :c:member:`event_length` is set to less than 7500 µs and a PHY update to Coded PHY is performed.

  The default value of :kconfig:option:`CONFIG_SDC_MAX_CONN_EVENT_LEN_DEFAULT` is 7500 us.
  The minimum event length supported by :kconfig:option:`CONFIG_SDC_MAX_CONN_EVENT_LEN_DEFAULT` is 2500 us.

  **Workaround:**

  * Set :c:union:`sdc_cfg_t` with :c:member:`event_length` to at least 2500 µs if the application is using 1M PHY.
  * Set :c:union:`sdc_cfg_t` with :c:member:`event_length` to at least 7500 µs if the application is using Coded PHY.

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

DRGN-15758: The controller might still have pending events after :c:func:`sdc_hci_evt_get` returns false
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
  The command ``HCI Read RSSI`` always returns ``Command Disallowed (0x0C)``.

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
  As a result, the function :c:func:`bt_enable` returns an error code.

  **Workaround:** Manually enable :kconfig:option:`CONFIG_SOFTDEVICE_CONTROLLER_MULTIROLE` for the project configuration.

.. rst-class:: v1-5-0

DRGN-15382: The SoftDevice Controller cannot be qualified on nRF52832
  The SoftDevice Controller cannot be qualified on nRF52832.

  **Affected platforms:** nRF52832

  **Workaround:** Upgrade to v1.5.1 or use the main branch.

.. rst-class:: v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0 v1-0-0

DRGN-14008: The advertising data is cleared every time the advertising set is configured
  This causes the ``HCI LE Set Extended Advertising Parameters`` command to accept data which cannot be fit within the advertising interval instead of returning ``Packet Too Long (0x45)``.
  This would only occur if the advertising set is configured to use maximum data length on LE Coded PHY with an advertising interval less than 30 milliseconds.

.. rst-class:: v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0 v1-0-0

DRGN-15291: The generation of QoS Connection events is not disabled after an HCI reset
  Some event reports might still occur after a reset.

.. rst-class:: v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0

DRGN-15226: Link disconnects with reason ``LMP Response Timeout (0x22)``
  If the slave receives an encryption request while the ``HCI LE Long Term Key Request`` event is disabled, the link disconnects with the reason ``LMP Response Timeout (0x22)``.
  The event is disabled when :kconfig:option:`CONFIG_BT_SMP` and/or :kconfig:option:`CONFIG_BT_CTLR_LE_ENC` is disabled.

.. rst-class:: v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0

DRGN-11963: LL control procedures cannot be initiated at the same time
  The LL control procedures (LE start encryption and LE connection parameter update) cannot be initiated at the same time or more than once.
  The controller will return an HCI error code ``Controller Busy (0x3a)`` as per specification's chapter 2.55.

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

  **Affected platforms:** nRF5340

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
  :c:func:`hci_data_get` might return "No data available" when there is data available.
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

The issues in this section are related to drivers provided in nrfx.

MDK
===

.. rst-class:: v1-2-1 v1-2-0

Incorrect pin definition for nRF5340
  For nRF5340, the pins **P1.12** to **P1.15** are unavailable due to an incorrect pin number definition in the MDK.

  **Affected platforms:** nRF5340

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

Integrations
************

The issues in this section are related to :ref:`integrations`.

Pelion
======

The issues in this section are related to Pelion Device Management integration.
The support for Pelion Device Management and the related library and application were removed in :ref:`nRF Connect SDK v1.9.0 <ncs_release_notes_190>`.

.. rst-class:: v1-6-1 v1-6-0

NCSDK-10196: DFU fails for some configurations with the quick session resume feature enabled
  Enabling :kconfig:option:`CONFIG_PELION_QUICK_SESSION_RESUME` together with the OpenThread network backend leads to the quick session resume failure during the DFU packet exchange.
  This is valid for the :ref:`nRF52840 DK <ug_nrf52>` and the :ref:`nRF5340 DK <ug_nrf5340>`.

  **Affected platforms:** nRF5340, nRF52840

  **Workaround:** Use the quick session resume feature only for configurations with the cellular network backend.

.. _known_issue_tfm:

Trusted Firmware-M (TF-M)
*************************

The issues in this section are related to the TF-M implementation in the |NCS|.

.. rst-class:: v3-1-1 v3-1-0 v3-0-2 v3-0-1 v3-0-0 v2-9-0-nRF54H20-1 v2-9-2 v2-9-1 v2-9-0 v2-8-0 v2-7-0 v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0 v2-5-3 v2-5-2 v2-5-1 v2-5-0 v2-4-4 v2-4-3 v2-4-2 v2-4-1 v2-4-0 v2-3-0 v2-2-0 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0

NCSDK-29095: Writing assets to NVM using TF-M causes increased interrupt latencies
  Writes to NVM provoke increased latencies even for the highest-priority interrupts.
  When TF-M is used (``*/ns`` board targets), writes to NVM made by the secure image can disturb the normal flow of operations happening on the non-secure image.

  This includes the SoftDevice Controller on the nRF54L15 DK.
  Its asserts might be triggered when the interrupt latency is too high, which will result in fatal errors.
  The interrupt latency increases are much higher on the nRF53 and nRF91 Series devices (~80ms) compared to the nRF54L15 (~100us).
  Writes to NVM happen on the secure image when writing assets to the Internal Trusted Storage (ITS) and the Protected Storage (PS).

  **Affected platforms:** nRF54L15, nRF5340, nRF91 Series

  **Workaround:** Write persistent assets using the PSA APIs only when there are no ongoing time-critical operations.

.. rst-class:: v3-1-1 v3-1-0 v3-0-2 v3-0-1 v3-0-0 v2-9-0-nRF54H20-1 v2-9-2 v2-9-1 v2-9-0 v2-8-0 v2-7-0 v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0 v2-5-3 v2-5-2 v2-5-1 v2-5-0

CIA-1182: TF-M flash partition overflow
  When building for Thingy:91 and enabling debug optimizations (or enabling Debug build in the VS code extension), the TF-M flash partition will overflow.

  **Affected platforms:** Thingy:91

  **Workaround:** Set the :kconfig:option:`CONFIG_TFM_CMAKE_BUILD_TYPE_MINSIZEREL` to ``y``.

.. rst-class:: v2-5-0

NCSDK-24986: TF-M does not configure PDM and I2S as non-secure peripherals on nRF91 Series devices
  The peripherals cannot be accessed by the non-secure application without triggering a security violation.

.. rst-class:: v2-5-0

NCSDK-24804: TF-M does not compile without the ``gpio0`` node enabled in devicetree
  This fails with the error message ``'TFM_PERIPHERAL_GPIO0_PIN_MASK_SECURE' undeclared.``

  **Workaround:** Enable the ``gpio0`` node in the devicetree configuration

  .. code-block::

        &gpio0 {
          status = "okay";
        };

.. rst-class:: v2-4-3 v2-4-2 v2-4-1 v2-4-0 v2-3-0

NCSDK-22907: TF-M does not build with :kconfig:option:`CONFIG_TFM_ITS_ENCRYPTED` enabled and :kconfig:option:`CONFIG_TFM_PARTITION_PROTECTED_STORAGE` disabled
  This fails with the error message ``Invalid config: NOT PS_ROLLBACK_PROTECTION and PS_ENCRYPTION and PSA_ALG_GCM or PSA_ALG_CCM! and TFM_SP_PS is not defined``.

  **Workaround:** Enable :kconfig:option:`CONFIG_TFM_PARTITION_PROTECTED_STORAGE` if encrypted ITS is used.

.. rst-class:: v2-4-0

NCSDK-22818: TF-M does not build with :kconfig:option:`CONFIG_TFM_PROFILE_TYPE_MINIMAL` disabled and :kconfig:option:`CONFIG_TFM_LOG_LEVEL_SILENCE` enabled
  This fails with the error message ``undefined reference to 'end'`` caused by printf being included in the build.

.. rst-class:: v2-4-3 v2-4-2 v2-4-1 v2-4-0 v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0

NRFJPROG-454: TF-M might fail to reset when using nrfjprog version 10.22.x on nRF9160 platforms
  Issuing a reset command using nrfjprog for nRF9160 platforms using the following command might fail to successfully complete and might cause a TF-M core panic:

  .. code-block::

     nrfjprog -f nrf91 --reset

  **Affected platforms:** nRF9160

  **Workaround:** Use an nrfjprog version that is not affected or do not use the --reset option, for example use --pinreset or --debugreset instead:

  .. code-block::

     nrfjprog -f nrf91 --pinreset

  .. code-block::

     nrfjprog -f nrf91 --debugreset

.. rst-class:: v3-1-1 v3-1-0 v3-0-2 v3-0-1 v3-0-0 v2-9-0-nRF54H20-1 v2-9-2 v2-9-1 v2-9-0 v2-8-0 v2-7-0 v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0 v2-5-3 v2-5-2 v2-5-1 v2-5-0 v2-4-4 v2-4-3 v2-4-2 v2-4-1 v2-4-0 v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0

NCSDK-18321: TF-M PSA architecture tests do not build with CMake v3.25.x
  The :ref:`tfm_psa_test` fails to build with CMake version 3.25.x with missing header files.
  This happens because the CMake install command is executed before the build command with the affected CMake versions.

  **Workaround:** Do not use the CMake version 3.25.x.

.. rst-class:: v2-3-0

NCSDK-20864: TF-M unaligned partitions when MCUboot padding and debug optimizations are enabled
  When building TF-M using the :ref:`partition_manager` with the MCUboot bootloader enabled (:kconfig:option:`CONFIG_BOOTLOADER_MCUBOOT`), with either :kconfig:option:`CONFIG_DEBUG_OPTIMIZATIONS` or :kconfig:option:`CONFIG_TFM_CMAKE_BUILD_TYPE_DEBUG` also enabled, the resulting partitions are not aligned with :kconfig:option:`CONFIG_NRF_SPU_FLASH_REGION_SIZE` as required by the :ref:`ug_tfm_partition_alignment_requirements`.
  This will cause the build to fail.

  **Workaround:** Disable the :kconfig:option:`CONFIG_DEBUG_OPTIMIZATIONS` and :kconfig:option:`CONFIG_TFM_CMAKE_BUILD_TYPE_DEBUG` options, or subtract 0x200 from the current value of the :kconfig:option:`CONFIG_PM_PARTITION_SIZE_TFM` option, to comply with the :ref:`ug_tfm_partition_alignment_requirements`.

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

.. rst-class:: v2-2-0

NCSDK-19536: TF-M does not compile when the board is missing a ``uart1`` node and TF-M logging is enabled
  TF-M does not compile when a ``uart1`` node is not defined in a board's devicetree configuration and TF-M logging is enabled.

  **Workaround:** Use one of the following workarounds:

  * Add uart1 node with baudrate property in the devicetree configuration.
  * Disable TF-M logging by enabling the :kconfig:option:`CONFIG_TFM_LOG_LEVEL_SILENCE` option.

.. rst-class:: v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0

NCSDK-15909: TF-M failing to build with Zephyr SDK 0.14.2
  TF-M might fail to build due to flash overflow with Zephyr SDK 0.14.2 when :kconfig:option:`CONFIG_TFM_PROFILE_TYPE_NOT_SET` is set to ``y``.

  **Workaround:** Use one of the following workarounds:

  * Increase the TF-M partition :kconfig:option:`CONFIG_PM_PARTITION_SIZE_TFM`.
  * Use Zephyr SDK version 0.14.1.

.. rst-class:: v2-4-3 v2-4-2 v2-4-1 v2-4-0 v2-3-0 v2-2-0 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0

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

  **Affected platforms:** Thingy:91

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

.. rst-class:: v3-1-1 v3-1-0 v3-0-2 v3-0-1 v3-0-0 v2-9-0-nRF54H20-1 v2-9-2 v2-9-1 v2-9-0 v2-8-0 v2-7-0 v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0 v2-5-3 v2-5-2 v2-5-1 v2-5-0 v2-4-4 v2-4-3 v2-4-2 v2-4-1 v2-4-0 v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0

NCSDK-11195: Build errors when enabling :kconfig:option:`CONFIG_BUILD_WITH_TFM` option
  Enabling the :kconfig:option:`CONFIG_BUILD_WITH_TFM` Kconfig option in SES project configuration or using ``west -t menuconfig`` results in build errors.

  **Workaround:** Set ``CONFIG_BUILD_WITH_TFM=y`` in project configuration file (:file:`prj.conf`) or through west command line (``west build -- -DCONFIG_BUILD_WITH_TFM=y``).

.. rst-class:: v1-9-2 v1-9-1 v1-9-0

NCSDK-12306: Enabling debug configuration causes usage fault on nRF9160
  When the debug configuration :kconfig:option:`CONFIG_TFM_CMAKE_BUILD_TYPE_DEBUG` is enabled, a usage fault is triggered during boot on nRF9160.

  **Affected platforms:** nRF9160

.. rst-class:: v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0

NCSDK-14590: Usage fault in interrupt handlers when using FPU extensions
  When the :kconfig:option:`CONFIG_ARM_NONSECURE_PREEMPTIBLE_SECURE_CALLS` Kconfig option is disabled, a usage fault can be triggered when an interrupt handler uses FPU extensions while interrupting the secure processing environment.

  **Workaround:** Do not disable the :kconfig:option:`CONFIG_ARM_NONSECURE_PREEMPTIBLE_SECURE_CALLS` option when the :kconfig:option:`FPU` option is enabled.

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

  **Affected platforms:** nRF5340

  **Workaround:** Place the ``rpmsg_nrf53_sram`` partition inside the ``sram_nonsecure`` partition using :ref:`partition_manager`.

.. rst-class:: v1-9-0

NCSDK-13949: TF-M Secure Image copies FICR to RAM on nRF9160
  TF-M Secure Image copies the FICR to RAM address between ``0x2003E000`` and ``0x2003F000`` during boot on nRF9160.

  **Affected platforms:** nRF9160

Zephyr
******

The issues in this section are related to the Zephyr downstream in the |NCS|.

.. rst-class:: v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0

NCSDK-20104: MCUboot configuration can prevent application from being able to run
  MCUboot will, by default, create a read-only RAM region on the MPU that is used for the stack guard feature (enabled by default).
  The intention is that the application that gets booted clears this region.
  If the user application's startup variables reside in this memory location, the application will stop with a fault and be unable to start.
  This issue replaces the issue NCSDK-18426, which mentioned a fault in the firmware when using RTT on nRF52 Series devices.

  **Affected platforms:** nRF52840, nRF52833, nRF52830, nRF52820

  **Workaround:** Enable :kconfig:option:`CONFIG_MCUBOOT_CLEANUP_ARM_CORE` in MCUboot configuration.

.. rst-class:: v2-5-3 v2-5-2 v2-5-1 v2-5-0

LwM2M engine blocks all composite operations
  Due to a bug in LwM2M engine, all composite operations get a return code of ``4.01 - Unauthorized``.
  This has been reported in `Zephyr issue #6412 <https://github.com/zephyrproject-rtos/zephyr/issues/64012>`_.

  **Workaround:** To fix the error, cherry-pick commits from the upstream `Zephyr PR #64016 <https://github.com/zephyrproject-rtos/zephyr/pull/64016>`_.

.. rst-class:: v2-5-3 v2-5-2 v2-5-1 v2-5-0 v2-4-4 v2-4-3 v2-4-2 v2-4-1 v2-4-0 v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0

LwM2M object's resource instance buffers might overlap
  If the LwM2M object statically allocates storage for more than one resource instance of the string type, these buffers will overlap.
  This has been reported in `Zephyr issue #6411 <https://github.com/zephyrproject-rtos/zephyr/issues/64011>`_.

  **Workaround:** To fix the error, cherry-pick commits from the upstream `Zephyr PR #64015 <https://github.com/zephyrproject-rtos/zephyr/pull/64015>`_.

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

The time returned by ``clock_gettime()`` API becomes incorrect after one week of uptime
  The time returned by POSIX ``clock_gettime()`` API becomes incorrect after one week elapses.
  This is due to an overflow in the uptime conversion.

.. rst-class:: v1-4-2 v1-4-1 v1-4-0

NCSDK-6330: USB Mass Storage Sample Application fails MSC Tests from USB3CV test tool
  :zephyr:code-sample:`usb-mass` fails the USB3CV compliance Command Set Test from the MSC Tests suite.

.. rst-class:: v1-4-2 v1-4-1 v1-4-0

NCSDK-6328: USB CDC ACM Composite Sample Application fails Chapter 9 Tests from USB3CV test tool
  USB CDC ACM Composite sample application fails the USB3CV compliance TD 9.1: Device Descriptor Test from the Chapter 9 Test suite.

.. rst-class:: v1-4-2 v1-4-1 v1-4-0

NCSDK-6331: WebUSB sample application fails Chapter 9 Tests from USB3CV test tool
  :zephyr:code-sample:`webusb` fails the USB3CV compliance TD 9.21: LPM L1 Suspend Resume Test from the Chapter 9 Test suite.

.. rst-class:: v1-3-2 v1-3-1 v1-3-0

FOTA does not work
  FOTA with the :zephyr:code-sample:`smp-svr` does not work.

  **Affected platforms:** nRF5340

.. rst-class:: v1-3-1 v1-3-0

NCSIDB-108: Thread context switch might lead to a kernel fault
  If the Zephyr kernel preempts the current thread and performs a context switch to a new thread while the current thread is executing a secure service, the behavior is undefined and might lead to a kernel fault.
  To prevent this situation, a thread that aims to call a secure service must temporarily lock the kernel scheduler (:cpp:func:`k_sched_lock`) and unlock the scheduler (:cpp:func:`k_sched_unlock`) after returning from the secure call.

.. rst-class:: v1-0-0

Counter Alarm sample does not work
  The :zephyr:code-sample:`alarm` does not work.
  A fix can be found in `Pull Request #16736 <https://github.com/zephyrproject-rtos/zephyr/pull/16736>`_.

.. rst-class:: v1-3-0 v1-2-1 v1-2-0 v1-1-0 v1-0-0

USB Mass Storage Sample Application compilation issues
  :zephyr:code-sample:`usb-mass` does not compile.

.. rst-class:: v1-7-1 v1-7-0 v1-4-2 v1-4-1 v1-4-0

NCSDK-6832: SMP Server sample fails upon initialization
  The :zephyr:code-sample:`smp-svr` will fail upon initialization when using the :file:`bt-overlay.conf` Kconfig overlay file.
  This happens because of a stack overflow.

  **Workaround:** Set :kconfig:option:`CONFIG_MAIN_STACK_SIZE` to ``2048``.

.. _known_issues_other:

Other issues
************

.. rst-class:: v3-0-2 v3-0-1 v3-0-0

The RRAM size has decreased from 1024 KB to 1012 KB for the nRF54L10 SoC.
  You must update the memory maps accordingly.

  **Affected platforms:** nRF54L10

.. rst-class:: v2-9-2 v2-9-1 v2-9-0

NCSDK-33153: nRF54L15 rev2 tools update
  An nRF54L15 SoC with revision 2 needs an update to the nRF Util ``device`` command.
  Update the command to v2.8.8 or later.

  **Affected platforms:** nRF54L

.. rst-class:: v3-1-1 v3-1-0 v3-0-2 v3-0-1 v3-0-0 v2-9-0-nRF54H20-1

HM-25973: SysCtrl does not always wake up when an interrupt is triggered by sending an IPC message through the local domains
  A problem related to communication between local domains and sysctrl core.
  The IPC from LD uses ``VEVIF TASKS_TRIGGERED[xx]`` to trigger ``systrl`` when a message is waiting for it in shared memory.
  Sometimes, the "trigger" does not wake up the VPR120.

  **Affected platforms:** nRF54H20

  **Workaround:** Each IPC TX interrupt is automatically retriggered once after a 12 µs delay.

.. rst-class:: v3-1-1 v3-1-0 v3-0-2 v3-0-1 v3-0-0 v2-9-0-nRF54H20-1

KRKNWK-31013: Issues with ``nrfutil trace`` and missing STM stream information on the nRF54H20 SoC
  While running ``nrfutil trace`` without resetting any domain, the tool can synchronize against Coresight packets and the underlying STPv2 protocol.
  However, the stream decoder receives packets that lack clear indicators for the start or end of the associated packet.
  This issue results in incomplete packet decoding and potential data loss.

  **Affected platforms:** nRF54H20

.. rst-class:: v3-1-1 v3-1-0 v3-0-2 v3-0-1 v3-0-0 v2-9-0-nRF54H20-1

KRKNWK-31038: Instruction trace reliability issues during ETM debugging on the nRF54H20 SoC
  During ETM debugging, disconnecting and reconnecting to the device often causes the instruction trace to fail to appear reliably.
  Multiple resets are required to restore functionality.

  **Affected platforms:** nRF54H20

 .. rst-class:: v3-1-1 v3-1-0 v3-0-2 v3-0-1 v3-0-0 v2-9-0-nRF54H20-1 v2-9-2 v2-9-1 v2-9-0 v2-8-0

NCSDK-28152: TPIU Trace Signal Skew on the nRF54H20 SoC
  Segger TPIU tracing on the nRF54H20 DK encounters instability due to a minimal skew between clock and data signals.
  This skew causes unreliable trace performance, resulting in intermittent data capture.

  **Workaround:** Adjusting the TPIU trace delay on SEGGER's J-Trace Pro can improve signal stability, enhancing ETM capture reliability.
  For more information on adjusting trace timing, refer to the SEGGER Wiki's `Adjusting Trace Timings and General Troubleshooting`_ section.

.. rst-class:: v3-1-1 v3-1-0 v3-0-2 v3-0-1 v3-0-0 v2-9-0-nRF54H20-1 v2-9-2 v2-9-1 v2-9-0 v2-8-0

NCSDK-30095: Cannot flash a device if the ``nrfutil device x-adac-discovery`` command is called before flashing
  It leaves the device into ``RomOperation`` mode.

  **Workaround:** The issue is fixed in ``nrfutil-device`` version 2.7.5.
  Update the tool version.

.. rst-class:: v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0 v1-5-2 v1-5-1 v1-5-0 v1-4-2 v1-4-1 v1-4-0 v1-3-2 v1-3-1 v1-3-0 v1-2-1 v1-2-0 v1-1-0 v1-0-0

Receive error with large packets
  nRF9160 fails to receive large packets (over 4000 bytes).

  **Affected platforms:** nRF9160

.. rst-class:: v1-2-0 v1-1-0 v1-0-0

Calling ``nrf_connect()`` immediately causes fail
  ``nrf_connect()`` fails if called immediately after initialization of the device.
  A delay of 1000 ms is required for this to work as intended.

  **Affected platforms:** nRF9160

Known issues for custom boards
******************************

.. rst-class:: v2-8-0

VSC-2817: Custom boards for the nRF54L15 rev. Engineering B include Kconfig options for the rev. Engineering A
  If you generated a custom board based on the nRF54L15 rev. Engineering B using the |NCS| v2.8.0, the custom board files mistakenly include the following Kconfig select options for the Engineering A version of the SoC:

  * ``SOC_NRF54L15_ENGA_CPUAPP``
  * ``SOC_NRF54L15_ENGA_CPUFLPR``

  **Affected platforms:** nRF54L15 (rev. Engineering B)

  **Workaround:** In the Kconfig file for your custom board (named :file:`Kconfig.<custom_board_name>`, where ``<custom_board_name>`` is the name of your board), manually replace the select options under the ``CONFIG_BOARD_CUSTOM_BOARD_NAME`` Kconfig option:

  * ``SOC_NRF54L15_ENGA_CPUAPP`` with ``SOC_NRF54L15_CPUAPP``
  * ``SOC_NRF54L15_ENGA_CPUFLPR`` with ``SOC_NRF54L15_CPUFLPR``

  Then, :ref:`manually regenerate the board <zephyr:custom_board_definition>`.

.. rst-class:: v2-9-2 v2-9-1 v2-9-0 v2-8-0

 VSC-2817: Custom boards for the nRF54L15 rev. Engineering B adds the :kconfig:option:`CONFIG_SOC_NRF_FORCE_CONSTLAT` setting
  If you generated a custom board based on the nRF54L15 rev. Engineering B using the |NCS| v2.9.0 or v2.8.0, the custom board files add the :kconfig:option:`CONFIG_SOC_NRF_FORCE_CONSTLAT` Kconfig option set to ``y``.

  **Affected platforms:** nRF54L15 (rev. Engineering B)

  **Workaround:** In the Kconfig file for your custom board (named :file:`Kconfig.<custom_board_name>`, where ``<custom_board_name>`` is the name of your board), manually remove the :kconfig:option:`CONFIG_SOC_NRF_FORCE_CONSTLAT` Kconfig option.
  Then, :ref:`manually regenerate the board <zephyr:custom_board_definition>`.

Known issues for deprecated components
**************************************

This section lists known issues for components that have been deprecated during the |NCS| development.
These issues are visible for older releases.

HomeKit
=======

.. note::
   HomeKit is deprecated with the |NCS| v2.5.0 release and removed in the |NCS| v.2.6.0 release.
   All HomeKit customers are recommended to use Matter for new designs of smart home products.

.. rst-class:: v2-4-3 v2-4-2 v2-4-1 v2-4-0 v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0

KRKNWK-17343: Accessories become significantly slower when some data pairs in the non-volatile storage (NVS) change frequently
  Accessing data pairs that rarely change can take a long time.
  It might happen, for example, in case of long booting or paired/unpaired identify response time, which can lead to certification issues.
  During the certification, the accessory is subjected to multiple resets and pair/unpair processes, which causes the NVS to store a large amount of new data pairs.
  As a result, the accessory does not pass the certification test cases exceeding the maximum operation time (for example, TCT012, TCT022 and TCT023).

  **Workaround:** Enable the NVS cache by setting the Kconfig options :kconfig:option:`CONFIG_NVS_LOOKUP_CACHE` to ``y`` and :kconfig:option:`CONFIG_NVS_LOOKUP_CACHE_SIZE` to ``512`` (requires additional 2 KB of RAM).
  Additional optimization can be enabled by setting the Kconfig options :kconfig:option:`CONFIG_SETTINGS_NVS_NAME_CACHE` to ``y`` and :kconfig:option:`CONFIG_SETTINGS_NVS_NAME_CACHE_SIZE` to ``512`` (requires additional 2 KB of RAM).
  Alternatively, you can manually cherry-pick and apply commit with fix from ``main`` (commit hash: ``216d6588d069390d2c5291560002ca47684fbfc0``).

.. rst-class:: v2-3-0 v2-2-0

KRKNWK-16503: OTA DFU using the iOS Home app (over UARP) does not work on the nRF5340 SoC
  Application core cannot be upgraded due to a problem with uploading images for two cores.
  Uploading the network core image overrides an already uploaded application core image.

  **Affected platforms:** nRF5340

  **Workaround:** Manually cherry-pick and apply commit from the main branch (commit hash: ``09874a36edf21ced7d3c9356de07df6f0ff3d457``).

.. rst-class:: wontfix v3-1-1 v3-1-0 v3-0-2 v3-0-1 v3-0-0 v2-9-0-nRF54H20-1 v2-9-2 v2-9-1 v2-9-0 v2-8-0 v2-7-0 v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0 v2-5-3 v2-5-2 v2-5-1 v2-5-0 v2-4-4 v2-4-3 v2-4-2 v2-4-1 v2-4-0 v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0

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

.. rst-class:: v2-4-1 v2-4-0 v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0

NCSDK-13947: Net core downgrade prevention does not work on nRF5340
  When updating firmware via SMP protocol (Nordic DFU), the downgrade prevention mechanism does not work for the network core.

  **Affected platforms:** nRF5340

  **Workaround:** Prevention mechanism can be implemented in the mobile application layer.

.. rst-class:: v2-0-2 v2-0-1 v2-0-0

KRKNWK-13607: Stateless switch application crashes upon factory reset
  When running Thread test suit on the stateless switch application, the CI crashes upon factory reset.

.. rst-class:: wontfix v3-1-1 v3-1-0 v3-0-2 v3-0-1 v3-0-0 v2-9-0-nRF54H20-1 v2-9-2 v2-9-1 v2-9-0 v2-8-0 v2-7-0 v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0 v2-5-3 v2-5-2 v2-5-1 v2-5-0 v2-4-4 v2-4-3 v2-4-2 v2-4-1 v2-4-0 v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0

KRKNWK-13249: Unexpected assertion in HAP Bluetooth Peripheral Manager
  When Bluetooth LE layer emits callback with a connect or disconnect event, one of its parameters is an underlying Bluetooth LE connection object.
  On rare occasions, this connection object is no longer valid by the time it is processed in HomeKit, and this results in assertion.
  There is no proven workaround yet.

.. rst-class:: wontfix v3-1-1 v3-1-0 v3-0-2 v3-0-1 v3-0-0 v2-9-0-nRF54H20-1 v2-9-2 v2-9-1 v2-9-0 v2-8-0 v2-7-0 v2-6-4 v2-6-3 v2-6-2 v2-6-1 v2-6-0 v2-5-3 v2-5-2 v2-5-1 v2-5-0 v2-4-4 v2-4-3 v2-4-2 v2-4-1 v2-4-0 v2-3-0 v2-2-0 v2-1-4 v2-1-3 v2-1-2 v2-1-1 v2-1-0 v2-0-2 v2-0-1 v2-0-0 v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0 v1-6-1 v1-6-0

KRKNWK-11729: Stateless switch event characteristic value not handled according to specification in Bluetooth LE mode
  The stateless programmable switch application does not handle the value of the stateless switch event characteristic in the Bluetooth LE mode according to the specification.
  According to the specification, the accessory is expected to return null once the characteristic has been read or after 10 seconds have passed.
  In its current implementation in the |NCS|, the characteristic value does not change to null immediately after it is read, and changes to null after 5 seconds instead.

  **Workaround:** The HomeKit specification in point 11.47 is going to be updated.

.. rst-class:: v1-9-2 v1-9-1 v1-9-0

KRKNWK-13063: RTT logs do not work with the Light Bulb multiprotocol sample with DFU on nRF52840
  The Light Bulb multiprotocol sample with Nordic DFU activated in debug version for nRF52840 platform does not display RTT logs properly.

  **Affected platforms:** nRF52840

  **Workaround:** Disable RTT logs for the bootloader.

.. rst-class:: v1-9-2 v1-9-1 v1-9-0

KRKNWK-13064: Nordic DFU is not compliant with HAP certification requirements
  Some of the HAP certification requirements are not met by the Nordic DFU solution.

  **Workaround:** Cherry-pick changes from PR #332 in the sdk-homekit repository.

.. rst-class:: v1-9-2 v1-9-1 v1-9-0 v1-8-0 v1-7-1 v1-7-0

KRKNWK-12474: Multiprotocol samples on nRF52840 might not switch to Thread
  Samples might not switch to Thread mode as expected and remain in Bluetooth mode instead.
  The issue is related to older iOS versions.

  **Affected platforms:** nRF52840

  **Workaround:** Update your iPhone to iOS 15.4.

.. rst-class:: v1-9-2 v1-9-1 v1-9-0 v1-8-0

KRKNWK-13095: Change in KVS key naming scheme causes an error for updated devices
  A previous implementation allowed for empty key in domain.
  This has been restricted during refactoring.

  **Workaround:** Cherry-pick changes from PR #329 in the sdk-homekit repository.

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

SEGGER Embedded Studio Nordic Edition
=====================================

.. note::
    SEGGER Embedded Studio Nordic Edition support has been deprecated with the |NCS| v2.0.0 release and |VSC| is now the default IDE for the |NCS|.
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

Secure Partition Manager (SPM)
==============================

.. note::
    The Secure Partition Manager (SPM) is deprecated as of |NCS| v2.1.0 and removed after |NCS| v2.2.0. It is replaced by :ref:`Trusted Firmware-M (TF-M) <ug_tfm>`.

.. rst-class:: v2-2-0

NCSDK-19156: Building SPM for other boards than ``nrf5340dk_nrf5340_cpuapp`` and ``nrf9160dk_nrf9160`` fails with compilation error in :file:`cortex_m_systick.c`
  This happens because the :kconfig:option:`CONFIG_CORTEX_M_SYSTICK` configuration option is enabled while the systick node is disabled in the devicetree.

  **Affected platforms:** nRF9160, nRF5340

  **Workaround:** Enable the systick node in a DTS overlay file for the SPM build by completing the following steps:

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
   All samples using the default SPM config fails to build for the ``thingy91_nrf9160_ns``  board target if the sample is not set up with MCUboot.

  **Affected platforms:** Thingy:91

  **Workaround:** Use the main branch.

Zephyr repository issues
************************

In addition to these known issues, check the current issues in the `official Zephyr repository`_, since these might apply to the |NCS| fork of the Zephyr repository as well.

Reporting |NCS| issues
**********************

To get help and report issues that are not related to Zephyr but to the |NCS|, go to Nordic's `DevZone`_.
