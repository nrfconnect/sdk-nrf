.. _bt_mesh_dtt_srv_readme:

Generic Default Transition Time Server
######################################

.. contents::
   :local:
   :depth: 2

The DTT Server provides a common way to specify the state transition time for other models on any element.
If a DTT Server is not present on the model's element, use the DTT Server model instance that is present on the element with the largest address that is smaller than the address of the given element.
This way, if other generic models on any element receive state change commands without transition parameters, they will use the default transition time specified by the DTT Server model.
The DTT Server can then define a consistent transition time for all states on their elements, without depending on client configurations.

Configuration
=============

The following configuration parameters are associated with the Generic DTT Server model:

* :kconfig:option:`CONFIG_BT_MESH_DTT_SRV_PERSISTENT` - Control whether changes to the Generic Default Transition Time are stored persistently.

  .. note::
    This option is only available if :kconfig:option:`CONFIG_BT_SETTINGS` is enabled.

States
======

The Generic Level Server model contains the following state:

Generic Default Transition Time: ``int32_t``
    This state can have the following values:

    * `0` to perform the state change instantly or when no transition is planned
    * Positive number of milliseconds that defines the transition time
    * ``SYS_FOREVER_MS`` if the transition time is undefined

    On the air, the transition time is encoded into a single byte, and loses some of its granularity:

    * Step count: 6 bits (range `0x00` to `0x3e`)
    * Step resolution: 2 bits

        * Step 0: 100-millisecond resolution
        * Step 1: 1-second resolution
        * Step 2: 10-second resolution
        * Step 3: 10-minute resolution

    The state is encoded with the highest resolution available, and rounded to the nearest representation.
    Values lower than 100 milliseconds and higher than 0 are encoded as 100 milliseconds.
    Values higher than the max value of 620 minutes are encoded as "undefined".

    The DTT Server holds the memory for this state itself, and optionally notifies the user of any changes through :c:member:`bt_mesh_dtt_srv.update_handler`.
    If your application changes the transition time manually, the change must be published using :c:func:`bt_mesh_dtt_srv_pub`.

Extended models
===============

None.

Persistent storage
==================

The Generic Default Transition Time is stored persistently if :kconfig:option:`CONFIG_BT_MESH_DTT_SRV_PERSISTENT` is enabled.

API documentation
=================

| Header file: :file:`include/bluetooth/mesh/gen_dtt_srv.h`
| Source file: :file:`subsys/bluetooth/mesh/gen_dtt_srv.c`

.. doxygengroup:: bt_mesh_dtt_srv
   :project: nrf
   :members:
