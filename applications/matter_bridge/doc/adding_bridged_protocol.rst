.. _matter_bridge_app_extending_protocol:

Adding support for a proprietary protocol
#########################################

The :ref:`Matter Bridge architecture <ug_matter_overview_bridge_ncs_implementation>` describes the two sides of bridging relationship, ``Matter Bridged Device`` and ``Bridged Device Data Provider``.
The ``Matter Bridged Device`` represents the side that natively supports Matter, and the ``Bridged Device Data Provider`` represents a non-Matter interface.
It is not specified what protocol should be used for a ``Bridged Device Data Provider`` role.

The Matter Bridge application supports bridging Matter to Bluetooth® LE data providers that use the Bluetooth LE protocol to communicate with physical end devices, and simulated data providers that emulate real devices behavior in the application.
You can add support for any protocol as long as you are able to describe data model translation between Matter and that protocol.

To provide support for a new protocol, you have to implement the ``Bridged Device Data Provider`` role that is able to communicate with the physical end devices using that protocol.
The interactions to be implemented by the ``Bridged Device Data Provider`` vary depending on the bridged Matter device type.

Typically the ``Bridged Device Data Provider`` has to provide up-to-date information about the device's state by pushing notification about the state changes.
This allows updating the Matter Data Model state and also enables handling Matter subscriptions.
The subscription handling can be implemented either by utilizing the implemented protocol's native support for pushing notifications by the end device, or by performing periodic data polling, which is less efficient.

If the Matter device can perform write operations or be controlled using Matter invoke command operations, the ``Bridged Device Data Provider`` shall be able to send appropriate information to the physical end device.

Complete the following steps to add support for a proprietary protocol:

1. Create a ``MyProtocol Data Provider`` class that inherits from the ``Bridged Device Data Provider`` class.
   This class should contain all the data and logic used by all your bridged devices.
   For reference, see the implementation in the :file:`src/ble/data_providers/ble_bridged_device.h` file.

#. For each bridged device type, create a ``MyProtocol MyDevice Data Provider`` class that inherits from the ``MyProtocol Data Provider`` class.
   These classes keep the state of the Matter device synchronized with the state of the device, using the following functions:
   * :c:func:`NotifyUpdateState` - Called to notify the bridge when the bridged device changed state.
   * :c:func:`UpdateState` - Called by the bridge when the bridged device should update its state (for example, turning a light on or off).

#. Create a ``MyProtocol Connectivity Manager`` class that holds information about all current connections of the bridged device.
   This class must implement the following functions:

   .. code-block:: C++

      CHIP_ERROR AddMyProtocolProvider(MyProtocolBridgedDeviceProvider *provider, const MyProtocolId &deviceId);
      CHIP_ERROR RemoveMyProtocolProvider(const MyProtocolId &deviceId);
      MyProtocolBridgedDeviceProvider *FindMyProtocolProvider(const MyProtocolId &deviceId);

   This class takes care of any connection-related code that your protocol needs.

#. Create a ``MyProtocol Bridged Device Factory`` class that handles device creation, storage, and removal.

#. Adapt the main application to use your newly created classes.
   Create a new ``BRIDGED_DEVICE_MY_PROTOCOL`` configuration, go through the application code and find where the :ref:`CONFIG_BRIDGED_DEVICE_BT <CONFIG_BRIDGED_DEVICE_BT>` Kconfig option is used by the preprocessor.
   Add support for your protocol in the found instances.

An application with support for ``MyProtocol`` would work like this:

1. The application reads the stored devices and creates them using the ``MyProtocol Bridged Device Factory`` class.
#. To bridge new devices, the application needs to know their ``MyProtocol`` connection details.
   Those details can be provided by the user or obtained through device discovery.
   To enable this, implement the necessary shell commands in the :file:`src/bridge_shell.cpp` file.
#. Using the ``MyProtocol Bridged Device Factory`` class and knowing the connection details, the application creates and stores new bridged devices.
   You can use the ``MyProtocol Connectivity Manager`` class to map the ``MyProtocol`` device ID to the corresponding ``MyProtocol Device Data Provider`` instance.
#. Matter traffic is routed through the ``MyProtocol MyDevice Data Provider`` class.
   When data from the device is received, you can use the ``MyProtocol Connectivity Manager`` class to forward the data to the correct ``MyProtocol MyDevice Data Provider`` instance.
#. If a device must be removed, the ``MyProtocol Bridged Device Factory`` class handles it.
