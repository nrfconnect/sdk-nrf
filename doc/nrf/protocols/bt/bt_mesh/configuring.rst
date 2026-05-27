.. _ug_bt_mesh_configuring:

Configuring Bluetooth Mesh in |NCS|
###################################

.. contents::
   :local:
   :depth: 2

The Bluetooth® Mesh support is controlled by :kconfig:option:`CONFIG_BT_MESH`, which depends on the following configuration options:

* :kconfig:option:`CONFIG_BT` - Enables the Bluetooth subsystem.
* :kconfig:option:`CONFIG_BT_OBSERVER` - Enables the Bluetooth Observer role.
* :kconfig:option:`CONFIG_BT_PERIPHERAL` - Enables the Bluetooth Peripheral role.

When the Bluetooth LE Controller is located on a separate image (like on the :zephyr:board:`nrf5340dk` and :zephyr:board:`thingy53` boards), the following configuration must be applied to the Bluetooth LE Controller configuration:

* :kconfig:option:`CONFIG_BT_EXT_ADV` =y.
* :kconfig:option:`CONFIG_BT_EXT_ADV_MAX_ADV_SET` =5.

Optional features configuration
*******************************

Optional features in the Bluetooth Mesh stack must be explicitly enabled:

* :kconfig:option:`CONFIG_BT_MESH_RELAY` - Enables message relaying.
* :kconfig:option:`CONFIG_BT_MESH_FRIEND` - Enables the Friend role.
* :kconfig:option:`CONFIG_BT_MESH_LOW_POWER` - Enables the Low Power role.
* :kconfig:option:`CONFIG_BT_MESH_PROVISIONER` - Enables the Provisioner role.
* :kconfig:option:`CONFIG_BT_MESH_GATT_PROXY` - Enables the GATT Proxy Server role.
* :kconfig:option:`CONFIG_BT_MESH_PB_GATT` - Enables the GATT provisioning bearer.
* :kconfig:option:`CONFIG_BT_MESH_CDB` - Enables the Configuration Database subsystem.

The persistent storage of the Bluetooth Mesh provisioning and configuration data is enabled by :kconfig:option:`CONFIG_BT_SETTINGS`.
See the :ref:`zephyr:bluetooth-persistent-storage` section of :ref:`zephyr:bluetooth-arch` for details.

Mesh models
===========

The |NCS| Bluetooth Mesh model implementations are optional features, and each model has individual Kconfig options that must be explicitly enabled.
See :ref:`bt_mesh_models` for details.

Mesh settings/performance
=========================

The following configuration options are used to configure the behavior and performance of a Bluetooth Mesh network.
For more information about configuration options affecting the memory footprint of Bluetooth Mesh, see :ref:`memory footprint optimization guide for Bluetooth Mesh <app_memory_bt_mesh>`.

* :kconfig:option:`CONFIG_BT_MESH_PROXY_USE_DEVICE_NAME` - Includes the GAP device name in a scan response when the GATT Proxy feature is enabled.
* :kconfig:option:`CONFIG_BT_MESH_DK_PROV` - Enables the Bluetooth Mesh provisioning handler for the nRF5x development kits.
* :kconfig:option:`CONFIG_BT_MESH_ADV_BUF_COUNT` - Defines the number of advertising buffers for local messages.
  Increase to improve the performance, at the cost of increased RAM usage.
* :kconfig:option:`CONFIG_BT_MESH_ADV_EXT_GATT_SEPARATE` - Enables the use of a separate extended advertising set for GATT Server Advertising.
* :kconfig:option:`CONFIG_BT_MESH_ADV_EXT_FRIEND_SEPARATE` - Enables the use of a separate extended advertising set for Friend advertising.
* :kconfig:option:`CONFIG_BT_MESH_RELAY_ADV_SETS` - Defines a maximum number of simultaneous relay messages.
* :kconfig:option:`CONFIG_BT_MESH_ADV_EXT_RELAY_USING_MAIN_ADV_SET` - Sets the main advertising set to relay messages.

Additional configuration options
================================

This section lists additional configuration options that can be used to configure behavior and performance of Bluetooth Mesh.
The provided values are meant as suggestions only, and should be individually adjusted for each application.

* :kconfig:option:`CONFIG_SYSTEM_WORKQUEUE_STACK_SIZE` - Sets the system workqueue stack size.
  Use the option to increase the system workqueue stack size if the default system workqueue stack size is low.
* :kconfig:option:`CONFIG_MAIN_STACK_SIZE` - Sets the initialization and main thread stack size.
  Use the option to increase the stack size of the default initialization and main thread if necessary.
* :kconfig:option:`CONFIG_HWINFO` - Enables the hardware information driver.
  The hardware information driver must be enabled to perform provisioning of the device.
  See the UUID section of :ref:`bt_mesh_dk_prov`.
* :kconfig:option:`CONFIG_SOC_FLASH_NRF_PARTIAL_ERASE` - Enables partial erase on supported hardware platforms.
  Partial erase allows the flash page erase operation to be split into several small chunks preventing longer CPU stalls.
  This improves responsiveness of a mesh node during the defragmentation of storage areas used by the settings subsystem.
* :kconfig:option:`CONFIG_DK_LIBRARY` - Enables the :ref:`dk_buttons_and_leds_readme` library for the nRF5x development kits.
  Use this option to enable the library if the nRF5x development kits are used.

Logging
-------

* :kconfig:option:`CONFIG_NCS_SAMPLES_DEFAULTS` - Enables the Zephyr system logger with minimal logging implementation.
  This is enabled by default for all samples in |NCS|.
  For more information, see :ref:`ug_logging_zephyr`.
* :kconfig:option:`CONFIG_LOG_MODE_DEFERRED` - Enables deferred logging.
  Setting this configuration option is recommended to avoid slowing down the processing of mesh messages.
  It improves the LPN power consumption when the friendship is established.
* :kconfig:option:`CONFIG_LOG_BUFFER_SIZE` - Sets the number of bytes dedicated for the logger internal buffer.
  Increase the number to avoid missing logs in case of a complex protocol or functionality issue.
* :kconfig:option:`CONFIG_LOG_PROCESS_THREAD_SLEEP_MS` - Sets the sleep period for the internal log processing thread.
  Decrease the value to flush logs more quickly.

GATT Proxy performance
----------------------

These options are only compatible with devices supporting Bluetooth Low Energy (LE) v4.2 or higher.


* The following configuration options allow fitting the full relayed mesh advertiser frame into a single Link Layer payload:

  * :kconfig:option:`CONFIG_BT_CTLR_DATA_LENGTH_MAX` set to 37.
  * :kconfig:option:`CONFIG_BT_BUF_ACL_TX_SIZE` set to 37.
  * :kconfig:option:`CONFIG_BT_BUF_ACL_RX_SIZE` set to 37.

* The following option allows sending up to several data frames during the single connection interval:

  * :kconfig:option:`CONFIG_BT_CTLR_SDC_TX_PACKET_COUNT` =10.

Bluetooth settings/performance
------------------------------

The following configuration options are used to configure the Bluetooth Low Energy behavior and performance:

* :kconfig:option:`CONFIG_BT_COMPANY_ID` - Sets the Bluetooth Company Identifier for this device.
* :kconfig:option:`CONFIG_BT_DEVICE_NAME` - Defines the Bluetooth device name.
* :kconfig:option:`CONFIG_BT_L2CAP_TX_MTU` - Sets the maximum L2CAP MTU for L2CAP TX buffers.
  When GATT is enabled, the recommended value is the value of :kconfig:option:`CONFIG_BT_BUF_ACL_TX_SIZE` minus 4.
* :kconfig:option:`CONFIG_BT_L2CAP_TX_BUF_COUNT` - Sets the number of buffers available for outgoing L2CAP packets.
* :kconfig:option:`CONFIG_BT_RX_STACK_SIZE` - Sets the size of the receiving thread stack.
* :kconfig:option:`CONFIG_BT_EXT_ADV_MAX_ADV_SET` - Sets the maximum number of simultaneous advertising sets.
* :kconfig:option:`CONFIG_BT_BUF_ACL_RX_SIZE` - Sets the data size needed for HCI ACL RX buffers.

Disabled and unused Bluetooth features
--------------------------------------

The following feature options are by default disabled in the samples, but it needs to be considered if any of them are required by the application and thus should be enabled:

* :kconfig:option:`CONFIG_BT_CTLR_DUP_FILTER_LEN` =0.
* :kconfig:option:`CONFIG_BT_CTLR_LE_ENC` =n.
* :kconfig:option:`CONFIG_BT_CTLR_CHAN_SEL_2` =n.
* :kconfig:option:`CONFIG_BT_CTLR_MIN_USED_CHAN` =n.
* :kconfig:option:`CONFIG_BT_CTLR_PRIVACY` =n.
* :kconfig:option:`CONFIG_BT_PHY_UPDATE` =n.

Emergency data storage (EMDS)
-----------------------------

The following configuration options should be considered in case of large networks with high demands on storing the replay protection list (RPL) data.
This will require additional hardware.
For more information, see :ref:`emds_readme`.

* :kconfig:option:`CONFIG_EMDS` - Enables the emergency data storage.
* :kconfig:option:`CONFIG_BT_MESH_RPL_STORAGE_MODE_EMDS` - Enables the persistent storage of RPL in EMDS.

.. _ug_bt_mesh_configuring_lpn:

Low Power node (LPN)
--------------------

The Low Power node (LPN) is a :ref:`power optimization <app_power_opt>` feature specific to Bluetooth Mesh.

The following configuration options are relevant when using the LPN feature:

* Serial communication consumes considerable power, and disabling it should be considered.

  * :kconfig:option:`CONFIG_SERIAL` =n.
  * :kconfig:option:`CONFIG_UART_CONSOLE` =n.

* While enabled, secure beacons will be advertised periodically.
  This consumes power, and is not required for a Low Power node.

  * :kconfig:option:`CONFIG_BT_MESH_BEACON_ENABLED` =n.

* Each LPN poll event consumes power.
  Extending the interval between the poll events will improve the power consumption.

  * :kconfig:option:`CONFIG_BT_MESH_LPN_POLL_TIMEOUT` =600.

* While the GATT Proxy feature is enabled, the Network ID is periodically advertised.
  Disabling it will conserve the energy.

  * :kconfig:option:`CONFIG_BT_MESH_GATT_PROXY_ENABLED` =n.

* Reducing the Node ID advertisement timeout decreases the period where the device consumes power for advertising.

  * :kconfig:option:`CONFIG_BT_MESH_NODE_ID_TIMEOUT` =30.

Persistent storage
------------------

Zephyr's Mesh implementation has been designed to use the :ref:`settings <zephyr:settings_api>` subsystem to store internal states and options in the :ref:`persistent storage <zephyr:bluetooth_mesh_persistent_storage>`.
The settings subsystem can be used with different backends.
Bluetooth Mesh is configured with the :ref:`non-volatile storage (NVS) <zephyr:nvs_api>` as the settings backend.

Using the settings subsystem based on NVS can in some cases result in a significant store time increase.
In a worst case scenario, the store time can be up to several minutes.
This can for example happen when storing a large size replay protection list.
It is recommended to configure the settings subsystem's internal caches to improve the performance.

NVS lookup cache reduces the number of search loops within NVS' application table.

* :kconfig:option:`CONFIG_NVS_LOOKUP_CACHE`.

The Settings NVS name cache reduces the number of search loops of internal parameter identifiers, keeping them in memory.

* :kconfig:option:`CONFIG_SETTINGS_NVS_NAME_CACHE`.

The size of the Settings NVS name cache, :kconfig:option:`CONFIG_SETTINGS_NVS_NAME_CACHE_SIZE`, is recommended to be at least equal to the number of settings entries the device is expected to store.

The Bluetooth Mesh stack stores the following data persistently:

* Network information (primary address and device key)
* Configuration parameters (supported features, default TTL, network transmit and relay retransmit parameters)
* IV index
* Sequence number
* Heartbeat publication information
* Application keys (the amount of entries is controlled by :kconfig:option:`CONFIG_BT_MESH_APP_KEY_COUNT`)
* Network keys (the amount of entries is controlled by :kconfig:option:`CONFIG_BT_MESH_SUBNET_COUNT`)
* Label UUIDs for virtual addressing (the amount of entries is controlled by :kconfig:option:`CONFIG_BT_MESH_LABEL_COUNT`)
* RPL entries (the RPL size is controlled by :kconfig:option:`CONFIG_BT_MESH_CRPL`)

The following data is stored for each model by the Bluetooth Mesh stack:

* Model subscription state
* Model publication state
* Bound application keys
* Subscription list for group addresses
* Subscription list for virtual addresses
* Label UUIDs the model is subscribed to
* Model-specific data

Model data stored persistently can be found under the ``Persistent storage`` section of the corresponding model documentation.

Using the :ref:`bluetooth_mesh_sensor_server` sample as an example, configured according to the sample's :ref:`configuration guide <bluetooth_mesh_sensor_server_conf_models>`, results in the following list of possible entries (entries mentioned above are not included unless specifying the amount of entries):

* 32 RPL entries - since the default Networked Lighting Control (NLC) configuration is used (:kconfig:option:`CONFIG_BT_MESH_NLC_PERF_DEFAULT` is set), the RPL size is 32.
* Application keys - three keys are used.
* Bound application keys - each of the three Sensor Server and Sensor Setup Server models has one bound application key.
* Network keys - only one key is used.
* Model subscriptions - each of the three Sensor Server and Sensor Setup Server models subscribes to a group address.
* Model publication information - each of the three Sensor Server models publishes to a group address.
* Virtual addressing is not used.
* :ref:`Sensor Server model data <bt_mesh_sensor_srv_persistent_readme>` - each of the three Sensor Server models stores the following data:

  * Minimum interval
  * Delta thresholds
  * Fast period divisor
  * Fast cadence range

* The following values are stored in the sample:

  * Temperature range
  * Presence motion threshold
  * Ambient light level gain

Adding up all entries, it is worth setting the cache size to minimum 71.

To have a sufficient space for wear leveling, it is strongly recommended to set the settings partition size to a minimum of 40 KB.
The NVM access backends (NVS and ZMS) also reserve space for the garbage collection process.
In case of ZMS backed (used on nRF54 Series devices) set CONFIG_SETTINGS_ZMS_CUSTOM_SECTOR_COUNT correctly to cover full partition size (that is, set it to 10 for a 40 KB partition).

RPL persistence
---------------

Replay protection list (RPL) is an important security feature that helps protect against replay attacks.
RPL entries are stored in the persistent memory and are used to track the sequence numbers of the messages received from a source node.
Following sub-sections provide information about the RPL persistence and the storage footprint as well as recommendations for the partition size.

Persistent storage sizing for RPL
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The Bluetooth Mesh stack persists RPL entries through power cycles so that replay protection remains effective after a device reboot.
How much flash storage the RPL consumes and how long it lasts depends on the following three parameters:

* The configured RPL size (:kconfig:option:`CONFIG_BT_MESH_CRPL`)
* The number of distinct source nodes that actively send messages to the device
* The mechanism used to persist the RPL (Settings + ZMS/NVS vs. EMDS)

For nRF52 and nRF53 Series devices, NVS backend is recommended and for the nRF54 Series devices, ZMS backend should be used.

In the |NCS|, the settings subsystem with ZMS/NVS backend is selected as the default option for RPL persistence.
For networks where power loss must not open a replay window, consider using EMDS instead of the settings subsystem for RPL persistence.
See :ref:`emds_readme`.

When RPL is persisted through the settings subsystem, the store timeout (:kconfig:option:`CONFIG_BT_MESH_RPL_STORE_TIMEOUT`) determines the interval at which changed RPL entries are written to storage.
Both the NVS and ZMS backends run garbage collection automatically when a write needs free space; GC is not application-controllable, may start on any Settings write, and blocks further Settings writes until the current sector GC completes.

**Per-entry storage footprint**

Each RPL entry is stored as a Settings key-value pair.
The path has the form ``bt/mesh/RPL/<addr>``, where ``<addr>`` is the unicast source address in lowercase hex.
Depending on the selected backend, RPL entries are stored with different overhead.

**RPL footprint by CRPL size (NVS backend, 36 B/entry)**

The total footprint for each entry (Settings + NVS backend (nRF52/nRF53)) is 36 B per occupied slot.
For more details see :ref:`non-volatile storage (NVS) <zephyr:nvs_api>` and :ref:`Settings subsystem <zephyr:settings_api>`.

.. note::
   The per-entry layout is:

   - 8 B ATE for the key string record
   - 16 B name data block (the path, such as ``bt/mesh/RPL/1234``, aligned to a 4-byte boundary and stored without a null terminator)
   - 8 B ATE for the value record
   - 4 B for the value data (the ``struct rpl_val``, aligned to 4 bytes and stored with its ATE in the sector data area)

   On an update of an existing entry only the value record (12 B: ATE + data) is rewritten.
   The name record (24 B) is written once when the entry is first created.

.. list-table:: Total RPL footprint in flash (Settings + NVS)
   :header-rows: 1
   :widths: 25 25 50

   * - :kconfig:option:`CONFIG_BT_MESH_CRPL`
     - Footprint (all slots occupied)
     - Notes
   * - 32
     - 1,152 B (~1.1 KB)
     - Occupies ~3.5% of a 32 KB partition
   * - 255
     - 9,180 B (~9.0 KB)
     - Maximum; occupies ~28% of a 32 KB partition

**RPL footprint by CRPL size (ZMS backend, 64 B/entry)**

The total footprint for each entry (Settings + ZMS backend (nRF54L RRAM)) is 64 B for each occupied slot.
For more details, see the ZMS storage model in :ref:`ZMS documentation in Zephyr <zephyr:zms_api>` and :ref:`zms_memory_storage`.

.. note::
   The per-entry layout is:

   - 16 B Value ATE (4-byte ``struct rpl_val``, seq:24, old_iv:1, fits inline)
   - 16 B linked-list node ATE (8-byte list node, fits inline)
   - 16 B Name ATE (key string overhead)
   - 16 B Name data block (16-char or shorter key ``bt/mesh/RPL/1234`` aligned to a 16-byte RRAM boundary)

   On an update of an existing entry only the value ATE (16 B) is rewritten; the name and linked-list records are not touched.

.. list-table:: Total RPL footprint in flash (Settings + ZMS)
   :header-rows: 1
   :widths: 25 25 50

   * - :kconfig:option:`CONFIG_BT_MESH_CRPL`
     - Footprint (all slots occupied)
     - Notes
   * - 32
     - 2,048 B (~2 KB)
     - Occupies ~6% of a 32 KB partition
   * - 255
     - 16,320 B (~16 KB)
     - Maximum; occupies ~50% of a 32 KB partition

**Write endurance (ZMS backend)**

The write endurance of a ZMS partition is limited by the RRAM erase-block cycle count (10,000 cycles for each 16 byte wordline on nRF54L Series devices (see `nRF54L15 RRAM programming parameters`_); ZMS ``sector_size`` is configured by the :kconfig:option:`SETTINGS_ZMS_SECTOR_SIZE_MULT` Kconfig option).
Garbage collection runs sector by sector: each GC pass copies only the still-valid entries in one closed sector forward, then reclaims that sector.

The lifetime of the partition can be estimated as follows (whole-partition model):

.. code-block:: none

   total_usable        = (sector_count - 1) x (sector_size - 2 x ate_size)
   valid_rpl_footprint = occupied_slots x 64
   net_writable        = total_usable - valid_rpl_footprint
   flushes_per_cycle   = net_writable / bytes_per_flush
   total_flushes       = 10,000 x flushes_per_cycle
   lifetime            = total_flushes x store_timeout

* ``valid_rpl_footprint`` is the total size of all live RPL records in the partition; GC must retain this data, so it is not available for new writes in a GC cycle.
* ``sector_count`` is the number of ZMS sectors in the partition (for example 8 for a 32 KB partition with 4 KB sectors); one sector is reserved for garbage collection, so only ``sector_count - 1`` sectors contribute to ``total_usable``.
* ``bytes_per_flush`` equals the number of dirty entries written for each store event multiplied by 16 bytes (one value ATE for each entry).

The formula treats the partition as a single pool of usable space minus live RPL data and yields a sizing estimate. Validate lifetime for your partition layout, RPL occupancy, and traffic.

The number of dirty entries for each flush on a mesh node is bounded by the number of distinct source nodes that send a message within one store-timeout window to that node:

.. code-block:: none

   dirty_entries_per_flush = ceil(message_rate_msg_per_s x store_timeout_s)

For a light bulb (or a mesh node) receiving messages at an average rate of 0.1 msg/s with a 30 s store timeout: ``ceil(0.1 x 30) = 3`` dirty entries for each flush, so ``bytes_per_flush = 3 x 16 = 48 B``.
Here, it is assumed that all messages are from distinct source nodes and hence all are counted as dirty entries.
If the messages are from the same source node, only one dirty entry will get counted for each flush.

The following tables show the estimated lifetime in years for Settings + ZMS at 0.1 msg/s and a 30 s store timeout (3 dirty entries for each flush, 48 B for each flush), for a partition that contains RPL data only.
ZMS reserves one close ATE and one empty ATE at the end of each sector, so usable space per sector is ``sector_size - 2 x ate_size`` (4,064 B for 4 KB sectors).
Additionally, one sector is reserved for garbage collection.
You can count the partition usable bytes as follows:

* 32 KB = 7 sectors × 4,064 B = 28,448 B
* 64 KB = 15 sectors × 4,064 B = 60,960 B
* 72 KB = 17 sectors × 4,064 B = 69,088 B
* 128 KB = 31 sectors × 4,064 B = 125,984 B

**Example (CRPL = 32, all slots occupied, 32 KB partition):**

.. code-block:: none

   total_usable        = 7 × 4,064 = 28,448 B
   valid_rpl_footprint = 32 × 64 = 2,048 B
   net_writable        = 28,448 − 2,048 = 26,400 B
   flushes_per_cycle   = 26,400 / 48 = 550
   total_flushes       = 10,000 × 550 = 5,500,000
   lifetime            = 5,500,000 × 30 s = 165,000,000 s = 1,909.7 d (5.23 yr)

**RPL write endurance - CRPL = 32 (Settings + ZMS, 0.1 msg/s, 30 s store timeout)**

.. list-table::
   :header-rows: 1
   :widths: 28 17 17 17 17

   * - Occupancy
     - 32 KB (yr)
     - 64 KB (yr)
     - 72 KB (yr)
     - 128 KB (yr)
   * - All 32 slots occupied
     - 5.23
     - **11.67**
     - 13.28
     - 24.55
   * - 10 slots occupied
     - 5.51
     - 11.95
     - 13.56
     - 24.82

**RPL write endurance - CRPL = 255 (Settings + ZMS, 0.1 msg/s, 30 s store timeout)**

.. list-table::
   :header-rows: 1
   :widths: 28 17 17 17 17

   * - Occupancy
     - 32 KB (yr)
     - 64 KB (yr)
     - 72 KB (yr)
     - 128 KB (yr)
   * - All 255 slots occupied
     - 2.40
     - 8.84
     - **10.45**
     - 21.72
   * - 10 slots occupied
     - 5.51
     - 11.95
     - 13.56
     - 24.82

Estimated lifetime scales proportionally with :kconfig:option:`CONFIG_BT_MESH_RPL_STORE_TIMEOUT`.
At a fixed incoming message rate, the estimated lifetime is largely independent of this setting.
A shorter timeout produces smaller but more frequent flushes, leaving the long-run write rate (and therefore the lifetime) essentially unchanged.
The timeout shortens lifetime only when it raises the long-run write rate, that is, when the number of dirty entries for each flush is held fixed while flushes occur more often (which implies a proportionally higher message rate).

.. note::

   For the Settings + NVS backend (nRF52/nRF53), the write size for each update is 12 B (value record only) rather than 16 B, and the valid footprint is 36 B rather than 64 B for each entry.
   This yields smaller bytes-per-flush (3 × 12 = 36 B at 0.1 msg/s) and lower valid footprint overhead per GC cycle, resulting in better endurance for equivalent CRPL size and partition configuration.
   NVS Backend is not recommended for RRAM based devices (nRF54 Series).

**Partition sizing recommendations**

* :kconfig:option:`CONFIG_BT_MESH_CRPL` = 32:
  This is the minimum required RPL size for the `Ambient Light Sensor NLC Profile`_ (see the :ref:`bluetooth_mesh_sensor_client` sample), `Basic Scene Selector NLC Profile`_ (see the :ref:`bluetooth_mesh_light_dim` sample), `Dimming Control NLC Profile`_ (see the :ref:`bluetooth_mesh_light_dim` sample), and `Occupancy Sensor NLC Profile`_ (see the :ref:`bluetooth_mesh_sensor_server` sample).
  For the endurance scenario in the tables (all 32 slots occupied), a 32 KB partition can be sufficient; worst-case lifetime exceeds five years under the assumptions.
  Recommended: 64 KB settings partition for an estimated 11.67 years lifetime under the same assumptions.
  Evaluate the lifetime for your specific use case and consider sizing the partition as needed.

* :kconfig:option:`CONFIG_BT_MESH_CRPL` = 255:
  This option is required for the `Basic Lightness Controller NLC Profile`_ (see the :ref:`bluetooth_mesh_light_lc` sample) and `HVAC Integration NLC Profile`_ (see the  :ref:`bluetooth_mesh_sensor_client` sample).
  Recommended: 72 KB settings partition for an estimated 10.45 years lifetime under the same assumptions.
  Evaluate the lifetime for your specific use case and consider sizing the partition as needed.

.. note::
   The `Energy Monitor NLC Profile`_ requires minimum of 32 CRPL entries.
   However, when it is combined with `Basic Lightness Controller NLC Profile`_ (see the :ref:`bluetooth_mesh_light_lc` sample), the minimum requirement becomes same as the profile requiring the higher number of entries, in this case 255 (see `Section 3.6 Combinations of NLC profiles`_ in Basic Lightness Controller NLC Profile specification).

.. note::

   These figures assume that the partition contains RPL entries only.
   Other Mesh settings (model settings or persisted model states (such as the last known non-zero Light level), network keys, application keys, sequence number, IV index) and application data (if any) share the same partition.
   Therefore, live data copied on each sector GC is a bit larger, and thus net space for wear leveling is smaller, and effective lifetime is shorter than the RPL-only figures.
   For example, in-case of the :ref:`bluetooth_mesh_light_lc` sample when a device is provisioned, and all models are bound to appkeys, subscribed to certain group address, and two model instances are configured to publish to a certain group address, the footprint of all mesh settings related data (excluding RPL) is 2544 Bytes.

Security toolbox
----------------

The Mesh security toolbox implementation uses Zephyr's OS Services that provide `PSA Certified Crypto API`_ for implementing the encryption and authentication functionality.
The :ref:`Mbed TLS <zephyr:psa_crypto>` crypto library is used as the default crypto service provider.
If the platform supports :ref:`Trusted Firmware-M (TF-M) <zephyr:tfm>`, it is used as the crypto service provider.

.. note::
   For Bluetooth Mesh provisioning, authenticating with the BTM_ECDH_P256_CMAC_AES128_AES_CCM (0x00) algorithm does not provide protection against an active man-in-the-middle (MITM) attacker during the provisioning process if OOB public keys are not used.

The Bluetooth Mesh security toolbox does not operate with open key values.
After Bluetooth Mesh receives an open key value, it immediately imports the key into the crypto library and receives the unique key identifier.
The key identifiers are used in the security toolbox and stored in the persistent memory.
The crypto library is responsible for storing of the key values in the Internal Trusted Storage (`PSA Certified Secure Storage API 1.0`_).
Bluetooth Mesh data structures based on TinyCrypt (now removed from the SDK) and the PSA API, as well as images of these structures stored in the persistent memory, are not compatible due to different key representations.
When a provisioned device updates its firmware binary from the TinyCrypt-based toolbox to firmware binary that uses the PSA API based toolbox, a provisioned device must be unprovisioned first and reprovisioned after the update.
The provisioned device cannot restore data from the persistent memory after firmware update.
If the image is changed over Mesh DFU, it is recommended to use :c:enumerator:`BT_MESH_DFU_EFFECT_UNPROV`.

A provisioned device can update its firmware image from the TinyCrypt-based toolbox to firmware image that uses the PSA API based toolbox without unprovisioning if the key importer functionality is used.
The :kconfig:option:`CONFIG_BT_MESH_KEY_IMPORTER` Kconfig option enables the key importer functionality.
The key importer is an application initialization functionality that is called with kernel initialization priority before starting main.
This functionality reads out the persistently stored Bluetooth Mesh data and if it finds keys stored by the TinyCrypt-based security toolbox, it imports them over the PSA API into the crypto library and stores the key identifiers in a format based on the PSA API toolbox.
Once the new firmware image starts Bluetooth Mesh initialization, the persistent area already has the stored data in the correct format.

The device can be vulnerable to attacks while the device uses the key importer functionality.
The following two types of security risks are possible:

* If the device is provided with a new image with the key importer functionality enabled, the new image is not yet activated and the attacker can write arbitrary data in the persistent memory during this time by whichever methods.
  The fake keys might be imported to the PSA crypto library after the next device reset (which activates the new firmware with the key importer).
  The device gets provisioned to the attackers network and the attacker can read out the mesh state data from the device.

  Mitigation:

  * Ensure that the device is protected from unauthorized writes to the non-volatile storage.

* Even after the key importer imports the keys to the crypto library, the plain text values are left in the flash until the next garbage collection is triggered by the storage backend.

  Mitigation:

  * Ensure that the device is protected from unauthorized reads (such as reading flash from programmer, or using mcumgr shell commands) from the non-volatile storage.
  * Execute a key refresh procedure for all existing keys used on the entire network as soon as possible by excluding the compromised device, if any.
    The mechanism to determine if the device is compromised is up to the OEM developers.

Additionally, after upgrading the device firmware with the key importer functionality enabled, and once the key import is complete, it is recommended to update device firmware with the key importer functionality disabled as soon as possible.

Secure storage
--------------

:ref:`secure_storage_in_ncs` lets you securely store and manage sensitive data.
Currently, all :ref:`bt_mesh_samples` in the |NCS| use the :ref:`trusted_storage_readme` library as the PSA Secure Storage API implementation for all supported platforms.

.. note::
   For the nRF52840 devices, in regards to :ref:`bt_mesh_samples` in |NCS|, AEAD keys are derived using hashes of entry UIDs (:kconfig:option:`CONFIG_TRUSTED_STORAGE_BACKEND_AEAD_KEY_HASH_UID`).
   This approach is less secure than using the :ref:`lib_hw_unique_key` library for key derivation as it only provides integrity of sensitive material.
   It is also possible to implement a custom AEAD key generation method when the :kconfig:option:`CONFIG_TRUSTED_STORAGE_BACKEND_AEAD_KEY_CUSTOM` Kconfig option is selected.

For more details about AEAD key generation and backend configuration, see the :ref:`trusted_storage_readme`.
