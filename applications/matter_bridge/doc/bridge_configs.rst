Matter bridge application configuration options
===============================================

.. toggle::

   Check and configure the following configuration options:

   .. _CONFIG_BRIDGED_DEVICE_SIMULATED:

   CONFIG_BRIDGED_DEVICE_SIMULATED
      ``bool`` - Implement a simulated bridged device.

   .. _CONFIG_BRIDGED_DEVICE_BT:

   CONFIG_BRIDGED_DEVICE_BT
      ``bool`` - Implement a Bluetooth LE bridged device.

   .. _CONFIG_BRIDGE_HUMIDITY_SENSOR_BRIDGED_DEVICE:

   CONFIG_BRIDGE_HUMIDITY_SENSOR_BRIDGED_DEVICE
      ``bool`` - Enable support for Humidity Sensor bridged device.

   .. _CONFIG_BRIDGE_ONOFF_LIGHT_BRIDGED_DEVICE:

   CONFIG_BRIDGE_ONOFF_LIGHT_BRIDGED_DEVICE
      ``bool`` - Enable support for OnOff Light bridged device.

   .. _CONFIG_BRIDGE_GENERIC_SWITCH_BRIDGED_DEVICE:

   CONFIG_BRIDGE_GENERIC_SWITCH_BRIDGED_DEVICE
      ``bool`` - Enable support for Generic Switch bridged device.

   .. _CONFIG_BRIDGE_ONOFF_LIGHT_SWITCH_BRIDGED_DEVICE:

   CONFIG_BRIDGE_ONOFF_LIGHT_SWITCH_BRIDGED_DEVICE
      ``bool`` - Enable support for OnOff Light Switch bridged device.

   .. _CONFIG_BRIDGE_TEMPERATURE_SENSOR_BRIDGED_DEVICE:

   CONFIG_BRIDGE_TEMPERATURE_SENSOR_BRIDGED_DEVICE
      ``bool`` - Enable support for Temperature Sensor bridged device.

   .. _CONFIG_BRIDGE_MIGRATE_PRE_2_7_0:

   CONFIG_BRIDGE_MIGRATE_PRE_2_7_0
      ``bool`` - Enable migration of bridged device data stored in old scheme from pre |NCS| 2.7.0 releases.

   .. _CONFIG_BRIDGE_MIGRATE_VERSION_1:

   CONFIG_BRIDGE_MIGRATE_VERSION_1
      ``bool`` - Enable migration of bridged device data stored in version 1 of new scheme.

      If you selected the simulated device implementation using the :ref:`CONFIG_BRIDGED_DEVICE_SIMULATED <CONFIG_BRIDGED_DEVICE_SIMULATED>` Kconfig option, also check and configure the following option:

   .. _CONFIG_BRIDGED_DEVICE_SIMULATED_ONOFF_AUTOMATIC:

   CONFIG_BRIDGED_DEVICE_SIMULATED_ONOFF_AUTOMATIC
      ``bool`` - Automatically simulated OnOff device.
      The simulated device automatically changes its state periodically.

   .. _CONFIG_BRIDGED_DEVICE_SIMULATED_ONOFF_SHELL:

   CONFIG_BRIDGED_DEVICE_SIMULATED_ONOFF_SHELL
      ``bool`` - Shell-controlled simulated OnOff device.
      The state of the simulated device is changed using shell commands.

   If you selected the Bluetooth LE device implementation using the :ref:`CONFIG_BRIDGED_DEVICE_BT <CONFIG_BRIDGED_DEVICE_BT>` Kconfig option, also check and configure the following options:

   .. _CONFIG_BRIDGE_BT_MAX_SCANNED_DEVICES:

   CONFIG_BRIDGE_BT_MAX_SCANNED_DEVICES
      ``int`` - Set the maximum number of scanned devices.

   .. _CONFIG_BRIDGE_BT_MINIMUM_SECURITY_LEVEL:

   CONFIG_BRIDGE_BT_MINIMUM_SECURITY_LEVEL
      ``int`` - Set the minimum Bluetooth security level of bridged devices that the bridge device will accept.
      Bridged devices using this or a higher level will be allowed to connect to the bridge.
      See the :ref:`matter_bridge_app_bt_security` section for more information.

   .. _CONFIG_BRIDGE_BT_RECOVERY_MAX_INTERVAL:

   CONFIG_BRIDGE_BT_RECOVERY_MAX_INTERVAL
      ``int`` - Set the maximum time (in seconds) between recovery attempts when the Bluetooth LE connection to the bridged device is lost.

   .. _CONFIG_BRIDGE_BT_RECOVERY_SCAN_TIMEOUT_MS:

   CONFIG_BRIDGE_BT_RECOVERY_SCAN_TIMEOUT_MS
      ``int`` - Set the time (in milliseconds) within which the Bridge will try to re-establish a connection to the lost Bluetooth LE device.

   .. _CONFIG_BRIDGE_BT_SCAN_TIMEOUT_MS:

   CONFIG_BRIDGE_BT_SCAN_TIMEOUT_MS
      ``int`` - Set the Bluetooth LE scan timeout in milliseconds.

   .. _CONFIG_BRIDGE_FORCE_BT_CONNECTION_PARAMS:

   CONFIG_BRIDGE_FORCE_BT_CONNECTION_PARAMS
      ``bool`` - Determines whether the Matter bridge forces connection parameters or accepts the Bluetooth LE peripheral device selection.

   .. _CONFIG_BRIDGE_BT_SCAN_WINDOW:

   CONFIG_BRIDGE_BT_SCAN_WINDOW
      ``int`` - Duration of a central actively scanning for devices within scan interval, equal to ``CONFIG_BRIDGE_BT_SCAN_WINDOW`` * 0.625 ms.

   .. _CONFIG_BRIDGE_BT_SCAN_INTERVAL:

   CONFIG_BRIDGE_BT_SCAN_INTERVAL
      ``int`` - Time between consecutive Bluetooth scan windows, equal to ``CONFIG_BRIDGE_BT_SCAN_INTERVAL`` * 0.625 ms.

   .. _CONFIG_BRIDGE_BT_CONNECTION_INTERVAL_MIN:

   CONFIG_BRIDGE_BT_CONNECTION_INTERVAL_MIN
      ``int`` - The minimum duration of time requested by central after the peripheral device should wake up to communicate, equal to ``CONFIG_BRIDGE_BT_CONNECTION_INTERVAL_MIN`` * 1.25 ms.

   .. _CONFIG_BRIDGE_BT_CONNECTION_INTERVAL_MAX:

   CONFIG_BRIDGE_BT_CONNECTION_INTERVAL_MAX
      ``int`` - The maximum duration of time requested by central after the peripheral device should wake up to communicate, equal to ``CONFIG_BRIDGE_BT_CONNECTION_INTERVAL_MAX`` * 1.25 ms.

   .. _CONFIG_BRIDGE_BT_CONNECTION_TIMEOUT:

   CONFIG_BRIDGE_BT_CONNECTION_TIMEOUT
      ``int`` - The time since the last packet was successfully received until the devices consider the connection lost, equal to ``CONFIG_BRIDGE_BT_CONNECTION_TIMEOUT`` cs.

   .. _CONFIG_BRIDGE_BT_CONNECTION_LATENCY:

   CONFIG_BRIDGE_BT_CONNECTION_LATENCY
      ``int`` - The number of connection events the peripheral can skip waking up for if it does not have any data to send.

   The following options affect how many bridged devices the application supports.
   See the :ref:`matter_bridge_app_bridged_support_configs` section for more information.

   .. _CONFIG_BRIDGE_MAX_BRIDGED_DEVICES_NUMBER:

   CONFIG_BRIDGE_MAX_BRIDGED_DEVICES_NUMBER
      ``int`` - Set the maximum number of physical non-Matter devices supported by the Bridge.

   .. _CONFIG_BRIDGE_MAX_DYNAMIC_ENDPOINTS_NUMBER:

   CONFIG_BRIDGE_MAX_DYNAMIC_ENDPOINTS_NUMBER
      ``int`` - Set the maximum number of dynamic endpoints supported by the Bridge.
