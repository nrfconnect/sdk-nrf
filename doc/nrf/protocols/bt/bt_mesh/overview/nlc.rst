.. _ug_bt_mesh_nlc:

Bluetooth Networked Lighting Control profiles
#############################################

Bluetooth® Networked Lighting Control (NLC) profiles are a set of device profiles built on top of the Bluetooth Mesh protocol, specified by the Bluetooth Special Interest Group (SIG) in the `Bluetooth NLC profile specifications`_.
The NLC profiles can be used to implement interoperable network controlled lighting setups, including sensors, light fixtures, energy monitoring, scene selectors and dimmer controls.
Each of the profiles specifies a set of models and a set of performance parameters.

Enable the :kconfig:option:`CONFIG_BT_MESH_NLC_PERF_CONF` Kconfig option to set the required performance configurations for the NLC profiles.
Multiple NLC profiles can be combined on a single device.
When combining profiles, the device will use the highest minimum requirements, as defined in the NLC Profile specifications.

For specific profile implementations, enable the following Kconfig options:

* :kconfig:option:`CONFIG_BT_MESH_NLC_PERF_LIGHTNESS_CTRL` - for the NLC Basic Lightness Controller profile
* :kconfig:option:`CONFIG_BT_MESH_NLC_PERF_HVAC` - for the NLC HVAC Integration profile

The |NCS| provides a demonstration of how to implement each of these profiles as part of the Bluetooth samples.
An overview of the NLC profiles and the samples supporting them is provided in a table below.

.. table::
   :align: center

   +-----------------------------------------+--------------------------------------+
   | NLC profile                             | Sample                               |
   +=========================================+======================================+
   | Ambient Light Sensor NLC Profile        | :ref:`bluetooth_mesh_sensor_server`  |
   +-----------------------------------------+--------------------------------------+
   | Basic Lightness Controller NLC Profile  | :ref:`bluetooth_mesh_light_lc`       |
   +-----------------------------------------+--------------------------------------+
   | Basic Scene Selector NLC Profile        | :ref:`bluetooth_mesh_light_dim`      |
   +-----------------------------------------+--------------------------------------+
   | Dimming Control NLC Profile             | :ref:`bluetooth_mesh_light_dim`      |
   +-----------------------------------------+--------------------------------------+
   | Energy Monitor NLC Profile              | :ref:`bluetooth_mesh_light_lc`       |
   +-----------------------------------------+--------------------------------------+
   | HVAC Integration NLC Profile            | :ref:`bluetooth_mesh_sensor_client`  |
   +-----------------------------------------+--------------------------------------+
   | Occupancy Sensor NLC Profile            | :ref:`bluetooth_mesh_sensor_server`  |
   +-----------------------------------------+--------------------------------------+
