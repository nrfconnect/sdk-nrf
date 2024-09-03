.. _bt_mesh_ponoff_srv_readme:

Generic Power OnOff Server
##########################

.. contents::
   :local:
   :depth: 2

The Generic Power OnOff Server controls the power-up behavior of other models on its element.

The Generic Power OnOff Server adds the following model instances in the composition data:

- The Generic Power OnOff Server
- The Generic Power OnOff Setup Server

These model instances share the states of the Generic Power OnOff Server, but accept different messages.
This allows for a fine-grained control of the access rights for the Generic Power OnOff state, as the two model instances can be bound to different application keys:

* The Generic Power OnOff Server only provides read access to the Generic On Power Up state.
* The Generic Power OnOff Setup Server provides write access to the Generic On Power Up state, allowing configurator devices to change the power-up behavior of the element.

Configuration
=============

The following configuration parameters are associated with the Generic Power OnOff Server model:

* :kconfig:option:`CONFIG_BT_SETTINGS` - The server requires this option to be enabled to work properly.
  Unless :kconfig:option:`CONFIG_BT_SETTINGS` is explicitly disabled, including the Generic Power OnOff Server will enable :kconfig:option:`CONFIG_BT_SETTINGS`.

States
======

The Generic Power OnOff Server model contains the following state:

Generic On Power Up: :c:enum:`bt_mesh_on_power_up`
    The Generic On Power Up state controls the initial value of the extended Generic OnOff state when the device powers up.
    The state can have the following initial values:

    * :c:enumerator:`BT_MESH_ON_POWER_UP_OFF` - The OnOff state is initialized to Off.
    * :c:enumerator:`BT_MESH_ON_POWER_UP_ON` - The OnOff state is initialized to On.
      If any other states are bound to the On Power Up state, they are initialized to their default values.
    * :c:enumerator:`BT_MESH_ON_POWER_UP_RESTORE` - The OnOff state is initialized to its last known value.
      If any other states are bound to the On Power Up state, they are initialized to their default values.

    The memory for the Generic On Power Up state is contained in the model structure, and state changes can be observed with the :c:member:`bt_mesh_ponoff_srv.update` callback.

Extended models
===============

The Generic Power OnOff Server extends the following models:

* :ref:`bt_mesh_onoff_srv_readme`
* :ref:`bt_mesh_dtt_srv_readme`

The On Power Up state is bound to the Generic OnOff state of the extended Generic OnOff model through its power-up behavior.
No other state bindings are present, and the callbacks for both the Generic OnOff server and the Generic DTT server are forwarded to the application as they are.

Persistent storage
==================

The Generic On Power Up state is stored persistently, along with the current Generic OnOff state of the extended :ref:`bt_mesh_onoff_srv_readme`.

If :kconfig:option:`CONFIG_BT_SETTINGS` is enabled, the Generic Power OnOff Server stores all its states persistently using a configurable storage delay to stagger storing.
See :kconfig:option:`CONFIG_BT_MESH_STORE_TIMEOUT`.

API documentation
=================

| Header file: :file:`include/bluetooth/mesh/gen_ponoff_srv.h`
| Source file: :file:`subsys/bluetooth/mesh/gen_ponoff_srv.c`

.. doxygengroup:: bt_mesh_ponoff_srv
   :project: nrf
   :members:
