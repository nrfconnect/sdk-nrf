.. _ug_bt_mesh_nlc:

Bluetooth Networked Lighting Control profiles
#############################################

Bluetooth® Networked Lighting Control (NLC) profiles are a set of device profiles built on top of the Bluetooth Mesh protocol, specified by the Bluetooth Special Interest Group (SIG) in the `Bluetooth NLC profile specifications`_.
The NLC profiles can be used to implement interoperable network controlled lighting setups, including sensors, light fixtures, energy monitoring, scene selectors and dimmer controls.
Each of the profiles specifies a set of models and a set of performance parameters.

Multiple NLC profiles can be combined on a single device.
When combining profiles, the device will use the highest minimum requirements, as defined in the NLC Profile specifications.

For specific profile implementations, enable the following Kconfig options:

* :kconfig:option:`CONFIG_BT_MESH_NLC_PERF_AMBIENT_LIGHT_SENSOR` - for the Ambient Light Sensor NLC profile
* :kconfig:option:`CONFIG_BT_MESH_NLC_PERF_DIMMING_CONTROL` - for the Dimming Control NLC profile
* :kconfig:option:`CONFIG_BT_MESH_NLC_PERF_ENERGY_MONITOR` - for the Energy Monitor NLC profile
* :kconfig:option:`CONFIG_BT_MESH_NLC_PERF_HVAC_INTEGRATION` - for the HVAC Integration NLC profile
* :kconfig:option:`CONFIG_BT_MESH_NLC_PERF_LIGHTNESS_CTRL` - for the Basic Lightness Controller NLC profile
* :kconfig:option:`CONFIG_BT_MESH_NLC_PERF_OCCUPANCY_SENSOR` - for the Occupancy Sensor NLC profile
* :kconfig:option:`CONFIG_BT_MESH_NLC_PERF_SCENE_SELECTOR` - for the Basic Scene Selector NLC profile

The |NCS| provides a demonstration of how to implement each of these profiles as part of the Bluetooth samples.
An overview of the NLC profiles and the samples supporting them is provided in a table below.

.. table::
   :align: center

   +-----------------------------------------+-----------------------------------------------+
   | NLC profile                             | Sample                                        |
   +=========================================+===============================================+
   | Ambient Light Sensor NLC Profile        | :ref:`bluetooth_mesh_nlc_ambient_light_sensor`|
   +-----------------------------------------+-----------------------------------------------+
   | Basic Lightness Controller NLC Profile  | :ref:`bluetooth_mesh_light_lc`                |
   +-----------------------------------------+-----------------------------------------------+
   | Basic Scene Selector NLC Profile        | :ref:`bluetooth_mesh_light_dim`               |
   +-----------------------------------------+-----------------------------------------------+
   | Dimming Control NLC Profile             | :ref:`bluetooth_mesh_light_dim`               |
   +-----------------------------------------+-----------------------------------------------+
   | Energy Monitor NLC Profile              | :ref:`bluetooth_mesh_light_lc`                |
   +-----------------------------------------+-----------------------------------------------+
   | HVAC Integration NLC Profile            | :ref:`bluetooth_mesh_sensor_client`           |
   +-----------------------------------------+-----------------------------------------------+
   | Occupancy Sensor NLC Profile            | :ref:`bluetooth_mesh_nlc_ambient_light_sensor`|
   +-----------------------------------------+-----------------------------------------------+
